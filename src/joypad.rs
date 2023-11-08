use agon_cpu_emulator::gpio::GpioSet;

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

pub fn on_button(gpios: &mut GpioSet, joystick_num: u32, button_idx: u8, is_down: bool) {
    let pin = 4 + (!(joystick_num as u8) & 1) + (button_idx & 1)*2;
    gpios.d.set_input_pin(pin, !is_down);
}
