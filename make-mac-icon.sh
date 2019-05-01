#!/bin/bash
if [ ! -f images/icon/mpc-qt.icns ]; then
    cd images/icon
	mkdir mpc-qt.iconset
    rsvg-convert -h 16 mpc-qt.svg > mpc-qt.iconset/icon_16x16.png
    rsvg-convert -h 32 mpc-qt.svg > mpc-qt.iconset/icon_16x16@2x.png
    rsvg-convert -h 32 mpc-qt.svg > mpc-qt.iconset/icon_32x32.png
    rsvg-convert -h 64 mpc-qt.svg > mpc-qt.iconset/icon_32x32@2x.png
    rsvg-convert -h 128 mpc-qt.svg > mpc-qt.iconset/icon_128x128.png
    rsvg-convert -h 256 mpc-qt.svg > mpc-qt.iconset/icon_128x128@2x.png
    rsvg-convert -h 256 mpc-qt.svg > mpc-qt.iconset/icon_256x256.png
    rsvg-convert -h 512 mpc-qt.svg > mpc-qt.iconset/icon_256x256@2x.png
    rsvg-convert -h 512 mpc-qt.svg > mpc-qt.iconset/icon_512x512.png
    rsvg-convert -h 1024 mpc-qt.svg > mpc-qt.iconset/icon_512x512@2x.png
    iconutil -c icns mpc-qt.iconset
    rm -R mpc-qt.iconset
    cd ../..
fi
