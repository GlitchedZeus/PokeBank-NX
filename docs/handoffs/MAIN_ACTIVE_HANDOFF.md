# Main application: active handoff (Claude → Codex)

> **Rules:** read [`docs/ENGINEERING_PROTOCOL.md`](../ENGINEERING_PROTOCOL.md) first.
> **Source of truth:** GitHub. Resume from the SHA below, not from any chat summary.
> **Updating:** update this file at every significant checkpoint, and push it.

Last updated: 2026-10-20, by the Claude coordinator session (`session_01AnyHUH96AxxgM3qguezsET`).

## 1. Where to start

| Item | Value |
| --- | --- |
| Repository | `GlitchedZeus/PokeBank-NX` |
| Integration branch | `claude/gen5-integration-20261014` (PR #307, **draft**: keep it draft) |
| Last code commit (the round-3 NRO was built from this) | `62a2f8c34476062ece344ef2188ab6b385369a7d` |
| Branch HEAD before this handoff commit | `c3b0ac4812ef9d584de5d74109189c3ac897fe24` (docs only after `62a2f8c3`) |
| Branch HEAD after this handoff commit | see `git ls-remote origin claude/gen5-integration-20261014` (docs only) |
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
| 3 | `62a2f8c3` (sha256 `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078`) | **Owner reports 70 PASS / 37 FAIL in total. The per-item results are NOT yet on GitHub.** | **missing:** create `docs/hardware/ROUND3_RESULTS_62a2f8c3.md` from the owner's sheet |

**What to do with the round-3 results:**
- **First task: get them in.** Before repairing anything, get the owner's round-3 sheet (PASS / FAIL / NOT VERIFIED
  per row of [`docs/hardware/RETEST_62a2f8c3.md`](../hardware/RETEST_62a2f8c3.md)) and commit it as
  `ROUND3_RESULTS_62a2f8c3.md`.
- **Do not guess them.** Never infer which items failed.
- **Protect the passes.** Add every new round-3 PASS to the hardware gate
  (`tests/test_hw_round1_regression_gate.py`, mutation test `tests/test_qa_hw_round1_gate_mutations.py`), the
  same way B01/B02/B03/B06/S01/S03 were added in round 3.
- **The 48 passes already guarded:** the gate protects the 42 round-1 passes plus the 6 round-2 passes today
  (138 guards). They are listed in Group A of `RETEST_62a2f8c3.md`.

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
- **R2-CHECK-011:** the Gen V "VGE" names match PKHeX.

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

**WIP checkpoint branches.**
- On 2026-10-20 every lane session was asked to push any unpushed work to `wip/<lane>-checkpoint-20261020`. The
  requests to lanes S and P were blocked by a permission check and never delivered.
- Run `git ls-remote origin 'refs/heads/wip/*'` to see which branches exist. Each one is unfinished work. Review it
  before integrating anything from it.
- If none exist, no lane reported unpushed work.

**Other lanes, separate and not to be mixed in:**
- `claude/gen1-5-legality-campaign-w9mkom` (legality, active 2026-10-20);
- `research/gen6-9-codex-20261018`;
- `claude/gen6/7/8-native-*`.

**Git conflicts:** none open on #307.

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
