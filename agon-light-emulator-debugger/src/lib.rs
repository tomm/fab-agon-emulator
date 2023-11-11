use std::sync::mpsc::{Sender, Receiver};
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;

mod parser;

use agon_cpu_emulator::debugger::{ DebugResp, DebugCmd, Registers, Reg16 };

#[derive(Clone)]
struct EmuState {
    pub in_debugger: std::sync::Arc<std::sync::atomic::AtomicBool>,
    pub emulator_shutdown: std::sync::Arc<std::sync::atomic::AtomicBool>,
}

impl EmuState {
    pub fn is_in_debugger(&self) -> bool {
        self.in_debugger.load(std::sync::atomic::Ordering::SeqCst)
    }

    pub fn set_in_debugger(&self, state: bool) {
        self.in_debugger.store(state, std::sync::atomic::Ordering::SeqCst);
    }

    pub fn is_emulator_shutdown(&self) -> bool {
        self.emulator_shutdown.load(std::sync::atomic::Ordering::SeqCst)
    }

    pub fn shutdown(&self) {
        self.emulator_shutdown.store(true, std::sync::atomic::Ordering::SeqCst);
        self.set_in_debugger(false);
    }
}

fn print_help() {
    println!("While CPU is running:");
    println!("<CTRL-C>                     Pause Agon CPU and enter debugger");
    println!();
    println!("While CPU is paused:");
    println!("br[eak] <address>            Set a breakpoint at the hex address");
    println!("c[ontinue]                   Resume (un-pause) Agon CPU");
    println!("delete <address>             Delete a breakpoint");
    println!("dis[assemble] [start] [end]  Disassemble in current ADL mode");
    println!("dis16 [start] [end]          Disassemble in ADL=0 (Z80) mode");
    println!("dis24 [start] [end]          Disassemble in ADL=1 (24-bit) mode");
    println!("exit                         Quit from Agon Light Emulator");
    println!("info breakpoints             List breakpoints");
    println!("[mem]ory <start> [len]       Dump memory");
    println!("n[ext]                       Step over function calls");
    println!("pause                        Pause execution and enter debugger");
    println!("state                        Show CPU state");
    println!(".                            Show CPU state");
    println!("s[tep]                       Execute one instuction");
    println!("trace on                     Enable logging every instruction");
    println!("trace off                    Disable logging every instruction");
    println!("trigger <address> cmd1 : cmd2 : ...");
    println!("    Perform debugger commands when <address> is reached");
    println!("    eg: break $123 is equivalent to:");
    println!("        trigger $123 pause:\"CPU paused at breakpoint\":state");
    println!();
    println!("triggers                     List triggers");
    println!();
    println!("The previous command can be repeated by pressing return.");
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
        Err(msg) => println!("{}", msg)
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
            println!("{}", s);
        }
        DebugResp::IsPaused(p) => {
            state.set_in_debugger(*p);
        }
        DebugResp::Triggers(bs) => {
            println!("Triggers:");
            for b in bs {
                println!("\t&{:06x} {:?}{}",
                         b.address,
                         b.actions,
                         if b.once { " (once)" } else { "" });
            }
        }
        DebugResp::Pong => {},
        DebugResp::Disassembly { pc, adl, disasm } => {
            println!("\t.assume adl={}", if *adl {1} else {0});
            for inst in disasm {
                print!("{} {:06x}: {:20} |",
                       if inst.loc == *pc { "*" } else { " " },
                       inst.loc,
                       inst.asm);
                for byte in &inst.bytes {
                    print!(" {:02x}", byte);
                }
                println!();
            }
        }
        DebugResp::State { registers, stack, pc_instruction, .. } => {
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

const PAUSE_AT_START: bool = true;

pub fn start(
    tx: Sender<DebugCmd>,
    rx: Receiver<DebugResp>,
    emulator_shutdown: std::sync::Arc<std::sync::atomic::AtomicBool>
) {
    let state = EmuState {
        in_debugger: std::sync::Arc::new(std::sync::atomic::AtomicBool::new(PAUSE_AT_START)),
        emulator_shutdown
    };
    let tx_from_ctrlc = tx.clone();

    // should be able to get this from rl.history(), but couldn't figure out the API...
    let mut last_cmd: Option<String> = None;

    println!("Agon Light Emulator Debugger");
    println!();
    print_help();
    if PAUSE_AT_START {
        println!("Interrupting execution.");
    }

    {
        let _state = state.clone();
        ctrlc::set_handler(move || {
            _state.set_in_debugger(true);
            println!("Interrupting execution.");
            tx_from_ctrlc.send(DebugCmd::Pause).unwrap();
            tx_from_ctrlc.send(DebugCmd::GetState).unwrap();
        }).expect("Error setting Ctrl-C handler");
    }

    // `()` can be used when no completer is required
    let mut rl = DefaultEditor::new().unwrap();
    while !state.is_emulator_shutdown() {
        while state.is_in_debugger() {
            drain_rx(&rx, &state);
            let readline = rl.readline(">> ");
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
                    } else if let Some (ref l) = last_cmd {
                        eval_cmd(l, &tx, &rx, &state);
                        //line = rl.history().last();
                    }
                },
                Err(ReadlineError::Interrupted) => {
                    break
                },
                Err(ReadlineError::Eof) => {
                    do_cmd(parser::Cmd::Core(DebugCmd::Continue), &tx, &rx, &state);
                    break
                },
                Err(err) => {
                    println!("Error: {:?}", err);
                    break
                }
            }
        }

        // when not reading debugger commands, periodically handle messages
        // from the CPU
        drain_rx(&rx, &state);
        std::thread::sleep(std::time::Duration::from_millis(50));
    }
}
