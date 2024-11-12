pub struct GpioSet {
    pub b: Gpio,
    pub c: Gpio,
    pub d: Gpio
}

impl GpioSet {
    pub fn new() -> Self {
        GpioSet {
            b: Gpio::new(),
            c: Gpio::new(),
            d: Gpio::new(),
        }
    }
}

#[derive(Debug)]
pub struct Gpio {
    io_level: u8,
    interrupt_due: u8, // bitmap of 8 gpio pins
    dr: u8,

    pub ddr: u8,
    pub alt1: u8,
    pub alt2: u8,
}

impl Gpio {
    pub fn new() -> Self {
        Gpio {
            io_level: 0, dr: 0, ddr: 0xff, alt1: 0, alt2: 0, interrupt_due: 0
        }
    }

    pub fn update(&mut self) {
        let level = self.get_output_level();
        self.set_input_pins(level);
    }

    pub fn get_interrupt_due(&mut self) -> u8 { self.interrupt_due }

    // Note these aren't the mode numbers zilog docs use, but these made more sense
    // - pin 0..=7
    pub fn get_mode(&self, pin: u8) -> u8 {
        let mask = 1<<pin;
        let mode = if self.dr & mask != 0 { 1 } else { 0 } +
                   if self.ddr & mask != 0 { 2 } else { 0 } +
                   if self.alt1 & mask != 0 { 4 } else { 0 } +
                   if self.alt2 & mask != 0 { 8 } else { 0 };
        assert!(mode < 16);
        mode
    }
    
    pub fn get_ddr(&self) -> u8 { self.ddr }
    pub fn get_alt1(&self) -> u8 { self.alt1 }
    pub fn get_alt2(&self) -> u8 { self.alt2 }
    pub fn set_ddr(&mut self, val: u8) {
        let modified = self.ddr ^ val;
        self.ddr = val;
        self.interrupt_due &= !modified;
    }
    pub fn set_alt1(&mut self, val: u8) {
        let modified = self.ddr ^ val;
        self.alt1 = val;
        self.interrupt_due &= !modified;
    }
    pub fn set_alt2(&mut self, val: u8) {
        let modified = self.ddr ^ val;
        self.alt2 = val;
        self.interrupt_due &= !modified;
    }

    pub fn get_dr(&self) -> u8 {
        self.io_level
    }

    pub fn set_dr(&mut self, dr: u8) {
        for pin in 0..=7 {
            let mode = self.get_mode(pin);
            let mask = 1<<pin;
            match mode {
                // output
                0 | 1 |
                // open drain output / IO
                4 | 5 |
                // open source IO / output
                6 | 7 => {
                    if dr & mask == 0 {
                        self.io_level &= !mask;
                        self.dr &= !mask;
                    } else {
                        self.io_level |= mask;
                        self.dr |= mask;
                    }
                }
                // input
                2 | 3 => {
                    if dr & mask == 0 {
                        self.dr &= !mask;
                    } else {
                        self.dr |= mask;
                    }
                }
                // reserved
                8 => {}
                // interrupt - dual edge triggered
                9 => {
                    if dr & mask != 0 {
                        self.interrupt_due &= !mask;
                    }
                }
                // Port B, C, or D—alternate function controls port I/O
                10 | 11 => {}
                // Interrupt—active Low
                12 => {
                    if dr & mask != 0 {
                        self.interrupt_due &= !(self.io_level & mask);
                    }
                }
                // Interrupt—active High
                13 => {
                    if dr & mask != 0 {
                        self.interrupt_due &= !(!self.io_level & mask);
                    }
                }
                // Interrupt—falling edge triggered
                14 => {
                    if dr & mask != 0 {
                        self.interrupt_due &= !mask;
                    }
                }
                // Interrupt—rising edge triggered
                15 => {
                    if dr & mask != 0 {
                        self.interrupt_due &= !mask;
                    }
                }
                _ => panic!()
            }
        }
    }

    pub fn get_output_level(&mut self) -> u8 { self.io_level }

    // pin: [0..7]
    pub fn set_input_pin(&mut self, pin: u8, state: bool) {
        let level = self.get_output_level();
        let bit = 1<<pin;
        self.set_input_pins(if state { level | bit } else { level & !bit });
    }

    pub fn set_input_pins(&mut self, levels: u8) {
        let old_levels = self.io_level;
        self.io_level = levels;

        for pin in 0..=7 {
            let mode = self.get_mode(pin);
            let mask = 1<<pin;
            match mode {
                // output
                0 | 1 => {}
                // input
                2 | 3 => {}
                // open drain output / IO
                4 | 5 => {}
                // open source IO / output
                6 | 7 => {}
                // reserved
                8 => {}
                // interrupt - dual edge triggered
                9 => {
                    self.interrupt_due |= mask & (old_levels ^ levels);
                }
                // Port B, C, or D—alternate function controls port I/O
                10 | 11 => {}
                // Interrupt—active Low
                12 => {
                    self.interrupt_due |= mask & !levels;
                }
                // Interrupt—active High
                13 => {
                    self.interrupt_due |= mask & levels;
                }
                // Interrupt—falling edge triggered
                14 => {
                    self.interrupt_due |= mask & ((old_levels ^ levels) & !levels);
                }
                // Interrupt—rising edge triggered
                15 => {
                    self.interrupt_due |= mask & ((old_levels ^ levels) & levels);
                }
                _ => panic!()
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Gpio;

    #[test]
    fn test_mode0_1() {
        let mut gpio = Gpio::new();
        gpio.ddr = 0;
        gpio.alt1 = 0;
        gpio.alt2 = 0;
        gpio.set_dr(0xc5);
        assert_eq!(gpio.get_dr(), 0xc5);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        assert_eq!(gpio.get_output_level(), 0xc5);
    }

    #[test]
    fn test_mode2_3() {
        let mut gpio = Gpio::new();
        gpio.ddr = 0xff;
        gpio.alt1 = 0;
        gpio.alt2 = 0;
        gpio.set_dr(0xc5);
        gpio.set_input_pins(0x5c);
        assert_eq!(gpio.get_dr(), 0x5c);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        assert_eq!(gpio.get_output_level(), 0x5c);
    }

    #[test]
    fn test_mode9_dual_edge_triggered() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x80);
        gpio.ddr = 0;
        gpio.alt1 = 0;
        gpio.alt2 = 0x80;
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x80);
        assert_eq!(gpio.get_interrupt_due(), 0x80);
        gpio.set_dr(0x80);
        assert_eq!(gpio.get_interrupt_due(), 0x00);
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x80);
    }

    #[test]
    fn test_mode12_active_low() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr = 0x0;
        gpio.alt1 = 0x40;
        gpio.alt2 = 0x40;
        // note -- no interrupt happens until input level is set
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_input_pins(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
    }

    #[test]
    fn test_mode12_active_low_using_update() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr = 0x0;
        gpio.alt1 = 0x40;
        gpio.alt2 = 0x40;
        // note -- no interrupt happens until input level is set
        gpio.update();
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_input_pins(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
    }

    #[test]
    fn test_mode13_active_high() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x40);
        gpio.ddr = 0x0;
        gpio.alt1 = 0x40;
        gpio.alt2 = 0x40;
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_input_pins(0x0);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
    }

    #[test]
    fn test_mode14_falling_edge_triggered() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr = 0x40;
        gpio.alt1 = 0x40;
        gpio.alt2 = 0x40;
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x00);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
    }

    #[test]
    fn test_mode15_rising_edge_triggered() {
        let mut gpio = Gpio::new();
        gpio.set_dr(0x40);
        gpio.ddr = 0x40;
        gpio.alt1 = 0x40;
        gpio.alt2 = 0x40;
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x40);
        gpio.set_dr(0x40);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        gpio.set_input_pins(0x00);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
    }

    // note - modes 4,5, 6,7, 10,11 are not tested (or probably implemented ;)
}
