#!/bin/sh
# Packs the release assets into $WORK/dist:
#   winhub-wine-<wine>-r<rev>-overlay.tar.xz  the engine. WinHub removes bin/ and lib/wine/ from the
#                                             official WineHQ build's "wine" folder and unpacks this
#                                             over it (the base keeps its libraries, wine-mono, gecko)
#   cabextract-<ver>-macos.tar.xz             universal cabextract
#   crossover-sources-<ver>.tar.gz, cJSON-<ver>.tar.gz, cabextract-<ver>.tar.gz,
#   winhub-engine-r<rev>-sources.tar.xz       corresponding sources
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

# Wine (bin/, lib/wine/, share/wine/) and winhubcompat.so next to ntdll.so. bin/ gets the loader,
# the server and the same program links as the official build (no development tools).
mkdir -p "$o/bin" "$o/lib" "$o/share"
cp "$WORK/wine-install/bin/wine" "$WORK/wine-install/bin/wineserver" "$o/bin/"
for program in msidb msiexec notepad regedit regsvr32 wineboot winecfg wineconsole winedbg winefile winemine winepath; do
    ln -s wine "$o/bin/$program"
done
cp -R "$WORK/wine-install/lib/wine" "$o/lib/wine"
cp -R "$WORK/wine-install/share/wine" "$o/share/wine"
cp "$WORK/winhubcompat/winhubcompat.so" "$o/lib/wine/x86_64-unix/"
strip -x "$o/bin/wine" "$o/bin/wineserver"
find "$o/lib/wine/x86_64-unix" -name '*.so' -exec strip -x {} \; 2>/dev/null
# Import libraries are only for building; the Windows-side files lose their debug info and symbols
# (the builtin marker stays).
find "$o/lib/wine" -name '*.a' -delete
find "$o/lib/wine/x86_64-windows" -type f -exec x86_64-w64-mingw32-strip --strip-unneeded {} \;
find "$o/lib/wine/i386-windows" -type f -exec i686-w64-mingw32-strip --strip-unneeded {} \;

# DXMT, laid out like a Wine library folder: winhubcompat.so prepends lib/dxmt to the DLL path.
mkdir -p "$o/lib/dxmt/x86_64-windows" "$o/lib/dxmt/i386-windows" "$o/lib/dxmt/x86_64-unix"
for dll in d3d11 dxgi d3d10core winemetal; do
    cp "$(find_one "$WORK/dxmt-install/x64" "$dll.dll")" "$o/lib/dxmt/x86_64-windows/$dll.dll"
    cp "$(find_one "$WORK/dxmt-install/x32" "$dll.dll")" "$o/lib/dxmt/i386-windows/$dll.dll"
done
cp "$(find_one "$WORK/dxmt-install/x64" winemetal.so)" "$o/lib/dxmt/x86_64-unix/winemetal.so"
# Wine finds a builtin it does not know only through its file in system32, which wineboot creates for
# the DLLs in lib/wine (CrossOver ships winemetal.dll there too). The one in lib/dxmt is the one loaded.
cp "$o/lib/dxmt/x86_64-windows/winemetal.dll" "$o/lib/wine/x86_64-windows/"
cp "$o/lib/dxmt/i386-windows/winemetal.dll" "$o/lib/wine/i386-windows/"
strip -x "$o/lib/dxmt/x86_64-unix/winemetal.so"

# DXVK, the same way (lib/dxvk).
mkdir -p "$o/lib/dxvk"
cp -R "$WORK/dxvk-install/x86_64-windows" "$WORK/dxvk-install/i386-windows" "$o/lib/dxvk/"

e="$o/share/winhub-engine"
mkdir -p "$e"
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$e/"
cp "$ROOT/data/compatdb.json" "$e/compatdb.json"
cp "$WORK/dxmt/LICENSE" "$e/DXMT-LICENSE"
cp "$WORK/dxvk-cx$CROSSOVER_VERSION/LICENSE" "$e/DXVK-LICENSE"
cp "$(cjson_dir)/LICENSE" "$e/cJSON-LICENSE"
features='["dxmt", "dxvk", "msync", "backend-dispatch", "compatdb"]'
cat > "$e/engine.json" <<EOF
{
  "revision": $ENGINE_REVISION,
  "wine": "$WINE_VERSION",
  "crossoverSources": "$CROSSOVER_VERSION",
  "dxmt": "$DXMT_TAG",
  "dxmtCommit": "$DXMT_COMMIT",
  "dxvk": "$(cat "$WORK/dxvk-cx$CROSSOVER_VERSION/RELEASE")",
  "features": $features
}
EOF
tar -cJf "$dist/$name-overlay.tar.xz" -C "$o" .

cab="cabextract-$CABEXTRACT_VERSION"
mkdir -p "$stage/$cab"
cp "$WORK/cabextract-install/cabextract" "$stage/$cab/"
cp "$WORK/$cab/COPYING" "$stage/$cab/COPYING"
tar -cJf "$dist/$cab-macos.tar.xz" -C "$stage" "$cab"

# Sources: the exact upstream archives and this repository (patches, winhubcompat, scripts).
cp "$DL/crossover-sources-$CROSSOVER_VERSION.tar.gz" "$DL/cJSON-$CJSON_VERSION.tar.gz" "$DL/$cab.tar.gz" "$dist/"
tar -cJf "$dist/winhub-engine-r$ENGINE_REVISION-sources.tar.xz" -C "$ROOT" \
    LICENSE THIRD_PARTY_NOTICES.md README.md versions.env patches src data scripts tests

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
    "name": "Wine $WINE_VERSION (CrossOver $CROSSOVER_VERSION sources) + WinHub",
    "version": "$WINE_VERSION-r$ENGINE_REVISION",
    "url": "$WINE_BASE_URL",
    "sha256": "$WINE_BASE_SHA256",
    "size": $WINE_BASE_SIZE,
    "license": "LGPL-2.1",
    "sourceURL": "https://github.com/$repo",
    "binPath": "$WINE_BASE_PREFIX/bin",
    "features": $features,
    "overlay": {
      "url": "$base/$name-overlay.tar.xz",
      "sha256": "$(sum "$name-overlay.tar.xz")",
      "size": $(size "$name-overlay.tar.xz"),
      "replace": ["bin", "lib/wine"]
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
