#!/usr/bin/env bash
# Standalone L4 validation from an EXACT checked-out PR application HEAD.
# Does not change any shared Makefile, test registry or source save.
# Usage: bash tests/test_legality_l4_runner.sh
#        L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
#        L4_EXPECTED_HEAD=<40-char git SHA> bash tests/test_legality_l4_runner.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"
if [[ -n "${L4_EXPECTED_HEAD:-}" ]]; then
    actual="$(git rev-parse HEAD)"
    if [[ "$actual" != "$L4_EXPECTED_HEAD" ]]; then
        echo "L4 exact-head mismatch: expected $L4_EXPECTED_HEAD, observed $actual" >&2
        exit 2
    fi
fi
compiler="${CXX:-g++}"
scratch="$(mktemp -d "${TMPDIR:-/tmp}/pokebank-l4-tests.XXXXXXXX")"
trap 'rm -rf "$scratch"' EXIT
base=( -std=c++20 -Wall -Wextra -Wpedantic -Werror
       -Wno-unused-parameter -Iinclude )
if [[ "${L4_SANITIZE:-0}" == "1" ]]; then
    base+=( -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer )
    export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
    export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1:halt_on_error=1}"
else
    base+=( -O2 )
fi

# Fully sourced, no mocks or code substitution. The read-only native
# parser uses the existing accepted encryption/personal-info link graph.
native_sources=(
    src/Pokemon/Pokemon4ReadOnly.cpp
    src/Pokemon/PersonalRecord4.cpp
    src/Pokemon/PersonalInfo4DP.cpp
    src/Pokemon/PersonalInfo4PT.cpp
    src/Pokemon/PersonalInfo4HGSS.cpp
    src/Encryption/Encryption4.cpp
)
header_only=(
    test_legality_l4_archive_projection
    test_legality_l4_unified_event_source
    test_legality_l4_contest_history
    test_legality_l4_event_text_history
    test_legality_l4_gen3_raw_text
)
native=(
    test_legality_l4_native_pk4_projection
    test_legality_l4_native_attested_event
    test_legality_l4_ranger_gift_history
    test_legality_l4_event_date_history
    test_legality_l4_ball_history
    test_legality_l4_other_metadata
)
for name in "${header_only[@]}"; do
    echo "L4 C++ compile: $name"
    "$compiler" "${base[@]}" "tests/$name.cpp" -o "$scratch/$name"
    "$scratch/$name"
done
for name in "${native[@]}"; do
    echo "L4 C++ native PK4 compile: $name"
    "$compiler" "${base[@]}" "tests/$name.cpp" "${native_sources[@]}" \
        -o "$scratch/$name"
    "$scratch/$name"
done
echo "L4 all 11 source-compiled test programs: PASS (sanitizers=${L4_SANITIZE:-0})"
