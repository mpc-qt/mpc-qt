#!/bin/bash
ICOFILE="build/mpc-qt.ico"
mkdir build
if [ ! -f $ICOFILE ]; then
    magick res/images/icon/mpc-qt.svg -define icon:auto-resize=16,24,32,48,64,72,96,128,256 -antialias -transparent white -compress zip $ICOFILE
fi
echo "$ICOFILE written"
