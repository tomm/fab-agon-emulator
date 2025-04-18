pub fn is_not_ascii(scancode: sdl2::keyboard::Scancode) -> bool {
    match scancode {
        sdl2::keyboard::Scancode::Backspace |
        sdl2::keyboard::Scancode::Tab |
        sdl2::keyboard::Scancode::CapsLock |
        sdl2::keyboard::Scancode::Return |
        sdl2::keyboard::Scancode::LShift |
        sdl2::keyboard::Scancode::RShift |
        sdl2::keyboard::Scancode::LCtrl |
        sdl2::keyboard::Scancode::LAlt |
        sdl2::keyboard::Scancode::RAlt |
        sdl2::keyboard::Scancode::RCtrl |
        sdl2::keyboard::Scancode::Insert |
        sdl2::keyboard::Scancode::Delete |
        sdl2::keyboard::Scancode::Left |
        sdl2::keyboard::Scancode::Home |
        sdl2::keyboard::Scancode::End |
        sdl2::keyboard::Scancode::Up |
        sdl2::keyboard::Scancode::Down |
        sdl2::keyboard::Scancode::PageUp |
        sdl2::keyboard::Scancode::PageDown |
        sdl2::keyboard::Scancode::Right |
        // numlock
        sdl2::keyboard::Scancode::KpEnter |
        sdl2::keyboard::Scancode::Escape |
        sdl2::keyboard::Scancode::F1 |
        sdl2::keyboard::Scancode::F2 |
        sdl2::keyboard::Scancode::F3 |
        sdl2::keyboard::Scancode::F4 |
        sdl2::keyboard::Scancode::F5 |
        sdl2::keyboard::Scancode::F6 |
        sdl2::keyboard::Scancode::F7 |
        sdl2::keyboard::Scancode::F8 |
        sdl2::keyboard::Scancode::F9 |
        sdl2::keyboard::Scancode::F10 |
        sdl2::keyboard::Scancode::F11 |
        sdl2::keyboard::Scancode::F12 => true,
        _ => false,
    }
}

/**
 * Convert SDL scancodes to PS/2 set 2 scancodes.
 */
pub fn sdl2ps2(scancode: sdl2::keyboard::Scancode) -> u16 {
    match scancode {
        sdl2::keyboard::Scancode::Grave => 0x0e,
        sdl2::keyboard::Scancode::Num1 => 0x16,
        sdl2::keyboard::Scancode::Num2 => 0x1e,
        sdl2::keyboard::Scancode::Num3 => 0x26,
        sdl2::keyboard::Scancode::Num4 => 0x25,
        sdl2::keyboard::Scancode::Num5 => 0x2e,
        sdl2::keyboard::Scancode::Num6 => 0x36,
        sdl2::keyboard::Scancode::Num7 => 0x3d,
        sdl2::keyboard::Scancode::Num8 => 0x3e,
        sdl2::keyboard::Scancode::Num9 => 0x46,
        sdl2::keyboard::Scancode::Num0 => 0x45,
        sdl2::keyboard::Scancode::Minus => 0x4e,
        sdl2::keyboard::Scancode::Equals => 0x55,
        sdl2::keyboard::Scancode::Backspace => 0x66,
        sdl2::keyboard::Scancode::Tab => 0x0d,
        sdl2::keyboard::Scancode::Q => 0x15,
        sdl2::keyboard::Scancode::W => 0x1d,
        sdl2::keyboard::Scancode::E => 0x24,
        sdl2::keyboard::Scancode::R => 0x2d,
        sdl2::keyboard::Scancode::T => 0x2c,
        sdl2::keyboard::Scancode::Y => 0x35,
        sdl2::keyboard::Scancode::U => 0x3C,
        sdl2::keyboard::Scancode::I => 0x43,
        sdl2::keyboard::Scancode::O => 0x44,
        sdl2::keyboard::Scancode::P => 0x4d,
        sdl2::keyboard::Scancode::LeftBracket => 0x54,
        sdl2::keyboard::Scancode::RightBracket => 0x5b,
        sdl2::keyboard::Scancode::CapsLock => 0x58,
        sdl2::keyboard::Scancode::A => 0x1c,
        sdl2::keyboard::Scancode::S => 0x1b,
        sdl2::keyboard::Scancode::D => 0x23,
        sdl2::keyboard::Scancode::F => 0x2b,
        sdl2::keyboard::Scancode::G => 0x34,
        sdl2::keyboard::Scancode::H => 0x33,
        sdl2::keyboard::Scancode::J => 0x3b,
        sdl2::keyboard::Scancode::K => 0x42,
        sdl2::keyboard::Scancode::L => 0x4b,
        sdl2::keyboard::Scancode::Semicolon => 0x4c,
        sdl2::keyboard::Scancode::Apostrophe => 0x52,
        sdl2::keyboard::Scancode::Return => 0x5a,
        sdl2::keyboard::Scancode::LShift => 0x12,
        sdl2::keyboard::Scancode::Z => 0x1a,
        sdl2::keyboard::Scancode::X => 0x22,
        sdl2::keyboard::Scancode::C => 0x21,
        sdl2::keyboard::Scancode::V => 0x2a,
        sdl2::keyboard::Scancode::B => 0x32,
        sdl2::keyboard::Scancode::N => 0x31,
        sdl2::keyboard::Scancode::M => 0x3a,
        sdl2::keyboard::Scancode::Comma => 0x41,
        sdl2::keyboard::Scancode::Period => 0x49,
        sdl2::keyboard::Scancode::Slash => 0x4a,
        sdl2::keyboard::Scancode::RShift => 0x59,
        sdl2::keyboard::Scancode::LCtrl => 0x14,
        sdl2::keyboard::Scancode::LAlt => 0x11,
        sdl2::keyboard::Scancode::Space => 0x29,
        sdl2::keyboard::Scancode::RAlt => 0xe011,
        sdl2::keyboard::Scancode::RCtrl => 0xe014,
        sdl2::keyboard::Scancode::Insert => 0xe070,
        sdl2::keyboard::Scancode::Delete => 0xe071,
        sdl2::keyboard::Scancode::Left => 0xe06b,
        sdl2::keyboard::Scancode::Home => 0xe06c,
        sdl2::keyboard::Scancode::End => 0xe069,
        sdl2::keyboard::Scancode::Up => 0xe075,
        sdl2::keyboard::Scancode::Down => 0xe072,
        sdl2::keyboard::Scancode::PageUp => 0xe07d,
        sdl2::keyboard::Scancode::PageDown => 0xe07a,
        sdl2::keyboard::Scancode::Right => 0xe074,
        sdl2::keyboard::Scancode::NumLockClear => 0x77,
        sdl2::keyboard::Scancode::Kp7 => 0x6c,
        sdl2::keyboard::Scancode::Kp4 => 0x6b,
        sdl2::keyboard::Scancode::Kp1 => 0x69,
        sdl2::keyboard::Scancode::KpDivide => 0xe04a,
        sdl2::keyboard::Scancode::Kp8 => 0x75,
        sdl2::keyboard::Scancode::Kp5 => 0x73,
        sdl2::keyboard::Scancode::Kp2 => 0x72,
        sdl2::keyboard::Scancode::Kp0 => 0x70,
        sdl2::keyboard::Scancode::KpMultiply => 0x7c,
        sdl2::keyboard::Scancode::Kp9 => 0x7d,
        sdl2::keyboard::Scancode::Kp6 => 0x74,
        sdl2::keyboard::Scancode::Kp3 => 0x7a,
        sdl2::keyboard::Scancode::KpPeriod => 0x71,
        sdl2::keyboard::Scancode::KpMinus => 0x7b,
        sdl2::keyboard::Scancode::KpPlus => 0x79,
        sdl2::keyboard::Scancode::KpEnter => 0xe05a,
        sdl2::keyboard::Scancode::Escape => 0x76,
        sdl2::keyboard::Scancode::F1 => 0x05,
        sdl2::keyboard::Scancode::F2 => 0x06,
        sdl2::keyboard::Scancode::F3 => 0x04,
        sdl2::keyboard::Scancode::F4 => 0x0c,
        sdl2::keyboard::Scancode::F5 => 0x03,
        sdl2::keyboard::Scancode::F6 => 0x0b,
        sdl2::keyboard::Scancode::F7 => 0x83,
        sdl2::keyboard::Scancode::F8 => 0x0a,
        sdl2::keyboard::Scancode::F9 => 0x01,
        sdl2::keyboard::Scancode::F10 => 0x09,
        sdl2::keyboard::Scancode::F11 => 0x78,
        sdl2::keyboard::Scancode::F12 => 0x07,
        sdl2::keyboard::Scancode::PrintScreen => 0xe07c, // kinda. good enough for fabgl
        sdl2::keyboard::Scancode::ScrollLock => 0x7e,
        sdl2::keyboard::Scancode::Pause => 0x62,
        // wrong. pause=0x62 is set3, not set2. I use this as pause in set2 is a pain in the arse 8 byte sequence
        sdl2::keyboard::Scancode::Backslash => 0x5d,
        sdl2::keyboard::Scancode::NonUsBackslash => 0x61,
        _ => 0,
    }
}
