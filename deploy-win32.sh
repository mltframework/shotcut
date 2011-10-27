#!/bin/sh

BUILD="$HOME/build"
DEST="$HOME/Shotcut"

rm -rf "$DEST"/*

# executables
install -d "$DEST"/lib/ladspa
cp -p ../shotcut-build-desktop/release/shotcut.exe "$DEST" 
cp -p "$BUILD"/melt.exe "$DEST"
[ -f "$BUILD/ffmpeg.exe" ] && cp -p "$BUILD"/bin/ffmpeg.exe "$DEST"

# libs
cp -p "$BUILD"/*.dll "$DEST"
cp -p /usr/bin/SDL.dll "$DEST"
cp -p /mingw/bin/libdl.dll "$DEST"
cp -p /mingw/bin/libgomp-1.dll "$DEST"
cp -p /mingw/bin/libpthread-2.dll "$DEST"
cp -p /mingw/bin/mingwm10.dll "$DEST"
cp -p /mingw/bin/libstdc++-6.dll "$DEST"

# plugins
cp -pr "$BUILD"/lib/{frei0r-1,mlt} "$DEST"/lib
cp -p "$BUILD"/lib/ladspa/*.dll "$DEST"/lib/ladspa
install -d "$DEST"/lib/qt4
cp -pr "$QTDIR"/plugins/* "$DEST"/lib/qt4

# data
install -d "$DEST"/share
cp -pr ~/build/share/{ffmpeg,mlt} "$DEST"/share
