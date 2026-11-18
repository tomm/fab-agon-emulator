use super::*;
use f92_watchdog::{Expiry, Watchdog, CONTROL, RELOAD};
use std::sync::atomic::Ordering;

const SYSTEM_HZ: u64 = 18_432_000;
const SHORT_PERIOD: u64 = 262_144;

#[test]
fn first_expiry_edges_at_each_system_and_crystal_period() {
    for (select, seconds_at_crystal) in [4096_u64, 1024, 128, 8].into_iter().enumerate() {
        for (clock_select, cycles) in [
            (0, seconds_at_crystal * 32_768),
            (8, seconds_at_crystal * SYSTEM_HZ),
        ] {
            let mut wdt = Watchdog::default();
            wdt.write_control(0xc0 | clock_select | select as u8);
            wdt.advance(cycles - 1, SYSTEM_HZ);
            assert_eq!(wdt.read_control() & 0x20, 0);
            assert_eq!(wdt.take_expiry(), None);
            wdt.advance(1, SYSTEM_HZ);
            assert_eq!(wdt.take_expiry(), Some(Expiry::Nmi));
            assert_ne!(wdt.read_control() & 0x20, 0);
            assert_eq!(wdt.take_expiry(), None);
        }
    }
}

#[test]
fn control_masks_enable_and_expiry_with_reserved_clock() {
    let (mut machine, _) = watchdog_fixture::fixture(1);
    assert_eq!(machine.port_in(CONTROL), 0);
    machine.port_out(CONTROL, 0x67); // Cannot set RST_FLAG or reserved bit 2.
    machine.watchdog.advance(SHORT_PERIOD, SYSTEM_HZ);
    assert_eq!(machine.port_in(CONTROL), 0x43);
    machine.port_out(CONTROL, 0xd3);
    machine.watchdog.advance(SHORT_PERIOD * 2, SYSTEM_HZ);
    assert_eq!(machine.port_in(CONTROL), 0xd3);
    machine.port_out(CONTROL, 0x5b); // Still enabled, second reserved clock.
    machine.watchdog.advance(SHORT_PERIOD * 2, SYSTEM_HZ);
    assert_eq!(machine.port_in(CONTROL), 0xdb);
    machine.port_out(CONTROL, 0x43);
    assert_eq!(machine.port_in(CONTROL), 0xc3);
    machine.watchdog.advance(SHORT_PERIOD, SYSTEM_HZ);
    machine.port_out(CONTROL, 0x43);
    assert_eq!(machine.port_in(CONTROL), 0xe3);
    assert_eq!(machine.port_in(RELOAD), 0);
    assert_eq!(machine.watchdog.take_expiry(), Some(Expiry::Nmi));
}

#[test]
fn reload_requires_a_pair_and_keeps_fractional_crystal_phase() {
    for (sequence, valid) in [
        (&[0xa5, 0x5a][..], true),
        (&[0x5a][..], false),
        (&[0x5a, 0xa5][..], false),
        (&[0xa5, 0x00, 0x5a][..], false),
    ] {
        let (mut machine, _) = watchdog_fixture::fixture(16);
        machine.port_out(CONTROL, 0xc3);
        machine.watchdog.advance(SHORT_PERIOD - 2, SYSTEM_HZ);
        for byte in sequence {
            machine.port_out(RELOAD, *byte);
        }
        machine.watchdog.advance(2, SYSTEM_HZ);
        assert_eq!(machine.watchdog.take_expiry(), (!valid).then_some(Expiry::Nmi));
        if valid {
            machine.watchdog.advance(SHORT_PERIOD - 3, SYSTEM_HZ);
            assert_eq!(machine.watchdog.take_expiry(), None);
            machine.watchdog.advance(1, SYSTEM_HZ);
            assert_eq!(machine.watchdog.take_expiry(), Some(Expiry::Nmi));
        }
    }

    let mut wdt = Watchdog::default();
    wdt.advance(563, SYSTEM_HZ); // First nominal crystal tick, before arm.
    wdt.write_control(0xcb);
    wdt.advance(561, SYSTEM_HZ);
    wdt.write_reload(0xa5);
    wdt.advance(2, SYSTEM_HZ); // One guest cycle past the second crystal edge.
    wdt.write_reload(0x5a);
    wdt.advance(8 * SYSTEM_HZ - 2, SYSTEM_HZ);
    assert_eq!(wdt.take_expiry(), None);
    wdt.advance(1, SYSTEM_HZ);
    assert_eq!(wdt.take_expiry(), Some(Expiry::Nmi));
}

#[test]
fn delivered_nmi_can_be_rearmed_without_clearing_the_flag() {
    let mut wdt = Watchdog::default();
    wdt.write_control(0xc3);
    wdt.advance(SHORT_PERIOD - 1, SYSTEM_HZ);
    wdt.write_reload(0xa5);
    wdt.advance(1, SYSTEM_HZ);
    wdt.write_reload(0x5a); // Too late to cancel the pending NMI.
    assert_eq!(wdt.take_expiry(), Some(Expiry::Nmi));
    wdt.advance(SHORT_PERIOD * 2, SYSTEM_HZ);
    wdt.write_reload(0x5a); // An unmatched byte must not rearm.
    wdt.advance(SHORT_PERIOD, SYSTEM_HZ);
    assert_eq!(wdt.take_expiry(), None);

    for _ in 0..3 {
        wdt.write_reload(0xa5);
        wdt.write_reload(0x5a);
        assert_eq!(wdt.read_control(), 0xe3);
        wdt.advance(SHORT_PERIOD - 1, SYSTEM_HZ);
        assert_eq!(wdt.take_expiry(), None);
        wdt.advance(1, SYSTEM_HZ);
        assert_eq!(wdt.take_expiry(), Some(Expiry::Nmi));
    }
}

#[test]
fn fast_path_nmi_frames_and_decoded_retn() {
    for (adl, madl, frame) in [
        (false, false, &[0x34, 0x12][..]),
        (true, false, &[0x34, 0x12, 0x05][..]),
        (false, true, &[0x02, 0x34, 0x12][..]),
        (true, true, &[0x03, 0x34, 0x12, 0x05][..]),
    ] {
        for enabled in [false, true] {
            let (mut machine, mut cpu) = watchdog_fixture::fixture(16);
            cpu.state.reg.adl = adl;
            cpu.state.reg.madl = madl;
            cpu.state.reg.mbase = 4;
            let sps = 0x047000;
            let spl = 0x066000;
            cpu.state.reg.set16(Reg16::SP, sps as u16);
            cpu.state.reg.set24(Reg16::SP, spl);
            let interrupted_pc = if adl { 0x051234 } else { 0x041234 };
            machine.poke(interrupted_pc - 1, if enabled { 0xfb } else { 0xf3 });
            cpu.state.set_pc(interrupted_pc - 1);
            machine.execute_instruction(&mut cpu);
            assert_eq!(cpu.state.pc(), interrupted_pc);
            assert_eq!(cpu.state.reg.get_iff1(), enabled);
            for top in [sps, spl] {
                for offset in 0..6 {
                    machine.poke(top - offset, 0xa9);
                }
            }

            // LD A,I exposes IFF2 in P/V before the actual RETN.
            let handler = if madl {
                &[0xed, 0x57, 0x5b, 0xed, 0x45][..]
            } else {
                &[0xed, 0x57, 0xed, 0x45][..]
            };
            if adl || madl {
                machine.mem_rom[0x66..0x66 + handler.len()].copy_from_slice(handler);
            } else {
                for (offset, byte) in handler.iter().enumerate() {
                    machine.poke(0x040066 + offset as u32, *byte);
                }
            }
            let retired = cpu.state.instructions_executed;
            cpu.signal_nmi();
            machine.execute_instruction(&mut cpu);
            let top = if adl || madl { spl } else { sps };
            let other = if top == spl { sps } else { spl };
            let sp = top - frame.len() as u32;
            assert_eq!(cpu.state.pc(), if adl || madl { 0x66 } else { 0x040066 });
            assert_eq!(cpu.state.reg.adl, adl || madl);
            assert_eq!(cpu.state.reg.madl, madl);
            assert!(!cpu.state.reg.get_iff1());
            assert!(!cpu.state.nmi_pending);
            assert_eq!(cpu.state.instructions_executed, retired);
            assert_eq!(cpu.state.sp(), sp);
            for (offset, byte) in frame.iter().enumerate() {
                assert_eq!(machine.peek(sp + offset as u32), *byte);
            }
            assert_eq!(machine.peek(sp - 1), 0xa9);
            assert_eq!(machine.peek(top), 0xa9);
            assert!((0..6).all(|offset| machine.peek(other - offset) == 0xa9));

            machine.execute_instruction(&mut cpu); // LD A,I
            assert_eq!(cpu.state.reg.get_flag(Flag::P), enabled);
            assert!(!cpu.state.reg.get_iff1());
            machine.execute_instruction(&mut cpu); // RETN
            assert_eq!(cpu.state.pc(), interrupted_pc);
            assert_eq!(cpu.state.reg.adl, adl);
            assert_eq!(cpu.state.reg.madl, madl);
            assert_eq!(cpu.state.reg.mbase, 4);
            assert_eq!(cpu.state.reg.get_iff1(), enabled);
            assert_eq!(cpu.state.reg.get16(Reg16::SP), sps as u16);
            assert_eq!(cpu.state.reg.get24(Reg16::SP), spl);
        }
    }
}

#[test]
fn watchdog_wakes_di_halt_and_returns_to_next_pc() {
    for precision in [1, 64] {
        let (mut machine, mut cpu) = watchdog_fixture::fixture(precision);
        machine.mem_rom[0x1100..0x1105].copy_from_slice(&[0xf3, 0xed, 0x39, 0x93, 0x76]);
        machine.mem_rom[0x66..0x68].copy_from_slice(&[0xed, 0x45]);
        cpu.state.set_pc(0x1100);
        cpu.state.reg.set_a(0xc3);
        machine.execute_instruction(&mut cpu); // DI
        machine.execute_instruction(&mut cpu); // OUT0 arms watchdog
        let before_halt = machine.cycle_counter.get();
        machine.execute_instruction(&mut cpu);
        assert!(cpu.is_halted());
        let halt_cost = (machine.cycle_counter.get() - before_halt) as u64;
        machine.prt_timers[0].write_ctl(0xc0); // Pending maskable IRQ.
        let retired = cpu.state.instructions_executed;
        let parked_steps_before_expiry = (SHORT_PERIOD - halt_cost - 1) / 4;
        for _ in 0..parked_steps_before_expiry {
            machine.execute_instruction(&mut cpu);
            if machine.cycle_counter.get() >= precision {
                machine.apply_elapsed_cycles();
                machine.do_interrupts(&mut cpu);
            }
        }
        assert_eq!(machine.peek(0x1100), 0xf3);
        assert!(cpu.is_halted());
        assert_eq!(cpu.state.pc(), 0x1105);
        machine.execute_instruction(&mut cpu);
        machine.do_interrupts(&mut cpu);
        assert!(!cpu.is_halted());
        assert_eq!(cpu.state.pc(), 0x66);
        assert_eq!(cpu.state.instructions_executed, retired);
        assert_eq!(machine.port_in(CONTROL), 0xe3);
        machine.execute_instruction(&mut cpu);
        assert_eq!(cpu.state.pc(), 0x1105);
        assert!(!cpu.state.reg.get_iff1());
    }
}

#[test]
fn latched_expiry_wins_over_same_instruction_service() {
    let (mut machine, mut cpu) = watchdog_fixture::fixture(1);
    machine.mem_rom[0x1100..0x1103].copy_from_slice(&[0xed, 0x39, 0x94]);
    machine.port_out(CONTROL, 0xc3);
    machine.port_out(RELOAD, 0xa5);
    machine.watchdog.advance(SHORT_PERIOD - 1, SYSTEM_HZ);
    cpu.state.set_pc(0x1100);
    cpu.state.reg.set_a(0x5a);
    machine.execute_instruction(&mut cpu);
    assert_eq!(cpu.state.pc(), 0x66);
    assert_eq!(machine.port_in(CONTROL), 0xe3);
}

#[test]
fn handler_reload_delivers_three_watchdog_nmis_and_resumes_halt_loop() {
    let (mut machine, mut cpu) = watchdog_fixture::fixture(64);
    machine.mem_rom[0x1100..0x1103].copy_from_slice(&[0x76, 0x18, 0xfd]); // HALT; JR back.
    machine.mem_rom[0x66..0x72].copy_from_slice(&[
        0x3e, 0xa5, 0xed, 0x39, 0x94, // OUT0 (94h),A5h
        0x3e, 0x5a, 0xed, 0x39, 0x94, // OUT0 (94h),5Ah
        0xed, 0x45, // RETN
    ]);
    cpu.state.reg.iff1 = false;
    cpu.state.reg.iff2 = false;
    cpu.state.set_pc(0x1100);
    let sp = cpu.state.sp();
    machine.port_out(CONTROL, 0xc3);

    for _ in 0..3 {
        for _ in 0..SHORT_PERIOD {
            machine.execute_instruction(&mut cpu);
            if machine.cycle_counter.get() >= 64 {
                machine.apply_elapsed_cycles();
                machine.do_interrupts(&mut cpu);
            }
            if cpu.state.pc() == 0x66 {
                break;
            }
        }
        assert_eq!(cpu.state.pc(), 0x66);
        assert_eq!(cpu.state.sp(), sp - 3);
        for _ in 0..5 {
            machine.execute_instruction(&mut cpu);
        }
        assert_eq!(cpu.state.pc(), 0x1101);
        assert_eq!(cpu.state.sp(), sp);
        assert!(!cpu.state.reg.get_iff1());
        assert_eq!(machine.port_in(CONTROL), 0xe3);
    }
}

#[test]
fn watchdog_chip_reset_holds_257_cycles_and_restarts_from_flash() {
    for precision in [1, 16, 64] {
        let (mut machine, mut cpu) = watchdog_fixture::fixture(precision);
        machine.port_out(0x80, 0xc0); // Set PRT0's IRQ latch.
        machine.port_out(0x9b, 0x00);
        machine.port_out(0x9c, 0xff);
        machine.port_out(0xa4, 0xff);
        machine.port_out(0xc1, 0x07);
        machine.port_out(0xc7, 0x55);
        machine.port_out(0xd3, 0x80);
        machine.port_out(0xd0, 0xe1);
        machine.port_out(0xd1, 0x12);
        machine.port_out(0xcb, 0x80);
        machine.port_out(0xf7, 2); // Relocate flash before reset.
        machine.port_out(0xf8, 0);
        machine.poke(0x04_3456, 0xa6);
        machine.poke(0xff_e002, 0x69);
        machine.port_out(0xb4, 0);
        machine.port_out(0xb5, 0x21);
        machine.mem_rom[0..3].copy_from_slice(&[0xed, 0x38, 0x93]); // IN0 A,(93h).
        machine.mem_rom[0x1100] = 0; // NOP at relocated flash.
        cpu.state.reg.madl = true;
        cpu.state.reg.mbase = 4;
        cpu.state.set_pc(0x02_1100);
        machine.port_out(CONTROL, 0x83);
        machine.watchdog.advance(SHORT_PERIOD - 1, SYSTEM_HZ);
        machine.execute_instruction(&mut cpu);
        assert!(!machine.emulator_shutdown.load(Ordering::Relaxed));
        assert!(!machine.soft_reset.load(Ordering::Relaxed));
        assert_eq!(machine.exit_status.load(Ordering::Relaxed), 0);
        assert_eq!(cpu.state.pc(), 0);
        assert!(!cpu.state.reg.adl && !cpu.state.reg.madl);
        assert_eq!(cpu.state.reg.mbase, 0);
        assert!(!cpu.state.reg.get_iff1() && !cpu.state.reg.iff2);
        assert!(!cpu.is_halted());
        assert_eq!(machine.port_in(CONTROL), 0x20);
        assert!(!machine.prt_timers[0].irq_due());
        for direction in [0x9b, 0x9f, 0xa3] {
            assert_eq!(machine.port_in(direction), 0xff);
            assert_eq!(machine.port_in(direction + 1), 0);
            assert_eq!(machine.port_in(direction + 2), 0);
        }
        assert_eq!(machine.port_in(0xc1), 0);
        assert_eq!(machine.port_in(0xc5), 0x60);
        assert_eq!(machine.port_in(0xc7), 0);
        assert_eq!(machine.port_in(0xd3), 0);
        assert_eq!(machine.uart1.brg_div, 2);
        assert_eq!(machine.port_in(0xcb), 0);
        assert_eq!(machine.port_in(0xb4), 0x80);
        assert_eq!(machine.port_in(0xb5), 0xff);
        assert_eq!(machine.port_in(0xf7), 0);
        assert_eq!(machine.flash_waitstates, 4);
        assert_eq!((machine.cs0_lbr, machine.cs0_ubr), (0, 0xff));
        assert_eq!(machine.peek(0x04_3456), 0xa6);
        assert_eq!(machine.peek(0xff_e002), 0x69);
        machine.apply_elapsed_cycles();
        let before_hold = machine.total_cycles_elapsed;
        let retired = cpu.state.instructions_executed;
        for _ in 0..257 {
            machine.execute_instruction(&mut cpu);
            if machine.cycle_counter.get() >= precision {
                machine.apply_elapsed_cycles();
                machine.do_interrupts(&mut cpu);
            }
            assert_eq!(cpu.state.pc(), 0);
            assert_eq!(cpu.state.instructions_executed, retired);
        }
        machine.apply_elapsed_cycles();
        assert_eq!(machine.total_cycles_elapsed - before_hold, 257);
        machine.execute_instruction(&mut cpu);
        assert_eq!(cpu.state.pc(), 3);
        assert_eq!(cpu.state.reg.a(), 0x20);
        assert_eq!(cpu.state.instructions_executed, retired + 1);

        machine.port_out(CONTROL, 0xc3);
        machine.watchdog.advance(SHORT_PERIOD, SYSTEM_HZ);
        assert_eq!(machine.watchdog.take_expiry(), Some(Expiry::Nmi));
    }
}

#[test]
fn pending_core_reset_prevents_nmi_stack_entry() {
    let (mut machine, mut cpu) = watchdog_fixture::fixture(16);
    let sp = cpu.state.sp();
    let pc = cpu.state.pc();
    cpu.signal_nmi();
    cpu.signal_reset();
    assert!(!cpu.accept_pending_ez80_nmi(&mut machine));
    assert!(cpu.state.reset_pending);
    assert!(cpu.state.nmi_pending);
    assert_eq!(cpu.state.pc(), pc);
    assert_eq!(cpu.state.sp(), sp);
}
