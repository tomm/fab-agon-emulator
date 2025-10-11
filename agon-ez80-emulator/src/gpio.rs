use std::sync::atomic::AtomicU8;
use std::sync::atomic::Ordering::Relaxed;

pub struct GpioSet {
    pub b: Gpio,
    pub c: Gpio,
    pub d: Gpio,
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
    io_level: AtomicU8,
    interrupt_due: AtomicU8, // bitmap of 8 gpio pins
    dr: AtomicU8,

    pub ddr: AtomicU8,
    pub alt1: AtomicU8,
    pub alt2: AtomicU8,
}

impl Gpio {
    pub fn new() -> Self {
        Gpio {
            io_level: AtomicU8::new(0),
            dr: AtomicU8::new(0),
            ddr: AtomicU8::new(0xff),
            alt1: AtomicU8::new(0),
            alt2: AtomicU8::new(0),
            interrupt_due: AtomicU8::new(0),
        }
    }

    pub fn update(&self) {
        let level = self.get_output_level();
        self.set_input_pins(level);
    }

    pub fn get_interrupt_due(&self) -> u8 {
        self.interrupt_due.load(Relaxed)
    }

    // Note these aren't the mode numbers zilog docs use, but these made more sense
    // - pin 0..=7
    pub fn get_mode(&self, pin: u8) -> u8 {
        let mask = 1 << pin;
        let mode = if self.dr.load(Relaxed) & mask != 0 {
            1
        } else {
            0
        } + if self.ddr.load(Relaxed) & mask != 0 {
            2
        } else {
            0
        } + if self.alt1.load(Relaxed) & mask != 0 {
            4
        } else {
            0
        } + if self.alt2.load(Relaxed) & mask != 0 {
            8
        } else {
            0
        };
        assert!(mode < 16);
        mode
    }

    pub fn get_ddr(&self) -> u8 {
        self.ddr.load(Relaxed)
    }
    pub fn get_alt1(&self) -> u8 {
        self.alt1.load(Relaxed)
    }
    pub fn get_alt2(&self) -> u8 {
        self.alt2.load(Relaxed)
    }
    pub fn set_ddr(&self, val: u8) {
        let modified = self.ddr.load(Relaxed) ^ val;
        self.ddr.store(val, Relaxed);
        self.interrupt_due
            .store(self.interrupt_due.load(Relaxed) & !modified, Relaxed);
    }
    pub fn set_alt1(&self, val: u8) {
        let modified = self.ddr.load(Relaxed) ^ val;
        self.alt1.store(val, Relaxed);
        self.interrupt_due
            .store(self.interrupt_due.load(Relaxed) & !modified, Relaxed);
    }
    pub fn set_alt2(&self, val: u8) {
        let modified = self.ddr.load(Relaxed) ^ val;
        self.alt2.store(val, Relaxed);
        self.interrupt_due
            .store(self.interrupt_due.load(Relaxed) & !modified, Relaxed);
    }

    pub fn get_dr(&self) -> u8 {
        self.io_level.load(Relaxed)
    }

    pub fn set_dr(&self, dr: u8) {
        for pin in 0..=7 {
            let mode = self.get_mode(pin);
            let mask = 1 << pin;
            match mode {
                // output
                0 | 1 |
                // open drain output / IO
                4 | 5 |
                // open source IO / output
                6 | 7 => {
                    if dr & mask == 0 {
                        self.io_level.fetch_and(!mask, Relaxed);
                        self.dr.fetch_and(!mask, Relaxed);
                    } else {
                        self.io_level.fetch_or(mask, Relaxed);
                        self.dr.fetch_or(mask, Relaxed);
                    }
                }
                // input
                2 | 3 => {
                    if dr & mask == 0 {
                        self.dr.fetch_and(!mask, Relaxed);
                    } else {
                        self.dr.fetch_or(mask, Relaxed);
                    }
                }
                // reserved
                8 => {}
                // interrupt - dual edge triggered
                9 => {
                    if dr & mask != 0 {
                        self.interrupt_due.fetch_and(!mask, Relaxed);
                    }
                }
                // Port B, C, or D—alternate function controls port I/O
                10 | 11 => {}
                // Interrupt—active Low
                12 => {
                    if dr & mask != 0 {
                        self.interrupt_due.fetch_and(!(self.io_level.load(Relaxed) & mask), Relaxed);
                    }
                }
                // Interrupt—active High
                13 => {
                    if dr & mask != 0 {
                        self.interrupt_due.fetch_and(!(!self.io_level.load(Relaxed) & mask), Relaxed);
                    }
                }
                // Interrupt—falling edge triggered
                14 => {
                    if dr & mask != 0 {
                        self.interrupt_due.fetch_and(!mask, Relaxed);
                    }
                }
                // Interrupt—rising edge triggered
                15 => {
                    if dr & mask != 0 {
                        self.interrupt_due.fetch_and(!mask, Relaxed);
                    }
                }
                _ => panic!()
            }
        }
    }

    pub fn get_output_level(&self) -> u8 {
        self.io_level.load(Relaxed)
    }

    // pin: [0..7]
    pub fn set_input_pin(&self, pin: u8, state: bool) {
        let level = self.get_output_level();
        let bit = 1 << pin;
        self.set_input_pins(if state { level | bit } else { level & !bit });
    }

    pub fn set_input_pins(&self, levels: u8) {
        let old_levels = self.io_level.load(Relaxed);
        self.io_level.store(levels, Relaxed);

        for pin in 0..=7 {
            let mode = self.get_mode(pin);
            let mask = 1 << pin;
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
                    self.interrupt_due
                        .fetch_or(mask & (old_levels ^ levels), Relaxed);
                }
                // Port B, C, or D—alternate function controls port I/O
                10 | 11 => {}
                // Interrupt—active Low
                12 => {
                    self.interrupt_due.fetch_or(mask & !levels, Relaxed);
                }
                // Interrupt—active High
                13 => {
                    self.interrupt_due.fetch_or(mask & levels, Relaxed);
                }
                // Interrupt—falling edge triggered
                14 => {
                    self.interrupt_due
                        .fetch_or(mask & ((old_levels ^ levels) & !levels), Relaxed);
                }
                // Interrupt—rising edge triggered
                15 => {
                    self.interrupt_due
                        .fetch_or(mask & ((old_levels ^ levels) & levels), Relaxed);
                }
                _ => panic!(),
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::Gpio;
    use std::sync::atomic::Ordering::Relaxed;

    #[test]
    fn test_mode0_1() {
        let gpio = Gpio::new();
        gpio.ddr.store(0, Relaxed);
        gpio.alt1.store(0, Relaxed);
        gpio.alt2.store(0, Relaxed);
        gpio.set_dr(0xc5);
        assert_eq!(gpio.get_dr(), 0xc5);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        assert_eq!(gpio.get_output_level(), 0xc5);
    }

    #[test]
    fn test_mode2_3() {
        let gpio = Gpio::new();
        gpio.ddr.store(0xff, Relaxed);
        gpio.alt1.store(0, Relaxed);
        gpio.alt2.store(0, Relaxed);
        gpio.set_dr(0xc5);
        gpio.set_input_pins(0x5c);
        assert_eq!(gpio.get_dr(), 0x5c);
        assert_eq!(gpio.get_interrupt_due(), 0x0);
        assert_eq!(gpio.get_output_level(), 0x5c);
    }

    #[test]
    fn test_mode9_dual_edge_triggered() {
        let gpio = Gpio::new();
        gpio.set_dr(0x80);
        gpio.ddr.store(0, Relaxed);
        gpio.alt1.store(0, Relaxed);
        gpio.alt2.store(0x80, Relaxed);
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
        let gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr.store(0x0, Relaxed);
        gpio.alt1.store(0x40, Relaxed);
        gpio.alt2.store(0x40, Relaxed);
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
        let gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr.store(0x0, Relaxed);
        gpio.alt1.store(0x40, Relaxed);
        gpio.alt2.store(0x40, Relaxed);
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
        let gpio = Gpio::new();
        gpio.set_dr(0x40);
        gpio.ddr.store(0x0, Relaxed);
        gpio.alt1.store(0x40, Relaxed);
        gpio.alt2.store(0x40, Relaxed);
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
        let gpio = Gpio::new();
        gpio.set_dr(0x0);
        gpio.ddr.store(0x40, Relaxed);
        gpio.alt1.store(0x40, Relaxed);
        gpio.alt2.store(0x40, Relaxed);
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
        let gpio = Gpio::new();
        gpio.set_dr(0x40);
        gpio.ddr.store(0x40, Relaxed);
        gpio.alt1.store(0x40, Relaxed);
        gpio.alt2.store(0x40, Relaxed);
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
