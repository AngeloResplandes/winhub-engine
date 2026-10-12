#!/bin/sh
# Builds DXVK (Direct3D 9/10/11 over Vulkan, which MoltenVK runs on Metal) from the copy in
# CodeWeavers' CrossOver sources: 1.10.3, the last series that works with MoltenVK, with their fixes.
# As in CrossOver, Wine keeps its own dxgi.dll (enable_dxgi=false).
# The DLLs are marked as Wine builtins (winebuild --builtin) because they are loaded from the
# runtime's lib/dxvk, which winhubcompat.so puts first in Wine's DLL search path.
# Output: $WORK/dxvk-install/{x86_64-windows,i386-windows}
. "$(dirname "$0")/common.sh"

archive="$(fetch "$WINE_URL" "$WINE_SHA256" "crossover-sources-$CROSSOVER_VERSION.tar.gz")"
src="$WORK/dxvk-cx$CROSSOVER_VERSION"
out="$WORK/dxvk-install"
winebuild="$WORK/wine-build/tools/winebuild/winebuild"
test -x "$winebuild" || { echo "run build-wine.sh first (needs winebuild)" >&2; exit 1; }

rm -rf "$src" "$out"
mkdir -p "$src"
tar -xzf "$archive" -C "$src" --strip-components 2 sources/dxvk
for patch in "$ROOT"/patches/dxvk/*.patch; do
    echo "Applying $(basename "$patch")"
    patch -d "$src" -p1 --forward < "$patch"
done
cd "$src"
# The version comes from `git describe`; keep git from finding this repository above $WORK.
export GIT_CEILING_DIRECTORIES="$WORK"
# 1.10.3 predates GCC 13, which stopped including <cstdint> through other headers.
for bits in 64 32; do
    meson setup --cross-file "build-win$bits.txt" --buildtype release --strip \
        --prefix "$src/install$bits" -Denable_tests=false -Denable_dxgi=false \
        -Dcpp_args="-include cstdint" "build$bits"
    meson compile -C "build$bits"
    meson install -C "build$bits"
done

mkdir -p "$out/x86_64-windows" "$out/i386-windows"
cp "$src"/install64/bin/*.dll "$out/x86_64-windows/"
cp "$src"/install32/bin/*.dll "$out/i386-windows/"
"$winebuild" --builtin "$out"/*/*.dll
find "$out" -type f | sort
