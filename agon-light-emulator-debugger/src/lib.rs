use inline_colorization::*;
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use std::sync::mpsc::{Receiver, Sender};

mod parser;

use agon_ez80_emulator::debugger::{DebugCmd, DebugResp, PauseReason, Reg16, Registers};

#[derive(Clone)]
struct EmuState {
    pub ez80_paused: std::cell::Cell<bool>,
    pub emulator_shutdown: std::sync::Arc<std::sync::atomic::AtomicBool>,
}

impl EmuState {
    pub fn is_in_debugger(&self) -> bool {
        self.ez80_paused.get()
    }

    pub fn set_in_debugger(&self, state: bool) {
        self.ez80_paused.set(state);
    }

    pub fn is_emulator_shutdown(&self) -> bool {
        self.emulator_shutdown
            .load(std::sync::atomic::Ordering::SeqCst)
    }

    pub fn shutdown(&self) {
        self.emulator_shutdown
            .store(true, std::sync::atomic::Ordering::SeqCst);
        self.set_in_debugger(false);
    }
}

fn print_help_line(command: &str, desc: &str) {
    println!("{color_cyan}{: <30}{color_white}{}", command, desc);
}

fn print_help() {
    print_help_line("br[eak] <address>", "Set a breakpoint at the hex address");
    print_help_line("c[ontinue]", "Resume (un-pause) Agon CPU");
    print_help_line("delete <address>", "Delete a breakpoint");
    print_help_line(
        "dis[assemble] [start] [end]",
        "Disassemble in current ADL mode",
    );
    print_help_line("dis16 [start] [end]", "Disassemble in ADL=0 (Z80) mode");
    print_help_line("dis24 [start] [end]", "Disassemble in ADL=1 (24-bit) mode");
    print_help_line("exit", "Quit from Agon Light Emulator");
    print_help_line("info breakpoints", "List breakpoints");
    print_help_line("[mem]ory <start> [len]", "Dump memory");
    print_help_line("n[ext]", "Step over function calls");
    print_help_line("pause", "Pause execution and enter debugger");
    print_help_line("state", "Show CPU state");
    print_help_line(".", "Show CPU state");
    print_help_line("s[tep]", "Execute one instuction");
    print_help_line("trace on", "Enable logging every instruction");
    print_help_line("trace off", "Disable logging every instruction");
    println!("{color_cyan}trigger <address> cmd1 : cmd2 : ...{color_white}");
    println!("    Perform debugger commands when <address> is reached");
    println!("    eg: break $123 is equivalent to:");
    println!("        trigger $123 pause:\"CPU paused at breakpoint\":state");
    println!();
    print_help_line("triggers", "List triggers");
    println!();
    println!("The previous command can be repeated by pressing return.");
    println!();
    println!("{color_green}CPU running. Press <CTRL-C> to pause");
    println!("{color_reset}");
}

fn do_cmd(cmd: parser::Cmd, tx: &Sender<DebugCmd>, rx: &Receiver<DebugResp>, state: &EmuState) {
    match cmd {
        parser::Cmd::Core(debug_cmd) => {
            tx.send(debug_cmd).unwrap();
            handle_debug_resp(&rx.recv().unwrap(), state);
        }
        parser::Cmd::UiHelp => print_help(),
        parser::Cmd::UiExit => state.shutdown(),
        parser::Cmd::End => {}
    }
}

fn eval_cmd(text: &str, tx: &Sender<DebugCmd>, rx: &Receiver<DebugResp>, state: &EmuState) {
    match parser::parse_cmd(&mut parser::tokenize(text).into_iter().peekable()) {
        Ok(cmd) => do_cmd(cmd, tx, rx, state),
        Err(msg) => println!("{color_red}{}{color_reset}", msg),
    }
}

fn print_registers(reg: &Registers) {
    println!("AF:{:04x} BC:{:06x} DE:{:06x} HL:{:06x} SPS:{:04x} SPL:{:06x} IX:{:06x} IY:{:06x} MB {:02x} ADL:{:01x} MADL:{:01x} IFF1:{}",
        reg.get16(Reg16::AF),
        reg.get24(Reg16::BC),
        reg.get24(Reg16::DE),
        reg.get24(Reg16::HL),
        reg.get16(Reg16::SP),
        reg.get24(Reg16::SP),
        reg.get24(Reg16::IX),
        reg.get24(Reg16::IY),
        reg.mbase,
        reg.adl as i32,
        reg.madl as i32,
        if reg.get_iff1() { '1' } else { '0' },
    );
}

fn handle_debug_resp(resp: &DebugResp, state: &EmuState) {
    match resp {
        DebugResp::Memory { start, data } => {
            let mut pos = *start;
            for chunk in &mut data.chunks(16) {
                print!("{:06x}: ", pos);
                for byte in chunk {
                    print!("{:02x} ", byte);
                }
                print!("| ");
                for byte in chunk {
                    let ch = if *byte >= 0x20 && byte.is_ascii() {
                        char::from_u32(*byte as u32).unwrap_or(' ')
                    } else {
                        ' '
                    };
                    print!("{}", ch);
                }
                println!();

                pos += 16;
            }
        }
        DebugResp::Message(s) => {
            println!("{color_yellow}{}{color_reset}", s);
        }
        DebugResp::Paused(reason) => {
            match reason {
                PauseReason::DebuggerRequested => {
                    println!("{color_yellow}CPU paused{color_reset}");
                }
                PauseReason::OutOfBoundsMemAccess(address) => {
                    println!(
                        "{color_yellow}CPU paused (memory 0x{:x} out of bounds){color_reset}",
                        address
                    );
                }
                PauseReason::DebuggerBreakpoint => {
                    println!("{color_yellow}CPU paused (breakpoint){color_reset}");
                }
                PauseReason::IOBreakpoint(io_address) => {
                    println!(
                        "{color_yellow}CPU paused (IO breakpoint 0x{:x}){color_reset}",
                        io_address
                    );
                }
            }
            state.set_in_debugger(true);
        }
        DebugResp::Resumed => {
            println!("{color_green}CPU running{color_reset}");
            state.set_in_debugger(false);
        }
        DebugResp::Triggers(bs) => {
            println!("Triggers:");
            for b in bs {
                println!(
                    "\t&{:06x} {:?}{}",
                    b.address,
                    b.actions,
                    if b.once { " (once)" } else { "" }
                );
            }
        }
        DebugResp::Pong => {}
        DebugResp::Disassembly { pc, adl, disasm } => {
            println!("\t.assume adl={}", if *adl { 1 } else { 0 });
            for inst in disasm {
                print!(
                    "{} {:06x}: {:20} |",
                    if inst.loc == *pc { "*" } else { " " },
                    inst.loc,
                    inst.asm
                );
                for byte in &inst.bytes {
                    print!(" {:02x}", byte);
                }
                println!();
            }
        }
        DebugResp::State {
            registers,
            stack,
            pc_instruction,
            ..
        } => {
            print!("* {:06x}: {:20} ", registers.pc, pc_instruction);
            print_registers(registers);
            if registers.adl {
                print!("{:30} SPL top ${:06x}:", "", registers.get24(Reg16::SP));
            } else {
                print!("{:30} SPS top ${:04x}:", "", registers.get16(Reg16::SP));
            }
            for byte in stack {
                print!(" {:02x}", byte);
            }
            println!();
        }
        DebugResp::Registers(registers) => {
            print!("PC={:06x} ", registers.pc);
            print_registers(registers);
        }
    }
}

fn drain_rx(rx: &Receiver<DebugResp>, state: &EmuState) {
    loop {
        if let Ok(resp) = rx.try_recv() {
            handle_debug_resp(&resp, state);
        } else {
            break;
        }
    }
}

pub fn start(
    tx: Sender<DebugCmd>,
    rx: Receiver<DebugResp>,
    emulator_shutdown: std::sync::Arc<std::sync::atomic::AtomicBool>,
    ez80_paused: bool,
) {
    let state = EmuState {
        ez80_paused: std::cell::Cell::new(ez80_paused),
        emulator_shutdown,
    };
    let tx_from_ctrlc = tx.clone();

    // should be able to get this from rl.history(), but couldn't figure out the API...
    let mut last_cmd: Option<String> = None;

    println!();
    println!("{style_bold}Agon EZ80 Debugger{style_reset}");
    println!();
    print_help();

    if state.is_in_debugger() {
        println!("{color_yellow}Interrupting execution.{color_reset}");
    }

    {
        let _state = state.clone();
        ctrlc::set_handler(move || {
            print!("\r");
            tx_from_ctrlc
                .send(DebugCmd::Pause(PauseReason::DebuggerRequested))
                .unwrap();
            tx_from_ctrlc.send(DebugCmd::GetState).unwrap();
        })
        .expect("Error setting Ctrl-C handler");
    }

    let prompt = &format!("{color_cyan}>> ");

    // `()` can be used when no completer is required
    let mut rl = DefaultEditor::new().unwrap();
    while !state.is_emulator_shutdown() {
        while state.is_in_debugger() {
            std::thread::sleep(std::time::Duration::from_millis(50));
            drain_rx(&rx, &state);
            let readline = rl.readline(prompt);
            print!("{color_reset}");
            match readline {
                Ok(line) => {
                    if line != "" {
                        rl.add_history_entry(line.as_str()).unwrap();
                        eval_cmd(&line, &tx, &rx, &state);

                        if state.is_in_debugger() {
                            last_cmd = Some(line);
                        } else {
                            last_cmd = None;
                        }
                    } else if let Some(ref l) = last_cmd {
                        eval_cmd(l, &tx, &rx, &state);
                        //line = rl.history().last();
                    }
                }
                Err(ReadlineError::Interrupted) => break,
                Err(ReadlineError::Eof) => {
                    do_cmd(parser::Cmd::Core(DebugCmd::Continue), &tx, &rx, &state);
                    break;
                }
                Err(err) => {
                    println!("{color_red}Error: {:?}{color_reset}", err);
                    break;
                }
            }
        }

        // when not reading debugger commands, periodically handle messages
        // from the CPU
        drain_rx(&rx, &state);
        std::thread::sleep(std::time::Duration::from_millis(50));
    }
}
