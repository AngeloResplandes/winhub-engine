#!/bin/sh
# Unit tests for the compatibility database code in winhubcompat (native build, no Wine needed).
. "$(dirname "$0")/common.sh"

cjson="$(cjson_dir)"
src="$ROOT/src/winhubcompat"
flags="-std=c99 -D_DARWIN_C_SOURCE -O1 -g -fsanitize=address,undefined"
# shellcheck disable=SC2086
cc $flags -w -c -o "$WORK/cJSON-test.o" "$cjson/cJSON.c"
# shellcheck disable=SC2086
cc $flags -Wall -Wextra -Werror -I"$src" -I"$cjson" -o "$WORK/compatdb-test" \
    "$ROOT/tests/compatdb-test.c" "$src/compatdb.c" "$WORK/cJSON-test.o"
"$WORK/compatdb-test"
