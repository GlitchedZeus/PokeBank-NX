# L1 Stage 1 — source-pinned bounded Gen I wild/rod source candidates (2026-10-12)

This tranche extends *existing* E02/E03 indexed Gen I R/B/Y static/gift/trade original-stage coverage with separately verified **E01 wild/fishing evidence**. It is **not** a complete R/B/Y encounter solver, does not add global Invalid, and does not modify production `Legality.cpp`.

## Exact independent original-code references

- `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`:
  - `data/wild/maps/Route1.asm`: Red/Blue Route 1 ten ordered grass slots.
  - `data/wild/maps/ViridianForest.asm`: Red and Blue each have eight version-conditional grass slots and two common Pikachu slots.
  - `data/wild/super_rod.asm`: **all 33** map→group assignments and ten original rod group definitions (including the nonuniform 5/15/23 fishing levels).
- `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`:
  - `data/wild/maps/Route1.asm`: Yellow's separately ordered ten grass slots.
- These are fixed pinned decompilation code references, **not** authenticated retail catch/save histories. Gen I Super Rod in Yellow is deliberately unmodeled until separately reviewed; R/B rod tables are never claimed as Yellow source rows.

## Source data and intentionally limited semantics

`include/Legality/Gen1OriginalWildFishingStage.h` carries **149 independent physical source slots**: Route 1 R/B (10 shared Red+Blue rows), Route 1 Yellow (10), Viridian Forest Red (10), Viridian Forest Blue (10), and R/B Super Rod on all 33 maps (109 rows). Physical duplicate entries are preserved, not species-level deduplicated. These yield **268 original game-qualified indexed candidate combinations**.

`evaluate()` checks original game, independently attested original slot, original map, method, species and level in order. Invalid/unproven original witnesses become `Unknown*`; cross-version rows are `ForeignGameRowNotEvidence` (not global Invalid). Mismatches of an independently selected row are **candidate-local only**, not a verdict on other maps, slots, breeding, gifts, trades, time capsule transfers or later evolutions. A positive returns `SourceSlotCompatibleRateAndHistoryUnverified`, which intentionally does **not** certify an original PK1 catch-rate, Time Capsule mutation, evolution chain, individual fishing/RNG transcript, catch ball or saved specimen.

No code is copied from upstream PKHeX; these short records are transcribed from the pinned original decompilation lists, with explicit limited source scope.

## Executed native tests, exact contents

`tests/test_legality_l1_gen1_wild_fishing.cpp` is a standalone C++20 source-linked matrix over **149** slots × three original games. The **exact same compiled file bytes** were confirmed by `git hash-object` versus the final GitHub blob hashes:
- header `e4a2ee9f2a13074c62b758c50ef385f766104108`
- test `1e458c346b730fb54c4bb9f35ccf3f0ddcc29ae1`

Executed locally with GCC `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror` and separately with `-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`, sanitizer halt-on-error. Both executions completed:
- **268 source-game positive slot comparisons**
- **1,072 selected-row local contradiction assertions**
- **1,787 missing-witness or foreign-title controls**
- Plus malformed method and out-of-range source-index controls.

This test has been registered as suite **#14** in `tests/test_legality_l1_run.sh`; registering it is NOT an assertion that the other 13 suites, whole-repo host/RSE or integrated #298 exact combined head have been executed here. Full checkout is blocked in this isolated compiler environment by GitHub DNS. The coordinator must rerun the entire exact final 14-suite normal+ASan/UBSan runner and host/RSE before acceptance.

## Pending Stage 1 scope and source blockers

Gen I Route 1 and R/B Viridian Forest are **examples**, not the wild-table totality. Remaining R/B/Y grass/surf, Yellow fishing, Old/Good Rod, Japanese Blue/Stadium, later transfer paths, original encounter time/area and catch-rate provenance must be independently verified before generalizing. Existing E02/E03 static, gifts and in-game trade rows remain bounded PKHeX-derived source-stage compatibility, not full retail provenance. Candidate-source negation must never escalate into global Invalid.

No modification to accepted legality, PR #298, production analyzer, shared workflow/Makefile, other lanes or MAIN. No reset/rebase, merges or force pushes.

## E02/E03 source-field provenance hardening in the same stage

Existing `Gen1IndexedOriginalSourceHistory.h` now distinguishes a known static/trade **row** from independently known **original species** and **original acquisition level**. Both fields have explicit witness flags. Missing, zero, out-of-domain species/level becomes `UnknownOriginalSpecies`/`UnknownOriginalLevel`, not a fabricated local-negative; conflicting positive Time Capsule/no-Time-Capsule flags also downgrade static origins to Unknown, not just trades. A positively attested, valid but different original species/level remains a selected-row contradiction.

The existing `test_legality_l1_gen1_original_source.cpp` updates all six fixture builders, exercises missing and malformed species/level across the static row matrix, and missing fields on trade rows. **These modified existing C++ tests have not been natively executed here**: unlike the new standalone wild/rod suite, they require additional repository headers and generated data inaccessible to this compiler's checkout. The previous passing 13-suite integration workflow was executed at **a different SHA**, so it must not be attributed to the current PR head. Exact-head L1 runner and whole-host/sanitizer execution are still acceptance blockers.
