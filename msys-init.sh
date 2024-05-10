#!/bin/sh
REPO=${REPO:-https://github.com/tomm/fab-agon-emulator.git}
set -euo pipefail # bash strict mode: https://gist.github.com/mohanpedala/1e2ff5661761d3abd0385e8223e16425

echo "
⚠️ MSYS2 may ask to restart after this update. If so, rerun this script afterwards.
"
pacman --noconfirm -Syu

echo "
⚙️ Installing build dependencies for fab-agon-emulator...
"
pacman --noconfirm -S \
    vim \
    git \
    make \
    ucrt64/mingw-w64-ucrt-x86_64-SDL2 \
    ucrt64/mingw-w64-ucrt-x86_64-gcc \
    ucrt64/mingw-w64-ucrt-x86_64-rust

echo "
⭐ MSYS2 initialization complete
⭐ Build dependencies for fab-egon-emulator installed

👋 Cloning ${REPO}...
"
git clone "${REPO}"

echo "
⭐ github repo ${REPO} cloned 

🤝 Please run the following commands:

cd fab-agon-emulator
git submodule update --init
make
./fab-agon-emulator.exe

🎉 Enjoy!
"
