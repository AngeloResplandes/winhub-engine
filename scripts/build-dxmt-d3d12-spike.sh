#!/bin/sh
# D3D12 spike only (branch spike-d3d12): builds upstream DXMT from main with its Direct3D 12
# (src/d3d12, meson option enable_d3d12, off by default) against $WORK/wine-build and
# $WORK/llvm-darwin, like build-dxmt.sh. Not part of the release build; versions.env is untouched.
# DXMT after v0.80 is LGPL-2.1 (v0.80 and older were MIT).
# Output: $WORK/dxmt-next-install/x64 (and x32 with SPIKE_32=1).
. "$(dirname "$0")/common.sh"

DXMT_NEXT_COMMIT=50102f994b406f58f56d5d09f2ad20f9b90390a2 # main, 2026-10-16

src="$WORK/dxmt-next"
if [ ! -d "$src/.git" ]; then
    mkdir -p "$src"
    git -C "$src" init -q
    git -C "$src" remote add origin "$DXMT_REPO"
    git -C "$src" fetch --depth 1 origin "$DXMT_NEXT_COMMIT"
    git -C "$src" checkout -q FETCH_HEAD
fi
actual="$(git -C "$src" rev-parse HEAD)"
[ "$actual" = "$DXMT_NEXT_COMMIT" ] || { echo "$src is $actual, expected $DXMT_NEXT_COMMIT" >&2; exit 1; }
# Same submodules as v0.80, pinned by the commit: DirectX headers and NVIDIA's NVAPI headers.
git -C "$src" submodule update --init --depth 1 include/native/directx external/nvapi
git -C "$src" checkout -- .
# patches/dxmt holds fixes taken from upstream; apply only the ones this commit does not have yet.
for patch in "$ROOT"/patches/dxmt/*.patch; do
    if git -C "$src" apply --check -R "$patch" 2>/dev/null; then
        echo "Skipping $(basename "$patch") (already upstream)"
    else
        echo "Applying $(basename "$patch")"
        git -C "$src" apply "$patch"
    fi
done

out="$WORK/dxmt-next-install"
rm -rf "$out" "$src/build64" "$src/build32"
cd "$src"
# The version comes from `git describe`; keep git from finding this repository above $WORK.
export GIT_CEILING_DIRECTORIES="$WORK"
meson setup --cross-file build-win64.txt \
    -Dnative_llvm_path="$WORK/llvm-darwin" -Dwine_build_path="$WORK/wine-build" -Dwine_builtin_dll=true \
    -Denable_nvapi=true -Denable_nvngx=true -Denable_d3d12=true \
    --buildtype release --strip --prefix "$out/x64" build64
meson compile -C build64 --ninja-args=-k0
meson install -C build64

if [ "${SPIKE_32:-0}" = 1 ]; then
    meson setup --cross-file build-win32.txt \
        -Dwine_build_path="$WORK/wine-build" -Dwine_builtin_dll=true -Denable_d3d12=true \
        --buildtype release --strip --prefix "$out/x32" build32
    meson compile -C build32 --ninja-args=-k0
    meson install -C build32
fi

find "$out" -type f | sort
