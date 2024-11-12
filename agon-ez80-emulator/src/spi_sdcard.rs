use std::io::{ Seek, SeekFrom, Read, Write };

pub struct SpiSdcard {
    in_buf: Vec<u8>,
    out_buf: Vec<u8>,
    spi_sr: u8,
    check_pattern: u8,
    image: Option<std::fs::File>,
    next_write_sector: Option<usize>,
    next_write_started: bool,
}

impl SpiSdcard {
    pub fn new() -> Self {
        SpiSdcard {
            spi_sr: 0,
            in_buf: vec![],
            out_buf: vec![],
            check_pattern: 0,
            image: None,
            next_write_sector: None,
            next_write_started: false,
        }
    }

    pub fn set_image_file(&mut self, file: Option<std::fs::File>) {
        self.image = file;
    }

    pub fn recv_byte(&mut self, val: u8) {
        if let Some(image) = self.image.as_mut() {
            // 0x80 means transfer finished (immediate on the emulator)
            self.spi_sr = 0x80;

            if (self.next_write_sector.is_none() || !self.next_write_started) && val == 255 {
                // init sequence. ignore
            } else if self.next_write_sector.is_some() && !self.next_write_started && val == 254 {
                // 254 is the 'SD_START_TOKEN'
                self.next_write_started = true;
                return;
            } else {
                self.in_buf.push(val);
            }

            if self.next_write_started {
                if self.in_buf.len() == 512 {
                    let sector = self.next_write_sector.unwrap();
                    self.next_write_started = false;
                    self.next_write_sector = None;
                    //println!("WRITE!!!!!!!!! sector {}, {:?}", sector, self.in_buf);
                    match image.seek(SeekFrom::Start(sector as u64 * 512))
                        .and_then(|_| image.write_all(&self.in_buf)) {
                        Ok(_) => {},
                        Err(e) => {
                            eprintln!("Error reading SDcard image: {:?}", e);
                        }
                    };

                    self.in_buf.clear();
                    self.out_buf.push(5);
                    self.out_buf.push(1);
                }
                return;
            }

            //println!("SPI in buffer contains {:?}", self.in_buf);
            //println!("SPI out buffer len {}", self.out_buf.len());

            const CMD0: u8 = 0 | 0x40;
            const CMD8: u8 = 8 | 0x40;
            const CMD55: u8 = 55 | 0x40;
            const ACMD41: u8 = 41 | 0x40;
            const CMD58: u8 = 58 | 0x40;
            const CMD17: u8 = 17 | 0x40;
            const CMD24: u8 = 24 | 0x40;

            // Try to match a command
            if self.in_buf.len() >= 6 {
                match self.in_buf[..] {
                    // CMD0 (go to idle. part of init)
                    [CMD0, _a3, _a2, _a1, _a0, _crc, ..] => {
                        self.in_buf.drain(0..6);
                        self.out_buf.push(1); // OK 
                    }
                    // CMD8 (interface condition)
                    [CMD8, _a3, _a2, _a1, check_pattern, _crc, ..] => {
                        self.check_pattern = check_pattern;
                        self.in_buf.drain(0..6);
                        self.out_buf.extend_from_slice(
                            &[1, 0, 0, 1, check_pattern]
                        );
                    }
                    // CMD55
                    [CMD55, _a3, _a2, _a1, _a0, _crc, ..] => {
                        self.out_buf.push(1); // OK 
                        self.in_buf.drain(0..6);
                    }
                    // ACMD41
                    [ACMD41, _a3, _a2, _a1, _a0, _crc, ..] => {
                        self.out_buf.push(0); // In idle state
                        self.in_buf.drain(0..6);
                    }
                    // CMD58
                    [CMD58, _a3, _a2, _a1, _a0, _crc, ..] => {
                        self.out_buf.push(0); // v2.0+ standard capacity
                        self.in_buf.drain(0..6);
                    }
                    // CMD17
                    [CMD17, sec3, sec2, sec1, sec0, _crc, ..] => {
                        let sector = sec0 as usize + ((sec1 as usize)<<8) + ((sec2 as usize)<<16) + ((sec3 as usize)<<24);
                        //println!("GOT CMD17. read sector {}", sector);
                        self.in_buf.drain(0..6);
                        self.out_buf.push(0);
                        self.out_buf.push(0xfe);
                        let mut buf = [0u8; 512];
                        match image.seek(SeekFrom::Start(sector as u64 * 512))
                            .and_then(|_| image.read_exact(&mut buf)) {
                            Ok(_) => {},
                            Err(e) => {
                                eprintln!("Error reading SDcard image: {:?}", e);
                            }
                        };
                        self.out_buf.extend_from_slice(&buf);
                        // 2 crc bytes that mos ignores
                        self.out_buf.push(0);
                        self.out_buf.push(0);
                    }
                    // CMD24
                    [CMD24, sec3, sec2, sec1, sec0, _crc, ..] => {
                        let sector = sec0 as usize + ((sec1 as usize)<<8) + ((sec2 as usize)<<16) + ((sec3 as usize)<<24);
                        self.next_write_sector = Some(sector);
                        self.next_write_started = false;
                        self.in_buf.drain(0..6);
                        //println!("GOT CMD24. write sector {:?}", self.next_write_sector);
                        self.out_buf.push(0);
                    }
                    _ => {
                        eprintln!("Unknown SDcard command {:?}: {:?}", self.in_buf.get(0).and_then(|v| Some(*v & !0x40)), self.in_buf);
                        // drop the command
                        self.in_buf.clear();
                    }
                }
            }
        }
    }

    pub fn send_byte(&mut self) -> Option<u8> {
        if self.out_buf.is_empty() {
            None
        } else {
            Some(self.out_buf.remove(0))
        }
    }

    pub fn get_spi_status_register(&mut self) -> u8 {
        let sr = self.spi_sr;
        self.spi_sr = 0;
        sr
    }

    pub fn get_spi_control_register(&mut self) -> u8 {
        0
    }
}
