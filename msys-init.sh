#!/bin/sh
REPO=${REPO:-https://github.com/tomm/fab-agon-emulator.git}
set -euxo pipefail # bash strict mode: https://gist.github.com/mohanpedala/1e2ff5661761d3abd0385e8223e16425

echo "⚠️ MSYS2 may ask to restart after this update. If so, rerun this script afterwards."
pacman --noconfirm -Syu
echo "👌 Installing build dependencies for fab-agon-emulator..."
pacman --noconfirm -S \
    vim \
    git \
    make \
    ucrt64/mingw-w64-ucrt-x86_64-SDL2 \
    ucrt64/mingw-w64-ucrt-x86_64-gcc \
    ucrt64/mingw-w64-ucrt-x86_64-rust
echo "👋 Cloning ${REPO}"
git clone 
cd fab-agon-emulator
echo "
⭐ MSYS2 initialization complete
⭐ Build dependencies for fab-egon-emulator installed
⭐ github repo ${REPO} cloned and submodules initialized
🪄 Enjoy!
"