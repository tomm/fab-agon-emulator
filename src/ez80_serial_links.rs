use agon_cpu_emulator::SerialLink;

pub struct DummySerialLink {
    pub name: String
}

impl SerialLink for DummySerialLink {
    fn send(&mut self, byte: u8) {
        println!("{}_send: 0x{:02x}", self.name, byte);
    }
    fn recv(&mut self) -> Option<u8> { None }
}

pub struct Ez80ToVdpSerialLink {
    pub z80_send_to_vdp: libloading::Symbol<'static, unsafe extern fn(b: u8)>,
    pub z80_recv_from_vdp: libloading::Symbol<'static, unsafe extern fn(out: *mut u8) -> bool>,
}

impl SerialLink for Ez80ToVdpSerialLink {
    fn send(&mut self, byte: u8) {
        unsafe{(*self.z80_send_to_vdp)(byte)}
    }
    fn recv(&mut self) -> Option<u8> {
        let mut data: u8 = 0;
        unsafe {
            if !(*self.z80_recv_from_vdp)(&mut data as *mut u8) {
                None
            } else {
                Some(data)
            }
        }
    }
}

pub struct Ez80ToHostSerialLink {
    port: Box<dyn serialport::SerialPort>,
}

impl Ez80ToHostSerialLink {
    pub fn try_open(device: &str, baud: u32) -> Option<Self> {
        match serialport::new(device, baud).open() {
            Ok(port) => Some(Ez80ToHostSerialLink { port }),
            Err(e) => {
                println!("Failed to open device {}: {}", device, e);
                None
            }
        }
    }
}

impl SerialLink for Ez80ToHostSerialLink {
    fn send(&mut self, byte: u8) {
        if self.port.write(&[byte]).is_err() {
            println!("Error writing to {:?}", self.port);
        }
    }
    fn recv(&mut self) -> Option<u8> {
        match self.port.bytes_to_read() {
            Ok(n) => {
                if n > 0 {
                    let mut buf: [u8; 1] = [0];
                    if self.port.read_exact(&mut buf).is_ok() {
                        Some(buf[0])
                    } else {
                        None
                    }
                } else {
                    None
                }
            }
            Err(_) => None
        }
    }
}
