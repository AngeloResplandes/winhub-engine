#!/bin/sh
# Builds DXMT against $WORK/wine-build (build-wine.sh) and $WORK/llvm-darwin (build-llvm.sh).
# wine_builtin_dll=true: d3d11/dxgi/d3d10core/winemetal are Wine builtin DLLs. They stay in the
# runtime's lib/dxmt and winhubcompat.so puts that folder first in Wine's DLL search path when
# DXMT is the backend; nothing is copied into bottles.
# Output: $WORK/dxmt-install/{x64,x32} (the 64-bit build also has the winemetal unix library and
# nvapi64/nvngx, NVIDIA's APIs over Metal; they only answer with DXMT_ENABLE_NVEXT=1).
. "$(dirname "$0")/common.sh"

src="$WORK/dxmt"
clone_pinned "$DXMT_REPO" "$DXMT_TAG" "$DXMT_COMMIT" "$src"
# The DirectX headers, and NVIDIA's NVAPI headers (MIT) for DXMT's nvapi64/nvngx. Both are pinned by
# the DXMT commit.
git -C "$src" submodule update --init --depth 1 include/native/directx external/nvapi
# vcs_tag runs `git describe`; a shallow clone needs the tag fetched.
git -C "$src" fetch --depth 1 origin "refs/tags/$DXMT_TAG:refs/tags/$DXMT_TAG" 2>/dev/null || true
# Start from the pinned sources, then apply patches/dxmt (small fixes taken from upstream).
git -C "$src" checkout -- .
for patch in "$ROOT"/patches/dxmt/*.patch; do
    echo "Applying $(basename "$patch")"
    git -C "$src" apply "$patch"
done

out="$WORK/dxmt-install"
rm -rf "$out" "$src/build64" "$src/build32"
cd "$src"
meson setup --cross-file build-win64.txt \
    -Dnative_llvm_path="$WORK/llvm-darwin" -Dwine_build_path="$WORK/wine-build" -Dwine_builtin_dll=true \
    -Denable_nvapi=true -Denable_nvngx=true \
    --buildtype release --strip --prefix "$out/x64" build64
meson compile -C build64 --ninja-args=-k0
meson install -C build64

meson setup --cross-file build-win32.txt \
    -Dwine_build_path="$WORK/wine-build" -Dwine_builtin_dll=true \
    --buildtype release --strip --prefix "$out/x32" build32
meson compile -C build32 --ninja-args=-k0
meson install -C build32

find "$out" -type f | sort
