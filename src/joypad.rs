use agon_cpu_emulator::gpio::GpioSet;
use sdl2::joystick::HatState;

const PIN_PORT1_DPAD_UP: u8 = 1;
const PIN_PORT1_DPAD_RIGHT: u8 = 7;
const PIN_PORT1_DPAD_DOWN: u8 = 3;
const PIN_PORT1_DPAD_LEFT: u8 = 5;

// note - joypad gpios are pulled high, so depressed state is 'false'

// set gpio joypad inputs to open switch position (high)
pub fn clear_state(gpios: &mut GpioSet) {
    gpios.c.set_input_pins(0xff);
    gpios.d.set_input_pin(4, true);
    gpios.d.set_input_pin(5, true);
    gpios.d.set_input_pin(6, true);
    gpios.d.set_input_pin(7, true);
}

pub fn on_axis_motion(gpios: &mut GpioSet, joystick_num: u32, axis_idx: u8, value: i16) {
    if axis_idx > 1 { return }

    let pin_offset = (!(joystick_num as u8) & 1) + if axis_idx == 0 { 4 } else { 0 };

    gpios.c.set_input_pin(0 + pin_offset, value > -16384);
    gpios.c.set_input_pin(2 + pin_offset, value < 16384);
}

pub fn on_hat_motion(gpios: &mut GpioSet, joystick_num: u32, hat_idx: u8, state: HatState) {
    // The pins of joystick port2 can be retrieved by subtracting 1 from the corresponding
    // pin of joystick port1. The pin_offset should toggle between 0 and 1 for increasing
    // joystick_nums, so every second joystick_num input registers for player2.
    let pin_offset = joystick_num as u8 & 1;

    match state {
        HatState::Centered => {
            gpios.c.set_input_pin(PIN_PORT1_DPAD_UP - pin_offset, true);
            gpios.c.set_input_pin(PIN_PORT1_DPAD_RIGHT - pin_offset, true);
            gpios.c.set_input_pin(PIN_PORT1_DPAD_DOWN - pin_offset, true);
            gpios.c.set_input_pin(PIN_PORT1_DPAD_LEFT - pin_offset, true);
        },
        HatState::Up => gpios.c.set_input_pin(PIN_PORT1_DPAD_UP - pin_offset, false),
        HatState::Right => gpios.c.set_input_pin(PIN_PORT1_DPAD_RIGHT - pin_offset, false),
        HatState::Down => gpios.c.set_input_pin(PIN_PORT1_DPAD_DOWN - pin_offset, false),
        HatState::Left => gpios.c.set_input_pin(PIN_PORT1_DPAD_LEFT - pin_offset, false),
        _ => {}
    }
}

pub fn on_button(gpios: &mut GpioSet, joystick_num: u32, button_idx: u8, is_down: bool) {
    let pin = 4 + (!(joystick_num as u8) & 1) + (button_idx & 1)*2;
    gpios.d.set_input_pin(pin, !is_down);
}
