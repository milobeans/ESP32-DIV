#!/usr/bin/env bash
# Run existing host-side driver fixtures against a patched GhostESP checkout.
# Usage: ./run.sh /path/to/GhostESP
set -euo pipefail
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 /path/to/patched/GhostESP" >&2
    exit 2
fi
fixtures="$(cd "$(dirname "$0")" && pwd)"
ghost_source="$(cd "$1" && pwd)"
drivers="$ghost_source/components/lvgl_esp32_drivers"
for source_file in "$drivers/hiwonder_board.c" "$drivers/lvgl_touch/ft6x36.c"; do
    if [ ! -f "$source_file" ]; then
        echo "Missing patched GhostESP driver: $source_file" >&2
        exit 2
    fi
done
test_build="$(mktemp -d "${TMPDIR:-/tmp}/ghostesp-hiwonder-tests.XXXXXX")"
trap 'rm -rf "$test_build"' EXIT
compiler="${CC:-cc}"
"$compiler" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -DLV_LVGL_H_INCLUDE_SIMPLE=1 \
    -I"$fixtures" -I"$drivers" \
    "$fixtures/check.c" "$drivers/hiwonder_board.c" -o "$test_build/expander-check"
"$test_build/expander-check" success
"$test_build/expander-check" failure
"$compiler" -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -DLV_LVGL_H_INCLUDE_SIMPLE=1 \
    -I"$fixtures" -I"$drivers" \
    "$fixtures/touch-check.c" "$drivers/lvgl_touch/ft6x36.c" -o "$test_build/touch-check"
"$test_build/touch-check"
