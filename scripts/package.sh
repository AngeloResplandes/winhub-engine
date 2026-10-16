#!/bin/sh
# Packs the release assets into $WORK/dist:
#   winhub-wine-<wine>-r<rev>-overlay.tar.xz  the engine. WinHub removes bin/, lib/wine/ and
#                                             lib/gstreamer-1.0/ from the official WineHQ build's "wine"
#                                             folder and unpacks this over it (the base keeps its
#                                             libraries, wine-mono, gecko)
#   cabextract-<ver>-macos.tar.xz             universal cabextract
#   crossover-sources-<ver>.tar.gz, cJSON-<ver>.tar.gz, cabextract-<ver>.tar.gz, pcre2-<ver>.tar.bz2,
#   pcre2_<ver>_patch.zip, gvdb-<commit>.tar.xz, winhub-engine-r<rev>-sources.tar.xz
#                                             corresponding sources
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
# NVIDIA's APIs over Metal (64-bit only); winhubcompat points system32's nvngx.dll here when on.
for dll in nvapi64 nvngx; do
    cp "$(find_one "$WORK/dxmt-install/x64" "$dll.dll")" "$o/lib/dxmt/x86_64-windows/$dll.dll"
done
# Wine finds a builtin it does not know only through its file in system32, which wineboot creates for
# the DLLs in lib/wine (CrossOver ships winemetal.dll there too). The one in lib/dxmt is the one loaded.
cp "$o/lib/dxmt/x86_64-windows/winemetal.dll" "$o/lib/wine/x86_64-windows/"
cp "$o/lib/dxmt/i386-windows/winemetal.dll" "$o/lib/wine/i386-windows/"
strip -x "$o/lib/dxmt/x86_64-unix/winemetal.so"

# DXVK, the same way (lib/dxvk).
mkdir -p "$o/lib/dxvk"
cp -R "$WORK/dxvk-install/x86_64-windows" "$WORK/dxvk-install/i386-windows" "$o/lib/dxvk/"

# GStreamer for game videos: the plug-ins winegstreamer needs in lib/gstreamer-1.0 (winhubcompat points
# GStreamer there) and, in lib/, the libraries winegstreamer.so and those plug-ins load (their @rpath
# closure). libintl comes from the base build, which stays in lib/; libffi, libiconv and zlib from macOS.
# Not shipped: the other plug-ins (opengl, pbtypes), static libraries, headers, pkg-config files and tools.
gst="$WORK/gstreamer-install"
mkdir -p "$o/lib/gstreamer-1.0"
for plugin in coreelements app typefindfunctions playback audioconvert audioresample videoconvertscale \
              isomp4 audioparsers wavparse deinterlace videofilter videoparsersbad applemedia; do
    cp "$gst/lib/gstreamer-1.0/libgst$plugin.dylib" "$o/lib/gstreamer-1.0/"
done
todo="$o/lib/wine/x86_64-unix/winegstreamer.so $(echo "$o"/lib/gstreamer-1.0/*.dylib)"
while [ -n "$todo" ]; do
    next=""
    for file in $todo; do
        for dep in $(otool -L "$file" | awk 'NR > 1 && $1 ~ /^@rpath\// { sub("@rpath/", "", $1); print $1 }'); do
            { [ -e "$o/lib/$dep" ] || [ ! -e "$gst/lib/$dep" ]; } && continue
            cp -L "$gst/lib/$dep" "$o/lib/$dep"
            next="$next $o/lib/$dep"
        done
    done
    todo="$next"
done
# Every @rpath library they load (and their own install names) must be next to them, in the overlay's
# lib/ or in the base build's.
base_lib="$(base_wine)/lib"
for file in "$o/lib/wine/x86_64-unix/winegstreamer.so" "$o"/lib/*.dylib "$o"/lib/gstreamer-1.0/*.dylib; do
    for dep in $(otool -L "$file" | awk 'NR > 1 && $1 ~ /^@rpath\// { sub("@rpath/", "", $1); print $1 }'); do
        [ -e "$(dirname "$file")/$dep" ] || [ -e "$o/lib/$dep" ] || [ -e "$base_lib/$dep" ] \
            || { echo "$file needs $dep, which is not shipped" >&2; exit 1; }
    done
done
# build-gstreamer.sh leaves them unsigned: strip, then sign (strip invalidates a signature, and Xcode 16's
# strip rejects a file whose signature was removed with codesign --remove-signature).
for file in "$o"/lib/*.dylib "$o"/lib/gstreamer-1.0/*.dylib; do
    strip -x "$file"
    codesign -f -s - "$file" 2>/dev/null
done

e="$o/share/winhub-engine"
mkdir -p "$e"
cp "$ROOT/LICENSE" "$ROOT/THIRD_PARTY_NOTICES.md" "$e/"
cp "$ROOT/data/compatdb.json" "$e/compatdb.json"
cp "$WORK/dxmt/LICENSE" "$e/DXMT-LICENSE"
cp "$WORK/dxmt/external/nvapi/License.txt" "$e/NVAPI-LICENSE"
cp "$WORK/dxvk-cx$CROSSOVER_VERSION/LICENSE" "$e/DXVK-LICENSE"
cp "$(cjson_dir)/LICENSE" "$e/cJSON-LICENSE"
cp "$WORK/gst-src/glib/COPYING" "$e/GLIB-LICENSE"
cp "$WORK/gst-src/gstreamer/subprojects/gstreamer/COPYING" "$e/GSTREAMER-LICENSE"
cp "$WORK/gst-src/glib/subprojects/pcre2-$PCRE2_VERSION/LICENCE" "$e/PCRE2-LICENSE"
features='["dxmt", "dxvk", "msync", "backend-dispatch", "compatdb", "nvext", "gstreamer"]'
cat > "$e/engine.json" <<EOF
{
  "revision": $ENGINE_REVISION,
  "wine": "$WINE_VERSION",
  "crossoverSources": "$CROSSOVER_VERSION",
  "dxmt": "$DXMT_TAG",
  "dxmtCommit": "$DXMT_COMMIT",
  "dxvk": "$(cat "$WORK/dxvk-cx$CROSSOVER_VERSION/RELEASE")",
  "gstreamer": "$GSTREAMER_VERSION",
  "features": $features
}
EOF
tar -cJf "$dist/$name-overlay.tar.xz" -C "$o" .

cab="cabextract-$CABEXTRACT_VERSION"
mkdir -p "$stage/$cab"
cp "$WORK/cabextract-install/cabextract" "$stage/$cab/"
cp "$WORK/$cab/COPYING" "$stage/$cab/COPYING"
tar -cJf "$dist/$cab-macos.tar.xz" -C "$stage" "$cab"

# Sources: the exact upstream archives and this repository (patches, winhubcompat, scripts). The
# CrossOver archive holds GLib and GStreamer; GLib's pcre2 (archive and meson wrap) and gvdb (a git
# subproject, archived at its pinned commit) come separately.
cp "$DL/crossover-sources-$CROSSOVER_VERSION.tar.gz" "$DL/cJSON-$CJSON_VERSION.tar.gz" "$DL/$cab.tar.gz" \
    "$DL/pcre2-$PCRE2_VERSION.tar.bz2" "$DL/$PCRE2_WRAP_FILE" "$dist/"
if [ "$(git -C "$WORK/gvdb" rev-parse HEAD)" != "$GVDB_COMMIT" ]; then
    echo "$WORK/gvdb is not at $GVDB_COMMIT; run build-gstreamer.sh" >&2
    exit 1
fi
git -C "$WORK/gvdb" archive --format=tar --prefix="gvdb-$GVDB_COMMIT/" HEAD | xz -c > "$dist/gvdb-$GVDB_COMMIT.tar.xz"
tar -cJf "$dist/winhub-engine-r$ENGINE_REVISION-sources.tar.xz" -C "$ROOT" \
    LICENSE THIRD_PARTY_NOTICES.md README.md versions.env patches src data scripts tests

cd "$dist"
shasum -a 256 ./*.tar.* ./*.zip | sed 's| \./| |' > SHA256SUMS

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
      "replace": ["bin", "lib/wine", "lib/gstreamer-1.0"]
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
