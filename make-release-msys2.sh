#!/usr/bin/bash
# Like make-release-win.sh, but everything comes from the msys2 installation

set -e

VERSION=""
if [ -v GITHUB_SHA ]; then
    VERSION=`date +'%Y.%m.%d'`
elif [ -v FORCE_VERSION ]; then
    VERSION=$FORCE_VERSION
else
    # v23.02-74-g60b18fa -> 23.02.74
    VERSION=`git describe --tags  | sed 's/[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\).*/\1.\2.\3/'`
fi
BUILD=release
EXECUTABLE="$BUILD/mpc-qt.exe"
BINDIR="/mingw64/bin"
SUFFIX="win-x64-$VERSION"
DEST="mpc-qt-$SUFFIX"

qmake6 "MPCQT_VERSION=$VERSION" mpc-qt.pro

mkdir -p $BUILD
rm $BUILD/*
make $BUILD-clean
if [ -v GITHUB_SHA ]; then
    make $BUILD
else
    make -j`nproc` $BUILD
fi

if [ ! -f "$EXECUTABLE" ]; then
    echo Failed to find executable
    exit 1
fi

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
cp "$PLUGDIR/iconengines/qsvgicon.dll"          "$DEST/iconengines"
cp "$PLUGDIR/imageformats/qjpeg.dll"            "$DEST/imageformats"
cp "$PLUGDIR/imageformats/qsvg.dll"             "$DEST/imageformats"
cp "$PLUGDIR/platforms/qdirect2d.dll"           "$DEST/platforms"
cp "$PLUGDIR/platforms/qminimal.dll"            "$DEST/platforms"
cp "$PLUGDIR/platforms/qoffscreen.dll"          "$DEST/platforms"
cp "$PLUGDIR/platforms/qwindows.dll"            "$DEST/platforms"
cp "$PLUGDIR/styles/qmodernwindowsstyle.dll"    "$DEST/styles"


# Use ldd to find dependencies and copy them
ldd "$EXECUTABLE" | awk '/=>/ {print $3}' | while read -r dll; do
  if [[ -n "$dll" && -f "$dll" ]]; then
    # Check if the DLL is in /mingw64/bin before copying
    if [[ "$dll" == "$BINDIR"* ]]; then
      echo "Copying $dll to $DEST"
      cp -u "$dll" "$DEST"
    else
      echo "Skipping $dll (not in $BINDIR)"
    fi
  fi
done

# Manually copy Qt6Svg.dll as it's dynamically loaded
cp $BINDIR/Qt6Svg.dll                   "$DEST"

echo "All required DLLs from $BINDIR have been copied to $DEST."


echo Copying executable
cp "$BUILD/mpc-qt.exe" "$DEST"

echo Zipping and checksumming
7z a "$DEST.zip" "./$DEST/*"
sha512sum "$DEST.zip" >"$DEST.zip.sha512"
