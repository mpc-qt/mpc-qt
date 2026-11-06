#!/bin/bash
# Build a standalone MPC-QT.app against Homebrew's Qt and mpv, and zip it.
#
# The app runs on the macOS version Homebrew's bottles were built for and
# later, so build on the oldest macOS you want to support.  The bundle is
# only ad-hoc signed.

set -euo pipefail
cd "$(dirname "$0")/.."

BREW=$(brew --prefix)
BUILDDIR=build
DEST="mpc-qt-macos-$(uname -m)"
APP="$DEST/MPC-QT.app"
LICENSES="$APP/Contents/Resources/Licenses"

# Like the Linux build, take the version from git unless one is forced.
# Clear versions an earlier run left in the cache first.
VERSION_ARGS=(-UMPCQT_VERSION -UMPCQT_VERSION_WIN)
if [[ -n "${FORCE_VERSION:-}" && $FORCE_VERSION != continuous ]]; then
    VERSION_ARGS+=(-DMPCQT_VERSION="$FORCE_VERSION")
fi

cmake "${VERSION_ARGS[@]}" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$BREW" -G Ninja -B "$BUILDDIR"
cmake --build "$BUILDDIR"

echo Copying the app bundle
rm -rf "$DEST" "$DEST.zip"
mkdir -p "$DEST"
ditto bin/mpc-qt.app "$APP"

echo Copying Qt and the libraries into the bundle
"$BREW/opt/qtbase/bin/macdeployqt" "$APP" -libpath="$BREW/lib"
install_name_tool -delete_rpath "$BREW/lib" "$APP/Contents/MacOS/mpc-qt" 2>/dev/null || true

# Some macdeployqt versions copy a library found through @rpath but leave
# its own install name (or a reference to it) pointing into Homebrew.
FRAMEWORKS="$APP/Contents/Frameworks"
while IFS= read -r -d '' f; do
    file -b "$f" | grep -q Mach-O || continue
    id=$(otool -D "$f" | sed -n 2p)
    case $id in
        "$BREW"/*) install_name_tool -id "@rpath/$(basename "$id")" "$f" 2>/dev/null ;;
    esac
    for dep in $(otool -L "$f" | sed 1d | awk '{print $1}' | grep "^$BREW/"); do
        [ -e "$FRAMEWORKS/$(basename "$dep")" ] || continue
        install_name_tool -change "$dep" "@executable_path/../Frameworks/$(basename "$dep")" "$f" 2>/dev/null
    done
done < <(find "$APP/Contents" -type f -print0)

echo Copying licenses
# Which Homebrew formulae ended up in the bundle.  opt/ also holds aliases
# (icu4c for icu4c@79), so ask brew for the real names.
found=$(
    for item in "$FRAMEWORKS"/*; do
        ls -d "$BREW"/opt/*/lib/"$(basename "$item")" 2>/dev/null | head -1
    done | sed -E "s#^$BREW/opt/([^/]+)/.*#\1#" | sort -u
)
info=$(brew info --json=v2 --formula qtbase qtsvg $found)
formulae=$(python3 -c 'import json, sys
print("\n".join(sorted({f["name"] for f in json.load(sys.stdin)["formulae"]})))' <<< "$info")

mkdir -p "$LICENSES"
cp LICENSE "$LICENSES/mpc-qt-LICENSE"
python3 -c 'import json, sys
for f in sorted(json.load(sys.stdin)["formulae"], key=lambda f: f["name"]):
    print(f["name"], f["installed"][0]["version"], f["license"], f["urls"]["stable"]["url"])' \
    <<< "$info" > "$LICENSES/VERSIONS"

for f in $formulae; do
    copied=0
    for l in "$BREW/opt/$f"/{LICENSE,LICENCE,COPYING,COPYRIGHT,NOTICE}*; do
        [ -f "$l" ] || continue
        cp "$l" "$LICENSES/$f-$(basename "$l")"
        copied=1
    done
    [ "$copied" = 1 ] && continue
    # Some kegs (Qt, glib) carry no license text; take it from the source
    brew fetch --quiet --formula --build-from-source "$f" > /dev/null
    src=$(brew --cache --formula --build-from-source "$f")
    tar -tf "$src" | grep -E '^[^/]+/(LICENSES/[^/]+|(LICENSE|LICENCE|COPYING)[^/]*)$' |
        while IFS= read -r member; do
            tar -xOf "$src" "$member" > "$LICENSES/$f-$(basename "$member")"
        done
done

echo Checking that nothing still loads from Homebrew
leftovers=0
while IFS= read -r -d '' f; do
    file -b "$f" | grep -q Mach-O || continue
    if otool -L "$f" | sed 1d | grep -q "$BREW/"; then
        echo "Still links into Homebrew: $f" >&2
        leftovers=1
    fi
done < <(find "$APP" -type f -print0)
[ "$leftovers" = 0 ]

echo Signing and zipping
codesign --force --deep --sign - "$APP"
ditto -c -k --keepParent "$APP" "$DEST.zip"
echo "$DEST.zip: $(wc -l < "$LICENSES/VERSIONS" | tr -d ' ') formulae bundled"
