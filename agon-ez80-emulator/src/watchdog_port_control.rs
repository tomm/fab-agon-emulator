//! Readback and NMI witnesses that also compile against the baseline.
use super::*;

#[test]
fn wdt_ctl_readback_through_guest_opcodes() {
    let (mut machine, mut cpu) = watchdog_fixture::fixture(16);
    machine.mem_rom[0x1200..0x1206].copy_from_slice(&[
        0xed, 0x39, 0x93, // OUT0 (93h),A
        0xed, 0x38, 0x93, // IN0 A,(93h)
    ]);
    cpu.state.set_pc(0x1200);
    cpu.state.reg.set_a(0x40);
    machine.execute_instruction(&mut cpu);
    cpu.state.reg.set_a(0);
    machine.execute_instruction(&mut cpu);
    assert_eq!(cpu.state.reg.a(), 0x40, "WDT_CTL readback through IN0/OUT0");
}

#[test]
fn pending_nmi_enters_vector_before_normal_fetch() {
    let (mut machine, mut cpu) = watchdog_fixture::fixture(16);
    machine.mem_rom[0x1200] = 0xf3; // DI
    cpu.state.set_pc(0x1200);
    machine.execute_instruction(&mut cpu);
    assert!(!cpu.state.reg.get_iff1());
    cpu.signal_nmi();
    machine.execute_instruction(&mut cpu);
    assert_eq!(cpu.state.pc(), 0x66, "NMI vector before normal fetch");
}
