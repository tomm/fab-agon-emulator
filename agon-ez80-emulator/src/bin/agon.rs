use agon_ez80_emulator::{ SerialLink, RamInit, AgonMachine, AgonMachineConfig, gpio };
use std::sync::mpsc;
use std::sync::mpsc::{Sender, Receiver};
use std::io::{ self, BufRead, Write };

fn send_bytes(tx: &Sender<u8>, msg: &Vec<u8>) {
    for b in msg {
        tx.send(*b).unwrap();
    }
}

fn send_keys(tx: &Sender<u8>, msg: &str) {
    for key in msg.as_bytes() {
        // cmd, len, keycode, modifiers, vkey, keydown
        // key down
        tx.send(0x81).unwrap();
        tx.send(4).unwrap();
        tx.send(*key).unwrap();
        tx.send(0).unwrap();
        tx.send(0).unwrap();
        tx.send(1).unwrap();

        // key up
        tx.send(0x81).unwrap();
        tx.send(4).unwrap();
        tx.send(*key).unwrap();
        tx.send(0).unwrap();
        tx.send(0).unwrap();
        tx.send(0).unwrap();
    }
}

// Fake VDP. Minimal for MOS to work, outputting to stdout */
fn handle_vdp(tx_to_ez80: &Sender<u8>, rx_from_ez80: &Receiver<u8>, vdp_terminal_mode: &mut bool) -> bool {
    match rx_from_ez80.try_recv() {
        Ok(data) => {
            match data {
                // one zero byte sent before everything else. real VDP ignores
                0 => {},
                1 => {},
                7 => {}, // bell
                9 => {}, // cursor right
                0xa => println!(),
                0xd => {},
                0x11 => {
                    rx_from_ez80.recv().unwrap();
                }, // color
                v if v >= 0x20 && v != 0x7f => {
                    //print!("\x1b[0m{}\x1b[90m", char::from_u32(data as u32).unwrap());
                    print!("{}", char::from_u32(data as u32).unwrap());
                }
                // VDP system control
                0x17 => {
                    match rx_from_ez80.recv().unwrap() {
                        // video
                        0 => {
                            match rx_from_ez80.recv().unwrap() {
                                // general poll. echo back the sent byte
                                0x80 => {
                                    let resp = rx_from_ez80.recv().unwrap();
                                    send_bytes(&tx_to_ez80, &vec![0x80, 1, resp]);
                                }
                                // video mode info
                                0x86 => {
                                    let w: u16 = 640;
                                    let h: u16 = 400;
                                    send_bytes(&tx_to_ez80, &vec![
                                       0x86, 7,
                                       (w & 0xff) as u8, ((w>>8) & 0xff) as u8,
                                       (h & 0xff) as u8, ((h>>8) & 0xff) as u8, 80, 25, 1
                                    ]);
                                }
                                // read RTC
                                0x87 => {
                                    let mode = rx_from_ez80.recv().unwrap();
                                    if mode == 0 {
                                        send_bytes(&tx_to_ez80, &vec![
                                            0x87, 6, 0, 0, 0, 0, 0, 0
                                        ]);
                                    } else {
                                        println!("unknown packet VDU 0x17, 0, 0x87, 0x{:x}", mode);
                                    }
                                }
                                0xff => {
                                    println!("ez80 request to enter VDP terminal mode.");
                                    *vdp_terminal_mode = true;
                                }
                                v => {
                                    println!("unknown packet VDU 0x17, 0, 0x{:x}", v);

                                }
                            }
                        }
                        v => {
                            println!("unknown packet VDU 0x17, 0x{:x}", v);
                        }
                    }
                }
                0x1e => {} // home cursor
                _ => {
                    println!("Unknown packet VDU 0x{:x}", data);//char::from_u32(data as u32).unwrap());
                }
            }
            std::io::stdout().flush().unwrap();
            true
        }
        Err(mpsc::TryRecvError::Disconnected) => panic!(),
        Err(mpsc::TryRecvError::Empty) => false
    }
}

fn start_vdp(tx_vdp_to_ez80: Sender<u8>, rx_ez80_to_vdp: Receiver<u8>,
             gpios: std::sync::Arc<std::sync::Mutex<gpio::GpioSet>>) {
    let (tx_stdin, rx_stdin): (Sender<String>, Receiver<String>) = mpsc::channel();

    // to avoid blocking on stdin, use a thread and channel to read from it
    let _stdin_thread = std::thread::spawn(move || {
        let stdin = io::stdin();
        for line in stdin.lock().lines() {
            tx_stdin.send(line.unwrap()).unwrap();
        }
    });

    println!("Tom\'s Fake VDP Version 1.03");

    let mut vdp_terminal_mode = false;
    let mut last_kb_input = std::time::Instant::now();
    let mut last_vsync = std::time::Instant::now();
    loop {
        if !handle_vdp(&tx_vdp_to_ez80, &rx_ez80_to_vdp, &mut vdp_terminal_mode) {
            // no packets from ez80. sleep a little
            std::thread::sleep(std::time::Duration::from_millis(1));
        }

        // a fake vsync every 16ms
        if last_vsync.elapsed() >= std::time::Duration::from_micros(16666) {
            // signal vsync to ez80 via GPIO (pin 1 (from 0) of GPIO port B)
            {
                let mut gpios = gpios.lock().unwrap();
                gpios.b.set_input_pin(1, true);
                gpios.b.set_input_pin(1, false);
            }

            last_vsync = last_vsync.checked_add(std::time::Duration::from_micros(16666)).unwrap_or(std::time::Instant::now());
        }

        // emit stdin input as keyboard events to the ez80, line by line,
        // with 1 second waits between lines
        if last_kb_input.elapsed() > std::time::Duration::from_secs(1) {
            match rx_stdin.try_recv() {
                Ok(line) => {
                    if vdp_terminal_mode {
                        for ch in line.as_bytes() {
                            tx_vdp_to_ez80.send(*ch).unwrap();
                            std::thread::sleep(std::time::Duration::from_micros(100));
                        }
                        tx_vdp_to_ez80.send(10).unwrap();
                    } else {
                        send_keys(&tx_vdp_to_ez80, &line);
                        send_keys(&tx_vdp_to_ez80, "\r");
                    }
                    last_kb_input = std::time::Instant::now();
                }
                Err(mpsc::TryRecvError::Disconnected) => {
                    // when stdin reaches EOF, terminate the emulator
                    std::process::exit(0);
                },
                Err(mpsc::TryRecvError::Empty) => {}
            }
        }
    }
}

pub struct DummySerialLink {}
impl SerialLink for DummySerialLink {
    fn send(&mut self, _byte: u8) {}
    fn recv(&mut self) -> Option<u8> { None }
    fn read_clear_to_send(&mut self) -> bool { true }
}

pub struct ChannelSerialLink {
    pub sender: Sender<u8>,
    pub receiver: Receiver<u8>
}
impl SerialLink for ChannelSerialLink {
    fn send(&mut self, byte: u8) { self.sender.send(byte).unwrap(); }
    fn recv(&mut self) -> Option<u8> {
        match self.receiver.try_recv() {
            Ok(data) => Some(data),
            Err(..) => None
        }
    }
    fn read_clear_to_send(&mut self) -> bool { true }
}

fn main() {
    let (tx_vdp_to_ez80, from_vdp): (Sender<u8>, Receiver<u8>) = mpsc::channel();
    let (to_vdp, rx_ez80_to_vdp): (Sender<u8>, Receiver<u8>) = mpsc::channel();
    let soft_reset = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let gpios = std::sync::Arc::new(std::sync::Mutex::new(gpio::GpioSet::new()));
    let gpios_ = gpios.clone();

    let mut unlimited_cpu = false;
    let mut sdcard_img = None;
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    while args.len() > 0 {
        let arg = args.remove(0);
        match arg.as_str() {
            "-u" | "--unlimited-cpu" => {
                unlimited_cpu = true;
            }
            "--sdcard-img" => {
                sdcard_img = if args.len() > 0 { Some(args.remove(0)) } else { None };
            }
            "--help" | "-h" | _ => {
                println!("Usage: agon [OPTIONS]");
                println!();
                println!("Options:");
                println!("  --sdcard-img <filename>  Use an SDcard image rather than hostfs");
                println!("  -u, --unlimited-cpu      Don't limit CPU to Agon Light 18.432MHz");
                std::process::exit(0);
            }
        }
    }

    let _cpu_thread = std::thread::spawn(move || {
        let mut machine = AgonMachine::new(AgonMachineConfig {
            ram_init: RamInit::Random,
            uart0_link: Box::new(ChannelSerialLink { sender: to_vdp, receiver: from_vdp }),
            uart1_link: Box::new(DummySerialLink {}),
            soft_reset,
            gpios: gpios_,
            clockspeed_hz: if unlimited_cpu { std::u64::MAX } else { 18_432_000 },
            mos_bin: std::path::PathBuf::from("../firmware/mos_console8.bin"),
        });

        if let Some(f) = sdcard_img {
            match std::fs::File::options().read(true).write(true).open(&f) {
                Ok(file) => machine.set_sdcard_image(Some(file)),
                Err(e) => {
                    eprintln!("Could not open sdcard image '{}': {:?}", f, e);
                    std::process::exit(-1);
                }
            }
        } else {
            machine.set_sdcard_directory(std::env::current_dir().unwrap().join("sdcard"));
        }

        machine.start(None);
    });

    start_vdp(tx_vdp_to_ez80, rx_ez80_to_vdp, gpios);
}
