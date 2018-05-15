#!/bin/bash
if [ ! -f mpc-qt.ico ]; then
    (magick convert -quiet images/icon/mpc-qt.svg -define icon:auto-resize=16,48,256 -compress zip mpc-qt.ico)2>&1>&/dev/null
fi
echo mpc-qt.ico
