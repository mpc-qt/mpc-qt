#!/bin/bash
ICONPATH="../res/images/icon"
    magick $ICONPATH/mpc-qt.svg -define icon:auto-resize=16,24,32,48,64,72,96,128,256 \
        -antialias -transparent white -compress zip $ICONPATH/mpc-qt.ico
