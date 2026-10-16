# Gen I-V legality: preparation for app integration

Status: preparation only. Nothing here has been merged. PR #314 must not be merged into #307 or
main without coordination with Main Development, full combined testing and broz's approval.

## Heads used

| Branch | PR | Head |
|---|---|---|
| `claude/gen1-5-legality-campaign-w9mkom` (legality engine) | #314 | `d4dff8a681d13dde60f46907ea11ab1a6287ccf8` |
| `claude/gen5-integration-20261014` (Gen I-V app, Main Development) | #307 | `c9e243343476a7abdbe43e69a1a1b1cf16b47734` |

Re-run the trial merge on the final heads before any real integration; GitHub is authoritative.

## Trial merge (local, not pushed)

`git merge --no-commit --no-ff` of the #307 head into the #314 head stops on five files. Every
other file merges cleanly.

| File | Owner | What collides | Proposed resolution |
|---|---|---|---|
| `.gitignore` | shared | #314 ignores `.claude/worktrees/`; #307 ignores the return-host build outputs | Keep both blocks. |
| `include/Pokemon/Pokemon3FRLG.h` | shared | Both quarantine a PK3 span of the wrong length. #314 keeps `inputSize_` and `sizeValid()`; #307 adds `inputShapeValid` | Keep #314's `sizeValid()`/`inputSize()`: the L6 Stage 2 native-format gate (`tests/fixtures/legality_stage2_native_format_properties.cpp`) asserts on them. Behaviour is the same (zeroed party-sized buffer, `checksumValid()` false). |
| `src/UI/Gen4SharedPokemonSurface.inc` | Main Development | adjacent `#include` lines | Keep both includes (`Legality/Legality.h` and `Integration/Gen4/Gen4BagCatalog.h`). |
| `src/UI/Modals/PokemonDetailsModal.cpp` | Main Development | #307 turns the legality overlay into a touch-scroll surface with a Close button; #314 adds the verdict line and the coverage rows | Keep #307's scroll surface, but build its lines from `legalityRep.verdict()` (see below). |
| `src/UI/TrainerViewScreenBase.inc` | Main Development | R / touch 95 opens the overlay. #314 passes the exact source game and opens it for every verdict except No problems found; #307 resets `legalityScroll` and calls `analyze` without the source game | Keep #314's call (exact source game, verdict test) and add #307's `details.legalityScroll = 0;`. |

### The overlay must not say "No problems found" for an incomplete report

#307's overlay (line 528 at its head, line 544 in the trial merge, which Git merged without a
conflict) shows "No problems found." when `legalityRep.ok()`. `ok()` only means no Invalid and no
Warning was raised (`problemCount() == 0`); it is true for an Incomplete report too. Merged as is, every Incomplete record
(for example a Gen III/IV-origin Pokemon in a Gen V save) would read "No problems found.", which is
the false completion the campaign forbids. The lines should be:

- `Verdict::NoProblemsFound`: "No problems found by complete checks."
- `problemCount() == 0` but `Verdict::Incomplete`: "No problems found by the checks that ran.
  Coverage is incomplete.", then one row per coverage field (source game, internal, moves,
  encounter, event/gift, PID/RNG, egg/breeding, transfer).
- Otherwise the Invalid and Warning issues, as both branches already list them.

The footer hint (`R: Legality`) and the summary pill already use `verdict()` after the merge.

## Gen V in the app

`Legality::analyzePk5(record, Pk5Input{encrypted})` is engine API on #314; this branch has no PK5
reader. On #307 the Gen V Trainer surface (`src/UI/Gen5SharedReadOnlySurface.inc`, "A narrow passive
PK5 panel ... NEVER calls Legality::analyze") shows no legality at all, and its record type is
`Pokemon5ReadOnly`. Wiring Gen V legality into the app needs one call from that panel (or from
`Legality::analyze` for a PK5 game group) with the record's 136/220 bytes. That is a Main
Development UI change and is left to them; the engine needs nothing more for it.

## Combined test plan (on the merged tree, before any merge is proposed)

1. `make -f Makefile.host host-test` and `host-sanitize`, GCC and Clang.
2. Every legality lane gate in `docs/legality/LEGALITY_GATE_MATRIX.md` (L1-L6 workflows and the
   invalid-site inventory), on the exact merged commit.
3. PKHeX differentials: `tools/pkhex_gb_diff` (Gen I and II), `tools/pkhex_gen34_diff` (Gen III
   and IV, mutation and grown corpora), `tools/pkhex_gen5_diff`; each must report 0 false Invalid
   and 0 false Valid.
4. The 20-title matrix (`tests/test_legality_title_matrix.cpp`).
5. #307's own Gen V suites (`test_gen5_source_e2e`, `test_gen5_export_validation`) and
   `gen15-integrated-candidate.yml` (one Gen I-V NRO, packaged RomFS verified).
6. A combined Gen I-V app and legality test campaign on a Switch, as broz requires before any
   Gen VI-IX work.
