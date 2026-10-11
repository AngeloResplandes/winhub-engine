#!/bin/sh
# Builds, from the WineHQ source plus patches/wine, the parts of Wine the engine replaces or
# that DXMT links against: winemac.so, ntdll.so, winebuild and the import libraries.
# Output: $WORK/wine-build (a Wine build tree, used as DXMT's wine_build_path).
. "$(dirname "$0")/common.sh"

archive="$(fetch "$WINE_URL" "$WINE_SHA256" "wine-$WINE_VERSION.tar.xz")"
src="$WORK/wine-$WINE_VERSION"
build="$WORK/wine-build"

rm -rf "$src" "$build"
tar -xf "$archive" -C "$WORK"
for patch in "$ROOT"/patches/wine/*.patch; do
    echo "Applying $(basename "$patch")"
    patch -d "$src" -p1 --forward < "$patch"
done

# Homebrew's bison (Wine needs 3.x); the system one is 2.3.
BISON_DIR="$(brew --prefix bison)/bin"
mkdir -p "$build"
cd "$build"
x86 /usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.15 PATH="$BISON_DIR:$PATH" \
    "$src/configure" \
    CC="clang -arch x86_64" CXX="clang++ -arch x86_64" OBJC="clang -arch x86_64" \
    --enable-archs=i386,x86_64 --disable-tests \
    --without-x --without-freetype --without-gstreamer --without-sdl --without-cups --without-gphoto \
    --without-sane --without-krb5 --without-netapi --without-pcap --without-usb --without-v4l2 \
    --without-pulse --without-oss --without-alsa --without-capi --without-unwind --without-inotify \
    --without-ffmpeg

targets="tools/winebuild/winebuild dlls/winemac.drv/winemac.so dlls/ntdll/ntdll.so"
for arch in x86_64 i386; do
    targets="$targets libs/winecrt0/$arch-windows/libwinecrt0.a dlls/ntdll/$arch-windows/libntdll.a"
    targets="$targets dlls/dbghelp/$arch-windows/libdbghelp.a"
done
# shellcheck disable=SC2086
x86 /usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.15 PATH="$BISON_DIR:$PATH" make -j"$JOBS" $targets

if ! nm -gU dlls/winemac.drv/winemac.so | grep -q ' _macdrv_functions$'; then
    echo "winemac.so does not export macdrv_functions" >&2
    exit 1
fi
echo "Wine build ready in $build"
