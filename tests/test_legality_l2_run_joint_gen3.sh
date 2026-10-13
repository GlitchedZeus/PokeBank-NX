#!/usr/bin/env bash
# L2-owned standalone SAME-SOURCE host and sanitizer runner. Does not
# modify the coordinator-owned Makefile or workflows. Run from any cwd.
# Requires a complete checkout at the exact PR head (including frozen
# accepted Gen34EggMoveData.inc), GCC or Clang supporting C++20.
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cxx="${CXX:-g++}"
if [[ ! -f "$repo_root/include/Legality/Gen34EggMoveData.inc" ]]; then
    echo 'Missing accepted Gen34EggMoveData.inc; need complete checkout' >&2
    exit 2
fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/pokebank-l2-joint-XXXXXX")"
trap 'rm -rf -- "$tmp"' EXIT
for test in \
    test_legality_l2_gen3_joint_pid_iv_moves \
    test_legality_l2_gen3_pid_iv_moves_alternatives
do
    src="$repo_root/tests/$test.cpp"
    for mode in normal sanitizers; do
        flags=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -I"$repo_root/include")
        if [[ "$mode" == sanitizers ]]; then
            flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
        else
            flags+=(-O2)
        fi
        echo "BUILD $cxx $test ($mode)"
        "$cxx" "${flags[@]}" "$src" -o "$tmp/$test-$mode"
        if [[ "$mode" == sanitizers ]]; then
            ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 \
            UBSAN_OPTIONS=halt_on_error=1 \
            "$tmp/$test-$mode"
        else
            "$tmp/$test-$mode"
        fi
    done
done
echo "L2 Gen III PID/IV/moves candidate and alternative tests: ALL PASS"
