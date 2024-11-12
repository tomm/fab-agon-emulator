use ez80::*;
use std::collections::HashMap;
use std::io::{ Seek, SeekFrom, Read, Write };
use crate::{ mos, debugger, prt_timer, gpio, uart, spi_sdcard, i2c };
use std::sync::{ Arc, Mutex };
use rand::Rng;
use chrono::{ Datelike, Timelike };

const ROM_SIZE: usize = 0x20000; // 128 KiB flash
const EXTERNAL_RAM_SIZE: usize = 0x80000; // 512 KiB
const ONCHIP_RAM_SIZE: u32 = 0x2000; // 8KiB

pub enum RamInit {
    Zero,
    Random
}

pub struct AgonMachine {
    mem_external: [u8; EXTERNAL_RAM_SIZE], // 512k external RAM
    mem_rom: [u8; ROM_SIZE], // 128K ROM
    mem_internal: [u8; ONCHIP_RAM_SIZE as usize],  // 8K SRAM on the EZ80F92
    uart0: uart::Uart,
    uart1: uart::Uart,
    i2c: i2c::I2c,
    spi_sdcard: spi_sdcard::SpiSdcard,
    // map from MOS fatfs FIL struct ptr to rust File handle
    open_files: HashMap<u32, std::fs::File>,
    open_dirs: HashMap<u32, std::fs::ReadDir>,
    enable_hostfs: bool,
    mos_map: mos::MosMap,
    hostfs_root_dir: std::path::PathBuf,
    mos_current_dir: MosPath,
    soft_reset: Arc<std::sync::atomic::AtomicBool>,
    clockspeed_hz: u64,
    prt_timers: [prt_timer::PrtTimer; 6],
    gpios: Arc<Mutex<gpio::GpioSet>>,
    ram_init: RamInit,
    mos_bin: std::path::PathBuf,

    // memory map config
    onchip_mem_enable: bool,
    onchip_mem_segment: u8,
    flash_addr_u: u8,
    cs0_lbr: u8,
    cs0_ubr: u8,

    // last_pc and mem_out_of_bounds are used by the debugger
    pub last_pc: u32,
    pub mem_out_of_bounds: std::cell::Cell<Option<u32>>, // address
    pub paused: bool,
    pub cycle_counter: std::cell::Cell<u32>
}

// a path relative to the hostfs_root_dir
pub struct MosPath(std::path::PathBuf);

impl Machine for AgonMachine {
    #[inline]
    fn use_cycles(&self, cycles: u32) {
        self.cycle_counter.set(self.cycle_counter.get() + cycles);
    }

    fn peek(&self, address: u32) -> u8 {
        if let Some(onchip_ram_addr) = self.get_internal_ram_address(address) {
            self.use_cycles(1);
            self.mem_internal[onchip_ram_addr as usize]
        } else if let Some(rom_addr) = self.get_rom_address(address) {
            self.use_cycles(2);
            self.mem_rom[rom_addr as usize]
        } else if let Some(ram_addr) = self.get_external_ram_address(address) {
            self.use_cycles(1);
            self.mem_external[ram_addr as usize]
        } else {
            self.use_cycles(1);
            self.mem_out_of_bounds.set(Some(address));
            0xf5
        }
    }

    fn poke(&mut self, address: u32, value: u8) {
        self.use_cycles(1);

        if let Some(onchip_ram_addr) = self.get_internal_ram_address(address) {
            self.mem_internal[onchip_ram_addr as usize] = value;
        } else if let Some(rom_addr) = self.get_rom_address(address) {
            self.mem_rom[rom_addr as usize] = value;
        } else if let Some(ram_addr) = self.get_external_ram_address(address) {
            self.mem_external[ram_addr as usize] = value;
        } else {
            self.mem_out_of_bounds.set(Some(address));
        }
    }

    fn port_in(&mut self, address: u16) -> u8 {
        self.use_cycles(1);
        match address {
            0x80 => self.prt_timers[0].read_ctl(),
            0x81 => self.prt_timers[0].read_counter_low(),
            0x82 => self.prt_timers[0].read_counter_high(),

            0x83 => self.prt_timers[1].read_ctl(),
            0x84 => self.prt_timers[1].read_counter_low(),
            0x85 => self.prt_timers[1].read_counter_high(),

            0x86 => self.prt_timers[2].read_ctl(),
            0x87 => self.prt_timers[2].read_counter_low(),
            0x88 => self.prt_timers[2].read_counter_high(),

            0x89 => self.prt_timers[3].read_ctl(),
            0x8a => self.prt_timers[3].read_counter_low(),
            0x8b => self.prt_timers[3].read_counter_high(),

            0x8c => self.prt_timers[4].read_ctl(),
            0x8d => self.prt_timers[4].read_counter_low(),
            0x8e => self.prt_timers[4].read_counter_high(),

            0x8f => self.prt_timers[5].read_ctl(),
            0x90 => self.prt_timers[5].read_counter_low(),
            0x91 => self.prt_timers[5].read_counter_high(),

            0x9a => { let gpios = self.gpios.lock().unwrap(); gpios.b.get_dr() }
            0x9b => { let gpios = self.gpios.lock().unwrap(); gpios.b.get_ddr() }
            0x9c => { let gpios = self.gpios.lock().unwrap(); gpios.b.get_alt1() }
            0x9d => { let gpios = self.gpios.lock().unwrap(); gpios.b.get_alt2() }

            0x9e => {
                let gpios = self.gpios.lock().unwrap();
                /* set bit 3 of gpio if uart1 is NOT CTS */
                let uart1_cts = if self.uart1.read_modem_status_register() & 0x10 != 0 { 0 } else { 8 };
                gpios.c.get_dr() | uart1_cts
            }
            0x9f => { let gpios = self.gpios.lock().unwrap(); gpios.c.get_ddr() }
            0xa0 => { let gpios = self.gpios.lock().unwrap(); gpios.c.get_alt1() }
            0xa1 => { let gpios = self.gpios.lock().unwrap(); gpios.c.get_alt2() }

            0xa2 => {
                let gpios = self.gpios.lock().unwrap();
                /* set bit 3 of gpio if uart0 is NOT CTS */
                let uart0_cts = if self.uart0.read_modem_status_register() & 0x10 != 0 { 0 } else { 8 };
                gpios.d.get_dr() | uart0_cts
            }
            0xa3 => { let gpios = self.gpios.lock().unwrap(); gpios.d.get_ddr() }
            0xa4 => { let gpios = self.gpios.lock().unwrap(); gpios.d.get_alt1() }
            0xa5 => { let gpios = self.gpios.lock().unwrap(); gpios.d.get_alt2() }

            0xb4 => {
                if self.onchip_mem_enable { 0x80 } else { 0 }
            }

            0xb5 => {
                self.onchip_mem_segment
            }

            0xba => {
                // SPI control register
                //println!("Read SPI_CTL");
                self.spi_sdcard.get_spi_control_register()
            }

            0xbb => {
                // SPI status register
                //println!("Read SPI_SR");
                self.spi_sdcard.get_spi_status_register()
            }

            0xbc => {
                // SPI receive buffer register
                //println!("Read SPI_RBR");
                self.spi_sdcard.send_byte().unwrap_or(0xff)
            }

            0xc0 => {
                if self.uart0.is_access_brg_registers() {
                    self.uart0.brg_div as u8
                } else {
                    self.uart0.receive_byte()
                }
            }
            0xc1 => {
                if self.uart0.is_access_brg_registers() {
                    (self.uart0.brg_div >> 8) as u8
                } else {
                    self.uart0.ier
                }
            }
            0xc2 => {
                // UART0_IIR
                if self.uart0.ier & 0x02 != 0 {    
                    self.uart0.ier = self.uart0.ier & 0b11111101;
                    0x02   
                }
                else {    
                    0x04    
                }
            }
            0xc3 => self.uart0.lctl,
            0xc5 => self.uart0.read_lsr(),
            0xc6 => self.uart0.read_modem_status_register(),
            0xc7 => self.uart0.spr,

            // i2c
            0xcb => self.i2c.get_ctl(),
            0xcc => self.i2c.get_sr(),

            /* uart1 is kindof useless in the emulator, but ... */
            0xd0 => {
                if self.uart1.is_access_brg_registers() {
                    self.uart1.brg_div as u8
                } else {
                    self.uart1.receive_byte()
                }
            }
            0xd1 => {
                if self.uart1.is_access_brg_registers() {
                    (self.uart1.brg_div >> 8) as u8
                } else {
                    self.uart1.ier
                }
            }
            0xd2 => {
                // UART1_IIR
                if self.uart1.ier & 0x02 != 0 {    
                    self.uart1.ier = self.uart1.ier & 0b11111101;
                    0x02   
                }
                else {    
                    0x04    
                }
            }
            0xd3 => self.uart1.lctl,
            0xd5 => self.uart1.read_lsr(),
            0xd6 => self.uart1.read_modem_status_register(),
            0xd7 => self.uart1.spr,

            0xf7 => self.flash_addr_u,

            _ => {
                //println!("IN({:02X})", address);
                0
            }
        }
    }
    fn port_out(&mut self, address: u16, value: u8) {
        self.use_cycles(1);
        match address {
            0x80 => self.prt_timers[0].write_ctl(value),
            0x81 => self.prt_timers[0].write_reload_low(value),
            0x82 => self.prt_timers[0].write_reload_high(value),

            0x83 => self.prt_timers[1].write_ctl(value),
            0x84 => self.prt_timers[1].write_reload_low(value),
            0x85 => self.prt_timers[1].write_reload_high(value),

            0x86 => self.prt_timers[2].write_ctl(value),
            0x87 => self.prt_timers[2].write_reload_low(value),
            0x88 => self.prt_timers[2].write_reload_high(value),

            0x89 => self.prt_timers[3].write_ctl(value),
            0x8a => self.prt_timers[3].write_reload_low(value),
            0x8b => self.prt_timers[3].write_reload_high(value),

            0x8c => self.prt_timers[4].write_ctl(value),
            0x8d => self.prt_timers[4].write_reload_low(value),
            0x8e => self.prt_timers[4].write_reload_high(value),

            0x8f => self.prt_timers[5].write_ctl(value),
            0x90 => self.prt_timers[5].write_reload_low(value),
            0x91 => self.prt_timers[5].write_reload_high(value),

            0x9a => { let mut gpios = self.gpios.lock().unwrap(); gpios.b.set_dr(value); gpios.b.update(); }
            0x9b => { let mut gpios = self.gpios.lock().unwrap(); gpios.b.set_ddr(value); gpios.b.update(); }
            0x9c => { let mut gpios = self.gpios.lock().unwrap(); gpios.b.set_alt1(value); gpios.b.update(); }
            0x9d => { let mut gpios = self.gpios.lock().unwrap(); gpios.b.set_alt2(value); gpios.b.update(); }

            0x9e => { let mut gpios = self.gpios.lock().unwrap(); gpios.c.set_dr(value); gpios.c.update(); }
            0x9f => { let mut gpios = self.gpios.lock().unwrap(); gpios.c.set_ddr(value); gpios.c.update(); }
            0xa0 => { let mut gpios = self.gpios.lock().unwrap(); gpios.c.set_alt1(value); gpios.c.update(); }
            0xa1 => { let mut gpios = self.gpios.lock().unwrap(); gpios.c.set_alt2(value); gpios.c.update(); }

            0xa2 => { let mut gpios = self.gpios.lock().unwrap(); gpios.d.set_dr(value); gpios.d.update(); }
            0xa3 => { let mut gpios = self.gpios.lock().unwrap(); gpios.d.set_ddr(value); gpios.d.update(); }
            0xa4 => { let mut gpios = self.gpios.lock().unwrap(); gpios.d.set_alt1(value); gpios.d.update(); }
            0xa5 => { let mut gpios = self.gpios.lock().unwrap(); gpios.d.set_alt2(value); gpios.d.update(); }

            // chip selects
            0xa8 => self.cs0_lbr = value,
            0xa9 => self.cs0_ubr = value,
            0xaa => {} // CS0_CTL
            0xab => {} // CS1_LBR
            0xac => {} // CS1_UBR
            0xad => {} // CS1_CTL
            0xae => {} // CS2_LBR
            0xaf => {} // CS2_UBR
            0xb0 => {} // CS2_CTL
            0xb1 => {} // CS3_LBR
            0xb2 => {} // CS3_UBR
            0xb3 => {} // CS3_CTL

            0xb4 => {
                self.onchip_mem_enable = value & 0x80 != 0;
            }

            0xb5 => {
                self.onchip_mem_segment = value;
            }

            0xba => {
                // SPI control register
                //println!("Write SPI_CTL with 0x{:x}", value);
            }

            0xbb => {
                // SPI status register
                //println!("Write SPI_SR with 0x{:x}", value);
            }

            0xbc => {
                // SPI transmit shift register
                //println!("Write SPI_TSR with 0x{:x}", value);
                self.spi_sdcard.recv_byte(value)
            }

            /* UART0_REG_THR - send data to VDP */
            0xc0 => {
                if self.uart0.is_access_brg_registers() {
                    self.uart0.brg_div &= 0xff00;
                    self.uart0.brg_div |= value as u16;
                } else {
                    self.uart0.send_byte(value);
                }
            }
            0xc1 => {
                if self.uart0.is_access_brg_registers() {
                    self.uart0.brg_div &= 0xff;
                    self.uart0.brg_div |= (value as u16) << 8;
                } else {
                    //println!("setting uart ier: 0x{:x}", value);
                    self.uart0.ier = value;
                }
            }
            0xc2 => self.uart0.write_fctl(value),
            0xc3 => self.uart0.lctl = value,
            0xc7 => self.uart0.spr = value,

            // i2c
            0xcb => self.i2c.set_ctl(value),
            0xcc => {} // can't set status register
            0xcd => self.i2c.reset(),

            /* uart1 is kindof useless in the emulator, but ... */
            0xd0 => {
                if self.uart1.is_access_brg_registers() {
                    self.uart1.brg_div &= 0xff00;
                    self.uart1.brg_div |= value as u16;
                } else {
                    self.uart1.send_byte(value);
                }
            }
            0xd1 => {
                if self.uart1.is_access_brg_registers() {
                    self.uart1.brg_div &= 0xff;
                    self.uart1.brg_div |= (value as u16) << 8;
                } else {
                    //println!("setting uart ier: 0x{:x}", value);
                    self.uart1.ier = value;
                }
            }
            0xd2 => self.uart1.write_fctl(value),
            0xd3 => self.uart1.lctl = value,
            0xd7 => self.uart1.spr = value,

            0xf7 => self.flash_addr_u = value,

            _ => {
                //println!("OUT(${:02X}) = ${:x}", address, value);
            }
        }
    }
}

pub struct AgonMachineConfig {
    pub uart0_link: Box<dyn uart::SerialLink>,
    pub uart1_link: Box<dyn uart::SerialLink>,
    pub soft_reset: Arc<std::sync::atomic::AtomicBool>,
    pub clockspeed_hz: u64,
    pub ram_init: RamInit,
    pub mos_bin: std::path::PathBuf,
    pub gpios: Arc<Mutex<gpio::GpioSet>>,
}

impl AgonMachine {
    pub fn new(config: AgonMachineConfig) -> Self {
        AgonMachine {
            mem_rom: [0; ROM_SIZE],
            mem_external: [0; EXTERNAL_RAM_SIZE],
            mem_internal: [0; ONCHIP_RAM_SIZE as usize],
            uart0: uart::Uart::new(config.uart0_link),
            uart1: uart::Uart::new(config.uart1_link),
            i2c: i2c::I2c::new(),
            spi_sdcard: spi_sdcard::SpiSdcard::new(),
            open_files: HashMap::new(),
            open_dirs: HashMap::new(),
            enable_hostfs: true,
            mos_map: mos::MosMap::default(),
            hostfs_root_dir: std::env::current_dir().unwrap(),
            mos_current_dir: MosPath(std::path::PathBuf::new()),
            soft_reset: config.soft_reset,
            clockspeed_hz: config.clockspeed_hz,
            prt_timers: [
                prt_timer::PrtTimer::new(),
                prt_timer::PrtTimer::new(),
                prt_timer::PrtTimer::new(),
                prt_timer::PrtTimer::new(),
                prt_timer::PrtTimer::new(),
                prt_timer::PrtTimer::new(),
            ],
            gpios: config.gpios,
            ram_init: config.ram_init,
            last_pc: 0,
            mem_out_of_bounds: std::cell::Cell::new(None),
            cycle_counter: std::cell::Cell::new(0),
            paused: false,
            mos_bin: config.mos_bin,
            onchip_mem_enable: true,
            onchip_mem_segment: 0xff,
            flash_addr_u: 0,
            cs0_lbr: 0,
            cs0_ubr: 0xff
        }
    }

    #[inline]
    fn get_rom_address(&self, address: u32) -> Option<u32> {
        let a: u32 = address.wrapping_sub((self.flash_addr_u as u32) << 16);
        if a < ROM_SIZE as u32 { Some(a) } else { None }
    }

    #[inline]
    fn get_internal_ram_address(&self, address: u32) -> Option<u32> {
        if !self.onchip_mem_enable { return None };
        let offset = address.wrapping_sub(((self.onchip_mem_segment as u32) << 16) + 0xe000);
        if offset < ONCHIP_RAM_SIZE { Some(offset) } else { None }
    }

    #[inline]
    fn get_external_ram_address(&self, address: u32) -> Option<u32> {
        if address >> 16 >= self.cs0_lbr as u32 && address >> 16 <= self.cs0_ubr as u32 {
            let offset = (address & 0xffff) | (((address >> 16) & (self.cs0_ubr.wrapping_sub(self.cs0_lbr) as u32)) << 16);
            if offset < EXTERNAL_RAM_SIZE as u32 { Some(offset) } else { None }
        } else {
            None
        }
    }

    pub fn set_sdcard_directory(&mut self, path: std::path::PathBuf) {
        self.hostfs_root_dir = path;
    }

    pub fn set_sdcard_image(&mut self, file: Option<std::fs::File>) {
        self.enable_hostfs = file.is_none();
        self.spi_sdcard.set_image_file(file);
    }

    fn load_mos(&mut self) {
        let code = match std::fs::read(&self.mos_bin) {
            Ok(data) => data,
            Err(e) => {
                eprintln!("Error opening {}: {:?}", self.mos_bin.display(), e);
                std::process::exit(-1);
            }
        };
        
        for (i, e) in code.iter().enumerate() {
            self.mem_rom[i] = *e;
        }

        let mos_map = self.mos_bin.with_extension("map");

        match crate::symbol_map::read_zds_map_file(mos_map.to_str().unwrap()) {
            Ok(map) => {
                match mos::MosMap::from_symbol_map(map) {
                    Ok(mos_map) => {
                        self.mos_map = mos_map;
                    }
                    Err(e) => {
                        println!("Error reading {}. hostfs integration disabled: {}", mos_map.display(), e);
                        self.enable_hostfs = false;
                    }
                }
            }
            Err(e) => {
                println!("Error reading {}. hostfs integration disabled: {}", mos_map.display(), e);
                self.enable_hostfs = false;
            }
        }
    }

    fn hostfs_mos_f_getlabel(&mut self, cpu: &mut Cpu) {
        let mut buf = self._peek24(cpu.state.sp() + 6);
        let label = "hostfs";
        for b in label.bytes() {
            self.poke(buf, b);
            buf += 1;
        }
        self.poke(buf, 0);

        // success
        cpu.state.reg.set24(Reg16::HL, 0); // success

        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_getcwd(&mut self, cpu: &mut Cpu) {
        let mut buf = self._peek24(cpu.state.sp() + 3);
        let cwd = self.mos_current_dir.0.clone();
        self.poke(buf, 47); // leading slash
        buf += 1;
        for b in cwd.to_str().unwrap_or("").bytes() {
            self.poke(buf, b);
            buf += 1;
        }
        self.poke(buf, 0);

        // success
        cpu.state.reg.set24(Reg16::HL, 0); // success

        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_close(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);
        //eprintln!("f_close(${:x})", fptr);

        // closes on Drop
        self.open_files.remove(&fptr);

        // success
        cpu.state.reg.set24(Reg16::HL, 0);

        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_gets(&mut self, cpu: &mut Cpu) {
        let buf = self._peek24(cpu.state.sp() + 3);
        let max_len = self._peek24(cpu.state.sp() + 6);
        let fptr = self._peek24(cpu.state.sp() + 9);
        //eprintln!("f_gets(buf: ${:x}, len: ${:x}, fptr: ${:x})", buf, max_len, fptr);

        let fresult = 'outer: {
            if let Some(mut f) = self.open_files.get(&fptr) {
                let mut line = vec![];
                let mut host_buf = vec![0; 1];
                for _ in 0..max_len {
                    let n_read = match f.read(host_buf.as_mut_slice()) {
                        Ok(n_read) => n_read,
                        Err(_) => break 'outer mos::FR_DISK_ERR
                    };

                    if n_read == 0 {
                        break 'outer mos::FR_DISK_ERR // EOF
                    }
                    line.push(host_buf[0]);

                    if host_buf[0] == 10 || host_buf[0] == 0 { break; }
                }
                // no f.tell()...
                let fpos = match f.seek(SeekFrom::Current(0)) {
                    Ok(n) => n,
                    Err(_) => break 'outer mos::FR_DISK_ERR
                };

                // save file position to FIL.fptr U32
                self._poke24(fptr + mos::FIL_MEMBER_FPTR, fpos as u32);
                let mut buf2 = buf;
                for b in line {
                    self.poke(buf2, b);
                    buf2 += 1;
                }
                self.poke(buf2, 0);

                mos::FR_OK
            } else {
                mos::FR_DISK_ERR
            }
        };
        if fresult==mos::FR_OK {
            cpu.state.reg.set24(Reg16::HL, buf);
        }
        else {
            cpu.state.reg.set24(Reg16::HL, 0);
        }
        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_putc(&mut self, cpu: &mut Cpu) {
        let ch = self._peek24(cpu.state.sp() + 3);
        let fptr = self._peek24(cpu.state.sp() + 6);

        let fresult = 'outer: {
            if let Some(mut f) = self.open_files.get(&fptr) {
                if f.write(&[ch as u8]).is_err() {
                    break 'outer mos::FR_DISK_ERR;
                }

                // no f.tell()...
                let fpos = match f.seek(SeekFrom::Current(0)) {
                    Ok(n) => n,
                    Err(_) => break 'outer mos::FR_DISK_ERR
                };
                // save file position to FIL.fptr
                self._poke24(fptr + mos::FIL_MEMBER_FPTR, fpos as u32);

                mos::FR_OK
            } else {
                mos::FR_DISK_ERR
            }
        };

        cpu.state.reg.set24(Reg16::HL, fresult);
        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_write(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);
        let buf = self._peek24(cpu.state.sp() + 6);
        let num = self._peek24(cpu.state.sp() + 9);
        let num_written_ptr = self._peek24(cpu.state.sp() + 12);
        //eprintln!("f_write(${:x}, ${:x}, {}, ${:x})", fptr, buf, num, num_written_ptr);

        let fresult = 'outer: {
            if let Some(mut f) = self.open_files.get(&fptr) {
                for i in 0..num {
                    let byte = self.peek(buf + i);
                    if f.write(&[byte]).is_err() { break 'outer mos::FR_DISK_ERR }
                }

                // no f.tell()...
                let fpos = match f.seek(SeekFrom::Current(0)) {
                    Ok(n) => n,
                    Err(_) => break 'outer mos::FR_DISK_ERR
                };
                // save file position to FIL.fptr
                self._poke24(fptr + mos::FIL_MEMBER_FPTR, fpos as u32);

                // inform caller that all bytes were written
                self._poke24(num_written_ptr, num);

                mos::FR_OK
            } else {
                mos::FR_DISK_ERR
            }
        };

        cpu.state.reg.set24(Reg16::HL, fresult);
        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_read(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);
        let mut buf = self._peek24(cpu.state.sp() + 6);
        let len = self._peek24(cpu.state.sp() + 9);
        let bytes_read_ptr = self._peek24(cpu.state.sp() + 12);
        //eprintln!("f_read(${:x}, ${:x}, ${:x}, ${:x})", fptr, buf, len, bytes_read_ptr);

        let fresult = 'outer: {
            if let Some(mut f) = self.open_files.get(&fptr) {
                let mut host_buf: Vec<u8> = vec![0; len as usize];
                if let Ok(num_bytes_read) = f.read(host_buf.as_mut_slice()) {
                    // no f.tell()...
                    let fpos = match f.seek(SeekFrom::Current(0)) {
                        Ok(fpos) => fpos,
                        Err(_) => break 'outer mos::FR_DISK_ERR
                    };
                    // copy to agon ram 
                    for b in host_buf {
                        self.poke(buf, b);
                        buf += 1;
                    }
                    // save file position to FIL.fptr
                    self._poke24(fptr + mos::FIL_MEMBER_FPTR, fpos as u32);
                    // save num bytes read
                    self._poke24(bytes_read_ptr, num_bytes_read as u32);

                    mos::FR_OK
                } else {
                    mos::FR_DISK_ERR
                }
            } else {
                mos::FR_DISK_ERR
            }
        };

        cpu.state.reg.set24(Reg16::HL, fresult);
        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_mos_f_closedir(&mut self, cpu: &mut Cpu) {
        let dir_ptr = self._peek24(cpu.state.sp() + 3);
        // closes on Drop
        self.open_dirs.remove(&dir_ptr);

        // success
        cpu.state.reg.set24(Reg16::HL, 0); // success

        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn hostfs_set_filinfo_from_metadata(&mut self, z80_filinfo_ptr: u32, path: &std::path::PathBuf, metadata: &std::fs::Metadata) {
        // XXX to_str can fail if not utf-8
        // write file name
        z80_mem_tools::memcpy_to_z80(
            self, z80_filinfo_ptr + mos::FILINFO_MEMBER_FNAME_256BYTES,
            path.file_name().unwrap().to_str().unwrap().as_bytes()
        );

        // write file length (U32)
        self._poke24(z80_filinfo_ptr + mos::FILINFO_MEMBER_FSIZE_U32, metadata.len() as u32);
        self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FSIZE_U32 + 3, (metadata.len() >> 24) as u8);

        // is directory?
        if metadata.is_dir() {
            self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FATTRIB_U8, 0x10 /* AM_DIR */);
        }

        // set fdate, ftime
        let mut fil_date = 0u16;
        let mut fil_time = 0u16;

        let raw_mtime = filetime::FileTime::from_last_modification_time(&metadata);
        if let Some(utc_mtime) = chrono::DateTime::from_timestamp(raw_mtime.unix_seconds(), 0) {
            let local_mtime = utc_mtime.with_timezone(&chrono::Local::now().timezone());
            fil_date |= ((local_mtime.year() - 1980) << 9) as u16;
            fil_date |= ((local_mtime.month()) << 5) as u16;
            fil_date |= local_mtime.day() as u16;
            fil_time |= (local_mtime.hour() << 11) as u16;
            fil_time |= (local_mtime.minute() << 5) as u16;
            fil_time |= local_mtime.second() as u16;
        }

        self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FDATE_U16, fil_date as u8);
        self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FDATE_U16 + 1, (fil_date >> 8) as u8);
        self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FTIME_U16, fil_time as u8);
        self.poke(z80_filinfo_ptr + mos::FILINFO_MEMBER_FTIME_U16 + 1, (fil_time >> 8) as u8);
    }

    fn hostfs_mos_f_readdir(&mut self, cpu: &mut Cpu) {
        let dir_ptr = self._peek24(cpu.state.sp() + 3);
        let file_info_ptr = self._peek24(cpu.state.sp() + 6);

        // clear the FILINFO struct
        z80_mem_tools::memset(self, file_info_ptr, 0, mos::SIZEOF_MOS_FILINFO_STRUCT);

        match self.open_dirs.get_mut(&dir_ptr) {
            Some(dir) => {

                match dir.next() {
                    Some(Ok(dir_entry)) => {
                        let path = dir_entry.path();
                        if let Ok(metadata) = std::fs::metadata(&path) {
                            self.hostfs_set_filinfo_from_metadata(file_info_ptr, &path, &metadata);

                            // success
                            cpu.state.reg.set24(Reg16::HL, 0);
                        } else {
                            // hm. why might std::fs::metadata fail?
                            z80_mem_tools::memcpy_to_z80(
                                self, file_info_ptr + mos::FILINFO_MEMBER_FNAME_256BYTES,
                                "<error reading file metadata>".as_bytes()
                            );
                            cpu.state.reg.set24(Reg16::HL, 0);
                        }
                    }
                    Some(Err(_)) => {
                        cpu.state.reg.set24(Reg16::HL, 1); // error
                    }
                    None => {
                        // directory has been read to the end.
                        // do nothing, since FILINFO.fname[0] == 0 indicates to MOS that
                        // the directory end has been reached

                        // success
                        cpu.state.reg.set24(Reg16::HL, 0);
                    }
                }
            }
            None => {
                cpu.state.reg.set24(Reg16::HL, 1); // error
            }
        }

        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_mkdir(&mut self, cpu: &mut Cpu) {
        let dir_name = unsafe {
            String::from_utf8_unchecked(mos::get_mos_path_string(self, self._peek24(cpu.state.sp() + 3)))
        };
        //eprintln!("f_mkdir(\"{}\")", dir_name);

        match std::fs::create_dir(self.host_path_from_mos_path_join(&dir_name)) {
            Ok(_) => {
                // success
                cpu.state.reg.set24(Reg16::HL, 0);
            }
            Err(_) => {
                // error
                cpu.state.reg.set24(Reg16::HL, 1);
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_rename(&mut self, cpu: &mut Cpu) {
        let old_name = unsafe {
            String::from_utf8_unchecked(mos::get_mos_path_string(self, self._peek24(cpu.state.sp() + 3)))
        };
        let new_name = unsafe {
            String::from_utf8_unchecked(mos::get_mos_path_string(self, self._peek24(cpu.state.sp() + 6)))
        };
        //eprintln!("f_rename(\"{}\", \"{}\")", old_name, new_name);

        match std::fs::rename(self.host_path_from_mos_path_join(&old_name),
                              self.host_path_from_mos_path_join(&new_name)) {
            Ok(_) => {
                // success
                cpu.state.reg.set24(Reg16::HL, 0);
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => {
                        cpu.state.reg.set24(Reg16::HL, 4);
                    }
                    _ => {
                        cpu.state.reg.set24(Reg16::HL, 1);
                    }
                }
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_chdir(&mut self, cpu: &mut Cpu) {
        let cd_to_ptr = self._peek24(cpu.state.sp() + 3);
        let cd_to = unsafe {
            // MOS filenames may not be valid utf-8
            String::from_utf8_unchecked(mos::get_mos_path_string(self, cd_to_ptr))
        };
        //eprintln!("f_chdir({})", cd_to);

        let new_path = self.mos_path_join(&cd_to);

        match std::fs::metadata(self.mos_path_to_host_path(&new_path)) {
            Ok(metadata) => {
                if metadata.is_dir() {
                    //eprintln!("setting path to {:?}", &new_path);
                    self.mos_current_dir = new_path;
                    cpu.state.reg.set24(Reg16::HL, 0);
                } else {
                    cpu.state.reg.set24(Reg16::HL, 1);
                }
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => {
                        cpu.state.reg.set24(Reg16::HL, 4);
                    }
                    _ => {
                        cpu.state.reg.set24(Reg16::HL, 1);
                    }
                }
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_mount(&mut self, cpu: &mut Cpu) {
        // always success. hostfs is mounted
        cpu.state.reg.set24(Reg16::HL, 0); // ok
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_unlink(&mut self, cpu: &mut Cpu) {
        let filename_ptr = self._peek24(cpu.state.sp() + 3);
        let filename = unsafe {
            String::from_utf8_unchecked(mos::get_mos_path_string(self, filename_ptr))
        };
        //eprintln!("f_unlink(\"{}\")", filename);

        match std::fs::remove_file(self.host_path_from_mos_path_join(&filename)) {
            Ok(()) => {
                cpu.state.reg.set24(Reg16::HL, 0); // ok
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => {
                        cpu.state.reg.set24(Reg16::HL, 4);
                    }
                    _ => {
                        cpu.state.reg.set24(Reg16::HL, 1);
                    }
                }
            }
        };

        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_opendir(&mut self, cpu: &mut Cpu) {
        //fr = f_opendir(&dir, path);
        let dir_ptr = self._peek24(cpu.state.sp() + 3);
        let path_ptr = self._peek24(cpu.state.sp() + 6);
        let path = unsafe {
            // MOS filenames may not be valid utf-8
            String::from_utf8_unchecked(mos::get_mos_path_string(self, path_ptr))
        };
        //eprintln!("f_opendir(${:x}, \"{}\")", dir_ptr, path.trim_end());

        match std::fs::read_dir(self.host_path_from_mos_path_join(&path)) {
            Ok(dir) => {
                // XXX should clear the DIR struct in z80 ram
                
                // store in map of z80 DIR ptr to rust ReadDir
                self.open_dirs.insert(dir_ptr, dir);
                cpu.state.reg.set24(Reg16::HL, 0); // ok
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => {
                        cpu.state.reg.set24(Reg16::HL, 4);
                    }
                    _ => {
                        cpu.state.reg.set24(Reg16::HL, 1);
                    }
                }
            }
        }

        cpu.state.reg.set24(Reg16::HL, 0); // ok
        let mut env = Environment::new(&mut cpu.state, self);
        env.subroutine_return();
    }

    fn mos_path_to_host_path(&mut self, path: &MosPath) -> std::path::PathBuf {
        self.hostfs_root_dir.join(&path.0)
    }

    /**
     * Return a new MosPath, `new_fragments` joined to mos_current_dir
     */
    fn mos_path_join(&mut self, new_fragments: &str) -> MosPath {
        let mut full_path = match new_fragments.get(0..1) {
            Some("/") | Some("\\") => std::path::PathBuf::new(),
            _ => self.mos_current_dir.0.clone()
        };

        for fragment in new_fragments.split(|c| c == '/' || c == '\\') {
            match fragment {
                "" | "." => {}
                ".." => {
                    full_path.pop();
                }
                "/" => {
                    full_path = std::path::PathBuf::new();
                }
                f => {
                    let abs_path = self.hostfs_root_dir.join(&full_path);
                    
                    // look for a case-insensitive match for this path fragment
                    let matched_f = match std::fs::read_dir(abs_path) {
                        Ok(dir) => {
                            if let Some(ci_f) = dir.into_iter().find(|item| {
                                match item {
                                    Ok(dir_entry) => dir_entry.file_name().to_ascii_lowercase().into_string() == Ok(f.to_ascii_lowercase()),
                                    Err(_) => false
                                }
                            }) {
                                // found a case-insensitive match
                                ci_f.unwrap().file_name()
                            } else {
                                std::ffi::OsString::from(f)
                            }
                        }
                        Err(_) => {
                            std::ffi::OsString::from(f)
                        }
                    };

                    full_path.push(matched_f);
                }
            }
        }

        MosPath(full_path)
    }

    fn host_path_from_mos_path_join(&mut self, new_fragments: &str) -> std::path::PathBuf {
        let rel_path = self.mos_path_join(new_fragments);
        self.mos_path_to_host_path(&rel_path)
    }

    fn hostfs_mos_f_truncate(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);

        // truncate from current file position cursor
        match self.open_files.get(&fptr) {
            Some(mut f) => {
                match f.seek(SeekFrom::Current(0)) {
                    Ok(pos) => {
                        match f.set_len(pos) {
                            Ok(_) => {
                                // store new file len in fatfs FIL structure
                                self._poke24(fptr + mos::FIL_MEMBER_OBJSIZE, pos as u32);

                                // success
                                cpu.state.reg.set24(Reg16::HL, 0);
                            }
                            Err(_) => {
                                cpu.state.reg.set24(Reg16::HL, 1); // error
                            }
                        }
                    }
                    Err(_) => {
                        cpu.state.reg.set24(Reg16::HL, 1); // error
                    }
                }
            }
            None => {
                cpu.state.reg.set24(Reg16::HL, 1); // error
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_lseek(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);
        let offset = self._peek24(cpu.state.sp() + 6);

        //eprintln!("f_lseek(${:x}, {})", fptr, offset);

        match self.open_files.get(&fptr) {
            Some(mut f) => {
                match f.seek(SeekFrom::Start(offset as u64)) {
                    Ok(pos) => {
                        // save file position to FIL.fptr
                        self._poke24(fptr + mos::FIL_MEMBER_FPTR, pos as u32);
                        // success
                        cpu.state.reg.set24(Reg16::HL, 0);
                    }
                    Err(_) => {
                        cpu.state.reg.set24(Reg16::HL, 1); // error
                    }
                }
            }
            None => {
                cpu.state.reg.set24(Reg16::HL, 1); // error
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_stat(&mut self, cpu: &mut Cpu) {
        let path_str = {
            let ptr = self._peek24(cpu.state.sp() + 3);
            unsafe {
                String::from_utf8_unchecked(mos::get_mos_path_string(self, ptr))
            }
        };
        let filinfo_ptr = self._peek24(cpu.state.sp() + 6);
        let path = self.host_path_from_mos_path_join(&path_str);
        //eprintln!("f_stat(\"{}\", ${:x})", path_str, filinfo_ptr);

        match std::fs::metadata(&path) {
            Ok(metadata) => {
                // clear the FILINFO struct
                z80_mem_tools::memset(self, filinfo_ptr, 0, mos::SIZEOF_MOS_FILINFO_STRUCT);

                self.hostfs_set_filinfo_from_metadata(filinfo_ptr, &path, &metadata);

                // success
                cpu.state.reg.set24(Reg16::HL, 0);
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => {
                        cpu.state.reg.set24(Reg16::HL, 4);
                    }
                    _ => {
                        cpu.state.reg.set24(Reg16::HL, 1);
                    }
                }
            }
        }

        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    fn hostfs_mos_f_open(&mut self, cpu: &mut Cpu) {
        let fptr = self._peek24(cpu.state.sp() + 3);
        let filename = {
            let ptr = self._peek24(cpu.state.sp() + 6);
            // MOS filenames may not be valid utf-8
            unsafe {
                String::from_utf8_unchecked(mos::get_mos_path_string(self, ptr))
            }
        };
        let path = self.mos_path_join(&filename);
        let mode = self._peek24(cpu.state.sp() + 9);
        if let Ok(metadata) = std::fs::metadata(self.mos_path_to_host_path(&path)) {
            if !metadata.is_file() {
                cpu.state.reg.set24(Reg16::HL, 4);
                Environment::new(&mut cpu.state, self).subroutine_return();
                return
            }
        }
        //eprintln!("f_open(${:x}, \"{}\", {})", fptr, &filename, mode);
        match std::fs::File::options()
            .read(true)
            .write(mode & mos::FA_WRITE != 0)
            .create((mode & mos::FA_CREATE_NEW != 0) || (mode & mos::FA_CREATE_ALWAYS != 0))
            .truncate(mode & mos::FA_CREATE_ALWAYS != 0)
            .open(self.mos_path_to_host_path(&path)) {
            Ok(mut f) => {
                // wipe the FIL structure
                z80_mem_tools::memset(self, fptr, 0, mos::SIZEOF_MOS_FIL_STRUCT);

                // save the size in the FIL structure
                let mut file_len = f.seek(SeekFrom::End(0)).unwrap();
                f.seek(SeekFrom::Start(0)).unwrap();

                // XXX don't support files larger than 512KiB
                file_len = file_len.min(1<<19);

                // store file len in fatfs FIL structure
                self._poke24(fptr + mos::FIL_MEMBER_OBJSIZE, file_len as u32);
                
                // store mapping from MOS *FIL to rust File
                self.open_files.insert(fptr, f);

                cpu.state.reg.set24(Reg16::HL, 0); // ok
            }
            Err(e) => {
                match e.kind() {
                    std::io::ErrorKind::NotFound => cpu.state.reg.set24(Reg16::HL, 4),
                    _ => cpu.state.reg.set24(Reg16::HL, 1)
                }
            }
        }
        Environment::new(&mut cpu.state, self).subroutine_return();
    }

    #[inline]
    // returns cycles elapsed
    pub fn execute_instruction(&mut self, cpu: &mut Cpu) -> u32 {
        let pc = cpu.state.pc();
        // remember PC before instruction executes. the debugger uses this
        // when out-of-bounds memory accesses happen (since they can't be
        // trapped mid-execution)
        self.last_pc = pc;

        if self.enable_hostfs && pc < 0x20000 && self.flash_addr_u == 0 {
            if pc == self.mos_map.f_close { self.hostfs_mos_f_close(cpu); }
            if pc == self.mos_map.f_gets { self.hostfs_mos_f_gets(cpu); }
            if pc == self.mos_map.f_read { self.hostfs_mos_f_read(cpu); }
            if pc == self.mos_map.f_open { self.hostfs_mos_f_open(cpu); }
            if pc == self.mos_map.f_write { self.hostfs_mos_f_write(cpu); }
            if pc == self.mos_map.f_chdir { self.hostfs_mos_f_chdir(cpu); }
            if pc == self.mos_map.f_closedir { self.hostfs_mos_f_closedir(cpu); }
            if pc == self.mos_map.f_getlabel { self.hostfs_mos_f_getlabel(cpu); }
            if pc == self.mos_map.f_getcwd { self.hostfs_mos_f_getcwd(cpu); }
            if pc == self.mos_map.f_lseek { self.hostfs_mos_f_lseek(cpu); }
            if pc == self.mos_map.f_mkdir { self.hostfs_mos_f_mkdir(cpu); }
            if pc == self.mos_map.f_mount { self.hostfs_mos_f_mount(cpu); }
            if pc == self.mos_map.f_opendir { self.hostfs_mos_f_opendir(cpu); }
            if pc == self.mos_map.f_putc { self.hostfs_mos_f_putc(cpu); }
            if pc == self.mos_map.f_readdir { self.hostfs_mos_f_readdir(cpu); }
            if pc == self.mos_map.f_rename { self.hostfs_mos_f_rename(cpu); }
            if pc == self.mos_map.f_stat { self.hostfs_mos_f_stat(cpu); }
            if pc == self.mos_map.f_unlink { self.hostfs_mos_f_unlink(cpu); }
            if pc == self.mos_map.f_truncate { self.hostfs_mos_f_truncate(cpu); }
            // never referenced in MOS
            //if pc == self.mos_map._f_puts { eprintln!("Un-trapped fatfs call: f_puts"); }
            //if pc == self.mos_map._f_setlabel { eprintln!("Un-trapped fatfs call: f_setlabel"); }
            //if pc == self.mos_map._f_chdrive { eprintln!("Un-trapped fatfs call: f_chdrive"); }
            //if pc == self.mos_map._f_getfree { eprintln!("Un-trapped fatfs call: f_getfree"); }
            //if pc == self.mos_map._f_printf { eprintln!("Un-trapped fatfs call: f_printf"); }
            //if pc == self.mos_map._f_sync { eprintln!("Un-trapped fatfs call: f_sync"); }
            //if pc == self.mos_map._f_truncate { eprintln!("Un-trapped fatfs call: f_truncate"); }
        }

        self.cycle_counter.set(0);
        cpu.execute_instruction(self);
        let cycles_elapsed = self.cycle_counter.get();
        //println!("{:2} cycles, {:?}", cycles_elapsed, ez80::disassembler::disassemble(self, cpu, None, pc, pc+1));

        for t in &mut self.prt_timers {
            t.apply_ticks(cycles_elapsed as u16);
        }

        self.uart0.apply_ticks(cycles_elapsed as i32);

        cycles_elapsed
    }

    #[inline]
    pub fn fire_gpio_interrupts(&mut self, cpu: &mut Cpu, vector_base: u32, interrupts_due: u8) -> bool {
        if interrupts_due != 0 {
            let mut env = Environment::new(&mut cpu.state, self);
            for pin in 0..=7 {
                if interrupts_due & (1<<pin) != 0 {
                    env.interrupt(vector_base + 2*pin);
                    return true;
                }
            }
        }
        false
    }

    #[inline]
    pub fn do_interrupts(&mut self, cpu: &mut Cpu) {
        // Not an interrupt. Is a soft-reset pending?
        if cpu.state.instructions_executed & 0xff == 0 {
            // perform a soft reset if requested
            if self.soft_reset.load(std::sync::atomic::Ordering::Relaxed) {
                // MOS soft reset code always runs from ADL mode.
                // and set_pc(0) will actually set pc := (mb<<16 + 0) in non-adl mode
                cpu.state.reg.adl = true;
                cpu.state.reg.madl = true;
                cpu.state.set_pc(0);
                self.soft_reset.store(false, std::sync::atomic::Ordering::Relaxed);
                return;
            }
        }

        if cpu.state.instructions_executed & 0x3f == 0 && cpu.state.reg.get_iff1() {
            // Interrupts in priority order
            for i in 0..self.prt_timers.len() {
                if self.prt_timers[i].irq_due() {
                    Environment::new(&mut cpu.state, self).interrupt(0xa + 2*(i as u32));
                    return;
                }
            }

            // fire uart interrupt
            if self.uart0.is_rx_interrupt_enabled() && self.uart0.maybe_fill_rx_buf() != None || // character(s) received
               self.uart0.ier & 0x02 != 0 { // character(s) to send
                let mut env = Environment::new(&mut cpu.state, self);
                //println!("uart interrupt!");
                env.interrupt(0x18); // uart0_handler
                return;
            }

            if self.i2c.is_interrupt_due() {
                let mut env = Environment::new(&mut cpu.state, self);
                //println!("i2c interrupt!");
                env.interrupt(0x1c);
                return;
            }

            // fire gpio interrupts
            {
                let (b_int, c_int, d_int) = {
                    let mut gpios = self.gpios.lock().unwrap();
                    let b_int = gpios.b.get_interrupt_due();
                    let c_int = gpios.c.get_interrupt_due();
                    let d_int = gpios.d.get_interrupt_due();
                    (b_int, c_int, d_int)
                };

                if self.fire_gpio_interrupts(cpu, 0x30, b_int) {
                    return;
                }
                if self.fire_gpio_interrupts(cpu, 0x40, c_int) {
                    return;
                }
                if self.fire_gpio_interrupts(cpu, 0x50, d_int) {
                    return;
                }
            }
        }
    }

    #[inline]
    fn debugger_tick(&mut self, debugger: &mut Option<debugger::DebuggerServer>, cpu: &mut Cpu) {
        if let Some(ref mut ds) = debugger {
            ds.tick(self, cpu);
        }
    }

    pub fn start(&mut self, debugger_con: Option<debugger::DebuggerConnection>) {
        let mut cpu = Cpu::new_ez80();

        let mut debugger = if debugger_con.is_some() {
            self.paused = true;
            Some(debugger::DebuggerServer::new(debugger_con.unwrap()))
        } else {
            None
        };

        match self.ram_init {
            RamInit::Random => {
                for i in 0x40000..0xc0000 {
                    self.poke(i, rand::thread_rng().gen_range(0..=255));
                }

                for i in 0..ONCHIP_RAM_SIZE {
                    self.mem_internal[i as usize] = rand::thread_rng().gen_range(0..=255);
                }
            }
            RamInit::Zero => {}
        }

        self.load_mos();

        cpu.state.set_pc(0);
        //cpu.set_trace(true);

        let cycles_per_ms: u64 = self.clockspeed_hz / 1000;
        let mut timeslice_start = std::time::Instant::now();
        loop {
            // in unlimited CPU mode, this inner loop never exits
            let mut cycle: u64 = 0;
            while cycle < cycles_per_ms {
                self.debugger_tick(&mut debugger, &mut cpu);
                if self.paused { break; }
                self.do_interrupts(&mut cpu);
                cycle += self.execute_instruction(&mut cpu) as u64;
            }

            while timeslice_start.elapsed() < std::time::Duration::from_millis(1) {
                std::thread::sleep(std::time::Duration::from_micros(500));
            }
            timeslice_start = timeslice_start.checked_add(
                std::time::Duration::from_millis(1)
            ).unwrap_or(std::time::Instant::now());
        }
    }
}
