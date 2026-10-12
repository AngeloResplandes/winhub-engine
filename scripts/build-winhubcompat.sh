#!/bin/sh
# Builds winhubcompat.so (src/winhubcompat) against $WORK/wine-build (build-wine.sh): the Unix
# library CrossOver's ntdll loads at startup, here picking the Direct3D backend and applying the
# compatibility rules. Output: $WORK/winhubcompat/winhubcompat.so
. "$(dirname "$0")/common.sh"

cjson="$(cjson_dir)"
src="$ROOT/src/winhubcompat"
wine_src="$WORK/wine-$WINE_VERSION-cx$CROSSOVER_VERSION"
build="$WORK/wine-build"
out="$WORK/winhubcompat"
rm -rf "$out"
mkdir -p "$out"

flags="-arch x86_64 -mmacosx-version-min=10.15 -O2 -fPIC -fvisibility=hidden -DWINE_UNIX_LIB -D__WINESRC__"
# shellcheck disable=SC2086
x86 clang $flags -w -c -o "$out/cJSON.o" "$cjson/cJSON.c"
# shellcheck disable=SC2086
x86 clang $flags -std=gnu11 -Wall -Wno-unused-function -I"$src" -I"$cjson" -I"$build/include" -I"$wine_src/include" \
    -c -o "$out/compatdb.o" "$src/compatdb.c"
# shellcheck disable=SC2086
x86 clang $flags -std=gnu11 -Wall -Wno-unused-function -I"$src" -I"$cjson" -I"$build/include" -I"$wine_src/include" \
    -c -o "$out/winhubcompat.o" "$src/winhubcompat.c"
# ntdll.so is next to it at run time (@rpath/ntdll.so, found through @loader_path/).
# shellcheck disable=SC2086
x86 clang $flags -dynamiclib -install_name winhubcompat.so -Wl,-rpath,@loader_path/ \
    -o "$out/winhubcompat.so" "$out/winhubcompat.o" "$out/compatdb.o" "$out/cJSON.o" \
    "$build/dlls/ntdll/ntdll.so"
strip -x "$out/winhubcompat.so"

if nm -gU "$out/winhubcompat.so" | grep -q cJSON; then
    echo "winhubcompat.so exports cJSON symbols" >&2
    exit 1
fi
otool -L "$out/winhubcompat.so"
echo "winhubcompat ready in $out"
