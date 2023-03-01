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
make -j`nproc` release

read -r -d '' dirs <<'EOF'
.
/doc
/iconengines
/imageformats
/platforms
/styles
EOF

read -r -d '' dlls <<'EOF'
libb2-1.dll
libbrotlicommon.dll
libbrotlidec.dll
libbz2-1.dll
libdouble-conversion.dll
libfreetype-6.dll
libgcc_s_seh-1.dll
libglib-2.0-0.dll
libgraphite2.dll
libharfbuzz-0.dll
libiconv-2.dll
libintl-8.dll
libjpeg-8.dll
libmd4c.dll
libpcre2-8-0.dll
libpcre2-16-0.dll
libpng16-16.dll
libstdc++-6.dll
libwinpthread-1.dll
zlib1.dll
Qt6Core.dll
Qt6Gui.dll
Qt6Network.dll
Qt6OpenGL.dll
Qt6OpenGLWidgets.dll
Qt6Svg.dll
Qt6Widgets.dll
Qt6Xml.dll
EOF

read -r -d '' plugins <<'EOF'
iconengines/qsvgicon.dll
imageformats/qjpeg.dll
imageformats/qsvg.dll
platforms/qdirect2d.dll
platforms/qminimal.dll
platforms/qoffscreen.dll
platforms/qwindows.dll
styles/qwindowsvistastyle.dll
EOF

read -r -d '' docs <<'EOF'
ipc.md
EOF

read -r -d '' binaries <<'EOF'
yt-dlp.exe
EOF

(while read -r dir; do
        mkdir -p "$DEST/$dir"
done)<<<"$dirs"

(while read -r dll; do
        cp `which "$dll"` "$DEST/$dll"
done)<<<"$dlls"

# The libicu dlls change name very often, copy with a glob
cp /mingw64/bin/libicudt*.dll "$DEST"
cp /mingw64/bin/libicuin*.dll "$DEST"
cp /mingw64/bin/libicuuc*.dll "$DEST"

(while read -r plugin; do
        cp "/mingw64/share/qt6/plugins/$plugin" "$DEST/$plugin"
done)<<<"$plugins"

(while read -r doc; do
        cp "DOCS/$doc" "$DEST/doc/$doc"
done)<<<"$docs"

(while read -r binary; do
        cp "binaries/$binary" "$DEST/$binary"
done)<<<"$binaries"

cp "$BUILD/mpc-qt.exe" "$DEST"
cp mpv-dev/lib/libmpv-2.dll "$DEST"
7z a "$DEST.zip" "./$DEST/*"
sha512sum "$DEST.zip" >"$DEST.zip.sha512"
