# Gen V production completion — progress checkpoint (Thread 2)

Branch `claude/gen5-integration-20261014` (draft PR #307), base
`claude/gen14-final-20261014` @ `4b9b9f65`. Work started from
`a17c503ad272cc55e69b8b3cbd925cec999567cc` after the coordinator confirmed
ownership. Forward commits only; no history rewritten.

Physical Switch status of every NRO built from this branch: **NOT DEVICE TESTED**.

## Gen I–V completion campaign checkpoint (Thread 2R)

HEAD `77ad7fb` (draft PR #307). No candidate NRO is posted until the campaign ends; there will be ONE final NRO, and it is NOT DEVICE TESTED.

| Commit | Unit |
| --- | --- |
| `8529557` | Gen V Clone and Move Pokemon use the Gen IV box (1–24) / slot (1–30) keypads |
| `5342d6e` | Gen V Release confirmation matches the Gen IV dialog |
| `bc83815` | QA-F4: the PID replay protects the gender and ability links independently (the QA case is ported and fails on the old guard) |
| `cb88043` | Merge of Thread C `claude/gen14-campaign-20261015` @ `8d7609f5` |
| `6c366bd` | QA-F5: the Gen V Home party strip keeps the native form |
| `b3f8871` | One Gen I–V Home party rule (`Integration/Common/HomePartyPreview.h`): species / level / form / shiny / gender, filled only where the format defines them |
| `7251977` | Home preview contract needles restated for the adapter (old→new list in the message) |
| `cedc2f8` | Gen V shared-panel captions are no longer than the Gen IV strings; caption glyph/length guard added |
| `96a4468` | QA-F2: the three invalid one-time workflows are repaired (whitespace only) and gated to `workflow_dispatch` |
| `a0b7220` | Merge of Thread C @ `cd2bbbff` (Review discard, DV reroll confirm, Gen II Clone, Gen IV bag layout, export separation tests) |
| `181423a` | Merge of Thread L @ `50491e88` (bounded chain plan, opt-in return host, return focus) |
| `77ad7fb` | Product Home shows the quit-route notice before launch (Thread L request), cached per launch target |

Gen V audit items, verified in code with tests:
- Exit guard: Browse with staged changes goes to ConfirmExit, where B stays, X reviews and Y discards and leaves (`test_gen5_readonly_save.cpp`).
- Focus restore: shared template, the same path as Gen IV.
- Glyphs: only ASCII plus — • é, all of which Gen IV already draws. Caption length is ≤ Gen IV (`test_gen5_readonly_ui_contract.py`).
- Popups:
  - Add opens the editor directly on A and on touch, as in Gen IV. The create dialog appears only for a moment, or after a failure.
  - Status toasts are the same kind Gen IV shows.

Gates at `cedc2f8`:
- GCC host-test: rc 0.
- Clang host-sanitize: rc 0. The local Clang ASan runtime comes from `apt-get install -y libclang-rt-18-dev`.
- Python contracts: all pass.
- `git diff --check`: clean.
- devkitA64: clean.
- At `b3f8871`, Clang host-test and GCC host-sanitize also passed (rc 0).

Open requests:
- Thread C: a Gen III gender accessor. Until then the Home slot's gender for Gen III is `GenderUnknown`.
- Gen I–III editor divergences 1–7 are with Thread C.
- Per the owner: no gender glyph on the Home icons.

Remaining:
- Further Thread C merges, and the QA merge once Thread 4's matrix is complete.
- Wiring Gen III gender once Thread C publishes the accessor.
- Final gates.
- One final NRO.

## Hardware failure of candidate `35cbf163` and the editor-parity repair (2026-10-15, Thread 2R)

The owner tested `35cbf1637da41e9607f2d524354c51d70e401fb2` on a Switch and Gen V **failed
hardware acceptance** on editor parity. Owner's rule: ONE PKSE interface, ONE shared editor,
ONE set of menus; only generation-specific data, restrictions and capabilities may differ.

**Root cause (all findings).** `Gen5SharedReadOnlySurface.inc` re-implemented its own
workspace, pickers, field model and navigation (`drawWorkspace5`, five Gen V-only pickers,
`Gen5WorkspaceLayout`, a 43-row `kEditableRows` focus list, Left/Right/ZL/ZR mapped to
`adjustField`), while Gen IV was built from the shared components. **Fix:** the Gen IV
workspace was extracted, unchanged, into the generation-neutral
`src/UI/SharedPokemonEditorSurface.inc` (templated on a generation adapter) plus the
host-testable focus model `include/UI/SharedPokemonEditorWorkspace.h`. Gen IV supplies
`Gen4Editor` (its exact strings, setters and catalogs); Gen V supplies `Gen5Editor` over the
new field model `include/Integration/Gen5/Gen5EditorFields.h`. The Gen V-only editor, pickers,
`Gen5WorkspaceLayout.h` and the backend picker/field-row state were deleted.

| Owner finding | Diagnosis | Fix (commit) |
| --- | --- | --- |
| P1 navigation differs; moving focus changes values | Gen V surface mapped Left/Right/ZL/ZR to `adjustField()` and had its own panel model | Gen V runs the shared Gen IV navigation: D-pad/stick cross DETAILS↔STATS↔MOVES, L/R switch panels, values change only after A + picker/keypad; touch on every field; 4 move rows × Move/PP/PP Ups editor (`5d8b833`, `de22e35`) |
| P2 Nature/Gender/Shiny/Ability/PID duplicated or unreachable | Gen V showed a read-only PID summary and edited Nature/Gender/Ability in DETAILS | Shared STATS PID-linked rows; Nature is a stored byte (PK5 0x41); Gender/Shiny/Ability change the PID only through cited deterministic procedures (PKHeX `6501f0ab` EntityGender/PK5/PKM/EntityPID), Gen V origins only, behind an explicit "Change PID?" confirmation with strict re-verify; PID read-only (`2af2563`, `6efd8c6`) |
| P3 Actions/Add menu not the Gen I–IV menu | Custom Gen V modal | `drawNarrowPokemonActions` with only available actions, Gen I–IV order (`de22e35`, `f041992`) |
| P4 field controls | Raw ID keypads for item/ball/location; ID-only ability/form lists | Shared Species (1–649, sprites, Y shiny, X search, refusal reasons, native Create rebuild), Held Item grid (PKHeX `ItemStorage5.GetAllHeld`), Ball 1–25, Met Location (ID 0 shown as text), Pokérus, compact Form, named Ability, Nature layout; keypads only for numeric fields; DETAILS order = Gen IV + OT Gender + Fateful (`de22e35`) |
| P5 move picker | Name + PP only | Shared move picker row + Gen V type icon, category, Pwr/Acc (— when undefined) and PP from the cited table `include/Names/MoveBattleStats5.h` (PKHeX MoveInfo5 + Showdown gen5, cross-checked); "Native" = valid ID, learnability not checked (`888b886`, `de22e35`) |
| P6 Box HP/stats 0 | `Pokemon5ReadOnlyView` returned party cache only (0 for boxed) | `displayStats5` shared by Box summary, View and Edit (`de22e35`) |
| P6 origin/OT/met text overlaps graph | Long data rows | Short GEN V DATA rows, values bounded (`de22e35`) |
| P6 Form/Nature menus oversized | Gen V-only 680×525 pickers | Shared NaturePickerLayout and compact Form picker (`de22e35`) |
| P6 Trainer/Items conventions | Clamp/first-tap-focus lists, read-only trainer rows focusable | Items: wrap, page-start paging, swipe scroll, tap = A; Trainer: focus only actionable rows, wrap, tap = A (`ad95b2d`) |
| P7 contract tests | none | `tests/test_shared_editor_contract.cpp` — the owner's 14 checks for Gen I–V layouts, plus routing/no-separate-editor locks (`f041992`, `6efd8c6`) |

### Gen I–IV files touched (authorised for this purpose, behaviour unchanged)
`src/UI/Gen4SharedPokemonSurface.inc` (workspace extracted; Gen IV adapter keeps every
string/setter/catalog), `include/UI/SharedPokemonEditorContract.h` (Gen V layout added; Gen
I–IV entries unchanged). No Integration/Gen1–4 file was touched. Every existing Gen I–IV host
test passes under GCC and Clang, with ASan/UBSan.

### Source-text contract readers (coordinator-approved, no assertion dropped)
`tests/test_gen4_hardware_surface_contract.cpp`, `tests/test_nature_picker_layout.cpp` and
`tests/test_shared_species_picker_surface_contract.cpp` (plus the UI-owned
`test_reachable_ui_consistency.py`, `test_touch_appwide_surface_contract.py`,
`test_touch_pokemon_editor_direct_rows_contract.py`) expand the one textual include
`SharedPokemonEditorSurface.inc` (commit `b7c7336`). `test_move_picker_presentation.cpp` needed
no change. Needles restated old → new:

| Old needle | New needle(s) |
| --- | --- |
| `Shared::normalizeMoveRowFocus(Shared::Generation::Gen4` | `static constexpr auto generation = Shared::Generation::Gen4;`, `Workspace::navigate(A::generation, state.focus, keys, passive, editable,`, model `focus = Shared::normalizeMoveRowFocus(generation, focus);` |
| `Shared::moveRowColumn(Shared::Generation::Gen4` | model `: Shared::moveRowColumn(generation, focus, -1);` |
| `Shared::passiveViewMoveColumn(Shared::Generation::Gen4` | model `focus = passive ? Shared::passiveViewMoveColumn(generation, focus, -1)` |
| `openMoveEditor(screen, state, state.focus.row)` | `openMoveEditor<A>(screen, state, activation.row)` |
| `state.moveBaseline = state.session.working->moves()` | `state.moveBaseline = A::workingMove(screen, state, slot)` + `gen4(state).session.working->moves()[static_cast<size_t>(slot)]` |
| `state.session.working->setMove(slot, state.moveBaseline)` | `A::setMove(screen, state, slot, state.moveBaseline)` |
| `if (detailEditable(state, state.focus.row, p)) return;` | model `if (detailEditable(static_cast<int>(focus.row))) return focus;` + `return UI::Gen4SharedEditorSurface::detailEditable(gen4(state), row, p);` |
| `if (pidLinkedRowEditable(state, state.focus.row, p)) return;` | model `if (pidEditable(static_cast<int>(focus.row))) return focus;` + `return pidLinkedRowEditable(gen4(state), row, p);` |
| `state.session.mode == SessionModel::Mode::View \|\| editable` | `state.focus.row == row && (viewMode \|\| editable);` + `return gen4(state).session.mode == SessionModel::Mode::View;` |
| order `drawMoveEditor(screen, fb, state)` < `drawValuePicker(screen, fb, state)` | order `drawMoveEditor<A>(screen, fb, state)` < `drawValuePicker<A>(screen, fb, state)` |
| `if (state.session.confirmExit)` | `if (A::confirmExit(screen, state)) {` + `return gen4(state).session.confirmExit;` |
| `species, 493, previewShiny` | `species, A::kMaxSpecies, previewShiny` + `static constexpr int kMaxSpecies = 493;` |

### Internal build `c952565c` (NOT an owner hardware candidate)
Per the owner's completion-campaign directive, only ONE final Gen I–V NRO will be posted.
`c952565cf878705726b2541546d085fa9724108d` (tree `3090368d1e84f97839c0d9012e473ec8280309db`) is an
internal build only: Gen I–V integrated NRO built in run 38694846639 (artifact 11953557955,
`PokeBank-NX-Gen1-5-Candidate-c952565c.nro`, 175,517,361 bytes, SHA-256
`e35d6efd33837aee2f9e726d29ac1ab350ace950bb298946c92991f99fecc995`), 8/9 checks green with
Host tests (GCC) still running when recorded. Local: GCC host-test + host-sanitize and Clang
host-test pass; local Clang host-sanitize cannot link here (container has no clang ASan
runtime), covered by CI "Host tests + ASan/UBSan (Clang)" green. NOT DEVICE TESTED.

Thread 2H (`claude/gen5-home-providers-20261015` @ `ece88e27`) was merged forward with a merge
commit (`9c038d9`) before the integrated candidate.

## Hardware failure of candidate `fa9cfcea` and the repair (2026-10-15)

The owner tested `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe` on a Switch and Gen V **failed
hardware acceptance**. Gen V is **not** complete and **not** hardware-ready. Diagnosis and fixes:

| # | Failure seen on hardware | Root cause | Fix (commit) |
| --- | --- | --- | --- |
| 1 | Opened straight into Party, no hub | `UI.cpp handleGen5View` forced `selectedMode=Party; detailViewActive=true` | Opens on the shared loaded-game hub; `Gen5HubRouting.h` decides the only Gen V intercepts (`7123035`) |
| 2 | Create showed only Cancel | `emptyCloneDestination` required raw zero bytes; retail empty slots hold the *encrypted* blank (PID 0, checksum 0 → non-zero keystream; PKHeX `SaveFile.ClearBoxes` writes `BlankPKM`), so Add was never offered. The provider also lacked `PokemonCreation` | `nativeBlankSlot5` accepts zeros or the encrypted blank; Remove now writes the retail blank (`3cbf368`) |
| 2b | Create was a ~30-prompt wizard | Separate Gen V UI | Create opens the SAME Edit draft/three-panel workspace on Species; nothing persists until Keep; Discard restores the native blank (`80bfce6`) |
| 3 | Real Black 2 DraStic `.dsv`: "Primary copy / Party 0 / Boxed 0", name "(unsupported glyphs)", Money 0, TID/SID 0, Male, Dex 0 | **Confirmed by the coordinator's local analysis of the owner's four Gen V `.dsv` files (not committed, not shared):** both copies are valid and identical and hold a freshly initialized new-game save (no name, TID=SID=0, play time 0, party 0, 720 encrypted-blank slots). The decode was correct; the zeros are the truth. The Platinum control file in the same setup holds real data, so the missing Gen V progress is an owner-side question (DraStic flush, savestate-only progress, another `.dsv`). The failure was presentation: an uninitialized trainer was shown as a played game with a male trainer | New explicit `NewGameNoTrainer` state: the save opens, the Save Instances row and Home card say "New-game save: no trainer yet", the portrait is neutral, and the trainer rows show "-" (never "Male"). Trainer edits and Create are refused with the reason, since no OT exists to copy. A new-game copy beside a real game is still refused as stale (`395e2a6`, `b9c1ea8`, this commit) |
| 4 | Male portrait for a female trainer | On these files the gender byte is 0 because no trainer exists yet; unknown gender fell back to Hilbert/Nate | Gender is shown only for an initialized trainer; otherwise a neutral "Unova Trainer"; hilbert/hilda/nate/rosa are required RomFS portraits (`b9c1ea8`) |
| 5 | Separate "Trainer – Generation V" pop-up | Gen V-only modal | Trainer is drawn in the shared content rect/HOME card; read-only rows labelled; scrolling; focus memory; B returns to the hub (`7123035`) |
| 6 | General parity | Gen V owned every input | Hub, Pokémon Actions, shared Create/Edit, Items, Review/Export routes now follow Gen IV; Legacy Storage (Bank) is refused with a status because no PK5 bank conversion exists |

### On-device save diagnostics
Save Instances → Gen V candidates → **+ Save Diagnostics** writes
`sdmc:/switch/PokeBank-NX/diagnostics/gen5_<game>_<timestamp>.txt` (exclusive create, never
overwrites, source opened read-only). It lists per copy: block checksums valid/total, version
byte, gender byte, whether TID/SID and a name are present (no values), party count and record
validity, occupied / encrypted-blank / zero / invalid box slots, dex seen/caught, play time and the
copy state (`valid, populated` / `no trainer data (blank or initial copy)` / `checksums invalid` /
…). It contains no names, IDs or save bytes. This is how the Black 2 `.dsv` case is confirmed.

### Shared files touched (Gen V branches only; Gen I–IV behaviour unchanged)
`src/UI/SaveSelectScreen.cpp` (Gen V portrait fallback, Gen V card label, diagnostics key),
`src/UI/UI.cpp` (`handleGen5View` only), `tools/verify_nro_romfs.py` (+4 Gen V portraits).
No Gen IV source file was changed.

## Status by step

| Step | Software | Automated verification | Hardware |
| --- | --- | --- | --- |
| 1. Bag in shared Items screen + Review + export | Done | `test_gen5_bag`, `test_gen5_bag_screen`, UI contract | Not tested |
| 2. Party/Box parity with Gen IV + fields | Done (Move Pokemon, read-only origin fields) | `test_gen5_party_box_parity` | Not tested |
| 3. Trainer (Name / Money / Play Time) | Done; other rows read-only | `test_gen5_trainer` | Not tested |
| 4. Pokédex | Read-only view done | `test_gen5_trainer` (dex section) | Not tested |
| 5. Source assignment / copy choice e2e | Tests done | `test_gen5_source_e2e` | Not tested |
| 6. Native record validation | Tests done | `test_gen5_export_validation` (572 randomized exports) | n/a |
| 7. Housekeeping | `patches/gen5-create` removed (verified live) | — | n/a |
| 8. Integrated Gen I–V candidate | Workflow `gen15-integrated-candidate.yml` | CI on the exact head | NOT DEVICE TESTED |

## What the user can do now (Gen V, all four titles) — as of the repair
- Open → the same hub as Gen I–IV: Pokémon (Boxes), Party, Storage (refused for Gen V), Items,
  Trainer. B on the hub exits through the staged-changes confirmation; + opens Review.
- A on a slot → Pokémon Actions. Add on an empty slot → shared editor (DETAILS / STATS / MOVES).
  The SAME shared editor as Gen IV: D-pad/stick move between fields and across DETAILS / STATS /
  MOVES, L/R switch panels, A opens the field's picker or keypad (focus never changes a value),
  X Keep/Add, B back (unsaved changes ask Save / Discard / Back).

### Earlier (pre-repair) notes

- **Items (Bag):** from the Trainer panel press **Y** (or tap the "Y Bag" footer), or use
  Product Home → Items, which runs the same explicit Save Instance chooser. Pockets: Items,
  Key Items, TMs & HMs, Medicine, Berries (L/R or the header arrows). **A** or a second tap on
  a row: quantity keypad. **X**: Add picker (native, released items not already held).
  **Y** then **Y**: Remove. Key Items and TMs/HMs are view-only. An unrecognized bag shows
  "Bag unavailable" and nothing is guessed. **+** opens Review.
- **Trainer:** **A** on the Trainer panel opens the field list. Name (1–7 printable ASCII),
  Money (0–9,999,999) and Play Time (to 999:59:59) are staged. TID, SID, gender, badges and
  language are read-only. The Pokédex row opens a read-only seen/caught list.
- **Boxes:** Pokémon Actions now include **Move Pokemon** (Gen IV parity), alongside View,
  Edit, Clone and Remove. Party keeps View/Edit/Clone.
- **Review** lists staged Pokémon, each Bag change (added / removed / quantity) and each
  Trainer change. Export writes `original_backup.sav`, `edited.sav` and a manifest that
  records `BAG_CHANGED` and `TRAINER_CHANGED`. The source save is never written.

## Citations

- Bag: PKHeX `6501f0ab` PlayerBag5BW/B2W2, ItemStorage5*, SaveBlockAccessor5* (block 25).
- Trainer: PKHeX `6501f0ab` SaveBlockAccessor5BW/B2W2 (PlayerData block 27, Misc block 52),
  PlayerData5.cs, Misc5.cs, SAV5.cs (`MaxStringLengthTrainer` 7), SaveFile.cs
  (`MaxMoney` 9,999,999), StringConverter5.cs. The 999-hour cap is app policy, not a format rule.
- Pokédex: PKHeX `6501f0ab` Zukan5.cs (BW block 55, B2W2 block 54; caught at 0x08, seen at
  0x5C, four 0x54-byte regions).
- PK5 origin fields: PKSM-Core `aa22d7a` `source/pkx/PK5.cpp` (0x78–0x7E, 0x85).

## Open items / limits

- If a source save's party stat cache or EXP disagree with its level (retail saves never do),
  staging a stat field and then reverting it makes that record fail export certification.
  This fails closed (export refused, nothing written); Discard restores it.
- PR #280's Gen III/IV empty-slot touch parity tests live in Thread 1 files
  (`tests/test_gen3_hardware_surface_contract.cpp`, `tests/test_gen4_hardware_surface_contract.cpp`,
  plus behaviour in the Gen III/IV surfaces); not ported here.
- Legality-session handoff `claude/gen5-handoff-from-legality-20261014` @ `fc519414`:
  reimplemented rather than merged. Its two changes (items from the staged/source bag; BW/B2W2
  pocket names) are covered by commit `b9c8d8f`.
- Gen V reuses the Gen IV DraStic launch path; automatic return to PokeBank NX is **not** claimed.
- The candidate NRO identity (code SHA, tree SHA, size, SHA-256, CI run IDs) is recorded in a
  PR #307 comment once CI finishes on the final head.
