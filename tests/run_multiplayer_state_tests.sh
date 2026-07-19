#!/usr/bin/env sh
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_file="${TMPDIR:-/tmp}/ygopro-core-multiplayer-state-tests"

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
	-I"$root_dir" \
	"$root_dir/multiplayer.cpp" \
	"$root_dir/tests/multiplayer_state_tests.cpp" \
	-o "$output_file"

"$output_file"
