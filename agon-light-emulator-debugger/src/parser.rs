use agon_cpu_emulator::debugger::{ DebugCmd, Trigger };

#[derive(Debug)]
pub enum Cmd {
    Core(DebugCmd),
    UiHelp,
    UiExit,
    End
}

type Tokens<'a> = std::iter::Peekable<std::vec::IntoIter<&'a str>>;

// trigger $40000 "hey" pause state
pub fn tokenize(line: &str) -> Vec<&str> {
    //line.split_whitespace().collect::<Vec<&str>>().into_iter()
    let mut l = line;
    let mut v: Vec<&str> = vec![];
    
    loop {
        let tok_start = match l.chars().position(|ch| !ch.is_whitespace()) {
            Some(p) => p,
            None => break
        };

        let tok_end = {
            match l.chars().nth(tok_start).unwrap() {
                ':' => tok_start + 1,
                '"' => {
                    match l.chars().skip(tok_start+1).position(|ch| ch == '"') {
                        Some(p) => tok_start + p + 2,
                        None => l.len()
                    }
                }
                _ => {
                    match l.chars().skip(tok_start).position(|ch| ch.is_whitespace() || ch == ':') {
                        Some(p) => tok_start + p,
                        None => l.len()
                    }
                }
            }
        };
        v.push(&l[tok_start..tok_end]);
        l = &l[tok_end..l.len()];
    }
    v
}

fn expect_end_of_cmd(tokens: &mut Tokens) -> Result<(), String> {
    match tokens.peek() {
        Some(&t) => {
            if t == ":" {
                Ok(())
            } else {
                Err(format!("Expected end of command but found '{}'", t))
            }
        }
        None => Ok(())
    }
}

pub fn parse_cmd(tokens: &mut Tokens) -> Result<Cmd, String> {
    if let Some(tok) = tokens.next() {
        match tok {
            "triggers" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::ListTriggers))
            }
            "trigger" => {
                if let Some(addr) = parse_number(tokens) {
                    let mut actions = vec![];
                    loop {
                        match parse_cmd(tokens)? {
                            Cmd::Core(a @ DebugCmd::AddTrigger(_)) => {
                                return Err(format!("Invalid action to trigger: {:?}", a));
                            }
                            Cmd::Core(a) => actions.push(a),
                            Cmd::End => break,
                            a => {
                                return Err(format!("Invalid action to trigger: {:?}", a));
                            }
                        }
                        if let Some(&t) = tokens.peek() {
                            if t != ":" { break }
                            else { tokens.next(); }
                        }
                    }
                    expect_end_of_cmd(tokens)?;
                    let trigger = DebugCmd::AddTrigger(Trigger {
                        address: addr,
                        once: false,
                        actions
                    });
                    Ok(Cmd::Core(trigger))
                } else {
                    Err(format!("trigger expects an address argument"))
                }
            }
            "pause" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::Pause))
            }
            "help" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::UiHelp)
            }
            "info" => {
                match tokens.next() {
                    Some("breakpoints") => {
                        expect_end_of_cmd(tokens)?;
                        Ok(Cmd::Core(DebugCmd::ListTriggers))
                    }
                    _ => Err("Unknown info type".to_string())
                }
            }
            "delete" => {
                if let Some(addr) = parse_number(tokens) {
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::DeleteTrigger(addr)))
                } else {
                    Err(format!("delete expects an address argument"))
                }
            }
            "br" | "break" => {
                if let Some(addr) = parse_number(tokens) {
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::AddTrigger(Trigger {
                        address: addr,
                        once: false,
                        actions: vec![
                            DebugCmd::Pause,
                            DebugCmd::Message("CPU paused at breakpoint".to_string()),
                            DebugCmd::GetState,
                        ]
                    })))
                } else {
                    Err(format!("break <address>"))
                }
            }
            "exit" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::UiExit)
            }
            "n" | "next" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::StepOver))
            }
            "s" | "step" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::Step))
            }
            "trace" => {
                if parse_exact(tokens, "on") {
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::SetTrace(true)))
                }
                else if parse_exact(tokens, "off") {
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::SetTrace(false)))
                }
                else {
                    Err(format!("expected 'on' or 'off'"))
                }
            }
            "registers" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::GetRegisters))
            }
            "mem" | "memory" => {
                let start_ = parse_number(tokens);
                if let Some(start) = start_ {
                    let len = parse_number(tokens).unwrap_or(16);
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::GetMemory { start, len }))
                } else {
                    Err(format!("mem <start> [len]"))
                }
            }
            "." | "state" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::GetState))
            }
            mode @ ("dis16" | "dis24" | "dis" | "disassemble") => {
                let adl = match mode {
                    "dis16" => Some(false),
                    "dis24" => Some(true),
                    _ => None
                };
                let start = parse_number(tokens);
                if let Some(start) = start {
                    let end = parse_number(tokens).unwrap_or(start + 0x20);
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::Disassemble { adl, start, end }))
                } else {
                    Ok(Cmd::Core(DebugCmd::DisassemblePc { adl }))
                }
            }
            "c" | "continue" => {
                expect_end_of_cmd(tokens)?;
                Ok(Cmd::Core(DebugCmd::Continue))
            }
            _ => {
                if tok.chars().nth(0) == Some('"') {
                    // string token
                    expect_end_of_cmd(tokens)?;
                    Ok(Cmd::Core(DebugCmd::Message(tok.to_string())))
                } else {
                    Err(format!("Unknown command: {}", tok))
                }
            }
        }
    } else {
        Ok(Cmd::End)
    }
}

fn parse_exact(tokens: &mut Tokens, expected: &str) -> bool {
    match tokens.peek() {
        Some(&s) if s == expected => {
            tokens.next();
            true
        }
        _ => false
    }
}

fn parse_number(tokens: &mut Tokens) -> Option<u32> {
    if let Some(&s) = tokens.peek() {
        let num = if s.starts_with('&') || s.starts_with('$') {
            u32::from_str_radix(s.get(1..s.len()).unwrap_or(""), 16).ok()
        }
        else if s.ends_with('h') || s.ends_with('H') {
            u32::from_str_radix(s.get(0..s.len()-1).unwrap_or(""), 16).ok()
        } else {
            u32::from_str_radix(s, 10).ok()
        };

        if num.is_some() {
            tokens.next();
        }
        num
    } else {
        None
    }
}

#[test]
fn test_tokenize() {
    assert_eq!(tokenize("   hello  world "), ["hello", "world"]);
    assert_eq!(tokenize(" &40cafe foo \n bar "), ["&40cafe", "foo", "bar"]);
    assert_eq!(tokenize(" \"string literals!\" and other stuff."), ["\"string literals!\"", "and", "other", "stuff."]);
    assert_eq!(tokenize("\"hello\":command :cmd2"), ["\"hello\"", ":", "command", ":", "cmd2"]);
}
