#!/bin/bash
# Source: https://docs.appimage.org/packaging-guide/from-source/native-binaries.html

set -x
set -e

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
SUFFIX="linux-x64-$VERSION"
DEST="mpc-qt-$SUFFIX"

# building in temporary directory to keep system clean
# use RAM disk if possible (as in: not building on CI system like Travis, and RAM disk is available)
if [ "$CI" == "" ] && [ -d /dev/shm ]; then
    TEMP_BASE=/dev/shm
else
    TEMP_BASE=/tmp
fi

BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" appimage-build-XXXXXX)

# make sure to clean up build dir, even if errors occur
cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

# store repo root as variable
REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
OLD_CWD=$(readlink -f .)

# switch to build dir
pushd "$BUILD_DIR"

# configure build files with cmake
cmake -DMPCQT_VERSION=$VERSION -DCMAKE_INSTALL_PREFIX=/usr "$REPO_ROOT" -G Ninja -B build

# build project and install files into AppDir
ninja -C build
DESTDIR=AppDir ninja -C build install

# now, build AppImage using linuxdeploy and linuxdeploy-plugin-qt
# download linuxdeploy and its Qt plugin
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

# make them executable
chmod +x linuxdeploy*.AppImage

# initialize AppDir, bundle shared libraries, add desktop file and icon, use Qt plugin to bundle additional resources, and build AppImage, all in one command
export EXTRA_QT_MODULES="svg;"
export QMAKE=/usr/bin/qmake6
export LDAI_OUTPUT="$DEST.AppImage"
export LDAI_UPDATE_INFORMATION="gh-releases-zsync|mpc-qt|mpc-qt|latest|mpc-qt-linux-x64-*.AppImage.zsync"
linuxdeploy/linuxdeploy-x86_64.AppImage \
    --appdir "$REPO_ROOT"/build/AppDir \
    -e "$REPO_ROOT"/build/AppDir/usr/bin/mpc-qt \
    -d "$REPO_ROOT"/build/AppDir/usr/share/applications/io.github.mpc_qt.mpc-qt.desktop \
    --custom-apprun "$REPO_ROOT"/packaging/appimage/AppRun \
    --plugin qt --output appimage

# move built AppImage back into original CWD
mv $DEST.AppImage $OLD_CWD
