# Fab Agon Emulator

An emulator for the Agon Light, Agon Light 2, and Agon Console8 8-bit computers.

## Keyboard Shortcuts

Emulator shortcuts are accessed with the *right alt* key (AltGr on some keyboards).

 * RightAlt-C - Toggle caps-lock
 * RightAlt-F - Toggle fullscreen mode
 * RightAlt-Q - Quit

## Compiling the Fab Agon Emulator (on Linux)

To make an optimised release build, run:

```
git submodule update --init
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

By default, Fab Agon Emulator boots with 1.04 firmware. To start up
with 1.03 firmware, run:

```
./target/release/fab-agon-emulator --firmware 1.03
```

## Other command-line options

Read about other command-line options with:

```
./target/release/fab-agon-emulator --help
```

## Compiling for Windows

### To cross-compile on a Linux machine

Read [make_win64_dist.sh](./make_win64_dist.sh).

### To build on Windows

TODO
