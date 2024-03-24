# TODO

## crash:
vdu 23 0 &88 250 0 50 0 0

## For 1.0

- [ ] symbol support in debugger
- [ ] uart1 host serial link

## Mañana mañana

- [ ] figure out ESP32 PSRAM accounting issues (C++ custom allocators)
- [ ] trs-os hang on "run auto command" -- IS CPU-emu BUG! Works with: real agon & emulated VDP!
- [ ] not all screen modes are 60Hz, but emulator assumes they are
- [ ] c8 joystick gpio mutex contention on windows -- probably use atomics instead
- [ ] vdp audio sample rate setting
- [ ] timestamps in hostfs
