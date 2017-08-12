#!/bin/bash
# Make windows release script using 64bit libs
#
# Usage:
# 	./make-release-win.sh SUFFIX
#
#	SUFFIX	name of zipfile will be called mpc-qt-SUFFIX.zip
#
# Example:
#	./make-release-win.sh win-x64-YYMM
#
# To change the version string displayed in Help->About This Program,
# add the following line to mpc-qt.pro beforehand.
#
#	DEFINES += MPCQT_VERSION_STR=YY.MM


BUILD=release
DEST=mpc-qt-$1

qmake mpc-qt.pro
make release-clean
make -j4 release

read -r -d '' dirs <<'EOF'
.
/doc
/iconengines
/imageformats
/platforms
/translations
EOF

read -r -d '' dlls <<'EOF'
libbz2-1.dll
libfreetype-6.dll
libgcc_s_seh-1.dll
libglib-2.0-0.dll
libgraphite2.dll
libharfbuzz-0.dll
libiconv-2.dll
libicudt58.dll
libicuin58.dll
libicuuc58.dll
libintl-8.dll
libpcre-1.dll
libpng16-16.dll
libstdc++-6.dll
libwinpthread-1.dll
Qt5Core.dll
Qt5Gui.dll
Qt5Network.dll
Qt5Svg.dll
Qt5Widgets.dll
Qt5Xml.dll
zlib1.dll
EOF

read -r -d '' plugins <<'EOF'
iconengines/qsvgicon.dll
imageformats/qjpeg.dll
imageformats/qsvg.dll
platforms/qdirect2d.dll
platforms/qminimal.dll
platforms/qoffscreen.dll
platforms/qwindows.dll
EOF

read -r -d '' docs <<'EOF'
ipc.md
EOF

translations=`ls resources/translations`

(while read -r dir; do
	mkdir -p "$DEST/$dir"
done)<<<"$dirs"

(while read -r dll; do
	cp `which "$dll"` "$DEST/$dll"
done)<<<"$dlls"

(while read -r plugin; do
	cp "/mingw64/share/qt5/plugins/$plugin" "$DEST/$plugin"
done)<<<"$plugins"

(while read -r translation; do
	cp "resources/translations/$translation" "$DEST/translations/$translation"
done)<<<"$translations"

(while read -r doc; do
	cp "DOCS/$doc" "$DEST/doc/$doc"
done)<<<"$docs"

cp "$BUILD/mpc-qt.exe" "$DEST/mpc-qt.exe"
cp mpv-dev/lib/mpv-1.dll "$DEST/mpv-1.dll"
7z a "mpc-qt-$1.zip" "./$DEST/*"
