#!/usr/bin/env bash
# Read-only candidate source test gate; does not claim original PKHeX parity.
set -euo pipefail
cd "$(dirname "$0")/.."
compiler="${CXX:-g++}"
mode="${GEN1_SANITIZE:-0}"
[[ "$mode" == 0 || "$mode" == 1 ]] || exit 2
flags=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude)
if [[ "$mode" == 1 ]]; then
  flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
  export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1}"
  export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:print_stacktrace=1}"
else
  flags+=(-O2)
fi
tmp="$(mktemp -d "${TMPDIR:-/tmp}/pbnx-gen1-rby-XXXXXXXX")"
trap 'rm -rf -- "$tmp"' EXIT
suites=(
  tests/test_legality_l1_gen1_pret_slots.cpp
  tests/test_legality_l1_gen1_pret_statics_trades.cpp
  tests/test_legality_l1_gen1_pret_prizes.cpp
  tests/test_legality_l1_gen1_pret_rod_init.cpp
  tests/test_legality_l1_gen1_pret_evolution.cpp
  tests/test_legality_l1_gen1_pret_gift_event.cpp
  tests/test_legality_l1_gen1_pret_wild_roll.cpp
  tests/test_legality_l1_gen1_pret_slot_weights.cpp
  tests/test_legality_l1_gen1_pret_fishing_roll.cpp
  tests/test_legality_l1_gen1_pret_trade_metadata.cpp
  tests/test_legality_l3_gen1_rby_original_joint.cpp
  tests/test_legality_l3_gen1_retail_catalog.cpp
  tests/test_legality_l3_gen1_all_species_joint.cpp
  tests/test_legality_l3_gen1_original_all_machines.cpp
  tests/test_legality_l3_gen1_all_species_machine_chronology.cpp
  tests/test_legality_gen1_editor_original_machines_subset.cpp
  tests/fixtures/legality_l4_gen1_preflight.cpp
  tests/fixtures/legality_l4_gen1_npc_metadata.cpp
  tests/fixtures/legality_l4_gen1_individual_chain.cpp
  tests/fixtures/legality_l4_gen1_original_native.cpp
  tests/fixtures/legality_l4_gen1_raw_record_witness.cpp
  tests/fixtures/legality_l5_gen1_rby_vector_integrity.cpp
)
for source in "${suites[@]}"; do
  [[ -s "$source" ]] || { echo "Missing source: $source" >&2; exit 2; }
  target="$tmp/$(basename "$source" .cpp)"
  "$compiler" "${flags[@]}" "$source" -o "$target"
  "$target"
done
echo "Gen I: ${#suites[@]} source-scoped tests PASS (not retail parity)"
