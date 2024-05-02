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

### To build on Windows (MSYS2)

* Download [MSYS2](https://www.msys2.org/) and follow the instructions to install it.
* Download the `msys-init.sh` script from this repo (making sure to download the raw version instead of the html from github).
* Start MSYS2 in UCRT64 mode (there is a separate icon for each mode in the start menu, but you can just search for UCRT64).
* Run the init script with bash: `bash msys-init.sh`.  It will update MSYS2 (probably requiring a restart), and then install all the build dependencies for the emulator.
* run `make` in the root of the project
* run `fab-agon-emulator.exe` in the root of the project

## Compiling for Mac

Fab Agon Emulator can be compiled on Mac, and will generate a fat binary supporting
both Intel and Apple Silicon CPUs.

### Instructions

TODO
