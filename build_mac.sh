#!/bin/sh
pushd deps/raylib/src
echo "Building raylib..."
make PLATFORM=PLATFORM_DESKTOP
popd

echo "Building chip8 emulator..."
gcc main.c -o chip8 -I deps/raylib/src -L deps/raylib/src -lraylib -framework CoreGraphics -framework IOKit -framework Cocoa