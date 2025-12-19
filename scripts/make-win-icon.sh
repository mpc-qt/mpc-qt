#!/bin/bash
if [ ! -f src/mpc-qt.ico ]; then
    magick res/images/icon/mpc-qt.svg -define icon:auto-resize=16,24,32,48,64,72,96,128,256 -antialias -transparent white -compress zip src/mpc-qt.ico
fi
echo mpc-qt.ico
