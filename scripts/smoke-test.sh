#!/bin/sh
# Installs the engine the way WinHub does (official WineHQ build, minus bin/ and lib/wine/, with the
# overlay unpacked over it) and checks it: winhubcompat.so is loaded and picks the backend asked
# for, and a Direct3D 11 program reaches feature level 11_0 through DXMT (64- and 32-bit) and DXVK
# (64-bit; a 32-bit program asking for DXVK falls back to DXMT), with no DLL copied into the bottle.
# A local replace_exe_path rule must make a 32-bit program start the 64-bit test instead, and DXMT's
# nvapi64/nvngx must answer only with DXMT_ENABLE_NVEXT=1.
# SMOKE_BACKENDS (default "dxmt dxvk") picks the backends to test.
. "$(dirname "$0")/common.sh"

name="winhub-wine-$WINE_VERSION-r$ENGINE_REVISION"
base="$(fetch "$WINE_BASE_URL" "$WINE_BASE_SHA256" "$(basename "$WINE_BASE_URL")")"
engine="$WORK/engine"
rm -rf "$engine"
mkdir -p "$engine"
tar -xf "$base" -C "$engine"
wine_dir="$engine/$WINE_BASE_PREFIX"
rm -rf "$wine_dir/bin" "$wine_dir/lib/wine"
tar -xf "$WORK/dist/$name-overlay.tar.xz" -C "$wine_dir"

nm -gU "$wine_dir/lib/wine/x86_64-unix/winemac.so" | grep ' _macdrv_functions$'
nm -gU "$wine_dir/lib/wine/x86_64-unix/ntdll.so" | grep ' _prepend_dll_path$'
test -f "$wine_dir/lib/wine/x86_64-unix/winhubcompat.so"

prefix="$WORK/smoke-prefix"
rm -rf "$prefix"
mkdir -p "$prefix"
wine() {
    env -i HOME="$HOME" USER="${USER:-runner}" TMPDIR="${TMPDIR:-/tmp}" LANG=en_US.UTF-8 \
        PATH="$wine_dir/bin:/usr/bin:/bin" WINEPREFIX="$prefix" ROSETTA_ADVERTISE_AVX=1 \
        WINEDEBUG="${DEBUG:--all}" WINHUB_GRAPHICS_BACKEND="${BACKEND:-auto}" WINHUB_COMPAT_RULES="${RULES:-}" \
        DXMT_ENABLE_NVEXT="${NVEXT:-}" \
        WINEDLLOVERRIDES="mscoree,mshtml=" "$wine_dir/bin/wine" "$@"
}
wine --version
wine wineboot --init

x86_64-w64-mingw32-gcc -O2 -o "$WORK/d3d11-smoke64.exe" "$ROOT/tests/d3d11-smoke.c" -ld3d11 -ldxgi -luuid
i686-w64-mingw32-gcc -O2 -o "$WORK/d3d11-smoke32.exe" "$ROOT/tests/d3d11-smoke.c" -ld3d11 -ldxgi -luuid
x86_64-w64-mingw32-gcc -O2 -o "$WORK/nvapi-smoke64.exe" "$ROOT/tests/nvapi-smoke.c"

# DXVK writes its logs and shader cache to the current folder.
cd "$WORK"
status=0
for backend in ${SMOKE_BACKENDS:-dxmt dxvk}; do
    for bits in 64 32; do
        echo "== $backend, $bits-bit"
        log="$WORK/smoke-$backend-$bits.log"
        BACKEND="$backend" DEBUG="-all,trace+winhubcompat" wine "$WORK/d3d11-smoke$bits.exe" b000 > "$log" 2>&1 || true
        grep -E '^(OK|FAIL|adapter|feature)' "$log" || true
        expected=$backend
        [ "$backend-$bits" = dxvk-32 ] && expected=dxmt  # see backend_usable() in winhubcompat.c
        if ! grep -q "using $expected as the graphics backend" "$log"; then
            echo "winhubcompat did not pick $expected" >&2
            status=1
        fi
        grep -q '^OK' "$log" || status=1
    done
done

# replace_exe_path only acts on child processes, so cmd starts the "launcher" (32-bit hostname.exe).
echo "== replace_exe_path"
mkdir -p "$prefix/drive_c/winhub-test"
cp "$prefix/drive_c/windows/syswow64/hostname.exe" "$prefix/drive_c/winhub-test/launcher.exe"
cp "$WORK/d3d11-smoke64.exe" "$prefix/drive_c/winhub-test/game.exe"
cat > "$WORK/smoke-rules.json" <<'RULES'
{ "version": 1, "rules": [ { "desc": "smoke launcher",
  "match": { "type": "path_suffix", "suffix": "winhub-test\\launcher.exe" },
  "hacks": [ { "type": "replace_exe_path", "path": "C:\\winhub-test\\game.exe" } ] } ] }
RULES
log="$WORK/smoke-replace.log"
RULES="$WORK/smoke-rules.json" BACKEND=dxmt DEBUG="-all,trace+winhubcompat" \
    wine cmd /c 'C:\winhub-test\launcher.exe' b000 > "$log" 2>&1 || true
grep -E '^(OK|FAIL)|replacing' "$log" || true
grep -q 'smoke launcher: replacing' "$log" || { echo "replace_exe_path was not applied" >&2; status=1; }
grep -q '^OK' "$log" || status=1

for nvext in on off; do
    echo "== nvidia extensions $nvext"
    log="$WORK/smoke-nvapi-$nvext.log"
    value=""; [ "$nvext" = on ] && value=1
    NVEXT="$value" BACKEND=dxmt DEBUG="-all,trace+winhubcompat" wine "$WORK/nvapi-smoke64.exe" "$nvext" > "$log" 2>&1 || true
    grep -E '^(OK|FAIL|nvapi64)' "$log" || true
    grep -q '^OK' "$log" || status=1
done

# Nothing was copied into the bottle: system32 only has the copies wineboot makes of lib/wine.
for dll in d3d11 dxgi d3d10core winemetal; do
    if ! cmp -s "$prefix/drive_c/windows/system32/$dll.dll" "$wine_dir/lib/wine/x86_64-windows/$dll.dll"; then
        echo "system32/$dll.dll is not Wine's own" >&2
        status=1
    fi
done

env -i PATH="$wine_dir/bin:/usr/bin:/bin" WINEPREFIX="$prefix" "$wine_dir/bin/wineserver" -k || true
exit $status
