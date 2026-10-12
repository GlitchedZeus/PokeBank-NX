#!/usr/bin/env bash
# L1 isolated exact-head native regression runner: zero shared Makefile edits.
# Coordinator may execute in a checked-out application SHA. This does not
# replace the full host and focused RSE CI gates.
set -euo pipefail
cd "$(dirname "$0")/.."
application_sha="$(git rev-parse HEAD)"
application_tree="$(git rev-parse 'HEAD^{tree}')"
if [[ -n "${L1_EXPECT_SHA:-}" && "$application_sha" != "$L1_EXPECT_SHA" ]]; then
  printf 'L1: REFUSING wrong HEAD: expected %s but found %s\n' "$L1_EXPECT_SHA" "$application_sha" >&2
  exit 2
fi
if [[ -n "${L1_EXPECT_TREE:-}" && "$application_tree" != "$L1_EXPECT_TREE" ]]; then
  printf 'L1: REFUSING wrong tree: expected %s but found %s\n' "$L1_EXPECT_TREE" "$application_tree" >&2
  exit 2
fi
printf 'L1: APPLICATION_SHA=%s TREE_SHA=%s\n' "$application_sha" "$application_tree"
compiler="${CXX:-g++}"
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
common=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Iinclude)
# Native HGSS special-stage test exercises *real encrypted PK4 storage*
# through the production checksum + read-only Pokemon4 parser.
native=(
  src/Pokemon/Pokemon4ReadOnly.cpp
  src/Pokemon/PersonalInfo4DP.cpp
  src/Pokemon/PersonalInfo4PT.cpp
  src/Pokemon/PersonalInfo4HGSS.cpp
  src/Pokemon/PersonalRecord4.cpp
  src/Encryption/Encryption4.cpp
)
suites=(
  test_legality_l1_emerald_methodh
  test_legality_l1_gen3_family_router
  test_legality_l1_hgss_special
  test_legality_l1_gen4_nature_replay
  test_legality_l1_safari_indexed_original
  test_legality_l1_gamecube_indexed_shadow
  test_legality_l1_gen1_original_source
  test_legality_l1_gen1_wild_fishing
  test_legality_l1_gen2_original_source
  test_legality_l1_gen2_fishing
  test_legality_l1_gen2_tree_contest
  test_legality_l1_gen2_swarms
  test_legality_l1_gen2_roamer_routes
  test_legality_l1_gen4_static_indexed
  test_legality_l1_gen4_sinnoh_special
  test_legality_l1_original_candidate_universe
  test_legality_l1_gen3_original_roamers
  test_legality_l1_gamecube_original_fixed
)
for profile in host sanitize; do
  if [[ "$profile" == host ]]; then
    flags=(-O2)
  else
    flags=(-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  fi
  for suite in "${suites[@]}"; do
    sources=("tests/$suite.cpp")
    if [[ "$suite" == test_legality_l1_hgss_special ]]; then
      sources+=("${native[@]}")
    fi
    target="$build_dir/$suite-$profile"
    echo "L1: compile $suite [$profile]"
    "$compiler" "${common[@]}" "${flags[@]}" "${sources[@]}" -o "$target"
    echo "L1: run $suite [$profile]"
    if [[ "$profile" == sanitize ]]; then
      ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$target"
    else
      "$target"
    fi
  done
done
echo "L1: all ${#suites[@]} native host and ASan/UBSan suites PASS at $application_sha"
