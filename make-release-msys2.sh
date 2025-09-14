#!/usr/bin/bash
# Like make-release-win.sh, but everything comes from the msys2 installation

set -e

VERSION=""
if [ -v FORCE_VERSION ]; then
    if [[ $FORCE_VERSION == "GithubAction" ]]; then
        VERSION=`date +'%Y.%m.%d'`
    else
        VERSION=$FORCE_VERSION
    fi
else
    # v23.02-74-g60b18fa -> 23.02.74
    VERSION=`git describe --tags  | sed 's/[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\)[^0-9]*\([0-9]*\).*/\1.\2.\3/'`
fi
BUILDDIR=bin
EXECUTABLE="$BUILDDIR/mpc-qt.exe"
BINDIR="/mingw64/bin"
SUFFIX="win-x64"
DEST="mpc-qt-$SUFFIX"

mkdir -p translations/qt
cp /mingw64/share/qt6/translations/qtbase_*.qm translations/qt

cmake -DMPCQT_VERSION=$VERSION -DENABLE_LOCAL_MPV=ON -G Ninja -B build
ninja -C build

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
echo Finding dependencies and copying them
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

DLLS=("Qt6Core.dll" "Qt6Gui.dll" "Qt6Network.dll" "Qt6OpenGLWidgets.dll")

for dll in "${DLLS[@]}"; do
  echo " "
  echo "Checking dependencies for $dll"
  ldd "$BINDIR/$dll" | awk '/=>/ {print $3}' | while read -r dep; do
    if [[ -n "$dep" && -f "$dep" ]]; then
      # Check if the DLL is in /mingw64/bin before copying
      if [[ "$dep" == "$BINDIR"* ]]; then
        echo "Copying $dep to $DEST"
        cp -u "$dep" "$DEST"
      else
        echo "Skipping $dep (not in $BINDIR)"
      fi
    fi
  done
done

# Manually copy Qt6Svg.dll as it's needed by imageformats/qjpeg.dll
echo "Copying libjpeg-*.dll to $DEST"
cp $BINDIR/libjpeg-*.dll                "$DEST"

echo "All required DLLs from $BINDIR have been copied to $DEST."

echo Copying executable
cp "$BUILDDIR/mpc-qt.exe" "$DEST"

echo Copying libmpv
cp $BINDIR/libmpv-2.dll "$DEST"

echo Copying yt-dlp
cp "yt-dlp.exe" "$DEST"

echo Creating NSIS installer
makensis deploy/dist/nsis/mpc-qt.nsi
