# Gen I–IV completion campaign (Thread C) — checkpoint

Branch: `claude/gen14-campaign-20261015`, created from `c952565c` (PR #307 head), forward-merged
with #307 `44a976e1`. Integrated into #307 by Thread 2R (merge only).

Every NRO built from this branch is **NOT DEVICE TESTED**.

## Gates (per push)

GCC and Clang `host-test` + `host-sanitize` (touched targets each unit, full suites per
milestone), every `tests/*.py` contract, `git diff --check`.

## Units

PR #316 (units up to cd2bbbff) was merged into #307. Later units are on draft PR #318.
Each new test fails against the previous code unless marked "pinning".

| # | Item | Commit | Tests |
|---|------|--------|-------|
| 1 | QA-F3b: Gen II export publishes with exclusive mkdir | 0bac3cf1 | `test_gsc_export_transaction` |
| 2 | PR #280 Gen III/IV empty-slot touch/A parity (pinning) | e2e9e0c7 | `test_gen3/gen4_hardware_surface_contract` |
| 4a | Gen I verified export transaction; foreign Bag splice refused | e0ef916a | `test_gen1_export_transaction` |
| 3a | Gen III PID-linked fields never reroll or flip silently | 8d7609f5 | `test_gen3_shared_pokemon_session` |
| 3b | Gen I/II two-step Review discard; Gen I modal exit footer; stick handoff; Gen II Species tap | 34c084a1 | `test_gen12_review_parity` |
| 4b | Phase 4 Gen II/III/IV Party/Box/Bag export separation (pinning) | 27940e54 | gsc/gen3/gen4 editor tests |
| 5a | Gen I/II audit lows | 1ca0d8b3 | `test_gen12_review_parity` |
| C1 | Gen II Actions -> Clone ownership (authorised `ClassicReleaseActionFix.inc`) | dd96d59f | `test_gen2_clone_flow` |
| C2 | Y Random DVs confirmation (Gen I/II) | 81541ea1 | `test_gen12_reroll_confirm` |
| D1 | Gen IV bag pouch layout per PKSM-Core (DP Battle overrun, HGSS TM offset) | cd2bbbff | `test_gen4_readonly_foundation` |
| D2 | Gen I current HP follows max HP; Gen II PP cap, OT length, unchanged name bytes | a99aa8df | `test_rby_pokemon_editor`, `test_gsc_pokemon_editor` |
| Q6 | QA-F6 Gen III release hint needs an isolated stem/folder token | 82032a9e | `test_retroarch_frlg_discovery` |
| G3 | Gen III PID gender accessor `PokemonRecord::gender()` (for 2R) | a48c98fe | `test_gen3_gender` |
| Qa | Gen IV Unown's 28 stored letters editable | (in c7b0f978 series) | `test_gen4_mutable_pokemon` |
| Qb | Gen IV review lines never show "(empty) (edited)" | c7b0f978 | `test_gen4_staged_pokemon_editor` |
| D3 | Un-nicknamed Pokemon default to the retail upper-case name | 965dee24 | rby/gsc/gen3 editor tests |
| V2 | X Keep in Gen I/II Edit | 88e53929 | `test_gen12_reroll_confirm` |
| V3 | Gen I View navigable/focusable, read-only | d5d78e8e | `test_gen1_view_navigation` |
| V5 | Gen II Pokerus / Met Time / OT Gender selectors | cea1da09 | `test_gen2_field_selector` |
| V7 | Picker paging matches the shared editor | 2849071b | `test_gen13_picker_paging` |
| V4 | Gen I Edit working-copy draft (Keep stages, Discard drops) | 74d4b16f | `test_gen1_edit_draft` |
| V6 | Gen I panels use the shared shell geometry; shell assessment below | ce82066f | static_asserts in the Gen I workspace |
| A1 | Gen IV review lists Bag changes (bag-only edits now have review lines) | 2b2be00f | `test_gen4_staged_pokemon_editor`, QA matrix review cell (7dec0c78) |
| A2 | Gen III Japanese name lengths (5/5); Gen II/III strain-0 Pokerus with days refused | c934a945 | `test_gen3_staged_pokemon_editor`, `test_gsc_pokemon_editor` |
| A3 | QA-F8 export half: Gen IV Release restores a slot's original empty record (clone+release is byte-identical) | f883f4d0 | `test_gen4_staged_pokemon_editor`; `qa-known-failures` F6/F7/F8 all fixed |
| A4 | Gen IV Castform/Cherrim: only stored form 0 is writable; an existing battle form is shown and kept (PKHeX 6501f0ab FormInfo.BattleForms, FormConverter) | 7e36220f | `test_gen4_mutable_pokemon` |
| A5 | Gen IV export: optional Open snapshot of the source file (raw or .dsv); export fails closed if size/mtime/SHA-256 changed or the payload no longer leads the file | 768c65fd | `test_gen4_staged_pokemon_editor` (synthetic .dsv fixture) |
| A6 | Gen IV export backs up the source FILE verbatim (`.dsv` footer kept) with SOURCE_CONTAINER / EDITED_CONTAINER / SOURCE_SHA256_SCOPE / ORIGINAL_BACKUP_IS keys matching Gen V (Common fields d9cdcf78) | 40f78f3c | `test_gen4_staged_pokemon_editor` |
| A7 | `.dsv` backup named `original_backup.dsv` (edited stays `edited.sav`, raw), via shared `editedFileExtension` (0c2fffb6) | this commit | `test_gen4_staged_pokemon_editor` |

## Owner campaign (2026-10-16, Gen I/II lane)

| Phase | Unit | Commit | Tests |
|---|---|---|---|
| #320 | Gen II Create/Edit: one input owner per frame; exit dialog footer is its own | ae099c77 | `test_gen2_workspace_layers` |
| 3 | Gen I/II Actions sheet: real commands only (View, Edit, Clone, Release, ...); no Make Shiny | dcd8770c | `test_classic_pokemon_action_menu` |
| 6 | Y re-rolls DVs immediately; keeps shiny, Gen II gender, Unown letter | dcd8770c | `test_gen12_dv_reroll` |
| 10 | Gen II Shiny Yes/No; Pokerus A-cycles in place (original byte restored) | e59df023 | `test_gen2_inline_fields` |
| 9 | Gen II DATA pane text wraps/clips inside its pane | e59df023 | `test_gsc_workspace_render` |
| 11 | Crystal Met Location chooser proportional, names only; Gen I has no met location | 47cac826 | `test_gen2_location_picker` |
| 1/5 | Gen I list editors: whole rows above status line; no Row x/y counters | 9d1d03f9 | `test_gen1_list_fit` |
| 2 | Bag Remove asks first (Y/B); one per-item quantity query | 3dcce1fd | `test_classic_inventory_ui_model` |
| 4/7 | "Origin & Legality" label; shared picker footers (pickerFooter); shared Pokerus rule | 4203ce0d + 6e900e27 (native fix) | `test_gen2_inline_fields`, route contracts |
| 1 | Gen I/II moves on the shared Moves panel (UI/ClassicMovesPanel.h); native max PP; touch == outlined rows; legality row in DATA | ddd52f55, c7355c57 | `test_gen2_moves_panel` |
| 5 | Gen I/II Actions sheet forwards to S's NarrowPokemonMenu::classic/gen1 | 5d9dc8e5 | `test_classic_pokemon_action_menu` |
| item 1 | Classic quantity rule delegates to QuantityPolicy (FRLG 999, RSE 99/999, Gen III PC 999) | 28707782 | `test_classic_inventory_ui_model`, `test_gen3_staged_inventory` |
| 1 | Gen I/II move choosers on the shared chooser rows (pret move types) | 307976f3 | `test_gen2_moves_panel` |
| item 6 | Gen II data pane on SharedDataPane::layoutRows | see git log | `test_gsc_workspace_render` |
| — | Gen I Create default #25 Pikachu L5 (Thunder Shock, Growl) | see git log | `test_rby_pokemon_editor`, qa matrix gen12 |

Note: 4203ce0d alone fails the native build (SharedEditorChrome.h first included under the composite's
`drawFooter` rename); 6e900e27 fixes it with push/pop_macro.

Remaining: owner detail on the "hidden row" in Gen I Create.

Open / reported:
- Coordinator one-liner: pass held/stick to `handleFoundationPassiveViewInput` (Gen I View stick parity).
- Coordinator wording for `SaveSelectScreen.cpp:914-915` (QA-c).
- Diamond vs Pearl not checked against the save (no pinned reference distinguishes them).

## Item 6 assessment: Gen I/II and SharedPokemonShell

- **Gen I panels — done (no pixel change).** DETAILS/STATS/MOVES now take their rectangles from
  `SharedPokemonShell::Geometry`. The previous Gen I constants were already identical
  (y 96; x 24/338/750; widths 300/398/rest), and static_asserts pin them.
- **Gen I header — not switched; needs a small shared change.** It differs from `drawChrome` in
  three ways:
  1. Gen I has no gender. `drawChrome` places the shiny star at name+66 to leave room for a gender
     glyph; Gen I draws it at name+40.
  2. The subtitle: Gen I calls `contextLine(record, exactGameName, mode)` with a display name,
     where `drawChrome` passes the game id. The rendered text must be compared on screen before
     switching.
  3. Dex label formatting is identical.

  Switching now would move the shiny star 26 px on a hardware-accepted screen. Proposal for 2R:
  `drawChrome(..., bool genderSlot = true)` so Gen I can call it with no visual change. After
  that, Gen I's header is the shared one.
- **Gen I/II panel contents — keep format-specific.** Gen I has 6 DETAILS rows, a DV / Stat Exp /
  Stat grid with derived HP DV, one Special and a Shiny row, and 4 moves. Gen II has the shared
  chrome and geometry already, plus DV-derived gender/shiny, a shared Special DV/Stat Exp, and
  Crystal caught data. Neither maps onto the shared IV/EV/Nature/Ability/PID rows without losing
  the native presentation the owner keeps (DV/Stat Exp, single Special, no Nature/held item in
  Gen I). Recommendation: keep these panels and share chrome, geometry, focus, pickers and
  modals, which is now the case except for the Gen I header above.

## Verification log

- Full GCC + Clang `host-test` + `host-sanitize`: PASS on the c934a945 tree.
- Full GCC + Clang `host-test`: PASS at c7b0f978. GCC + Clang `host-sanitize`: PASS on the
  f5db8074 merge tree.
- Every unit: touched targets GCC+Clang, plain + ASan/UBSan; all `tests/*.py`; `git diff --check`;
  native devkitA64 (pinned image) build.

## Next

Coordinator: C FINAL head goes into the candidate NRO; add nothing after it unless asked.
Party check (S 647917e): Gen I/II staged editors are box-only, so Party → View loses nothing.

Continue the Gen I–IV audit; Gen IV `.dsv` container once the owner decides; watch PR #318 CI.

## Round 3 (Lane C) — checkpoint

Base: #307 `0faf7e9c` (fast-forward). Branch `claude/gen14-campaign-20261015`.

| Item | Commit | State |
| --- | --- | --- |
| R2-BUG-002 Gen IV Trainer Name / Money (D, P, Pt, HG, SS) | e5dec3a2, 8dca7d07 | gated green |
| R2-BUG-002 Gen III Trainer Name / Money (R, S, E, FR, LG) | 02cf6d1a, ea1a7a4a | gated green |
| R2-BUG-001 Trainer footer on a read-only source (Gen I-IV) | cccec27b | gated green |
| QA Part 9 Gen IV Storage save-pane Move / Y | c6f9011c, reverted | moved to Lane S (83e3d00, integrated); C copy reverted to avoid a ClassicPackedMoveOverlay conflict |
| R2-CHECK-009 Yellow Box Actions (no Move / Review) | this unit | coordinator ruling 2026-10-19: Move added to Gen I Box Actions |
| R2-BUG-008 Ruby Pikachu Lv. 5 Met Location | this unit | see below |

### R2-CHECK-009 — findings and fix

- Gen I Move was supported (the Boxes-view hold-Y carry, ClassicPackedMove beginGen1) but absent
  from Box Actions, while Gen II-V list it in the shared E02 order. Per the owner directive
  ("repair the actual action routing" when supported but absent), Gen I Box Actions now lists
  View, Edit, Clone, Move, Release, Origin & Legality (+ Review while staged changes exist),
  Cancel. Move starts the SAME Boxes carry with the Pokemon picked up
  (ClassicPackedMove::beginGen1FromAction): Y drops, B cancels.
- No Move in the read-only Storage pane or on an empty slot (actionsForSlot boxMove = false);
  every sheet / hit-test / dispatch uses the same flag, so rows, touch ids and actions line up.
- "Review Pending Changes" appears only while staged changes exist (same rule as Gen II-IV); the
  hardware screenshot had nothing staged.
- Test: test_gen12_packed_move (now in host-test): the sheet order with / without Move, Move ->
  drop stages one verified move of the same record, B leaves the staged box byte-identical, and
  the production routing. QA owns the E02 Gen I literal (test_classic_pokemon_action_menu); its
  default (Storage / boxMove false) sheet is unchanged.

### R2-BUG-008 — findings

- Data is correct: in Ruby / Sapphire / Emerald, Pikachu's only exact-game template is the Hoenn
  Safari Zone (location 57), Wild Lv. 25-27 (EncounterGuardrailsData). So a Lv. 5 / Met Lv. 5
  Pikachu did not come from a wild Hoenn encounter: it was traded in, hatched and evolved,
  received from an event, or has another origin the template data does not model. The repo
  cannot tell which, so it must not claim one.
- Demonstrated defects: (1) the picker listed only the Safari template and focused it as if it
  were the record's origin (selectedRow falls back to row 0 when the stored location is not a
  template); (2) choosing it silently rewrote Met Lv. 5 to 25 and raised the Pokemon to Lv. 25
  (setEncounterMetLevel), with nothing saying the template does not match the record.
- Fix: the stored location is offered first as "Keep stored: <location>, Met Lv. N as stored";
  choosing it changes nothing, and the picker opens on it. A template whose range excludes the
  stored met level is labelled "not stored Met Lv. N". Templates stay choosable (a deliberate
  re-origin is still an edit the user can make); no origin is judged legal or illegal here.
- Test: test_gen3_encounter_keep_stored (real R / S / E template data).

## Round 4 (Lane C) — checkpoint

Base: #307 `c2287f50` (fast-forward). Branch `claude/gen14-campaign-20261015`.

| Item | Commit | State |
| --- | --- | --- |
| NEW-04 (P0) Gen II PC box Move fails at pickup | c3807c63 | gated green (GCC / Clang host-test + sanitize, qa x2, qa-tools, py contracts, CG 0 failing, native) |
| NEW-19 + R2-TRAINER + R2-BUG-002 staged exit menu (Gen I-IV) | — | next |
| R2-REVIEWMOVE readable Review rows | — | pending |
| NEW-18 Play Time / Pokedex seen-caught | — | pending |

### NEW-04 — root cause and fix

- Pickup: ClassicPackedMove beginGen2 -> StagedEditor::beginPackedMove -> refreshGen2 ->
  GSCReadOnlyTrainer::refreshStagedBoxPresentation, which rebuilt the grid from finalizedBytes().
  finalizedBytes() refuses while a carry is active ("Place or cancel carried Pokemon before
  finalizing"), so every pickup failed its refresh and the carry was cancelled. The editor-level
  move was already correct (test_gen12_packed_move).
- Fix: StagedEditor::presentationBytes() (staged image repaired for a strict reload, display
  only, never exported); the grid uses it while a carry is active. finalizedBytes() still refuses
  mid-carry, so nothing can be exported with Pokemon in hand.
- Test: test_gen2_box_move_presentation (Gold, Silver, Crystal; real GSCReadOnlyTrainer): pickup,
  same-box drop, other-box drop, cancel, finalize + verified export, source unchanged. Fails
  without the fix.

## Round 6 (Lane C) — checkpoint

Base: #307 `a96a9559` merged forward. Branch HEAD `cd6d89ff` (verified with ls-remote).

| Item | Commit | State |
| --- | --- | --- |
| P2 / R2-TRAINER / NEW-18 Trainer Info in Gen V design, no overlap | eb0ae8ee | integrated (#307 1dd0a338) |
| 7A Gen II Special SpA/SpD on the STATS grid | e39a92e2 | integrated (#307 a96a9559) |
| CG72 Gold / Silver report no trainer gender; XFAIL dropped | d2a7f7d9 | gated green |
| P3 Gen I/II Create Review details (CG74, lane S shape) | f082ac1d | gated green |
| "Native touch contracts" fix: GSC surface contract builds without -Iinclude | 1208301b | gated green |
| Gen II Create seeds native level-up moves (seedNativeMoves, R2-BUG-004 data) | 48fbf94b | host-test x2, py, CG, native green; qa targets pending |

Gate for f082ac1d + 1208301b: GCC and Clang host-test and host-sanitize; qa-fuzz-smoke /
qa-contracts / qa-matrix for both compilers; qa-tools-test; py contracts (fail=0); cross-gen
contract (78 PASS; CG74 keeps only the Gen III / Gen V violations, which other lanes own);
`git diff --check`; native objs + NRO. NOT DEVICE TESTED.

Remaining: nothing assigned. Not mine, reported: #307 head Clang host-ui build error in
`src/Save/GetSaveFileContents.cpp` (ambiguous `Trainer`).

## Round 7 (Lane C) — checkpoint

Base: #307 `6ec8dc53` merged forward (fast-forward). Owner results:
`docs/hardware/ROUND6_RESULTS_26186464.md`.

| Item | Commit | State |
| --- | --- | --- |
| P1 B06 / C06: Gen I-IV Trainer painted by Gen V's Trainer painter (UI/SharedTrainerPanel.h); neutral values; Gen V row heights; 200 x 36 Review in a measured slot | 7eab05cf | gated green, pushed, reported |
| P1 A22 / C01: Gen II Special like Gen I (plain STATS columns, lower-middle CALCULATED SPECIAL STATS block) | 0e58157b | gated green (with 4f549775) |
| Merge #307 ab46883e (HostUi recorders reconciled with S's d88056c) | 67c9b0f5 | gated green (with 4f549775) |
| Gen V Trainer on the same page model (measured Review slot, language line in the model's detail slot); CG83 / CG84 / CG90 XFAIL dropped | 4f549775 | gated green: host x2, sanitize x2, qa x2, qa-tools, py, CG 0 failing, native |

Tests: test_ui_layout_host compares the visible Gen I-IV Trainer frame against Black's (HostUi now
records text colour and rounded fills, additively) and checks the Gen II STATS capture against the
new layout; test_classic_trainer_page; test_gen2_special_stat. Re-pins: test_gen5_create_edit_parity
(R2-BUG-003 divider call), and the Gen II Special pins in five tests now follow the owner's rule.
