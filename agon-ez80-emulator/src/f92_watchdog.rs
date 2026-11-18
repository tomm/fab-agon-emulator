//! F92 watchdog registers and expiry (PS0153, pp. 73–75).

pub(super) const CONTROL: u16 = 0x0093;
pub(super) const RELOAD: u16 = 0x0094;
const ENABLE: u8 = 0x80;
const SELECT_NMI: u8 = 0x40;
const EXPIRED: u8 = 0x20;
const CLOCK_AND_PERIOD: u8 = 0x18 | 0x03;
const CONFIG: u8 = SELECT_NMI | CLOCK_AND_PERIOD;
const RTC_HZ: u128 = 32_768;
const PERIODS: [u64; 4] = [1 << 27, 1 << 25, 1 << 22, 1 << 18];

#[derive(Debug, PartialEq, Eq)]
pub(super) enum Expiry {
    Nmi,
    ChipReset,
}

#[derive(Default)]
pub(super) struct Watchdog {
    control: u8,
    guest_cycles: u64,
    edges: u64,
    armed: bool,
    saw_a5: bool,
    pending: Option<Expiry>,
}

impl Watchdog {
    pub(super) fn read_control(&self) -> u8 {
        self.control
    }

    pub(super) fn write_control(&mut self, value: u8) {
        let previous = self.control;
        self.control = (previous & (ENABLE | EXPIRED)) | (value & (ENABLE | CONFIG));
        if previous & ENABLE == 0 && self.control & ENABLE != 0 {
            self.armed = true;
        }
        // The manual does not specify the running divisor-write transition.
        // This candidate starts a fresh interval when clock or period changes.
        if (previous ^ self.control) & (ENABLE | CLOCK_AND_PERIOD) != 0 {
            self.edges = 0;
            self.saw_a5 = false;
        }
    }

    pub(super) fn write_reload(&mut self, value: u8) {
        if self.control & ENABLE == 0 || self.pending.is_some() {
            return;
        }
        if self.saw_a5 && value == 0x5a {
            self.edges = 0;
            // Allow a fresh interval after a delivered NMI. The sticky expiry
            // flag reports the event and does not control the counter.
            self.armed = true;
        }
        // An incorrect byte ends an incomplete pair. A5 can start a new pair.
        self.saw_a5 = value == 0xa5;
    }

    pub(super) fn advance(&mut self, cycles: u64, system_hz: u64) {
        let before = self.guest_cycles;
        self.guest_cycles += cycles;
        if !self.armed {
            return;
        }
        let edges = match (self.control >> 3) & 3 {
            0 => cycles,
            1 => {
                // Absolute guest time preserves crystal phase across service,
                // clock selection, and intervals before the watchdog is armed.
                let crystal_edges = |time| u128::from(time) * RTC_HZ / u128::from(system_hz.max(1));
                (crystal_edges(self.guest_cycles) - crystal_edges(before)) as u64
            }
            _ => return, // Reserved clock select.
        };
        self.edges += edges;
        if self.edges >= PERIODS[usize::from(self.control & 3)] {
            self.control |= EXPIRED;
            self.armed = false;
            self.saw_a5 = false;
            self.pending = Some(if self.control & SELECT_NMI != 0 {
                Expiry::Nmi
            } else {
                Expiry::ChipReset
            });
        }
    }

    pub(super) fn take_expiry(&mut self) -> Option<Expiry> {
        self.pending.take()
    }

    pub(super) fn expiry_pending(&self) -> bool {
        self.pending.is_some()
    }

    pub(super) fn reset_after_timeout(&mut self) {
        // The RTC source runs independently of the watchdog reset.
        *self = Self { control: EXPIRED, guest_cycles: self.guest_cycles, ..Self::default() };
    }
}
