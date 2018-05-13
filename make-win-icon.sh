#!/bin/bash
(magick convert -quiet images/icon/mpc-qt.svg -define icon:auto-resize=16,48,256 -compress zip mpc-qt.ico)2>&1>&/dev/null
echo mpc-qt.ico
