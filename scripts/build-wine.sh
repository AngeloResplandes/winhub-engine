#!/bin/sh
# Builds all of Wine from CodeWeavers' LGPL sources for CrossOver plus patches/wine, as x86_64 with
# WoW64 (32-bit programs run inside 64-bit processes, as in CrossOver). freetype, gnutls, SDL2 and
# Vulkan (MoltenVK) are linked against the libraries of the official WineHQ build the engine is laid
# over; Homebrew only supplies their headers.
# Output: $WORK/wine-build (build tree, also DXMT's wine_build_path) and $WORK/wine-install
# (bin/, lib/wine/, share/wine/).
. "$(dirname "$0")/common.sh"

archive="$(fetch "$WINE_URL" "$WINE_SHA256" "crossover-sources-$CROSSOVER_VERSION.tar.gz")"
src="$WORK/wine-$WINE_VERSION-cx$CROSSOVER_VERSION"
build="$WORK/wine-build"
install="$WORK/wine-install"
base="$(base_wine)"

rm -rf "$src" "$build" "$install"
mkdir -p "$src"
tar -xzf "$archive" -C "$src" --strip-components 2 sources/wine
for patch in "$ROOT"/patches/wine/*.patch; do
    echo "Applying $(basename "$patch")"
    patch -d "$src" -p1 --forward < "$patch"
done

# configure takes each library's file name (its "soname") from the install name of the dylib it links
# against. The base build's dylibs have bare install names (libSDL2.dylib), so link against copies
# whose install names carry the versioned file name Wine should dlopen at run time.
# Vulkan goes straight to MoltenVK, as in CrossOver: CodeWeavers' win32u does not ask the Vulkan
# loader for portability drivers, so the loader would find none. Leaving libvulkan out makes
# configure pick libMoltenVK.dylib.
deps="$WORK/deps/lib"
rm -rf "$WORK/deps"
mkdir -p "$deps"
for lib in libfreetype.6 libgnutls.30 libSDL2-2.0.0 libMoltenVK; do
    cp -L "$base/lib/$lib.dylib" "$deps/$lib.dylib"
    chmod u+w "$deps/$lib.dylib"
    install_name_tool -id "$deps/$lib.dylib" "$deps/$lib.dylib" 2>/dev/null
    codesign -f -s - "$deps/$lib.dylib" 2>/dev/null
done
ln -s libfreetype.6.dylib "$deps/libfreetype.dylib"
ln -s libgnutls.30.dylib "$deps/libgnutls.dylib"
ln -s libSDL2-2.0.0.dylib "$deps/libSDL2.dylib"
# Their own dependencies (libpng, nettle...) are found next to them (@loader_path); build tools such
# as sfnt2fon run against these copies.
for lib in "$base"/lib/*.dylib; do
    case "$(basename "$lib")" in libvulkan*) continue ;; esac
    [ -e "$deps/$(basename "$lib")" ] || ln -s "$lib" "$deps/$(basename "$lib")"
done

# Homebrew's bison (Wine needs 3.x); the system one is 2.3.
BISON_DIR="$(brew --prefix bison)/bin"
inc() { echo "-I$(brew --prefix "$1")/include${2:-}"; }
# Unix libraries find the base build's dylibs in lib/ (lib/wine/x86_64-unix/../../), like the base itself.
rpath="-Wl,-rpath,@loader_path/../../ -Wl,-rpath,@loader_path/../lib"

# GStreamer for game videos (scripts/build-gstreamer.sh); winegstreamer finds it in lib/ at run time.
# gobject-2.0.pc requires libffi.pc, which build-gstreamer.sh writes to $WORK/gst-deps/pkgconfig (the SDK's).
# gstreamer-1.0.pc adds an rpath to its build folder, which is dropped. -lintl (from glib-2.0.pc) must
# find $WORK/gst-deps/lib's libintl, whose install name is @rpath/libintl.8.dylib, before $deps' (the
# base's, whose install name is a bare libintl.dylib that dyld would not find).
gst="$WORK/gstreamer-install"
test -f "$gst/lib/pkgconfig/gstreamer-1.0.pc" || { echo "run build-gstreamer.sh first" >&2; exit 1; }
gst_pc() {
    PKG_CONFIG_LIBDIR="$gst/lib/pkgconfig:$WORK/gst-deps/pkgconfig" pkgconf "$@" \
        gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-tag-1.0
}

mkdir -p "$build"
cd "$build"
x86 /usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.15 PATH="$BISON_DIR:$PATH" \
    "$src/configure" \
    CC="clang -arch x86_64" CXX="clang++ -arch x86_64" OBJC="clang -arch x86_64" \
    LDFLAGS="$rpath" \
    FREETYPE_CFLAGS="$(inc freetype /freetype2)" FREETYPE_LIBS="-L$deps -lfreetype" \
    GNUTLS_CFLAGS="$(inc gnutls) $(inc nettle) $(inc libtasn1) $(inc p11-kit /p11-kit-1)" GNUTLS_LIBS="-L$deps -lgnutls" \
    SDL2_CFLAGS="$(inc sdl2 /SDL2)" SDL2_LIBS="-L$deps -lSDL2" \
    GSTREAMER_CFLAGS="$(gst_pc --cflags)" \
    GSTREAMER_LIBS="$(gst_pc --libs | sed 's/-Wl,-rpath,[^ ]*//g') -L$WORK/gst-deps/lib" \
    LIBS="-L$deps" \
    --prefix=/ --enable-archs=i386,x86_64 --disable-tests \
    --with-freetype --with-gnutls --with-sdl --with-vulkan \
    --without-x --with-gstreamer --without-cups --without-gphoto --without-sane --without-krb5 \
    --without-netapi --without-pcap --without-usb --without-v4l2 --without-pulse --without-oss \
    --without-alsa --without-capi --without-unwind --without-inotify --without-ffmpeg

for feature in SONAME_LIBFREETYPE SONAME_LIBGNUTLS SONAME_LIBSDL2; do
    if ! grep -q "#define $feature" include/config.h; then
        echo "configure did not find $feature" >&2
        exit 1
    fi
done
if ! grep -q '#define SONAME_LIBVULKAN "libMoltenVK.dylib"' include/config.h; then
    echo "configure did not pick libMoltenVK.dylib for Vulkan" >&2
    exit 1
fi

x86 /usr/bin/env MACOSX_DEPLOYMENT_TARGET=10.15 PATH="$BISON_DIR:$PATH" make -j"$JOBS"
x86 make install DESTDIR="$install" >/dev/null

for f in lib/wine/x86_64-unix/winemac.so lib/wine/x86_64-unix/ntdll.so; do
    test -f "$install/$f" || { echo "missing $f" >&2; exit 1; }
done
test -f "$install/lib/wine/x86_64-unix/winegstreamer.so" || { echo "missing winegstreamer.so" >&2; exit 1; }
otool -L "$install/lib/wine/x86_64-unix/winegstreamer.so" | grep -q '@rpath/libgstreamer-1.0.0.dylib' \
    || { echo "winegstreamer.so does not link the engine's GStreamer" >&2; exit 1; }
if otool -L "$install/lib/wine/x86_64-unix/winegstreamer.so" | sed 1d \
        | grep -v -E '^[[:space:]]*(@rpath/|/usr/lib/|/System/Library/)' | grep -q .; then
    echo "winegstreamer.so links a library outside @rpath and the system" >&2
    exit 1
fi
if otool -l "$install/lib/wine/x86_64-unix/winegstreamer.so" | grep -E -q "^[[:space:]]*path ($WORK|/opt/homebrew|/usr/local)"; then
    echo "winegstreamer.so has a build or Homebrew rpath" >&2
    exit 1
fi
nm -gU "$install/lib/wine/x86_64-unix/winemac.so" | grep -q ' _macdrv_functions$'
nm -gU "$install/lib/wine/x86_64-unix/ntdll.so" | grep -q ' _prepend_dll_path$'
echo "Wine $WINE_VERSION (CrossOver $CROSSOVER_VERSION sources) installed in $install"
