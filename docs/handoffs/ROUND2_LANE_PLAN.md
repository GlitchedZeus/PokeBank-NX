# Gen I–V Hardware Round 2 — lane plan and file ownership

**Coordinator:** session_01AnyHUH96AxxgM3qguezsET (branch `claude/optimistic-euler-ph2z4f`).
**Integration:** PR #307, branch `claude/gen5-integration-20261014`, base of this round `8587f20b`.
**Regression baseline:** `PokeBank-NX-Gen1-5-Candidate-7fea1797.nro` (code `7fea1797`), owner hardware
round 1: 42 of 54 PASS.

Round 1 results as reported by the owner (verbatim classification, do not reclassify):

| Status | IDs |
| --- | --- |
| FAIL (confirmed on device) | B01 Gen I Add Item asks quantity first; B02 Gen II same; B03 Gen III same; B06 stackable quantity workflow; S01 Gen V PP / PP Ups captions too close; S03 Gen V Create/Edit left-panel text clipped + parity |
| NOT VERIFIED (not failures) | B04 Gen IV/V Add Item; E03 Origin & Legality wording; E11 shiny vs non-shiny rerolls; L06 party/summary taps; S04 Gen V Review/Discard/Export; S06 exported copy in emulator |
| PASS (42) | everything in "Part 1" of the owner's directive; must not regress |

## Rules for every lane (owner's standing rules)

1. Fetch first. Merge **forward** from `origin/claude/gen5-integration-20261014` before starting and
   before each handoff. Never force-push, reset backward, rebase or amend pushed commits.
2. Production C++ with tests. No documentation-only commits except the hardware/retest docs owned by
   QA and the coordinator.
3. Staged edits only. No source-save writes, no Inject Save, no emulator-save mutation.
4. Never invent format rules: cite pinned sources (PKHeX `6501f0ab`, pret disassemblies, PKSM-Core
   `aa22d7a`) in the header of any code that depends on a format fact.
5. No Gen VI–IX, no Master Vault, no changes to #314 or legality.
6. One shared editor architecture. Same kind of menu = same presentation contract. No per-generation
   copies of UI code; extend the shared component and call it.
7. Safe input semantics are frozen: A select, B back/cancel, Y commits destructive or launch decisions,
   A/B stay. Events consumed once. Never change this to match button colours.
8. Do not regress any of the 42 PASS items. Keep every existing contract test green; if a literal-string
   contract blocks a legitimate change, tell the coordinator (QA owns the contract tests).
9. Gate before handoff: `make -f Makefile.host host-test` (GCC and Clang), `host-sanitize`,
   `make -f Makefile.host qa`, and the Python contracts that cover your files
   (`python3 -m pytest tests/test_qa_cross_gen_contract.py` and the others listed in area 12 of the
   survey), plus `git diff --check`.
10. Stay inside your file ownership. If you need a change elsewhere, send the exact diff request to the
    owner lane or the coordinator; do not edit the file.
11. Report to the coordinator by `send_message` with the pushed SHA, files, tests run, and anything
    unresolved. 2R integrates; no lane merges into #307 or main.

## Lanes

| Lane | Session | Branch | Phases |
| --- | --- | --- | --- |
| I (items) | session_01WwfzZtJWqDsNSrMV866RJE | `claude/item-quantity-policy-20261016` | 1 (bag Add/quantity state machine, Gen I–V), 13/B04 host tests |
| C (Gen I/II) | session_012Qk8aPZvVGKZhMUyUC6rya | `claude/gen14-campaign-20261015` | 4 (Gen I gold/HP DV; Gen II Special), 5 (Gen I trainer editing + Trainer footer all gens), 11 (Crystal Met Time inline) |
| S (shared editor, Gen III–V) | session_019GeN9mKTkgfpfuqDGPzWhz | `claude/shared-editor-gen35-20261016` | 2 (SharedMenuGeometry), 4 (shared field-state styling contract), 9 (move chooser rows, Gen V PP captions), 12 (Gen V Create/Edit parity + Review/Discard/Export trace), 14 |
| P (pickers) | new | `claude/pickers-search-scrollbar-20261017` | 3 (live species search), 8 (draggable scrollbars) |
| R (rerolls) | new | `claude/reroll-mechanics-20261017` | 10 (Gen I–V reroll mechanics audit + tests + doc), 13/E11 |
| 2R (Home/integration) | session_01Gn6iaKx8qUJWbAydeQ6ST6 | `claude/gen5-integration-20261014` | 6 (launch dialog), 7 (emulator backups), 13/L06, integration, CI, ONE candidate |
| QA | session_01VXXPj238uqedMPANHRVViz | `audit/app-qa-20261014` | Part 1 regression gate (42), Part 2 classification, 13 instructions, 15 tests/matrix/audit, retest checklist |

## File ownership (exact)

**Lane I**
- `src/UI/ClassicInventoryOverlay.cpp`, `src/UI/ClassicInventoryUIModel.cpp`, `include/UI/ClassicGenericUiGuard.h`
- `include/Inventory/*`, `src/Inventory/*`
- `src/UI/Gen4SharedPokemonSurface.inc` bag section only: `gen4QuantityRule`, `handleItemQuantity`,
  Add path, picker open, `drawItemAddPicker` (today :1673–2060); `include/UI/Gen4BagQuantity.h`
- `src/UI/Gen5SharedReadOnlySurface.inc` bag section only: `gen5QuantityRule`, `handleBag`, `drawBag`
  (today :886–1040 and :1778–1850); `include/Integration/Gen5/Gen5BagScreen.h`, `Gen5Bag.h`
- `src/Integration/Gen4/Gen4StagedPokemonEditor.cpp` bag functions; Gen1/Gen2/Gen3 staged inventory editors
- `src/UI/Panels/ItemsPanel.cpp`, `src/UI/Dialogs/ItemEditDialog.cpp`, `include/UI/InventoryUIContract.h`
- `src/UI/TrainerViewScreenBase.inc` bag/pouch paths only (PouchItem picker open :4464, apply :2811–2880,
  classic footer :5098, Gen IV bag footer :5265–5290)
- New: `include/UI/SharedBagAddFlow.h` (one state machine: Picker → stage qty 1 → inventory row → A edits
  quantity only when the rule is editable), tests `tests/test_bag_add_flow_gen15.cpp`

**Lane C**
- `src/UI/Gen1*.inc`, `src/UI/Gen1PokemonDetailsPresentation.cpp`, `src/Integration/Gen1/*`, `include/Integration/Gen1/*`,
  `src/Legacy/RBYReadOnlyTrainer.cpp`
- `src/UI/Gen2*.inc`, `src/UI/Modals/Gen2PokemonDetailsModal.cpp`, `include/UI/Gen2*.h`, `src/Integration/Gen2/*`
- `src/UI/TrainerViewScreenBase.inc` Trainer page only: `drawTrainerView` (:443–520), `editTrainerName`,
  `editTrainerMoney` (:1600–1660), dispatch :2706, input :4744, and the Trainer footer
- `src/UI/TrainerViewScreenGSCOverlay.inc`
- New: `src/Integration/Gen1/Gen1StagedTrainerEditor.{h,cpp}` (name encoding, BCD money, checksum),
  tests `tests/test_gen1_trainer_edit.cpp`, `tests/test_gen2_special_stat.cpp`, `tests/test_crystal_met_time_inline.cpp`
- Applies S's `SharedFieldState.h` to Gen I/II once S publishes it

**Lane S**
- `include/UI/SharedEditorPresentation.h`, `SharedEditorChrome.h`, `SharedDataPaneLayout.h`, `SharedMovesPanel.h`,
  `SharedMoveFacts.h`, `MovePickerPresentation.h`, `MovePickerRowUI.h`, `SharedPokemonShell.h`,
  `SharedPokemonEditorContract.h`, `ClassicGameContext.h` (add Gen IV/V `exactGameName`)
- `src/UI/SharedPokemonEditorSurface.inc` except the species-picker functions (P) and scrollbar lines (P)
- `src/UI/Gen3SharedPokemonSurface.inc` except `handleSpeciesPicker`/species part of `openValuePicker` (P)
- `src/UI/Gen5SharedReadOnlySurface.inc` except the bag section (I) and `speciesSearch` (P)
- `src/UI/Gen4SharedPokemonSurface.inc` except the bag section (I) and `kSpeciesSearch` stub (P)
- `src/UI/Dialogs/PickerDialog.cpp`, `include/UI/Dialogs/PickerDialog.h`, `src/UI/Dialogs/SaveConfirmDialog.cpp`
- `include/Integration/Gen5/*` except `Gen5BagScreen.h`/`Gen5Bag.h` (I) and `Gen5PidCoordination.h` (R);
  `src/Integration/Gen5/*`; `src/Integration/Gen3/*`, `src/Integration/Gen4/*` except bag functions (I)
- New: `include/UI/SharedMenuGeometry.h` (menu-type width/row/padding/sprite-gap rules, content-driven
  height), `include/UI/SharedFieldState.h` (Editable / ReadOnly-gold / Disabled styling contract),
  tests `tests/test_shared_menu_geometry.cpp` (rectangle bounds, no clipping, text width),
  `tests/test_gen5_create_edit_parity.cpp`, `tests/test_gen5_review_discard_export_trace.cpp`
- Publishes `SharedMenuGeometry.h` and `SharedFieldState.h` FIRST (first push) so C, P, 2R can consume them.

**Lane P**
- `include/UI/SharedSpeciesPicker.h`, new `include/UI/SharedSearchInput.h` (app-owned incremental text entry:
  on-screen key grid + Backspace/Clear, D-pad/stick/touch), new `include/UI/SharedScrollbar.h`
  (track + draggable thumb, hit-test, page on track tap), `include/UI/TouchScroll.h`
- Species-picker call sites only: `Gen1PokemonEditorOverlayUXCleanup3.inc` `ux3*SpeciesPicker` fns and
  `ux3DrawPicker` species branch; `Gen2PokemonPickerOverlay.inc` species functions;
  `Gen3SharedPokemonSurface.inc` `handleSpeciesPicker` + species part of `openValuePicker`;
  `SharedPokemonEditorSurface.inc` `handleValuePicker` species branch, X search hook, species modal in
  `drawValuePicker`; `Gen5SharedReadOnlySurface.inc` `speciesSearch`; Gen4 `kSpeciesSearch`
- Scrollbar call-site lines only (replace `drawScrollbar` / `drawVerticalScrollIndicator` with the shared
  component and add hit-testing): PickerDialog.cpp:199, BackupSelectionScreen.cpp:340,
  TrainerViewScreenGSCOverlay.inc:852, PokemonDetailsModal.cpp, Gen2PokemonDetailsModal.cpp:303,
  SaveSelectScreen.cpp:4960, SharedPokemonShell.h:150, SharedPokemonEditorSurface.inc:670/:757,
  Gen3SharedPokemonSurface.inc:1709/:1801/:2238, Gen2UnifiedPokemonWorkspace.inc:888,
  Gen2HardwareFinalFix.inc:151; add scrollbars to the classic Add Item picker, Gen IV/V add pickers,
  ItemsPanel and the species picker through the same component (coordinate the item pickers with I)
- `include/UI/ScreenChrome.h` `drawScrollbar` only; `include/Utils/Keyboard.h` `textChanged` hook
- Tests: `tests/test_species_search_gen15.cpp`, `tests/test_shared_scrollbar.cpp`

**Lane R**
- `include/UI/Gen12DVReroll.h`, `include/UI/SharedStatReroll.h`, `include/Conversion/Gen3PidSearch.h`,
  `include/Integration/Gen1/Gen1Shiny.h`, `include/Integration/Gen5/Gen5PidCoordination.h`
- `src/Pokemon/Pokemon3FRLG.cpp` `rerollPID`/`rerollPreservingShiny`; `src/Pokemon/Pokemon4Mutable.cpp` `rerollPid`/shiny setter
- New: `tests/test_reroll_mechanics_gen15.cpp` (≥100 rerolls per representative shiny/non-shiny case per
  generation; Gen II DV constraints, gender, Unown; Gen III–V PID/TID/SID), `docs/REROLL_MECHANICS.md`
- Must NOT edit the editor `.inc` files. If a call site must change, send the diff to C (Gen I/II) or S (Gen III–V).

**Lane 2R**
- `src/UI/SaveSelectScreen.{cpp,h}`, `src/UI/BackupSelectionScreen.cpp` (except P's scrollbar lines),
  `include/UI/HomeQuitConfirm.h`, `include/UI/ScreenChrome.h` except `drawScrollbar`, `src/UI/SystemIcons.cpp`,
  `src/UI/UI.cpp`, `include/Source/*`, `src/Legacy/RetroArch*Discovery.cpp`, `src/Integration/Gen4/Gen4SourceDiscovery.cpp`,
  `src/Integration/Gen5/Gen5SourceDiscovery.cpp`, `.github/workflows/*`, `docs/qa/CANDIDATE_UNDER_AUDIT.env`
- New: `include/UI/SourceBackupClassification.h` (OriginalSource read-only / AppBackup deletable-with-Y /
  StagedWorkingCopy / ExportedCopy), tests `tests/test_emulator_backup_access.cpp` (RetroArch, DraStic, melonDS)

**Lane QA**
- `tests/test_*.py` contracts, `tests/test_qa_matrix*`, `tools/qa/*`, `docs/qa/*` except the env file,
  `docs/hardware/*` round-2 retest doc (with the coordinator), `Makefile.host.qa`
- New: `tests/test_hw_round1_regression_gate.py` mapping each of the 42 PASS items to the automated check
  that guards it; `docs/hardware/ROUND1_RESULTS_7fea1797.md` (the owner's 42/6/6 as reported).

## Integration order (2R)

1. S first push (geometry + field-state headers) → merge → all lanes merge forward.
2. P scrollbar component + call sites (many files, small line-local edits) → merge early → all lanes merge forward.
3. I, C, R, P search, S parity, 2R's own phases 6/7, QA gate — merge as each lane reports green.
4. QA 46c5a0b (all CG enforced) merges now that round 1 results are in.
5. Full gates on the integrated head, independent exact-binary audit, ONE candidate NRO, retest doc
   mapped to B01–B06, S01, S03, B04, E03, E11, L06, S04, S06 and the 42 PASS items.
