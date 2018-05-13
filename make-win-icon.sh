#!/bin/bash
(magick convert -quiet images/icon/mpc-qt.svg -define icon:auto-resize=16,48,256 -compress zip mpc-qt.ico)2>&1>&/dev/null
echo IDI_ICON1 ICON DISCARDABLE mpc-qt.ico >mpc-qt.rc
echo mpc-qt.rc
