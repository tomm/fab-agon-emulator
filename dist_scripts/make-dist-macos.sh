#!/bin/bash
make clean
make

VERSION=`cargo tree --depth 0 | awk '{print $2;}'`
DIST_DIR=fab-agon-emulator-$VERSION-macos

rm -rf $DIST_DIR
mkdir $DIST_DIR
cp ./target/release/fab-agon-emulator $DIST_DIR
cp -r ./firmware $DIST_DIR
cp LICENSE README.md $DIST_DIR
mkdir $DIST_DIR/sdcard
cp -r sdcard/* $DIST_DIR/sdcard/
zip $DIST_DIR.zip -r $DIST_DIR/*
