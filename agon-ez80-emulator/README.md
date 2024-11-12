# Agon CPU Emulator

An emulator of the Agon Light CPU (eZ80F92) and CPU-controlled peripherals,
with host filesystem integration.

For a complete Agon Light emulator, including the VDP (ESP32) side of the system,
see [Fab Agon Emulator](https://github.com/tomm/fab-agon-emulator),
which uses this crate combined with a graphical (SDL) Agon VDP emulator.

This Agon CPU Emulator can be used stand-alone, as a text-only emulation
of Agon Light:

```
cargo run --release
```

This will pass stdin input to the Agon CPU as VDP keypress packets.
You can use this for a scripted Agon emulation:

```
$ echo "credits
cat" | cargo run -r
Tom's Fake VDP Version 1.03
Agon Quark MOS Version 1.03

*credits
FabGL 1.0.8 (c) 2019-2022 by Fabrizio Di Vittorio
FatFS R0.14b (c) 2021 ChaN

*cat
Volume: hostfs

1980/00/0000:00 D     4096 agon_regression_suite
1980/00/0000:00       1518 LICENSE
1980/00/0000:00 D     4096 src
1980/00/0000:00 D     4096 target
1980/00/0000:00 D     4096 basic
1980/00/0000:00        234 Cargo.lock
```

This is how the regression suite is implemented. You can run the
regression suite with:

```
sh ./run_regression_test.sh
```

## TODO

- [ ] Timer clock sources other than the system clock
- [ ] Real SPI sdcard support
