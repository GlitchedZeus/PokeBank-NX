# Main application: active handoff (Claude → Codex)

> **Rules:** read [`docs/ENGINEERING_PROTOCOL.md`](../ENGINEERING_PROTOCOL.md) first.
> **Source of truth:** GitHub. Resume from the SHA below, not from any chat summary.
> **Updating:** update this file at every significant checkpoint, and push it.

Last updated: 2026-10-21 UTC, by Codex (Round 4 takeover; owner-authorized application work).

## 1. Where to start

| Item | Value |
| --- | --- |
| Repository | `GlitchedZeus/PokeBank-NX` |
| Integration branch | `claude/gen5-integration-20261014` (PR #307, **draft**: keep it draft) |
| Last code commit (the round-3 NRO was built from this) | `62a2f8c34476062ece344ef2188ab6b385369a7d` |
| Verified application integration HEAD at takeover | `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea` |
| Codex working checkpoint | `wip/codex-round4-takeover-20261021`; see section 6c; not yet integrated into #307 |
| Legality engine | PR #314: a **separate lane**. Do not merge it with #307 or `main` without the owner's authorization. |
| `main` | Not touched by this campaign. Nothing merges to `main` without the owner. |

To start:

```sh
git fetch origin
git checkout -B work origin/claude/gen5-integration-20261014
git log --oneline -1 62a2f8c34476062ece344ef2188ab6b385369a7d   # must exist: the round-3 code head
```

## 2. Hardware status (owner-recorded only)

| Round | NRO tested | Owner result | Recorded in |
| --- | --- | --- | --- |
| 1 | `7fea1797` | 42 PASS | `docs/hardware/ROUND1_RESULTS_7fea1797.md` |
| 2 | `e10f307f` | 6 more PASS (B01, B02, B03, B06, S01, S03) | `docs/hardware/ROUND2_RESULTS_e10f307f.md` |
| 3 | `62a2f8c3` (sha256 `0c7a6fb0…2078`) | **110 checks: 70 PASS, 37 FAIL, 3 NOT TESTED** | [`docs/hardware/ROUND3_RESULTS_62a2f8c3.md`](../hardware/ROUND3_RESULTS_62a2f8c3.md) (owner, `c2287f50`) |

**Round-3 results are on GitHub.** Read `ROUND3_RESULTS_62a2f8c3.md` §9 for the owner's priority order (P0–P3).
Protect all 70 PASS items. The hardware gate guards the 48 round-1/2 passes today; QA was asked (2026-10-20) to
add the remaining round-3 passes to `tests/test_hw_round1_regression_gate.py`.

## 3. Round-3 state of each repair

Each repair is at "Implemented → Tested → Pushed → Integrated". None is hardware verified until the
round-3 results file says so.

| ID | Repair | Integrated at |
| --- | --- | --- |
| R2-BUG-004 | Species change resets moves to the game's level-up moves (`SpeciesMoveDefaults`, `tools/gen_levelup_learnsets15.py`, pinned pret + PKHeX 6501f0ab) | 8a84128e |
| R2-BUG-002 | Gen III/IV staged Trainer Name/Money editors and export (`Gen3StagedTrainerEditor`, `Gen4StagedTrainerEditor`) | 7ed1cc16 |
| R2-BUG-001 | Trainer "A: Edit" footer, including Gen I (`ClassicTrainerSession::readOnlyForFooterOverride`) | 11940524 |
| R2-BUG-003 | Gen V Trainer layout matches Gen I–IV (`Gen5TrainerScreen::footer`) | 7ed1cc16 |
| R2-BUG-012 | Boxed-row modals (`include/UI/SharedBoxedRows.h`) | 7ed1cc16, 9d4b8b71 |
| R2-BUG-013 | Coloured controller glyphs (`GlyphInstruction.h`, `ScreenChrome::drawGlyphHelpLine`) | aeccd13, 678627f5 |
| R2-BUG-006 | 4-column item grid (`SharedItemGridPicker.h`, `MenuType::ItemPicker`) | 9d4b8b71 |
| R2-BUG-005 | TM/HM move details (`MachineMoves.h`), Gen V HM labels and feathers | 7ed1cc16, 9d4b8b71 |
| R2-BUG-007 | Move chooser type badges | 9d4b8b71 |
| R2-CHECK-009 | Gen I Box Actions → Move (`beginGen1FromAction`) | 9d4b8b71 |
| R2-BUG-008 | Gen III Met Location "keep stored" (`Gen3EncounterPickerModel.h`) | 5949b15 |
| Part 9 #1, 3, 4, 6, 8, 9 | Gen V staged-slot actions, provider labels, RetroArch core from save evidence, Gen V item art, Gen IV Storage carry, Home/Profile focus | see `RETEST_62a2f8c3.md` Group B |

Investigated, not defects:
- **R2-CHECK-010:** the Super Rod in the R/S PC is native behaviour. The R/S Devon Goods / Letter / Orb and
  Emerald HM PC offers were removed.
- **R2-CHECK-011:** the names match PKHeX, but the **owner FAILED it in round 3** (location lists too broad); QA CG53 covers it and lane P owns it with NEW-12.

Never device-verified: B04, E03, E11, L06, S04, S06, C09, M08, and the real RetroArch/DraStic launch and return.

## 4. Tests actually run on `62a2f8c3` (all PASS)

**GitHub Actions on `62a2f8c3`, all success:**
- Native PR Gate 39227970367
- Host Tests GCC 39227970682 (41.6 min)
- Host Tests Clang 39227970563
- Gen I–V Integrated Candidate 39227970775
- Gen I–IV HW Candidate 39227970763
- Gen V Focused 39227970525
- App QA Contracts 39227970615
- Fuzz 39227970787
- Workflow Hygiene 39227970810
- Touch 39227970818
- Packed Move 39227970721 / 39227970952

**QA-F9 exact-binary audit:** run 39233872565, job 117998117020, on binding commit `887552e7`. Result
`NRO AUDIT: PASS`; CG `--candidate` reports 32 contracts, 0 failing.

**The integrator's local gate on the same app code** (`9d4b8b71`, clean build dirs; it differs from `62a2f8c3` only by one CI timeout line):
- GCC and Clang `host-test`, `host-sanitize` and `qa`: 429 PASS lines each;
- py contracts 29/29;
- hardware gate PASS;
- devkitA64 OK.

**Failing tests:** none on `62a2f8c3`.

## 5. NRO

| | |
| --- | --- |
| Built natively in CI | yes |
| File | `PokeBank-NX-Gen1-5-Candidate-62a2f8c3.nro`, 176029361 bytes |
| Artifact | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39227970775/artifacts/12175187129 (expires 2027-01-17) |
| SHA-256 | `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078` |

Later docs-only commits on #307 also trigger a candidate build. Those artifacts are **not** the audited candidate.

## 6. Branches and parallel work

**Lane branches.** All are integrated into #307 by SHA. Their tips are history; do not resume from them.

| Lane | Branch | Tip |
| --- | --- | --- |
| C | `claude/gen14-campaign-20261015` | `114a22f3` |
| S | `claude/shared-editor-gen35-20261016` | `3ff061bf` |
| P | `claude/pickers-search-scrollbar-20261017` | `8d533b52` |
| I | `claude/item-quantity-policy-20261016` | `18ffcc76` |
| R | `claude/reroll-mechanics-20261017` | `a1c96ed6` |
| L | `claude/launch-return-20261015` | `4f57bde4` (draft PR #319, CI only) |
| QA | `audit/app-qa-20261014` | `5c965cb3` |
| Coordinator | `claude/optimistic-euler-ph2z4f` | `docs/handoffs/MAIN_DEV_CHECKPOINT.md`, `ROUND3_LANE_PLAN.md` |

**WIP checkpoint branches: none.** On 2026-10-20 every lane session was asked to push any unpushed work. Six lanes replied
"clean, nothing unpushed", each with an `ls-remote`-verified tip matching the table above: 2R `c3b0ac48`, QA `5c965cb3`,
C `114a22f3`, P `8d533b52`, R `a1c96ed6`, L `4f57bde4`. The messages to **S** and **I** were blocked by a permission
check, so their local state is unconfirmed; their remote tips are S `3ff061bf` and I `18ffcc76`.
`git ls-remote origin 'refs/heads/wip/*'` returned nothing.

**Other lanes, separate and not to be mixed in:**
- `claude/gen1-5-legality-campaign-w9mkom` (legality, active 2026-10-20);
- `research/gen6-9-codex-20261018`;
- `claude/gen6/7/8-native-*`.

**Git conflicts:** none open on #307.

## 6b. Round 4 (2026-10-20): live repair status

States: Implemented → Tested (host) → Pushed → Integrated (#307) → Hardware verified (owner only).

| Owner IDs | Repair | State | Branch @ SHA |
| --- | --- | --- | --- |
| NEW-16, C01 | Gen V Home A / Items open the validated assigned save directly; anything not ready falls back to Save Instances with the reason (`SaveSelectScreen::openAssignedGen5Directly`) | **Integrated** (Native PR Gate 39511458784 green) | #307 `c59ac178` (from `wip/coord-gen5-direct-open-20261020` @ `61491b53`) |
| B08 | Export result / status messages painted on every surface (`TrainerViewScreen::drawTransientStatus`, scope guards in the composite `draw`/`update`); root cause: Gen II–V surfaces return before the only status painter | **Integrated**; `tests/test_status_toast_contract.py` | #307 `c59ac178` |
| R2-BUG-006 | Short item grids use balanced rows and a narrower frame (lane P) | **Integrated**; host test `test_item_grid_picker` | #307 `5887c56c` (P `e3c81745`) |
| R2-REROLL | Gen II Unown letter recomputed from DVs and the editor sprite follows it; DV and shiny rules audited correct against pret (lane R) | **Integrated**; host test `test_gen2_unown_letter` | #307 `aea3730e` (R `fd30e0fc`) |
| NEW-11, NEW-05, NEW-08, NEW-10 | "Pikachu #25" label; search keyboard no longer covers the species list; scrollbar gutter in value lists; compact Met Location popups (lane P) | **Integrated** | #307 `1d803f01` (P `9573e8e8`) |
| QA gate | All 70 round-3 PASS items guarded (196 guards); CG29; CG30–CG35 XFAIL contracts reproduce R2-TRAINER/R2-BUG-002, NEW-19, R2-REVIEWMOVE, L06/E02/NEW-20, NEW-17, AUD-01 | **Integrated** | #307 `847fe390`, `0d810f7a` |
| NEW-04 | Gen II PC box Move: the grid refresh used `finalizedBytes()`, which refuses mid-carry, so every pickup cancelled. Added display-only `StagedEditor::presentationBytes()`; finalize still refuses mid-carry (lane C) | **Integrated**; host test `test_gen2_box_move_presentation` (G/S/C pickup, same/other-box drop, cancel, finalize + export) | #307 `131fb23f` (C `c3807c63`) |
| NEW-07, NEW-06, NEW-13 | Crystal OT Gender cycles inline; Held Item search; Ability names + short effects (lane P). **Caveat:** the ability effect lines in `include/UI/AbilityEffects.h` are P's own summaries; no source in the repo. Owner or I to spot-check | **Integrated**; `test_gen2_field_selector`, `test_item_picker_search`, `test_ability_effects` | #307 `75546e44` (P `77576217`) |
| QA contracts | CG30–CG60: all 37 owner FAILs as XFAIL, with owners; CG39/57/58 enforced | **Integrated** | #307 `c80a86c5` |
| R2-EMUBACKUP, C07, S05 | PKSE-style emulator backup browser: create, browse, list, delete with Y; no automatic backup (lane L) | Pushed (WIP), not integrated yet | `wip/l-backup-browser-20261020` @ `6cb161c4` |

**XFAIL contracts:** CG30–CG35 fail in `--candidate` mode until their lanes land. Normal mode reports 0 failing. A round-4 candidate needs all of them fixed (or the owner's decision).

**Lane assignments sent 2026-10-20.** Each lane works from #307 `c2287f50` and pushes to its own branch or `wip/<lane>-<topic>-20261020`:

| Lane | Owner IDs |
| --- | --- |
| L | R2-EMUBACKUP, C07, S05: PKSE-style backup browser |
| C | NEW-04: Gen II PC Move; then NEW-19, R2-TRAINER, R2-BUG-002: Gen I–IV staged exit menu; then R2-REVIEWMOVE and NEW-18 |
| S | L06, E02, NEW-20: Party Actions; E03: legality layout; M04/M05; AUD-01; NEW-01/NEW-02 |
| I | NEW-17: Gen V Key Items / TM / HM editing; NEW-03; R2-CHECK-010 glyphs |
| R | R2-REROLL / Unown audit; NEW-15; R2-SPECIAL; NEW-09; NEW-14 |
| P | NEW-12 / R2-CHECK-011; NEW-10; NEW-05; NEW-06; NEW-07; NEW-08; NEW-11; NEW-13; R2-BUG-006 |
| QA | Gate the 70 PASS items; contracts for the 37 FAIL items |

The 2R integrator lane was not reached (permission check), so the coordinator is integrating. Before resuming, check each lane
branch and every `wip/*` branch with `git ls-remote origin`.

## 6c. Codex takeover checkpoint (2026-10-21 UTC)

Codex owns `wip/codex-round4-takeover-20261021`; the owner explicitly authorized taking over #307,
reviewing/integrating the backup work and finishing the remaining Round 4 repairs. Do not write this
branch concurrently. No messages were sent to other contributors. Legality #314 remains separate.

- Integration base: `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea` (all existing 12 repair entries retained).
- Reviewed backup tip: `44fb4b7eaa58f3e627ff981d84f2caeaed293667`.
- Working code checkpoint: `fe5c3ad76fd5b1e54d5f8e4d773986b7757405ef`, a forward merge preserving both parents.
- Implemented: explicit emulator/installed backup creation; newest-first combined manual/export history;
  Y-confirmed eligible deletion; original-source refusal; no automatic backup on unchanged open;
  removal of obsolete Auto-Backup setting; exact selected legacy source routing; live last-copy
  recheck at deletion; recovery of earlier Working bytes after a failed partial refresh, without
  touching an unexpected callback destination.
- Tested: nine focused suites under GCC and Clang normal builds (18 executions): emulator backup
  access/layout, classic verified export, verbatim export, Gen I export transaction, Gen V export
  validation, Gen V Review/Discard/Export trace, source-mutation policy and write policy. Game Hub
  source contract and repository safety passed. All 70 owner PASS items remain guarded (196 guards).
- ASan/UBSan focused runs and complete GCC export contracts/20-game matrix are in progress. Initial
  sanitizer execution in the tracing sandbox hit the documented LeakSanitizer/ptrace limitation;
  reruns preserve leak detection outside that sandbox. No sanitizer checks were disabled.
- NOT yet integrated into #307 and NOT hardware verified. No new official candidate is ready.
- Remaining: finish/record phase-1 tests, enforce CG59/60 after their genuine passing behavior, then
  Trainer Review/exit/named rows, Party Actions, Gen V inventory/language/defaults, remaining UI and
  source-backed location work, all candidate contracts, full gates, exact NRO build/binding/audit.

The owner's Round 3 report is already committed. Do not request it again. S06 remains NOT TESTED;
exports are tested only on disposable host sources. The pre-takeover `wip/coord-gen5-direct-open-20261020`
tip `61491b533ffba38a03bd46639a8ac3ea2ee11d0e` is already represented in #307. The backup WIP branch
remains preserved. Do not delete WIP refs or claim this checkpoint completes all 37 owner FAILs.

## 7. Next development tasks (in order)

1. **Record the round-3 results** (§2). Then add the round-3 PASS items to the hardware gate.
2. **Reproduce each round-3 FAIL from source before changing code.** Use the owner's input trace when a failure
   is input-related (`sdmc:/switch/PokeBank-NX/config/input_trace.enable`).
3. **Owner priority areas.** Which items actually failed comes from the results file. Files to inspect first:
   - **Save / backup management:**
     - `include/UI/BackupSelectionScreen.h`
     - `include/UI/EmulatorBackupLayout.h`
     - `include/UI/SourceBackupClassification.h`
   - **Gen II Move Pokémon:**
     - `src/Integration/Gen2/Gen2PackedMove.cpp`
     - `include/UI/ClassicPackedMovePresentation.h`
     - `tests/test_gen12_packed_move.cpp`
   - **Gen I–IV Review / exit parity:**
     - `include/UI/Gen12ReviewDiscardConfirm.h`
     - `include/UI/PokemonEditorExitGuard.h`
     - `include/UI/LeaveDecisionLayout.h`
     - reference: `include/Integration/Gen5/Gen5SharedReview.h`
   - **Gen V direct save opening:**
     - `include/Integration/Gen5/Gen5SourceSelection.h`
     - `Gen5SourceDiscovery.h`
     - `Gen5AssignedSource.h`
   - **Export Copy:**
     - `include/Integration/Common/VerifiedStagedExport.h`
     - `Gen{1..4}ExportTransaction.h`
     - `Gen5WorkingCopyExport.h`
   - **Gen V Bag (Key Item / TM / HM editing):**
     - `include/Integration/Gen5/Gen5Bag.h`
     - `Gen5BagScreen.h`
     - `ItemQuantityPolicy`
     - `MachineMoves.h`
4. **For each working repair:**
   - run the gates in the protocol;
   - commit and push (to a `wip/` branch if anything is unfinished);
   - update this file.
5. **Build one round-4 candidate.** When the round is complete, build the single round-4 candidate on the #307 head,
   bind it in `docs/qa/CANDIDATE_UNDER_AUDIT.env` (docs-only), and run QA-F9.

**Partially implemented files:** none known on #307. Check any `wip/*` branches (§6).

## 8. Source-save protection risks

- **Export writes a separate copy.** `BUILD_IDENTITY.txt` must keep `SOURCE_SAVE_WRITES=DISABLED`.
- **Never write a live emulator save.** For S06, the owner copies the export into a separate slot by hand.
- **Repairs to Export Copy, Gen V direct save opening, and backup management touch the immutability path.** Keep
  these green:
  - `tests/test_qa_export_contracts`;
  - the QA matrix `export`, `export.checksum` and `immutability` cells (20/20 games);
  - `test_ci_repository_safety_contract.py`.
- **Never add saves, ROMs or keys to the repository.**
