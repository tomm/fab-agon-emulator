/// Interface for a debugger
///
use ez80::{ Environment, Machine };
use std::sync::mpsc::{Sender, Receiver};
use std::sync::mpsc;
use crate::AgonMachine;

pub struct DebuggerConnection {
    pub tx: Sender<DebugResp>,
    pub rx: Receiver<DebugCmd>
}

pub type Registers = ez80::Registers;
pub type Reg8 = ez80::Reg8;
pub type Reg16 = ez80::Reg16;

#[derive(Debug, Clone)]
pub enum DebugCmd {
    Ping,
    Pause,
    Continue,
    Step,
    StepOver,
    SetTrace(bool),
    Message(String),
    AddTrigger(Trigger),
    DeleteTrigger(u32),
    ListTriggers,
    GetMemory { start: u32, len: u32 },
    GetRegisters,
    GetState,
    DisassemblePc { adl: Option<bool> },
    Disassemble { adl: Option<bool>, start: u32, end: u32 },
}

#[derive(Debug)]
pub enum DebugResp {
    IsPaused(bool),
    Pong,
    Message(String),
    Registers(Registers),
    State {
        registers: Registers,
        instructions_executed: u64,
        stack: [u8; 16],
        pc_instruction: String,
    },
    Memory {
        start: u32,
        data: Vec<u8>
    },
    // (start, disasm, bytes)
    Disassembly {
        pc: u32,
        adl: bool,
        disasm: Vec<ez80::disassembler::Disasm>
    },
    Triggers(Vec<Trigger>),
}

#[derive(Debug,Clone)]
pub struct Trigger {
    pub address: u32,
    pub once: bool,
    pub actions: Vec<DebugCmd>
}

pub struct DebuggerServer {
    con: DebuggerConnection,
    triggers: Vec<Trigger>,
}

impl DebuggerServer {
    pub fn new(con: DebuggerConnection) -> Self {
        DebuggerServer { con, triggers: vec![] }
    }

    fn on_out_of_bounds(&mut self, machine: &mut AgonMachine, cpu: &mut ez80::Cpu) -> bool {
        if let Some(address) = machine.mem_out_of_bounds.get() {
            self.con.tx.send(DebugResp::IsPaused(true)).unwrap();
            self.con.tx.send(DebugResp::Message(format!("Out of bounds memory access at PC=${:x}: ${:x}", machine.last_pc, address))).unwrap();
            self.send_disassembly(machine, cpu, None, machine.last_pc, machine.last_pc+1);
            self.send_state(machine, cpu);

            machine.paused = true;
            true
        } else {
            false
        }
    }

    /// Called before each instruction is executed
    pub fn tick(&mut self, machine: &mut AgonMachine, cpu: &mut ez80::Cpu) {
        let pc = cpu.state.pc();

        // catch out of bounds memory accesses
        self.on_out_of_bounds(machine, cpu);

        // check triggers
        if !machine.paused {
            let to_run: Vec<Trigger> = self
                .triggers
                .iter()
                .filter(|t| t.address == pc)
                .map(|f| f.clone())
                .collect();

            for t in to_run {
                for a in &t.actions {
                    self.handle_debug_cmd(a, machine, cpu);
                }
            }
        }

        // delete triggers that are only to execute once
        self.triggers.retain(|b| !(b.address == pc && b.once));

        loop {
            match self.con.rx.try_recv() {
                Ok(cmd) => self.handle_debug_cmd(&cmd, machine, cpu),
                Err(mpsc::TryRecvError::Disconnected) => break,
                Err(mpsc::TryRecvError::Empty) => break,
            }
        }
        
        // clear any out of bounds memory access marker (either from z80 code
        // prior to this function's invocation, or caused by memory accesses
        // of the debugger here.
        machine.mem_out_of_bounds.set(None);
    }

    fn handle_debug_cmd(&mut self, cmd: &DebugCmd, machine: &mut AgonMachine, cpu: &mut ez80::Cpu) {
        let pc = cpu.state.pc();

        match cmd {
            DebugCmd::Message(s) => {
                self.con.tx.send(DebugResp::Message(s.clone())).unwrap()
            }
            DebugCmd::SetTrace(b) => {
                cpu.set_trace(*b);
                self.con.tx.send(DebugResp::Pong).unwrap();
            }
            DebugCmd::ListTriggers => {
                self.con.tx.send(DebugResp::Triggers(self.triggers.clone())).unwrap();
            }
            DebugCmd::DisassemblePc { adl } => {
                let start = cpu.state.pc();
                let end = start + 0x20;
                self.send_disassembly(machine, cpu, *adl, start, end);
            }
            DebugCmd::Disassemble { adl, start, mut end } => {
                if end <= *start { end = *start + 0x20 };
                self.send_disassembly(machine, cpu, *adl, *start, end);
            }
            DebugCmd::StepOver => {
                // if the opcode at PC is a call, set a 'once' breakpoint on the
                // instruction after it
                let prefix_override = match machine.peek(pc) {
                    0x52 | 0x5b => Some(true),
                    0x40 | 0x49 => Some(false),
                    _ => None
                };
                match machine.peek(if prefix_override.is_some() { pc+1 } else { pc }) {
                    // RST instruction at (pc)
                    0xc7 | 0xd7 | 0xe7 | 0xf7 | 0xcf | 0xdf | 0xef | 0xff => {
                        let addr_next = pc + (if prefix_override.is_some() { 2 } else { 1 });
                        self.triggers.push(Trigger {
                            address: addr_next,
                            once: true,
                            actions: vec![
                                DebugCmd::Pause,
                                DebugCmd::Message("Stepped over RST".to_string()),
                                DebugCmd::GetState,
                            ]
                        });
                        machine.paused = false;
                        self.con.tx.send(DebugResp::IsPaused(false)).unwrap();
                    }
                    // CALL instruction at (pc)
                    0xc4 | 0xd4 | 0xe4 | 0xf4 | 0xcc | 0xcd | 0xdc | 0xec | 0xfc => {
                        let addr_next = pc + match prefix_override {
                            Some(true) => 5, // opcode + prefix byte + 3 byte immediate
                            Some(false) => 4,
                            _ => if cpu.state.reg.adl { 4 } else { 3 }
                        };
                        self.triggers.push(Trigger {
                            address: addr_next,
                            once: true,
                            actions: vec![
                                DebugCmd::Pause,
                                DebugCmd::Message("Stepped over CALL".to_string()),
                                DebugCmd::GetState,
                            ]
                        });
                        machine.paused = false;
                        self.con.tx.send(DebugResp::IsPaused(false)).unwrap();
                    }
                    // other instructions. just step
                    _ => {
                        machine.paused = false;
                        machine.execute_instruction(cpu);
                        machine.paused = true;
                        self.send_state(machine, cpu);
                    }
                }

            }
            DebugCmd::Step => {
                machine.paused = false;
                machine.execute_instruction(cpu);
                machine.paused = true;
                self.send_state(machine, cpu);
            }
            DebugCmd::Pause => {
                machine.paused = true;
                self.con.tx.send(DebugResp::IsPaused(true)).unwrap();
            }
            DebugCmd::Continue => {
                machine.mem_out_of_bounds.set(None);
                machine.paused = false;
                // force one instruction to be executed, just to
                // get over any breakpoint on the current PC
                machine.execute_instruction(cpu);

                if !self.on_out_of_bounds(machine, cpu) {
                    self.con.tx.send(DebugResp::IsPaused(false)).unwrap();
                }
            }
            DebugCmd::AddTrigger(t) => {
                self.triggers.push(t.clone());
                self.con.tx.send(DebugResp::Pong).unwrap();
            }
            DebugCmd::DeleteTrigger(addr) => {
                self.triggers.retain(|b| b.address != *addr);
                self.con.tx.send(DebugResp::Pong).unwrap();
            }
            DebugCmd::Ping => self.con.tx.send(DebugResp::Pong).unwrap(),
            DebugCmd::GetRegisters => self.send_registers(cpu),
            DebugCmd::GetState => self.send_state(machine, cpu),
            DebugCmd::GetMemory { start, len } => {
                self.send_mem(machine, cpu, *start, *len);
            }
        }
    }

    fn send_disassembly(&self, machine: &mut AgonMachine, cpu: &mut ez80::Cpu, adl_override: Option<bool>, start: u32, end: u32) {
        let dis = ez80::disassembler::disassemble(machine, cpu, adl_override, start, end);

        self.con.tx.send(DebugResp::Disassembly {
            pc: cpu.state.pc(),
            adl: cpu.state.reg.adl,
            disasm: dis
        }).unwrap();
    }

    fn send_mem(&self, machine: &mut AgonMachine, cpu: &mut ez80::Cpu, start: u32, len: u32) {
        let env = Environment::new(&mut cpu.state, machine);
        let mut data = vec![];

        for i in start..start+len {
            data.push(env.peek(i));
        }

        self.con.tx.send(DebugResp::Memory { start, data }).unwrap();
    }

    fn send_state(&self, machine: &mut AgonMachine, cpu: &mut ez80::Cpu) {
        let mut stack: [u8; 16] = [0; 16];
        let pc_instruction: String;

        let sp: u32 = if cpu.registers().adl {
            cpu.registers().get24(ez80::Reg16::SP)
        } else {
            cpu.state.reg.get16_mbase(ez80::Reg16::SP)
        };

        {
            let env = Environment::new(&mut cpu.state, machine);
            for i in 0..16 {
                stack[i] = env.peek(sp + i as u32);
            }

            // iz80 (which ez80 is based on) doesn't allow disassembling
            // without advancing the PC, so we hack around this
            let pc = cpu.state.pc();
            pc_instruction = cpu.disasm_instruction(machine);
            cpu.state.set_pc(pc);
        }

        self.con.tx.send(DebugResp::State {
            registers: cpu.registers().clone(),
            instructions_executed: cpu.state.instructions_executed,
            stack,
            pc_instruction,
        }).unwrap();
    }

    fn send_registers(&self, cpu: &mut ez80::Cpu) {
        self.con.tx.send(DebugResp::Registers(cpu.registers().clone())).unwrap();
    }
}
