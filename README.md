# Fab Agon Emulator

An emulator for the Agon Light, Agon Light 2, and Agon Console8 8-bit computers.

## Keyboard Shortcuts

Emulator shortcuts are accessed with the *right alt* key (AltGr on some keyboards).

 * RightAlt-F - Toggle fullscreen mode
 * RightAlt-Q - Quit

## Compiling the Fab Agon Emulator

To make an optimised release build, run:

```
cargo build -r
```

Then you can run the emulator with either:

```
cargo run -r
```

or

```
./target/release/fab-agon-emulator
```

## Changing VDP version

By default, Fab Agon Emulator is compiled with Console8 VDP firmware.
To build with Quark VDP 1.03 firmware, run:

```
VDP_VERSION=quark103 cargo build -r
```

If you want to quickly switch between VDP versions, the best option currently
is to keep a fab-agon-emulator binary for each VDP version you wish to use.

## Changing MOS version

To start the emulator using a different MOS binary:

```
./target/release/fab-agon-emulator --mos my_mos.bin
```

Note that you will need a .map file with the same name (ie my_mos.map), or
host filesystem integration will not be enabled.

## Other command-line options

Read about other command-line options with:

```
./target/release/fab-agon-emulator --help
```
