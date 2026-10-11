#!/bin/sh
# Packs the release assets into $WORK/dist:
#   winhub-wine-<wine>-r<rev>-overlay.tar.xz  files laid over the official WineHQ build's "wine" folder
#   cabextract-<ver>-macos.tar.xz             universal cabextract
#   wine-<ver>.tar.xz, cabextract-<ver>.tar.gz, winhub-engine-r<rev>-patches.tar.xz  corresponding sources
#   SHA256SUMS and winhub-manifests.json (entries for WinHub's runtimes.json / components.json)
. "$(dirname "$0")/common.sh"

name="winhub-wine-$WINE_VERSION-r$ENGINE_REVISION"
dist="$WORK/dist"
stage="$WORK/stage"
rm -rf "$dist" "$stage"
mkdir -p "$dist" "$stage/overlay"

# find_one <dir> <file name>: the single file with that name under dir.
find_one() {
    found="$(find "$1" -type f -name "$2")"
    if [ -z "$found" ] || [ "$(printf '%s\n' "$found" | wc -l)" -ne 1 ]; then
        echo "expected one $2 under $1, found: ${found:-none}" >&2
        exit 1
    fi
    echo "$found"
}

o="$stage/overlay"
mkdir -p "$o/lib/wine/x86_64-unix" "$o/lib/wine/x86_64-windows" "$o/lib/wine/i386-windows" \
         "$o/lib/dxmt/x64" "$o/lib/dxmt/x32" "$o/share/winhub-engine"
cp "$WORK/wine-build/dlls/winemac.drv/winemac.so" "$o/lib/wine/x86_64-unix/winemac.so"
strip -x "$o/lib/wine/x86_64-unix/winemac.so"
cp "$(find_one "$WORK/dxmt-install/x64" winemetal.so)" "$o/lib/wine/x86_64-unix/winemetal.so"
cp "$(find_one "$WORK/dxmt-install/x64" winemetal.dll)" "$o/lib/wine/x86_64-windows/winemetal.dll"
cp "$(find_one "$WORK/dxmt-install/x32" winemetal.dll)" "$o/lib/wine/i386-windows/winemetal.dll"
for arch in x64 x32; do
    for dll in d3d11 dxgi d3d10core winemetal; do
        cp "$(find_one "$WORK/dxmt-install/$arch" "$dll.dll")" "$o/lib/dxmt/$arch/$dll.dll"
    done
done
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$o/share/winhub-engine/"
cp "$WORK/dxmt/LICENSE" "$o/share/winhub-engine/DXMT-LICENSE"
cat > "$o/share/winhub-engine/engine.json" <<EOF
{
  "revision": $ENGINE_REVISION,
  "wine": "$WINE_VERSION",
  "dxmt": "$DXMT_TAG",
  "dxmtCommit": "$DXMT_COMMIT",
  "features": ["dxmt"]
}
EOF
tar -cJf "$dist/$name-overlay.tar.xz" -C "$o" .

cab="cabextract-$CABEXTRACT_VERSION"
mkdir -p "$stage/$cab"
cp "$WORK/cabextract-install/cabextract" "$stage/$cab/"
cp "$WORK/$cab/COPYING" "$stage/$cab/COPYING"
tar -cJf "$dist/$cab-macos.tar.xz" -C "$stage" "$cab"

# Sources: the exact upstream archives and this repository's patches and scripts.
cp "$DL/wine-$WINE_VERSION.tar.xz" "$DL/$cab.tar.gz" "$dist/"
tar -cJf "$dist/winhub-engine-r$ENGINE_REVISION-patches.tar.xz" -C "$ROOT" \
    LICENSE THIRD_PARTY_NOTICES.md README.md versions.env patches scripts

cd "$dist"
shasum -a 256 ./*.tar.* | sed 's| \./| |' > SHA256SUMS

sum() { shasum -a 256 "$1" | cut -d' ' -f1; }
size() { stat -f %z "$1"; }
repo="${GITHUB_REPOSITORY:-AngeloResplandes/winhub-engine}"
base="https://github.com/$repo/releases/download/wine-$WINE_VERSION-r$ENGINE_REVISION"
cat > winhub-manifests.json <<EOF
{
  "runtime": {
    "id": "$name",
    "name": "Wine (WineHQ) + WinHub",
    "version": "$WINE_VERSION-r$ENGINE_REVISION",
    "url": "$WINE_BASE_URL",
    "sha256": "$WINE_BASE_SHA256",
    "size": $WINE_BASE_SIZE,
    "license": "LGPL-2.1",
    "sourceURL": "https://github.com/$repo",
    "binPath": "$WINE_BASE_PREFIX/bin",
    "features": ["dxmt"],
    "overlay": {
      "url": "$base/$name-overlay.tar.xz",
      "sha256": "$(sum "$name-overlay.tar.xz")",
      "size": $(size "$name-overlay.tar.xz")
    }
  },
  "cabextract": {
    "id": "$cab-r$ENGINE_REVISION",
    "name": "cabextract",
    "version": "$CABEXTRACT_VERSION",
    "kind": "archive",
    "url": "$base/$cab-macos.tar.xz",
    "sha256": "$(sum "$cab-macos.tar.xz")",
    "size": $(size "$cab-macos.tar.xz"),
    "license": "GPL-3.0-or-later",
    "sourceURL": "$base/$cab.tar.gz",
    "path": "$cab"
  }
}
EOF
ls -l "$dist"
