# PKSE October 2026 (Oct 1–28) → PokeBank NX integration matrix

**Status: NOT DEVICE TESTED.** Host-tested only. Work lands on `integration/gen15-combined` (owner Oct 28:
one combined Gen I–V candidate holding Rounds 6/7 (#307), Round 8, the multilingual milestones and this
campaign; nothing reaches `main` until the owner confirms on hardware). Legality PR #314 is not integrated.

## Pinned sources

| Ref | Commit |
|---|---|
| PKSE default branch (`master`, v1.2.1 release) | `94ffce5866a032f1fe9577b0025021df6bd935bc` |
| PKSE `version-1.2.2` (pinned dev head) | `a79fe61a6d6d596bacae09e6299647aebf5c4340` |
| PKHeX (sparse; PK1–PK5, SAV1/2 only) | `8e3eb3d4` |
| pret pokered / pokefirered / pokeemerald / pokecrystal | `171e2c2` / `ce6406d8` / `731ad5bf` / `5c877ef5` |
| PokeBank NX base | `wip/pkse-1.2.x-gen15-audit` `66f8a53f` + `feature/multilingual-gen15` `e6bdbebb` |

Both PKSE branches were fetched on 2026-10-28. `git log --all --since=2026-10-01 --until=2026-10-29` lists 60
commits; every merge (#126, #128, #129, #131–#134, #136, #137, #142–#144, #146) was expanded to its second
parent, and all merged content falls inside the window. This file supersedes nothing: the earlier
[`PKSE_1_2_X_GEN15_GAP_MATRIX.md`](PKSE_1_2_X_GEN15_GAP_MATRIX.md) keeps its decided rows; this one adds the
rest and records what PokeBank NX now does about each.

## Commit index (every commit in the window)

| Commit(s) | Author | What | Gens | Decision | PokeBank NX |
|---|---|---|---|---|---|
| `c4b7efc` (#126) | Kiasta | PKSE's own on-screen keyboard | all | ALREADY IMPLEMENTED | our `KeyboardDialog` (gap matrix §6); limits fixed in `0489dd20` |
| `b78dd72` `821fac2` `d131907` `b529c03` `9932432` `ff2f9d1` `f9220c7` `b133ebf` `e2a8a0f` `7152194` `e00dfa0` `fa06a1b` `bfa9491` `aa4b72e` `1f8f865` (#128) | GlitchedZeus (owner) | Standalone Pokémon-file import/export | 1–9 | ADOPT (Feature A, re-implemented on our staged editors) | in progress, see Feature A below |
| `9458c7f` `3e017e0` `a23ff7e` `2d37590` `b0d6101` `b361cf2` `e457eb2` `3ef6f99` `ef9257a` `2c563b6` `8f13dba` `a2918ef` `e5c4f71` | GlitchedZeus / bot | CI scaffolding for #128 | – | NOT APPLICABLE | – |
| `f27bd82` (#131) | Kiasta | SV Sweet Herba Mystica add (OBTAINED bit) | IX | DEFER | backlog B-SV-1; Gen I–V picker side effect already equivalent (A6) |
| `292fd5e` (#129) | Kiasta | Emulator save containers | II–III | ALREADY IMPLEMENTED / FIXED | gap matrix §2; RTC Bag/Trainer fix `e13ced8e`; 0x22000 and headers stay backlog B-03/B-06 |
| `0c993bc` (#129) | Kiasta | Accept every loose save PKHeX accepts | II–III | ALREADY IMPLEMENTED (Gen II); REJECT (Gen III signature) | gap matrix §1/§2; Round 8 Q02/Q05 |
| `0d68c21` `03d2293`† | Kiasta | Docs, reformat | – | NOT APPLICABLE | †`03d2293` also carries PR #134's UI routing (A1) |
| `18c2555` (#132) | Kiasta | Pokémon-file repairs and test hooks | all | ADOPT with Feature A (A2) | Feature A |
| `56a0667` (#132) | Kiasta | Host harnesses, dev-build self test | all | IMPROVE (opt-in real-save round-trip runner); REJECT SD-writing console self test (A3) | Feature D |
| `620c5f6` (#133) | Kiasta | Bank safety, PID rerolls, item flags, discard state, `.ekX` | all | see rows below and gap matrix §4/§8 (A4) | Bank: Round 8 Q01; C-4 fixed here |
| `03d2293` (#134) | Kiasta | Boxes-view Y menu, X box menu, Clone/Export reachable | UI | REJECT redesign; safety part ALREADY IMPLEMENTED (A1) | staged explicit-destination clones |
| `9a9a731` (#136) | Kiasta | LA research, Let's Go round trip, pouches, Gen 2 padding, BDSP | I–II, VI–IX | Gen I–II: FIXED in Round 8; Gen VII–IX: C-1 and C-2 FIXED here, rest DEFER (A5) | see below |
| `379bb7d` `15b48b6` `cd2c0a8` | Kiasta | Version bumps | – | NOT APPLICABLE | – |
| `f4fd79a` (#142) | Kiasta | Favourite folders, remembered last folder | all | ADOPT (Feature C) after hardening (A9) | settings hardening done here; browser pending |
| `c671aa4` (#143) | Kiasta | Alpha toggle; empty start folder | VIII–IX; all | Alpha DEFER (B-ALPHA-1); empty folder FIXED here (A7) | `directoryExists("")` guard |
| `d2ad323` (#144) | Kiasta | Add All Items | all | ADOPT as staged Top Up / Add Missing (Feature B); not the upstream algorithm as is (A10) | pending |
| `65dff1d` (#146) | Kiasta | Hyper Training | VII–IX | DEFER (B-HT-1) (A8) | – |

## Inherited bugs reproduced and fixed in PokeBank NX (this campaign)

Each fix has a host test that fails on the old code (reproduction stated). Synthetic data only.

| ID | Upstream | Our defect (reproduced) | Fix | Test |
|---|---|---|---|---|
| E-1 | `d2ad323` table; pret | Gen I Exp. All (75) was a "key item" capped at 1. pret pokered `data/items/key_items.asm:77` (`171e2c2`) says `dbit FALSE ; EXP_ALL`; PKHeX stacks it to 99. `test_item_quantity_policy` failed on `r.valid`/`r.editable` | 75 moves from the Gen I key list to ordinary items | `test_item_quantity_policy` (classic inventory gate) |
| C-1 | `9a9a731` (#135) | Legends: Arceus: every working-copy build, including every Bank ↔ save true Move post-image, added +1 "Number caught" for every stored Pokémon. Host repro: building the same state twice took a count from 1 to 2 | Pokémon already accounted for (species, EC) are skipped once; only new arrivals register; the snapshot is retaken after each build | `test_pla_research_counts` (unedited rebuild, box move, new arrival, identical clone) |
| C-2 | `9a9a731` | Let's Go: every storage write zeroed the game's own empty-slot records (EC 0 + non-zero bytes); an unedited write was not byte-identical | The file's blank records follow the packed list in order; a slot freed this session gets a copy of a genuine blank, never zeros | `test_lgpe_blank_slots` |
| C-4 | `620c5f6` | Details-page stat dialog called `regeneratePID` after any IV/EV edit (Let's Go: new encryption constant); discarding details edits left "unsaved changes" set. Reachable in Gen VII+ sessions only (Gen I–V details pages are read-only) | Call removed; dirty flag restored with the bytes | `test_pkse_october_contracts` |
| B-14 | `f4fd79a` | `settings.cfg` read in 128-byte chunks: a long line's tail became its own setting. Repro on the exact old loop: `favoriteFolder=` + 112 chars + `debugLogging=1` switched debug logging on | Whole-file read (≤ 64 KiB), whole-line parse (line ≤ 4096, value ≤ 1024, identifier keys, no control bytes), over-long lines dropped whole | `test_settings_file_parse` |
| B-15 | `c671aa4` | `directoryExists("")` relied on `stat("")` failing | explicit empty guard | `test_settings_file_parse` |
| A-1 | `620c5f6` review (PID routes) | Gen IV/V Ability picker opened on the PID's slot while the row showed the stored ability; pressing A on a record whose stored ability disagrees with its PID rewrote it (Sturdy → Rock Head) and dirtied the draft without a prompt. Found by the new route tests | Picker opens on the slot holding the stored ability; choosing the stored ability is a no-op (`Gen4AbilityChoice.h`, `Gen5EditorFields::abilityChoiceFor/abilityChoiceKeepsStored`) | `test_gen345_pid_preservation_routes` (`abilityPickerKeepsUnusualStoredAbility`) |
| PID | `620c5f6` | (coverage) production routes for PID/shiny/gender preservation | Gen III (R/S/E/FR/LG, box and party), Gen IV (DP/Pt/HGSS), Gen V (BW/B2W2, party and box): IV/EV edits through Keep → Review → serialization → strict reparse keep PID, shiny, gender, nature, ability; unchanged re-stages are no-ops; Discard/rollback restores bytes and clears pending state; unusual records stay unusual | `test_gen345_pid_preservation_routes` (GCC, ASan/UBSan, Clang) |
| T-01 | `9a9a731` | (coverage) Gen IV had no full-pouch test | Every legal Items-pouch item fits (no false "full"); Berries (64 legal = 64 slots) fills, further Adds refused, bytes unchanged, reparse | `test_gen4_readonly_foundation` |

Not reproducible in Gen I–V (verified): the post-IV/EV `regeneratePID` and the discard flag sit behind the
classic-session guard (`include/UI/ClassicGenericUiGuard.h:35`); Gen I–V PID/shiny/gender preservation is
covered through the staged editors (see the PID route tests).

## Features (Phase 3)

| Feature | Upstream | Decision | Design for PokeBank NX |
|---|---|---|---|
| A. Standalone Pokémon files | #128 + `fa06a1b` `aa4b72e` `18c2555` `620c5f6` | ADOPT | Our Bank stores only FireRed/LeafGreen among Gen I–V formats, so `.pk1`–`.pk5` import into the **open save's staged workspace** (same native format only) as a staged change with Review/Keep/Discard. Encryption detected from content (PKHeX `IsEncrypted3/45` predicates cross-checked with the checksum; disagreement refuses). Box vs party by size; malformed sizes, bad checksums and wrong-format content refused; nothing invented; no cross-generation transfer. Export writes the native record to PokeBank NX's own exports folder. |
| B. Top Up / Add Missing items | `d2ad323` | ADOPT (safe form); planner + all-or-nothing applier done (`BulkItemPlan.h`, `BulkItemApply.h`, tests `test_bulk_item_plan`, `test_bulk_item_apply`); Bag-screen UI pending | Staged batch on our Bag editors with preview and explicit confirmation. Key items, HMs and story items excluded by default. Uses our per-item quantity rule (RSE 99, not upstream's 999; Gen II one slot per id). Never lowers a count, never exceeds pouch slots, reports what did not fit. |
| C. Favourite / remembered folders | `f4fd79a` `c671aa4` | ADOPT after hardening | Bounded settings (done), canonical paths with case/trailing-slash dedup, atomic writes (existing), missing folders flagged and kept, read-only consent, never a write grant. |
| D. Dev self-test | `56a0667` | IMPROVE | Opt-in host real-save round-trip runner (saves never committed). No SD-writing on-console self test in production. |
| E. Cloning / L-R repeat | `03d2293` | ALREADY IMPLEMENTED / REJECT | Our staged explicit-destination clones already refuse silent spill; held L/R repeat stays edge-only on actions (backlog B-12). |

## Gen VI–IX

Recorded permanently in [`../future/GEN6_9_PKSE_BACKLOG.md`](../future/GEN6_9_PKSE_BACKLOG.md) (Hyper
Training, Alpha, Legends: Arceus research/size, Let's Go dex sizes and partner forms, BDSP round trip and
trainer name, SV item flags, Gen 6/7 pouches, per-item limits). C-1 and C-2 were fixed now because they are
reachable today through the Bank true Move.

---

# Appendix: per-commit evidence (source-level research, 2026-10-28)

## Per-commit rows

### A1. `03d2293`: PR #134 "extend-cloning-feature" (2026-10-14)

| Field | Finding |
|---|---|
| Feature (from the diff) | **UI routing only; the clone algorithm itself does not change.** (1) In the **Boxes** view, **Y** now opens the per-Pokémon action menu (Move / Edit / Clone / Export / Release / Cancel). Before, Y grabbed the slot for a swap straight away. Clone and Export are therefore reachable from the Boxes view for the first time. (2) **Move** from that menu in the Boxes view starts the existing *swap* grab (`swapActive`, `swapSourceBox/Slot`). In Storage it still lifts a 1×1 carry. (3) `storageMenuRowNeedsUnlockedSlot(menuRow, ViewMode)` disables **Move** on a party-linked (Let's Go) slot only in Storage. In Boxes the swap carries the party pointer, so Move stays enabled there. Release stays disabled on a locked slot in both views. (4) **X** in the Boxes view no longer releases the Pokémon under the cursor. It now opens a new **box menu** (Sort / Search / Cancel; `BOX_MENU_*`, `openBoxMenu`, `boxMenuActive`). In Storage, X (when not holding anything) opens the same menu instead of sorting immediately. (5) "Find…" moves from the per-Pokémon menu to the box menu as `searchStorage()`. The Boxes view searches the save only; Storage also searches the bank. The Boxes grid now highlights matches and dims the rest (`Colors::SearchMissShade`). Choosing the Boxes HOME entry clears `storageSearch`. (6) The context-help bar gains a row for `storageMenuActive || boxMenuActive || groupMenuActive`. |
| Clone semantics (unchanged, for comparison) | `STORAGE_MENU_ROW_CLONE`: `source->clone()` goes into the first empty, **unlocked** slot of the source box, then of **any other box in the same pane** (bank or save). It never crosses panes. A full pane gives "No free slot to clone into." Byte-identical copy, no PID/EC change. Marks `hasUnsavedChanges`. |
| Held repeat / selection | No functional change. `NavigationRepeat.h` and the `repeatableShoulders` expression were only reformatted. Rectangle selection (`grabSelection`, X = copy) is untouched. |
| Native gens | All (UI layer). The locked-slot rule only matters for Let's Go (Gen VII). |
| Upstream files/functions | `include/UI/TrainerViewScreen.h` (`STORAGE_MENU_ROW_*`, `BOX_MENU_*`, `storageMenuRowNeedsUnlockedSlot`, `openBoxMenu`, `searchStorage`); `src/UI/TrainerViewScreen.cpp` (Boxes-view X/Y handlers, the storage action menu handler, the new box-menu handler, `searchStorage`, `openBoxMenu`, help text); `src/UI/Panels/StoragePanel.cpp` (`drawStorageBoxMenu`, disabled-row mask); `src/UI/Panels/BoxPokemonPanel.cpp` (search highlight); `tools/harness/uihost/uitest.cpp` `testBoxMenus`. |
| PokeBank NX equivalent | **Different implementation.** Gen I–V clones are staged, explicit-destination operations on the native editors: `Gen1StagedPokemonEditor::stageClone` (`include/Integration/Gen1/Gen1StagedPokemonEditor.h:62`), `Gen2 StagedEditor::stageCloneBoxPokemon` (`src/Integration/Gen2/Gen2StagedEditor.cpp:1062`), `Gen3 StagedPokemonEditor::stageCloneBoxPokemon` (`src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:913-960`: empty destination required, byte-identical re-parse check, rollback on failure), `Gen4StagedPokemonEditor::stageCloneBoxPokemon` (`src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:430-460`: explicit box/slot prompt, occupied destination refused). UI entry points: `src/UI/Gen2SharedPokemonSurface.inc:207-250` (L/R picks the box, first writable slot), `src/UI/Gen1PokemonEditorOverlayUX.inc:644-660`, `src/UI/Gen4SharedPokemonSurface.inc:1053-1066`, `src/UI/Gen3SharedPokemonSurface.inc:1737`. The inherited PKSE storage grid in `src/UI/TrainerViewScreenBase.inc` keeps the **pre-#134** layout: X = copy-grab of a selection (`:2560-2577`, duplicate made at `:2156`) or sort. It has no Clone row. |
| Equivalent fix? / defect reproducible? | No defect is involved; #134 is an interaction redesign. Our Gen I–V clones never auto-spill into another box without telling the user (Gen II/Gen I pick the box explicitly; Gen IV prompts for box and slot). No inherited bug. |
| Upstream correctness | Coherent. One risk is in the design: **X in the Boxes view used to mean Release and now means "box menu"**, so muscle memory now lands on Sort. Sort runs straight from the menu with no confirmation (`BOX_MENU_ROW_SORT` → `sortStorageBox`). That reorders a box the player arranged, and the only undo is Discard. |
| Suggested tests | If we ever adopt a box menu: (1) a party-linked Gen VII slot keeps Move enabled in Boxes and disabled in Storage; (2) Sort is unreachable while carrying (upstream suppresses it, `:3187-3191`); (3) clone into a full pane refuses with no mutation. |
| Decision | **REJECT** (UI redesign that conflicts with our NarrowPokemonActionMenu order and staged clone). **ALREADY IMPLEMENTED** for the safety part (no cross-pane spill; explicit destination). |

### A2. `18c2555`: "fixed bugs that were discovered…" (2026-10-13)

Functional fixes, enumerated from the diff:

| # | Fix | Gens | Upstream location | PokeBank NX | Decision |
|---|---|---|---|---|---|
| 1 | **Plaintext first, then encrypted, when importing `.pkX`.** Before this, a file's bytes were handed to the entity constructor as if encrypted first, and the plaintext path was only a retry. A plaintext record that happened to pass as encrypted (≈1/65 536 by checksum) was decoded as garbage. | 3–9 | `src/Trainer/PokemonFile.cpp` `parseUnchecked` (lambda order) | No PokemonFile code (`grep -rl 'PokemonFile\|\.pk[1-9]' src include` = none) | DEFER with F-01 (matrix §9) |
| 2 | **Content-vs-extension check.** A record whose decrypted contents contradict its extension is refused. Ports PKHeX `EntityFormat.GetFormat45` (0x04 sanity ≠ 0 → Battle Revolution; 0x5F < 0x10 && u16@0x80 < 0x3333 → PK4; u16@0x46 ≠ 0 → PK4; else PK5), `IsFormatReally7` (origin > OR, species > 721, moves > 621, ability > 191, item > 775, 0xDE ground tile vs HT, markings > 0xAAA, 0x2A > 20), `IsFormatReally8b` and `GetFormat89`. | 4–9 | `extensionsForGen45/67/8/89`, `contentMismatch` | none | DEFER (F-01). The Gen IV/V part is the only one inside our scope. |
| 3 | **Gen 1/2 box record imported with zero party stats.** When `statHPMax()==0`, `recalculateStats()` + `refreshChecksum()` run before the record enters the Bank. Otherwise the Bank shows 0 HP and a withdrawal writes a 0-HP party member. | 1–2 | `prepareForBank` | none (F-01). The matrix lists `aa4b72e` for the related "no party-level cache" case. | DEFER (F-01) |
| 4 | **Let's Go `.pb7` accepts origin game 34 (GO).** Previously `matchesGameFormat` accepted only GP/GE, so every GO Park transfer (Meltan, Melmetal…) was refused. | 7 (LGPE) | `ORIGIN_GAME_GO`, `extensionsForGen67` | none | DEFER (backlog) |
| 5 | **Single normalisation.** `importIntoBank` called `prepareForBank` a second time on a record `load()` had already normalised. It also **set `bank.currentBox` to the import box**, an unsaved bank change. Both are removed. | all | `src/Trainer/PokemonFileBank.cpp` `importIntoBank` | none | DEFER (F-01): if ported, never move the current box on import |
| 6 | **Export file name** uses `pokemon.hasPID() ? pid : id32`. Before, it used `pid()!=0`, so Gen 1/2 names used whatever `pid()` returned. Also: an empty path reports "no free file name in …" instead of a blank error. | 1–9 | `exportPath`, Storage Export handler | none | DEFER (F-01) |
| 7 | Every PokemonFile refusal is logged (`reportFailure`). Import into a null bank reports "the Bank is not loaded". | – | `PokemonFile.cpp`, `TrainerViewScreen.cpp` import handler | n/a | DEFER |
| 8 | **Storage menu rows named by constants** (`STORAGE_MENU_ROW_*`, `STORAGE_MENU_LABELS`). This keeps the handler, the grey-mask and the start row in step, so a new row cannot make the handler act on its neighbour. | UI | `TrainerViewScreen.h`, `StoragePanel.cpp` | Our menus are table-driven (`include/UI/NarrowPokemonActionMenu.h`, `Gen4SharedPokemonSurface.inc:954-961` `add(MenuAction::…)`) | ALREADY IMPLEMENTED |
| 9 | Self-test hooks: `g_suppressKeyboard` (prompts return "cancel" and are counted), `Bank::setRootOverride` (scratch bank, legacy migration skipped), `Bank::encryptRecord`, the save picker's `-` = self test. | – | `Globals.h`, `Keyboard.cpp`, `Bank.cpp`, `SaveSelectScreen.cpp`, `UI.cpp` | Our host tests stub the keyboard and never run against a user root | NOT APPLICABLE |

Upstream correctness: fixes 1–4 follow PKHeX's own heuristics. The Gen 4/5 rule is PKHeX's, but it is
applied *after* a checksum-validated decode, so it is stricter than PKHeX, which decides encryption from
`IsEncrypted45` (`PKHeX.Core/PKM/Util/PokeCrypto.cs:415`, u32@0x64 ≠ 0). Fix 3 is right in its effect:
PKHeX's PK1/PK2 box records carry no stats, and the games compute them when a Pokémon leaves the PC.
Suggested tests (only if F-01 is ported): a `.pk4` holding PK5 content is refused, and vice versa; a
plaintext `.pk3` whose bytes also pass the PK3 checksum when read as encrypted is still read as plaintext;
a Gen 1 box-record `.pk1` with stats of 0 lands with recomputed stats; importing does not change the bank's
current box.

### A3. `56a0667`: test harnesses and on-console self test, gated by a development build (2026-10-14)

| Field | Finding |
|---|---|
| Dev gating (build flags) | `Makefile`: `BUILD_TYPE ?= production`; `BUILD_TYPE=development` → `SOURCES += src/Test`, `DEFINES += -DPKSE_DEVELOPMENT_BUILD`, `TARGET := PKSE-dev`, `BUILD := build-development`, `APP_TITLE += " (Development)"`, `APP_VERSION += -dev` (the build refuses a version over 15 chars, which `nacptool` would truncate). An unknown `BUILD_TYPE` is an `$(error)`. The two build types have separate object directories (the old `-DPKSE_PROD` modifier shared one, so switching left stale objects). `Globals.h`: `SELF_TEST_AVAILABLE` = `#ifdef PKSE_DEVELOPMENT_BUILD`. It replaces the `__has_include("UI/TestRunnerScreen.h")` used in `18c2555`. `UI.cpp` includes `Test/TestRunnerScreen.h` only under the define. The hooks (`g_suppressKeyboard`, `Bank::setRootOverride`, later `Utils::setSettingsFileOverride` from `f4fd79a`) ship in production builds but do nothing unless set. `620c5f6` adds `make all` = both build types. |
| Host harnesses (`tools/harness/`) | `build.sh` (`roundtrip.cpp`): loads each **real** save from a per-title save tree, writes it back **unedited** and diffs. It adds a Sword/Shield item-edit step (positional vs packed pouch writers), an FR/LG/RSE "interrupted" step (one sector of the newer slot erased → must open the older slot), and a Let's Go/Legends: Arceus step in `9a9a731`. `build-gen1.sh` (`gen1.cpp`): real PKSM bank or `.sav`. It proves Gen 1 party stats against the formula as a *band*, recomputing on clones, and proves the unedited write only touches the current box's flush region, the initialised bit and the checksum. `build-oldgens.sh` (`oldgens.cpp`): real DP/Pt/HGSS/B2/XY/ORAS/USUM byte-identical round trips plus edit persistence. Gen 2 is proven to change only mirror destinations and checksum words. It also runs a synthetic Gen 2 codec and every save **envelope** (RTC, Switch Online header, TGB, MiSTer, VBA Next, BizHawk, no$gba RLE/stored, DeSmuME, Action Replay, YSMenu, padding), with near-misses and a stray-padding-byte refusal. `build-pksmimport.sh`, `build-pokemonfile.sh` (against PKHeX's 200 test files), `build-legality.sh` (no Invalid finding on real saves), `build-selftest.sh` (runs the console suites on the host), `blockprobe`, `editpersist`, `memecrypto.sh`. `uihost/` replaces `PKSEFramebuffer.cpp`, `TouchInput.cpp` and `<switch.h>` with recording/scripted stand-ins and drives real screens (`uitest.cpp`, 1 688 lines). |
| On-console self test | `-` on the save picker of a dev build opens `TestRunnerScreen`. It picks **one save per installed title** plus loose-file saves, copies each into `sdmc:/PKSE/tests/work/` (title-prefixed names), and runs 13 suites in a fixed order: tables, assets (the `.nro` really contains 3 260 sprites, 3 fonts and 19 type icons), picker, browser, gentables, entity, setters, legality, ui, pokemonfile, save (structure, round trip, edit persistence: an empty slot filled and a new item added, envelopes), conversion, bank (deposit/save/reload/verify against a scratch bank). `injectToTitle(false)` is unconditional and backups are never touched. It is a frame-budgeted step machine (`Runner::step(budgetMs)`), so the HOME button keeps working. Cancel writes a PARTIAL report to `sdmc:/PKSE/tests/report_<ts>.log`. A keyboard prompt reached during a run counts as a failure. |
| Also in this commit | Gen 4/5 `setShiny` builds the shiny PID constructively (low half walked from a random start, high half from TSV) instead of drawing a million random PIDs (`src/Pokemon/Pokemon4DP.cpp`, `…5BW.cpp` etc.). The matrix §4 already decides this via `620c5f6`. |
| What would catch gaps our host tests miss | **Native save round trips: YES, a real gap.** Our corpus is synthetic only by policy (`tests/fixtures/qa/README.md`: "no real save … is ever committed"). Only Gen 3 has an opt-in real-file hook (`POKEBANK_QA_GEN3_FIXTURE`, `tests/test_retroarch_frlg_discovery.cpp:82`, `tests/test_legacy_frlg_browse.cpp:18`). Upstream's oldgens and gen1 harnesses found faults that synthetic builders cannot: stale Gen 2 mirrors, Gen 4 blank-slot templates, Gen 1 list-terminator bytes, `0x00` vs `0x50` name padding. Suggest an opt-in `POKEBANK_REAL_SAVE_ROOT` runner over every Gen I–V staged editor: unedited export must be byte-identical, or limited to a proven region list such as the Gen 2 mirrors. **UI navigation/dialog focus: mostly covered.** `tests/host-ui/*` and `test_ui_*_host.cpp` drive real screens. Upstream's per-step "a keyboard prompt was reached" failure is a cheap addition. **Pokémon-file import/export: not applicable** (no PokemonFile). **Item additions: partly.** `tests/test_bag_add_flow_gen15.cpp` uses synthetic fixtures. A real-save "add one item, export, re-parse, diff" step would catch layout faults that synthetic fixtures copy from the same assumptions. **Bank recovery: we are stronger** (`test_bank_load_policy`, `test_bank_recovery_contract`, `test_bank_fault_injection`, `test_durable_file`). **Original-byte preservation: covered for synthetic data** (`test_verified_export_verbatim`, `test_gen2/3_inventory_byte_fidelity`). **Encrypted records: covered** (`fuzz_pk_records.cpp` re-encryption round trip plus corpus). **Regional wrappers/envelopes:** we refuse most envelopes on purpose (matrix B-03/B-06). Regional layouts (Japanese GSC, the Japanese 32 KiB case) are covered synthetically (`test_gsc_gen2_adapter`). **Asset presence:** `tests/test_verify_nro_romfs.py` already checks the NRO's RomFS. |
| Decision | **IMPROVE** (adopt the *method*, an opt-in real-save unedited-round-trip runner with region-proof diffs, never committed saves). **REJECT** a console self test that writes to the SD card (our policy: originals immutable; a test pass must not depend on the user's SD layout). |

### A4. `620c5f6`: the parts the matrix does not cover (2026-10-14)

| Item | Finding |
|---|---|
| **Discard state** (UI) | Screen: the Pokémon **details page** (`PokemonDetailsModal`), reached from Boxes, Party and Storage (save and bank panes). Bug: every edit on the page sets `hasUnsavedChanges = true`. Closing without Save restores the bytes from `details.editSnapshot` (`restoreEditTarget`) **but left the flag set**. Discarding the only edit therefore still marked the save changed: Leave prompted to save, and the Save/Discard dialog offered to write an unchanged save. Fix: `DetailsState::unsavedChangesAtSnapshot` is captured in `snapshotEditTarget()` and restored in `restoreEditTarget()` (`src/UI/TrainerViewScreen.cpp:1493-1533`). Gens: all. **PokeBank NX:** the same pre-fix code exists in `src/UI/TrainerViewScreenBase.inc:1453-1483` (`snapshotEditTarget` / `restoreEditTarget`, no flag restore). **Reachability for Gen I–V: not reachable.** `openActionSheetTargetDetails` forces `readOnly` when `genericUiAllowedForSession(…DetailsEditor)` is false (`:2002-2005`; `include/UI/ClassicGenericUiGuard.h:35` `genericUiAllowed(classic, …) = !classic`), and read-only strips A/X/L and the edit taps (`:3614-3620`). It is reachable only in non-classic (Gen VII+ Switch) sessions. **Decision: DEFER** (backlog G-UI-1). |
| **`.ekX` import** | Not detection by content. **The extension picks the order:** `.ek3/.ek4/.ek5/.ek6/.ek7/.eb7/.ek8/.eb8/.ea8/.ek9/.ea9` map to their plain spec (`ENCRYPTED_EXTENSIONS`, `plainExtensionFor`). For a name in `.ekX` form, the bytes are first handed to the entity constructor as **encrypted** (`Bank::makePokemon(group, bytes)`). If `structurallyValid` (checksum) fails, they are encrypted with EC = u32@0 and retried as plaintext. For `.pkX` the order is reversed, and Game Boy formats never take the encrypted path. Then the `18c2555` content check runs. Export is plain only. PKHeX instead decides by content (`IsEncrypted3` checksum mismatch, `PokeCrypto.cs:405-410`; `IsEncrypted45` u32@0x64 ≠ 0, `:415`; `IsEncrypted67` text terminators 0xC8/0x58, `:421`; `IsEncrypted8` 0x70/0x110, `:427`; `IsEncrypted8A` 0x78/0x128, `:433`). Upstream's checksum-first rule is equivalent for Gen 3. For Gen 4+ it carries the documented ≈2⁻¹⁶ ambiguity, which the extension order resolves. Gens 3–9. **PokeBank NX:** none. **Decision: DEFER** (F-01; if ported, use PKHeX's content predicates as a cross-check and refuse when they disagree with the checksum result). |
| **Item flags** | `InventoryItem::isNew/isFavorite` default to false. The matrix covers that part. New here: **Scarlet/Violet and Z-A readers** set `isNew = flags & 1`, `isFavorite = flags & 2` from u32 flags at item record +8 (`include/Trainer/Inventory9SV.h`, `Inventory9LZA.h`). Before, owned items were re-marked "new" on write. Gens: **IX only**. PokeBank NX legacy readers already do this (`include/Trainer/Inventory9SV.h:60-61`, `Inventory9LZA.h:60-61`). **ALREADY IMPLEMENTED**. The `f27bd82` OBTAINED bit is **not** present (see B-SV-1). |
| **Bank cloning/selection** | None in this commit. Bank changes are file safety only (matrix §8). |
| **Post-IV/EV `regeneratePID` removal** | Maps exactly to `src/UI/TrainerViewScreenBase.inc:3396-3423`: `ivEvModified` is set on an IV or EV change, then `pokemon->regeneratePID(pokemon->id32())` runs at `:3420-3422`. **When it can change a Gen III PID:** `Pokemon3FRLG::regeneratePID` (`src/Pokemon/Pokemon3FRLG.cpp:327-329`) → `rerollPID(isShiny(id32), gender(), nature(), abilityPidBit())` (`:197-227`). `gender()` (`:51-57`) and `nature()` are *derived from the PID*. Unown's letter and the Wurmple value (`:215-221`) are taken from the current PID. `shiny` uses the record's own TID/SID. So every pinned trait already holds **except possibly the ability bit**. `abilityPidBit()` (`:299-307`) returns `IV32 bit 31` for a species with two different Gen 3 abilities and −1 otherwise. `Search::satisfies` (`include/Conversion/Gen3PidSearch.h:95-103`) therefore fails, and the PID is rebuilt, **only when the species has two abilities and `(PID & 1) != (IV32 >> 31)`**. The rebuilt PID keeps shiny, gender, nature, Unown letter and Wurmple value, and sets bit 0 to the ability bit. The new PID re-keys the record and changes the substructure order (`recalculateStats()`, `refreshChecksum()`). In every other case (game-generated mons, single-ability species, Unown) it returns at `:223` with no change. **Reachability: not reachable in Gen I–V sessions.** The stat dialog opens only from an editable details page (`:3949-3965`), which the classic guard makes read-only. For Gen IV/V the legacy classes are read-only views whose `regeneratePID` is empty (`include/Pokemon/Pokemon4ReadOnlyView.h:97`, `Pokemon5ReadOnlyView.h:147`). In Gen VII+ sessions the call **is** live and harmful: `Pokemon7LGPE::regeneratePID` rewrites the **encryption constant** (`src/Pokemon/Pokemon7LGPE.cpp:229-239`, `newEC = (oldEC ^ 0x10101010) + species`), and the SWSH/BDSP/LA/SV/LZA overrides (`src/Pokemon/Pokemon8SWSH.cpp:241`, `Pokemon8BDSP.cpp:247`, `Pokemon8LA.cpp:241`, `Pokemon9SV.cpp:240`, `Pokemon9LZA.cpp:240`) run on every IV/EV edit. This refines matrix B-11: for Gen I–V the call is dead code, and the real exposure is Gen VII+. **Decision: ADOPT the removal** (one-line deletion, no behaviour lost; see C-4). |

### A5. `9a9a731`: Gen VI+ parts only (2026-10-20). Everything here is DEFER, see backlog B-LA-1/2, B-LGPE-1..3, B-BDSP-1..2, B-BAG67.

### A6. `f27bd82`: SV Sweet Herba Mystica add-item (2026-10-04). DEFER, see B-SV-1. Gen I–V side effect: `PickerDialog.cpp` hides the items a pocket already holds and says so ("Items already in this pocket aren't listed here", plus a match hint) for `PouchItem/PouchItemG1/PouchItemG3`. Ours: `SharedBagAddFlow` focuses the existing stack instead of hiding it (`tests/test_bag_add_flow_gen15.cpp` header: "A on a held item focuses the existing stack (never a duplicate)"). **ALREADY IMPLEMENTED** (a different but equivalent rule).

### A7. `c671aa4`: Alpha toggle and empty start folder (2026-10-23)

| Field | Finding |
|---|---|
| Alpha (LA, PA8) | New `Pokemon::hasAlphaFlag()` (true for PA8/PA9) and `setAlpha(bool)`. PA8: bit `0x20` of byte `0x16`. **Setting** also writes `HeightScalar 0x50 = WeightScalar 0x51 = Scale 0x52 = 0xFF`, then `resetSizeAbsolutes()` (float height @`0xAC` = `personal.height × p(h)`, float weight @`0xB0` = `personal.weight × p(h) × p(w)`, `p(s) = (s/255)·0.40000004 + 0.8`, computed through a `volatile` so the compiler cannot fuse it into an FMA), then the checksum. **Clearing** writes only the bit; the size stays at 255. The cited rule is PKHeX `LegendsArceusVerifier` / `MiscScaleVerifier` (not in the sparse checkout: **not confirmed**). |
| Alpha (Z-A, PA9) | Byte `0x23` = 1/0. Setting writes Scale `0x4A = 0xFF`. Height `0x48` / weight `0x49` become `0xFF` **only if either was already non-zero** (the game leaves them 0 and HOME copies the scale). Checksum refreshed. |
| UI | Details row 34 "Alpha Yes/No" toggles, sets `hasUnsavedChanges`, and mirrors to the Let's Go party (no-op here). |
| Empty start folder | `FileBrowserDialog.cpp` `directoryExists("")` → false. `f4fd79a` put `Utils::lastFolder(...)` first in the start list. When nothing has been remembered it is `""`, and libnx resolves `opendir("")` against the working directory, so the browser opened in the app's cwd instead of falling through to `sdmc:/PKSE/saves/`. Matrix B-15 covers our latent `PokeBankPaths.h:119-122`. |
| PokeBank NX | `isAlpha()` read-only at `include/Pokemon/Pokemon8LA.h:720` (same bit) and `include/Pokemon/Pokemon9LZA.h:641` (same byte). No setter and no `resetSizeAbsolutes`. |
| Decision | **DEFER** (B-ALPHA-1). The empty-folder part is already matrix B-15. |

### A8. `65dff1d`: Hyper Training (2026-10-26)

| Field | Finding |
|---|---|
| Formats/offsets | One byte with a bit per stat in the order HP, Atk, Def, SpA, SpD, Spe (`HT_*` = 0..5): **PK7/USUM `0xDE`, PB7 `0xDE`, PK8/PB8/PK9/PA9 `0x126`, PA8 `0x13E`** (`include/Pokemon/Pokemon7SM.h:409`, `Pokemon7LGPE.h:612`, `Pokemon8SWSH.h:204`, `Pokemon8BDSP.h:219`, `Pokemon8LA.h:246`, `Pokemon9SV.h:218`, `Pokemon9LZA.h:221` at `a79fe61`). Gens 1–6 report `hasHyperTraining()==false`. PK6's `0xDE` is the Gen 4 ground tile, which the `18c2555` PK6/PK7 discriminator relies on. |
| Rules | `hyperTrainMinimumLevel()` = 100, overridden to **50 for SV and Z-A**. `isHyperTrainingAvailable()` = `hasHyperTraining()`, overridden for **PA8 to `homeTracker() != 0`** (LA sells no Bottle Caps). `getHyperTrainBlockedReason`: not available → "Not available"; the level is `level()`, or from EXP when the box record has no level byte; level < minimum → "Needs Lv. N"; `IV >= maxIV()` → "IV is already 31". Setting a flag requires an empty reason or "Allow illegal values". Clearing is always allowed. Raising a trained IV to the maximum drops the flag (unless illegal values are allowed). |
| Effective IV | `effectiveIV(i)` = `maxIV()` when trained, else the stored IV. Every `recalculateStats()` must read it (from the earlier PKSE fix, documented in manual ch. 06: a Meowscarada with IV 20 in HP/Def/SpD and flags `0x15` is stored at HP 356 / Def 176 / SpD 176; recomputing from raw IVs gave 345/165/165). `setHyperTrainFlags()` does **not** recompute stats; the stat editor calls `recalculateStats()` + `refreshChecksum()` after a flag change. Stored IVs stay raw, because legality matches those. |
| UI | The stat dialog gains a third row "Hyper Trained Yes/No". Left/Right or a second tap toggles it, and L/R/ZL/ZR are masked on that row. The note reads "Plays as N" or the blocked reason. The details page shows "HT: Hyper Trained, plays as 31" and the party card adds "HT" after the IV. `selectedStat` is now the HT bit order. |
| UI harness | `testHyperTraining` in `tools/harness/uihost/uitest.cpp`: it sets level 100, IV 10 and flags 0 (and a HOME tracker for LA) on the first boxed non-egg mon. It then sets a flag by button and by tap and requires: only the flag byte changes, the stored IV is unchanged, the stat follows, the checksum holds. It also checks Cancel keeps nothing, IV→max drops the flag, below the level the row refuses until illegal edits are on, and the details/party markers. `runUI` now walks **BDSP and SV** for every Hyper Training-capable format (before, they were opened only for Add All Items). |
| Upstream correctness | Bit order and offsets match my reading of PKHeX (`IHyperTrain`, `PK7.HyperTrainFlags@0xDE`, `PK8/PB8/PK9 @0x126`, `PA8 @0x13E`). **Not confirmed** against the pinned PKHeX, whose sparse checkout lacks these files. The Z-A level-50 rule is **not confirmed**: PKHeX's `GetHyperTrainMinLevel` is cited for SV only. |
| PokeBank NX | No HT accessors at all (`grep hyperTrain\|effectiveIV` = none). Our legacy Gen VII+ `recalculateStats()` therefore computes from raw IVs, which is the bug the manual describes. That matters only where Gen VII+ workspace edits exist (see C-3). |
| Decision | **DEFER** (B-HT-1). The effective-IV half is a save-safety prerequisite before any Gen VII+ stat recalculation is written. |

### A9. `f4fd79a`: favourite folders and remembered last folder (2026-10-23)

| Field | Finding |
|---|---|
| Settings keys | `lastSaveFolder=<path>`, `lastImportFolder=<path>`, `favoriteFolder=<path>` (repeated, one per line, in the order added). Written only when non-empty, after the existing keys. |
| Storage format | `key=value\n` in `settings.cfg` (or `setSettingsFileOverride` for the self test). The value is everything after the first `=`, so paths containing `=` survive. `loadSettings` clears all three first (so a re-read replaces rather than appends) and reads lines into `char line[1024]` (was 128). `saveSettings` is **`fopen(path,"w")` + `fprintf`, not atomic**: a power cut during the write loses every setting, favourites included. |
| Path normalisation | `directoryLength` trims trailing `/` except the root's (`sdmc:/`). `sameDirectory` compares trimmed prefixes. `toggleFavorite` stores the trimmed current directory, or the stored spelling if an equivalent favourite already exists. The comparison is **case-sensitive** (FAT is not), so `sdmc:/Roms` and `sdmc:/roms` can both be favourites. |
| Dedup | On load: `favoriteFolder` is skipped if empty or **exactly** equal to an existing entry (`isFavoriteFolder`, no slash normalisation). Hand-edited `a/` and `a` therefore both load. In the UI, `toggleFavoriteFolder` removes on an exact match of the normalised spelling, else appends. |
| Missing directories | `lastFolder`: the browser falls through its start list (`lastFolder`, then `sdmc:/PKSE/saves/`, `sdmc:/PKSE/`, `sdmc:/roms/`, …) to the first that exists. Before `c671aa4`, an empty `lastFolder` was treated as existing. A missing favourite opened from the list gives "That folder isn't on the SD card. ZR removes it from your favorites." and is **not** auto-removed. |
| Last folder | Separate for the save browser (shared by the save pickers) and the import browser. `setLastFolder` saves only on change. |
| Line buffer | 128 → 1024. A line over 1023 chars is still split, and its tail is parsed as its own line. If the tail contains `=`, its prefix is taken as a key. |
| PokeBank NX | `src/Utils/Settings.cpp:28` `char line[128]` (matrix B-14). Saves go through `AtomicTextFile::replace` (`:85`), and an interrupted write is recovered (`:22`), which is stronger than upstream. `include/Utils/PokeBankPaths.h:119-122` `directoryExists` has no empty guard (matrix B-15). We have no file browser. |
| Decision | **ALREADY DECIDED** (matrix §7: NOT APPLICABLE today; B-14/B-15). Added detail: if a browser is ever added, store favourites through `AtomicTextFile` (already ours), normalise case and trailing slash before dedup, refuse lines over the buffer instead of splitting them, and validate each path with `PokeBank::Paths::isOwnedPath`/SD-root rules before opening it. |

### A10. `d2ad323`: Add All Items, exact algorithm (2026-10-24)

**Entry:** ZR in the Items list → dialog (amount 1..999, default 999; ±1/±10/±100; X toggles "Include Key
Items", default **off**) → A runs `TrainerViewScreen::addAllItems(amount, includeKeyItems)`.

**Algorithm** (`src/UI/TrainerViewScreen.cpp` at `d2ad323`, after `itemMaxCount`):
1. `amount <= 0` → nothing.
2. For every pouch index `c` in `trainer.items`, for every `id` in `Names::getPouchItems(group, c)` (the
   generated PKHeX legal spans, the same list the Add Item picker offers):
   - skip if `!includeKeyItems && Names::isKeyItem(group, id)`;
   - `cap = itemMaxCount(c, id)`; skip if `cap <= 0`;
   - if the pouch already has an entry with that id (the **first** match only): `new = min(old + amount, cap)`.
     If `new <= old`, skip. **It never lowers a count**, so an over-cap stack such as a real Gen 1 save's 158 is
     left alone. If `old == 0`, `isNew = true`;
   - else, if `pouch.size() < pouchCapacity(c)`: append `{id, min(amount, cap), isNew=true, fav=false}`;
   - else count it as "no room" (the user sees "N items did not fit").
3. `hasUnsavedChanges = true` if anything changed.

**`itemMaxCount(c, id)` (per-item cap):** `pouchMax = pouchItemMaxCount(c)`, then:
- **HM → `min(pouchMax, 1)`**: GSC ids 243–249; any Gen 3 group ids 339–346; DP/PT/HGSS ids 420–427 **or 737**.
  737 is the ORAS HM07 id and does not exist in Gen 4, so that term is dead code. Gen 5 HMs (420–425) are not
  listed, but Gen 5's TM pouch max is already 1.
- SM/USUM Z-Ring (797) → 2. SWSH TM pouch non-TR (TR = 1130..1229) → 1. SV Ingredients pouch picnic accessories
  (2311–2400 or 2417–2437, excluding picks 2334–2342, 2385–2394 and 2548) → 1. Z-A Cherished Ring 2612 and
  Colorful Screw 2619 → 0 (never given).
- `Names::isKeyItem(group, id)` → `min(pouchMax, 1)`.
- else `pouchMax`.

**`pouchItemMaxCount(c)` (per-pouch):** FRLG: Key 1, else 999. LGPE: TMs 1, else 999. SWSH, BDSP, SV: Key 1, else 999.
PLA: Key and Recipes 1, else 999. Z-A: Key, TMs and Mega Stones 1, else 999. **Every other group uses
`trainer.getItemMaxCount(c)`, or 999 if 0.** RBY is 99 for all pouches. GSC is TMHM 99, Items 99, Key 1, Balls 99,
PC 99. DP/Pt/HGSS are Items 999, Key 1, TMHM 99, Mail/Medicine/Berries/Balls/Battle 999. BW/B2W2 are Items 999,
Key 1, TMHM 1, Medicine/Berries 999. XY/ORAS are Items 999, Key 1, TMHM 1, Medicine/Berries 999. SM/USUM are
Key 1, TMHM 1, Z-Crystals 1, else 999. **RSE has no override, so every pouch gets 999.**

**`pouchCapacity(c)` (slots):** FRLG and RSE (RS 20/20/16/64/46/50, E 30/30/16/64/46/50) come from their
PouchInfo tables. LGPE uses PouchInfo7LGPE.maxSlots. **SWSH uses `.maxCount`** (it reads the per-stack field as
a slot count). PLA uses the trainer's capacity. Id-indexed formats (BDSP/SV/Z-A) use 1<<20. The rest use
`trainer.getItemPouchCapacity(c)`.

**Key-item tables** (`isKeyItem`, generated by `tools/gen_itempouches.py` from PKHeX spans, except Gen 1).
RBY: 36 ids from pret `KeyItemFlags` plus HM01–05 (5–9, 21–28, 31, 41–45, 48, 63, 64, 69–74, 76–78, 196–200;
Exp. All 75 excluded). GSC uses **`KeyCrystal` for G/S too**. DP and Pt use `Key`/`KeyPt`. HGSS uses `KeyHGSS`.
BW/B2W2/XY/ORAS/SM/USUM use `Key`. FRLG uses `ItemStorage3FRLG.Key` (now including Meteorite 280). RSE uses
`ItemStorage3E.Key` for Ruby/Sapphire too. LGPE uses only the `Key` half of its mixed pouch. SV uses
`Event`. Item-availability tables: none beyond the legal spans (no per-version release filter, so event-only
items such as RSE Enigma Berry 175 or Gen 4 event key items like Azure Flute 454 are offered). Key items are
skipped only while the switch is off.

**Per-generation disagreements with PKHeX/pret** (for designing our safe Top Up):

| Gen | Upstream | Reference | Verdict |
|---|---|---|---|
| I | Bag/PC 99; Exp. All not a key item | pret `KeyItemFlags`; PKHeX `PlayerBag1` 99 | Agrees. **Ours** caps Exp. All at 1 (`src/Inventory/ClassicInventoryCatalog.cpp:53` `kG1Keys`, matrix B-08) |
| II | Stack 99; Key list = Crystal's for G/S; first matching entry only | pokecrystal `MAX_ITEM_STACK` 99, and `engine/items/items.asm:171-215` **spills an overflow into a new slot**, so one id can legally occupy two slots | Mostly agrees. Top-up must treat *all* slots of one id, not only the first. A Crystal-only key id in a G/S pouch is skipped (harmless) |
| III RSE | **999 for Items/Balls/TMHM** | pret `pokeemerald/include/constants/items.h:453-455` `MAX_BAG_ITEM_CAPACITY 99`, `MAX_PC_ITEM_CAPACITY 999`, `MAX_BERRY_CAPACITY 999`; ours `ItemQuantityPolicy.cpp:206` (99/999/999) | **Upstream wrong** (matrix §3, "do not port") |
| III FRLG | 999 | PKHeX `PlayerBag3FRLG` 999 (ours `ItemQuantityPolicy.cpp:208-210`) | Agrees |
| III | HM 339–346 → 1 | pret | Agrees |
| IV | TMHM 99, HM 420–427 → 1, plus a dead `737` term | PKHeX `PlayerBag4*` TMHM 99 (ours `:211-215`) | Agrees (dead code only) |
| V | TMHM 1 | PKHeX `PlayerBag5*` 1 (ours `:216-219`) | Agrees |
| VI–IX | as listed above | PKHeX `PlayerBag6/7/7b/8/8a/8b/9/9a` (**not confirmed**, not in the sparse checkout) | backlog B-BAG-1 |

**Design for our "Top Up existing stacks" (matrix B-10)**, using our own policy rather than upstream's tables. For each
staged pocket entry with `count > 0`, look up `quantityRule(game, pocket, id)`
(`src/Inventory/ItemQuantityPolicy.cpp:281-330`). If `!rule.valid || rule.unique || !rule.editable`, skip.
Otherwise `new = max(old, min(old + amount, rule.maximum))`. Never create a slot, never touch key items, HMs,
Gen V TMs or count-0 entries, and never lower a count. For Gen II, apply it to every slot of the id (pret overflow).
Stage everything as one transaction through the existing staged Bag writers, so Discard restores the source.
**Decision: IMPROVE** (safe subset only). Full Add All stays **REJECT** (matrix).
Tests: (1) a Gen 3 Emerald Items stack of 50 tops up to 99 and its PC stack of 50 to 999; Berries to 999;
(2) a Gen 1 stack of 158 stays 158; (3) Gen 2 with an id in two slots (99 + 5): only the 5 rises, to 99;
(4) key items, HMs and Gen V TMs are unchanged byte for byte; (5) a full pocket gets no new slot; (6) export,
re-parse and diff: only count bytes (and the Gen 3 XOR-keyed counts in E/FRLG) differ; (7) Discard leaves the source
byte-identical.

---


## Inherited-bug candidates as found (before the fixes above)

Gen I–V: **no new reachable candidate was found** beyond the matrix's latent B-14/B-15. The two Gen I–V-looking
leftovers (the legacy `regeneratePID` call and the discard-flag rollback) sit behind the classic-session guard
(`include/UI/ClassicGenericUiGuard.h:35`; `TrainerViewScreenBase.inc:2002-2005, 3614-3620`), which makes the
details page read-only, so they cannot run for Gen I–V.

The candidates below are in the inherited **Gen VII–IX** classes. They are reachable when a Switch-title backup
workspace is saved. Whether the current product UI exposes those sessions is **not confirmed**.

| ID | Code path | Failing input | Expected vs actual |
|---|---|---|---|
| **C-1** LA research counts inflate on every write | `Save::buildWorkspaceImage` PLA branch (`src/Save/GetSaveFileContents.cpp:279-288`) → `Trainer8LA::updatePokedexBlock` (`src/Trainer/Trainer8LA.cpp:586`) → `registerMon` for every party/box mon (`:684-686`) → `+0x0A` catch count `+1` (`:665-667`). `buildWorkspaceImage` is also called to build the post-image of a cross-store move (`src/UI/TrainerViewScreenBase.inc:961`) | A Legends: Arceus workspace with N stored Pokémon; save the working copy (or make one Bank↔save move) | Expected: research unchanged for Pokémon already present. Actual (by code reading): each species' "Number caught" rises by its stored count on every build. This is PKSE issue #135, fixed upstream in `9a9a731`. Not run on a real save here |
| **C-2** Let's Go unedited write rewrites storage tail and dex sizes | Same builder, GG branch (`GetSaveFileContents.cpp:228-235`) → `Trainer7LGPE::updateBoxBlock` zeroes every slot past the packed list (`src/Trainer/Trainer7LGPE.cpp:426-430`); `updatePokedexBlock` folds partner forms (`:760`) and writes the height as one byte plus a 0 marker (`:663-664`) | Any Let's Go workspace, saved with no edit | Expected: byte-identical except the dex. Actual: empty slots become zeros (upstream measured 239 164 changed bytes on one save). If upstream's u16 = 2× reading is right, height records are halved, and a partner Pikachu/Eevee is written into form 0's size records. The game impact of zeroed blanks is **not confirmed** |
| **C-3** Gen VII+ stats recomputed without Hyper Training | Any legacy Gen VII+ path that calls `recalculateStats()` (stat dialog `TrainerViewScreenBase.inc:3396-3423`, setters) | A Hyper Trained SV/SWSH/LGPE/USUM Pokémon whose IVs or EVs are edited | Expected: stats from `effectiveIV` (e.g. HP 356). Actual: stats from raw IVs (e.g. 345), written back. Code reading only |
| **C-4** Legacy stat dialog rerolls PID/EC on Gen VII+ and the discard flag sticks | `TrainerViewScreenBase.inc:3420-3422` `regeneratePID` after any IV/EV edit. For LGPE it rewrites the EC (`src/Pokemon/Pokemon7LGPE.cpp:229-239`); SWSH/BDSP/LA/SV/LZA `regeneratePID` overrides run likewise (behaviour per class **not inspected**). Separately, `restoreEditTarget` (`:1475-1483`) does not restore `hasUnsavedChanges` | Edit one IV of a Let's Go Pokémon in the details page, then Save; or edit and close without Save | Expected: only the IV and stats change; discarding clears the dirty state. Actual: the EC changes (`(EC ^ 0x10101010) + species`), and after a discard Leave still prompts to save. **ADOPT:** delete the call (matrix B-11) and restore the flag |
| **C-5** SV added items lack the OBTAINED bit | `src/Trainer/Trainer9SV.cpp:849` sets only bit 0 on a newly written record | Add an item the save never held (upstream's report: Sweet Herba Mystica) to an SV workspace | Expected (per upstream and its PKHeX citation): flags `|= 0x04`. Actual: bit 2 clear. Whether the game hides such an item is **not confirmed** |
