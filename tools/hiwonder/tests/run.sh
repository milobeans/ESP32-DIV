#!/bin/sh
# Usage: tools/hiwonder/tests/run.sh
# Compiles the current board driver against host I2C/display fixtures; no device
# I/O or ESP32/radio simulation. Set CXX to choose a host C++17 compiler.
set -eu
check_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir="$check_dir/../../../ESP32-DIV"
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/div-driver-check.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -DBOARD_HIWONDER_ESP32_S3 \
  -I"$check_dir/include" -I"$source_dir" \
  "$check_dir/driver_check.cpp" "$source_dir/HiwonderBoard.cpp" \
  -o "$build_dir/driver_check"
for scenario in success expander-begin-failure expander-nack touch-begin-failure touch-unknown-id touch-short-id touch-transient-nack touch-transient-short-read touch-persistent-nack; do
  "$build_dir/driver_check" "$scenario"
done
