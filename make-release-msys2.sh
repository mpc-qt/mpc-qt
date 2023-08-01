#!/usr/bin/bash
# Like make-release-win.sh, but everything comes from the msys2 installation

VERSION=""
if [ -v GITHUB_SHA ]; then
    VERSION=$GITHUB_SHA
elif [ -v FORCE_VERSION ]; then
    VERSION=$FORCE_VERSION
else
    # v23.02-74-g60b18fa -> 23.02.74
    VERSION=`git describe --tags  | sed 's/[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\).*/\1.\2.\3/'`
fi
BUILD=release
SUFFIX="win-x64-$VERSION"
DEST="mpc-qt-$SUFFIX"

qmake6 "MPCQT_VERSION=$VERSION" mpc-qt.pro

mkdir -p $BUILD
rm $BUILD/*
make $BUILD-clean
make -j`nproc` $BUILD

echo Making directories
mkdir -p "$DEST"
mkdir -p "$DEST/doc"
mkdir -p "$DEST/iconengines"
mkdir -p "$DEST/imageformats"
mkdir -p "$DEST/platforms"
mkdir -p "$DEST/styles"

echo Copying documents
cp DOCS/ipc.md  "$DEST/doc"

echo Copying plugins
PLUGDIR=/mingw64/share/qt6/plugins
cp "$PLUGDIR/iconengines/qsvgicon.dll"      "$DEST/iconengines"
cp "$PLUGDIR/imageformats/qjpeg.dll"        "$DEST/imageformats"
cp "$PLUGDIR/imageformats/qsvg.dll"         "$DEST/imageformats"
cp "$PLUGDIR/platforms/qdirect2d.dll"       "$DEST/platforms"
cp "$PLUGDIR/platforms/qminimal.dll"        "$DEST/platforms"
cp "$PLUGDIR/platforms/qoffscreen.dll"      "$DEST/platforms"
cp "$PLUGDIR/platforms/qwindows.dll"        "$DEST/platforms"
cp "$PLUGDIR/styles/qwindowsvistastyle.dll" "$DEST/styles"

BINDIR=/mingw64/bin
echo Copying support dlls
cp $BINDIR/libb2*.dll                   "$DEST"
cp $BINDIR/libbrotlicommon.dll          "$DEST"
cp $BINDIR/libbrotlidec.dll             "$DEST"
cp $BINDIR/libbrotlienc.dll             "$DEST"
cp $BINDIR/libbz2-*.dll                 "$DEST"
cp $BINDIR/libdouble-conversion.dll     "$DEST"
cp $BINDIR/libfreetype-*.dll            "$DEST"
cp $BINDIR/libgcc_s_seh-*.dll           "$DEST"
cp $BINDIR/libglib-*.dll                "$DEST"
cp $BINDIR/libgraphite*.dll             "$DEST"
cp $BINDIR/libharfbuzz-0.dll            "$DEST"
cp $BINDIR/libiconv-*.dll               "$DEST"
cp $BINDIR/libicudt*.dll                "$DEST"
cp $BINDIR/libicuin*.dll                "$DEST"
cp $BINDIR/libicuuc*.dll                "$DEST"
cp $BINDIR/libintl-*.dll                "$DEST"
cp $BINDIR/libjpeg-*.dll                "$DEST"
cp $BINDIR/libmd4c.dll                  "$DEST"
cp $BINDIR/libpcre2-8-*.dll             "$DEST"
cp $BINDIR/libpcre2-16-*.dll            "$DEST"
cp $BINDIR/libpng16-*.dll               "$DEST"
cp $BINDIR/libstdc++-*.dll              "$DEST"
cp $BINDIR/libwinpthread-*.dll          "$DEST"
cp $BINDIR/zlib*.dll                    "$DEST"
echo Copying Qt dlls
cp $BINDIR/Qt6Core.dll                  "$DEST"
cp $BINDIR/Qt6Gui.dll                   "$DEST"
cp $BINDIR/Qt6Network.dll               "$DEST"
cp $BINDIR/Qt6OpenGL.dll                "$DEST"
cp $BINDIR/Qt6OpenGLWidgets.dll         "$DEST"
cp $BINDIR/Qt6Svg.dll                   "$DEST"
cp $BINDIR/Qt6Widgets.dll               "$DEST"
cp $BINDIR/Qt6Xml.dll                   "$DEST"
echo Copying mpv dlls
cp $BINDIR/libmpv-*.dll                 "$DEST"
cp $BINDIR/libarchive-*.dll             "$DEST"
cp $BINDIR/libass-*.dll                 "$DEST"
cp $BINDIR/avcodec-*.dll                "$DEST"
cp $BINDIR/avdevice-*.dll               "$DEST"
cp $BINDIR/avfilter-*.dll               "$DEST"
cp $BINDIR/avformat-*.dll               "$DEST"
cp $BINDIR/avutil-*.dll                 "$DEST"
cp $BINDIR/libbluray-*.dll              "$DEST"
cp $BINDIR/libcaca-*.dll                "$DEST"
cp $BINDIR/liblcms2-*.dll               "$DEST"
cp $BINDIR/lua*.dll                     "$DEST"
cp $BINDIR/libplacebo-*.dll             "$DEST"
cp $BINDIR/librubberband-*.dll          "$DEST"
cp $BINDIR/libshaderc_shared.dll        "$DEST"
cp $BINDIR/libspirv-cross-c-shared.dll  "$DEST"
cp $BINDIR/swresample-*.dll             "$DEST"
cp $BINDIR/swscale-*.dll                "$DEST"
cp $BINDIR/libuchardet.dll              "$DEST"
cp $BINDIR/libvapoursynth-script-*.dll  "$DEST"
cp $BINDIR/libzimg-*.dll                "$DEST"
cp $BINDIR/libfontconfig-*.dll          "$DEST"
cp $BINDIR/libfribidi-*.dll             "$DEST"
cp $BINDIR/libcrypto-*-x64.dll          "$DEST"
cp $BINDIR/liblz4.dll                   "$DEST"
cp $BINDIR/libexpat-*.dll               "$DEST"
cp $BINDIR/liblzma-*.dll                "$DEST"
cp $BINDIR/libunibreak-*.dll            "$DEST"
cp $BINDIR/libzstd.dll                  "$DEST"
cp $BINDIR/libopenal-*.dll              "$DEST"
cp $BINDIR/SDL2.dll                     "$DEST"
cp $BINDIR/libxml2-*.dll                "$DEST"
cp $BINDIR/libmfx-*.dll                 "$DEST"
cp $BINDIR/libvidstab.dll               "$DEST"
cp $BINDIR/libgme.dll                   "$DEST"
cp $BINDIR/libgnutls-*.dll              "$DEST"
cp $BINDIR/postproc-*.dll               "$DEST"
cp $BINDIR/libmodplug-*.dll             "$DEST"
cp $BINDIR/librtmp-*.dll                "$DEST"
cp $BINDIR/libsrt.dll                   "$DEST"
cp $BINDIR/libssh.dll                   "$DEST"
cp $BINDIR/libfftw3-*.dll               "$DEST"
cp $BINDIR/dovi.dll                     "$DEST"
cp $BINDIR/libsamplerate-*.dll          "$DEST"
cp $BINDIR/libaom.dll                   "$DEST"
cp $BINDIR/libcairo-*.dll               "$DEST"
cp $BINDIR/libsoxr.dll                  "$DEST"
cp $BINDIR/libpython3.*.dll             "$DEST"
cp $BINDIR/libdav1d.dll                 "$DEST"
cp $BINDIR/libgobject-2.0-*.dll         "$DEST"
cp $BINDIR/libgsm.dll                   "$DEST"
cp $BINDIR/libmp3lame-*.dll             "$DEST"
cp $BINDIR/libopencore-amrnb-*.dll      "$DEST"
cp $BINDIR/libopencore-amrwb-*.dll      "$DEST"
cp $BINDIR/libopenjp2-*.dll             "$DEST"
cp $BINDIR/libopus-*.dll                "$DEST"
cp $BINDIR/rav1e.dll                    "$DEST"
cp $BINDIR/librsvg-*.dll                "$DEST"
cp $BINDIR/libspeex-*.dll               "$DEST"
cp $BINDIR/libSvtAv1Enc.dll             "$DEST"
cp $BINDIR/libtheoradec-*.dll           "$DEST"
cp $BINDIR/libtheoraenc-*.dll           "$DEST"
cp $BINDIR/libvorbis-*.dll              "$DEST"
cp $BINDIR/libvorbisenc-*.dll           "$DEST"
cp $BINDIR/libvpx-*.dll                 "$DEST"
cp $BINDIR/libwebp-*.dll                "$DEST"
cp $BINDIR/libwebpmux-*.dll             "$DEST"
cp $BINDIR/libx264-*.dll                "$DEST"
cp $BINDIR/libx265.dll                  "$DEST"
cp $BINDIR/xvidcore.dll                 "$DEST"
cp $BINDIR/libgmp-*.dll                 "$DEST"
cp $BINDIR/libhogweed-*.dll             "$DEST"
cp $BINDIR/libnettle-*.dll              "$DEST"
echo Copying extra libraries
cp $BINDIR/libidn2-*.dll                "$DEST"
cp $BINDIR/libp11-kit-*.dll             "$DEST"
cp $BINDIR/libtasn1-*.dll               "$DEST"
cp $BINDIR/libunistring-*.dll           "$DEST"
cp $BINDIR/libgomp-*.dll                "$DEST"
cp $BINDIR/libffi-*.dll                 "$DEST"
cp $BINDIR/libpixman-*.dll              "$DEST"
cp $BINDIR/libogg-*.dll                 "$DEST"
cp $BINDIR/libsharpyuv-*.dll            "$DEST"
cp $BINDIR/libgdk_pixbuf-*.dll          "$DEST"
cp $BINDIR/libgio-*.dll                 "$DEST"
cp $BINDIR/libpango-*.dll               "$DEST"
cp $BINDIR/libpangocairo-*.dll          "$DEST"
cp $BINDIR/libthai-*.dll                "$DEST"
cp $BINDIR/libgmodule-*.dll             "$DEST"
cp $BINDIR/libpangoft2-*.dll            "$DEST"
cp $BINDIR/libpangowin32-*.dll          "$DEST"
cp $BINDIR/libdatrie-*.dll              "$DEST"

echo Copying executable
cp "$BUILD/mpc-qt.exe" "$DEST"

echo Zipping and checksumming
7z a "$DEST.zip" "./$DEST/*"
sha512sum "$DEST.zip" >"$DEST.zip.sha512"
