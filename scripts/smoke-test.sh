#!/bin/sh
# Lays the overlay over the official WineHQ build, then checks the result:
# winemac.so exports macdrv_functions, Wine starts, and a Direct3D 11 program reaches
# feature level 11_0 through DXMT (64- and 32-bit), the way WinHub sets up a bottle.
. "$(dirname "$0")/common.sh"

name="winhub-wine-$WINE_VERSION-r$ENGINE_REVISION"
base="$(fetch "$WINE_BASE_URL" "$WINE_BASE_SHA256" "$(basename "$WINE_BASE_URL")")"
engine="$WORK/engine"
rm -rf "$engine"
mkdir -p "$engine"
tar -xf "$base" -C "$engine"
wine_dir="$engine/$WINE_BASE_PREFIX"
tar -xf "$WORK/dist/$name-overlay.tar.xz" -C "$wine_dir"

nm -gU "$wine_dir/lib/wine/x86_64-unix/winemac.so" | grep ' _macdrv_functions$'

prefix="$WORK/smoke-prefix"
rm -rf "$prefix"
mkdir -p "$prefix"
wine() {
    env -i HOME="$HOME" USER="${USER:-runner}" TMPDIR="${TMPDIR:-/tmp}" LANG=en_US.UTF-8 \
        PATH="$wine_dir/bin:/usr/bin:/bin" WINEPREFIX="$prefix" WINEDEBUG=-all ROSETTA_ADVERTISE_AVX=1 \
        WINEDLLOVERRIDES="mscoree,mshtml=;${OVERRIDES:-}" "$wine_dir/bin/wine" "$@"
}
wine --version
wine wineboot --init

# Same files and overrides WinHub uses when a bottle switches to DXMT.
cp "$wine_dir"/lib/dxmt/x64/*.dll "$prefix/drive_c/windows/system32/"
cp "$wine_dir"/lib/dxmt/x32/*.dll "$prefix/drive_c/windows/syswow64/"

x86_64-w64-mingw32-gcc -O2 -o "$WORK/d3d11-smoke64.exe" "$ROOT/tests/d3d11-smoke.c" -ld3d11 -ldxgi -luuid
i686-w64-mingw32-gcc -O2 -o "$WORK/d3d11-smoke32.exe" "$ROOT/tests/d3d11-smoke.c" -ld3d11 -ldxgi -luuid

status=0
for bits in 64 32; do
    echo "== DXMT, $bits-bit"
    OVERRIDES="d3d11,dxgi,d3d10core=n,b" wine "$WORK/d3d11-smoke$bits.exe" b000 2>&1 | tee "$WORK/smoke$bits.log" | grep -E '^(OK|FAIL|adapter|feature)'
    grep -q '^OK' "$WORK/smoke$bits.log" || status=1
done
env -i PATH="$wine_dir/bin:/usr/bin:/bin" WINEPREFIX="$prefix" "$wine_dir/bin/wineserver" -k || true
exit $status
