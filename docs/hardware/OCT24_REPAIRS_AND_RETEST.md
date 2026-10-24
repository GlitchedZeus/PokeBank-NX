October 24 repair session — detailed engineering record

Starting application SHA: `92f956120019673733bd8d65cd65ab33c46cc237`.
Final application SHA: `7d661fa33689681a0cb741dbaa3407ca79990960` (docs-only candidate binding).
Validated production checkpoint: `52053c3b38904aac6cbaa4abe05b888fca731405`. Application and binding identities are recorded below.
Implementation and host validation record. The official candidate identity below was recorded only after the build succeeded and its exact downloaded bytes passed local audit.


Official October 24 hardware candidate (one selected deliverable):

- Application code: `8804f8f819a0da34c5d7b983a92bfb3a390eff16`.
- Source tree: `66c321b13ce734239271ff0b9e924ca43cdaf92f`.
- Filename: `PokeBank-NX-Gen1-5-Candidate-8804f8f8.nro`.
- SHA-256: `5bbc090e186b624dcad8a9fcbc6c87a087254b6e737d157c3616309878536085`.
- Size: 176783025 bytes.
- [Official build 40099575742](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40099575742) PASS.
- [Download the selected official artifact](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40099575742/artifacts/12562103597).
- Local exact-file audit PASS: single NRO, GitHub run/artifact records, BUILD_IDENTITY/SHA256SUMS, embedded `8804f8f8`, NACP, source tree and bytes agree. Packaged RomFS PASS: 4,267 files,156.6 MiB, content digest `bc09c152ac4232a2`.
- `docs/qa/CANDIDATE_UNDER_AUDIT.env` is bound in a docs-only commit after the test-only polish guard correction. No application file changed after the selected code SHA. Independent [QA-F9 audit40100073045](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073045) reports NRO AUDIT:PASS on docs-only binding `7d661fa33689681a0cb741dbaa3407ca79990960`. All 13 final-head workflows /17 checks PASS; exact run links are recorded below.
- **NOT DEVICE TESTED**. The build and audit do not change the historical hardware result mapping.

Original hardware evidence remains 27 PASS / 6 FAIL / 4 unmarked. No new code or host result is a hardware PASS. S06 NOT TESTED.

| Checklist evidence | Production outcome | Hardware state |
| --- | --- | --- |
| B08, R2-BUG-002, NEW-19, AUD-01 | Gen I–V use the accepted shared Review and four-action staged exit; Gen V Trainer Y/Review button is accessible. Review rows retain named moves/items and real native diffs. | Retest required |
| R2-EMUBACKUP, C07, S05 | Fullscreen common backup rows, newest first, exact immutable selected-copy Gen I–V sessions, recovery history when live assignment is missing, verified separate export; stale/symlink/traversal rejection; protected deletion. Differing internal Gen V copies require Primary/Backup choice. | Retest required |
| R2-TRAINER, R2-BUG-002, NEW-18 | Native badges throughout, 16 Johto/Kanto for Gen II/HGSS, Platinum native facts, consistent gold read-only values and Help. Existing safe Name/Money and Gen V Time writers preserved. | Retest required |
| E05 | Descriptive classic Help pages through existing colored glyph renderer and B-close paths. | Retest required |
| NEW-06 | Docked Species keyboard beside two-column Held Item results and artwork; folded live filtering, sort, controller/touch, no-match blocking. | Retest required |
| NEW-10, NEW-13 | 620px Met Location / Ability windows; source context, names/effects, scroll gutter, native IDs and restrictions preserved. | Retest required |
| R2-REVIEWMOVE | Gen III Create separates native held-item support from Bag addability. Safari Ball is supported without bypassing caught-ball validation; test crosses all 12 caught balls and five held choices on five titles. | Retest required |
| M05 | Exact BW/B2W2 + native form move pools, version/form restrictions, transfer/prior-form labels and preserved existing unknown moves. Pikachu has 55/64 direct moves; Smeargle remains the genuine Sketch exception. | Retest required |
| L06 | Release-confirmed detail tap reveals the field before activation, matching controller A for IV/V Pokérus. | Retest required |
| R2-CHECK-011 | Native event-year labels (2010/2011/2013 etc.) and honest provenance text: exact distribution/dates unverified. | Retest required |
| R2-SPECIAL | Existing source patch verified: shared Special DV/Stat Exp, separate SpA/SpD in the Calculated column; stale old layout test corrected. | Existing patch; retest required |
| NEW-14 | Existing inline supported-language cycle verified in actual shared source and behavior tests; named Review retained. | Existing patch; retest required |
| R2-REROLL | Existing DV-derived Unown preview/record mechanics verified; random-DV tests preserve shiny/gender/form. | Existing patch; retest required |
| NEW-09 | Approved green/red Nature effects preserved and protected. | Existing behavior; regression retest |
| NEW-16, C01 | Assigned Gen V direct-open path preserved; C01 remains historically unmarked despite host coverage. | Historical mapping preserved |

Limits: older Trainer Gender/Time remains read-only where no validated source-backed writer exists; no Gen I gender invented. Native Switch historical backups open immutable read-only previews; Gen I–V emulator archives support staged edits/Review/Discard/Export. Full encounter/event legality remains incomplete; PR314 is untouched.

Historical owner status mapping:

- PASS: E03, L06, R2-TRAINER, C07, M04, B08, E02, E05, R2-BUG-002, R2-CHECK-011, NEW-02, NEW-03, NEW-04, NEW-05, NEW-06, NEW-07, NEW-08, NEW-09, NEW-11, NEW-12, NEW-13, NEW-15, NEW-16, NEW-17, NEW-18, NEW-19, NEW-20.
- FAIL: R2-SPECIAL, R2-REVIEWMOVE, M05, S05, NEW-10, NEW-14.
- UNMARKED / TRACKED: R2-EMUBACKUP, R2-REROLL, C01, AUD-01.


Validation commands (all run from the repository, with separate fresh compiler build directories):

| Command | Result |
| --- | --- |
| `make -f Makefile.host HOST_BUILD=/tmp/oct24-gcc -j3 host-test` | PASS, 202 host binaries |
| `make -f Makefile.host HOST_BUILD=/tmp/oct24-clang CXX=clang++ -j3 host-test` | PASS, 202 host binaries |
| `ASAN_OPTIONS=detect_leaks=1 make -f Makefile.host HOST_BUILD=/tmp/oct24-gcc-sanitize -j4 host-sanitize` | PASS, 202 ASan/UBSan binaries, leak detection enabled |
| `ASAN_OPTIONS=detect_leaks=1 make -f Makefile.host HOST_BUILD=/tmp/oct24-clang-sanitize CXX=clang++ -j4 host-sanitize` | PASS, 202 ASan/UBSan binaries, leak detection enabled |
| `make -f Makefile.host HOST_BUILD=/tmp/oct24-gcc -j2 qa-contracts qa-matrix ASAN_OPTIONS=detect_leaks=1` | PASS, plain and sanitizer; 489 cells, 0 problems |
| `make -f Makefile.host HOST_BUILD=/tmp/oct24-clang CXX=clang++ -j2 qa-contracts qa-matrix ASAN_OPTIONS=detect_leaks=1` | PASS, plain and sanitizer; 489 cells, 0 problems |
| `python3 tests/test_qa_cross_gen_contract.py --candidate --list` | PASS, 0 contracts failing |
| `python3 tests/test_hw_round1_regression_gate.py` | PASS, 70 Round 3 checks / 196 guards |
| `python3 tests/test_round4_shared_contract_mutations.py` | PASS, 26 deliberately broken paths rejected |
| `python3 tests/test_round4_backup_contract_mutations.py` | PASS, 4 deliberately broken paths rejected |
| `python3 tests/test_ci_repository_safety_contract.py` | PASS |
| `python3 tests/test_touch_appwide_surface_contract.py` and `python3 tests/test_touch_pokemon_editor_direct_rows_contract.py` | PASS |
| `python3 tests/test_verify_nro_romfs.py`, `python3 tests/test_device_asset_png_validation.py`, asset regression/known-failure controls | PASS |
| `python3 tests/test_v1_polish_contract.py` and the GCC CI pre-build recovery/species/font/authority checks | PASS; footer guard follows Y-only deletion and shared Open/Back controls |
| `make -f Makefile.host HOST_BUILD=/tmp/oct24-gcc qa-tools-test` | PASS, complete audit/mutations/header-dependency/tooling suite |
| `git diff --check` | PASS |
| Native devkitA64 compile/link/packaged RomFS, workflow run 40075492367 at production checkpoint52053c3b | PASS |

The QA matrix covers 20 exact selected-backup sessions, differing Gen V internal-copy choices, and all 18 C1–C6 export paths. Its three reported cross-generation fact differences are expected native capability, identity source and cache implementation differences; there are zero problems. Original/archive bytes, filesystem identity and timestamps stay unchanged; edits export to a different validated file. The Gen III Create regression covers five titles × twelve caught balls × five held-item choices (300 cases), plus invalid ball/item rejection. Ability rendering checks all 164 native abilities against pinned real-font metrics, including full effects and Hidden labels.

Validation corrections: older exact-source assertions were updated to follow the newly requested shared renderers and controls, while preserving locked identities, Y/B confirmations, deletion scope, native ID and immutability checks. A missing overload was added to the isolated renderer mock only. A concurrently compiled Clang test retained an obsolete `+` footer assertion; rebuilding that generated binary tests the correct `Y Review` source. No production assertion was disabled. Direct QA matrix invocation initially lacked the required fixture environment; corrected explicit fixture invocations passed plain and sanitizer variants.

Source evidence: pinned PKHeX6042efa241a9925ce57e645329ea7f976deabdd3 and PokeAPI168b1e89467054cda2e7df43ccebbb69b459497a; exact paths, hashes and generator inputs are in `docs/data/gen5-move-compatibility-sources.json`. Native event labels identify the game's year marker only, without inventing distribution provenance. The separate legality PR314 is untouched.

Production and build/data-tool files changed (63, automatically enumerated against the starting SHA):

- `Makefile.host.gen5`
- `Makefile.host.qa`
- `Makefile.host.round4-codex`
- `Makefile.host.shared-editor`
- `include/Integration/Common/NativeBadges.h`
- `include/Integration/Gen1/Gen1ReadOnlySave.h`
- `include/Integration/Gen2/Gen2ReadOnlySave.h`
- `include/Integration/Gen3/Gen3HeldItems.h`
- `include/Integration/Gen3/PKSMGen3Adapter.h`
- `include/Integration/Gen4/Gen4ReadOnlySave.h`
- `include/Integration/Gen5/Gen5MoveCompatibility.h`
- `include/Integration/Gen5/Gen5MoveFormData.inc`
- `include/Integration/Gen5/Gen5TrainerScreen.h`
- `include/Legacy/BackupReadOnlyTrainer.h`
- `include/Names/LocationDisplayNames.h`
- `include/Safety/SourceMutationPolicy.h`
- `include/Source/ImmutableSourceSnapshot.h`
- `include/Trainer/Trainer.h`
- `include/UI/BackupBrowserChrome.h`
- `include/UI/BackupHistoryPresentation.h`
- `include/UI/BackupSelectionScreen.h`
- `include/UI/ClassicHelpPresentation.h`
- `include/UI/EmulatorBackupLayout.h`
- `include/UI/Gen2HeldItemPicker.h`
- `include/UI/ItemPickerSearch.h`
- `include/UI/SaveSelectScreen.h`
- `include/UI/SharedAbilityPicker.h`
- `include/UI/SharedEditorChrome.h`
- `include/UI/SharedHeldItemPicker.h`
- `include/UI/SharedHeldItemPickerPresentation.h`
- `include/UI/SharedHeldItemSearch.h`
- `include/UI/SharedMenuGeometry.h`
- `include/UI/SharedPokemonEditorWorkspace.h`
- `include/UI/SharedReviewPresentation.h`
- `include/UI/SourceBackupClassification.h`
- `include/UI/SourceBackupOpen.h`
- `include/UI/UI.h`
- `src/Integration/Gen1/Gen1ReadOnlySave.cpp`
- `src/Integration/Gen2/Gen2ReadOnlySave.cpp`
- `src/Integration/Gen3/FRLGReadModel.cpp`
- `src/Integration/Gen3/Gen3StagedPokemonEditor.cpp`
- `src/Integration/Gen4/Gen4ReadOnlySave.cpp`
- `src/Legacy/BackupReadOnlyTrainer.cpp`
- `src/Legacy/FRLGReadOnlyTrainer.cpp`
- `src/Legacy/GSCReadOnlyTrainer.cpp`
- `src/Legacy/Gen4ReadOnlyTrainer.cpp`
- `src/Legacy/Gen5ReadOnlyTrainer.cpp`
- `src/Legacy/RBYReadOnlyTrainer.cpp`
- `src/UI/BackupSelectionScreen.cpp`
- `src/UI/Dialogs/SaveConfirmDialog.cpp`
- `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc`
- `src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc`
- `src/UI/Gen2PokemonEditorFoundation.inc`
- `src/UI/Gen2SharedPokemonSurface.inc`
- `src/UI/Gen2SharedSurfaceParity.inc`
- `src/UI/Gen3SharedPokemonSurface.inc`
- `src/UI/Gen4SharedPokemonSurface.inc`
- `src/UI/Gen5SharedReadOnlySurface.inc`
- `src/UI/SaveSelectScreen.cpp`
- `src/UI/SharedPokemonEditorSurface.inc`
- `src/UI/TrainerViewScreenBase.inc`
- `src/UI/UI.cpp`
- `tools/gen_gen5_move_compatibility.py`

Regression test/support files added or repaired (35):

- `tests/test_ability_effects.cpp`
- `tests/test_backup_namespace_contract.cpp`
- `tests/test_backup_session_trace.cpp`
- `tests/test_backup_session_trace.h`
- `tests/test_backup_snapshot_open.cpp`
- `tests/test_classic_stat_field_state.cpp`
- `tests/test_emulator_backup_layout.cpp`
- `tests/test_gen12_review_parity.cpp`
- `tests/test_gen3_staged_pokemon_editor.cpp`
- `tests/test_gen4_hardware_surface_contract.cpp`
- `tests/test_gen5_create_edit_parity.cpp`
- `tests/test_gen5_move_compatibility.cpp`
- `tests/test_gen5_readonly_ui_contract.py`
- `tests/test_gen5_trainer.cpp`
- `tests/test_gsc_workspace_render.cpp`
- `tests/test_hw_round1_regression_gate.py`
- `tests/test_item_picker_search.cpp`
- `tests/test_native_trainer_badges.cpp`
- `tests/test_nature_picker_layout.cpp`
- `tests/test_oct24_picker_help_events.cpp`
- `tests/test_qa_cross_gen_contract.py`
- `tests/test_qa_matrix.h`
- `tests/test_qa_matrix_gen12.cpp`
- `tests/test_qa_matrix_gen3.cpp`
- `tests/test_qa_matrix_gen4.cpp`
- `tests/test_qa_matrix_gen5.cpp`
- `tests/test_round4_shared_contract_mutations.py`
- `tests/test_shared_boxed_rows.cpp`
- `tests/test_shared_editor_contract.cpp`
- `tests/test_shared_editor_unit.cpp`
- `tests/test_shared_menu_geometry.cpp`
- `tests/test_source_mutation_policy.cpp`
- `tests/test_touch_appwide_surface_contract.py`
- `tests/test_touch_pokemon_editor_direct_rows_contract.py`
- `tests/test_v1_polish_contract.py`

October 24 candidate — Switch retest checklist

NOT DEVICE TESTED. Install `PokeBank-NX-Gen1-5-Candidate-8804f8f8.nro` and confirm Settings shows build `8804f8f8`; compare the full hash and size in the identity record above.
Historical results remain 27 PASS, 6 FAIL and 4 unmarked. S06 NOT TESTED.

1. Review / exit (B08, R2-BUG-002, NEW-19, AUD-01): on each generation stage a named move/item or Money change. Open Review from Pokémon/Bag/Trainer and Actions. Check the same four-row pages, names and before/after values. Leave: A Review, B Continue, Y Discard & Exit, X Export Copy. Review Y asks before Discard All; B cancels. Include empty Review and enough changes for a second page.
2. Backups (R2-EMUBACKUP, C07, S05): open the full-screen list. Check game/provider, readable dates/types, newest-first history and protected legacy/unknown ownership. Open an older copy with visibly different Money. Stage Money, export separately, Discard and reopen the archive: its original Money must remain. Open the export: edited Money must appear. Gen V archives with two differing native copies must offer Primary / Backup / Cancel. Native Switch historical copies are read-only previews.
3. Trainer (R2-TRAINER, NEW-18): compare native Name, Money, Gender where stored, Play Time, Badges and Dex with the game. Gen II/HGSS must show Johto x/8 and Kanto y/8. Include Platinum. Gen I has no Gender row. Gold informational values stay read-only; Name/Money and Gen V Time use existing safe staged edits. Y/Review must work on every Trainer card.
4. Compact pickers/search/Help (E05, NEW-06, NEW-10, NEW-13): compare narrow Location/Ability windows in Gen III–V, long names/effects, source context, scrollbar and A/B/touch. Held Item Minus opens keys beside results/artwork. Type POKE/POTION, clear/backspace, test no match, B to close keys, X sort and touch selection. Help must explain actions and close on B.
5. Gen II Special / Unown (R2-SPECIAL, R2-REROLL): check one Special DV and one Stat Exp with separate calculated SpA/SpD in their original column. For Unown, chooser portrait, kept Pokémon and DV-derived letter must agree. Check shiny controls and Y rerolls preserve consistent native combinations.
6. Gen III Create (R2-REVIEWMOVE): Pikachu, Safari caught Ball, Keep; also try Safari as Held Item and native Hyper Potion. Named values must appear in Review; export and reopen. Repeat on at least FRLG and one RSE title.
7. Gen V Moves / Language (M05, NEW-14): Pikachu must not receive the arbitrary 559-move catalog. Learnable / Transfer / Prior form / Preserved have different meanings; compatibility is not full legality. Compare Wormadam cloaks, Rotom appliances and B2W2 White/Black Kyurem. Language A and a completed tap must cycle inline, remain staged and show a named Review change.
8. Gen IV Pokérus (L06): tap its visible Details row, including after scrolling; it must perform the same action as A. A swipe must only scroll.
9. Black 2 events (R2-CHECK-011): inspect native event entries for 2010, 2011 and 2013. Year markers may be named; actual distribution and exact dates must remain unverified without evidence.
10. Regression: retain Actions order, Bag Add/Remove confirmations, Party actions, Gen II Move, assigned Gen V Home opening, zero Create IV/DV defaults, species label/search, ball gutter, Origin & Legality wording and green/red Nature effects. C01 stays historically unmarked; record its new result separately.

Report PASS / FAIL / NOT TESTED by checklist ID with exact NRO SHA and any reproduction steps. Host/CI results never fill in a hardware PASS.

Final integration and CI record:

Final PR307 application head / docs-only binding: `7d661fa33689681a0cb741dbaa3407ca79990960`. All **13 workflows /17 checks PASS** on this exact head. Completed GCC and Clang full-job logs reach the final tests and contain no ASan/UBSan/leak or runtime-error diagnostics.

| Workflow | Result |
| --- | --- |
| [Gen I/II Packed Move Focused](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073017) | PASS |
| [Gen I/II Packed Multi-Move Focused](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100072948) | PASS |
| [PokeBank NX App QA Contracts](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073031) | PASS |
| [PokeBank NX App QA Fuzz](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073129) | PASS |
| [PokeBank NX App QA NRO Audit](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073045) | PASS |
| [PokeBank NX App QA Workflow Hygiene](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073109) | PASS |
| [PokeBank NX Gen I-IV Hardware Candidate](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073057) | PASS |
| [PokeBank NX Gen I-V Integrated Candidate](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100072981) | PASS |
| [PokeBank NX Gen V Focused Host](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100072951) | PASS |
| [PokeBank NX Host Tests](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073074) | PASS |
| [PokeBank NX Host Tests (Clang)](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100073020) | PASS |
| [PokeBank NX Native PR Gate](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100072950) | PASS |
| [Touch Input Gating](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40100072953) | PASS |

The selected hardware file is the original bound8804f8f8artifact. Later automatic CI builds are not additional hardware deliverables. No application file changed between that candidate and the final binding head. PR307 remains draft on `claude/gen14-final-20261014`; main and PR314 were not modified. The final handoff is checkpointed separately on WIP as documentation only so the passing application head remains frozen.
