#!/bin/sh
# Builds GLib and the parts of GStreamer Wine's winegstreamer needs to play game videos (MP4 with H.264
# and AAC), as x86_64, from the copies in CodeWeavers' CrossOver sources. H.264 and AAC are decoded by
# macOS (gst-plugins-bad's applemedia: vtdec, atdec); no FFmpeg, x264 or GPL plug-ins.
# meson runs arm64 (Homebrew's Python), so the x86_64 build goes through a cross file; Rosetta runs the
# configure checks. libffi comes from the macOS SDK and libintl from the WineHQ base build, which the
# engine is laid over and which keeps them.
# Output: $WORK/gstreamer-install (lib/*.dylib, lib/gstreamer-1.0/, lib/pkgconfig/, include/)
. "$(dirname "$0")/common.sh"

archive="$(fetch "$WINE_URL" "$WINE_SHA256" "crossover-sources-$CROSSOVER_VERSION.tar.gz")"
pcre2="$(fetch "$PCRE2_URL" "$PCRE2_SHA256" "pcre2-$PCRE2_VERSION.tar.bz2")"
pcre2_wrap="$(fetch "$PCRE2_WRAP_URL" "$PCRE2_WRAP_SHA256" "$PCRE2_WRAP_FILE")"
src="$WORK/gst-src"
build="$WORK/gst-build"
prefix="$WORK/gstreamer-install"
deps="$WORK/gst-deps"
gvdb="$WORK/gvdb"
base="$(base_wine)"
sdk="$(xcrun --show-sdk-path)"

rm -rf "$src" "$build" "$prefix" "$deps"
mkdir -p "$src" "$build" "$prefix/lib" "$deps/lib" "$deps/pkgconfig"
tar -xzf "$archive" -C "$src" --strip-components 1 sources/glib sources/gstreamer
mkdir -p "$src/glib/subprojects/packagecache"
cp "$pcre2" "$pcre2_wrap" "$src/glib/subprojects/packagecache/"
# gvdb, a git subproject the CrossOver archive leaves empty. It has no tag, so fetch the pinned commit.
if [ ! -d "$gvdb/.git" ] || ! git -C "$gvdb" rev-parse -q --verify HEAD >/dev/null 2>&1; then
    rm -rf "$gvdb"
    git init -q "$gvdb"
    git -C "$gvdb" fetch -q --depth 1 "$GVDB_REPO" "$GVDB_COMMIT"
    git -C "$gvdb" checkout -q FETCH_HEAD
fi
if [ "$(git -C "$gvdb" rev-parse HEAD)" != "$GVDB_COMMIT" ]; then
    echo "$GVDB_REPO is $(git -C "$gvdb" rev-parse HEAD), expected $GVDB_COMMIT" >&2
    exit 1
fi
rm -rf "$src/glib/subprojects/gvdb"
mkdir "$src/glib/subprojects/gvdb"
git -C "$gvdb" archive HEAD | tar -xf - -C "$src/glib/subprojects/gvdb"

# libintl from the base build, with an @rpath install name (the base's is bare: libintl.dylib); at run
# time @loader_path finds the base's own copy in lib/. libffi is the SDK's (universal).
cp -L "$base/lib/libintl.8.dylib" "$deps/lib/"
chmod u+w "$deps/lib/libintl.8.dylib"
install_name_tool -id @rpath/libintl.8.dylib "$deps/lib/libintl.8.dylib" 2>/dev/null
ln -s libintl.8.dylib "$deps/lib/libintl.dylib"
ln -s "$base/lib/libiconv.2.dylib" "$deps/lib/libiconv.2.dylib"
cat > "$deps/pkgconfig/libffi.pc" <<EOF
Name: libffi
Description: libffi from the macOS SDK
Version: 3.4.4
Cflags: -I$sdk/usr/include/ffi
Libs: -lffi
EOF

flags="'-arch', 'x86_64', '-mmacosx-version-min=10.15'"
cat > "$WORK/gst-cross.ini" <<EOF
[binaries]
c = ['clang', $flags]
cpp = ['clang++', $flags]
objc = ['clang', $flags]
objcpp = ['clang++', $flags]
ar = 'ar'
strip = 'strip'
pkg-config = '$(command -v pkgconf)'
python = '/usr/bin/python3'

[constants]
args = ['-I$(brew --prefix gettext)/include', '-Werror=unguarded-availability-new']
link_args = ['-L$deps/lib', '-Wl,-rpath,@loader_path']

[built-in options]
c_args = args
c_link_args = link_args
cpp_args = args
cpp_link_args = link_args
objc_args = args
objc_link_args = link_args
objcpp_args = args
objcpp_link_args = link_args

[properties]
needs_exe_wrapper = false
# Newer than the deployment target, but meson's has_function only checks that the symbol links.
has_function_pipe2 = false
pkg_config_libdir = ['$prefix/lib/pkgconfig', '$deps/pkgconfig']

[host_machine]
system = 'darwin'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
EOF

# rpath_names: meson install gives the dylibs absolute install names in $prefix/lib. Make them, and every
# reference to them, @rpath/<file>; programs in bin/ also look in ../lib.
rpath_names() {
    find "$prefix" -type f \( -name '*.dylib' -o -name '*.so' -o -perm -u+x \) | while read -r f; do
        file -b "$f" | grep -q '^Mach-O' || continue
        set --
        id="$(otool -D "$f" | sed -n 2p)"
        case "$id" in "$prefix"/lib/*) set -- -id "@rpath/${id##*/}" ;; esac
        for dep in $(otool -L "$f" | sed -n "s|^[[:space:]]*\($prefix/lib/[^ ]*\) (.*|\1|p"); do
            set -- "$@" -change "$dep" "@rpath/${dep##*/}"
        done
        case "$f" in
            "$prefix"/bin/*)
                otool -l "$f" | grep -q 'path @loader_path/../lib ' || set -- "$@" -add_rpath @loader_path/../lib ;;
        esac
        [ $# -gt 0 ] || continue
        install_name_tool "$@" "$f" 2>/dev/null
        codesign -f -s - "$f" 2>/dev/null
    done
}

# meson_build <source dir> <name> [options]: configures, builds and installs one project into $prefix.
meson_build() {
    dir=$1 name=$2
    shift 2
    meson setup "$build/$name" "$dir" --cross-file "$WORK/gst-cross.ini" --prefix "$prefix" \
        --libdir lib --buildtype release --wrap-mode nodownload -Dauto_features=disabled "$@"
    meson compile -C "$build/$name"
    meson install -C "$build/$name" >/dev/null
    rpath_names
}

meson_build "$src/glib" glib --force-fallback-for=pcre2 \
    -Dtests=false -Dinstalled_tests=false -Dnls=disabled -Dman=false -Dgtk_doc=false \
    -Dselinux=disabled -Dxattr=false -Dlibmount=disabled -Ddtrace=false -Dsystemtap=false \
    -Dsysprof=disabled -Dlibelf=disabled -Dglib_debug=disabled

for lib in libglib-2.0.0 libgobject-2.0.0 libgmodule-2.0.0 libgio-2.0.0; do
    test -f "$prefix/lib/$lib.dylib" || { echo "missing $lib.dylib" >&2; exit 1; }
    lipo -archs "$prefix/lib/$lib.dylib" | grep -qx x86_64 || { echo "$lib.dylib is not x86_64" >&2; exit 1; }
    otool -D "$prefix/lib/$lib.dylib" | grep -q "@rpath/$lib.dylib" || { echo "$lib.dylib: install name is not @rpath" >&2; exit 1; }
    if otool -L "$prefix/lib/$lib.dylib" | sed 1d | grep -E -q "$WORK|/opt/homebrew|/usr/local"; then
        echo "$lib.dylib links against a build or Homebrew path" >&2
        exit 1
    fi
done
test -f "$prefix/lib/pkgconfig/glib-2.0.pc" || { echo "missing glib-2.0.pc" >&2; exit 1; }
echo "GLib $GLIB_VERSION installed in $prefix"
