# Guide to compiling the Fab Agon Emulator

Before compiling (on all architectures), make sure the git submodules are up-to-date:

```
git submodule update --init
```

## Compiling for Linux

To make an optimised release build, run:

```
make
```

Then you can run the emulator with:

```
./fab-agon-emulator
```

You can also install to a prefix (eg /usr/local or $HOME/.local):

```
PREFIX=/usr/local make
sudo make install
```

## Compiling for Windows

### To build on Windows (mingw)

* Install rustup from https://rustup.rs/
* rustup toolchain install stable-x86_64-pc-windows-gnu
* rustup default stable-x86_64-pc-windows-gnu
* Install make winget install GnuWin32.Make
* add C:\Program Files (x86)\GnuWin32\bin to path
* Install mingw64 from https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0mcf-16.0.6-11.0.1-ucrt-r2/winlibs-x86_64-mcf-seh-gcc-13.2.0-llvm-16.0.6-mingw-w64ucrt-11.0.1-r2.zip
* extract to C:\mingw64 and add C:\mingw64\bin to path
* Install CoreUtils for Windows from https://sourceforge.net/projects/gnuwin32/files/coreutils/5.3.0/coreutils-5.3.0-bin.zip
* extract to C:\coreutils (or other location of your choice) and add C:\coreutils\bin to path
* get SDL2 from https://github.com/libsdl-org/SDL/releases/download/release-2.28.3/SDL2-devel-2.28.3-mingw.zip
* extract to C:\Users\<user>\.rustup\toolchains\stable-x86_64-pc-windows-gnu
* copy SDL2.dll to the root of the project
* run `make` in the root of the project
* run `fab-agon-emulator.exe` in the root of the project

## Compiling for Mac

Fab Agon Emulator can be compiled on Mac, and will generate a fat binary supporting
both Intel and Apple Silicon CPUs.

### Instructions

TODO
