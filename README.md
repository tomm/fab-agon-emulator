# Fab Agon Emulator

An emulator of the Agon Light, Agon Light 2, and Agon Console8 8-bit computers.

## How to compile

You may not need to compile, as there are regular pre-compiled
[releases](https://github.com/tomm/fab-agon-emulator/releases)
for Linux (amd64), Windows (x64) and Mac (Intel & ARM).

Otherwise, read the [guide to compiling Fab Agon Emulator](./docs/compiling.md)

## Keyboard Shortcuts

Emulator shortcuts are accessed with the *right ctrl*.

 * RightCtrl-C - Toggle caps-lock
 * RightCtrl-F - Toggle fullscreen mode
 * RightCtrl-M - Print ESP32 memory stats to the console
 * RightCtrl-R - Soft-reset
 * RightCtrl-S - Cycle screen scaling methods (see --scale command line option)
 * RightCtrl-Q - Quit

## Emulated SDCard

If a directory is specified with `fab-agon-emulator --sdcard <dir>` then that will
be used as the emulated SDCard. Otherwise, the `.agon-sdcard/` directory in your
home directory will be used if present, and if not then `sdcard/` in the current
directory is used.

Alternatively you can use SDCard images (full MBR partitioned images, or raw
FAT32 images), with the --sdcard-img option.

## Changing VDP version

By default, Fab Agon Emulator boots with Console8 firmware. To start up
with quark firmware, run:

```
fab-agon-emulator --firmware quark
```

Legacy 1.03 firmware is also available:

```
fab-agon-emulator --firmware 1.03
```

And Electron firmware:

```
fab-agon-emulator --firmware electron
```

## The Z80 debugger

Start the emulator with the `-d` or `--debugger` option to enable the Z80
debugger:

```
fab-agon-emulator -d
```

At the debugger prompt (which will be in the terminal window you invoked the
emulator from), type `help` for instructions on the use of the debugger.

## Debug IO space

Some IO addresses unused by the EZ80F92 are used by the emulator for debugging
purposes:

| IO addresses  | Function                                                           |
| ------------- | ------------------------------------------------------------------ |
| 0x00          | Terminate emulator (exit code will be the value written to IO 0x0) |
| 0x10-0x1f     | Breakpoint (requires --debugger)                                   |
| 0x20-0x2f     | Print CPU state (requires --debugger)                              |

These functions are activated by write (not read), and the upper 8-bits of the
IO address are ignored. ie:

```
	out (0),a
```

will shut down the emulator.

## Other command-line options

Read about other command-line options with:

```
fab-agon-emulator --help
```

## Mac-specific issues

The Fab Agon Emulator executables provided on [releases](https://github.com/tomm/fab-agon-emulator/releases)
are not signed, so in order to run them on your Mac you need to run the following command from
the directory containing the fab-agon-emulator executable:

```
xattr -dr com.apple.quarantine fab-agon-emulator firmware/*.so
```
