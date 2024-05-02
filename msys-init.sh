#!/bin/sh
echo "MSYS2 may ask to restart after this update. If so, rerun this script afterwards."
pacman --noconfirm -Syu
echo "Installing build dependencies for fab-agon-emulator..."
pacman --noconfirm -S \
    vim \
    git \
    make \
    ucrt64/mingw-w64-ucrt-x86_64-SDL2 \
    ucrt64/mingw-w64-ucrt-x86_64-gcc \
    ucrt64/mingw-w64-ucrt-x86_64-rust
git clone https://github.com/tomm/fab-agon-emulator.git