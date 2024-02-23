# TODO

## For 1.0

- [ ] symbol support in debugger
- [ ] enforce uart0 transfer rate limit

## Mañana mañana

- [ ] figure out ESP32 PSRAM accounting issues (C++ custom allocators)
- [ ] trs-os hang on "run auto command" -- IS CPU-emu BUG! Works with: real agon & emulated VDP!
- [ ] not all screen modes are 60Hz, but emulator assumes they are
- [ ] c8 joystick gpio mutex contention on windows -- probably use atomics instead
