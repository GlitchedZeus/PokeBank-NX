#!/usr/bin/env bash
# L2 source-owned standalone suite runner. No coordinator Makefile/workflow edits.
# Run from a COMPLETE exact-head checkout:
#   bash tests/test_legality_l2_run_all.sh
#   CXX=clang++ bash tests/test_legality_l2_run_all.sh
#   L2_TEST_FILTER=gen3_joint_pid_iv_moves bash tests/test_legality_l2_run_all.sh
#   L2_MODES=normal bash tests/test_legality_l2_run_all.sh
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cxx="${CXX:-g++}"
modes="${L2_MODES:-normal sanitizers}"
filter="${L2_TEST_FILTER:-}"
if [[ ! -f "$root/include/Legality/Gen34EggMoveData.inc" ||
      ! -f "$root/src/Pokemon/Pokemon4ReadOnly.cpp" ||
      ! -f "$root/src/Encryption/Encryption4.cpp" ]]; then
    echo "L2: full accepted source checkout required (not a partial copy)" >&2
    exit 2
fi
if [[ ! -f "$root/tests/test_legality_l2_gen3_egg_pid_creation.cpp" ]]; then
    echo "L2: run against branch containing the nine earlier L2 suites" >&2
    exit 2
fi
for mode in $modes; do
    if [[ "$mode" != normal && "$mode" != sanitizers ]]; then
        echo "L2: L2_MODES allows only normal and sanitizers" >&2
        exit 2
    fi
done
if command -v git >/dev/null 2>&1 && git -C "$root" rev-parse HEAD >/dev/null 2>&1; then
    echo "L2 exact checkout head: $(git -C "$root" rev-parse HEAD)"
    if [[ -n "$(git -C "$root" status --porcelain --untracked-files=no)" ]]; then
        echo "L2 WARNING: checkout has tracked modifications; not a clean exact-head run" >&2
    fi
fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/pbnx-l2-all-XXXXXX")"
trap 'rm -rf -- "$tmp"' EXIT

# The accepted native PK4 test needs real Pokemon4ReadOnly and Encryption4,
# plus the SAME production link dependencies as Makefile.host.gen4's
# GEN4_MUTABLE_SOURCES target. There is no mock/encrypted-PK4 substitution.
pk4_sources=(
    src/Pokemon/Pokemon4Mutable.cpp
    src/Pokemon/Pokemon4ReadOnly.cpp
    src/Pokemon/PersonalRecord4.cpp
    src/Pokemon/PersonalInfo4DP.cpp
    src/Pokemon/PersonalInfo4PT.cpp
    src/Pokemon/PersonalInfo4HGSS.cpp
    src/Encryption/Encryption4.cpp
    src/Pokemon/Experience.cpp
    src/Names/SpeciesNames.cpp
    src/Names/ItemPresence.cpp
    src/Names/MoveInfo.cpp
    src/Names/MovePresence.cpp
    src/Utils/StringHelpers.cpp
    src/Utils/HelperUtilities.cpp
)
tests=(
    test_legality_l2_gen1_native_dv_stat
    test_legality_l2_gen1_native_record_adversarial
    test_legality_l2_gen1_native_exp_growth
    test_legality_l2_gen1_original_stat_exp_battles
    test_legality_l2_gen1_stat_exp_math_boundaries
    test_legality_l2_dp_alternative_egg_histories
    test_legality_l2_dp_move_stage
    test_legality_l2_dp_egg_iv
    test_legality_l2_dp_egg_pid
    test_legality_l2_dp_native_egg_history
    test_legality_l2_gen3_egg_pid_creation
    test_legality_l2_gen3_iv_attested_alternatives
    test_legality_l2_gen3_joint_egg_pid_iv
    test_legality_l2_gen3_joint_pid_iv_moves
    test_legality_l2_gen3_pid_iv_moves_alternatives
    test_legality_l2_gen3_retail_daycare_pair
    test_legality_l2_gen4_pt_hgss_pid
    test_legality_l2_gen4_pt_hgss_joint
    test_legality_l2_gen4_pt_hgss_move_stage
    test_legality_l2_gen4_pt_hgss_full_moves
    test_legality_l2_gen4_iv_candidate_set
    test_legality_l2_gen4_native_iv_history
)
passed=0
attempted=0
failures=()
for test in "${tests[@]}"; do
    if [[ -n "$filter" && "$test" != *"$filter"* ]]; then
        continue
    fi
    source_file="$root/tests/$test.cpp"
    if [[ ! -f "$source_file" ]]; then
        echo "L2 ERROR: expected test missing: $source_file" >&2
        exit 2
    fi
    # Files API transport failures must never be mistaken for committed
    # C++ test sources. Fail before compiler execution and surface the
    # corrupted branch artifact instead of reporting a sanitizer failure.
    if ! head -n 2 "$source_file" | grep -Eq '^[[:space:]]*#include[[:space:]]'; then
        echo "L2 ERROR: test does not contain valid C++ source prologue: $source_file" >&2
        exit 2
    fi
    sources=("$source_file")
    if [[ "$test" == test_legality_l2_gen1_native_dv_stat ||
          "$test" == test_legality_l2_gen1_native_record_adversarial ||
          "$test" == test_legality_l2_gen1_native_exp_growth ]]; then
        sources+=(
            "$root/src/Legality/Gen1NativeDVStatEvidence.cpp"
            "$root/src/Legality/Gen1NativeExperienceEvidence.cpp"
            "$root/src/Pokemon/Experience.cpp"
            "$root/src/Integration/Gen1/Gen1BattleStats.cpp"
            "$root/src/Integration/Gen1/Gen1StagedPokemonEditor.cpp"
            "$root/src/Integration/Gen1/Gen1ReadOnlySave.cpp"
        )
    fi
    if [[ "$test" == test_legality_l2_gen1_stat_exp_math_boundaries ]]; then
        sources+=(
            "$root/src/Integration/Gen1/Gen1BattleStats.cpp"
            "$root/src/Integration/Gen1/Gen1StagedPokemonEditor.cpp"
            "$root/src/Integration/Gen1/Gen1ReadOnlySave.cpp"
        )
    fi
    if [[ "$test" == test_legality_l2_gen1_original_stat_exp_battles ]]; then
        sources+=(
            "$root/src/Legality/Gen1OriginalStatExpBattleHistory.cpp"
            "$root/src/Integration/Gen1/Gen1ReadOnlySave.cpp"
        )
    fi
    if [[ "$test" == test_legality_l2_dp_native_egg_history ||
          "$test" == test_legality_l2_gen4_native_iv_history ]]; then
        for src in "${pk4_sources[@]}"; do
            if [[ ! -f "$root/$src" ]]; then
                echo "L2 ERROR: production native PK4 dependency missing: $src" >&2
                exit 2
            fi
            sources+=("$root/$src")
        done
    fi
    for mode in $modes; do
        attempted=$((attempted+1))
        flags=(-std=c++20 -Wall -Wextra -Wpedantic -Werror
               -Wno-unused-parameter -I"$root/include")
        if [[ "$test" == test_legality_l2_gen1_native_dv_stat ||
              "$test" == test_legality_l2_gen1_native_record_adversarial ||
              "$test" == test_legality_l2_gen1_native_exp_growth ||
              "$test" == test_legality_l2_gen1_stat_exp_math_boundaries ]]; then
            flags+=(-ffunction-sections -fdata-sections -Wl,--gc-sections)
        fi
        if [[ "$mode" == sanitizers ]]; then
            flags+=(-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
        else
            flags+=(-O2)
        fi
        dest="$tmp/$test-$mode"
        echo "L2 compile: $test [$mode] using $cxx"
        if ! "$cxx" "${flags[@]}" "${sources[@]}" -o "$dest"; then
            echo "L2 FAIL compile: $test [$mode]" >&2
            failures+=("$test:$mode:compile")
            continue
        fi
        echo "L2 run: $test [$mode]"
        if [[ "$mode" == sanitizers ]]; then
            if ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 \
               UBSAN_OPTIONS=halt_on_error=1 "$dest"; then
                passed=$((passed+1))
                echo "L2 PASS: $test [$mode]"
            else
                echo "L2 FAIL runtime: $test [$mode]" >&2
                failures+=("$test:$mode:runtime")
            fi
        elif "$dest"; then
            passed=$((passed+1))
            echo "L2 PASS: $test [$mode]"
        else
            echo "L2 FAIL runtime: $test [$mode]" >&2
            failures+=("$test:$mode:runtime")
        fi
    done
done
if (( attempted==0 )); then
    echo "L2 ERROR: no test matched filter: $filter" >&2
    exit 2
fi
echo "L2 complete: $passed/$attempted exact-checked-out-source standalone test executions passed."
if (( ${#failures[@]}>0 )); then
    printf 'L2 FAILED EXECUTIONS: %s\n' "${failures[@]}" >&2
    exit 1
fi
