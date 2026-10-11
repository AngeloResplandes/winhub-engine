#!/bin/sh
# Builds cabextract as a universal (arm64 + x86_64) command-line tool with an ad-hoc signature.
# Output: $WORK/cabextract-install/cabextract
. "$(dirname "$0")/common.sh"

archive="$(fetch "$CABEXTRACT_URL" "$CABEXTRACT_SHA256" "cabextract-$CABEXTRACT_VERSION.tar.gz")"
src="$WORK/cabextract-$CABEXTRACT_VERSION"
out="$WORK/cabextract-install"
rm -rf "$src" "$out"
tar -xzf "$archive" -C "$WORK"
cd "$src"
./configure --disable-dependency-tracking \
    CFLAGS="-O2 -arch arm64 -arch x86_64 -mmacosx-version-min=11.0"
make -j"$JOBS"
mkdir -p "$out"
cp cabextract "$out/cabextract"
codesign --force --sign - "$out/cabextract"
lipo -info "$out/cabextract"
"$out/cabextract" --version
