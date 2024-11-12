// ez80f92 programmable reload timer 

#[derive(Debug)]
pub struct PrtTimer {
    // bit 0 - PRT_EN (enable)
    //     1 - RST_EN (reload & restart enabled)
    //     2-3 - CLK_DIV (4, 16, 64, 256)
    //     4 - PRT_MODE (0 - single pass, 1 - continuous)
    //     5 - n/a
    //     6 - IRQ_EN (irq enabled)
    //     7 - PRT_IRQ
    ctl: u8,
    reload: u16,
    counter: u16,
    step_: u16,
    latch_counter_high: u8,

    //t_: std::time::Instant
}

const PRT_EN: u8 = 0x1;
const RST_EN: u8 = 0x2;
const PRT_MODE: u8 = 0x10;

impl PrtTimer {
    pub fn new() -> Self {
        PrtTimer {
            ctl: 0, reload: 0, counter: 0, step_: 0, latch_counter_high: 0,
            //t_: std::time::Instant::now()
        }
    }

    pub fn irq_due(&self) -> bool {
        (self.ctl & 0xc0) == 0xc0
    }

    pub fn write_ctl(&mut self, c: u8) {
        self.ctl = c;
    }

    pub fn read_ctl(&mut self) -> u8 {
        let c = self.ctl;
        self.ctl &= 0x7f; // clear bit 7 (PRT_IRQ)
        c
    }

    pub fn read_counter_low(&mut self) -> u8 {
        self.latch_counter_high = (self.counter >> 8) as u8;
        (self.counter & 0xff) as u8
    }

    pub fn read_counter_high(&mut self) -> u8 {
        self.latch_counter_high
    }

    pub fn write_reload_low(&mut self, rl: u8) {
        self.reload = (self.reload & 0xff00) | (rl as u16);
    }

    pub fn write_reload_high(&mut self, rh: u8) {
        self.reload = (self.reload & 0xff) | ((rh as u16) << 8);
    }

    fn clk_div(&self) -> u16 {
        match (self.ctl >> 2) & 3 {
            0 => 4,
            1 => 16,
            2 => 64,
            3 => 256,
            _ => panic!()
        }
    }

    // intended to be called per-instruction, so clock_ticks should be < 8
    pub fn apply_ticks(&mut self, clock_ticks: u16) {
        if self.ctl & PRT_EN == 0 { return }
        if self.ctl & RST_EN != 0 {
            self.counter = self.reload;
            self.ctl &= !RST_EN;
        }
        let clk_div = self.clk_div();
        self.step_ = self.step_.wrapping_add(clock_ticks);
        if self.step_ >= clk_div {
            self.step_ -= clk_div;
            if self.counter == 0 {
                if self.ctl & PRT_MODE != 0 {
                    self.counter = self.reload;
                }
                //println("Elapsed {}", self.t_.elapsed());
                //self.t_ = std::time::Instant::now();
            } else {
                self.counter -= 1;
                if self.counter == 0 {
                    // set PRT_IRQ
                    self.ctl |= 0x80;

                    if self.ctl & PRT_MODE == 0 {
                        // clear RST_EN and PRT_EN
                        self.ctl &= 0b1111_1100;
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::PrtTimer;

    #[test]
    fn test_timer_single_pass() {
        let mut timer = PrtTimer::new();
        timer.write_reload_low(2);
        timer.apply_ticks(4);
        // not enabled yet
        assert_eq!(timer.counter, 0);

        timer.write_ctl(3);
        assert_eq!(timer.counter, 0);
        timer.apply_ticks(3);
        assert_eq!(timer.counter, 2);
        timer.apply_ticks(1);
        assert_eq!(timer.counter, 1);
        timer.apply_ticks(0);
        assert_eq!(timer.counter, 1);
        timer.apply_ticks(4);
        assert_eq!(timer.counter, 0);
        // not in continuous mode
        timer.apply_ticks(4);
        assert_eq!(timer.counter, 0);
        // first read - PRT_IRQ set
        assert_eq!(timer.read_ctl(), 0x80);
        // second read - PRT_IRQ has been cleared
        assert_eq!(timer.read_ctl(), 0x0);
    }

    #[test]
    fn test_timer_continuous() {
        let mut timer = PrtTimer::new();
        timer.write_reload_low(2);
        timer.apply_ticks(4);
        // not enabled yet
        assert_eq!(timer.counter, 0);

        timer.write_ctl(3 | 0x10);
        assert_eq!(timer.counter, 0);
        timer.apply_ticks(3);
        assert_eq!(timer.counter, 2);
        timer.apply_ticks(1);
        assert_eq!(timer.counter, 1);
        timer.apply_ticks(0);
        assert_eq!(timer.counter, 1);
        timer.apply_ticks(4);
        assert_eq!(timer.counter, 0);
        println!("last call");
        timer.apply_ticks(4);
        assert_eq!(timer.counter, 2);
        // first read - PRT_IRQ set
        assert_eq!(timer.read_ctl(), 0x91);
        // second read - PRT_IRQ has been cleared
        assert_eq!(timer.read_ctl(), 0x11);
    }

    #[test]
    fn test_write_reload() {
        let mut timer = PrtTimer::new();
        timer.write_reload_low(0xfe);
        timer.write_reload_high(0xca);
        assert_eq!(timer.reload, 0xcafe);
    }
}
