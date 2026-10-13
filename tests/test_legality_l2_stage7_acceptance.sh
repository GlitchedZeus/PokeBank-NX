#!/usr/bin/env bash
# L2 final gate: exact immutable checkout, full existing 20-suite matrix,
# every separately attested 7-stage fixture, source-linked integration and
# GCC/Clang normal + ASan/UBSan. NO synthetic checkout accepted.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
fail() { printf 'L2 STAGE7 ACCEPTANCE BLOCKED: %s\n' "$*" >&2; exit 2; }
command -v git >/dev/null || fail 'git unavailable'
git -C "$root" rev-parse --show-toplevel >/dev/null 2>&1 || fail 'full git checkout unavailable'
head_sha="$(git -C "$root" rev-parse HEAD)"
[[ "$head_sha" =~ ^[0-9a-f]{40}$ ]] || fail 'cannot establish 40-character source head'
[[ -z "${GITHUB_SHA:-}" || "$GITHUB_SHA" == "$head_sha" ]] ||
    fail "workflow source mismatch: $head_sha != $GITHUB_SHA"
[[ -z "$(git -C "$root" status --porcelain --untracked-files=no)" ]] ||
    fail 'tracked source is dirty; exact committed source is required'
require=(
  src/Pokemon/Pokemon4ReadOnly.cpp
  src/Encryption/Encryption4.cpp
  include/Legality/Gen34EggMoveData.inc
  tests/test_legality_l2_run_all.sh
  tests/test_legality_l2_gen3_egg_pid_creation.cpp
  tests/test_legality_l2_gen4_pt_hgss_full_moves.cpp
  tests/test_legality_l2_dp_native_egg_history.cpp
)
for f in "${require[@]}"; do
    [[ -f "$root/$f" ]] || fail "missing full-checkout dependency: $f"
done
extra=(
  tests/fixtures/legality_l2_original_daycare_roles.cpp
  tests/fixtures/legality_l2_original_personal_snapshot_guard.cpp
  tests/fixtures/legality_l2_breeding_personal_join.cpp
  tests/fixtures/legality_l2_gen3_original_acquisition_properties.cpp
  tests/fixtures/legality_l2_dp_original_parent_witness.cpp
  tests/fixtures/legality_l2_pt_hgss_original_daycare_source.cpp
  tests/fixtures/legality_l2_same_original_route_matrix.cpp
  tests/fixtures/legality_l2_joint_route_alternatives.cpp
  tests/fixtures/legality_l2_original_parent_lineage.cpp
  tests/fixtures/legality_l2_lineage_alternative_routes.cpp
  tests/fixtures/legality_l2_stage7_malformed_routes.cpp
  tests/fixtures/legality_l2_stage7_lineage_aliases.cpp
  tests/test_legality_l2_gen4_pt_hgss_source_bound.cpp
  tests/test_legality_l2_stage5_gen3_joint_routes.cpp
  tests/test_legality_l2_stage5_dp_joint_routes.cpp
  tests/test_legality_l2_stage5_pt_hgss_joint_routes.cpp
  tests/test_legality_l2_stage6_gen3_traded_parents.cpp
  tests/test_legality_l2_stage6_dp_palpark_parents.cpp
  tests/test_legality_l2_stage6_pt_hgss_lineage.cpp
)
for f in "${extra[@]}"; do
    [[ -f "$root/$f" ]] || fail "missing L2 regression file: $f"
done
# Don't let NDEBUG disable C++ asserts: every fixture uses assert().
[[ "${CXXFLAGS:-}" != *-DNDEBUG* ]] || fail 'NDEBUG would disable regression checks'
for compiler in g++ clang++; do
    command -v "$compiler" >/dev/null || fail "compiler missing: $compiler"
done
tmp="$(mktemp -d "${TMPDIR:-/tmp}/pbnx-l2-acceptance-XXXXXX")"
trap 'rm -rf -- "$tmp"' EXIT
executions=0
for compiler in g++ clang++; do
  for mode in normal sanitizers; do
    echo "L2 ACCEPTANCE: head=$head_sha compiler=$compiler mode=$mode"
    # Disable the normal developer subset filter: final acceptance requires
    # ALL twenty production-native programs in every compiler/mode pair.
    CXX="$compiler" L2_MODES="$mode" L2_TEST_FILTER='' \
        bash "$root/tests/test_legality_l2_run_all.sh"
    flags=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter
           -I"$root/include")
    if [[ "$mode" == sanitizers ]]; then
        flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    else
        flags+=(-O2)
    fi
    for source in "${extra[@]}"; do
        name="$(basename "$source" .cpp)"
        target="$tmp/$name-$compiler-$mode"
        echo "L2 COMPILE: $source [$compiler,$mode]"
        "$compiler" "${flags[@]}" "$root/$source" -o "$target"
        echo "L2 RUN: $source [$compiler,$mode]"
        ASAN_OPTIONS=halt_on_error=1:detect_leaks=1 \
          UBSAN_OPTIONS=halt_on_error=1 "$target"
        executions=$((executions+1))
    done
  done
done
[[ "$executions" -eq 76 ]] || fail "only $executions of 76 extra source programs executed"
echo "L2 STAGE7 ACCEPTANCE PASSED: exact head=$head_sha; 80 native + 76 extra = 156 GCC/Clang normal and ASan/UBSan program executions"
