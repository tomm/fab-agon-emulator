#!/bin/bash

#
# Cross-compiling for windows-x64, on a Linux system.
# 
# $ apt install mingw-w64
# $ rustup target add x86_64-pc-windows-gnu
#
# download SDL2 libs for mingw:
# https://github.com/libsdl-org/SDL/releases/download/release-2.28.3/SDL2-devel-2.28.3-mingw.tar.gz
#
# extract, copy contents of x86_64-w64-mingw32/lib to
# $HOME/.rustup/toolchains/stable-x86_64-unknown-linux-gnu/lib/rustlib/x86_64-pc-windows-gnu/lib
#
# copy bin/SDL2.dll to .
#
# locate libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll (posix threads version) somewhere
# in /usr/..... , and copy to .
#
# Once this is all done, running this script should build a Windows binary zip.

make clean
cd src/vdp
CXX=x86_64-w64-mingw32-g++-posix make
cp *.so ../../firmware
cd ../..

FORCE=1 cargo build -r --target=x86_64-pc-windows-gnu

VERSION=`cargo tree --depth 0 | awk '{print $2;}'`
DIST_DIR=fab-agon-emulator-$VERSION-windows-x64

rm -rf $DIST_DIR
mkdir $DIST_DIR
cp ./target/x86_64-pc-windows-gnu/release/fab-agon-emulator.exe $DIST_DIR
cp -r ./firmware $DIST_DIR
cp SDL2.dll $DIST_DIR
cp libgcc_s_seh-1.dll $DIST_DIR
cp libstdc++-6.dll $DIST_DIR
cp libwinpthread-1.dll $DIST_DIR
cp LICENSE README.md $DIST_DIR
mkdir $DIST_DIR/sdcard
cp -r sdcard/* $DIST_DIR/sdcard/
x86_64-w64-mingw32-strip $DIST_DIR/firmware/*.so $DIST_DIR/*.exe $DIST_DIR/*.dll
zip -r $DIST_DIR.zip $DIST_DIR

