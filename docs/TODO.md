# TODO

## Known bugs
- [ ] This (setting key repeat rates) causes a crash: *vdu 23 0 &88 250 0 50 0 0
- [ ] Mixed mode push ((ismixed<<1) | isadl) to stack. I haven't implemented the ismixed part
- [ ] c8 joystick gpio mutex contention on windows -- probably use atomics instead
- [ ] trs-os hang on "run auto command" -- IS CPU-emu BUG! Works with: real agon & emulated VDP!
- [ ] ez80f92 PRT gpio-b pin 1 source (vblank) not implemented

## VDP flaws and omissions
- [ ] ESP32 RAM is unlimited. Figure out ESP32 PSRAM accounting issues (C++ custom allocators)
- [ ] vdp audio sample rate setting not implemented
- [ ] VDP keyboard shortcircuits the queue of ps2 scancodes, and instead injects vkeys. this causes issues (eg. PAUSE key is erroneously subject to key repeat)

## Feature ideas
- [ ] symbol support in debugger
- [ ] uart1 host serial link (some work done but untested)
- [ ] gdbserver support?
