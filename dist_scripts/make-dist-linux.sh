#!/bin/bash
make clean
make -j8

VERSION=`cargo tree --depth 0 | awk '{print $2;}'`
ARCH=`uname -m`
DIST_DIR=fab-agon-emulator-$VERSION-linux-$ARCH

rm -rf $DIST_DIR
mkdir $DIST_DIR
cp ./target/release/fab-agon-emulator $DIST_DIR
cp -r ./firmware $DIST_DIR
cp LICENSE README.md $DIST_DIR
mkdir $DIST_DIR/sdcard
cp -r sdcard/* $DIST_DIR/sdcard/
strip $DIST_DIR/firmware/*.so $DIST_DIR/fab-agon-emulator
tar cvjf ./artifacts/$DIST_DIR.tar.bz2 $DIST_DIR
