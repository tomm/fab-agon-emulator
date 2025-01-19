# TODO

## crash:
vdu 23 0 &88 250 0 50 0 0

## For 1.0

- [ ] symbol support in debugger
- [ ] uart1 host serial link
- [ ] gdbserver support?
- [ ] prt gpio-b pin 1 source (vblank)

## CPU bugs

- [ ] Mixed mode push ((ismixed<<1) | isadl) to stack. I haven't implemented the ismixed part

## Mañana mañana

- [ ] figure out ESP32 PSRAM accounting issues (C++ custom allocators)
- [ ] trs-os hang on "run auto command" -- IS CPU-emu BUG! Works with: real agon & emulated VDP!
- [ ] c8 joystick gpio mutex contention on windows -- probably use atomics instead
- [ ] vdp audio sample rate setting
