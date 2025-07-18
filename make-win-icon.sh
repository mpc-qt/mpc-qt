#!/bin/bash
if [ ! -f mpc-qt.ico ]; then
    magick images/icon/mpc-qt.svg -define icon:auto-resize=16,32,48,64,128,256 -antialias -transparent white -compress zip mpc-qt.ico
fi
echo mpc-qt.ico
