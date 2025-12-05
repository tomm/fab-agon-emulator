#!/bin/bash

#
# Cross-compiling for windows-x64, on a Linux system.
# 
# $ apt install mingw-w64
# $ rustup target add x86_64-pc-windows-gnu
#
# download SDL3 libs for mingw:
# https://github.com/libsdl-org/SDL/releases/download/release-3.2.28/SDL3-devel-3.2.28-mingw.tar.gz
#
# extract into fab-agon-emulator project root, rename directory from SDL3-3.2.28 to SDL3
# copy SDL3/x86_64-w64-mingw32/bin/SDL3.dll to .
#
# locate libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll (posix threads version) somewhere
# in /usr/..... , and copy to .
#
# Once this is all done, running this script should build a Windows binary zip.

make clean
cd src/vdp
CXX=x86_64-w64-mingw32-g++-posix make -j8
cp *.so ../../firmware
cp vdp_console8.so ../../firmware/vdp_platform.so
cd ../..

FORCE=1 cargo build -r --target=x86_64-pc-windows-gnu
cargo build -r --target=x86_64-pc-windows-gnu --manifest-path=./agon-cli-emulator/Cargo.toml

VERSION=`cargo tree --depth 0 | awk '{print $2;}'`
DIST_DIR=fab-agon-emulator-$VERSION-windows-x64

rm -rf $DIST_DIR
mkdir $DIST_DIR
cp ./target/x86_64-pc-windows-gnu/release/fab-agon-emulator.exe $DIST_DIR
cp ./target/x86_64-pc-windows-gnu/release/agon-cli-emulator.exe $DIST_DIR
cp -r ./firmware $DIST_DIR
cp SDL3.dll $DIST_DIR
cp libgcc_s_seh-1.dll $DIST_DIR
cp libstdc++-6.dll $DIST_DIR
cp libwinpthread-1.dll $DIST_DIR
cp LICENSE README.md $DIST_DIR
mkdir $DIST_DIR/sdcard
cp -r sdcard/* $DIST_DIR/sdcard/
x86_64-w64-mingw32-strip $DIST_DIR/firmware/*.so $DIST_DIR/*.exe $DIST_DIR/*.dll
zip -r ./artifacts/$DIST_DIR.zip $DIST_DIR

