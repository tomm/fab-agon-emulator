# TODO

## For 1.0

- [ ] symbol support in debugger
- [ ] shutdown hangs:
  - [ ] on firmware 1.03
  - [ ] on firmware 1.04
  - [ ] in terminal mode
- [ ] consider what to do about firmware paths (eg on linux installing to /usr/local)
- [ ] finish electron firmware merge
- [x] playmod slow playback issue
- [ ] enforce uart0 transfer rate limit

## Mañana mañana

- [ ] figure out ESP32 PSRAM accounting issues (C++ custom allocators)
- [ ] trs-os hang on "run auto command" -- IS CPU-emu BUG! Works with: real agon & emulated VDP!
- [ ] DEBUG logging for VDP
- [ ] not all screen modes are 60Hz, but emulator assumes they are
- [ ] c8 joystick gpio mutex contention on windows -- probably use atomics instead
