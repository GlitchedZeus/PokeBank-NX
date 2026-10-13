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
    test_legality_l4_attestation_matrix
    test_legality_l4_gen1_3_event_sources
    test_legality_l4_gen12_gift_history
    test_legality_l4_gen3_distribution_matrix
    test_legality_l4_gen4_pcd_pgt_container
    test_legality_l4_gen4_mystery_gift_histories
    test_legality_l4_historical_event_path
    test_legality_l4_clock_receipt_history
    test_legality_l4_historical_adversarial
    test_legality_l4_stage5_coordinator_join
    test_legality_l4_stage5_lane_reconciliation
)
native=(
    test_legality_l4_native_pk4_projection
    test_legality_l4_native_attested_event
    test_legality_l4_ranger_gift_history
    test_legality_l4_event_date_history
    test_legality_l4_ball_history
    test_legality_l4_other_metadata
    test_legality_l4_trainer_identity
    test_legality_l4_gen4_mystery_gift_native
)
# L4 discovery/registration parity: no newly added test may silently miss
# normal or sanitizer execution. Duplicates are also exposed by the diff.
registered=(
    "${header_only[@]}" "${native[@]}"
    test_legality_l4_native_pk3_text
    test_legality_l4_gen3_distribution_native_pk3
    test_legality_l4_gen3_palpark_metadata_bridge
    test_legality_l4_stage5_native_pk3_join
)
mapfile -t discovered < <(
    find tests -maxdepth 1 -type f -name 'test_legality_l4_*.cpp' -printf '%f\n' |
        sed 's/\.cpp$//' | LC_ALL=C sort
)
if [[ "${#discovered[@]}" -ne 28 || "${#registered[@]}" -ne 28 ]] ||
    ! diff -u <(printf '%s\n' "${discovered[@]}") \
               <(printf '%s\n' "${registered[@]}" | LC_ALL=C sort); then
    echo "L4 registry mismatch: ${#discovered[@]} discovered / ${#registered[@]} registered; expected 28" >&2
    exit 3
fi
echo "L4 discovery/registration parity: 28/28 C++ suites"

# Independent L4-only source boundary contract. This does not replace any
# of the 28 registered complete-checkout C++ suites.
echo "L4 Ranger native storage field contract (196861 exhaustive cases)"
"$compiler" "${base[@]}" tests/fixtures/legality_l4_ranger_storage_ball.cpp \
    -o "$scratch/l4_ranger_storage_ball"
"$scratch/l4_ranger_storage_ball"
# Gen I native source-linked gift/trade provenance tests (not whole legality).
echo "L4 new Gen I native provenance fixtures"
for fixture in legality_l4_gen1_preflight legality_l4_gen1_npc_metadata legality_l4_gen1_individual_chain legality_l4_gen1_original_native; do
    "$compiler" "${base[@]}" "tests/fixtures/$fixture.cpp" -o "$scratch/$fixture"
    "$scratch/$fixture"
done
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
# Strict native encrypted PK3 boundary. Gen III Encryption3FRLG.cpp uses
# GCC-specific diagnostic pragmas; Clang needs only this local suppression.
pk3_flags=()
if [[ "$compiler" == *clang* ]]; then
    pk3_flags+=( -Wno-unknown-warning-option )
fi
echo "L4 C++ native PK3 compile: test_legality_l4_native_pk3_text"
"$compiler" "${base[@]}" "${pk3_flags[@]}" \
    tests/test_legality_l4_native_pk3_text.cpp \
    src/Encryption/Encryption3FRLG.cpp \
    -o "$scratch/test_legality_l4_native_pk3_text"
"$scratch/test_legality_l4_native_pk3_text"
echo "L4 C++ native PK3 compile: test_legality_l4_gen3_distribution_native_pk3"
"$compiler" "${base[@]}" "${pk3_flags[@]}" \
    tests/test_legality_l4_gen3_distribution_native_pk3.cpp \
    src/Encryption/Encryption3FRLG.cpp \
    -o "$scratch/test_legality_l4_gen3_distribution_native_pk3"
"$scratch/test_legality_l4_gen3_distribution_native_pk3"
echo "L4 C++ native PK3 compile: test_legality_l4_gen3_palpark_metadata_bridge"
"$compiler" "${base[@]}" "${pk3_flags[@]}" \
    tests/test_legality_l4_gen3_palpark_metadata_bridge.cpp \
    src/Encryption/Encryption3FRLG.cpp \
    -o "$scratch/test_legality_l4_gen3_palpark_metadata_bridge"
"$scratch/test_legality_l4_gen3_palpark_metadata_bridge"
echo "L4 C++ native PK3 compile: test_legality_l4_stage5_native_pk3_join"
"$compiler" "${base[@]}" "${pk3_flags[@]}" \
    tests/test_legality_l4_stage5_native_pk3_join.cpp \
    src/Encryption/Encryption3FRLG.cpp \
    -o "$scratch/test_legality_l4_stage5_native_pk3_join"
"$scratch/test_legality_l4_stage5_native_pk3_join"
echo "L4 Python frozen original-source projection integrity audit"
python3 tools/legality_l4_projection_audit.py
python3 -m unittest discover -v -s tests -p 'test_legality_l4_projection_audit.py'
python3 -m unittest discover -v -s tests -p 'test_legality_l4_l5_q07_join.py'
python3 -m unittest discover -v -s tests -p 'test_legality_l4_stage5_source_rights.py'
# L5's independent Q07 document is NOT on the L4 branch. When coordinator
# supplies its exact separately fetched copy, verify all 247 cross-lane
# identities and the source-projection Git blob digest without merging.
if [[ -n "${L4_L5_Q07_JSON:-}" ]]; then
    python3 tools/legality_l4_l5_q07_join.py --l5-q07 "$L4_L5_Q07_JSON"
fi
if [[ -n "${L4_L5_Q08_JSON:-}" ]]; then
    python3 tools/legality_l4_stage5_source_rights.py --l5-q08 "$L4_L5_Q08_JSON"
fi
echo "L4 all 28 source-compiled test programs: PASS (sanitizers=${L4_SANITIZE:-0})"
