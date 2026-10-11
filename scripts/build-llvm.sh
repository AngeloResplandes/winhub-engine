#!/bin/sh
# Builds the x86_64 LLVM 15 libraries DXMT's shader converter links against
# (same settings as DXMT's own CI). Output: $WORK/llvm-darwin. Slow; the workflow caches it.
. "$(dirname "$0")/common.sh"

prefix="$WORK/llvm-darwin"
if [ -f "$prefix/.winhub-llvm-$LLVM_COMMIT" ]; then
    echo "LLVM already built in $prefix"
    exit 0
fi

src="$WORK/llvm-project"
clone_pinned "$LLVM_REPO" "$LLVM_TAG" "$LLVM_COMMIT" "$src"

rm -rf "$WORK/llvm-darwin-build" "$prefix"
cmake -B "$WORK/llvm-darwin-build" -S "$src/llvm" \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 \
    -DLLVM_HOST_TRIPLE=x86_64-apple-darwin \
    -DLLVM_ENABLE_ASSERTIONS=On \
    -DLLVM_ENABLE_ZSTD=Off \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD="" \
    -DLLVM_BUILD_TOOLS=Off \
    -DLLVM_VERSION_PRINTER_SHOW_HOST_TARGET_INFO=Off \
    -G Ninja
cmake --build "$WORK/llvm-darwin-build"
cmake --install "$WORK/llvm-darwin-build"
touch "$prefix/.winhub-llvm-$LLVM_COMMIT"
echo "LLVM ready in $prefix"
