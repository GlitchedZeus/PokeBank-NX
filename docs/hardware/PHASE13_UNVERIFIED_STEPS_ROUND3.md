# Round 3: device steps for the items still NOT VERIFIED after round 2

Round 2 on `e10f307f` left six items NOT VERIFIED: B04, E03, E11, L06, S04 and S06. They are **not
failures** (see [ROUND2_RESULTS_e10f307f.md](ROUND2_RESULTS_e10f307f.md)).

This page is for the round-3 candidate NRO, the one named in `docs/qa/CANDIDATE_UNDER_AUDIT.env` once 2R sets it.

- Every check under **Host evidence** passes on the host and in CI. That makes it **host-verified**, NOT device-verified.
- Only the owner's run of that exact NRO counts as device evidence.
- No emulator runs in CI. Nothing here claims an emulator loaded a file.

Report each item as PASS / FAIL / NOT VERIFIED, with the game and save used. If something misbehaves, use
the input trace in [RETEST_7fea1797.md](RETEST_7fea1797.md) §0.

## E03: Origin & Legality wording

**Device steps**
1. Use one save per generation, Gen I to V. Open a Pokémon, then Actions.
2. The row is labelled **Origin & Legality**. The modal heading and the right-hand provenance pane use the same words.
3. None of "Encounter legality", "Legality & Provenance" or "Deep encounter legality" appears anywhere.

**Host evidence**
- CG03 `origin-legality-heading` (enforced) requires the exact heading in all five generations and bans the three old strings in UI sources.
- The hardware gate pins CG03 for the round-1 accepted items.

**Known gap (Part 9, report 1):** Gen V Actions has **no** Origin & Legality row, while Gen IV does
(`Gen4SharedPokemonSurface.inc:954`). On a Black/White save, expect the heading only where a provenance
pane exists. Report the missing row as part of S03/S's Gen V parity, not as an E03 regression.

## E11: Rerolls keep the requested Shiny / Gender

**Device steps**
1. Gen I/II: edit a non-shiny Pokémon and press **Y** several times.
   - The DVs change at once, with no confirm, and the Pokémon stays non-shiny.
   - On a shiny one, it stays shiny (Gen II).
   - In Gen II, gender and the Unown letter follow the DVs.
2. Gen III–V: do the same on a non-shiny and a shiny Pokémon.
   - The IVs change at once.
   - Shininess, gender, nature and (Gen III) the Unown letter are kept.
3. Discard. The source save is unchanged.

**Host evidence**
- CG06 `reroll-without-confirmation` (enforced).
- `tests/test_reroll_mechanics_gen15.cpp` (lane R, now in `host-test`): 100+ rerolls per shiny and non-shiny case in every generation.
- `tests/test_gen12_dv_reroll.cpp` and `tests/test_shared_stat_reroll.cpp` (in `host-test`).
- Acceptance-matrix `reroll` cells: PASS in all 20 versions.

In Gen I/II a shiny and a non-shiny can never share DVs, because shininess *is* the DV pattern. In
Gen III–V shininess comes from the PID and IDs, so identical IVs are correct (`docs/REROLL_MECHANICS.md`).

## L06: Party / summary-panel taps

**Device steps**
1. In a Gen III, IV and V save, open Boxes and tap the selected Pokémon's summary panel. Then open Party and tap a Pokémon.
2. Each opens that game's own Actions sheet, with no "Make Shiny" row and no flash of a generic View.
3. In a Gen I/II save, do the same. The generation's own sheet/View opens.

**Host evidence**
- `tests/test_party_summary_tap_routing.cpp` (in `host-test`) walks the Gen III/IV and Gen V routing rules over every hit id the box panel publishes. It also reads the shipped call sites: summary id 2000 and the party taps reach the native Actions path before any generic fallthrough.
- CG11 (enforced): no generic action sheet or details modal opens in a Gen I–V session.

**Not covered on host:** Gen I/II taps have no routing test of their own; CG11 is their only host guard.

## S06: The exported edited save loads in an emulator

**Device steps**
1. Take the export from S04 (Black/White) and one export each from Gen I–IV.
2. Load each in the emulator as a **separate** save. Never overwrite the original.
3. The edits are present and the game reports no corrupted save.
4. Load the original save. It is unchanged.

**Host evidence, all 20 games** (acceptance matrix, `make -f Makefile.host qa-matrix`, GCC + clang, plain + ASan/UBSan):
- The `export` cell publishes into a new directory:
  - `original_backup.*` equals the source;
  - `edited.*` **strictly reparses** as the exact game with the edit;
  - only whitelisted record/block/checksum bytes differ;
  - a second export gets a new directory.
- Round 3 adds `exportCorruptionProbe`. A copy of `edited.*` with one checksummed byte changed must **stop** reparsing as the edited save. That proves the reparse checks the checksum an emulator checks, on the exact bytes written. The runner fails unless all 20 versions record `export.checksum=detects-corruption` (see [GEN1_5_MATRIX.md](../qa/GEN1_5_MATRIX.md), "Round 3").
- The `immutability` cell: source bytes, size and nanosecond mtime are unchanged after discovery, linking, every edit and the export, in all 20 games.
- Export contracts C1–C5 (`tests/test_qa_export_contracts.cpp`).

This is evidence that the file is internally consistent. It is **not** evidence that an emulator accepts it.

## B04 and S04

These are unchanged from [PHASE13_UNVERIFIED_STEPS.md](PHASE13_UNVERIFIED_STEPS.md):
- **B04** host check: `test_bag_add_flow_gen15` + CG12.
- **S04** host check: export contracts + `tests/test_gen5_review_discard_export_trace.cpp`, now in `host-test`.

Every NRO is **NOT DEVICE TESTED** until the owner tests that exact file.
