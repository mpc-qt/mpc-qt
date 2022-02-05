#!/bin/bash
# Make windows release script using 64bit libs

VERSION=`date +'%y%m'`
DOTTEDVERSION=`date +'%y.%m'`
BUILD=release
SUFFIX="win-x64-$VERSION"
DEST="mpc-qt-$SUFFIX"

qmake "MPCQT_VERSION=$DOTTEDVERSION" mpc-qt.pro
mkdir -p release
rm release/*
make release-clean
make -j4 release

read -r -d '' dirs <<'EOF'
.
/doc
/iconengines
/imageformats
/platforms
/styles
/translations
EOF

read -r -d '' dlls <<'EOF'
libbrotlidec.dll
libbrotlicommon.dll
libbz2-1.dll
libdouble-conversion.dll
libfreetype-6.dll
libgcc_s_seh-1.dll
libglib-2.0-0.dll
libgraphite2.dll
libharfbuzz-0.dll
libiconv-2.dll
libicudt69.dll
libicuin69.dll
libicuuc69.dll
libintl-8.dll
libjpeg-8.dll
libmd4c.dll
libpcre-1.dll
libpcre2-16-0.dll
libpng16-16.dll
libstdc++-6.dll
libwinpthread-1.dll
libzstd.dll
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
platforms/qwebgl.dll
platforms/qwindows.dll
styles/qwindowsvistastyle.dll
EOF

read -r -d '' docs <<'EOF'
ipc.md
EOF

read -r -d '' binaries <<'EOF'
yt-dlp.exe
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

(while read -r binary; do
        cp "binaries/$binary" "$DEST/$binary"
done)<<<"$binaries"

cp "$BUILD/mpc-qt.exe" "$DEST/mpc-qt.exe"
cp mpv-dev/lib/mpv-2.dll "$DEST/mpv-2.dll"
7z a "$DEST.zip" "./$DEST/*"
sha512sum "$DEST.zip" >"$DEST.zip.sha512"
