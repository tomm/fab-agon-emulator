//! Deterministic machine fixture for watchdog tests.
use super::*;
use ez80::Reg16;
use std::sync::atomic::{AtomicBool, AtomicI32};

struct DisconnectedSerial;
impl uart::SerialLink for DisconnectedSerial {
    fn send(&mut self, _: u8) {}
    fn recv(&mut self) -> Option<u8> { None }
    fn read_clear_to_send(&mut self) -> bool { true }
}
pub(super) fn fixture(precision: i32) -> (AgonMachine, Cpu) {
    let (tx, _) = std::sync::mpsc::channel();
    let mut m = AgonMachine::new(AgonMachineConfig {
        uart0_link: Box::new(DisconnectedSerial), uart1_link: Box::new(DisconnectedSerial),
        soft_reset: Arc::new(AtomicBool::new(false)),
        emulator_shutdown: Arc::new(AtomicBool::new(false)),
        exit_status: Arc::new(AtomicI32::new(0)), paused: Arc::new(AtomicBool::new(false)),
        clockspeed_hz: 18_432_000, ram_init: RamInit::Zero,
        mos_bin: std::path::PathBuf::new(), gpios: Arc::new(gpio::GpioSet::new()),
        tx_gpio_vga_frame: tx, interrupt_precision: precision, external_ram_size: 512*1024,
    });
    m.enable_hostfs = false;
    m.cs0_lbr = 4; m.cs0_ubr = 11;
    let mut cpu = Cpu::new_ez80();
    cpu.state.reg.adl = true;
    cpu.state.reg.set24(Reg16::SP, 0x060000);
    cpu.state.reg.set_a(0);
    (m, cpu)
}
