#!/usr/bin/env bash
# Standalone L3 integration gate. Runs all exact committed L3 native suites
# against the full accepted Emerald data. Does not modify saves or source.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
CXX="${CXX:-g++}"
DATA=include/Legality/Gen3EmeraldLevelUpData.inc
HEADER=include/Legality/Gen3EmeraldLevelUpEvidence.h
if [[ ! -f "$DATA" || ! -f "$HEADER" ]]; then
    echo "L3 BLOCKED: full accepted Emerald learnset source is missing" >&2
    exit 2
fi
if ! grep -q 'std::array<uint16_t,3947> kEmeraldPackedLevelMoves' "$DATA"; then
    echo "L3 BLOCKED: expected 3947-entry original Emerald table not present" >&2
    exit 2
fi
if [[ -n "${L3_EXPECTED_EMERALD_BLOB:-}" ]]; then
    actual="$(git hash-object "$DATA")"
    if [[ "$actual" != "$L3_EXPECTED_EMERALD_BLOB" ]]; then
        echo "L3 BLOCKED: Emerald table blob changed: $actual" >&2
        exit 2
    fi
fi
printf 'L3 original Emerald table blob: %s\n' "$(git hash-object "$DATA")"
printf 'L3 source HEAD: %s\n' "$(git rev-parse HEAD 2>/dev/null || printf 'unavailable')"
BUILD="$(mktemp -d "${TMPDIR:-/tmp}/pokebank-l3-XXXXXXXX")"
trap 'rm -rf "$BUILD"' EXIT
SUITES=(
    test_legality_l3_gen12_joint_chronology
    test_legality_l3_gba_original_full_tables
    test_legality_l3_gba_original_machine_tables
    test_legality_l3_gba_full_machine_classification
    test_legality_l3_move_chronology
    test_legality_l3_gba_versioned_levelup
    test_legality_l3_gba_starter_evolution
    test_legality_l3_shedinja_copy
    test_legality_l3_machine_methods
    test_legality_l3_gba_relearner
    test_legality_l3_palpark_joint_stage
)
FLAGS=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude)
for suite in "${SUITES[@]}"; do
    source="tests/${suite}.cpp"
    [[ -f "$source" ]] || { echo "L3 BLOCKED: missing $source" >&2; exit 2; }
    echo "L3 native: $suite"
    "$CXX" "${FLAGS[@]}" -O2 "$source" -o "$BUILD/$suite"
    "$BUILD/$suite"
    echo "L3 ASan/UBSan: $suite"
    "$CXX" "${FLAGS[@]}" -O1 -g -fsanitize=address,undefined \
        -fno-omit-frame-pointer "$source" -o "$BUILD/${suite}_sanitize"
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:abort_on_error=1}" \
        UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1}" \
        "$BUILD/${suite}_sanitize"
done
echo 'L3 PASS: all eleven native C++20 suites and eleven ASan/UBSan executions (full accepted Emerald source)'
