const FCTL_FIFOEN: u8 = 0x1;
const IER_RIE: u8 = 0x01;
const IER_TIE: u8 = 0x02;
const IIR_NO_PENDING: u8 = 0x01;
const IIR_RX_DATA_READY: u8 = 0x04;
const IIR_TX_EMPTY: u8 = 0x02;
const IIR_FIFO_ENABLED: u8 = 0xC0;

pub trait SerialLink {
    fn send(&mut self, byte: u8);
    fn recv(&mut self) -> Option<u8>;
    fn read_clear_to_send(&mut self) -> bool;
}

pub struct Uart {
    link: Box<dyn SerialLink>,
    rx_buf: Option<u8>,

    /* number of clock cycles before a byte can be sent again */
    transmit_cooldown: i32,
    receive_cooldown: i32,

    // interrupt enable register
    pub ier: u8,
    // FIFO control register
    fctl: u8,
    // Line control register
    pub lctl: u8, // lctl & 0x80 enables access to baud rate generator
    // registers where rbr/thr and ier are normally accessed
    pub brg_div: u16,
    // scratch pad register
    pub spr: u8,

    tx_fifo: Vec<u8>,
}

impl Uart {
    pub fn new(link: Box<dyn SerialLink>) -> Self {
        Uart {
            link,
            transmit_cooldown: 0,
            receive_cooldown: 0,
            tx_fifo: vec![],
            ier: 0,
            fctl: 0,
            lctl: 0,
            brg_div: 2,
            spr: 0,
            rx_buf: None,
        }
    }

    pub fn apply_ticks(&mut self, cycles: i32) {
        self.transmit_cooldown = i32::max(0, self.transmit_cooldown - cycles);
        if self.transmit_cooldown == 0 {
            if !self.tx_fifo.is_empty() {
                let val = self.tx_fifo.remove(0);
                // actually send
                self.link.send(val);
                self.transmit_cooldown += self.brg_div as i32 * 16 * 10; /* 10 = 8bits data, 1 start bit, 1 stop bit, 0 parity bits */
            }
        }
        self.receive_cooldown = i32::max(0, self.receive_cooldown - cycles);
        if self.receive_cooldown == 0 {
            if self.rx_buf == None {
                self.rx_buf = self.link.recv();
            }
            if self.rx_buf.is_some() {
                // Note uart0 RX is arbitrarily made 4x slower than tx rate, to keep
                // bad MOS keyboard routines (mos_getkey) happy with keyboard packets
                // queued up with no wait between them. Perhaps this difference is due
                // to emulator frontend only periodically checking for keyboard events,
                // while the real agon is watching for PS/2 data constantly
                self.receive_cooldown += self.brg_div as i32 * 64 * 10; /* 10 = 8bits data, 1 start bit, 1 stop bit, 0 parity bits */
            }
        }
    }

    pub fn send_byte(&mut self, value: u8) {
        if (self.tx_fifo.len() < 16 && self.fctl & FCTL_FIFOEN != 0)
            || (self.tx_fifo.is_empty() && self.fctl & FCTL_FIFOEN == 0)
        {
            self.tx_fifo.push(value);
        } else {
            // drop the data. the ez80 was pushing data too fast
        }
    }

    pub fn receive_byte(&mut self) -> u8 {
        let maybe_data = self.rx_buf;
        self.rx_buf = None;

        match maybe_data {
            Some(data) => data,
            None => 0,
        }
    }

    pub fn rx_byte_ready(&self) -> bool {
        self.rx_buf.is_some()
    }

    /** line status register */
    pub fn read_lsr(&mut self) -> u8 {
        // 0x01 = DR (data ready: ie can receive)
        (if self.rx_byte_ready() { 1 } else { 0 }) |
        // 0x20 = TRHE (fifo / transmit  holding register empty)
        (if self.tx_fifo.is_empty() { 0x20 } else { 0 }) |
        // 0x40 = TEMT (fifo / transmit holding register empty & transmitter idle)
        (if self.tx_fifo.is_empty() && self.transmit_cooldown == 0 { 0x40 } else { 0 })
    }

    pub fn write_fctl(&mut self, val: u8) {
        self.fctl = val;
    }

    pub fn read_modem_status_register(&mut self) -> u8 {
        if self.link.read_clear_to_send() {
            0x10
        } else {
            0
        }
    }

    /*
    pub fn get_baud_rate(&self) -> u32 {
        18_432_000 / (self.brg_div as u32 * 16)
    }
    */

    pub fn is_access_brg_registers(&self) -> bool {
        self.lctl & 0x80 != 0
    }

    pub fn is_rx_interrupt_enabled(&self) -> bool {
        self.ier & IER_RIE != 0
    }

    pub fn is_interrupt_due(&self) -> bool {
        (self.is_rx_interrupt_enabled() && self.rx_byte_ready())
            || (self.ier & IER_TIE != 0 && self.tx_fifo.is_empty())
    }

    pub fn read_iir(&self) -> u8 {
        let fifo_status = if self.fctl & FCTL_FIFOEN != 0 {
            IIR_FIFO_ENABLED
        } else {
            0
        };
        let cause = if self.is_rx_interrupt_enabled() && self.rx_byte_ready() {
            IIR_RX_DATA_READY
        } else if self.ier & IER_TIE != 0 && self.tx_fifo.is_empty() {
            IIR_TX_EMPTY
        } else {
            IIR_NO_PENDING
        };
        fifo_status | cause
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::RefCell;
    use std::collections::VecDeque;
    use std::rc::Rc;

    #[derive(Default)]
    struct LinkState {
        rx: VecDeque<u8>,
        tx: Vec<u8>,
    }

    struct TestLink(Rc<RefCell<LinkState>>);

    impl SerialLink for TestLink {
        fn send(&mut self, byte: u8) {
            self.0.borrow_mut().tx.push(byte);
        }

        fn recv(&mut self) -> Option<u8> {
            self.0.borrow_mut().rx.pop_front()
        }

        fn read_clear_to_send(&mut self) -> bool {
            true
        }
    }

    fn test_uart() -> (Uart, Rc<RefCell<LinkState>>) {
        let state = Rc::new(RefCell::new(LinkState::default()));
        (Uart::new(Box::new(TestLink(state.clone()))), state)
    }

    #[test]
    fn iir_reports_no_pending_and_preserves_ier() {
        let (mut uart, _) = test_uart();
        uart.write_fctl(FCTL_FIFOEN);
        uart.ier = IER_RIE;

        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_NO_PENDING);
        assert_eq!(uart.ier, IER_RIE);
        assert!(!uart.is_interrupt_due());
    }

    #[test]
    fn tx_empty_interrupt_repeats_after_fifo_drains_without_clearing_tie() {
        let (mut uart, state) = test_uart();
        uart.write_fctl(FCTL_FIFOEN);
        uart.ier = IER_TIE;

        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_TX_EMPTY);
        assert_eq!(uart.ier, IER_TIE);
        uart.send_byte(b'X');
        assert!(!uart.is_interrupt_due());
        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_NO_PENDING);

        uart.apply_ticks(1);
        assert_eq!(state.borrow().tx, b"X");
        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_TX_EMPTY);
        assert_eq!(uart.ier, IER_TIE);
        assert!(uart.is_interrupt_due());
    }

    #[test]
    fn rx_interrupt_requires_data_and_preserves_rie() {
        let (mut uart, state) = test_uart();
        uart.write_fctl(FCTL_FIFOEN);
        uart.ier = IER_RIE;
        state.borrow_mut().rx.push_back(b'R');

        assert!(!uart.is_interrupt_due());
        uart.apply_ticks(1);
        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_RX_DATA_READY);
        assert_eq!(uart.receive_byte(), b'R');
        assert_eq!(uart.read_iir(), IIR_FIFO_ENABLED | IIR_NO_PENDING);
        assert_eq!(uart.ier, IER_RIE);
    }
}
