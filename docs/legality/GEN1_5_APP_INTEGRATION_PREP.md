# Gen I-V legality: preparation for app integration

Status: preparation only. Nothing here has been merged. PR #314 must not be merged into #307 or
main without coordination with Main Development, full combined testing and broz's approval.

## Heads used

| Branch | PR | Head |
|---|---|---|
| `lane-c-gen5` (Gen V lane of the legality campaign; branched from `claude/gen1-5-legality-campaign-w9mkom` at `cfe0e7dcebfb195c15f16510595377da11543545`) | #314 | `3ee9b348dd7639bbd9f2eefd2b4d26e1a4ee336f` |
| `claude/gen5-integration-20261014` (Gen I-V app, Main Development) | #307 | `d104310da30d8095931ca16e097a2f6e2652ddfe` (fetched 2026-10-18; committed 2026-10-17T17:14:56Z) |

The earlier table in this file was measured on `d4dff8a681d13dde60f46907ea11ab1a6287ccf8` (#314)
against `c9e243343476a7abdbe43e69a1a1b1cf16b47734` (#307) and listed five files. Both branches
moved since; the live table below replaces it. Re-run the trial merge on the final heads before
any real integration; GitHub is authoritative.

## Trial merge (local, not pushed)

```
git fetch origin claude/gen5-integration-20261014
git merge-tree --write-tree HEAD origin/claude/gen5-integration-20261014
```

Exit status 1; merged tree `00c5463d96b749a195744774b340761f23033fc2`. Ten files conflict (every
other file, including `Makefile.host`, `Makefile.host.base`, `Makefile.host.gen4`,
`Makefile.host.gsc`, `.github/workflows/host-tests.yml`, `include/Pokemon/Pokemon.h`,
`src/UI/Modals/Gen1PokemonDetailsModal.cpp`, `tests/test_gen3_hardware_surface_contract.cpp` and
`tests/test_gen4_readonly_foundation.cpp`, auto-merges). No file of the Gen V legality lane
(`src/Legality/Gen5*`, `include/Legality/Gen5*`, `tests/test_legality_gen5*`,
`tools/pkhex_gen5_diff/**`) conflicts: #307 does not touch them.

| # | File | Owner | Hunks | What collides | Proposed resolution |
|---|---|---|---|---|---|
| 1 | `.gitignore` | shared | 1 | #314 ignores `.claude/worktrees/`; #307 ignores the return-host build outputs | Keep both blocks. |
| 2 | `include/Pokemon/Pokemon3FRLG.h` | shared | 2 | Both quarantine a PK3 span of the wrong length. #314 keeps `inputSize_` and `sizeValid()`; #307 adds `inputShapeValid` | Keep #314's `sizeValid()`/`inputSize()`: the L6 Stage 2 native-format gate (`tests/fixtures/legality_stage2_native_format_properties.cpp`) asserts on them. Behaviour is the same (zeroed party-sized buffer, `checksumValid()` false). |
| 3 | `src/UI/Gen1PokemonDetailsPresentation.cpp` | Main Development | 1 | #314 adds the legality and compatibility rows to the presentation; #307 splits the pane at `SharedMovesPanel::kDataPaneOffset` | Keep #314's rows and adopt #307's offset constant for their placement. |
| 4 | `src/UI/Gen3SharedPokemonSurface.inc` | Main Development | 1 | #314 draws the verdict line and the coverage rows; #307 draws a guardrail text | Keep #314's verdict/coverage panel (it is what the owner asked for on every Gen I-V surface). |
| 5 | `src/UI/Gen4SharedPokemonSurface.inc` | Main Development | 3 | (a) adjacent `#include` lines; (b) the panel caption; (c) #314's verdict/coverage rows vs #307's `legalityLabel(Incomplete)` | (a) keep both includes (`Legality/Legality.h` and `Integration/Gen4/Gen4BagCatalog.h`); (b) keep #314's caption; (c) keep #314's rows, which already render Incomplete as "checks incomplete" plus the coverage fields. |
| 6 | `src/UI/Modals/Gen2PokemonDetailsModal.cpp` | Main Development | 1 | #314's verdict row vs #307's "Not checked" row | Keep the verdict row: Gen II records are checked (`GB::checkGen2`), so "Not checked" would be wrong. |
| 7 | `src/UI/Modals/PokemonDetailsModal.cpp` | Main Development | 1 (lines 565-642 of the merged file) | #307 turns the legality overlay into a touch-scroll surface with a Close button; #314 adds the verdict line and the coverage rows | Keep #307's scroll surface and apply `docs/legality/handoff/overlay-verdict-fix.patch` (below), which builds its lines from `legalityRep.verdict()`. The merged file still has `if (legalityRep.ok())` at line 544, outside the conflict; the patch replaces it. |
| 8 | `src/UI/TrainerViewScreenBase.inc` | Main Development | 1 | R / touch 95 opens the overlay. #314 passes the exact source game and opens it for every verdict except No problems found; #307 resets `legalityScroll` and calls `analyze` without the source game | Keep #314's call (exact source game, verdict test) and add #307's `details.legalityScroll = 0;`. |
| 9 | `tests/test_gen1_pokemon_editor_ui_contract.cpp` | Main Development | 1 | #314 asserts `p.legalityLabel` / `p.legalityInvalid`; #307 asserts `ClassicMovesPanel::Status` | Keep both assert sets; they test different members of the same presentation. |
| 10 | `tests/test_passive_pokemon_view_contract.cpp` | Main Development | 1 | #314 asserts the Gen II modal's verdict row; #307 asserts `kOriginLegalityHeading` and "Not checked" | Keep #314's asserts; drop the "Not checked" expectation together with row 6. |

### The overlay must not say "No problems found" for an incomplete report

#307's overlay (line 528 at its head, line 544 in the trial merge, which Git merges without a
conflict) shows "No problems found." when `legalityRep.ok()`. `ok()` only means no Invalid and no
Warning was raised (`problemCount() == 0`); it is true for an Incomplete report too. Merged as is,
every Incomplete record (for example a Gen III/IV-origin Pokemon in a Gen V save) would read "No
problems found.", which is the false completion the campaign forbids. The lines should be:

- `Verdict::NoProblemsFound`: "No problems found by complete checks."
- `problemCount() == 0` but `Verdict::Incomplete`: "No problems found by the checks that ran.
  Coverage is incomplete.", then one row per coverage field (source game, internal, moves,
  encounter, event/gift, PID/RNG, egg/breeding, transfer).
- Otherwise the Invalid and Warning issues, as both branches already list them.

`docs/legality/handoff/overlay-verdict-fix.patch` is that change against #307's
`src/UI/Modals/PokemonDetailsModal.cpp` at `d104310d` (unified diff, `a/` and `b/` prefixes; apply
with `git apply` on a checkout of #307 or on the merged tree). It adds `legalityVerdict`,
`legalityClean` and `coverageLabel` next to the `analyze` call, makes the summary pill read
"Legality: no problems found by complete checks", "Legality: checks incomplete - R / tap to view" or
"N issue(s)", derives the footer segment from `legalityClean`, and replaces the `ok()` branch of the
overlay with the two wordings above plus the eight coverage rows. It is a handoff for Main
Development; the Gen V lane does not edit that file.

## Gen V in the app

Engine API on #314:

- `Legality::analyzePk5(record, Pk5Input)` (`include/Legality/Gen5Legality.h`). `record` is the raw
  136-byte box or 220-byte party PK5, encrypted (`Pk5Input::encrypted`, the default) or decrypted.
  `Pk5Input::game` is the save's game (20 White, 21 Black, 22 White 2, 23 Black 2; 0 unknown),
  `Pk5Input::slot` Party / Box / Unknown, and `Pk5Input::trainer` the save's TID, SID, OT, OT
  gender and language when the caller has them. The call reads the record only; the same input
  gives the same `Report`; malformed sizes, an undecryptable record, a bad checksum or a bad
  sanity byte are reported inside the `Report`, never thrown. Existing call sites
  (`analyzePk5(record)`) are unchanged.
- `Legality::Gen5App::analyze(record, sourceGameId, Options)`
  (`include/Legality/Gen5AppAdapter.h`, `src/Legality/Gen5AppAdapter.cpp`) is the one-call adapter
  for the app. `sourceGameId` is the app's id string (`"black_nds"`, `"white_nds"`,
  `"black2_nds"`, `"white2_nds"`; `versionFromSourceGameId` maps them). It returns the `Report`,
  its `Verdict`, and ready-to-draw text: `verdictLine` ("No problems found by complete checks." /
  "No problems found by the checks that ran. Coverage is incomplete." / "Checks incomplete: N
  warning(s)." / "Invalid: N issue(s)." / "Empty slot."), `coverageLines` (one per coverage
  field, only when the verdict is Incomplete) and `issueLines` ("Invalid: ..." / "Warning: ...").
  An Incomplete report never renders as "No problems found" (`tests/test_legality_gen5_adapter.cpp`
  asserts it on every fixture and on 5,000 fuzzed records). An unknown, non-empty id adds the
  Warning "Source game ... is not a Gen V game; the save context was not used". The adapter
  depends on nothing from #307.

On #307 the Gen V Trainer surface (`src/UI/Gen5SharedReadOnlySurface.inc`, "A narrow passive PK5
panel ... NEVER calls Legality::analyze") shows no legality at all, and its record type is
`Pokemon5ReadOnly`. Wiring Gen V legality into the app is one call from that panel:
`Legality::Gen5App::analyze(bytes, screen.sourceGameId, {.encrypted = true, .slot = ...})`, then
drawing `verdictLine`, `coverageLines` and `issueLines` the way the Gen III/IV surfaces draw
theirs. That is a Main Development UI change and is left to them; the engine needs nothing more.

## Combined test plan (on the merged tree, before any merge is proposed)

1. `make -f Makefile.host host-test` and `host-sanitize`, GCC and Clang. The Gen V targets are
   `test_legality_gen5`, `test_legality_gen5_adapter` and their `_sanitize` builds
   (`Makefile.host.gen5-legality`).
2. Every legality lane gate in `docs/legality/LEGALITY_GATE_MATRIX.md` (L1-L6 workflows and the
   invalid-site inventory), on the exact merged commit.
3. PKHeX differentials: `tools/pkhex_gb_diff` (Gen I and II), `tools/pkhex_gen34_diff` (Gen III
   and IV, mutation and grown corpora), `tools/pkhex_gen5_diff` (seeds 1, 2, 7, 11, 13, 17); each
   must report 0 false Invalid and 0 false Valid.
4. The 20-title matrix (`tests/test_legality_title_matrix.cpp`).
5. #307's own Gen V suites (`test_gen5_source_e2e`, `test_gen5_export_validation`) and
   `gen15-integrated-candidate.yml` (one Gen I-V NRO, packaged RomFS verified).
6. On the merged tree, `grep -n "legalityRep.ok()" src/UI` must find nothing: every "no problems"
   wording comes from `verdict()`.
7. A combined Gen I-V app and legality test campaign on a Switch, as broz requires before any
   Gen VI-IX work.
