pub struct I2c {
    pub ctl: u8, // IO[0xcb]
}

impl I2c {
    pub fn new() -> Self {
        I2c { ctl: 0 }
    }

    pub fn is_interrupt_due(&self) -> bool {
        // IEN (interrupt enabled) && IFLG (interrupt flag)
        (self.ctl & 0x84) == 0x84
    }

    pub fn reset(&mut self) {
        self.ctl = 0;
    }

    pub fn get_sr(&self) -> u8 {
        // always return 'ACK not received'
        0x20 << 3
    }

    pub fn get_ctl(&self) -> u8 { self.ctl }

    pub fn set_ctl(&mut self, val: u8) {
        self.ctl = val;
        if val & 0x80 != 0 {
            // if interrupt is enabled, flag an interrupt immediately
            self.ctl |= 4;
        }
    }
}
