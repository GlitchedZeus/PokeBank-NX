# Round 11 engineering intake — Gen I–V repair campaign

**Source:** the owner's completed Round 11 worksheet (`docs/hardware/ROUND11_OWNER_WORKSHEET.md`): 48 tests, 34 PASS / 13 FAIL / 1 NOT TESTED; essential H01–H17 15 PASS / 2 FAIL. Every entry, including comments on PASS items and the "Extra bugs and requests", was read.

**Build tested:** not recorded in the worksheet. Inferred from the checklist structure to be the Round 10 NRO `3634d1e6` (SHA-256 `b6cd0b89…c9e8cd9`); unverified. No screenshots were attached.

**Method:** one read-only investigator per finding area traced the root cause in the Round 10 application code (identical tree to `3634d1e6`), and an independent skeptic re-opened every cited `file:line`. **All 19 root causes were confirmed**; skeptic corrections and missed paths are recorded below and are part of the fix scope.

Classification codes: **1** reproduced software bug · **2** cross-generation UI inconsistency · **3** newly requested behaviour · **4** known missing v1.0 functionality · **5** safety concern · **6** hardware test needing independent confirmation.

## 1. Every finding, with the owner's original words

| Test | Result | Owner's words (verbatim, abridged only where marked …) | Class | Priority | Area |
|---|---|---|---|---|---|
| H04 | FAIL | "top up and add only fill the current menu, the bag categoties arnt filled which they should be it and option of add all i think" | 2, 3, 6 | P0 | H04-H34 |
| H07 | PASS (with request) | "this needs to be added for all gens" | 3 | P1 | CREATE-VALIDATION |
| H12 | FAIL | "this doesnt work i cant see what is selected and nothing in the footer and nothing on screen changes for it, this needs worked on" | 1, 2, 6 | P0 | H12 |
| H13 | PASS (not evidence) | "i guess this passes i didnt test a japenese save but when i swap languages it has the proper text" | 6 — recorded as NOT TESTED for Japanese saves | — | H27 |
| H22 | FAIL | "we still dont have a number in the calculated column for special calculated stat, add it when i swithc unowns i get a pop in middle bottom about stat changes and i dont need it … i also get the pop uo for rerolling the de/ev/iv/ev's can we fix this for all gens … when im in gen 5 and go to choose shiny … i get a pop up menu instead of a inline choose" | 2, 3 (+1 for Gen V shiny) | P1 | H22, H29 |
| H23 | PASS (with request) | "can we fix for all gens when we pick up a pokemon can we change the drop control to A … can we also fix the footer and all the help menu" | 3, 2 | P1 | H23 |
| H25 | FAIL | "gen leaves info in the left side panel ffrom the previous pokemon … i had a pikachu … met location to trohpy arden and changed species to arbok and arbok has the same met location and pokerus stat aswell" | 1, 2, 3 | P0 | H25 |
| H27 | FAIL | "language changes but the top names doesnt change like in gen 1-4 … korean letters … look like this "[][][][][]" … keyboards … japenese and korean and all languges … feedback ??" | 1, 2, 3, 4, 6 | P2 | H27 |
| H28 | PASS (with request) | "… when the editor is opened the new pokemon … doesnt come with its default moves and alson when we switch species i dont want it to come with moves auto filled in … a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves …" | 3, 2, 5 | P1 | CREATE-MOVES, CREATE-VALIDATION |
| H29 | FAIL | "gen 5 in the editor view doesnt have inline selcetion for shiny option, it opens a menu and warns about pid, this needs changed to be inline like eveery other gen (1-4)" | 1, 2 | P1 | H29 |
| H31 | FAIL | "gen 1 works perfect, gen 2 … revert back to the action menu … gen 3 has the numpad come up … even when i entered a number on the numpad on gen 3 it didnt work … gen 4/5 has same problem as gen 3 … look at PKSE … hover over box feature … press A to confirm the clone" | 1, 2, 3, 6 | P0 | H31 |
| H32 | NOT TESTED | "dont know how to test this" | 5, 6 | P0 | H32 |
| H34 | FAIL | "the pc items doesnt get filled with top up feature and also gen 1 doesnt seems to work … every pocket and bag categotry and pc items, key items dont worrry about tho cause that could mess up users saves" | 2, 3, 6 | P0 | H04-H34 |
| H38 | PASS (question) | "how are we suppose to move a gen 1 pokemon  to like gen 5 ?? does it have to go through bank first or how we doing that ??" | 4 | Vault | VAULT |
| H39 | FAIL | "can we add a cancel feature … B goes to the parent directory … we need to add a buttoin that is cancel … can we make it "-" add the footer for all needed spaces and gens and app wide" | 3, 2 | P1 | H39 |
| H40 | FAIL | "can rename a box inside the save its doesnt allow me to select it, the only boxes i can rename are bank boxes …" | 4, 2 | Vault | VAULT |
| H41 | FAIL | "ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet" | 4 | Vault | VAULT |
| H42 | FAIL | "not supported yet" | 4 | Vault | VAULT |
| H43 | FAIL | "gen 1 does not have "move to vault or bank or what ever"" | 4 (the test itself expects NOT IMPLEMENTED as an observation) | Vault | VAULT |
| Extra 1 | — | "Gen 1 bag items, fill all doesn't fill all bags, only does current bag not tm or items" | 2, 3 | P0 | H04-H34 |
| Extra 2 | — | "leaf green and fire red on gba so gen 3 dont have a working met location for pokemon … i thik it is reading RSE for species … i dont think i can get a rayquaza in leaf green or fire red." | 1, 2, 4, 6 | P0 | FRLG |
| Extra 3 | — | "gen 5 quick view graph … doesn't work and isn't updated with the graph that is in the editor … it also looks smaller in the quick view." | 1, 2 | P1 | G5-RADAR |
| Extra 4 | — | "… X to add and change it to import/export instead like if the cursor is on a pokemon X would say export but it its on a blank space it would say import, makes this app wide …" | 3, 2 | P1 | X-IMPEXP |
| Extra 5 | — | "The quit option … has a highlighted option in the Y exit option, it also has a footer which isn't needed … get rid of "stay" … In the species picker … 1 photo … Gen 1/2 have moves when editor is opened, it should be blank …" | 3, 2 | P1 | QUIT, SPRITE, CREATE-MOVES |
| Extra 6 | — | "gen 1/2 have moves when pokemon editor comes up, pikachu has growl and thunder shock … blank canvas … the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add … universal rule." | 3, 2, 5 | P1 | CREATE-MOVES, CREATE-VALIDATION |
| Extra 7 | — | "gen 2/3 doesnt have 'move or review pending changes in the action menu … they should all have "view, edit,clone,move,move to vault, release, origin and legality, export to file, reviewing pending changes, cancel"" | 1, 2, 3, 4 | P1 | ACTIONS |
| Extra 8 | — | "pkse has a nice search function in the bank … could we add just before 1.0." | 4, 3 | Vault | VAULT |

The 34 PASS results (H01–H03, H05–H11, H13–H21, H23, H24, H26, H28, H30, H33, H35–H38, H44–H48) are **not** permission to change working behaviour; every repair keeps them and adds regressions for the paths it touches.

## 2. Shared root causes (one fix each, inherited by every generation)

| Shared cause | Findings it explains | Shared owner |
|---|---|---|
| One carry/placement layer exists (`ClassicPackedMoveOverlay.inc`) but Clone does not use it, and it places with Y | H31 (Gen II menu reopen, Gen III–V keypads, wrong default slot), H23 (A to place), H32 (occupied-destination behaviour) | `src/UI/ClassicPackedMoveOverlay.inc` + a host-testable rules header |
| No shared species-change / Create-draft policy: each generation keeps or resets history and moves on its own | H25 (stale Met/Pokérus), H28 + Extra 5/6 (auto-filled moves), H07 + Extra 6 (required fields only in Gen V) | new `SpeciesChangeHistoryPolicy.h`, `SpeciesChangeMoves.h`, a shared Create required-fields check |
| Routine successful edits post status toasts; no shared toast rule | H22 (Unown, reroll, shiny toasts) — same class as the earlier Gen V Language toast (CG79) | shared editor status policy |
| Gen V routes Shiny through the PID-change confirmation (`offer()`), unlike Gen I–IV inline cycling | H29, H22 note | `Gen5SharedReadOnlySurface.inc` ShinyRow |
| Box actions are built separately per generation (four Gen II builders) with no shared capability model | Extra 7 (missing Move / Review), Extra 4 (X = Import/Export) | `SharedPokemonEditorContract.h` action model |
| Footer/Help omit real controls in modal lists | H12 (no Deselect/Help/B), H39 (no Cancel), QUIT (redundant Stay), H23 (Y-place wording) | per-screen footer helpers |
| Closed per-game encounter-template lists fail closed when a game has no templates | Extra 2 (FireRed/LeafGreen Met Location); skeptic: Gen II Crystal has the same pattern | Gen III / Gen II encounter pickers |
| Gen V names are decoded ASCII-only; Korean glyphs absent from the loaded fonts | H27 | `Gen5NativeText.h` / `Gen5TrainerName.h` decoder, font loading |

## 3. Leads from independent API sessions (unverified until reproduced)

| Lead | Status in this campaign | Owner |
|---|---|---|
| Gen IV Clone defaults to the occupied source slot | **Confirmed** in code (H31): Gen III and Gen IV keypads default to the source slot | Round 11 (fixed by the shared Clone placement) |
| Gen V quick-view radar scaling | **Confirmed** (G5-RADAR) | Round 11 |
| Import browser lacks a dedicated Cancel | **Confirmed** (H39) | Round 11 |
| Gen V shiny UI inconsistency | **Confirmed** (H29) | Round 11 |
| Misleading legacy green legality wording | To verify during the Actions/Origin & Legality pass; legality UI must never imply a clean verdict (CG48) | Round 11 (UI wording only) |
| Gen VI–IX registry marks untested titles OwnerAccepted | **Confirmed, narrower:** on `development/gen6-plus` every *Gen I–V* row is `OwnerAccepted` (false while #313 is open); Gen VI–IX rows are `NotDeviceTested` | Gen VI–IX branch owner (not changed here) |
| Bank silently drops unsupported records; PK3 length; oversized box names; backup growth; recovery ordering/torn files; one damaged record blocks loading | Partly corroborated (VAULT investigation: `Bank::hasChanged()` goes through `serialize()`) | Separate Vault plan |
| Gen IV EV/EXP false clean; incomplete coverage promoted to Complete | Recorded on #329 | Legality owners (#314 / #327 / #328) |

## 4. Decisions applied in this campaign (owner may override)

- **H25 Edit vs Create.** A new Pokémon draft starts clean, and a species change in Create resets Pokérus and any Met Location that is not valid for the new species. On an **existing** Pokémon, its own history (Met Location, Pokérus) is legitimate and is **kept**; nothing is silently cleared.
- **Moves.** New drafts start with no moves in every generation, and choosing a species never auto-fills moves. Existing Pokémon keep their moves; H28's passing behaviour (incompatible moves are cleared on a species change) stays. Keep/Add requires at least one move because a moveless Pokémon is not usable in game.
- **Required fields.** Create requires the fields the native format and writer need: Met Location in Gen III–V (Gen V already does), at least one move everywhere. Gen I has no met data and Gold/Silver have none; Crystal caught data stays optional (legitimately blank for traded Pokémon). The suggested "level 100 should have 1–2 moves" rule is **not** a game rule and is not invented.
- **Actions menu.** Every generation offers the same order — View, Edit, Clone, Move, Release, Origin & Legality, Export to file, Review pending changes, Cancel — showing only actions that really work. Move to Vault is not shown until the Vault exists.
- **Recovery files.** The global list stays (protection is computed over the full listing); focus starts on the opening game's group, preferably its first removable copy.
- **Box controls.** Y picks up, A places, B cancels (all generations). X exports on an occupied slot and imports on an empty slot; Create stays in Actions.

### Owner safety clarifications (October 31) — these override section 4 where they differ

1. **Create validation is exact-game / format-aware.** "At least one move" is an **app policy** for a usable Pokémon, not a legality proof, and is labelled as such. Met Location is required as an **explicit choice** in the exact game's own location list. Never require a *nonzero* value: a real native Pokémon can legitimately carry location 0 or no recorded location (for example Gen III RSE index 0, Gen IV/V special or "----" entries, traded or event records). The check must use each game's native semantics, never one universal rule.
2. **H25 history vs stale editor state.** On an existing Pokémon, recorded history (Met Location, Met Level, Pokérus, ball, egg/fateful data) is **preserved**. When it is incompatible with a newly chosen species, it is **flagged** in the editor and in Review, never silently deleted. New drafts reset properly and never inherit another record's state; the editor panels always re-read the record being edited, so nothing stale is shown.
3. **Recovery files.** Focus may start on the current game's backups, but **nothing starts selected for deletion**. All protection (newest copy, Bank, unverified) and both confirmation steps stay exactly as they are.
4. **Scope of the Round 11 repair NRO:** Gen I–V only. No unfinished Vault/Bank features and no Gen VI–IX code are integrated.

## 5. Separate from the repair candidate

- **Vault / Bank (H38, H40–H43, bank search):** a v1.0 feature with its own reviewed plan and branch; not part of the Round 11 repair NRO.
- **Legality engine:** isolated (#314 / #327 / #328, tracker #329).
- **Gen VI–IX application:** isolated on `development/gen6-plus` until Gen I–V is accepted.

## 6. Per-area evidence (investigation + skeptic check)

### H12 — Recovery files UI (P0)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 6 Hardware test needing independent confirmation  ·  **Confidence:** medium  ·  **Skeptic confirmed root cause:** yes
- **Generations:** All of Gen I–V equally. The Recovery files view is one Home modal in SaveSelectScreen and does not depend on generation: emulator Gen I–III saves, Gen IV/V .sav/.dsv files, Switch-save DurableFile siblings and Gen V account export records all go through the same rows, draw and input code. No generation currently behaves correctly on the points below. Generation-specific editors are not involved.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Open path: Backups footer X -> openRecoveryFiles() (src/UI/SaveSelectScreen.cpp:4213 -> 3235-3267). It lists every backup and export record of the profile, DurableFile .previous/.failed/.tmp files under backups/account-<profile> (depth 5), and every Bank file. It is NOT limited to the game whose Backups screen opened it: emuBackupGameId is never used in 3235-3267. Rows are grouped by source string, newest first (include/UI/RecoveryManagementModel.h:330-375). Focus starts on the first item row (SaveSelectScreen.cpp:3263-3265). That row is the newest copy of the first group, which classify() always marks Protected (Model.h:228-236).

Drawing (SaveSelectScreen.cpp:6102-6151): every row, headings included, is drawn by BackupBrowserChrome::drawRow(title, detail, badge, i==recoveryIndex) (include/UI/BackupBrowserChrome.h:18-25). The focus flag changes only the box outline: a 2px PartyNumber outline instead of a 1px Divider (include/UI/SharedBoxedRows.h:65-74). Title colour, fill and badge colour do not change. The badge is always drawn in Colors::TextSecondary Caption (BackupBrowserChrome.h:24). A picked item differs from an unpicked one only by the right-aligned caption word "Selected" instead of "Removable" (Model.h:371). There is no check mark, fill, colour or count on the row.

Footer (6139-6150): {D-pad Choose}, {A "Select" or "Why kept"}, then {Y "Remove selected (n)"} only when picked>0, then {B Back}. A never reads "Deselect" on a picked row. There is no Help entry, and minus is not handled in this overlay (4166-4206 handles only B/Up/Down/A/Y). On a heading row the footer still says "A Why kept" (removable=false because item<0).

Input (4166-4206): A on a Removable row toggles it and rebuilds rows (4187-4192). A on a Protected row sets recoveryNotice = verdict.reason (4194). The row's detail line already shows that same text (Model.h:369), so on screen the toast only repeats what the row says. Y with no selection sets the notice "Select removable copies with A first..." (4197-4199). Y with a selection opens RecoveryConfirm (4200-4203).

Touch: a touch-down on any overlayRect sets recoveryIndex = r.idx (3647), including heading rows. A tap then adds A (3679, 3683). The A handler ignores headings (4187 `row.item >= 0`), so focus lands on a heading and nothing happens. The D-pad step() at 4175-4180 never lands on a heading.

Confirmation (3464-3496, 6152-6185): HomeQuit::decide gives Stay for A or B, and A/B win over Y in the same frame (include/UI/HomeQuit*.h:26-31). Stay returns to RecoveryFiles with the selection kept. The footer reads "Y Remove / A Cancel" and never names B, although the body text says "A or B keeps them".

Host test tests/test_ui_recovery_files_host.cpp:80-172 drives the real screen and passes these mechanics, but it checks text only (HostUi::texts). Nothing tests whether selection or focus is visible.

**Root cause.**

No logic bug in toggle, plan or delete was found. The host harness proves A toggles, Y confirms and B keeps everything. The failure is in presentation plus a starting state where nothing can be selected:

(1) Picked state is almost invisible. RecoveryManagement::rows() encodes the pick only as badge text (include/UI/RecoveryManagementModel.h:370-371). BackupBrowserChrome::drawRow has no "picked" parameter and draws every badge in the same TextSecondary caption (include/UI/BackupBrowserChrome.h:18-25). The only per-row state the shared box shows is focus, a thin outline (include/UI/SharedBoxedRows.h:65-74). SharedBoxedRows has no style for a multi-select "marked" row, and no multi-select style was found anywhere in include/UI or src/UI.

(2) Focus starts on a row where A cannot select. openRecoveryFiles focuses the first item (SaveSelectScreen.cpp:3263-3265), which is always the newest, protected copy of its group (Model.h:228-236). There A only re-shows the row's own detail text as a toast (SaveSelectScreen.cpp:4194 vs Model.h:369), the footer keeps "A Why kept", and Y is hidden (6146). If the owner's older copy was also Protected (for example unverified, "not the size its manifest recorded", or holding extra files: include/UI/RecoveryManagementFs.h:217-221, 246; Model.h:219-226), then nothing on screen or in the footer changes and Y only shows a toast. This matches "nothing in the footer and nothing on screen changes". Which verdict the owner's real copy had cannot be determined without their SD tree (needs hardware log or confirmation).

(3) The list is not limited to the game the owner opened it from (3235-3267 never uses emuBackupGameId). On a real SD card the game's older backup can sit below other games' groups, Switch-save .previous files and Bank files, with focus starting elsewhere.

(4) Touch can put focus on a non-selectable heading (SaveSelectScreen.cpp:3647/3679). A then does nothing silently (4187) while the footer advertises "A Why kept" (6141-6145).

(5) Footer and Help gaps: no "Deselect" label, no Help entry or minus handler in RecoveryFiles (4166-4206; Help exists only for GameWorkspace and Home at 3842, 4372, 4535), and the confirmation footer omits B (6183-6184).

**Shared owner.**

The pure model include/UI/RecoveryManagementModel.h, through Row and rows(). It should expose structured per-row state (heading, picked, removable, focusable) instead of badge text only, plus a footer-hint helper. The row style for a picked or marked item belongs in include/UI/SharedBoxedRows.h (State gains a `marked` field), used through include/UI/BackupBrowserChrome.h drawRow. Every list that uses the boxed-row style then gets the same visible multi-select look. The SaveSelectScreen.cpp RecoveryFiles input, draw and footer branches should only consume those helpers.

**Proposed fix.**

Minimal, presentation-only. No change to classify, buildPlan or removePlanned.

1. Model: add `bool picked, removable, heading` to RecoveryManagement::Row, filled in rows(). Add a pure `footerHints(const Row&, size_t picked)` returning {Choose, A: "Select"/"Deselect"/"Why kept" (none on a heading), Y: "Remove selected (n)" only when picked>0, "-": Help, B: Back}. Add a pure `initialFocus(rows, preferSource)` that prefers the first Removable row of the opening game's source(s), and falls back to the first item row.

2. Shared style: add `bool marked` to SharedBoxedRows::State and draw it clearly: a filled accent tint or a left accent bar plus a check glyph, different from the focus outline. Add a `marked` argument (default false) to BackupBrowserChrome::drawRow, and draw the badge in an accent colour when marked and dimmed when protected. Draw heading rows unboxed, as section labels, so they do not look selectable.

3. SaveSelectScreen.cpp: pass row.picked into drawRow (6120-6121). Do not publish overlayRects for heading rows (6122), or move a touch on a heading to the next item, so a tap can never focus or "activate" a heading (3647/3679). Use footerHints for the nav bar (6139-6150). Add a minus/Help handler with a short help text. In openRecoveryFiles, pass the opening game's listing sources (emuBackupListings[*].sourcePath) so focus starts on that game's group (preferably its first Removable row), or add a game-scoped filter with a "Show all" toggle. Make the toast for A on a Protected row say why the row cannot be selected ("Protected: <reason>. Only Removable copies can be selected.") and show a short "Nothing here can be removed" line when the listing has no Removable item. Change the confirmation footer to name B too (for example confirmFooter Y Remove / B Keep, plus A), to match "A or B keeps them".

4. Optional: show a header count "n selected" next to the title.

**Risks.**

Deletion safety is sound today and the proposed fix must not touch it. Protected items cannot be deleted. RM::toggle refuses non-Removable items (Model.h:281-287). buildPlan keeps only selected Removable items and never empties a source (Model.h:302-321). removePlanned refuses items that were protected at review, re-enumerates and re-classifies right before each removal, and checks fingerprint, members, sizes and source and the physical path (RecoveryManagementFs.h:376-427). Bank files are protected by kind and by location (Model.h:182-183, 207-210). In RecoveryFiles, B only calls openEmulatorBackups (SaveSelectScreen.cpp:4169). In the confirmation, B or A gives Stay, which wins over a Y in the same frame (HomeQuit decide). So B after Y cannot delete anything.

Things to avoid regressing:
(a) The behaviour the owner marked PASS in H44: newest copy and latest .previous labelled protected, and cancel. Keep the protected labels and reasons unchanged.
(b) If initial focus moves to a Removable row, a stray A selects it. That is still harmless, because removal needs Y then a second Y on the listing confirmation. Never auto-pick anything, and keep "nothing selected at first".
(c) Scoping or filtering by game must only affect focus or what is shown. It must never change the classify() inputs, because newest and newest-verified are computed over the full listing, and filtering before classify could make an older copy look newest.
(d) Changing SharedBoxedRows::State affects every boxed-row user. Add `marked` with default false so other screens do not change.

**Open questions.**

1. Needs hardware confirmation. On the owner's SD card, was the older backup they chose Removable or Protected? If Protected, what reason did its row detail show ("not the size its manifest recorded", "files PokeBank NX did not write", unverified, and so on)? A screenshot of the Recovery files list, or the app log, would settle whether there was anything selectable at all. If a genuinely older verified backup showed Protected, that is a separate classification bug to investigate (RecoveryManagementFs.h:212-221, 246).
2. Did the owner use touch? A tap on a heading row silently focuses it and does nothing.
3. Owner decision: should Recovery files show only the opening game's records by default (with "Show all"), or keep the global list and only start focus in the game's group?
4. Owner decision on the marked-row look (check mark plus accent fill vs left bar), since it becomes the shared multi-select style.
5. Owner decision on the confirmation footer: show both A and B as Keep, or only B?

**Files to change:** `include/UI/RecoveryManagementModel.h`, `include/UI/SharedBoxedRows.h`, `include/UI/BackupBrowserChrome.h`, `src/UI/SaveSelectScreen.cpp`, `include/UI/SaveSelectScreen.h`, `tests/test_recovery_management_model.cpp`, `tests/test_ui_recovery_files_host.cpp`, `tests/test_shared_boxed_rows.cpp`

**Existing tests affected:**
- tests/test_recovery_management_model.cpp:243-252: pins Row.badge strings "Protected"/"Selected"/"Removable". Keep the strings and add assertions on the new picked/removable/heading fields.
- tests/test_ui_recovery_files_host.cpp:122-143: pins badge text, "Remove selected (1)" in the footer, and that focus starts on the newest protected backup (133-136: 'Focus starts on the newest (protected) backup: A only explains'). If initial focus moves to the first Removable row, these lines must change on purpose: A on that row would select it, so press Up first to test the protected explanation.
- tests/test_ui_recovery_files_host.cpp:151-154: confirmation text and B-keeps check. Keep it, and extend it if the confirm footer wording changes.
- tests/test_game_hub_contract.py:847-856: requires exactly 20 Home modal branches, each drawing its own footer. Unaffected unless a Help sub-overlay is added as a new branch, in which case the count changes on purpose.
- tests/test_qa_cross_gen_contract.py:1416-1461: boxed-row API guard. Must still find SharedBoxedRows::drawBox in BackupBrowserChrome.
- tests/test_recovery_management_fs.cpp: safety policy. Must stay unchanged and passing.

**New tests:**
- Model: rows() sets picked/removable/heading for each kind. footerHints gives 'Deselect' on a picked row, 'Why kept' on a protected row, no A on a heading, Y only when picked>0, and always includes Help and B.
- Model: initialFocus prefers the opening game's source group (and its first Removable row) over other groups, and falls back to the first item row when nothing matches.
- Host UI (HostUi fills/rect capture, not just texts): after A on a removable row, that row's cell is drawn with the marked style (distinct fill or bar) and the others are not. The focused row and the marked row are visually different.
- Host UI: a touch tap on a heading row neither moves focus to it nor changes the selection (no overlayRect for headings).
- Host UI: '-' in Recovery files opens Help and B/- returns with the selection kept. The footer lists Help.
- Host UI: opened from game X's Backups with other games' records on the SD tree, initial focus is in game X's group.
- Host UI safety regression: select, Y, B, then B, B (back to Backups): every file still exists byte for byte, including the newest backup, .previous files and Bank. Also Y+B pressed in the same frame on the confirmation keeps everything.
- Host UI: when every item is Protected, A shows the 'only Removable copies can be selected' explanation and Y shows the no-selection notice. No confirmation opens.

**Skeptic corrections.**

I checked this at HEAD 67b1b6a0. `git diff 3634d1e6 HEAD -- src include romfs` is empty, so the application tree is the same as the hardware build. Every cited line in SaveSelectScreen.cpp matches: 3235-3267, 3264-3265, 3464-3496, 3647, 3679, 4166-4206, 4187-4194, 4213, 6102-6151 and 6152-6185. RecoveryManagementModel.h matches too: classify 187-236, toggle 281-287, buildPlan 302-321, Row/rows 330-375 with detail=reason at 369 and badge text at 371. BackupBrowserChrome.h:18-25, SharedBoxedRows.h:43-46 (State is only {focused, enabled}) and 65-74, and HomeQuitConfirm.h decide also match. I confirm the root cause: toggle, plan and delete work, but the UI does not show it. A picked row differs only by the caption word "Selected" in TextSecondary. Focus starts on a Protected row. Touch can focus a heading. The footer and Help have gaps. I found no hidden input or redraw bug: draw runs every frame and the overlay branch is reached.

Corrections and additions:
(a) Classification. "1 Reproduced software bug" is too strong for the main symptom. The host test proves A toggles, the badge changes and the footer shows Y. The defects that can actually be reproduced are the touch focus on a heading (3647/3679 together with overlayRects pushed for every row at 6123) and the misleading "A Why kept" footer on a heading (6140-6145). The rest is 2 plus 6.
(b) The owner's worksheet text was misread. Round 11's H12 is titled "Recovery files cancellation" (commit c0f1c967, Round11 checklist), not "Recovery files UI". H44 PASS ("Inspect Recovery files only; verify latest prior generation is labelled protected. Cancel any deletion.") shows the owner could read the Protected badges and reasons on hardware. So the list does render, and the complaint is about selection feedback and focus, not about a list that never draws.
(c) Weak test. tests/test_ui_recovery_files_host.cpp:133-136 ("A on a protected row selects nothing") passes even if A does nothing. The text it checks, "its newest copy is always kept.", is already drawn by the row's own detail line (Model.h:369). The toast at 4194 only repeats that text, so the test cannot tell the toast happened. It needs to check the toast rect or the HostUi fill, not texts.
(d) The recoveryNotice toast is cleared only by a successful toggle (4190) or by reopening the view. It stays up while the D-pad moves, so after a protected A, the stale "why kept" text stays on screen above other rows. The fix should clear it on focus moves.
(e) The doc comment at SaveSelectScreen.cpp:3233-3234 says "Reading only; nothing is selected." That is stale: the view now selects and deletes. Minor, but update it with the fix.
(f) The tie-break claim does not matter in practice. classify breaks ties with path> (Model.h:170-171) and rows() with path< (Model.h ~343). That ordering is inconsistent, but a ManualBackup sortKey is timestamp|leafName(dir), which is unique per record, so in practice "first item row = protected newest" holds.
(g) Fix detail for SharedBoxedRows. State is used by aggregate initialisation everywhere, e.g. `{selected}` at BackupBrowserChrome.h:20 and 19 files that use State/drawBox. `marked` must be added as the LAST member with a default of false, or positional initialisers break. The marked look must also differ from the pressed tint (PartyNumber at kPressedAlpha=112, SharedBoxedRows.h:41,66-69) and from the focus outline. Otherwise a touch press and a pick look the same.

**Missed generations or paths.**

1. A third drawRow caller was missed. BackupBrowserChrome::drawRow is also used by src/UI/BackupSelectionScreen.cpp:397 (the Switch-save backup browser) and by SaveSelectScreen.cpp:6046 (the EmulatorBackups list). Any change to drawRow or the badge colour changes both. The `marked` parameter must default to false, and those two screens need a check that nothing changed (PASS risk).

2. A second deletion path with a different policy was missed, and it has a safety and consistency side. The Backups screen itself deletes one backup with Y (SaveSelectScreen.cpp:4261-4266 leads to EmulatorBackupDeleteConfirm at 3446). Its check is SB::canDelete (include/UI/SourceBackupClassification.h:356-395). lastCopyGuard returns None whenever the original source file exists (around line 375), so the Backups screen lets the user delete the NEWEST manual backup. Meanwhile the Recovery files explainer (6126-6127) and confirmation (6178) say "Newest copies ... always stay" or "are never removed", and classify protects that copy (Model.h:228-232). The two Gen I-V backup surfaces disagree on what is protected. The owner may be confused when "Removable" in one place differs from what Y allows in the other. This should be flagged (categories 2 and 5, as a wording/policy question for the owner) and not changed silently. A shared verdict should ideally come from one model.

3. Scoping. Emulator manual backups live under emulator-saves/<gameId>/ (RecoveryManagementFs.h:~350), so game scoping could key on gameId. Export manifests carry gameId and SOURCE_PATH, while DurableFile siblings and Bank have neither. Focus or filter should therefore match on source path (emuBackupListings[*].sourcePath, which uses the same exact-string equality as SourceBackupClassification.h:297/326). The fix must never filter before classify.

4. Generations. I agree all of Gen I-V are affected the same way, since this is one shared modal. No generation behaves correctly. Gen V account records and Switch-save .previous files get the same rows.

5. Tests the investigator did not list:
- The EmulatorBackups and BackupSelectionScreen rows must look unchanged after the drawRow and State change.
- The toast should clear when focus moves.
- The 133-136 assertion should be rewritten to check the toast itself.
- An owner decision is needed on whether the Backups-screen Y delete may remove the newest manual backup.

Save safety of the proposed presentation-only fix: confirmed safe. toggle, buildPlan and removePlanned are not touched, and HomeQuit::decide makes A or B win over Y. H44 PASS (protected labels and reasons, cancel) is safe as long as the badge and reason strings stay the same (pinned by tests/test_recovery_management_model.cpp:249-251).

</details>

### H25 — Stale fields after species change; Create draft state (P0)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 3 Newly requested behavior  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Gen IV is the generation reported. It carries both Met Location and Pokerus across a species change, in Create and in Edit. Gen V Create keeps Met Location but drops Pokerus (the rebuild has no Pokerus field), and Gen V Edit keeps both. Gen III mostly behaves correctly: in Create and Edit it clears Met Location/Met Level when they are not a valid encounter for the new species, but it keeps Pokerus. Gen II keeps caughtData and Pokerus; it was not tested on hardware. Gen I has no met or Pokerus fields, so it is not affected.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

The cause is (a), not (b) or (c). The owner changed species inside one Create draft. Pikachu (25) is the Gen IV Create default (src/UI/Gen4SharedPokemonSurface.inc:598-600), so the 'previous pokemon in the editor' is that same draft's default Pikachu. The owner set Met Location to Trophy Garden and a Pokerus state on it, then picked Arbok. The species change rewrites only identity fields of the same record, so Met Location and Pokerus stay.

Not (c), a draft copying an old draft or record:
- beginCreate (Gen4SharedPokemonSurface.inc:584-615) gets a new record from Gen4StagedPokemonEditor::createBoxDraft (src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:287-322), which sets metLocation = 0 (line 319).
- Pokemon4Mutable::createStored starts from a zeroed 0x88-byte buffer (src/Pokemon/Pokemon4Mutable.cpp:110), so Pokerus 0x82 is 0. Line 159 also writes 0x82 = 0.
- Session::beginCreate replaces `working` and clears the baseline (include/UI/Gen4SharedPokemonSession.h:46-56). close() resets it (lines 133-138).

Not (b), a stale UI binding:
- The DETAILS (left) panel is redrawn every frame from A::preview (src/UI/SharedPokemonEditorSurface.inc:1112, 1122, 1150).
- For Gen IV, A::preview is workingRecord(), which reparses session.working->encryptedBytes() each frame (Gen4SharedPokemonSurface.inc:396-403, 1321-1324).
- The Met and Pokerus rows (lines 550-552, 1547-1555) therefore show the bytes the working record really holds. Nothing is cached.

Actual path (a):
- The species picker A calls applyGen4Picker (Gen4SharedPokemonSurface.inc:750-778).
- That calls Pokemon4Mutable::setSpecies (Pokemon4Mutable.cpp:418-500), then resets moves via SpeciesMoveDefaults.
- setSpecies writes only: species 0x08, form bits 0x40, EXP 0x10, PID/gender/ability, held item (Giratina/Arceus case only), and the default nickname.
- It never touches met location, met level, egg location, Pokerus 0x82, ball or fateful.
- Neither applyGen4Picker nor the Create session does any reconciliation afterwards, so Arbok keeps Pikachu's Trophy Garden met location and Pokerus. The same happens in Edit mode.

Comparison with other generations:
- Gen III (src/UI/Gen3SharedPokemonSurface.inc:1344-1366): after session.setSpecies it checks Encounter::forGameSpeciesWithGen3Provenance. If the location/level is no longer valid it zeroes metLocation/metLevel and posts 'Species changed; choose a new legitimate encounter'. Pokerus is kept (include/UI/Gen3SharedPokemonSession.h:114-138 does not touch it).
- Gen V Create: recreateCreateSpecies (include/Integration/Gen5/Gen5SharedScreenState.h:509-588) rebuilds the draft and copies p->metLocation()/metLevel/ball/date into r.encounter (lines 549-552). CreateRequest5 (include/Integration/Gen5/Gen5CreateValidation.h:41) has no Pokerus field, so Pokerus resets and Met Location stays.
- Gen V Edit (Gen5SharedReadOnlySurface.inc:772-802): stages only Field::Species plus moves and shiny, so both fields are kept.
- Gen II (include/UI/Gen2PokemonSession.h:73-96): keeps caughtData and Pokerus.

**Root cause.**

There is no species-change policy for draft-authored history fields; each generation decides on its own.

In Gen IV, applyGen4Picker's Species branch (src/UI/Gen4SharedPokemonSurface.inc:754-777) calls Pokemon4Mutable::setSpecies (src/Pokemon/Pokemon4Mutable.cpp:418-500). setSpecies deliberately keeps every non-identity byte: met location 0x80/0x46, met level, Pokerus 0x82, ball, egg location and fateful. Nothing checks or resets them, in Create or in Edit.

The data needed for a Gen IV check already exists and is not used. EncounterContext45::Catalogue (include/Integration/Encounter/EncounterContext45.h:58-135) has wildRow(location) for species plus ancestry. It is used only to rank picker rows (SharedPokemonEditorSurface.inc:206-216), never to reconcile after a species change.

Gen III is the only generation that does the check, and it does it inline in its own surface (Gen3SharedPokemonSurface.inc:1347-1365), not in a shared component. Gen V does something different: in Create it keeps Met and drops Pokerus. The result is three different behaviours across Gen III, IV and V.

**Shared owner.**

A new shared policy header next to the existing shared species-change policies (include/UI/SpeciesChangeLevelPolicy.h, include/UI/SpeciesChangeMoves.h / SpeciesMoveDefaults), for example include/UI/SpeciesChangeHistoryPolicy.h. It would be a pure function: (mode Create|Edit, oldSpecies, newSpecies, current history {metLocation, metLevel, pokerus, eggLocation, fateful}, isValidEncounter predicate) gives the history to keep or reset, plus a status message.

Each generation supplies its own validity predicate:
- Gen III: Encounter::forGameSpeciesWithGen3Provenance.
- Gen IV/V: EncounterContext45::Catalogue::wildRow plus Names::specialMetLocation.

Each generation calls the policy from its species-apply point:
- Gen IV: applyGen4Picker. Better still, move it into a Gen4SharedEditor::Session::changeSpecies in include/UI/Gen4SharedPokemonSession.h so a host test can reach it.
- Gen V: recreateCreateSpecies and applySpecies.
- Gen III: replace the inline block at Gen3SharedPokemonSurface.inc:1347-1365.
- Gen II: Session::setSpecies.

**Proposed fix.**

Minimal fix, Gen IV first (the generation reported), done in a way the other generations can reuse.

1. Add the shared policy SpeciesChangeHistoryPolicy::reconcile(mode, history, stillValid).
   - Create mode: always reset Pokerus to None. Keep Met Location/Met Level only if the location is a valid encounter for the new species or its ancestry. Otherwise set them to 0 / the Create default met level ('(not recorded)', the createBoxDraft default) and return the Gen III message 'Species changed; choose a new legitimate encounter'.
   - Edit mode: apply the same validity test to Met Location, which matches Gen III's current Edit behaviour. Keep Pokerus, since it is the existing record's own state. Whether Edit should reset at all is an owner decision; see open questions.

2. Gen IV: move the Species branch of applyGen4Picker (Gen4SharedPokemonSurface.inc:754-777) into Gen4SharedEditor::Session::changeSpecies(value, shinyWanted, group, gameId).
   - After setSpecies and the moveset reset, build EncounterContext45::Catalogue(origin, newSpecies, metLocation, {}, history) and use wildRow(metLocation) != nullptr || Names::specialMetLocation(...) as the stillValid predicate.
   - If the location is not valid: call setMetLocation(0) and setMetLevel(level()). In Create, also call setPokerusMode(PokerusMode::None).
   - Keep the existing rollback from the `before` bytes if any setter fails.

3. Gen V: in recreateCreateSpecies (Gen5SharedScreenState.h:549-552), pass the reconciled metLocation (0 when not valid) instead of p->metLocation() unconditionally. Pokerus is already reset in Create.

4. Gen III: replace the inline block with the shared policy call and add the Create Pokerus reset.

5. Gen II: Create-mode Pokerus/caughtData reset through the same policy (optional; owner to confirm).

None of this changes byte serialisation, checksums or the Keep/Add commit paths. Only values in the working draft change.

**Risks.**

Save safety: low. The change only alters values in the in-memory draft before Keep/Add. The commit paths (commitBoxPokemon and stageCreateBoxPokemon with reparse verification) are unchanged.

Behaviour risk:
- Clearing a valid but unusual history in Edit mode, such as gift, trade or event locations that are absent from the kWild45 tables. The predicate must also accept Names::specialMetLocation and egg or fateful histories, or Edit should only warn and not clear. Today Gen III's Edit clear has the same exposure.
- Gen IV PID-preservation behaviour the owner marked PASS (species change preserving nature, shiny, gender and ability, plus R2-BUG-004 moves) must be untouched. Apply the history reconcile after setSpecies and the moves reset, and include it in the same rollback.
- Gen IV met location storage splits DP 0x80 and Pt/HGSS 0x46. Use setMetLocation, not raw writes.

**Open questions.**

1. In Edit mode, should a species change on an existing Pokemon reset Met Location or Pokerus? That is legitimate stored history; PKHeX keeps it. Or should Edit only warn? This needs the owner's decision.
2. In Create, should Pokerus always reset, or only Met Location when it is not valid?
3. The kWild45 coverage of Platinum Trophy Garden (daily Backlot species) was not verified here. Whether Arbok counts as not valid there depends on that table.
4. Only hardware can confirm that the owner's sequence was Create default Pikachu, then Met/Pokerus edits, then Species to Arbok within one draft. The code shows no other way for the values to carry over, because each Create draft starts zeroed and the panel re-reads the working record every frame.

**Files to change:** `include/UI/SpeciesChangeHistoryPolicy.h (new)`, `include/UI/Gen4SharedPokemonSession.h`, `src/UI/Gen4SharedPokemonSurface.inc`, `include/Integration/Gen5/Gen5SharedScreenState.h`, `src/UI/Gen3SharedPokemonSurface.inc`, `include/UI/Gen3SharedPokemonSession.h (optional: Create Pokerus reset)`, `include/UI/Gen2PokemonSession.h (optional)`, `Makefile.host* (register new tests)`

**Existing tests affected:**
- No found test asserts that Met Location or Pokerus survive a species change. Checked tests/test_species_change_moves_gen4.cpp, test_species_change_moves_gen15.cpp, test_gen345_pid_preservation_routes.cpp, test_qa_matrix_gen4.cpp, test_gen3_staged_pokemon_editor.cpp, test_gsc_pokemon_session.cpp and test_gen5_party_box_parity.cpp.
- tests/test_species_change_moves_gen15.cpp:333-336 does a source-text check on the recreateCreateSpecies signature and call. Moving the Gen V rebuild needs that signature kept or the test updated deliberately.
- tests/test_species_change_moves_gen4.cpp: if the Species branch moves from applyGen4Picker into Session::changeSpecies, any source-text contract that greps applyGen4Picker for SpeciesMoveDefaults::setMoves must follow the move. Re-check before editing.
- tests/test_gen5_readonly_ui_contract.py and tests/test_touch_pokemon_editor_direct_rows_contract.py contain PickerTarget::Species source-text requirements. These should keep passing if the dispatch names are unchanged.

**New tests:**
- tests/test_species_change_history_policy.cpp: pure policy table. Create resets Pokerus always and resets Met Location only when it is not valid. Edit keeps Pokerus and resets Met Location only when it is not valid. A valid ancestry location (Pichu to Pikachu) is kept.
- tests/test_gen4_create_species_change_history.cpp:
- Platinum Create draft via Pokemon4Mutable::createStored(species 25). setMetLocation(Trophy Garden Pt id), setPokerusMode(Infected). Session::changeSpecies(24 Arbok). Assert metLocation()==0, pokerus()==0, species 24, and that the bytes reparse valid.
- Pikachu to Raichu (or a species that has a Trophy Garden wild row) keeps the location.
- Edit-mode existing record: Pokerus is kept.
- Gen V host test: recreateCreateSpecies with a stored met location that is not valid for the new species gives metLocation 0.
- Gen III host test: the Create species change also resets Pokerus (mirrors the Gen IV policy).
- Source contract: applyGen4Picker, the Gen III species handler and the Gen V rebuild all call SpeciesChangeHistoryPolicy, so a future generation cannot skip it.

**Skeptic corrections.**

I confirmed the core root cause by reading the code. HEAD of wmain is 67b1b6a0, and `git diff 3634d1e6 HEAD -- src include` is empty, so the code matches the hardware build.

Confirmed:
- src/UI/Gen4SharedPokemonSurface.inc:750-778 (applyGen4Picker, Species branch): calls setSpecies, then setShiny, then SpeciesMoveDefaults::setMoves. It rolls back from `before` only on failure. It does not reconcile met or Pokerus.
- src/Pokemon/Pokemon4Mutable.cpp:418-500 (setSpecies): writes only 0x08, 0x40, EXP 0x10, PID/gender, ability 0x15, item 0x0A (Giratina/Arceus only) and the default nickname. It never writes met location 0x80/0x46, met level, 0x82 Pokerus, ball or fateful.
- Gen4SharedPokemonSurface.inc:598-600: the Create default is Pikachu 25.
- Gen4StagedPokemonEditor.cpp:319: Create starts with metLocation = 0.
- The DETAILS rows (Gen4SharedPokemonSurface.inc:548-554) and the summary rows (1547-1555) read the live record.
- Explanation (a) holds. The owner's own Create Pikachu carried its fields into Arbok.

Corrections and gaps:

1. Open question 3 can be resolved from repo data, no hardware needed. include/Integration/Encounter/EncounterContext45Data.inc has Trophy Garden (location 68) rows for Pikachu 25 and Pichu 172 in diamond_nds, pearl_nds and platinum_nds (Pt Lv 22/24). It has none for Ekans 23 or Arbok 24. So the proposed wildRow predicate would keep Pikachu/Raichu and clear Arbok, as intended.

2. The proposed predicate turns a deliberately non-judging catalogue into a gate that clears data. EncounterContext45::Catalogue declares itself 'unchecked'. scope() says 'Wild data, species + ancestry; unchecked', and detail() has the 'Gift / static / other history; unverified' fallback. SharedPokemonEditorSurface.inc:211 says 'native histories are never rejected here'.
   - kWild45 holds only wild methods: Grass, rods, Surf, Headbutt, HoneyTree, Swarm, Safari, BugContest, HiddenGrotto and RockSmash. It has no Static, Gift, Trade, Egg or Event rows.
   - In Edit mode, wildRow plus specialMetLocation would therefore clear real static or gift met locations, such as legendary or starter-gift locations that are not in the special-location notes.
   - Recommendation: in Edit, never clear. At most show a status warning. Clear only in Create, where met location is draft-authored, and even there keep any location the user explicitly picked when it passes the predicate.
   - Gen III's existing Edit clear (Gen3SharedPokemonSurface.inc:1347-1365) has the same problem. It should not be presented as the model for Edit.

3. They missed existing tests that pin the current code:
   - tests/test_species_change_moves_gen4.cpp:196-205 contains a step-for-step mirror of the applyGen4Picker Species branch (setSpecies, setShiny, setMoves). If the branch moves into Session::changeSpecies or gains a history step, this mirror must be updated or replaced with a call to the real function. Otherwise it silently tests stale logic.
   - tests/test_gen3_hardware_surface_contract.cpp:209 and 213 require the literal 'Encounter::forGameSpeciesWithGen3Provenance' in the Gen III surface source. Replacing the inline block with a shared policy call could break this contract unless the predicate lambda still names that function there.
   - tests/test_gen3_encounter_keep_stored.cpp (R2-BUG-008) pins 'keep stored' picker semantics: a location outside the templates is kept and labelled, not dropped. That is the opposite of auto-clearing in Edit, which supports point 2.

4. Gen V has a second call site. recreateCreateSpecies is called from applySpecies (Gen5SharedReadOnlySurface.inc:762) and from the PID-confirm path for a Create species change that also flips shiny (Gen5SharedReadOnlySurface.inc:885-890, pending.rebuildCreate set at 744-758). The fix must go inside recreateCreateSpecies at Gen5SharedScreenState.h:549-552, not in applySpecies, or the shiny-change route is missed. Their proposal does point at recreateCreateSpecies, which is correct.

5. Their Gen V Create Pokerus claim is confirmed. A grep for Pokerus in include/Integration/Gen5/Gen5CreateValidation.h finds nothing. So Gen V Create already drops Pokerus while Gen IV keeps it, which is the cross-generation inconsistency.

6. A minor point on metLevel. createBoxDraft uses metLevel 5 (Gen4StagedPokemonEditor.cpp). The proposal resets metLevel to level(). In Create, level is preserved across species changes by setSpecies, so this differs from the createBoxDraft default only if the user changed level. That is acceptable, but the test should assert it explicitly.

The rest checks out. The classification of 1 and 2 is correct. 3 applies only to the Edit-mode policy decision. Save-safety risk is low: only draft values change, and the commit paths are untouched.

**Missed generations or paths.**

- Gen V: a second route into recreateCreateSpecies through the PID-confirm modal for a Create species change that also flips shiny (Gen5SharedReadOnlySurface.inc:885-890). Putting the fix inside recreateCreateSpecies (Gen5SharedScreenState.h:549-552) covers both routes.
- Gen II level policy: Gen II setSpecies also resets level through SpeciesChangeLevelPolicy::defaultLevel (include/UI/Gen2PokemonSession.h:76-77). Gen IV setSpecies preserves level. This is another per-generation difference in species-change policy. It is not in scope for H25, but a shared policy header should not mix it in by accident.
- Legacy path: TrainerViewScreenBase.inc:3118 (the legacy pkPick->setSpecies Dialogs picker) is another species-change path with no history reconcile. Whether it is reachable for Gen I-V in v1.0 was not verified.
- Gen III touch: species taps route through handleSpeciesPicker (Gen3SharedPokemonSurface.inc:1292 onward), the same handler as the A-button path, so no separate Gen III touch path skips the check.
- Gen I: confirmed unaffected, since there are no met or Pokerus fields.
- Tests the investigation missed:
  - tests/test_species_change_moves_gen4.cpp:196-205 (applyGen4Picker mirror)
  - tests/test_gen3_hardware_surface_contract.cpp:209,213 (source-text contract)
  - tests/test_gen3_encounter_keep_stored.cpp (keep-stored semantics that conflict with clearing in Edit)
  - tests/test_met_location_fit.cpp:123 (uses the Gen III provenance API)
- Encounter data: kWild45 has no static, gift, egg or trade rows, so the Gen IV/V validity predicate cannot safely clear history in Edit mode.

</details>

### H31 — Clone workflow across Gen I-V (P0)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Gen I works as the owner wants (PASS, and it must not regress): Clone opens a destination modal, A stages the clone, and the app goes back to the box with no Actions menu. The owner asked for a picker with a carried preview, which Gen I does not have yet, so Gen I needs that added too. Gen II has the bug: after a successful clone it reopens the Actions menu. Gen III and Gen IV have the bug: Clone opens two number keypads, entering a number usually fails, and the Actions menu stays open. Gen V opens keypads too, but its default is the first empty slot and the screen goes back to the box after staging, so only the keypad is wrong there.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Each generation has its own Clone UI. None of them uses the shared carry layer.

GEN I (works). The Actions menu calls Clone at src/UI/ClassicReleaseActionFix.inc:49, which runs ux2OpenClone at src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:435-443. That sets UX2Mode::CloneDestination, with the cursor on the first empty slot. Input is at Cleanup2.inc:721-736: L/R changes box, D-pad moves the slot, B goes back to Actions, A runs ux2StageClone. ux2StageClone (Cleanup2.inc:445-475) refuses an occupied slot (455) and any slot other than the first writable one (463-466, because Gen I boxes are packed lists). It calls StagedPokemonEditor::stageClone (src/Integration/Gen1/Gen1StagedPokemonEditor.cpp:474), then refreshBoxes, then ux2Close(state) at 474. ux2Close is what returns to the box with no menu. The modal is drawn by ux3DrawClone at src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:628, with footer 'D-pad/Stick Slot, L/R Box, A Stage Clone, B Cancel' (Cleanup3.inc:721/757). There is no carried sprite.

GEN II. Actions -> Clone at ClassicReleaseActionFix.inc:384-393 sets shared.cloneActive=true and cloneBox, and hides the action sheet. The dialog is handleSharedClone at src/UI/Gen2SharedPokemonSurface.inc:207-251. It only changes box (L/R) and always targets firstEmptyGen2Slot (231). A calls StagedEditor::stageCloneBoxPokemon (src/Integration/Gen2/Gen2StagedEditor.cpp:1257-1325), which appends to the packed list and returns the slot through an out-parameter. Then at line 246 the code calls backToActions(). That lambda (lines 217-224) sets actions.active=true and actions.pokemonActions=true, which reopens Pokemon Actions (Clone/Edit/View/Release/Origin & Legality). The comment at 215-216 says this was intended, and tests/test_gen2_clone_flow.cpp:67-69 asserts it.

GEN III. Actions -> Clone at src/UI/Gen3SharedPokemonSurface.inc:1744-1758 opens two blocking keypads: Utils::promptNumber("Clone to Gen III Box", state.box+1, 1, 14) at 1745, then ("Clone to Slot", state.slot+1, 1, 30) at 1747. It then calls StagedPokemonEditor::stageCloneBoxPokemon (src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:950-1001). The Clone case never sets state.actions=false. Remove, Legality and Review do clear it (1731-1742), so after a clone the Actions sheet stays open.

GEN IV. The same keypad pattern is at src/UI/Gen4SharedPokemonSurface.inc:1067-1081 ("Clone to Gen IV Box" with default state.box+1, range 1-18; slot default state.slot+1, range 1-30), calling Gen4StagedPokemonEditor::stageCloneBoxPokemon (src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:484-545). state.actions is never cleared. Party Clone is refused at 1040-1041.

GEN V. Actions -> Clone runs state.activateSelected. Gen5SharedScreenState::activate (include/Integration/Gen5/Gen5SharedScreenState.h:114-126) sets Surface::CloneDestination with cloneDestinationIndex_=firstEmptyCloneDestination(). Then src/UI/Gen5SharedReadOnlySurface.inc:1697-1698 calls stageCloneOrMoveByKeypad (1466-1491), which opens box and slot keypads (1472-1475) defaulting to that first empty slot, then calls confirmClone (Gen5SharedScreenState.h:182-207). confirmClone sets surface_=Browse, so there is no menu afterwards. In the same file, Move already uses the shared visual carry (Gen5SharedReadOnlySurface.inc:1689-1695, through ClassicPackedMove::beginGen5FromAction).

KEYPAD RESULT HANDLING. Utils::promptNumber (src/Utils/Keyboard.cpp:115-160) uses the in-app KeyboardDialog. That dialog preselects the initial digits (src/UI/Dialogs/KeyboardDialog.cpp:279), so typing replaces them. The value is clamped to the range and returned with accepted=true. The keypad parsing itself works.

EXISTING CARRY MACHINERY. src/UI/ClassicPackedMoveOverlay.inc, namespace UI::ClassicPackedMove, handles Gen I-V Move. Its CarryState (31-71) holds the sourceBox/slot and the visuals (species, form, shiny). Entry points are beginGen1/2/3/4/5 (274, 312, 390, 429, 476) and beginGenNFromAction (1004/1015/1027). The carried sprite floats over the hovered cell with a bob and shadow (draw, 1218-1270). place() is at 842-892, with placeGen4 at 676 and placeGen5 at 804. Y drops (1047, 1124), B cancels, A/X are refused ('Place with Y or cancel with B first', 1053/1130), and the D-pad and L/R fall through to normal box navigation (1140). Gen IV and Gen V carries are deferred: nothing is staged until the drop, and the pickup bytes are re-verified. Gen I, II and III carries take the source out of the staged image immediately (beginPackedMove / beginSparseMove).

PKSE (upstream, e.g. git show e861fb60:src/UI/TrainerViewScreen.cpp). The storage menu's Clone (3157-3197) places the copy automatically in the first free slot, searching this box first and then any box. PKSE's 'hover' duplicate is grabSelection(false) (1385-1416, triggered by X during a rectangle select at 1765-1770). It pushes src->clone() into the carried moveMon while leaving the original in place. The user then moves the carried copy around the grid and drops it with the normal place action. The owner's 'hover over box' request is this duplicate-and-carry workflow.

**Root cause.**

1) Gen II reopens Actions. In src/UI/Gen2SharedPokemonSurface.inc:246, handleSharedClone calls backToActions() after a successful stageCloneBoxPokemon. The lambda at 217-224 sets editorOverlayState.active=true and pokemonActions=true, so the Actions sheet is drawn again. Gen I does the opposite: it calls ux2Close at Gen1PokemonEditorOverlayUXCleanup2.inc:474. tests/test_gen2_clone_flow.cpp:68-69 deliberately asserts the wrong behaviour.

2) Gen III/IV/V open a keypad. The Clone code itself calls Utils::promptNumber twice:
- Gen3SharedPokemonSurface.inc:1745/1747
- Gen4SharedPokemonSurface.inc:1069/1071
- Gen5SharedReadOnlySurface.inc:1472/1475 (stageCloneOrMoveByKeypad, called at 1697-1698)
The Gen V comment at 1463-1465 says Gen I-IV 'share' these keypads. That is false for Gen I and II. No generation from III to V uses the shared ClassicPackedMove carry for Clone.

3) Why entering a number on Gen III fails. This comes from the code; hardware should confirm the exact digits the owner typed.
(a) The slot keypad's default is state.slot+1 (Gen3SharedPokemonSurface.inc:1747), the source slot. The source slot always holds the Pokemon being cloned. So accepting the default, or keeping the current box and typing any occupied slot, always fails with 'Generation III clone destination must be empty' (Gen3StagedPokemonEditor.cpp:968-970). Gen IV has the same default at Gen4SharedPokemonSurface.inc:1071 and fails with 'Generation IV Clone destination is occupied' (Gen4StagedPokemonEditor.cpp:502-505). Gen V does not have this problem because its default is the first empty slot (Gen5SharedReadOnlySurface.inc:1471-1475).
(b) Even when the clone succeeds, the Clone case never sets state.actions=false (Gen3:1744-1758, Gen4:1067-1081). The dimmed Actions sheet stays on top of the grid, which hides the new slot and makes the clone look like it did nothing.
(c) A Gen III clone is also refused with 'Place or cancel the carried Pokemon first' while a sparse Move carry is active (Gen3StagedPokemonEditor.cpp:955).
The keypad parsing itself (Keyboard.cpp:115-160) is correct.

4) Structure. Clone destination selection is built five separate times (Gen I modal, Gen II modal, Gen III/IV keypads, Gen V keypad over a CloneDestination surface). The shared carry layer that already gives the preview and the placement (ClassicPackedMoveOverlay.inc) supports only Move, places with Y, and refuses A.

**Shared owner.**

src/UI/ClassicPackedMoveOverlay.inc (namespace UI::ClassicPackedMove: CarryState, begin*/place*, draw). This is the Gen I-V carry with the floating sprite, and Gen IV and Gen V Move already use it. Put the pure decision rules (whether a destination is allowed, and for Gen I/II snapping to the first writable slot) in a host-testable header under include/UI/, for example next to ClassicCarryGrid.h / ClassicPackedMovePresentation.h. The per-generation staging stays in the existing backends: Gen1 stageClone, Gen2 stageCloneBoxPokemon, Gen3 stageCloneBoxPokemon, Gen4 stageCloneBoxPokemon, and Gen5 cloneToEmptyBox / cloneStagedToEmptyBox through a new Gen5SharedScreenState drop method.

**Proposed fix.**

Build one shared Clone carry in ClassicPackedMove:

1. Add `enum class CarryIntent { Move, Clone }` to CarryState, and add `bool beginCloneFromAction(TrainerViewScreen&, int box, int slot)` for Gen 1-5.

2. A Clone carry is always deferred, like the Gen IV and Gen V Move carries. On pickup it records the source box/slot, the source record bytes (Gen1/2 packed body+OT+nickname, Gen3 encrypted PK3, Gen4 PK4, Gen5 PK5) and the visual. It never calls beginPackedMove/beginSparseMove and never changes the staged image. The source stays visible in its slot.

3. Each generation's Actions -> Clone (ClassicReleaseActionFix.inc:49 for Gen1 and :384 for Gen2; Gen3SharedPokemonSurface.inc:1744; Gen4SharedPokemonSurface.inc:1067; Gen5SharedReadOnlySurface.inc:1697) does three things:
- closes its action sheet (ux2Close / legacy.pokemonActions=false and active=false / state.actions=false / state.cancelBoxAction());
- puts the box cursor on the first empty slot of the current box;
- calls beginCloneFromAction.
Delete the promptNumber clone calls, stageCloneOrMoveByKeypad's clone branch, the Gen II cloneActive dialog, and Gen I's CloneDestination modal.

4. While a Clone carry is active:
- D-pad/stick, L/R and touch move the cursor and change box (the existing fall-through at ClassicPackedMoveOverlay.inc:1140).
- A confirms. Y may also confirm, since Y is the Move drop key. X is refused.
- B cancels and stages nothing.
- The ghost sprite is drawn over the hovered cell, translucent or with a 'Copy' badge so it reads as a copy.
- The footer reads 'A Place Clone / L/R Box / B Cancel'.

5. On confirm, run a pure decision `CloneDrop::decide(packed, occupied, targetSlot, firstWritable)`:
- Occupied target: keep the carry and show 'Choose an empty slot'. Never overwrite.
- Gen I/II packed boxes: only the first writable slot is valid. Either snap the ghost to it while hovering, or refuse other slots with the existing message. That keeps the current Gen I rule.
- Gen III-V: any verified empty slot is valid.
Then re-verify the source bytes against the pickup, as placeGen4/placeGen5 do. If they differ, cancel safely. Otherwise call the generation's existing clone backend, refresh the presentation, and roll back if the refresh fails (Gen5 needs a new Gen5SharedScreenState::dropCarriedClone mirroring dropCarriedPokemon). Clear the carry, leave the cursor on the new slot, and post 'Clone staged — + Review; source save unchanged'. Do not reopen any menu.

6. Gen V party sources: the carry should allow source region Party, since Gen V currently clones from party. Gen IV party Clone stays disabled.

If a smaller first step is wanted, these three changes fix the reported bugs on their own:
(i) remove backToActions() after the stage at Gen2SharedPokemonSurface.inc:246 and close the actions instead;
(ii) set state.actions=false in the Gen III and Gen IV Clone cases;
(iii) default the Gen III/IV slot keypad to the first empty slot.
They do not give the requested visual picker, so the recommended fix is the shared carry.

**Risks.**

Save safety:
- The Clone carry must be deferred, meaning nothing changes until the drop. Do not reuse beginPackedMove/beginSparseMove for Clone: on Gen I-III those remove the source from the staged image, and if the drop path broke, a clone would turn into a move or lose data.
- Every drop must re-verify the source bytes and check that the destination is empty inside the backend. The backends already refuse occupied destinations (Gen3:968, Gen4:502-505, Gen5 emptyCloneDestination, Gen2 appends to the packed list only).
- Gen II stageCloneBoxPokemon ignores the requested slot and always appends. The UI must therefore show the first writable slot as the destination, otherwise the ghost would show the wrong slot.
- The routing order matters. ClassicPackedMove::handleInput runs at TrainerViewScreenCompositeOverlay.cpp:440, before the Gen1/Gen2 action handlers at 443-444. Gen V routes the carry from inside its own surface (Gen5SharedReadOnlySurface.inc:1594). The Clone carry needs the same routing, or the A key will be swallowed by the Gen III/IV surfaces.

Behaviour the owner marked PASS:
- Gen I Clone already works. The rules must not change: only the first writable contiguous slot, the source untouched, no menu afterwards.
- The Move Y-drop behaviour that the existing help and contract tests pin must stay the same.
- Gen V Clone from a party slot works today and must keep working.

**Open questions.**

1. Gen I/II packed boxes: on A over a non-first empty slot, should the clone snap to the first writable slot, or should the app refuse and keep the carry? Gen I refuses today.
2. Should Y also place a Clone carry, so the drop key is the same as Move, or only A, as the owner asked?
3. Should the Clone carry be able to cross into the Bank or Storage panes? Recommendation: no, same save only.
4. Should Gen IV party Clone be enabled through the carry? It is currently disabled at Gen4SharedPokemonSurface.inc:1040-1041.
5. Hardware needs to confirm what exactly 'entered a number on Gen III didn't work' was. The code points to two causes: the source-slot default (occupied, so the clone is rejected) and the Actions sheet staying open after a clone that did succeed. Check whether a Gen III clone into a typed empty slot actually showed up in Review.
6. Should the ghost carry look different from a Move carry (for example translucent with a 'Copy' badge)?

**Files to change:** `src/UI/ClassicPackedMoveOverlay.inc`, `include/UI/ClassicCarryGrid.h (or new include/UI/CloneDropDecision.h)`, `include/UI/ClassicPackedMovePresentation.h (footer/labels)`, `src/UI/ClassicReleaseActionFix.inc`, `src/UI/Gen2SharedPokemonSurface.inc`, `src/UI/Gen2SharedSurfaceParity.inc`, `src/UI/Gen2HardwareWorkspaceFix.inc`, `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc`, `src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc`, `src/UI/Gen3SharedPokemonSurface.inc`, `src/UI/Gen4SharedPokemonSurface.inc`, `src/UI/Gen5SharedReadOnlySurface.inc`, `include/Integration/Gen5/Gen5SharedScreenState.h`, `src/UI/TrainerViewScreenCompositeOverlay.cpp (routing, if the clone carry must pre-empt surface handlers)`

**Existing tests affected:**
- tests/test_gen2_clone_flow.cpp:43-73: it pins the Gen II cloneActive dialog and asserts backToActions() after staging (68-69). This must be rewritten on purpose, because it encodes the bug the owner reported.
- tests/test_gen5_readonly_ui_contract.py:1050-1051: it requires stageCloneOrMoveByKeypad and the strings "Clone to Gen V Box" / "Move to Gen V Box". Update it to require the shared clone carry.
- tests/test_gen1_pokemon_editor_ui_contract.cpp:140: it pins FooterSurface::CloneDestination. Update it if the Gen I modal is replaced.
- tests/test_classic_help_accuracy.cpp:110, tests/test_gen3_hardware_surface_contract.cpp:316, tests/test_gen4_hardware_surface_contract.cpp:226: they pin 'if (down & HidNpadButton_Y) return place(screen, state)'. Keep the Move Y drop exactly as it is so these still pass, and add the A path only for intent==Clone.
- tests/test_gen4_hardware_surface_contract.cpp:494: it pins the Clone menu row (unaffected).
- tests/test_gen5_party_box_parity.cpp:120, tests/test_gen5_staged_slot_actions.cpp:169, tests/test_gen5_readonly_save.cpp:1464, tests/test_gen5_review_discard_export_trace.cpp:353: they use Surface::CloneDestination at the state-model level. They can stay if confirmClone is kept for Move or as a backend; recheck them if CloneDestination is removed.
- tests/test_gen3_staged_pokemon_editor.cpp, tests/test_gen4_staged_pokemon_editor.cpp, tests/test_gsc_staged_editor.cpp, tests/test_rby_pokemon_editor.cpp: backend clone semantics should not change.

**New tests:**
- Host unit test for the pure CloneDrop::decide model, covering all of these cases: packed Gen I/II with an occupied target (refuse); packed target not equal to the first writable slot (refuse or snap); packed full box (refuse); sparse Gen III-V empty target (accept); sparse occupied target (refuse); target equal to the source (refuse, no-op); cancel stages nothing.
- Contract test that every generation's Actions -> Clone path calls ClassicPackedMove::beginCloneFromAction and contains no Utils::promptNumber call with 'Clone' in it (Gen3SharedPokemonSurface.inc, Gen4SharedPokemonSurface.inc, Gen5SharedReadOnlySurface.inc).
- Contract test that after a successful clone drop, no Gen I-V path reopens Actions: no pokemonActions=true / state.actions=true / Surface::Actions after the stage call.
- Contract test that in a Clone carry, A and Y place, B cancels, and A in a Move carry is still refused (the Move carry stays unchanged).
- Backend round-trip test per generation (Gen1 stageClone, Gen2 stageCloneBoxPokemon, Gen3/Gen4 stageCloneBoxPokemon, Gen5 cloneToEmptyBox/cloneStagedToEmptyBox through a new dropCarriedClone): the source bytes are unchanged, the destination is byte-identical, exactly one Review row exists, and a source that changed between pickup and drop is refused with nothing staged.
- A test that a Gen V clone carried from a party source lands in a box slot.
- A test that an occupied destination keeps the carry and leaves the staged image byte-identical.

**Skeptic corrections.**

I checked every cited line at HEAD 67b1b6a0 in wmain. The core root cause is correct. A few details are wrong, overstated or missing.

CONFIRMED:
- Gen II reopens Actions. The backToActions lambda at src/UI/Gen2SharedPokemonSurface.inc:217-224 sets actions.active=true and pokemonActions=true. It is called after a successful stage at :246. The intent is in the comment at :215-216. tests/test_gen2_clone_flow.cpp:68-69 asserts the bug ("After staging the dialog also returns to Actions").
- Gen III opens keypads. Utils::promptNumber is called at src/UI/Gen3SharedPokemonSurface.inc:1745 and :1747, with slot default state.slot+1. The Clone case (1744-1758) never sets state.actions=false. Remove, Legality and Review do clear it (1731-1742). The A that runs the action comes from the Actions handler at 1712-1716.
- Gen IV has the same pattern at src/UI/Gen4SharedPokemonSurface.inc:1067-1081 (calls at 1069/1071), and state.actions is never cleared there. Party Clone is refused at 1040-1041.
- Gen V. stageCloneOrMoveByKeypad is at src/UI/Gen5SharedReadOnlySurface.inc:1466-1491 and is reached from :1697-1698. Gen5SharedScreenState.h:115-126 (activate) and :182-207 (confirmClone, which sets surface_=Browse) match the investigation.
- Backend refusals. Gen III refuses an occupied destination at Gen3StagedPokemonEditor.cpp:968-970 and refuses while a carry is active at :955. Gen IV refuses at Gen4StagedPokemonEditor.cpp:502-505. So the source-slot default always fails for Gen III and IV. The keypad itself works: Keyboard.cpp:115-160 parses correctly, KeyboardDialog.cpp:279 preselects the digits, and UIManager::runKeyboard (src/UI/UI.cpp:253-284) waits for the confirming button to be released, so no stale A press leaks back into the Actions handler. That supports the "Actions sheet stays open" plus "occupied default" explanation.
- Gen I. Cleanup2.inc:435-475 (ux2OpenClone, ux2StageClone, ux2Close at 474) and the input at 721-736 match the investigation.

WRONG OR OVERSTATED:
1. Routing risk. The investigation says "the A key will be swallowed by the Gen III/IV surfaces". In fact ClassicPackedMove::handleInput runs at TrainerViewScreenCompositeOverlay.cpp:439-440, before Gen4SharedEditorSurface (441) and Gen3SharedEditorSurface (442). It is gated by classicPackedMoveLayerAvailable (250-262), which requires !ownsFrame for Gen III/IV and ux2 mode==Closed for Gen I. So the real requirements are:
   - the Clone entry must close state.actions, ux2Close and so on before the carry begins, otherwise the layer stays unavailable;
   - the carry's own A refusal must be bypassed for a Clone intent. That refusal is at ClassicPackedMoveOverlay.inc:1127-1130 for Gen I-IV and in handleGen5CarryInput at :1052-1055 (Gen V, called from Gen5SharedReadOnlySurface.inc:1594).
   No surface swallows A.
2. "Gen II stageCloneBoxPokemon ignores the requested slot": it does not take a requested slot at all. The signature at Gen2StagedEditor.cpp:1257-1259 has destinationSlot as a std::size_t& out-parameter only. The UI must derive the drop slot from firstEmptyGen2Slot.
3. The Gen V comment at Gen5SharedReadOnlySurface.inc:1463-1465 ("Gen I-IV share them") is not wholly false for Gen II, because a legacy Gen II keypad clone still exists (see missed paths below).
4. Gen V's keypad default is the first empty slot in ANY box (workspace.firstEmptyCloneDestination, 0-719), not the first empty slot of the current box. A hover picker should start in the current box.
5. The test plan says "exactly one Review row". The Gen IV backend (Gen4StagedPokemonEditor.cpp:484-545) pushes no change record, unlike Gen III (:996-999). Its Review is diff-derived, so that assertion has to be written per generation.
6. The test plan is almost entirely string-contract tests. A real host-UI harness exists in tests/host-ui/ (HostUi.h:73 queues promptNumber answers; HostUiStubs.cpp:93), with input-driven tests such as tests/test_ui_classic_picker_dismiss_host.cpp. The new A/B/no-menu-after-clone behaviour should be pinned there end-to-end, not only by grep contracts.

Everything else (Gen I stays PASS, keep the Move Y-drop unchanged, deferred non-mutating Clone carry) is sound. The warning not to reuse beginPackedMove/beginSparseMove for Clone is correct and important for save safety.

**Missed generations or paths.**

Clone entry points and legacy keypad or modal paths the investigation did not list:
- src/UI/Gen2SharedPokemonSurface.inc:161-168. This is a second Gen II Clone entry, the shared-surface case SharedEditor::Action::Clone. It sets cloneActive and also feeds handleSharedClone. tests/test_gen2_clone_flow.cpp:72-73 pins it ("Both Clone entry points"). The fix must convert both Gen II entries.
- src/UI/TrainerViewScreenGSCOverlay.inc:445-458. In handlePokemonActionInput (reached from handleStagedEditorInput via :624 and :929) there is a legacy Gen II keypad clone: Utils::promptNumber("Clone to Gen II Box") followed by stageCloneBoxPokemon. It is normally shadowed because ClassicReleaseActionFix.inc:309-317 runs first at Composite:444 when pokemonActions is active. That handler bails out whenever stateFor(screen).mode != Mode::None, however, and then the legacy path can run. Delete it or route it to the carry, or a keypad clone can resurface on Gen II.
- src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:681. This is a second Gen I ux2OpenClone entry, besides ClassicReleaseActionFix.inc:49.
- src/UI/Gen1PokemonEditorOverlayUX.inc:791/829/1099/1127/1213. This is a legacy Gen I Mode::CloneConfirm, a box-only Left/Right dialog. Reachability was not verified. Audit it so no old dialog survives.
- Tests not listed:
  - tests/test_gen2_clone_flow.cpp:44-73 in full, including the input and draw guards at :50/:55 on shared.cloneActive.
  - tests/test_gen5_readonly_ui_contract.py:594/:762, which pin Utils::promptNumber( in Gen V regions. Check whether these cover the clone keypad before deleting it.
- Gen V Move also uses stageCloneOrMoveByKeypad's move branch, but Actions->Move now goes through beginGen5FromAction (1684-1689). Removing only the clone branch leaves dead Move keypad code. Confirm before deleting the whole function and its test pin at py:1050-1051.

</details>

### H32 — Occupied-destination safety for Clone/Move/Import (P0, NOT TESTED on hardware)

- **Classification:** 5 Safety concern, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** No generation lets Clone, Move or Import silently overwrite an occupied save slot through the shipped paths. All five refuse, insert or swap without loss. One real defect remains, and it is latent: in Gen I and Gen II the backend still accepts other staged changes while a Pokemon is carried, and Cancel then silently discards those changes. Gen III already refuses them (carryingSparseMove). Gen IV and Gen V stage nothing until the drop and check the carried record again first, so they are unaffected.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

How each operation behaves today, by generation (file:line).

CLONE
- Gen I: stageClone (src/Integration/Gen1/Gen1StagedPokemonEditor.cpp:474-484) calls destination() (:242-248). It refuses unless slot == count, with the messages 'Destination slot is occupied' or 'Select the first empty slot; Gen I boxes are contiguous'. A full box is refused too. The UI also forces the next writable slot (src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:455-470).
- Gen II: stageCloneBoxPokemon (src/Integration/Gen2/Gen2StagedEditor.cpp:1257-1280) ignores the requested slot and always appends at count. It refuses when count >= capacity ('clone destination box is full') and refuses eggs ('egg cloning is not enabled'). The UI uses firstEmptyGen2Slot (src/UI/Gen2SharedPokemonSurface.inc:229-243).
- Gen III: stageCloneBoxPokemon (src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:950-1001) refuses an occupied destination, a slot holding an unreadable species index (holdsUnreadableRecord, :332-342), and any clone while a Pokemon is carried (:955). A failed clone rolls back.
- Gen IV: stageCloneBoxPokemon (src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:484-548) refuses 'destination is occupied' and rolls back if the exact reparse check fails. Clone works on box Pokemon only (src/UI/Gen4SharedPokemonSurface.inc:1067-1082).
- Gen V: cloneToEmptyBox and cloneStagedToEmptyBox (include/Integration/Gen5/Gen5StagedPokemonWorkspace.h:104-142 and :294-327) require emptyCloneDestination (:383-393). That means a source slot that is empty and unchanged, holds a native blank PK5, and has no pending edit or staged replacement. The picker starts at firstEmptyCloneDestination (include/Integration/Gen5/Gen5SharedScreenState.h:115-127), and confirmClone checks again (:182-208).

MOVE (Y carry, owned by src/UI/ClassicPackedMoveOverlay.inc)
- Gen I: placePackedGroupMove (src/Integration/Gen1/Gen1PackedMove.cpp:80-108) inserts at min(dest, count) and shifts later Pokemon down. It never overwrites, and refuses when count + group > capacity. The carried records come from the pickup snapshot. Cancel restores the whole pickup snapshot (:110-115).
- Gen II: same insert behaviour (src/Integration/Gen2/Gen2PackedMove.cpp:66-77), plus a byte-for-byte round-trip check of every placed Pokemon. Cancel restores staged_, changes_ and expectations from the pickup snapshot (:79).
- Gen III: placeSparseMove (src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:1323-1369) refuses the whole group if any target is occupied or unreadable, or if the shape does not fit. A refused drop keeps the carry.
- Gen IV: the UI stages nothing until the drop. placeGen4 (src/UI/ClassicPackedMoveOverlay.inc:676-800) re-reads the pickup records and refuses if they changed. A multi-select drop requires empty, non-overlapping targets (:752-767). A single drop calls stageMoveBoxPokemon (src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:716-782): an occupied target is SWAPPED verbatim and the read-back of both records is checked. This is lossless, and the status message says 'moved/swapped' (:794).
- Gen V: placeGen5 (src/UI/ClassicPackedMoveOverlay.inc:804-840) and dropCarriedPokemon (include/Integration/Gen5/Gen5SharedScreenState.h:250-280) refuse a changed carried record and any target that is not an empty verified slot. An occupied target keeps the carry. Clone and Remove are one transaction with a snapshot rollback (Gen5StagedPokemonWorkspace.h:256-268).
- Party: every generation carries box Pokemon only (gridOwnsInput / beginGen1FromAction require the Boxes or Storage grid). Gen V Move refuses a party source (Gen5SharedScreenState.h:116-119).

IMPORT (src/UI/PokemonFileMenus.inc)
- Gen I and Gen II pick the first empty slot (:23-62). Gen I stageImport (Gen1StagedPokemonEditor.cpp:509-560) refuses while a Pokemon is carried, refuses a full box (:530) and calls destination(). Gen II (Gen2StagedEditor.cpp:1486-1530) refuses while carrying, refuses a full box, and refuses 'Destination slot is occupied'.
- Gen III (:1106-1135) refuses an occupied or unreadable slot and refuses while carrying.
- Gen IV (Gen4StagedPokemonEditor.cpp:626-700) refuses 'refuses to overwrite an occupied box slot'.
- Gen V importToEmptyBox (Gen5StagedPokemonWorkspace.h:190-226) refuses an occupied slot, a slot with a staged change, and a slot that is not a native blank.

LATENT GAP (reproduced in the backend with a scratch probe; see Root cause).

**Root cause.**

No path silently overwrites a Pokemon. The one defect is in the Gen I/II carry transaction, which assumes nothing else gets staged while a Pokemon is carried, but the backend does not enforce that.

- Gen I: beginPackedGroupMove saves the whole staged image (packedMove_.beforeBytes, src/Integration/Gen1/Gen1PackedMove.cpp:55) and cancelPackedMove commits that image back (:110-115). Only stagePartyPokemonEdit (:347), stageImport (:511), the exports and finalizedBytes refuse while packedMove_.active. stageEdit (box, :343), stageAdd (:423), stageClone (:474) and stageRemove (:486) have no guard.
- Gen II: cancelPackedMove sets staged_ = stagedBefore and changes_ = changesBefore (src/Integration/Gen2/Gen2PackedMove.cpp:79). Only the party edit (Gen2StagedEditor.cpp:962), Import (:1490), the exports and Release (Gen2PackedMove.cpp:80) refuse. stageCloneBoxPokemon (:1257), stageAddBoxPokemon (:1327), stageBoxPokemonEdit/Shiny, the trainer edits (:730-846) and the inventory edits (:848+) do not.

The probe (scratchpad/probe-h32/probe.cpp, built from the test_gen12_packed_move fixture) confirmed Gen I: during a carry, stageAdd, stageClone, stageEdit and stageRemove all returned true. After cancelPackedMove the added Pokemon, the clone and the nickname edit had all disappeared with no error. The same pattern is in the Gen II code (not probed).

Through the UI this appears unreachable today, because ClassicPackedMove::handleInput refuses A and X whenever state.active, in any view mode (src/UI/ClassicPackedMoveOverlay.inc:1121-1135). That is a UI-only guard. Gen III gets this right in the backend (carryingSparseMove at Gen3StagedPokemonEditor.cpp:402, 751, 955, 1006, 1110, 1386).

Two smaller inconsistencies, neither a safety issue:
- Gen II refuses to clone an egg (Gen2StagedEditor.cpp:1263) but imports egg .pk2 files, and Gen III/IV/V clone eggs byte for byte.
- Gen V treats a slot freed by a staged Release as not empty (emptyCloneDestination, Gen5StagedPokemonWorkspace.h:385). Gen III and IV reparse the staged image and accept such a slot.

**Shared owner.**

For destination policy: each generation's staged editor is the right owner, and all of them already enforce it. For the carry transaction: the Gen I/II packed-move backends should own the guard the way Gen III's carryingSparseMove does: src/Integration/Gen1/Gen1StagedPokemonEditor.cpp, src/Integration/Gen1/Gen1PackedMove.cpp, src/Integration/Gen2/Gen2StagedEditor.cpp and src/Integration/Gen2/Gen2PackedMove.cpp. The UI rule stays in src/UI/ClassicPackedMoveOverlay.inc, the one shared Y-carry layer for Gen I-V. Coverage should live in one new cross-generation host test matrix, so every generation is held to the same rules.

**Proposed fix.**

1. Gen I: add the guard `if (packedMove_.active) { error = "Place or cancel carried Pokemon first"; return false; }` to the public box-path entry points: stageEdit, stageAdd, stageClone, stageRemove and revertPokemon. Do not add it inside stageRecordEdit, because the party path is already guarded. Caution: beginPackedGroupMove calls stageRemove after setting packedMove_ (Gen1PackedMove.cpp:55-64). Either move the removal loop into a private unguarded helper, or set packedMove_ only after the removals succeed.
2. Gen II: add the same guard to stageBoxPokemonEdit, stageBoxPokemonShiny, stageCloneBoxPokemon, stageAddBoxPokemon, the trainer setters (stageTrainerEdit, stageTrainerName, stageMoney, stagePlayTime, stageTrainerGender) and the inventory setters. Caution: stageReleaseBoxPokemon uses beginPackedMove internally (Gen2PackedMove.cpp:80) and must keep working.
3. Optionally, make Gen V emptyCloneDestination accept a slot whose only replacement is a RemoveBox native blank, for parity with Gen III/IV. This is a deliberate product decision and not required for safety.
4. Optionally, unify egg-clone policy across generations (owner decision).
Do not change the Gen IV single-drop swap or the Gen I/II insert placement. Both are lossless and already pinned by tests.

**Risks.**

The guard must not block the internal stageRemove calls in Gen I beginPackedGroupMove or the beginPackedMove call in Gen II stageReleaseBoxPokemon. Either would break pickup or Release, which the owner marked PASS. Do not add the guard to trainer or inventory setters that run before a Pokemon is carried. Leave the Gen IV swap behaviour and the Gen I/II insert placement as they are; both are hardware-accepted. The guards are only defense in depth while the UI blocks A and X during a carry. They must not change what the player sees or the status messages pinned by contract tests.

**Open questions.**

Should the owner confirm on hardware:
(a) Gen I/II: drop onto a full box is refused, and the carry stays.
(b) Gen III: drop onto an occupied slot is refused.
(c) Gen IV: a single drop onto an occupied slot swaps, which is the intended behaviour.
(d) Gen V: drop onto an occupied slot keeps the carry.
(e) In every generation, Import offered on an occupied or full target refuses.
The owner asked how to test this. Any ordinary save that has at least one full box and at least one occupied slot is enough as a fixture.

Owner decisions:
- Should Gen V accept a slot freed by a staged Release?
- Should egg cloning be allowed or refused consistently across generations?

Not verified: whether any touch-only path (for example taps on Bag/Trainer rows) can stage changes during a Gen I/II carry without going through the A/X keys that the overlay blocks.

**Files to change:** `src/Integration/Gen1/Gen1StagedPokemonEditor.cpp`, `src/Integration/Gen1/Gen1PackedMove.cpp`, `src/Integration/Gen2/Gen2StagedEditor.cpp`, `src/Integration/Gen2/Gen2PackedMove.cpp`, `tests/test_gen12_packed_move.cpp (or a new tests/test_occupied_destination_matrix.cpp)`, `Makefile.host.gen14 (register the new test)`

**Existing tests affected:**
- tests/test_gen12_packed_move.cpp and tests/test_gen12_packed_multimove.cpp: pin insert placement, full-box refusal (:265, :434, :191, :309) and carry-blocks-finalize. These must keep passing. Releasing (Gen II) and picking up (Gen I) go through stageRemove/beginPackedMove internally, so the guard placement must not break them.
- tests/test_rby_pokemon_editor.cpp:144-155 (Gen I add/clone refused on full box or occupied slot, bytes unchanged)
- tests/test_gsc_pokemon_editor.cpp:279-284 (Gen II full box refuses add/clone)
- tests/test_gen3_staged_pokemon_editor.cpp:694-740 (sparse carry: refuses occupied/wrap/edge, release blocked while carrying, cancel restores)
- tests/test_gen4_staged_pokemon_editor.cpp:469-474 (clone onto occupied refused, bytes unchanged), :497-560 (move/swap keeps both records)
- tests/test_gen5_party_box_parity.cpp:147-151, tests/test_gen5_staged_save_candidate.cpp:404-424 and 482, tests/test_gen5_staged_slot_actions.cpp:213-225 (Gen V clone/move/drop refusals)
- tests/test_pokemon_file_gen12.cpp:198-230 and 372-401 (Gen I/II import: occupied, not first empty, full, carried; Snapshot unchanged)
- tests/test_pokemon_file_gen3.cpp:415-443 and 502-507 (occupied, unreadable, carried)
- tests/test_pokemon_file_gen4.cpp:285 (occupied)
- tests/test_pokemon_file_gen5.cpp:314-323 (occupied, not a native blank, already holds a staged change)
- tests/test_qa_cross_gen_contract.py:721 (source contract for the beginGen2 carry message)

**New tests:**
- Gen I backend: during a carry, stageEdit(box), stageAdd, stageClone, stageRemove and revertPokemon are refused and the staged bytes are unchanged. Cancel then restores exactly the pickup snapshot. Before the fix this test fails: the probe showed all four calls accepted and their changes lost.
- Gen II backend: the same for stageBoxPokemonEdit, Shiny, Clone, Add, trainer and inventory setters. Release and pickup still work.
- Gen I stageClone onto an occupied slot inside a non-full box (s < count) and onto a non-contiguous slot (s > count): refused, bytes unchanged. Not covered today; only the full-box case is.
- Gen II stageCloneBoxPokemon of an egg is refused. Not covered today.
- Gen III stageCloneBoxPokemon onto an occupied slot and onto an unreadable-record slot: refused, staged_ and changes_ unchanged. Not covered today; only the import and sparse-move variants are.
- Gen III stageCloneBoxPokemon while a sparse carry is active is refused. Not covered today.
- Gen IV multi-select drop onto a partly occupied or overlapping target is refused (placeGen4 :752-767). No host test covers that UI logic; consider extracting it into a testable model.
- Gen V cloneStagedToEmptyBox / moveStagedToEmptyBox onto an occupied slot, and onto a slot freed by a staged Remove: refused (or accepted, if the parity change is made).
- Cross-generation matrix: for each of Clone/Move/Import x Gen I-V x {empty, occupied, full box, staged-replacement slot, egg source}, assert the result is ok/refused and that a refused call leaves the staged bytes AND the Review list unchanged.

**Skeptic corrections.**

I confirmed the main claims against the code. No shipped Clone, Move or Import path overwrites an occupied slot. The only defect is the latent carry-transaction gap in Gen I and Gen II, and it lives in the backend. Below are what I verified, what needs correcting, and what was missed.

VERIFIED
- Gen I pickup sets `packedMove_` before calling `stageRemove` (src/Integration/Gen1/Gen1PackedMove.cpp:58-64). Cancel commits the whole `beforeBytes` image back (:110-115). Placement copies the carried records from `beforeBytes`, not from the live staged image (:98-99).
- Gen I guards exist only at Gen1StagedPokemonEditor.cpp:347 (party edit), :511 (import), :572 and :591 (exports) and :602 (finalize). `stageEdit` (:343-345), `stageAdd` (:423), `stageClone` (:474), `stageRemove` (:486) and `revertPokemon` (:498) have no guard. Confirmed.
- `destination()` refuses unless `s == count` (Gen1StagedPokemonEditor.cpp:242-248). Confirmed.
- Gen II `cancelPackedMove` restores `staged_`, `changes_` and `pokemonExpectations_` (Gen2PackedMove.cpp:79). `stageReleaseBoxPokemon` is guarded at its top, then calls `beginPackedMove` (:80). Confirmed.
- Gen II guards exist only at Gen2StagedEditor.cpp:962 (party edit), :1490 (import) and :1574, :1609, :1648. The egg-clone refusal is at :1263-1265. Confirmed.
- Gen III `carryingSparseMove` is checked at Gen3StagedPokemonEditor.cpp:402 (`stageRecordEdit`, which covers both the box path and the party path), :751 (Add), :955 (Clone), :1006 (Release), :1110 (Import) and :1386 (finalize). Confirmed.
- The UI blocks A and X during any carry (src/UI/ClassicPackedMoveOverlay.inc:1121-1135). I traced the touch paths too. The storage-pane tap that turns into A (TrainerViewScreenCompositeOverlay.cpp:428-434) is injected before `ClassicPackedMove::handleInput` (:439-440), so the carry branch still refuses it. Taps on 1002 (box-name pill) and 2000 (summary panel), which become A on the base path (TrainerViewScreenBase.inc:4748-4753), are consumed by the carry layer. Storage carry consumes every tap (ClassicPackedMoveOverlay.inc:953-965). Review opens only when `idle()` is true (TrainerViewScreenCompositeOverlay.cpp:281-283, ClassicPackedMoveOverlay.inc:87-90). Changing view mode needs the home menu, and B or Plus cancel the carry first. Party actions run earlier in the chain (CompositeOverlay.cpp:391), but they are party-only and the backend refuses them. This settles the investigator's open question: I found no touch-only path that stages a change during a Gen I or Gen II carry. The gap is latent, not reachable today.

CORRECTIONS
1. The Gen II trainer-setter consequence is stated wrongly. It is not a silent discard; it leaves the editor in an inconsistent state. `stageTrainerName` and `stageMoney` update both `staged_` and the cached `trainerName_` and `money_` (Gen2StagedEditor.cpp:744, :756). `cancelPackedMove` restores `staged_` and `changes_` but not `trainerName_` or `money_` (Gen2PackedMove.cpp:79). After a cancel, the UI shows the edited name and money (header :87, :89) while the bytes are reverted. `finalizedBytes` then fails on the check `reparsed name != trainerName_ || money != money_` (Gen2StagedEditor.cpp:1669), so Save gets stuck on an error until Discard. Play time, gender and inventory are byte-only and really are discarded silently. This makes the trainer-setter guard more important than the investigator suggested.
2. The test coverage claim is slightly off. tests/test_rby_pokemon_editor.cpp:145-146 already pins Gen I `stageAdd` onto an occupied slot (2,0) and onto a non-contiguous slot (2,2) in a non-full box. Only the Clone variant is missing. It goes through the same `destination()` check, so it is low value.
3. The claim "Gen IV/V stage nothing until drop" is right, but it rests on the UI layer (`placeGen4`/`placeGen5`), not on a backend carry state. Any backend-level matrix test for Gen IV and Gen V has to go through the UI model or Gen5SharedScreenState.

MISSED (same defect class)
4. `discard()` does not clear the carry in Gen I or Gen II. Gen I `discard` (Gen1StagedPokemonEditor.cpp:599) resets `view_`, `changes_` and `imported_` but leaves `packedMove_` active. Gen II `discard` (Gen2StagedEditor.cpp:1630-1637) leaves `packedMove_` active as well. Gen III clears the carry (Gen3StagedPokemonEditor.cpp:1380-1384). If Discard ran during a Gen I carry, a later cancel would commit `beforeBytes` and bring back everything that was just discarded. A later place would copy records out of the stale snapshot. Discard is called from Gen1PokemonEditorOverlayUX.inc:744, Gen1PokemonEditorOverlay.cpp:599, ClassicInventoryOverlay.cpp:321-324 and Gen2SharedPokemonSurface.inc:273. Every reachable entry I traced is gated by the carry or idle checks, so this is latent too. The fix should add `packedMove_ = {}` to both `discard()` functions, or refuse while carrying, and needs a test.
5. The UI layer and the carry state are owned separately. `classicPackedMoveLayerAvailable` (CompositeOverlay.cpp:250-270) switches off the carry layer, and with it the A/X block, whenever a Gen I UX2 mode or a Gen II editor overlay is open. The only thing keeping that safe today is that nothing can open those modes during a carry. It is another reason for the backend guards.

FIX REVIEW
- The proposed guards are correct, and the caveats about pickup and Release are valid. In Gen I, `beginPackedGroupMove` calls `stageRemove` after setting `packedMove_.active` (Gen1PackedMove.cpp:58-61). A guard inside `stageRemove` would make every pickup fail. Use a private, unguarded remove helper.
- In Gen II, add `trainerName_` and `money_` to `PackedMoveState` and restore them on cancel, or guard the setters (preferred). Guarding is the better choice.
- Do not guard `revertPokemon` in Gen I without checking whether any cancel or Review path calls it. I found none inside the carry code.
- The existing PASS behaviour (insert placement, the Gen IV swap, Release, pickup) is unaffected as long as the guards sit on the public entry points only.

**Missed generations or paths.**

1. Gen II trainer name and money: `cancelPackedMove` (src/Integration/Gen2/Gen2PackedMove.cpp:79) restores `staged_` but not the cached `trainerName_` or `money_` (Gen2StagedEditor.cpp:744, :756). Finalize then fails its consistency check (Gen2StagedEditor.cpp:1669) and the UI shows stale values. This is a stuck or inconsistent state, not a silent discard.
2. Gen I `discard()` (Gen1StagedPokemonEditor.cpp:599) and Gen II `discard()` (Gen2StagedEditor.cpp:1630-1637) do not clear `packedMove_`. Gen III does (Gen3StagedPokemonEditor.cpp:1380-1384). A Gen I cancel after a discard would bring the discarded changes back. It is latent; the UI gates block it today.
3. The UI A/X block disappears whenever `classicPackedMoveLayerAvailable` returns false (TrainerViewScreenCompositeOverlay.cpp:250-270). The block is not tied to the carry state itself.
4. I closed the investigator's open question about touch paths. Every tap that becomes A is either injected before the carry layer (CompositeOverlay.cpp:428-434) or consumed by it (ClassicPackedMoveOverlay.inc:1134-1137, 953-965). Review needs `idle()`. I found no reachable UI path today.
5. Test note: Gen I Add onto occupied and non-contiguous slots is already pinned at tests/test_rby_pokemon_editor.cpp:145-146. Only the Clone variant is new. New tests are also needed for discard during a carry (Gen I and Gen II) and for a Gen II trainer name or money edit during a carry followed by cancel and finalize.

</details>

### FRLG — FireRed/LeafGreen species and met-location data (P0)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 4 Known missing v1.0 functionality, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Gen III, all five games (Ruby, Sapphire, Emerald, FireRed, LeafGreen). FRLG is not singled out. Gen II Crystal has the same closed-list pattern: Gen2PokemonPickerModel.h:63-75 uses forGameSpecies, and Gen2UnifiedPokemonWorkspace.inc:603-605 refuses with "No supported Crystal encounter templates for this species". It is out of scope here but should inherit the same fix. Gen IV and Gen V already work correctly: their picker is the full native location list plus context (SharedPokemonEditorSurface.inc:206-215, Gen4SharedPokemonSurface.inc:666-687, Gen5SharedReadOnlySurface.inc:477). Gen I has no met data.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

SPECIES (the owner's guess is not confirmed). Every Gen III game, FRLG included, offers the whole National Dex 1-386 and nothing is filtered by game: gen3SpeciesAvailable always returns true (src/UI/Gen3SharedPokemonSurface.inc:1289) and is used for refilter(...,386,...) at :1303/:2438. Session::setSpecies only checks 1..386 (include/UI/Gen3SharedPokemonSession.h:114-115). The backend checks the same range (src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:439-441 for edit, :769 for create). So Rayquaza (384) can be picked in FireRed and LeafGreen. Nothing reads RSE species data. Species names, base stats and learnsets are FRLG/Gen III data (Pokemon3FRLG.cpp:34,124-129; the LEARN_FRLG tables in LearnsetTable.cpp:31755+).

MET LOCATION DATA. The data itself is correct and FRLG-specific. The picker reads Encounter::forGameSpeciesWithGen3Provenance(screen.sourceGameId, species) (Gen3SharedPokemonSurface.inc:727-729). The rows come from kEncounterTemplates in include/Integration/Encounter/EncounterGuardrailsData.inc, generated from PKHeX 08c27668 (Encounters3FRLG.cs plus the encounter pkl files). The firered_gba and leafgreen_gba rows use only Kanto/Sevii IDs 88-194 plus 253, for example Bulbasaur at Pallet 88 (line 2530), Mewtwo at Cerulean Cave 141 (:3212) and Articuno at 139 (:3207). Emerald rows use Hoenn IDs 0-85 and 197-211. Display names come from the shared G3_LOC_NAMES table (src/Names/LocationNames.cpp:25-258, which covers both regions, used at :4247), called with the record's origin game.

THE ACTUAL FAILURE. The templates cover only native catches. FireRed has templates for 125 of 386 species and LeafGreen for 127 (Ruby 124, Emerald 156). That leaves 261 and 259 species in FRLG with no rows. The evolution-ancestry fallback has exactly one edge, Horsea->Seadra (EncounterGuardrails.h:84-86). So any evolved species without its own wild row has no choices, for example Ivysaur, Charizard, Raichu, Alakazam, Gengar and Dragonite, as does every non-Kanto species such as Rayquaza. When the list is empty, the picker posts "No supported exact-game encounter templates for this Pokemon" and refuses to open (Gen3SharedPokemonSurface.inc:730-733). It returns before the R2-BUG-008 "Keep stored" row is built (:736), so even a stored location cannot be shown in the picker. The Met Level row then refuses with "Choose a legitimate Met Location encounter first" (:944-958). Changing species clears the met location to 0 when it no longer matches a template (:1347-1365). Create starts with metLocation=0 and metLevel=5 (Gen3SharedPokemonSession.h:95-109). The Create backend accepts location 0 (Gen3StagedPokemonEditor.cpp:769-774, :795). It forces originGame to the save's own game (:792, checked at :900), and Origin is locked (Gen3SharedPokemonSurface.inc:487-489). The result: a Rayquaza created in FireRed is written as FireRed origin, met location 0, met level 5. Location 0 is "Littleroot Town", a Hoenn ID (LocationNames.cpp row 0), but the editor shows it as "None" (include/Names/LocationDisplayNames.h:72).

TRADE/TRANSFER. Not distinguished. Create offers no traded-in history: origin stays the save's game, and there are no rows for in-game trade (254), fateful encounter (255) or an RSE/Colosseum/XD origin. In Edit, the templates come from the save's game (screen.sourceGameId), not the record's originGame. So a Ruby-origin Pokemon in a FireRed save is offered Kanto rows, and choosing one writes a Kanto location under Ruby origin (backend only checks <=255, Gen3StagedPokemonEditor.cpp:573-578). Gen IV handles this differently: it uses the record's own origin and refuses met edits for non-native origins (Gen4SharedPokemonSurface.inc:668-676).

Data gaps also found in the FRLG templates. There are no in-game trade rows (Mr. Mime 122, Jynx 124, Lickitung 108, Farfetch'd 83 are absent) and no Gift-method rows. Deoxys (386) Birth Island 187 appears for LeafGreen only, not FireRed. That needs checking against PKHeX, which is not available here.

**Root cause.**

The Gen III Met Location picker is a closed list of native encounter templates for the save's game, and it fails closed when the list is empty. Evidence: src/UI/Gen3SharedPokemonSurface.inc:727-733 (refuse on empty, before the keep-stored row at :736). The templates hold native catches only (125 of 386 species for FR, 127 for LG). The Gen III evolution-ancestry table has one edge (include/Integration/Encounter/EncounterGuardrails.h:84-86). Create pins origin to the save's game and leaves metLocation=0, which is the Hoenn ID "Littleroot Town" but is shown as "None" (Gen3StagedPokemonEditor.cpp:792-795; Gen3SharedPokemonSession.h:95-109; LocationDisplayNames.h:72). Gen IV/V do not have this problem because they use the shared EncounterContext45::Catalogue picker: the full native location list, with encounter context as advice only and an X toggle for other histories. That picker only knows generations 4 and 5 (EncounterContext45.h:14-31, 61-84), so Gen III was never moved onto it. The species list is not the cause: it is the whole National Dex 1-386 for every Gen III game.

**Shared owner.**

include/Integration/Encounter/EncounterContext45.h (Catalogue) together with SharedPokemonEditorSurface.inc::openContextualMetLocationPicker, generalised to Gen III so Gen III uses the same contextual met-location picker as Gen IV/V. Gen II Crystal could later inherit it. The encounter facts stay in EncounterGuardrails (kEncounterTemplates).

**Proposed fix.**

Minimal fix:
(1) Extend EncounterContext45 to Gen III. gameId() should map origin 1-5 to sapphire/ruby/emerald/firered/leafgreen_gba and generation() should return 3. Catalogue should read Gen III wild/static rows from kEncounterTemplates for the record's origin game. Ancestry should use Gen III evolution edges, either the kEvolution45 edges whose child and parent are both 386 or lower, or a generated Gen III edge table. This replaces the single Horsea->Seadra edge.
(2) In Gen3SharedPokemonSurface.inc openValuePicker(MetLocation), build the native list from the record's origin game. That list is the non-empty G3_LOC_NAMES IDs valid for that origin: FRLG 88-196 plus 253/254/255; RSE 0-87 and 197+ as PKHeX defines them. Remove the empty-list refusal. The stored value is always row one, as Gen IV/V already do.
(3) Base the choices on the record's originGame, not screen.sourceGameId. That fixes the Ruby-origin-in-FireRed mismatch. Alternatively, follow Gen IV and make met location read-only for origins other than the save's game.
(4) Met Level: when no template matches, allow a manual 0/1-100 entry instead of blocking on "Choose a legitimate Met Location encounter first".
(5) Create: require an explicit met location, as Gen V's createNeedsMetLocation does ("Choose a location"), instead of silently writing 0. If 0 must stay as a value, display it as "Littleroot Town (0)".

Out of scope for a P0 repair and a v1.0 feature decision: Create with a non-native origin, so that a Rayquaza in an FRLG save can be recorded as Ruby/Sapphire/Emerald origin, met at Sky Pillar (85), Lv70. That needs an Origin picker in Create and backend validation of the origin/location pairing (Gen3StagedPokemonEditor.cpp:792,900). Do not restrict the species list to the regional dex.

**Risks.**

Save-byte risk is low. The PK3 met-location field is 8 bits and the backend already bounds it (Gen3StagedPokemonEditor.cpp:574-578, 774); the fix changes which values the UI offers, not how they are written. The main legality risk is a wider list that lets users choose implausible native locations. Gen IV/V handle this by focusing on context rows and putting other histories behind X; Gen III should copy that. Owner-PASS behaviour to protect:
- the R2-BUG-008 keep-stored row;
- the Horsea->Seadra provenance text ("met as Horsea (evo. 32)");
- fixed-level encounters setting the met level automatically;
- the 620 px picker width (test_met_location_fit);
- the Gen IV/V Catalogue picker, which is shared code, so generalising it must not change Gen IV/V output.
Changing the meaning of Create's location 0 changes the default draft and needs the session tests updated. Allowing a non-native origin in Create changes the origin check at Gen3StagedPokemonEditor.cpp:900 and must stay verified by the post-write re-read.

**Open questions.**

1) Owner decision: should Create in an FRLG save let the user pick a non-native origin (R/S/E, or Colosseum/XD) so traded legendaries such as Rayquaza get a correct origin and location? Or should v1.0 only allow native-origin Create with any National Dex species plus a free met location? 2) Hardware: how do FireRed and LeafGreen summary screens show met location 0 (a Hoenn ID) on a FireRed-origin record, as the current Create writes it? Does the game show the Hoenn place name or a 'distant land'-style message? 3) Data: confirm against PKHeX that FireRed (not only LeafGreen) has the Deoxys Birth Island 187 event row, and whether the in-game trade and Game Corner rows were dropped by the generator on purpose. 4) Should Gen II Crystal (same closed-list refusal at Gen2UnifiedPokemonWorkspace.inc:603-605) be fixed in the same repair? 5) Hardware retest: in FireRed and LeafGreen, open Create, choose Rayquaza, open Met Location and confirm the 'No supported exact-game encounter templates' toast on the Round 10 build; then repeat with Charizard to show it is not limited to non-Kanto species.

**Files to change:** `include/Integration/Encounter/EncounterContext45.h`, `include/Integration/Encounter/EncounterGuardrails.h`, `src/UI/Gen3SharedPokemonSurface.inc`, `include/UI/Gen3EncounterPickerModel.h`, `include/UI/Gen3SharedPokemonSession.h`, `src/Names/LocationNames.cpp (native Gen III location validity helper, analogous to isGen4NativeMetLocation)`, `src/Integration/Gen3/Gen3StagedPokemonEditor.cpp (only if Create gains a non-native origin, or to validate origin/location pairing)`

**Existing tests affected:**
- tests/test_gen3_hardware_surface_contract.cpp:209-217 (pins the forGameSpeciesWithGen3Provenance call and the literal 'No supported exact-game encounter templates for this Pokemon' refusal; must be deliberately updated)
- tests/test_encounter_guardrails.cpp:33-60 (Seadra/Horsea provenance; asserts Sapphire Mewtwo 150 has empty provenance; location 88 not allowed for Sapphire Seadra; these stay valid as facts but may move to the Catalogue layer)
- tests/test_gen3_encounter_keep_stored.cpp (R2-BUG-008 keep-stored row semantics)
- tests/test_met_location_fit.cpp:123 (row-text width over forGameSpeciesWithGen3Provenance choices)
- tests/test_met_location_rows_distinct.cpp
- tests/test_gen45_encounter_context.cpp (Catalogue behaviour for Gen IV/V must be unchanged)
- tests/test_gen3_shared_pokemon_session.cpp (beginCreate defaults metLocation 0 / metLevel 5)
- tests/test_gen3_staged_pokemon_editor.cpp (Create writes origin = save game)

**New tests:**
- Gen III Catalogue: for origin 4 (FireRed) and species 384 (Rayquaza) the choices are non-empty, the stored value comes first, and the other-histories list has only FRLG-valid IDs (88-196, 253-255), with no Hoenn IDs 1-87/197+.
- Gen III Catalogue: for origin 3 (Emerald) and species 384, Sky Pillar (85) is a wild/static context row; for origin 4 it is not offered.
- Ancestry coverage: every species 1-386 that has a native FRLG template, or whose Gen III ancestor does, gets context rows (for example Charizard -> Charmander at Pallet 88; Raichu -> Pikachu at Viridian Forest 126/Power Plant 142).
- Edit of a Ruby-origin record inside a FireRed save offers Hoenn locations (or is read-only), never Kanto rows; the backend refuses or flags a Kanto location with Ruby origin.
- Create in FRLG with a non-template species (384) can choose a location and met level and stage, with the bytes verified by the Gen3StagedPokemonEditor round trip; Create with no location chosen is blocked, not written as 0.
- Regression: Gen IV/V Catalogue output is byte-identical for a fixed sample of origin/species pairs.
- Data audit: compare the FRLG template rows to PKHeX Encounters3FRLG for in-game trades (83/108/122/124 at 254) and the FireRed Deoxys at Birth Island 187.

**Skeptic corrections.**

I confirmed the root cause against wmain HEAD 67b1b6a0. The Gen III Met Location picker is a closed list of templates for the save's game, and it fails closed when that list is empty. Species is not the cause.

**Claims I confirmed directly**
- src/UI/Gen3SharedPokemonSurface.inc:727-733 refuses with "No supported exact-game encounter templates for this Pokemon" before the R2-BUG-008 keep-stored row is built at :735-741.
- The Met Level row refuses at :944-958 with "Choose a legitimate Met Location encounter first".
- gen3SpeciesAvailable always returns true (:1289), and the species range is 386 (:1302-1303).
- When species changes, the met location is cleared only if it was not 0 (:1344-1365).
- Gen3SharedPokemonSession.h:95-109: beginCreate sets metLevel=5 and leaves metLocation=0. :114 setSpecies only checks 1..386.
- Gen3StagedPokemonEditor.cpp:439-441 and :573-578 (edit), :769-774, :792 and :900 (create; origin forced to the save's game, location 0 accepted).
- LocationNames.cpp:28: G3 location 0 is "Littleroot Town". LocationDisplayNames.h:72 shows it as "None".
- EncounterGuardrails.h:84-86 has a single Horsea->Seadra edge.
- EncounterGuardrailsData.inc:2530 is Bulbasaur FR at location 88.
- My own count of the data matches theirs: FR 125, LG 127, Ruby 124, Sapphire 127, Emerald 156 species. FRLG uses locations 88-253. Deoxys 386 appears only for emerald (200) and leafgreen (187), not firered. Species 83, 108, 122 and 124 have no FRLG rows (Crystal only). Rayquaza 384 appears only for R/S/E.
- Neither FRLG game has Gift-method rows. FRLG does have Static, Event and Egg rows.

**Corrections and omissions**

(a) The proposed shared owner cannot be dropped in as described.
- Gen3SharedPokemonSurface uses its own State (Gen3SharedPokemonSurface.inc:120-150). It has no pickerMetContext and no pickerMetContextActive, and does not use SharedPokemonEditorSurface's EditorState (SharedPokemonEditorSurface.inc:75). So Surface::openContextualMetLocationPicker (SharedPokemonEditorSurface.inc:206-215) cannot be called from Gen III as-is.
- Gen III picker values are not location IDs. They are indices into encounterChoices, or EncounterPicker::kKeepStoredRow. See applyValuePicker (:823-828, choiceAt(state.encounterChoices, value)) and metLocationChoiceText (:601-610).
- Moving to Catalogue therefore changes what a picker value means. It also touches the R2-BUG-008 keep-stored contract, test_gen3_encounter_keep_stored, and the 620 px row-text path.

(b) EncounterContext45::Catalogue reads kWild45 (EncounterContext45.h:69-70; EncounterContext45Data.inc:5), not kEncounterTemplates. The ancestry check returns early for any generation other than 4 or 5 (:35). So "make Catalogue read Gen III rows from kEncounterTemplates" means a second data source inside Catalogue, or a Gen III table in its own format. It is not a one-line mapping in gameId()/generation().

(c) The ancestry idea of reusing kEvolution45 edges with child and parent both at 386 or lower needs the edge.generation filter (:41). That filter excludes Gen IV-only methods, so filtering only by species ID is not enough.

(d) They missed a third sourceGameId consumer: encounterOriginText (Gen3SharedPokemonSurface.inc:418-433). It uses screen.sourceGameId and returns "Unknown" for location 0 and "Unresolved" for any location without a template. Every evolved FRLG species and every traded-in species shows "Unresolved" in the origin text, so this needs the same switch to the record's origin game.

(e) There is also a template/name mismatch inside the picker. Template locations come from sourceGameId (:728-729), but their names are drawn with working.originGame (:610). For a Ruby-origin record in a FireRed save, Kanto IDs are shown under Ruby-origin naming. This confirms their trade/transfer point at a precise line.

(f) Their "line :4247" reference for the G3_LOC_NAMES call site is unverified. The callers I found are LocationDisplayNames.h getMetLocationDisplayName and Gen3SharedPokemonSurface.inc:409, :604 and :610.

(g) Their classification should stay as given (1, 2, 4 and 6), but on the species point the owner's wording is incorrect. No RSE species list is read. Rayquaza is selectable today. It simply cannot get a met location in FRLG.

**Missed generations or paths.**

**Generations**
- Gen II Crystal has the same closed-list failure. Gen2UnifiedPokemonWorkspace.inc:600-606 refuses with "No supported Crystal encounter templates for this species". The same closed list is used at :303 and :564, and in Gen2PokemonPickerModel.h:66 via Encounter::forGameSpecies.
- Gen I has no met data.
- Gen IV is protected: Gen4SharedPokemonSurface.inc:666-687 uses the full native list plus the shared Catalogue, and refuses non-Gen IV origins.
- Gen V uses the same Catalogue.

**Missed Gen III paths**
- encounterOriginText at Gen3SharedPokemonSurface.inc:418-433 (displays "Unresolved" or "Unknown").
- The template/name mismatch between :728 (sourceGameId) and :610 (originGame).
- Create never requires a location, so it silently writes Hoenn ID 0 under FRLG origin (Gen3StagedPokemonEditor.cpp:769-795).

**Data gaps beyond what they listed**
- Species 6, 26 and 149 (Charizard, Raichu, Dragonite) have no template in any game. Evolved forms are unreachable everywhere in Gen III, not only in FRLG.
- No Gen III game has Gift rows. Ruby and Sapphire have no Event rows.

**Fix risks**
- Generalising Catalogue changes shared Gen IV/V code: ancestry() at EncounterContext45.h:33-46 and the constructor. Gen IV/V output must be pinned by test_gen45_encounter_context before and after the change.
- Swapping index-based Gen III picker values for location-ID values must keep these owner-PASS behaviours intact:
  - the keep-stored row (test_gen3_encounter_keep_stored);
  - fixed-level auto met level (:959-964);
  - the "met as Horsea (evo. 32)" text.
- The less invasive option is to keep Gen III's State and instead:
  1. Build native IDs from the record's originGame.
  2. Remove the empty-list refusal at :730-733.
  3. Use templates only as advice.
- tests/test_gen3_hardware_surface_contract.cpp pins the literal refusal string and must be updated deliberately.

**Save safety**
The byte-level risk is low because the 8-bit bounds are enforced at Gen3StagedPokemonEditor.cpp:574 and :769-774. Allowing a non-native origin in Create would change the check at :900.

</details>

### H04-H34 — Bulk Top Up / Add Missing scopes, PC items, Gen I (P0)

- **Classification:** 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** All five generations use the same shared model, so the setup screen behaves the same everywhere: the scope starts at "This pocket" and "All Bag pockets" never includes the PC. PC Items exists only in Gen I, II and III. In each of them, Top Up / Add Missing changes the PC only when the PC Items tab is open and the scope is "This pocket". Gen IV and V have no PC item storage, so nothing about PC applies there. Gen I is affected the most: its Bag is a single 20-slot list that the Items, Medicine, Balls, Key Items and TMs tabs all share, so "add missing" across every pocket can never reach TMs. Gen II and III have separate pockets with their own slots, so "All Bag pockets" fills each pocket on its own and is correct today. Gen IV and V are also correct today, Bag only.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

How to open it. ZR on the Bag tab, or Inventory Options row 3 in Gen I-III (src/UI/ClassicInventoryOverlay.cpp:983-986 and :1010-1013). The footer shows "ZR: Top Up" (include/UI/InventoryUIContract.h:57), and the setup modal comes from include/UI/SharedBagTopUp.h:97-114.

The Scope row.
- Scope is the second setup row. The model starts it at `allPockets = false` (include/UI/BagTopUpModel.h:50), so the row reads "This pocket (<name>)" (:103).
- The only way to change it is to move down to Scope and press Left, Right or A (:121). The setup modal never mentions PC Items.
- Once changed, the choice is kept for the rest of the session (:57-66). On first use it is "This pocket".

Which pockets get planned.
- `request()` plans either just the pocket on screen or the list it is given (BagTopUpModel.h:136-143).
- Every surface passes `BI::bagPockets(pockets)` as that list. The call sites are ClassicInventoryOverlay.cpp:695-696, Gen4SharedPokemonSurface.inc:1771-1772 and Gen5SharedReadOnlySurface.inc:1016-1017.
- `bagPockets()` removes Pocket::PCItems on purpose (include/Inventory/BulkItemPlan.h:104-110). So "All Bag pockets" never touches the PC in Gen I, II or III.
- PC Items is still a normal tab in all three. On that tab, "This pocket" sends Pocket::PCItems to the planner, and the adapters already build a PC PocketState for it: Gen I at BulkItemAdaptersClassic.h:63-64 with 50 slots, Gen II at :154-167 using inventoryCapacity, Gen III at :233-246 with 50 slots for RSE and 30 for FRLG (src/Integration/Gen3/Gen3StagedInventoryEditor.cpp:34,43,52).
- So the PC can only be filled one tab at a time, and that path has never been shown or tested as a feature.

Gen I specifically.
- Gen1Bag::pockets marks every Bag tab with slotGroup 0. Together they share one capacity of bagHeld + bagFree, which is 20 slots (BulkItemAdaptersClassic.h:52-75; kBagCapacity = 20 at src/Integration/Gen1/Gen1ReadOnlyInventory.cpp:16).
- The planner counts those free slots once for the whole group (BulkItemPlan.h:124-133). It then works through the tabs in catalog order: Items, Medicine, Balls, Key Items, TMs, PC (src/Inventory/ClassicInventoryCatalog.cpp:31-34). Within each tab it adds item IDs in ascending order (BulkItemPlan.h:167-187).
- The Items tab alone offers 19 candidates (ClassicInventoryCatalog.cpp:46-48), so the free slots are used up before Medicine and long before TMs.
- Every item after that is reported as PocketFull, "pocket full" (BulkItemPlan.h:182). This is pinned by tests/test_bag_top_up_adapters.cpp:153, `plan.adds.size() == 2`, which only has 2 free slots in total.
- Top-ups across all tabs do work: the Poke Ball and TM top-ups at :155-156. The exceptions are items held in more than one stack, which are skipped as SeveralStacks (:154).

Coverage gaps.
- No code path makes Gen I fail outright. Any Japanese-region Gen I or II save is refused at ClassicInventoryOverlay.cpp:675-677.
- The real-save harness never runs English Gen I or II saves. Files named _G1_/_G2_ are skipped as Japanese, and other Gen I/II names fall through to "NO FIXTURE" (tests/test_bag_top_up_real_saves.cpp:139-158). So Gen I/II have only synthetic-fixture coverage.

**Root cause.**

There is no software crash or write bug. Three separate causes add up to what the owner saw.

(a) Scope defaults to "This pocket". include/UI/BagTopUpModel.h:50 sets `bool allPockets = false;`, and the choice is only changed through the second setup row (:121). An owner who presses ZR and goes straight to Preview gets the current pocket only, in every generation. That matches "only fill the current menu" in H04 and "only does current bag" in the extra note. Whether the owner ever toggled Scope needs confirming on hardware.

(b) PC Items is deliberately left out of the all-pockets scope. include/Inventory/BulkItemPlan.h:104-110 removes Pocket::PCItems from bagPockets(), and BagTopUpModel.h:7 and :140 apply it. There is no "PC Items" or "Bag + PC" scope. This is a design decision from Feature B, documented in the same lines and pinned by tests, not a defect. The owner is now asking for new behaviour (H34).

(c) Gen I has a hard native limit. All five Bag tabs view one 20-slot list (BulkItemAdaptersClassic.h:62-75 and the Gen1ReadOnlyInventory.cpp:16 capacity). The planner fills slots in pocket order and then ascending item ID (BulkItemPlan.h:157-187; pocket order at ClassicInventoryCatalog.cpp:31-34). So in Add Missing mode the Items tab uses up every free slot, and TMs and most Medicine are always skipped as "pocket full". This is how the game itself stores the Bag (pret wNumBagItems, 20 entries). It cannot be "fixed" so that every tab fills. The preview text "pocket full" (BulkItemPlan.h:93) does not explain that the 20 slots are shared across tabs, which is why it looks like Gen I does not work.

**Shared owner.**

Put the scope change in include/UI/BagTopUpModel.h (Scope choices, request()) and include/Inventory/BulkItemPlan.h (a pcPockets helper next to bagPockets, plus the skip-reason text). That way all three classic surfaces and the Gen IV/V surfaces pick it up without per-generation code. The surfaces only need to stop passing a bare bagPockets(...) and pass the full PocketState list instead: ClassicInventoryOverlay.cpp:695-696, Gen4SharedPokemonSurface.inc:1771-1772 and Gen5SharedReadOnlySurface.inc:1016-1017. The adapters in BulkItemAdaptersClassic.h already build and write PC PocketState for Gen I, II and III and need no change.

**Proposed fix.**

Minimal, shared, opt-in. Nothing changes silently for the owner.

1. BagTopUpModel.h: replace `bool allPockets` with `enum class Scope { ThisPocket, AllBag, PcItems, BagAndPc }`.
   - Left and Right cycle only through the choices that exist for this save, worked out from the PocketState list.
   - Gen IV/V never show PcItems or BagAndPc because their state has no Pocket::PCItems.
   - request() takes the full state: AllBag maps to bagPockets(state); PcItems maps to {PCItems}; BagAndPc maps to bagPockets plus PCItems.
   - Keep ThisPocket as the default; see the open questions. Optionally show a hint on the Scope row or the context line, e.g. "Left/Right: This pocket / All Bag / PC Items", so the choice can be found.
2. BulkItemPlan.h: add `pcPockets(state)` next to bagPockets.
   - No planner change is needed for PC capacity or exclusivity. Key Items and HMs are already skipped by item kind, so a Bicycle, HM or key item offered by kG1PC or the Gen II/III PC lists is reported, never added (src/Inventory/ItemQuantityPolicy.cpp:306-314).
   - Exclusivity already counts held items across Bag and PC (BulkItemPlan.h:119-122).
   - PC caps come from nativePocketCap: Gen I/II 99, RSE PC 999, FRLG PC 999 (ItemQuantityPolicy.cpp:205-208).
3. Gen I explanation, no change to native behaviour: give PocketFull a group-aware text, e.g. "Bag full (Gen I: 20 slots shared by every category)", when the pocket has slotGroup >= 0.
   - Optionally interleave or sort Gen I adds differently. Recommend against it: any order still caps at 20 stacks.
   - Optionally add a preview note line saying how many Bag slots were free.
   - For Gen I, "fill everything" can realistically only mean "PC Items" (50 slots, 124 candidates, so ascending-ID order still leaves most TMs as "pocket full") or topping up what is already held.
4. Surfaces: the three call sites pass `pockets` (the full state) instead of bagPockets(pockets). The preview context line at SharedBagTopUp.h:125 names the scope from a model label function instead of the allPockets ternary.
5. Key Items and HMs stay excluded. The owner's words, "key items dont worrry about tho", match the current policy, so no override.

**Risks.**

Save safety is unchanged as long as the applier and adapters are reused. PC writes already go through the same staged editors: Gen1 stageSetQuantity on a PC pocket, Gen2 pair-list slot writes, and Gen3 pouch writes, and apply() is all-or-nothing with read-back.

PC risks:
- Flooding a 50-slot PC with ascending-ID items, e.g. Gen I Balls, Potions, then key-item IDs skipped, could crowd out room the player wants. The preview lists every add, which mitigates this.
- FRLG PC is only 30 slots.

Changing the default scope to AllBag would change the PASS-observed flow for anyone used to "This pocket", so keep the default unless the owner asks.

Gen II/III/IV/V All-Bag behaviour, which the owner did not fail, must stay exactly the same. The tests at :293, :384, :450 and :513 guard this.

Gen I: no reordering can make every category fit into 20 slots. Promising "fill all" there would be misleading, so the UI must explain the limit rather than appear to fix it.

**Open questions.**

- Owner (hardware): on the failing runs, did you change the Scope row to "All Bag pockets"? The code defaults to "This pocket" (BagTopUpModel.h:50). Please re-test H04 with Scope set to All Bag pockets on Gen II/III/IV/V. Expected result: every non-key pocket fills.
- Owner decision: should a PC Items scope be added (PC only, or Bag + PC)? And should the default scope stay "This pocket"?
- Owner decision for Gen I: accept that the 20-slot Bag cannot hold every category, with a clearer "Bag full (20 shared slots)" message? Or prefer a different fill priority, e.g. TMs before Items? Any priority still caps at 20 stacks.
- Owner decision for the PC fill order: ascending ID, or by category? Gen I's PC holds 50 of 124 candidates.
- Confirm on hardware that an English Gen I save shows "ZR: Top Up" in the footer. The code path is gated only by stagedInventoryAvailable and the Japanese gate (ClassicInventoryOverlay.cpp:157-165, :650-668). There is no real-save automated coverage for English Gen I/II.

**Files to change:** `include/UI/BagTopUpModel.h`, `include/Inventory/BulkItemPlan.h`, `include/UI/SharedBagTopUp.h`, `src/UI/ClassicInventoryOverlay.cpp`, `src/UI/Gen4SharedPokemonSurface.inc`, `src/UI/Gen5SharedReadOnlySurface.inc`, `include/UI/InventoryHelp.h`, `tests/test_bag_top_up_adapters.cpp`, `tests/test_bulk_item_plan.cpp`, `tests/test_bag_top_up_real_saves.cpp`

**Existing tests affected:**
- tests/test_bag_top_up_adapters.cpp:75 planIsSafe CHECK(a.pocket != PCItems). This stays true for the AllBag scope but must become scope-aware once PC scopes exist. Deliberate update: pass the expected scope.
- tests/test_bag_top_up_adapters.cpp:149,293,384,450,513 build the request as Model::request(State{allPockets=true}, current, bagPockets(state)). The signature and field change to Scope::AllBag plus the full state; same expected results.
- tests/test_bag_top_up_adapters.cpp:153 Gen I plan.adds.size()==2 and :157 full>0. These pin the shared 20-slot behaviour, which is correct, and must be kept. Only the skip text changes if the reason wording is made group-aware.
- tests/test_bag_top_up_adapters.cpp:552-572 model(): setupInput toggling of allPockets, request() with allPockets true/false, and bagPockets excluding PCItems. Update for the Scope enum cycle. Keep the bagPockets assertion.
- tests/test_bulk_item_plan.cpp:124-125 bagPockets excludes PCItems. Keep unchanged.
- tests/test_bag_top_up_real_saves.cpp:44-49 everything() uses allPockets. Update to Scope::AllBag; optionally add a BagAndPc pass.

**New tests:**
- Model: the Scope cycle offers PcItems/BagAndPc only when the state contains Pocket::PCItems (Gen I/II/III fixtures), never for Gen IV/V states. Default is still ThisPocket.
- Gen I fixture, PcItems scope: adds go only to PCItems, up to 50 minus held stacks. Key items in kG1PC (e.g. 5, 6, 31) and HMs 196-200 are skipped as KeyOrStoryItem. The 20-slot Bag bytes are unchanged. Apply, finalize, reparse and read back.
- Gen I fixture, BagAndPc scope: the Bag group still takes at most its free slots and the PC its own 50. No item ends up in both lists through a single add, and exclusivity counts both.
- Gen II fixture, PcItems scope: per-slot top-up of a duplicated PC stack (pair list), capacity from inventoryCapacity(PCItems). Crystal vs GS PC candidate lists.
- Gen III RSE (50) and FRLG (30) PC scope: capacity, 999 cap, FRLG PC limited to General items (ClassicInventoryCatalog.cpp:172).
- Gen I skip text: PocketFull on a slotGroup pocket reports the shared-Bag reason.
- Real-save harness: dispatch English Gen I/II .sav names (Red/Blue/Yellow/Gold/Silver/Crystal) instead of NO FIXTURE, using AllBag and PcItems scopes.

**Skeptic corrections.**

I checked the cited code and the root cause holds. The three causes (a), (b) and (c) are correct. There is no crash and no write bug. Some details need correcting, and some things were missed.

Confirmed, with exact lines (paths relative to wmain/):
- include/UI/BagTopUpModel.h:50 sets `bool allPockets = false`. Line :121 toggles it. Line :140 is `r.pockets = s.allPockets ? bag : {current}`. Lines :59-66 keep the choice for the session.
- include/Inventory/BulkItemPlan.h:104-110 is bagPockets(), which drops PCItems.
- The three surfaces pass bagPockets(...) at src/UI/ClassicInventoryOverlay.cpp:695-696, src/UI/Gen4SharedPokemonSurface.inc:1771-1772 and src/UI/Gen5SharedReadOnlySurface.inc:1016-1017.
- The Gen I adapter is in include/Inventory/BulkItemAdaptersClassic.h:48-76. Every Bag view has slotGroup 0 and slotCapacity = bagHeld + bagFree. PC is its own list (:63-64).
- The group free count is at BulkItemPlan.h:126-135 (cited as 124-133). Adds go in ascending ID order at :164-187, and PocketFull is raised at :182.
- Gen I pocket order is at src/Inventory/ClassicInventoryCatalog.cpp:31-34. kG1Items has 19 entries (:46-48).
- The Gen I test that pins this is tests/test_bag_top_up_adapters.cpp:153, `plan.adds.size()==2`.
- The PC Items tab with "This pocket" works today. ZR is not gated by tab (ClassicInventoryOverlay.cpp:1010-1013), and request() then returns {PCItems}.

Corrections:
1. Gen II and III are only partly "correct today". Their Items pockets have the same kind of native slot limit as Gen I, just less severe:
   - Gen II Items: pairCapacity 20 (src/Integration/Gen2/Gen2UniversalInventory.cpp:75) against 131 kG2Items candidates.
   - Gen III Items: 20 slots in RS, 30 in Emerald, 42 in FRLG (src/Integration/Gen3/Gen3StagedInventoryEditor.cpp:29,38,47) against 138 kG3Items candidates.
   - So with "All Bag pockets" and Add Missing, the Items pocket fills in ascending ID order and most items show "pocket full". If the owner re-tests H04 with Scope set to All Bag, they will still not see "every item". The re-test request must say so, or H04 will be re-failed.
   - The PocketFull text fix should therefore cover per-pocket capacity as well ("Items pocket full: N slots"), not only Gen I's slotGroup.
2. Gen I Bag items can be invisible, and the investigation missed this. src/Integration/Gen1/Gen1StagedInventoryEditor.cpp:79-82 filters each Bag view by isAddableItem for that pocket. freeSlots (:86-92) counts the whole raw bag.
   - Max Elixer (id 83, ItemNames.cpp:2803) is in kG1PC (ClassicInventoryCatalog.cpp:64-70) but in no Bag list. kG1Medicines (:52-54) stops at 82.
   - So a Max Elixer in a Gen I Bag shows on no tab and is never topped up, yet still uses one of the 20 shared slots. The same applies to any other held id that is in no view list.
   - This is a separate small catalog bug: add 83 to kG1Medicines. It adds to the "Gen I doesn't fill" impression.
3. Line citations that are off:
   - The FRLG PC list being "General items only" is the frlg lambda at ClassicInventoryCatalog.cpp:193-197 (comment at :172). It is not the code at :172.
   - The Key/HM unique rule is at src/Inventory/ItemQuantityPolicy.cpp:307-313, plus itemKind at :225-230 for Gen I.
4. One proposed new test is wrong as worded: "No item ends up in both lists through a single add" (Gen I BagAndPc).
   - For ordinary items it is native and legal to hold the same item in both the Bag and the PC, e.g. Potion in each.
   - The planner's `held` check is per pocket (BulkItemPlan.h:169-170). heldAnywhere is used only for exclusivity (:177-181), and the Gen1/2/3 adapters' add() checks only the target list (BulkItemAdaptersClassic.h:90,171,251).
   - So BagAndPc will add Potion to both, which is correct. Pinning the opposite would be wrong. The test should check that each list stays within its own capacity, that no story item is added, and that a Bag add and a PC add of the same ordinary ID are both allowed.
5. Discoverability is understated. The Scope row is never mentioned in Help (include/UI/InventoryHelp.h:38) or the footer (include/UI/InventoryUIContract.h:57). Only the generic "Left/Right Change" nav bar shows it (SharedBagTopUp.h:113). This supports adding a hint.

Everything else checks out:
- Japanese gate: ClassicInventoryOverlay.cpp:650-668 and :675-677.
- The real-save harness skips Gen I/II at tests/test_bag_top_up_real_saves.cpp:139-158.
- allPockets is used only in BagTopUpModel.h, SharedBagTopUp.h:125 and the tests listed, so the files_to_change list is complete.

**Missed generations or paths.**

Missed:
- Gen II Items (20 slots) and Gen III Items (RS 20, Emerald 30, FRLG 42) are also capacity-limited in Add Missing with All Bag scope. Most candidates are reported as PocketFull, so these generations are not "fully correct" as the owner means it.
- Gen I catalog gap: Max Elixer (83) is in kG1PC but in no Bag view list. A held Max Elixer is filtered out of every Bag tab (Gen1StagedInventoryEditor.cpp:79-82) but still counts against the 20 shared slots (:86-92).
- Gen IV and V call sites mirror the classic overlay and need only the signature change. There is no PC state there, so the Scope cycle must hide the PC choices. The investigator covered this correctly.
- The preview line at SharedBagTopUp.h:125 is the only non-model reader of allPockets.

Risks in the proposal:
- The BagAndPc "never in both lists" test would pin non-native behaviour.
- The existing planIsSafe check at test_bag_top_up_adapters.cpp:75 (adds never target PCItems) must become scope-aware, as they said.
- Keeping ThisPocket as the default protects PASS flows.
- Save safety does not change. The apply path and per-generation staged writers are reused; apply is all-or-nothing, and Gen I stageSetQuantity reads back with a round-trip check at Gen1StagedInventoryEditor.cpp:170-183.

</details>

### H22 — Gen II calculated Special; routine-edit toasts across gens (P1)

- **Classification:** 2 Cross-generation UI inconsistency, 3 Newly requested behavior  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Part A (Calculated Special number): only Gen II is affected. In Gen I the Special row's Calculated cell shows a number (stats[4]), and Gen I also has the lower "CALCULATED SPECIAL STATS" block. Gen III-V have six real stats and no Special row, so they are not affected.
Part B (pop-ups after routine edits): rerolling posts a success toast in every generation. Gen I: Gen1PokemonEditorOverlayFoundation.inc:213 (Create) and :218 through ux2CommitEdit (Edit). Gen II: Gen2UnifiedPokemonWorkspace.inc:811-815. Gen III: Gen3SharedPokemonSurface.inc:1615. Gen IV and V: SharedPokemonEditorSurface.inc:782. Changing the Unown letter posts a toast only in Gen II (Gen2UnifiedPokemonWorkspace.inc:374, plus "Unown unchanged" at :390). Gen III Unown is already silent (Gen3SharedPokemonSurface.inc:819-821, setUnownLetter with no postStatus). The Gen IV and V Form picker posts on refusal only (Gen4SharedPokemonSurface.inc:700, Gen5SharedReadOnlySurface.inc:515). Gen V Language is already silent (CG79, 64fe8f9).
Related, not named by the owner: every Gen I field edit posts a success toast through ux2CommitEdit (Gen1PokemonEditorOverlayUXCleanup2.inc:208; callers in Foundation.inc:237-336, Cleanup2.inc:286-557, Cleanup3.inc:133-372).

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

PART A - Gen II Calculated column, Special row
- include/UI/Gen2WorkspacePresentation.h:31-36: kSpecialRow = 4, and calculatedCell() returns a number only when row < kSpecialRow. Row 4 (Special) returns "". The comment at :24-30 records this as a deliberate Round 7 choice (owner Round 6 A22 / C01).
- src/UI/Gen2UnifiedPokemonWorkspace.inc:922-928: drawGen2CalculatedColumn loops `for (int row = 0; row < W::kSpecialRow; ++row)`, so it never paints row 4.
- The same column painter is called from the unified workspace (:971) and from the final hardware repaint (src/UI/Gen2HardwareFinalFix.inc:202).
- SpA and SpD appear only in the lower block, drawGen2SpecialBlock (:933-942, "CALCULATED SPECIAL STATS").
- The legacy Foundation painter (src/UI/Gen2PokemonEditorFoundation.inc:877) also uses calculatedCell.
- The View modal keeps its own copy of the rule: src/UI/Modals/Gen2PokemonDetailsModal.cpp:218-222 `if (i < 4)` drawText(battleStats[i]), so its Special Stat cell is empty too.
- Gen I reference: src/UI/Gen1PokemonEditorFoundationHardwareFix.inc:262-269 paints stats[r] for every row, Special included, and also has the lower block (:281-287).
- Result: the owner sees an empty Calculated cell for Special in Gen II but a number in Gen I.

PART B - toasts after routine edits. postStatus (include/UI/TrainerViewScreenBase.h:242) sets storageStatus. drawTransientStatus paints it last, as the bottom-centre pop-up (test_status_toast_contract.py).
- Gen II Unown letter applied: src/UI/Gen2UnifiedPokemonWorkspace.inc:370-376, applyUnownDVs posts "Unown X, shiny/not shiny: <DV changes> (stats follow)" for 360 frames. Reached from the picker Apply path (handleUnownPicker, about line 421) and from question A (:384).
- Gen II Unown question cancelled with B: :390 posts "Unown unchanged".
- Gen II Y reroll success: :811-815 posts rerollStatus(...) for Unown or "Generation II DVs re-rolled in this draft; Keep/Add stages them".
- Gen II no-change cases: :816-821.
- Gen II legacy Foundation reroll toasts at Gen2PokemonEditorFoundation.inc:786-789 are not dispatched; the CG85 note says so.
- Gen I Y reroll: Gen1PokemonEditorOverlayFoundation.inc:213 posts "Generation I DVs re-rolled in this draft; Add stages them" (Create). In Edit, :218 calls ux2CommitEdit, which posts "Randomized DVs set in this edit draft; X Keep stages it" (Cleanup2.inc:208). No-change: :206.
- Gen III Y reroll: Gen3SharedPokemonSurface.inc:1615 posts SharedStatReroll::kRerollStatus ("IVs and EVs re-rolled; Shiny, Gender, Nature and Ability kept", include/UI/SharedStatReroll.h:91).
- Gen IV and V Y reroll: src/UI/SharedPokemonEditorSurface.inc:782 posts the same kRerollStatus.
- Earlier precedent: 64fe8f9 (CG79) replaced `if (stage(...)) screen.postStatus("Language: ...")` with a bare stage() call in Gen5SharedReadOnlySurface.inc editDetail Language. The comment there reads: "The changed value is the feedback: no status toast".

**Root cause.**

Neither part is a coding slip. Both are deliberate earlier decisions that the owner has now reversed.

(A) After Round 6 A22 / C01 ("do not squeeze them into the numeric STATS column"), Round 7 (0e58157b) made the Gen II Special row's Calculated cell intentionally empty:
- Gen2WorkspacePresentation.h:33-36 returns "" for row 4.
- Gen2UnifiedPokemonWorkspace.inc:925 stops the loop at kSpecialRow.
- Gen2PokemonDetailsModal.cpp:220 has `if (i < 4)`.
Gen I, the stated reference, actually shows a number in that cell AND the lower block (Gen1PokemonEditorFoundationHardwareFix.inc:265-269, 281-287). So Gen II copied only half of the reference.

(B) The editors have no shared rule for whether a successful in-place edit posts feedback. Each surface calls postStatus on success by hand:
- Gen II Unown: Gen2UnifiedPokemonWorkspace.inc:374.
- Gen II reroll: :812.
- Gen I reroll: Gen1PokemonEditorOverlayFoundation.inc:213/218.
- Gen I ux2CommitEdit: Cleanup2.inc:208, on every success.
- Gen III reroll: Gen3SharedPokemonSurface.inc:1615.
- Gen IV and V reroll: SharedPokemonEditorSurface.inc:782.
The Gen II Unown and reroll toasts were added on purpose for Round 6 B07 / C09 ("Reroll seems not to work"; "choosing a letter does not explain whether the result should be shiny"), and CG85 enforces them. CG79 fixed only the one Gen V Language case and did not set a general rule.

**Shared owner.**

(A) PokeBank::UIModel::Gen2Workspace::calculatedCell in include/UI/Gen2WorkspacePresentation.h. Every Gen II STATS painter should get its Special Calculated value from it: drawGen2CalculatedColumn (unified workspace and HardwareFinalFix), the legacy Foundation painter, and Gen2PokemonDetailsModal, which currently duplicates the rule with `if (i < 4)`.

(B) A small shared feedback policy header, for example include/UI/EditFeedback.h, used by every generation's editor surface. Today SharedStatReroll::kRerollStatus is shared by Gen III-V, and Gen2UnownLetter::rerollStatus is used by Gen II. Rule: a routine successful in-place edit, where the changed value is visible on screen, posts no toast. Refusals, no-change outcomes, warnings, derived/locked-field explanations, Keep/Review/Export/Discard commits and errors still post. The single gate for the reroll cases is the Y branch in each surface. Better still, the shared helper returns an optional status (std::nullopt on success), so the decision lives in one tested place.

**Proposed fix.**

(A) Minimal fix, matching Gen I:
1. Change calculatedCell so row kSpecialRow returns std::to_string(battle[4]). Rows 0-3 are unchanged; row 5 and out-of-range rows return "".
2. In drawGen2CalculatedColumn (Gen2UnifiedPokemonWorkspace.inc:925), loop `row <= W::kSpecialRow`. Do NOT use kSpecialDefenseRow, which CG84 forbids.
3. In Gen2PokemonDetailsModal.cpp:218-222, replace `if (i < 4)` with Gen2Workspace::calculatedCell(battleStats, i).
4. Keep drawGen2SpecialBlock (the SpA and SpD labels and note) unchanged, as in Gen I.
5. Keep the cell as a plain number: no slash and no "Sp." tag, so CG51 and the CG84 calculatedCell regex still pass.
Which number goes in the Special row is an owner decision; see open questions. The recommended default is Sp. Atk (battle[4]), with Sp. Atk and Sp. Def both labelled in the block below.

(B) Shared rule: routine successful edits do not toast; warnings, errors, refusals and no-change outcomes still do.
- Gen III, IV and V: remove the success postStatus at Gen3SharedPokemonSurface.inc:1615 and SharedPokemonEditorSurface.inc:782. Retire kRerollStatus, or keep it for Help text only.
- Gen II Y reroll: post nothing when randomizeDVs succeeds (:811-815). Keep the no-change messages (:816-821), because without them a reroll that cannot change anything looks broken (B07).
- Gen II Unown: in applyUnownDVs (:374), drop the toast. The Letter row, sprite, DV column and Calculated stats all update. Shiny state is already stated in the picker title (pickerTitle) and in the confirm question.
- Gen II question cancel: consider dropping "Unown unchanged" (:390), since a cancel is also routine. This needs CG85 and the round5 mutation updated.
- Gen I reroll: drop :213. For Edit, call ux2CommitEdit with a "silent" option (for example nullptr meaning no toast, or a bool) at :218. Keep the :206 no-change and rejection toasts.
- Optional, if the owner agrees to "all gens" in the wider sense: silence ux2CommitEdit's success toast (Cleanup2.inc:208) for in-place field edits, matching Gen II-V. Gen II-V stage silently, apart from the Gen IV keepWorkspace commit message.
- Implement this through one helper, for example EditFeedback::successStatus(kind) returning std::nullopt for routine kinds, so each generation inherits the rule. Keep the X Keep, Review, Export and Discard confirmations; those are commits, not routine edits.
- No staging, encryption, checksum or save-write path changes.

**Risks.**

Save safety: none. All changes are presentation or feedback only. calculatedCell is display text; no DV, Stat Exp, staging, checksum or write path changes. CG51's 'no sixth DV / Stat Exp' checks remain.

Regression against earlier owner decisions:
- Round 3 rejected a cramped "10/9" in the column. Round 6 A22 said "do not squeeze them into the numeric STATS column". Adding a single number to the Special row partly reverses A22. It matches Gen I exactly (number plus block), but the owner should confirm that one number (not two) is what they want.
- Removing the Gen II Unown and reroll success toasts reverses part of Round 6 B07 / C09, which complained that reroll "seems not to work" and that choosing a letter did not explain shiny. Mitigations: the DV / Stat Exp / Calculated values change visibly; the no-change explanations are kept; pickerTitle and the question dialog still state shiny.
- Do not remove toasts for refusals, read-only/derived-field hints (for example "HP DV is derived..."), Keep/Review/Export/Discard, or PID-change notices (Gen V :903). The owner did not ask for those, and B08 (Export does nothing) depended on visible results.
- PASS items to watch: R2-SPECIAL layout (lower block position, CG84 geometry static_asserts in test_gen2_special_stat.cpp:96-103); Gen V Language (CG79).

**Open questions.**

1. Which number should the Gen II Special row's Calculated cell show? Options: (a) Sp. Atk (battle[4]), recommended, with both values labelled in the lower block as Gen I does; (b) both SpA and SpD on one row, which the owner rejected in Round 3 as "10/9" and which CG51 forbids; (c) a sixth display-only Sp. Def grid row, which is the Round 5 7A layout rejected in Round 6 A22. Owner to confirm.
2. Does "for all gens" also cover the general Gen I per-field success toasts from ux2CommitEdit (Cleanup2.inc:208), for example "DV change set in this edit draft; X Keep stages it"? The owner named only Unown and reroll.
3. Should cancelling the Unown confirmation with B ("Unown unchanged") also be silent?
4. Hardware confirmation is needed that the bottom-centre pop-up is gone in Gen I-V after Y reroll and after a Unown letter change, and that the Special number reads correctly at 1280x720 in Edit, Create and View.

**Files to change:** `include/UI/Gen2WorkspacePresentation.h`, `src/UI/Gen2UnifiedPokemonWorkspace.inc`, `src/UI/Modals/Gen2PokemonDetailsModal.cpp`, `src/UI/Gen2PokemonEditorFoundation.inc (comment only; it already uses calculatedCell at :877)`, `src/UI/Gen1PokemonEditorOverlayFoundation.inc`, `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc (ux2CommitEdit silent option)`, `src/UI/Gen3SharedPokemonSurface.inc`, `src/UI/SharedPokemonEditorSurface.inc`, `include/UI/SharedStatReroll.h (kRerollStatus)`, `include/UI/Gen2UnownLetter.h (rerollStatus: keep no-change text; success text unused)`, `new: include/UI/EditFeedback.h (shared routine-edit feedback policy)`

**Existing tests affected:**
- tests/test_gen2_special_stat.cpp:56-60 asserts calculatedCell(battle, kSpecialRow).empty(); flip it to == to_string(battle[4]) and keep row 5 empty.
- tests/test_gen2_special_stat.cpp:82-83 pins the `row < W::kSpecialRow` loop text; update it to the new bound.
- tests/test_gen2_special_stat.cpp: the modal section after :103 (the 'Special row Stat cell empty' comment and asserts) needs updating.
- tests/test_qa_matrix_gen12.cpp:835-843 c.expect(calculatedCell(battle, kSpecialRow).empty()); flip it.
- tests/test_gsc_editor_surface_contract.cpp:202 pins the exact `if (row >= 0 && row < kSpecialRow) return ...` line, and nearby comments describe the empty Special cell; update it.
- tests/test_qa_cross_gen_contract.py CG84 (:3530-3548) still passes if the cell is a plain number and the loop does not use kSpecialDefenseRow. Its docstring and comments say 'not the numeric column' and should be reworded to 'number plus block, as in Gen I'.
- tests/test_qa_cross_gen_contract.py CG51 (:441-470): unaffected if there is no slash, there are no Sp. labels in the column, and nothing indexes dvs[5].
- tests/test_qa_cross_gen_contract.py CG85 (:3557-3620) requires applyUnownDVs to mention shiny + dvChanges, the B-cancel branch to postStatus, and the Y branch to call rerollStatus twice or dvChanges. All need deliberate rewording to: success silent, no-change still explained.
- tests/test_qa_round5_contract_mutations.py:95-98: the mutations 'B on the Unown question is silent' and 'Unown reroll no longer names the changed DVs' would no longer be violations; remove or invert them.
- tests/test_gen2_unown_letter_selector.cpp:187 requires 'Gen2UnownLetter::rerollStatus(' in the workspace. Still satisfied if the no-change path keeps it. :242 unit-tests the rerollStatus text, which is unaffected if the function stays.
- tests/test_gen12_dv_reroll.cpp:121 pins `if (down & HidNpadButton_Y) {\n        if (randomizeDVs(state))` and :127 pins the Gen I route text; keep these structures or update them.
- tests/test_ui_editor_input_host.cpp gen5LanguageNoToast (:863-919) is the pattern to copy; it stays unchanged.

**New tests:**
- Host-UI (test_ui_editor_input_host style): Gen II unified editor, Special row Calculated cell drawn at StatsGrid::kCalculatedX / rowY(4) equals battle[4]; the block still shows Sp. Atk battle[4] and Sp. Def battle[5]. Run in Edit, Create and the View modal.
- Unit: calculatedCell(battle, 4) == to_string(battle[4]); calculatedCell(battle, 5) and calculatedCell(battle, -1) are empty; no '/' and no 'Sp.' in any cell.
- Cross-gen contract (new CG): no Gen I-V editor Y-reroll success branch calls postStatus. Gen1 foundationRandomizeDVs success, Gen2 Y branch after randomizeDVs true, Gen3 :1615 and SharedPokemonEditorSurface :782. No-change branches must still postStatus.
- Host-UI, all five generations: press Y in Create and in Edit; the IVs/EVs or DVs change and storageStatus stays empty. Force a no-change roll (Gen I/II shiny or gender lock) and storageStatus is non-empty.
- Host-UI Gen II Unown: choose a letter from the grid (direct Apply and question-A path); the letter, sprite and DVs change and storageStatus stays empty. Shiny state is still visible in pickerTitle and the question text.
- Optional Gen I: an in-place field edit through ux2CommitEdit in Edit posts no success toast, while a rejected edit still posts its error.
- Mutation entries in test_qa_round5_contract_mutations.py: reintroducing a success toast on reroll or on a Unown letter apply must fail the new CG.

**Skeptic corrections.**

I checked every cited location in wmain (HEAD 67b1b6a0). All of them match.

Part A
- include/UI/Gen2WorkspacePresentation.h:31-36 returns "" for the Special row (row 4).
- src/UI/Gen2UnifiedPokemonWorkspace.inc:925 loops only while `row < W::kSpecialRow`, so row 4 is never painted.
- The same painter, drawGen2CalculatedColumn, is called at :971 and at src/UI/Gen2HardwareFinalFix.inc:202. Fixing it once fixes both.
- src/UI/Modals/Gen2PokemonDetailsModal.cpp:220 has its own `if (i < 4)` rule.
- No other painter draws the Gen II Calculated column. I grepped for kCalculatedX, x+292 and x+286. Gen2SharedPokemonSurface.inc:475 also draws at x+292, but it writes compatibility text, not a stat.
So "deliberate Round 7 choice, half-copied from Gen I" holds.

Part B
- Confirmed toasts at Gen2UnifiedPokemonWorkspace.inc:374, :390 and :811-821.
- Confirmed Gen1PokemonEditorOverlayFoundation.inc:213 and :218, dispatched from :402.
- Confirmed Gen1PokemonEditorOverlayUXCleanup2.inc:208.
- Confirmed Gen3SharedPokemonSurface.inc:1615 and SharedPokemonEditorSurface.inc:782. The Gen IV and V setIv/setEv adapters (Gen4SharedPokemonSurface.inc:1398-1402, Gen5SharedReadOnlySurface.inc:528-532) call stage() without posting, so :782 is the only toast for Gen IV and V.
- The other Y handlers are not rerolls: Gen4 :1219 is discard-all, Gen1 UXCleanup2 :1268 and UX :1176 are Remove, and Gen2SharedSurfaceParity :248 opens review.

What they got wrong or left out

1. A missed Gen II Unown toast path: the Shiny row. Toggling Shiny on Unown changes or keeps the letter, which is the same "switch unown" action the owner described.
- Gen2UnifiedPokemonWorkspace.inc:666 calls toggleShiny.
- That reaches Gen2PokemonEditorFoundation.inc:410-427 toggleUnownShiny, which posts "Shiny off: Unown X kept; DVs ..." (:418), "Only Unown I, V can be shiny: letter A -> B ..." (:421) and "Shiny on: Unown X kept ..." (:425).
- When the Shiny question is shown and answered with A (requestShinyToggle :659-664 then handleUnownQuestion :386), the change goes through applyUnownDVs and toasts at :374. They mention applyUnownDVs, but not that the Shiny row reaches it too.
- Non-Unown Gen II Shiny toggles also toast, e.g. "Shiny off: DVs ... (stats and Hidden Power follow)" at Gen2PokemonEditorFoundation.inc:442-443. This is the same routine-edit pattern.
- These toasts are live, not legacy: the unified workspace calls toggleShiny directly.

2. A missed existing test that pins the reroll success toast at runtime. tests/test_ui_unown_letter_host.cpp:139 checks drew("DVs re-rolled, Unown <L>, shiny kept: ") and :158 checks drew("DVs re-rolled, Unown K, not shiny kept: "). Silencing the Gen II reroll toast fails this host-UI test. It must be changed to assert that nothing is drawn while the letter, shiny state and DVs still change.

3. tests/test_gen12_review_parity.cpp:182 pins the "Shiny off: DVs " + describeDVChanges text. It is affected only if the Shiny-toggle toasts are also silenced.

4. A correction to their test list. CG85 (tests/test_qa_cross_gen_contract.py) does three things:
- requires applyUnownDVs to contain "shiny" and "dvChanges" (:3604-3606);
- requires the B branch of handleUnownQuestion to call postStatus (:3598);
- requires the Y branch to contain at least 2 rerollStatus( calls or dvChanges (:3607-3612).
If the success call at :813 is removed and only the no-change rerollStatus at :817 remains, the count is 1, so the Y check fails unless CG85 is reworded. They noted CG85 needs rewording, but not this specific count.

5. Their fix for the Gen II reroll keeps the no-change toasts. That is reasonable, but the owner said "i dont need it" for rerolls in general. Whether "No other shiny DVs keep this Pokemon's gender; DVs unchanged" stays should go to the owner as a question. The proposal presents keeping it as settled.

6. Smaller points:
- The Special block (drawGen2SpecialBlock) already shows battle[4] next to "Sp. Atk". Putting battle[4] in the grid's Special cell repeats that value, and Gen I has no SpA/SpD split, so "matching Gen I" is only approximate. The open question they raised (which number goes in the cell) is the right one.
- Changing the loop to `row <= W::kSpecialRow` paints the cell with gen2StatCellColor(4,2), the same colour the block uses. That is fine.
- tests/test_gen2_special_stat.cpp:109 pins the exact modal text "if (i < 4)\n            fb.drawText(midX + 290, y, std::to_string(battleStats". The assert is at :109, not "after :103".

Save safety: confirmed none. Every change is to toast/feedback text or to calculatedCell display. randomizeDVs, applyEditableDVs, stage(), ux2CommitEdit staging and the checksum/write paths are untouched.

PASS risks:
- CG84 geometry static_asserts (test_gen2_special_stat.cpp:96-103) do not change.
- The Gen V Language no-toast check (test_ui_editor_input_host.cpp:906-918) is unaffected.
- Keep CG51: no '/' and no "Sp." text in the column.

**Missed generations or paths.**

Missed paths (all Gen II, live unified workspace):
(a) Shiny row on Unown. Gen2UnifiedPokemonWorkspace.inc:666 calls toggleShiny, which reaches Gen2PokemonEditorFoundation.inc:410-427 toggleUnownShiny. It toasts at :418, :421 and :425 on every Shiny toggle that keeps or changes the Unown letter.
(b) Shiny question confirmed with A. Gen2UnifiedPokemonWorkspace.inc:659-664 opens the question, then :386 applies it through applyUnownDVs, which toasts at :374. This is a second way into the Unown letter toast besides the picker.
(c) Non-Unown Gen II Shiny toggle. Gen2PokemonEditorFoundation.inc:430-445 toasts "Shiny off/on: DVs ...". This is the same routine-edit pattern, not named by the owner.

Missed tests:
- tests/test_ui_unown_letter_host.cpp:139 and :158 assert that the Gen II reroll success toast is drawn.
- tests/test_gen12_review_parity.cpp:182 pins the Shiny-off text.
- CG85's Y-branch check (tests/test_qa_cross_gen_contract.py:3607-3612) requires at least 2 rerollStatus( calls, so it fails once the success call is dropped.

No additional generations are affected.
- Gen I: the reroll and the per-edit ux2CommitEdit toasts are already in their report.
- Gen III: the reroll at :1615 is the only routine success toast they cite; Unown there is already silent.
- Gen IV and V: :782 is the only reroll toast, and setIv/setEv do not post. Their Y handlers at Gen4 :1219 and in Gen5 are discard/review actions, not rerolls.

Part A affects only Gen II. drawGen2CalculatedColumn covers both the unified workspace and HardwareFinalFix, plus the View modal at Gen2PokemonDetailsModal.cpp:220. The legacy Foundation painter at :877 inherits the fix through calculatedCell.

</details>

### H29 — Gen V Shiny inline like Gen I-IV (P1)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Only Gen V is affected. Gen I and Gen II toggle Shiny in place by rewriting DVs, with a status line (Gen2PokemonEditorFoundation.inc:430-465, and the editor dispatch at line 572). Gen III toggles in place (Gen3SharedPokemonSurface.inc:1008-1011, `state.session.setShiny(!...)`). Gen IV toggles in place (Gen4SharedPokemonSurface.inc:936-940, `state.session.cycleShiny()` in include/UI/Gen4SharedPokemonSession.h:76-78). Gen V Gender is already inline (H08 PASS, commit edb7bda1).

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

In Gen V, pressing A or tapping the Shiny row (PID panel row 8, `Fields::ShinyRow` in include/Integration/Gen5/Gen5EditorFields.h:46) runs `Gen5Editor::editPidRow` in src/UI/Gen5SharedReadOnlySurface.inc:590-594:

    } else if (row == Fields::ShinyRow) {
        auto probe = d;
        if (!Fields::toggleShiny(probe, error)) screen.postStatus(error, 300);
        else offer(screen, state, std::move(probe),
                   std::string("Shiny ") + (p.shiny() ? "Yes -> No" : "No -> Yes"));

`offer()` (lines 321-339) applies the probe at once only when the PID did not change. A shiny toggle always changes the PID, so `offer()` fills `PendingPidChange5` and returns. On the next frame, `handleSharedWorkspace` (line 940) hands input to `handlePidConfirmation` (lines 869-909), and `drawPidConfirmation` (lines 910-933) draws the modal "Change PID?" dialog with the PID warning. This is the pop-up and PID warning the owner saw.

Gender, on the row just above (lines 573-589, added in edb7bda1 "inline Pokemon Gender (#6)"), works differently. It probes `Fields::toggleGender`, re-checks the result with `pidLinksHold`, assigns the draft directly, and posts "Gender X -> Y | PID 0x.. -> 0x.. (nature, shininess and ability kept)". The edb7bda1 commit message says plainly that "Shiny, Ability and the other PID edits keep their confirmation". Shiny was left on the dialog path on purpose, not by oversight.

**Root cause.**

The Gen V adapter `Gen5Editor::editPidRow` (src/UI/Gen5SharedReadOnlySurface.inc:590-594) sends the Shiny toggle through `offer()` (lines 321-339). Because a shiny toggle always changes the PID, `offer()` always sets up the modal "Change PID?" confirmation (lines 869-933). The H08 Gender change converted only the Gender row to the direct, re-verified, inline-reported path (lines 573-589).

What toggling Shiny changes in Gen V:
- The draft stage `case Field::Shiny` (include/Integration/Gen5/Gen5StagedPokemonRecord.h:232-243) refuses earlier-generation origins. Otherwise it rewrites only the PID's high 16 bits through `withShiny5` (include/Integration/Gen5/Gen5PidCoordination.h:56-69).
- `withShiny5` keeps the low 16 bits, so the gender byte is kept. The `& 0xFFFE | (high & 1)` keeps bit 16, the normal ability slot. The static_asserts at lines 100-103 cover shiny round-trip, ability-bit keep and low-half keep.
- Nature is a stored byte (PK5 0x41, per docs/handoffs/GEN5_9_PROGRESS.md:91), so the PID rewrite does not affect it.

So the toggle is already safe without the dialog. The dialog is only UI policy.

**Shared owner.**

The editor dispatch itself is shared: `SharedPokemonEditorSurface.inc:637` calls `A::editPidRow`. The behaviour for each field belongs to each generation's adapter, and here that is `Gen5Editor::editPidRow` in src/UI/Gen5SharedReadOnlySurface.inc. Gen I-IV already toggle inline in their own adapters and sessions, so no change to the shared surface is needed.

To make one owner for the Gen V PID-row rule, the proposal adds a small Gen V helper that both Gender and Shiny use: probe, then `pidLinksHold`, then assign, then post an inline PID note. Call it `applyPidToggleInline(screen, d, probe, label)`, placed in the Gen5Editor struct. Shiny then inherits exactly the validation that Gender already passed on hardware (H08).

**Proposed fix.**

Minimal fix, all in src/UI/Gen5SharedReadOnlySurface.inc:

1. Lift the body of the Gender branch (lines 578-588) into a private static helper, for example `static void applyPidToggleInline(TrainerViewScreen&, Draft& d, Draft probe, const Record& before, const std::string& change, const char* kept)`. The helper must:
   - assign the probe only when `probe.current()` exists and `pidLinksHold(*after, b2w2(screen))` passes;
   - otherwise post "The new PID failed strict re-verification; nothing was changed";
   - on success, post "<change>  |  PID 0x.. -> 0x.. (<kept>)".
   Also require `after->shiny()` to have actually flipped for Shiny, and the gender for Gender. This is defence-in-depth, because `pidLinksHold` does not check shininess.

2. Replace the Shiny branch at lines 590-594 with: probe `Fields::toggleShiny(probe, error)`; on error post `error`; otherwise call the helper with change "Shiny Yes -> No" / "No -> Yes" and kept "nature, gender and ability kept". The probe is assigned directly. `offer()` is not called, so the dialog never opens.

3. Change the Gender branch to call the same helper. Its text stays byte-identical, so the existing H08 assertions still hold.

Leave everything else alone:
- `offer()` and the "Change PID?" dialog stay for Ability-slot (line 731), TID/SID (line 437) and Species-with-shiny-preview (lines 744-800). The owner did not ask for those to change, and the "Change PID?" strings stay pinned by tests/test_shared_editor_contract.cpp:409-413.
- `Fields::pidRowEditable` and `pidRowRestriction` (Gen5EditorFields.h:185-210) keep gating earlier-origin records and the view-only mode before the toggle runs (lines 571-574).

Optional: update the stale comment at lines 163-165 to say Gender and Shiny are now inline.

**Risks.**

Save safety is unchanged in substance:
- The same native `Field::Shiny` stage, the same earlier-origin refusal, and the same `pidLinksHold` strict re-verification that the dialog's confirm path used (line 896) all still run.
- Nothing is written to the save until Keep, Review and Export. The only behaviour removed is the extra confirmation click.

Remaining risks:
- A player may now change the PID with one press. This matches the H08 Gender decision the owner approved, and the change is stated inline.
- `pidLinksHold` does not check shininess, so the helper should assert that `after->shiny()` flipped.

Rows the owner marked PASS:
- H08 Gender inline. If a shared helper is introduced, its status text must stay byte-identical.
- Ability, TID/SID and Species dialogs are untouched.

Gen IV `cycleShiny` changes the PID silently, with no status note. That is a minor parity gap in the other direction and out of scope here.

**Open questions.**

1. Should the Species picker's Y shiny preview (lines 744-800) and the Ability-slot and TID/SID edits also drop the "Change PID?" dialog for parity? The owner named only the Shiny row, so this fix does not touch them.
2. Should Gen IV's `cycleShiny` and `cycleGender` gain the same inline "PID 0x.. -> 0x.." status note, so Gen IV and Gen V read identically?
3. Hardware confirmation needed: on Switch, the Shiny row flips in place, the sprite updates, and the status line fits at 1280 px with the longer "nature, gender and ability kept" note.

**Files to change:** `src/UI/Gen5SharedReadOnlySurface.inc`, `tests/test_ui_editor_input_host.cpp`, `tests/test_gen5_readonly_ui_contract.py`, `docs/hardware/RETEST_ROUND10_TEMPLATE.md (or next round's RETEST: Shiny expected inline)`

**Existing tests affected:**
- tests/test_gen5_readonly_ui_contract.py:1224-1225 requires the strings "Fields::toggleGender(probe, error)" and "Fields::toggleShiny(probe, error)". Both survive if the branches keep those calls. They break if the probe call moves into the helper. Either keep the calls at the call sites, or update the strings deliberately.
- tests/test_shared_editor_contract.cpp:409-413 pins "Change PID?", pidLinksHold and `if (before->pid() == after->pid()) {`. All remain because Ability, TID/SID and Species still use offer(), so no change is expected.
- tests/test_shared_editor_contract.cpp:264-271 and tests/test_qa_matrix_gen5.cpp:729,1255, tests/test_gen5_party_box_parity.cpp:380 exercise Fields::toggleShiny at the model level. They are unaffected because the model is unchanged.
- tests/test_qa_cross_gen_contract.py:863 regex `Field::Shiny\b|Fields::toggleShiny\s*\(` for Gen V still matches.
- tests/test_ui_editor_input_host.cpp:230-251 (H08 Gender inline) must keep passing unchanged. It checks that the status contains "Gender" and "PID".

**New tests:**
- tests/test_ui_editor_input_host.cpp: add a Gen V Create block that mirrors the Gender block (lines 230-251) for `kShinyPidRow = 8`. Tap `Workspace::kPidTouchBase + 8` and render one frame. Then check that: shiny flips; the PID changes; the PID low 16 bits are unchanged; gender, nature and ability are unchanged; no drawn text equals "Change PID?"; `screen.storageStatus` contains "Shiny" and "PID". Then press A and check that shiny and the original PID are restored (`withShiny5` is reversible for the off path only modulo bit 3). Assert only that shiny flips back and that gender, ability and nature are still kept. Do not assert an exact PID.
- The same host test on an Edit draft of an earlier-origin (Gen III/IV transferred) PK5: A on Shiny posts the `pidRowRestriction` text, and the draft and PID are unchanged.
- A negative/mutation check in test_gen5_readonly_ui_contract.py: the ShinyRow branch must not contain `offer(`, and it must call `pidLinksHold`.
- Update the hardware RETEST row: in Gen V, Shiny flips straight away with no "Change PID?" window, and the status line states the PID change.

**Skeptic corrections.**

The root cause is correct. I checked every Gen V citation at checkout 67b1b6a0:
- src/UI/Gen5SharedReadOnlySurface.inc:590-594: the ShinyRow branch probes `Fields::toggleShiny` and then calls `offer(...)`.
- `offer()` at 321-339: when `before->pid() != after->pid()` it fills `PendingPidChange5` and returns, so it never applies the change.
- handleSharedWorkspace (line 940) calls `handlePidConfirmation` (867-909) first, and `drawPidConfirmation` (910-933) draws the "Change PID?" modal.
- A Shiny toggle always changes the PID. Field::Shiny (include/Integration/Gen5/Gen5StagedPokemonRecord.h:232-243) calls `withShiny5` (Gen5PidCoordination.h:56-69), which rewrites only the high half and keeps bit 16. The static_asserts at 100-104 cover this.
- The inline Gender branch is at 573-589 and runs strict re-verification through `pidLinksHold` (310-318).
- `pidRowEditable` and `pidRowRestriction` (Gen5EditorFields.h:185-210) run before the branch, so earlier-origin and view-only records are already refused.
- The shared dispatch is at SharedPokemonEditorSurface.inc:637 (`return A::editPidRow(...)`).
- "Inline" in the owner's sense means an in-place cycle with no window. Gen V Language (Gen5SharedReadOnlySurface.inc:447-452) and Pokerus (481-486) stage directly, so a direct toggle is the right model.

Corrections and refinements:
1. Gen I is not in Gen2PokemonEditorFoundation.inc. The live Gen I Shiny toggle is src/UI/Gen1PokemonEditorOverlayFoundation.inc:340-342 and src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:359-360, which call `ux3SetDraftShiny` / `ux3SetExistingShiny`, defined at UXCleanup3.inc:99 and 113. Gen I is inline either way.
2. Gen II: `toggleShiny` (Gen2PokemonEditorFoundation.inc:430) is also called from src/UI/Gen2UnifiedPokemonWorkspace.inc:666 (`if (focus.row == 5) { toggleShiny(screen, p); return true; }`), which is the unified workspace path. Line 572 is the older foundation dispatch. Both are inline, so the conclusion holds, but 666 is the more relevant citation.
3. The comment at Gen5SharedReadOnlySurface.inc:163-165 is already stale today: it still lists Gender as dialog-gated even though Gender went inline in edb7bda1. The fix should rewrite it to name only Ability slot, TID/SID and Species-with-shiny.
4. The proposed defence-in-depth `after->shiny()` check duplicates what staging already guarantees. Field::Shiny either stages `withShiny5`, which the static_asserts prove is shiny or non-shiny as asked, or it fails. The check is harmless but redundant.
5. Toggling Shiny on and then off does not restore the original PID. The "on" path recomputes the high half from tid^sid^low, and the "off" path flips bit 3. This drift was already reachable through the dialog's confirm path, which assigns the probe the same way (line 896-897). Gender has the same precedent through `withGender5`. The new host test must not assert PID equality after the round trip, as the investigator already said.
6. Tests: the investigator's list is accurate.
   - tests/test_ui_editor_input_host.cpp:230-251 is the Gender inline block to mirror for row 8.
   - tests/test_gen5_readonly_ui_contract.py:1224-1225 requires the literal strings "Fields::toggleGender(probe, error)" and "Fields::toggleShiny(probe, error)". Keep those calls at the call sites rather than moving them into the helper.
   - tests/test_shared_editor_contract.cpp:409-413 pins "Change PID?", `Gen5Editor::pidLinksHold(*after, b2w2)`, the handlePidConfirmation call and `if (before->pid() == after->pid()) {`. All of these survive because Ability, TID/SID and Species still use `offer()`.
   - No existing test or doc pins the Shiny dialog itself. grep for "No -> Yes", "Yes -> No" and "Change PID" in tests/ and docs/hardware/ finds only Gender rows.
7. The RETEST docs (RETEST_ROUND10_TEMPLATE.md:107, RETEST_3634d1e6.md:111) have a Gender inline row but no Shiny row. A new Shiny row should be added for the next round.

**Missed generations or paths.**

Missed paths:
- Gen I's real inline path (Gen1PokemonEditorOverlayFoundation.inc:340-342 / UXCleanup3.inc:99-120,359-360) was attributed to the Gen II file.
- Gen II's unified-workspace call site (Gen2UnifiedPokemonWorkspace.inc:666) was not cited.

Neither changes the verdict: Gen I-IV are inline and only Gen V's ShinyRow uses the dialog.

Other Gen V shiny-related PID paths that still use the dialog, intentionally out of scope:
- Species picker with Y shiny preview / `rebuildCreate` (`handlePidConfirmation` 883-893).
- TID/SID through `stageTrainerId` and `keepShinyForIds5`.
- Ability slot.

The owner named only the middle-column Shiny row, so these stay as they are, but they are the remaining "Change PID?" surfaces if the owner later asks for full parity.

No shared-surface change is needed. SharedPokemonEditorSurface.inc:637 delegates to the adapter, and `viewOnlyActivate` (around line 641) already handles view mode.

Save safety:
- Nothing new is written until Keep, Review and Export.
- The inline assignment `d = std::move(probe)` is the same operation the dialog's confirm path performs (line 897), and `pidLinksHold` re-verification is retained.

PASS risk: the H08 Gender text and behaviour must stay byte-identical if a shared helper is extracted. The tests/test_ui_editor_input_host.cpp:230-251 assertions and the RETEST Gender rows depend on it.

</details>

### H23 — Move controls: Y pick up, A place (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Gen I, II, III and IV: all use the shared ClassicPackedMove carry. A Y tap picks up, Y places, B cancels, and A or X is refused while carrying. Gen IV's Storage save pane uses the same carry. Gen V uses the same carry (CarryGeneration::Gen5), but it can only start from Actions -> Move, because Y is Review on the Gen V Boxes page. The Gen V carry also places on Y and refuses A. No Gen I-V Boxes carry places on A today. The only place A already places is the base Storage/Bank carry (TrainerViewScreenBase.inc:2586-2611, footer "A: Drop / Swap" / "A: Place Here" at :5383-5384), which every generation reaches through the Bank pane. So the classic box carry (Y places) and the Bank carry (A places) already disagree with each other.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

This is not a bug: the owner marked H23 PASS. The request is to change the key that places a carried Pokemon. One shared overlay owns the carry for all five generations: /tmp/claude-0/-home-user-PokeBank-NX/e3d26a85-4f48-5ea5-b234-ef4545c1ae50/scratchpad/wmain/src/UI/ClassicPackedMoveOverlay.inc.

How a carry starts:
- Gen I-IV: a Y tap picks up when Y is released. Holding Y for 8 frames (kMultiHoldFrames) starts a multi-select rectangle. See handleInput :1185-1219.
- Gen I-IV touch: dragging across the grid also starts a multi-select (:1069-1117).
- Actions -> Move: Gen I (ClassicReleaseActionFix.inc:50-56 and Gen1PokemonEditorOverlayUXCleanup2.inc:681-686), Gen IV (Gen4SharedPokemonSurface.inc:1057-1062) and Gen V (Gen5SharedReadOnlySurface.inc:1673-1678 -> beginGen5FromAction :1027).

While carrying in Gen I-IV (:1120-1138):
- B calls cancel().
- Y calls place().
- + cancels the carry and falls through.
- A or X is refused with the status "Place with Y or cancel with B first" (:1129-1131).
- Tapping a cell only moves the cursor (:1133-1136). In the Storage pane, storageCarryNavigation (:950-985) moves the cursor.

The Gen V carry, handleGen5CarryInput (:1039-1062), mirrors this: B cancels (:1046), Y places (:1047), and A or X is refused (:1052-1054).

Text that tells the player to use Y:
- Carry footer: {"Y","Drop"} or {"Y","Drop Group"}, then {"L/R","Box"} and {"B","Cancel"} (:1267-1268).
- Pickup status messages say "... Y places ..." at :308, :346, :422, :470, :495 and :636.
- Help, Pokemon page: "Y   While moving: place it here     B   While moving: cancel" (include/UI/ClassicHelpPresentation.h:40). Gen V reads this line too.
- Browse footer: "Y Move / Hold Multi" (:1282-1287). This stays.

place() and every place path (placeGen4, placeGen5, Gen I-III) always return true (:676-890), so the key never falls through to the base handlers.

Touch:
- Tapping the footer glyph sends its button through navTouchButton (TrainerViewScreenCompositeOverlay.cpp:377). Today, tapping "Y Drop" places the Pokemon.
- The Gen IV Storage tap-to-A path (TrainerViewScreenCompositeOverlay.cpp:397-436) is blocked during a carry, because setLegacyHoldingPresentation sets swapActive=true (:211-215) and that path requires !swapActive.

**Root cause.**

This is a design choice, not a defect. The shared classic carry hardcodes Y as the place key in two copies of the key dispatch:
- Gen I-IV: src/UI/ClassicPackedMoveOverlay.inc:1124 `if (down & HidNpadButton_Y) return place(screen, state);`
- Gen V: :1047, the same line.

Both copies refuse A at :1052-1054 and :1129-1131. The footer (:1267), Help (ClassicHelpPresentation.h:40) and six pickup status strings (:308, :346, :422, :470, :495, :636) all repeat "Y places".

Separately, the base Bank/Storage carry (TrainerViewScreenBase.inc:2380-2392 storagePickup, :2586-2611) already places with A. That is the cross-generation inconsistency: the same app uses A to place in one carry and Y to place in the other.

**Shared owner.**

src/UI/ClassicPackedMoveOverlay.inc owns every classic carry for Gen I-V: handleInput, handleGen5CarryInput and draw. Moving the key decision into a small pure model header (for example include/UI/ClassicCarryKeys.h, or an addition to include/UI/ClassicCarryGrid.h) would let both dispatch sites, the footer and a host test share one table. The Help text is owned by include/UI/ClassicHelpPresentation.h, and TrainerViewScreenBase.inc:5587-5591 is its only caller.

**Proposed fix.**

The fix is the same for all five generations because they share one overlay.

1. Add a pure carry-key model, for example `PokeBank::UIModel::ClassicCarryKeys::decide(down) -> {Cancel, Place, Leave, RefuseHint, None}`:
   - B -> Cancel (checked first, as today).
   - A -> Place.
   - + -> Leave (cancel, then fall through, as today).
   - Y or X -> RefuseHint.
   Use it in both handleInput :1120-1131 and handleGen5CarryInput :1046-1055, so Gen I-IV and Gen V cannot drift apart. Y while carrying should show "A places it here, B cancels" and do nothing else. Whether Y should instead stay as a silent alias is an owner question.
2. Footer, :1267: change {"Y","Drop"/"Drop Group"} to {"A","Place"/"Place Group"}, keeping {"L/R","Box"} and {"B","Cancel"}. Tapping the footer button then sends A through navTouchButton, so touch placement keeps working with no other change.
3. Status strings: :308 and :346 become "Pokemon picked up - A places it, B cancels". Update :422, :470, :495 and :636 the same way. :1130 and :1053 become "Place with A or cancel with B first" (or the Y hint above).
4. Help, ClassicHelpPresentation.h:40: replace the line with "A   While moving: place it here     B   While moving: cancel". Gen I-IV Help never names Y as the pickup key today, so add "Y   Tap: pick up (Move); hold: multi-select" to the Gen I-IV Pokemon page only. Gen V's Y is Review, and its Help already says A -> Actions -> Move.
5. Optional touch improvement: tapping the cell the cursor is already on could place, as A does. Today a tap only moves the cursor (:1133-1136 and storageCarryNavigation :953-958). The Gen V tap at :1057-1059 would get the same treatment.
6. Update stale comments: ClassicPackedMoveOverlay.inc:48, :428 and :1039; ClassicPackedMovePresentation.h:14; ClassicReleaseActionFix.inc:51; Gen5SharedReadOnlySurface.inc:1589-1590, :1676 and :2299; BoxPokemonPanel.cpp:116 and :154; StoragePanel.cpp:204; ClassicCarryGrid.h:7.
7. Leave these unchanged:
   - The Y-tap pickup and the hold-Y multi-select.
   - B cancel.
   - All place() and cancel() backends (staged, validated writes).
   - The base Storage/Bank carry, which already uses A.
   - The Gen VI+ base Boxes swap footer, "Y: Drop Here" at TrainerViewScreenBase.inc:5232, which is outside Gen I-V scope.

**Risks.**

Save safety: low. Only the key that calls the same validated place() and cancel() changes. place() returns true on every path, so an A press during a carry never reaches the action sheet or the base handlers.

Multi-select: hold-Y depends on Y being held and pickup fires on Y release. Freeing Y from placing removes the case where a Y meant to place a Pokemon is read as the start of a new pickup.

Same-frame A risk: Actions -> Move starts the carry on an A press. All overlay entry points run before the action-menu handlers (Gen V's carry check :1590-1595 runs only when gen5CarryActive is already true), and kDown is edge-triggered. So the A that starts a carry cannot also place it on the same frame. Confirm this on hardware for each Actions -> Move entry point: Gen I, Gen IV Boxes and Storage, and Gen V.

PASS-marked behaviour that must not regress:
- H23 itself (the carry works).
- The Gen IV Storage-pane carry (test_gen4_storage_carry).
- Gen II and III packed and sparse moves.
- Pickup with a Y tap, hold-Y multi-select, B cancel and touch drag multi-select.

Muscle memory: players used to pressing Y twice will now get the hint on the second Y instead of a placement. This is safer than the reverse, because a Y never commits anything.

**Open questions.**

1. While carrying, should Y show a hint and do nothing (recommended), or stay as a silent alias for A?
2. The owner asked for "Y pick up" in every generation, but Gen V Boxes uses Y for Review (ClassicHelpPresentation.h:39; Gen5SharedReadOnlySurface Boxes). Gen V can keep Actions -> Move as its only pickup, or Review can move to + so Y picks up as in Gen I-IV. Moving Review would undo the Round 7 one-Review-key decision, so the owner must choose.
3. Should tapping the already-focused cell during a carry place the Pokemon, as a second way besides the footer's A button?
4. The Bank/Storage pane picks up with X (Move mode) or a mode cycle, not Y (TrainerViewScreenBase.inc:2456-2584). Should the pickup key be unified too, or is this request only about the place key?
5. Hardware: confirm on device for each generation that A after Actions -> Move places on the next press, not on the same press.

**Files to change:** `src/UI/ClassicPackedMoveOverlay.inc`, `include/UI/ClassicHelpPresentation.h`, `include/UI/ClassicCarryKeys.h (new, or extend include/UI/ClassicCarryGrid.h)`, `include/UI/ClassicPackedMovePresentation.h (comment)`, `src/UI/ClassicReleaseActionFix.inc (comment)`, `src/UI/Gen5SharedReadOnlySurface.inc (comments only)`, `src/UI/Panels/BoxPokemonPanel.cpp (comments only)`, `src/UI/Panels/StoragePanel.cpp (comment only)`, `tests/test_classic_help_accuracy.cpp`, `tests/test_gen3_hardware_surface_contract.cpp`, `tests/test_gen4_hardware_surface_contract.cpp`

**Existing tests affected:**
- tests/test_classic_help_accuracy.cpp:108-111 asserts `if (down & HidNpadButton_Y) return place(screen, state);` (comment: "Moving a Pokemon: Y places, B cancels"). Change it to the A form or the shared decide() call, and tie Help line 40 to it.
- tests/test_gen3_hardware_surface_contract.cpp:316-317 asserts `if (down & HidNpadButton_Y) return place(screen, state)` and the presence of `down & (HidNpadButton_A | HidNpadButton_X)` (the A/X refusal). Update both deliberately.
- tests/test_gen4_hardware_surface_contract.cpp:226 asserts `if (down & HidNpadButton_Y) return place(screen, state)`. Update it. Line 225 (the hold-Y beginSelectedGroup) is unchanged.
- tests/test_gen3_hardware_surface_contract.cpp:301 asserts "Move / Hold Multi" and "Hold Multi". This browse footer is unchanged, so the assertion should keep passing.
- tests/test_gen4_storage_carry.cpp:155-161 asserts the !swapActive tap guard and the handleInput ordering. These are unchanged and should keep passing.
- tests/test_gen5_staged_slot_actions.cpp:309 asserts handleGen5CarryInput is wired. The wiring is unchanged and should keep passing.

**New tests:**
- Host unit test for the new ClassicCarryKeys::decide: A -> Place, B -> Cancel (B wins over A when both are pressed), Y -> RefuseHint (never Place), X -> RefuseHint, + -> Leave, D-pad and L/R -> None (they still fall through to grid navigation).
- Source contract: handleInput's state.active branch and handleGen5CarryInput both route through the same decide() call. Neither contains `HidNpadButton_Y) return place(`.
- Footer contract: the carry draw() nav bar contains {"A", "Place"} or {"A", "Place Group"}, contains no {"Y", "Drop"}, and still has {"B", "Cancel"}. Optionally render through BrowseFooter and assert the tappable glyph is A.
- Help accuracy: ClassicHelp::lines(Page::Pokemon, g) for g = 1..5 contains the A "While moving: place" row and no Y "place" row. For g = 1..4 it contains the Y pickup / hold multi-select row. For g = 5 it does not, because Y is Review there.
- Status-string guard: no "Y places" or "Place with Y" text remains anywhere in ClassicPackedMoveOverlay.inc.
- Optional cross-gen contract in tests/test_qa_cross_gen_contract.py: the Gen I-V carry place key equals the base Storage carry place key (A).

**Skeptic corrections.**

Every load-bearing citation checks out against the code.
- The Gen I-IV carry dispatch is at src/UI/ClassicPackedMoveOverlay.inc:1120-1138. B cancels (:1122), Y places (:1123-1124), + cancels and falls through, and A or X is refused with "Place with Y or cancel with B first" (:1129-1131).
- The Gen V copy is handleGen5CarryInput (:1042-1062), with Y placing at :1047 and A or X refused at :1052-1054.
- The carry footer is at :1267-1268. The six "Y places" status strings are at :308, :346, :422, :470, :495 and :636.
- Help line 40 is in include/UI/ClassicHelpPresentation.h.
- The base Bank carry places with A (TrainerViewScreenBase.inc:2585-2592), and its footer says "A: Drop / Swap" / "A: Place Here" (:5382-5384).

So the investigation is right: this is a design choice, not a defect, and the classification (3 + 2) stands.

Corrections and gaps:
1. Some test assertions that will break were not listed. tests/test_gen3_hardware_surface_contract.cpp:315 asserts the literal `if (down & HidNpadButton_B) return cancel(screen, state)`, and tests/test_classic_help_accuracy.cpp:111 asserts the same string with a `;`. The proposed decide() refactor would change the dispatch shape, so both fail along with the Y lines already listed (gen3:316-317, help:110, gen4:226). gen3:318 also pins the comment "D-pad and L/R keep using the accepted box navigation path", so that comment must survive. The comment at tests/test_gen4_hardware_surface_contract.cpp:220-221 ("atomic PK4 drop", Y path) is stale but harmless.
2. Y must stay consumed while carrying. This is a safety requirement, not a preference. In Gen V, Y outside a carry is Review. If handleGen5CarryInput returned false for Y, the call site at Gen5SharedReadOnlySurface.inc:1591-1595 would fall through and Gen V Review handling could see a Y during a live carry. In Gen I-IV, the state.active branch runs before the yPending pickup code (:1180-1215). A Y that falls through would reach Gen4SharedEditorSurface, Gen3SharedEditorSurface, Gen1 and Gen2 handlers (TrainerViewScreenCompositeOverlay.cpp:441-444). The RefuseHint must return true at both sites. Making Y a "silent alias" (open question 1) is safe; a fall-through is not.
3. The same-frame A risk is lower than stated, and the code ordering supports that:
   - The Gen V carry check (:1591-1595) runs before the Actions handler (:1596+). Move activation inside Actions starts the carry and returns, so the same kDown never reaches the carry branch.
   - For Gen I-IV, ClassicPackedMove::handleInput (Composite:439-440) runs before the editor or Actions handlers that start the carry, so a carry started by A is not active until the next frame.
   - Keep the hardware confirmation anyway.
4. Gen IV Storage tap-to-A: the synthesized `down |= HidNpadButton_A` (Composite:~431) is guarded by `!swapActive` (Composite:401). Today it is unreachable mid-carry. After the change it would be harmless anyway, because it would place.
5. The investigation called the Bank carry an inconsistency, but the docs already expected A. docs/hardware/RETEST_62a2f8c3.md:109 (R2-CHECK-009) tells the owner Gen I Move "Y / A drops it", and the code refuses A. That checklist line was already inaccurate. The change makes it true and supports the owner's mental model.
6. Docs and comments that also say "Y places" or "Y drop" and were not listed:
   - docs/hardware/RETEST_1ca2faec.md:115 ("Moving a Pokemon: Y places it, B cancels")
   - docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md:140 ("Y drops, B cancels")
   - docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md:71
   - tests/test_gen12_packed_move.cpp:459
   - tests/test_gen4_staged_pokemon_editor.cpp:601
   - src/UI/Panels/StoragePanel.cpp:204 ("validated Y drop")
   - Gen5SharedReadOnlySurface.inc:2299 (the comment literally says "Y Drop / L/R Box / B Cancel")
   The owner asked for "all the help menu and anything else", so the new RETEST doc for this change should update these.
7. The multi-select footer {"Y","Hold / Release Pick Up"} (:1232) and the status at :1195 belong to pickup and must stay. The investigation does not mention them; they are correct as they are.
8. Gen II has its own browse footer (TrainerViewScreenCompositeOverlay.cpp:286-319, "A Actions / X Add / L/R Box / + Review / B Back", no Y). It is gated on `!screen.swapActive`, so it is hidden during a carry and the change does not affect it. It is a separate pre-existing difference: Gen II's footer omits "Y Move". Which footer wins when both draw (drawFinalGen2Surface, then ClassicPackedMove::draw at Composite:503-504) was not verified. Classify it as 6, needing hardware confirmation, if it matters.

**Missed generations or paths.**

Generations:
- No generation was missed. Gen I-IV go through ClassicPackedMove::handleInput (Composite:439-440), and Gen V through handleGen5CarryInput (Gen5SharedReadOnlySurface.inc:1594).
- Both the Gen IV Storage-pane carry (state.storage, storageCarryNavigation) and the Gen I Actions -> Move carry go through the same state.active branch. A placing with A covers them with no extra wiring.

Paths and side effects the investigation missed:
1. Y must return true (be consumed) during a carry. Otherwise Gen V Y-Review, and the later Gen I-IV editor handlers, could act mid-carry.
2. Footer touch works as claimed. A tapped footer glyph goes through navTouchButton, which is ORed into `down` at Composite:378 before Gen5SharedReadOnlySurface::handleInput, so a {"A","Place"} footer stays tappable in Gen V too.
3. These pin tests were not listed: gen3_hardware_surface_contract:315 and :318 (B literal and the comment literal) and classic_help_accuracy:111 (B literal). They break if the dispatch is refactored to decide().
4. Doc and comment strings not listed: RETEST_1ca2faec.md:115, RETEST_62a2f8c3.md:109 (already claimed "Y / A drops"), GEN14_CAMPAIGN_PROGRESS.md:140, ROUND3_PART9_REPRO_0faf7e9c.md:71, test_gen12_packed_move.cpp:459, test_gen4_staged_pokemon_editor.cpp:601, Gen5SharedReadOnlySurface.inc:2299.
5. The multi-select footer at ClassicPackedMoveOverlay.inc:1232 is pickup UI and must not be changed to A.
6. The Gen II-only browse footer (Composite:286-319) omits Y Move. It is gated off during a carry, and which footer draws on top in Gen II was not verified.

No save-safety regressions were found in the proposed fix. place() and cancel() stay unchanged, and every place path returns true.

</details>

### H39 — File browser Cancel on '-' (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** All of Gen I, II, III, IV and V are affected the same way, and none of them already has a Cancel control. One shared dialog serves every generation. src/UI/PokemonFileMenus.inc:180-195 (beginImport) builds a gen1Import..gen5Import stage callback, then calls the one PokemonFileDialog::openImport. src/UI/TrainerViewScreenCompositeOverlay.cpp:386-387 passes it input before any generation surface, including the Gen V surface at :389. Export to file has no browser: exportPokemon (PokemonFileMenus.inc:197-213) writes to a fixed folder through Io::exportPokemonFile, so this request does not apply to it.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

The dialog is the only SD-card file browser in the app. It has two parts: the pure model include/UI/PokemonFileDialogModel.h and the renderer/runner src/UI/PokemonFileDialog.cpp.

Controls today (model header comment, PokemonFileDialogModel.h:7-11):
- A opens a folder or picks a file.
- B goes up one folder. At the start folder or the SD root, B closes the dialog.
- In Favourites, B or X returns to Browse and does not close the dialog.

B handling in Browse mode (PokemonFileDialogModel.h:162-173): Command::Close is returned only when `!parent || sameFolder(listing.folder, startFolder)`. In every other case B returns OpenFolder(parent). In Favourites mode (:127-128), B and X only set mode = Browse.

So B at the root or start folder already closes the dialog and cancels the import. From any subfolder, the only controller way out is to press B once per folder level until you reach the start folder (or the root, if you went above the start folder). The one shortcut is touch: a completed tap outside the frame calls finish() directly (PokemonFileDialog.cpp:184-188).

The key table at PokemonFileDialog.cpp:204-213 maps Up, Down, L/Left, R/Right, A, B, Y and X. It does not map HidNpadButton_Minus or Plus, and the Key enum (PokemonFileDialogModel.h:30) has no Cancel value. While the dialog is open, handleInput always returns true (PokemonFileDialog.cpp:174, :233), so TrainerViewScreen::update returns at CompositeOverlay.cpp:387 and Minus is swallowed and does nothing. It never reaches the Help toggle at TrainerViewScreenBase.inc:2813.

Footers (PokemonFileDialog.cpp:296-300):
- Browse: "A Open / Y Favourite|Unfavourite / X Favourites / L/R Page / B Back"
- Favourites: "A Open / Y Remove / B Back"
"B Back" does not say whether B goes up a folder or closes the dialog.

Closing is non-destructive. finish() (:82-87) clears active, stage, the scrollbar and controller navigation. Nothing is staged, and the import folder is remembered only on a successful pick (:129-131). The B close posts no status line.

**Root cause.**

This is not a defect in existing code. It is a missing control. The dialog model has no Cancel key or command path that closes from any depth or mode. B has two jobs ("up one folder" until the start folder, then "close"), and both are in one switch case at include/UI/PokemonFileDialogModel.h:162-173. The runner never maps HidNpadButton_Minus (src/UI/PokemonFileDialog.cpp:204-213), so a press of '-' is consumed and dropped. The footers (:296-300) show only "B Back".

Is '-' already used? App-wide, '-' means Help on screens: ClassicBrowseFooter.h:41 {"-","Help"}, TrainerViewScreenBase.inc:2813, AppShellScreen.cpp:433, SaveSelectScreen.cpp:3841/4370/4534. Inside this modal, though, '-' does nothing today because the dialog consumes all input (CompositeOverlay.cpp:387) and offers no Help. So there is no conflict. There is also a precedent for '-' = Cancel inside a modal: KeyboardDialog.cpp:443-446 calls finish(false) on Minus, and its footer says "+: OK | -: Cancel" (KeyboardDialog.cpp:805-813).

A footer "-" badge is already touch-capable: ScreenChrome.h:435 maps "-" to HidNpadButton_Minus, and CompositeOverlay.cpp:378 ORs navTouchButton(touch) into `down` before the dialog sees it.

**Shared owner.**

The fix belongs to the dialog model, include/UI/PokemonFileDialogModel.h (press() and the Key enum), and its runner and footer, src/UI/PokemonFileDialog.cpp. Every generation reaches import through beginImport → PokemonFileDialog::openImport, so one change here covers Gen I-V. No other SD folder browser exists that would need to share it. Export writes to a fixed folder. The Legacy RBY/GSC/FRLG "SourceBrowser" files are discovery card builders that read no input (0 HidNpad references), and BackupSelectionScreen is a flat backup list with its own B and delete-cancel path. Any future SD browser, such as save or bank import, should reuse this model rather than copy it.

**Proposed fix.**

The fix is small and in the model first:

1. In PokemonFileDialogModel.h, add `Key::Cancel` to the enum at :30. In press(), handle `Key::Cancel` in both Favourites mode (:122-143) and Browse mode (:149-200) by returning `Command::Close` without changing folder or mode. Update the header comment (:7-11) to say: "- cancel: close the dialog from any folder or list; nothing is imported".
   - Keep B exactly as it is: parent folder, close at the start folder or root, leave Favourites. That keeps the B behaviour the owner described and the B behaviour existing tests pin.

2. In PokemonFileDialog.cpp:204-213, add `{down & HidNpadButton_Minus, Model::Key::Cancel}`. Put it before A so that '-' wins if pressed together with A in the same frame. The loop already stops after a command.
   - Optionally, give run()'s Close case (:98) a status message, for example "Import cancelled; nothing was staged". This needs either a separate Command::Cancel or an Outcome message, so that B-close stays silent if the owner prefers.

3. Footers, PokemonFileDialog.cpp:296-300:
   - Browse: `{"A","Open"},{"Y","Favourite|Unfavourite"},{"X","Favourites"},{"L/R","Page"},{"B", atStartOrRoot ? "Close" : "Up folder"},{"-","Cancel"}`
   - Favourites: `{"A","Open"},{"Y","Remove"},{"B","Back"},{"-","Cancel"}`
   - The "-" badge is tappable through the existing navTouchButton path.
   - To compute the B label, add a small pure helper in the model, e.g. `bool bCloses(const State&)`, that reuses the :164 condition so the label and the behaviour cannot drift.

4. The owner asked for this "app wide". No other file browser exists, so app-wide here means: the import dialog in every generation, both of its modes, and a footer contract that covers it. Do not reassign '-' on normal screens, where it stays Help. Document the convention: inside a modal picker or dialog that has no Help, '-' = Cancel, matching KeyboardDialog.

**Risks.**

Save safety: low. Cancel goes through the same finish() path as the existing B-close and tap-outside close (PokemonFileDialog.cpp:82-87). That path stages nothing, writes nothing, and leaves the remembered folder alone. The one write the dialog does, a favourite toggle saved to settings.cfg (:111), happens only on Y, so Cancel cannot cause it.

PASS behaviour: B semantics are unchanged, so tests and owner checks that pin "B goes to the parent / B at start closes" still hold. The only visible change to a PASS footer is the B label wording, if the dynamic "Up folder"/"Close" label is adopted. That label change is optional and can be dropped to keep "B Back" byte-identical.

Conventions: '-' means Help on ordinary screens. Using it as Cancel inside this modal follows the KeyboardDialog precedent and cannot conflict, because the dialog consumes all input. Owners may still expect '-' to open Help here, so the footer must show "- Cancel".

Two smaller points. If Minus and A arrive in the same frame, the order of the key table decides which wins; put Cancel first. The dialog is static per TrainerViewScreen (stateFor), and finish() already clears stage, so no stale callback can run after Cancel.

**Open questions.**

1. Should B at the start folder or root keep closing the dialog, which is a second cancel path, or should B only ever go up a folder now that '-' exists? The recommendation is to keep it, since current tests and owner PASS checks pin it.
2. Should the B footer label change from "Back" to a context label ("Up folder" / "Close")?
3. Should Cancel post a status line such as "Import cancelled; nothing was staged"?
4. "App wide": the owner may mean other modal pickers, such as the species, move and item pickers, which today map X/Minus to search or other actions (e.g. SharedPokemonEditorSurface.inc:349, Gen3SharedPokemonSurface.inc:1306). This finding covers only the file browser. Reusing '-' as Cancel in those pickers would conflict with their existing Minus mappings and needs a separate owner decision.
5. On hardware, confirm that the "-" footer badge is reachable and that the touch hit works over the modal.

**Files to change:** `include/UI/PokemonFileDialogModel.h`, `src/UI/PokemonFileDialog.cpp`, `tests/test_pokemon_file_dialog_model.cpp`, `tests/test_ui_pokemon_file_menus_host.cpp`, `tests/test_footer_help_handlers.py`

**Existing tests affected:**
- tests/test_pokemon_file_dialog_model.cpp:81-94,107 pin B behaviour: Close at the start folder, OpenFolder(parent) with focus when deeper, Close at the SD root, and None plus Browse in Favourites. These should stay unchanged because B is kept as is.
- tests/test_ui_pokemon_file_menus_host.cpp:301-302 checks that the footer contains 'Open', 'Favourite', 'Favourites' and 'Back' using drawn(). If the B label changes to 'Up folder'/'Close', update this test on purpose (keep 'Back' only in Favourites) and add 'Cancel'.
- tests/test_ui_pokemon_file_menus_host.cpp:333-334 (B leaves Favourites, dialog stays open) and :361-364 (B at the start folder closes, nothing staged) stay valid.
- tests/test_popup_dismiss.cpp:139,148 counts PokemonFileDialog.cpp as a SharedPopupDismiss publisher (publishers == 20). It is unaffected if no new publish call is added.

**New tests:**
- test_pokemon_file_dialog_model.cpp: from a subfolder deeper than the start folder, press(Key::Cancel) returns Command::Close, and the listing folder and mode are unchanged.
- test_pokemon_file_dialog_model.cpp: in Favourites mode, press(Key::Cancel) returns Close (contrast with B, which returns None plus Browse). At the start folder and at the SD root, Cancel also returns Close.
- test_pokemon_file_dialog_model.cpp: the bCloses()/footer-label helper agrees with what press(Key::B) returns, for start folder, root, deeper folder and a folder above the start folder.
- test_ui_pokemon_file_menus_host.cpp, per generation 1-5: open Import from file, press A into ok/, press Minus. The dialog is closed, c.snapshot() is unchanged, Io::openedFileCount() is unchanged, and g_folderPreferences.lastImport is not updated. Repeat from the Favourites list.
- test_ui_pokemon_file_menus_host.cpp: the footer shows '-' 'Cancel' in both modes, and tapping the '-' footer badge (navTouchButton) closes the dialog the same way. Pressing Minus while the dialog is open does not open the screen Help overlay (helpOverlayActive stays false).
- tests/test_footer_help_handlers.py: add a SCREENS entry 'Import from file dialog' with files ['src/UI/PokemonFileDialog.cpp'] and handlers ['src/UI/PokemonFileDialog.cpp','include/UI/PokemonFileDialogModel.h'], so every footer key, including '-', must map to a real HidNpadButton_* handler. Check that the extractor can parse the initializer-list drawNavBar form used here.

**Skeptic corrections.**

Every file:line I checked in the import dialog matches the code. The commit checked out is 67b1b6a0. Mostly confirmed; the one claim that does not hold is that no other SD browser exists.

CONFIRMED:
- include/UI/PokemonFileDialogModel.h:30: the Key enum has no Cancel. :7-11 is the header comment.
- PokemonFileDialogModel.h:127-128: in Favourites, B and X only set mode = Browse.
- PokemonFileDialogModel.h:162-173: B returns Close only when `!parent || sameFolder(folder, startFolder)`. Otherwise it returns OpenFolder(parent) with focus.
- src/UI/PokemonFileDialog.cpp:82-87 finish() clears active, stage, the scrollbar and controller navigation. :98 is the Close case. :129-131 remember the folder only after a successful pick. :111 saveSettings runs only on ToggleFavorite.
- PokemonFileDialog.cpp:174 and :233: handleInput returns true whenever the dialog is open. :184-188 is tap-outside → finish().
- PokemonFileDialog.cpp:204-213: the key table has no Minus or Plus.
- PokemonFileDialog.cpp:296-300: the footers say "B Back".
- src/UI/TrainerViewScreenCompositeOverlay.cpp:378: `down |= navTouchButton(touch)`. :387: the dialog takes input before Gen5SharedReadOnlySurface (:389), so Minus is swallowed and never reaches the Help toggle at TrainerViewScreenBase.inc:2813.
- src/UI/PokemonFileMenus.inc:182-195 beginImport dispatches gen1..gen5Import to the one openImport.
- Footer badge taps work over the modal: SharedPopupDismiss::tappedOutside (include/UI/SharedPopupDismiss.h:36-40) ignores taps whose start or end is at or below kFooterTop. A tap on the "-" footer badge will not be eaten by tap-outside, and can reach the dialog as a Minus through navTouchButton.
- The cited tests match:
  - tests/test_pokemon_file_dialog_model.cpp:82, 88, 93 and 107 pin B.
  - tests/test_ui_pokemon_file_menus_host.cpp:301-302 checks drawn("Back"). :333-334 and :363-364 pin B leaving Favourites and B at the start folder closing.
  - tests/test_popup_dismiss.cpp:148 asserts publishers == 20.

CORRECTIONS:
1. The KeyboardDialog path is wrong. It is src/UI/Dialogs/KeyboardDialog.cpp, not src/UI/KeyboardDialog.cpp. The Minus → finish(false) is at :444-447 and the "-: Cancel" footer strings are at :805-813. The precedent is real.

2. REFUTED: "No other SD folder browser exists". A second SD-card file browser exists: SaveSelectScreen's Link Game File picker (Overlay::GameFilePicker).
   - Code: src/UI/SaveSelectScreen.cpp openGameFilePicker :1975-1991, refreshGameFilePicker (opendir/readdir) :1993-2035, activateGameFilePicker :2037-2066, browseGameFileParent :2068-2073. Input is at :3861-3909.
   - Its controls are the opposite of the import dialog's. B = Cancel, closing the whole picker from any depth (:3880-3884). Y = Up Folder (:3885-3888). X = Start Folder (:3889-3893). Minus has no handler, so it does nothing while the picker is open.
   - Footer at :5726-5727: {"Y","Up Folder"},{"X","Start Folder"},{"B","Cancel"}.
   - So the app already has two SD browsers that disagree. In one, B = Cancel and Y = up. In the other, B = up (and close only at the start) and Y = favourite. Class 2 (cross-UI inconsistency) is stronger than the investigation says.
   - The owner asked for this "app wide", so '-' = Cancel should also be added to the GameFilePicker. This is safe, since Minus is unused there and Help is reached elsewhere (:3841 is GameWorkspace, not the picker).
   - At minimum, the inconsistency must be put to the owner as a decision. A follow-up would be to move the GameFilePicker onto the PokemonFileDialogModel-style pure model.
   - Linking a launch file writes settings through saveGameLaunchBinding (:2047) only on A on a file. A Minus cancel there would write nothing.

3. tests/test_footer_help_handlers.py does not list PokemonFileDialog.cpp in SCREENS (grep finds no match), so the import dialog footer is not footer-contract tested today. The proposal to add it is valid.
   - Footer pairs are extracted by the PAIR regex inside drawNavBar(...).
   - The Y entry uses a ternary label (`s.model.listing.favorite ? ...`), which PAIR will not match. The Y key is therefore silently skipped, not failed.
   - If a dynamic B label is added the same way, it will also be skipped by the checker.

4. More tests pin the GameFilePicker and would be touched by an app-wide change:
   - tests/test_game_hub_contract.py:173-180 requires `"Y", "Up Folder"` and the GameFilePicker overlay. :679 checks the draw block.
   - tests/test_footer_help_handlers.py "HOME / game selection / Game Browser" (SaveSelectScreen.cpp) would check a new {"-","Cancel"} pair against the HidNpadButton_Minus handlers. Those already exist in the file, so the check passes even with no Minus handler in the picker, because it is not scoped per overlay. A dedicated host test is needed.

5. Minor: the investigation's model fix is right. Handle Key::Cancel before the Favourites switch returns. The Favourites switch at :122-143 enumerates keys with no default, so adding Key::Cancel to the enum without handling it there would be a -Wswitch warning or a silent no-op. Handle it in both switches, or once before the mode branch.

Save safety and PASS behaviour: I agree the risk is low. Cancel → finish() stages nothing. B is unchanged, so the PASS checks at test_ui_pokemon_file_menus_host.cpp:333-334 and :363-364 hold. Changing "B Back" breaks :301 drawn("Back") only if Favourites also loses "Back".

**Missed generations or paths.**

The generation coverage is right. All of Gen I-V reach the dialog through PokemonFileMenus::beginImport. Call sites: ClassicReleaseActionFix.inc:67 (Gen I) and :418 (Gen II), Gen3SharedPokemonSurface.inc:1725, Gen4SharedPokemonSurface.inc:1095, Gen5SharedReadOnlySurface.inc:1680.

Missed app-wide path: the SaveSelectScreen Link Game File SD browser (Overlay::GameFilePicker, src/UI/SaveSelectScreen.cpp:1975-2073, input :3861-3909, footer :5726-5727). It uses B = Cancel and Y = Up Folder, the opposite of the import dialog. Minus does nothing there today. "App wide" '-' = Cancel should cover it too: add `if (kDown & HidNpadButton_Minus) { launchFileTouchScroll.stop(); overlay = launchFileReturnToLegacy ? Overlay::LegacyInstances : Overlay::None; return; }` next to B, and add {"-","Cancel"} to its footer. Pinning tests: tests/test_game_hub_contract.py:173-180, :679, and the footer contract for SaveSelectScreen.cpp. Its new tests must be host or contract tests scoped to the picker block, because the footer checker is file-wide.

Lesser modal dialogs with Cancel/B semantics, such as the profile picker at :5579 and the :5812 dialog, are not file browsers. They need an owner decision before they are included.

</details>

### G5-RADAR — Gen V quick-view stat graph (P1)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Only Gen V (BW/B2W2 PK5) is affected. These already use the shared StatsRadar renderer and adaptive scale that the editor and View use:
- Gen I: src/UI/Panels/BoxPokemonPanel.cpp:314-318, drawGen1Labeled.
- Gen II: lines 319-322, drawGen2Labeled.
- Gen III/FRLG and Gen IV: lines 323-335, drawGen3Labeled.

Gen V is the only shipped generation still in the legacy else branch (lines 336-365). Gen VI-IX are not shipped editors here, but they would also land in that branch.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

The Box summary side panel is Panels::drawBoxSummaryPanel (src/UI/Panels/BoxPokemonPanel.cpp:220), called from src/UI/TrainerViewScreenBase.inc:5202 with summaryW=452. It picks the radar by generation at BoxPokemonPanel.cpp:314-365.

The Gen III/IV branch condition at lines 323-324 is `p->getGameGroup()==FRLG || (exact && exact->identity.generation == Foundation::Generation::Gen4)`. Gen V has an exact capability (include/UI/ExactSaveCapabilities.h, the NintendoDS/Gen5/PK5 entry), but it does not match this condition. So it falls into the else branch (lines 336-365), commented "Other later formats keep the inherited summary renderer until their shared-editor pass".

That branch calls the inherited PKSE `fb.drawStatHexagon(hexCx, hexCy, 70, vals, 6, 255.0f, ...)` (line 347; renderer at src/UI/PKSEFramebuffer.cpp:458-491) and draws hand-placed labels (lines 350-363).

The Gen V editor and View take a different path:
1. SharedPokemonEditorSurface.inc:1174 calls A::stats.
2. That is Gen5SharedReadOnlySurface.inc:524-527, which returns displayStats5.
3. SharedPokemonShell::drawDataAndGraph (SharedPokemonEditorSurface.inc:1254; include/UI/SharedPokemonShell.h:187-212) draws the graph.
4. It calls StatsRadar::drawGen3Labeled (include/UI/StatsRadar.h:170), which is drawGen2Labeled (StatsRadar.h:129-168), built on radarModel (include/UI/BattleStatRadarModel.h:23-34).

The stat values are the same on both sides. Pokemon5ReadOnlyView::statHPMax() and the other stat getters (include/Pokemon/Pokemon5ReadOnlyView.h:122-127, 166-169) return displayStats5, the same function the editor uses (include/Integration/Gen5/Gen5DisplayStats.h:18-34). The boxes are rebuilt from the staged save in Gen5ReadOnlyTrainer::rebuildPresentation (src/Legacy/Gen5ReadOnlyTrainer.cpp:64-96). So the label numbers do change between Pokemon. The polygon shape does not visibly change.

**Root cause.**

The Gen V box summary still uses the inherited PKSE hexagon instead of the shared StatsRadar. This is because the branch condition at src/UI/Panels/BoxPokemonPanel.cpp:323-324 lists FRLG and Gen4 only, so Gen5 falls into the else branch at lines 336-365.

The legacy path differs from the editor's radar in four ways:

1. **Fixed 255 scale with a 0.06 floor.** maxValue is 255.0f (BoxPokemonPanel.cpp:347), and PKSEFramebuffer.cpp:481-482 clamps each axis to [0.06, 1.0].
   - Any stat at or below about 15 (0.06 x 255) draws at the same minimum point.
   - Stats in the 20-60 range (typical for in-game Gen V Pokemon below about level 30) are only 8-24% of a 70 px radius.
   - So adjacent Pokemon draw almost the same tiny blob, which reads as "doesn't work / isn't updated" while scrolling.
   - This is the same defect the Gen IV branch comment (lines 326-328) says was fixed for Gen IV: "fell through to the legacy fixed-255 renderer, collapsing low-level Pokemon to a dot".
   - The editor instead uses the adaptive bucket scale in radarModel (BattleStatRadarModel.h:23-34): peak x 1.1, rounded up to a step of 5, 10 or 50, minimum 20. It also prints a "Scale N" caption (StatsRadar.h:163-167).

2. **Smaller size.** The radius is hard-coded to 70 (BoxPokemonPanel.cpp:342). The shared renderer takes the radius from the box it is given: min((width-24)/2, (height-2*lineH-22)/2) (StatsRadar.h:146-147). Gen I-IV pass width-36 = 416 wide and height up to 236 (lines 333-334), which gives a radius of roughly 90-100 px.

3. **Different axis layout.** The legacy hexagon uses HOME order HP, Atk, Def, Spe, SpD, SpA, with SpD at lower-left and SpA at upper-left (BoxPokemonPanel.cpp:338-340, PKSEFramebuffer.cpp:460, PKSEFramebuffer.h:99-101). The shared radar uses HP, Atk, Def, Spe, SpA, SpD clockwise (BattleStatRadarModel.h:11), with SpA at lower-left and SpD at upper-left. So Sp.Atk and Sp.Def swap sides between the quick view and the editor.

4. **Different visual style.** The legacy hexagon has a filled translucent polygon in Accent colours. The shared radar uses a dotted web with a FocusBorder outline (StatsRadar.h:63-91).

**Not found:** I found no caching or stale-frame mechanism. The panel redraws every frame from screen.trainer.boxes[selectedBox][selectedItem], so "not updated" comes from the scale problem, not stale data.

**Shared owner.**

The fix belongs in the radar-branch selection in drawBoxSummaryPanel (src/UI/Panels/BoxPokemonPanel.cpp:314-365), routed through the shared renderer UI::StatsRadar::drawGen3Labeled and PokeBank::UIModel::radarModel (include/UI/StatsRadar.h, include/UI/BattleStatRadarModel.h). These are the same components SharedPokemonShell::drawDataAndGraph uses for every Gen I-V editor and View.

The cleanest owner is a single six-stat default branch: any non-Gen I and non-Gen II record uses drawGen3Labeled with the record's six stat getters, and drawStatHexagon is no longer used in the summary panel. That way Gen V, and any future generation, inherits the editor's graph automatically instead of needing to be added to a whitelist.

**Proposed fix.**

Minimal change, in src/UI/Panels/BoxPokemonPanel.cpp only:

1. Replace the Gen III/IV condition at lines 323-324 with a plain `else` six-stat branch (or, more conservatively, add `|| (exact && exact->identity.generation == Foundation::Generation::Gen5)`). The branch body is unchanged:
   - build `std::array<uint16_t,6>{statHPMax, statATK, statDEF, statSPE, statSPA, statSPD}`;
   - set `radarH = std::min(236, y + height - 36 - (cy2 + 4))`;
   - call `StatsRadar::drawGen3Labeled(fb, x+18, cy2+4, width-36, radarH, stats)`;
   - set `iy = cy2 + radarH + 8`.
2. Delete the legacy else block (lines 336-365) that calls drawStatHexagon.

This makes the quick view show the editor's own model:
- Values: Pokemon5ReadOnlyView already returns displayStats5, the same source as Gen5SharedReadOnlySurface::stats. For extra safety, you could call PokeVault::Integration::Gen5::displayStats5(view.source(), b2w2) directly, but the getters are already identical.
- Order: HP/Atk/Def/Spe/SpA/SpD.
- Scale: adaptive, with the "Scale N" caption.
- Size: the same as the Gen III/IV quick view.

The Nature/Ability/Held Item rows below keep working, because iy is set from radarH exactly as for Gen III/IV.

Optional follow-up: PKSEFramebuffer::drawStatHexagon then has no callers. It can stay, since a host stub exists at tests/host-ui/HostUiStubs.cpp:67, or be removed in a separate cleanup.

**Risks.**

There is no save-safety risk. This is a read-only drawing path: it reads stat getters that are already computed for display and calls no mutator. Pokemon5ReadOnlyView's mutators are inert anyway.

Behaviour the owner marked PASS:
- The Gen I, II, III and IV quick-view branches must be left byte-identical.
- The Gen IV carried-source hiding (lines 233-234) and the touch target 2000 (line 386) must be preserved.

Layout: the shared radar at radarH up to 236 is taller than the legacy layout (whose labels started at roughly hexCy+104). The Nature/Ability/Held Item rows plus the hint at y+height-24 must still fit for Gen V. This is the same geometry Gen III/IV already pass on hardware, and Gen V shows the same three rows, so the risk is low. It still needs one visual hardware check.

Behaviour changes:
- Visual: the SpA and SpD vertices move to match the editor. Users who were used to HOME's SpD-left layout will see a change, but this is the intended consistency.
- Records with unknown personal data, or EXP beyond level 100: displayStats5 returns all zeros. The adaptive model then gives scale 20 with all axes at 0, a collapsed point. That is the same as the editor and acceptable, but worth noting.

**Open questions.**

1. The owner should confirm the desired axis order for the quick view. I assumed the editor's HP/Atk/Def/Spe/SpA/SpD rather than the HOME SpD/SpA order, because they asked for it to match the editor.
2. Should drawStatHexagon be deleted now that nothing calls it, or kept for future Gen VI+ work? It is cosmetic only.
3. Hardware confirmation:
   - The Gen V summary radar now changes shape between low-level box Pokemon.
   - It is the same size as the Gen III/IV quick view.
   - The Nature/Ability/Held Item rows still clear the bottom "A / tap" hint.
4. The 'doesn't update' symptom is attributed to the fixed 255 scale with the 0.06 floor, not to stale data. I found no caching, and the panel re-reads the box slot every frame. If hardware still shows identical shapes after the fix for Pokemon whose labelled values differ, investigate further.

**Files to change:** `src/UI/Panels/BoxPokemonPanel.cpp`, `tests/test_gen5_readonly_ui_contract.py (or a new C++ surface-contract test)`, `tests/test_gen4_hardware_surface_contract.cpp (optional: extend the summary-radar assertion to all six-stat generations)`

**Existing tests affected:**
- No existing test pins the legacy Gen V else branch: grep found no references to drawStatHexagon or hexR outside src/UI/PKSEFramebuffer.cpp and BoxPokemonPanel.cpp, apart from the empty host stub at tests/host-ui/HostUiStubs.cpp:67, which can stay.
- tests/test_gsc_editor_surface_contract.cpp:184-186 asserts BoxPokemonPanel.cpp contains StatsRadar::drawGen2Labeled and Gen2Workspace::battleStats(record). Both must stay unchanged.
- tests/test_gen4_hardware_surface_contract.cpp:245-251 and tests/test_touch_appwide_surface_contract.py:911-916 assert the hidesGen4CarriedSource strings in BoxPokemonPanel.cpp. These must be preserved; they are untouched by this fix.
- tests/test_gen2_unown_letter_selector.cpp:201 and tests/test_party_summary_tap_routing.cpp read BoxPokemonPanel.cpp, including the 2000 touch id. Both are unaffected.
- tests/test_gen5_readonly_save.cpp:1386-1388 and tests/test_shared_editor_contract.cpp:296-298 pin that boxed Gen V stats come from displayStats5. They remain valid and guarantee the value source matches the editor.

**New tests:**
- A source-contract test, in tests/test_gen5_readonly_ui_contract.py or a new C++ contract test: BoxPokemonPanel.cpp no longer contains `drawStatHexagon` or `255.0f`, and the Gen V summary route reaches `StatsRadar::drawGen3Labeled`. This is best checked by asserting there is no generation whitelist that excludes Gen5, i.e. the six-stat branch is the default.
- A host model test (pure C++, no framebuffer) on a real Gen V fixture, e.g. the boxPokemon(0,0) used in test_shared_editor_contract.cpp. Build the six-stat array from Pokemon5ReadOnlyView getters and assert it equals displayStats5 / Gen5SharedReadOnlySurface::stats ordering (HP, Atk, Def, Spe, SpA, SpD). Then assert radarModel(...) gives the same scale and normalized values as the editor path, so the quick view and the editor are numerically identical.
- A regression test for the reported symptom: two low-level Gen V box Pokemon with different stats (e.g. under 30) must produce different radarModel normalized vectors, with at least one axis differing by 0.1 or more. Under the legacy 255 scale with the 0.06 floor, they collapse together.
- An optional cross-generation table test: for every shipped six-stat generation (III FRLG, IV, V), the summary-panel radar selector maps to the same renderer the editor's drawDataAndGraph uses.

**Skeptic corrections.**

I checked every cited line in wmain (HEAD 67b1b6a0) and the root cause holds. There are no material errors in the cited evidence. Below are what I confirmed, then the corrections and caveats.

**Confirmed**

- **Branch selection.** src/UI/Panels/BoxPokemonPanel.cpp:314-318 is Gen I, drawGen1Labeled. Lines 319-322 are GSC, drawGen2Labeled. Lines 323-335 are FRLG or exact Gen4, drawGen3Labeled with `radarH = min(236, y+height-36-(cy2+4))` and `iy = cy2+radarH+8`. Lines 336-365 are the legacy else branch: `hexR=70` (342), `drawStatHexagon(..., 255.0f, ...)` (347), vals in SpD-before-SpA order (338-340), and `iy = hexCy+hexR+34` (365).
- **Legacy renderer.** src/UI/PKSEFramebuffer.cpp:458-491 uses kAngles {90,30,-30,270,210,150} and `std::clamp(v, 0.06, 1.0)` at 481-482. The fixed 255 scale plus this floor confirms the "tiny, unchanging blob" explanation.
- **Shared model.** include/UI/BattleStatRadarModel.h:11 sets the HP/Atk/Def/Spe/SpA/SpD order. Lines 23-34 hold the adaptive bucket scale (min 20, step 5/10/50, peak x 1.1).
- **Shared renderer.** include/UI/StatsRadar.h:129-168 computes the radius from the box (146-147) and draws the "Scale N" caption (163-167). Line 171 makes drawGen3Labeled a forward to drawGen2Labeled.
- **Editor path.** include/UI/SharedPokemonShell.h:211 calls drawGen3Labeled. src/UI/SharedPokemonEditorSurface.inc:1174 calls A::stats and line 1254 calls drawDataAndGraph. src/UI/Gen5SharedReadOnlySurface.inc:524-527 returns displayStats5.
- **Value source.** include/Pokemon/Pokemon5ReadOnlyView.h:122-127 has the stat getters, which go through displayStat (166-169) to displayStats5. Its getGameGroup returns sourceGroup_ (line 91), which is BW or B2W2 and never FRLG. The view is built at src/Legacy/Gen5ReadOnlyTrainer.cpp:76 and :86. So the stat values match the editor; only the renderer differs.
- **Gen V exact capability.** include/UI/ExactSaveCapabilities.h:86-99 gives Gen V exact capabilities with generation Gen5, which does not match the line 323-324 condition.

**Routing (the investigator did not show this)**

TrainerViewScreenCompositeOverlay.cpp:485 calls Gen5SharedReadOnlySurface::draw. In Browse plus detailViewActive with Boxes mode, that falls through to `screen.drawGSCOverlay(fb)` at Gen5SharedReadOnlySurface.inc:2277. That reaches the base renderer, whose ViewMode::Boxes case calls Panels::drawBoxSummaryPanel at TrainerViewScreenBase.inc:5202 (summaryW=452). Gen V has no quick-view renderer of its own: grep finds no Summary or Radar drawing in Gen5SharedReadOnlySurface.inc. So this function is the only fix site.

**Gen III RSE is correct but mis-explained**

The condition names only FRLG, yet Ruby/Sapphire/Emerald are not broken. Their records are Pokemon3FRLG (src/Legacy/FRLGReadOnlyTrainer.cpp:21 and :32 cover ruby/sapphire/emerald), and Pokemon3FRLG.h:59 returns FRLG. The investigator's "Gen III/FRLG" wording hides this. Any fix must keep the FRLG game-group test, or replace it with a check that RSE records also satisfy.

**Correction to the proposed fix and its risks**

The "plain else" option is not risk-free. Those later loaders and their per-game save-select entries are still in the tree:
- src/Save/GetSaveFileContents.cpp has Trainer7LGPE, Trainer8SWSH, Trainer9SV, Trainer8LA and Trainer8BDSP loaders (lines 228-280 and 387-635).
- src/UI/SaveSelectScreen.cpp has portrait and date handling for Let's Go, Sword/Shield, Legends Arceus, Scarlet/Violet and Z-A (lines 181-188 and 501-520).
- ExactSaveCapabilities.h:100-117 maps PB7, PA8 and PK9.

I did not find a gate that makes these unreachable from the Box view; the "not shipped" claim is unproven. A plain else would therefore also change the summary panel for every save type that still reaches the legacy branch: SWSH, BDSP and Z-A have no exact entry, and LGPE, LA and SV do.

For a v1 repair, the minimal and safest fix is the conservative one: add `|| (exact && exact->identity.generation == Foundation::Generation::Gen5)` to line 323-324. Making the six-stat branch the default should be a separate, deliberate change that names those Gen VII-IX paths.

**Other corrections**

- **Stale comments.** The comments at BoxPokemonPanel.cpp:217-219 ("stat hexagon") and at TrainerViewScreenBase.inc:5198 ("render + hexagon") will be stale. That is cosmetic.
- **Missing pinning test.** Only tests/test_gen3_hardware_surface_contract.cpp:191 pins drawGen3Labeled, and only in the shell. No test pins drawGen3Labeled in BoxPokemonPanel.cpp, so a contract assertion that the Gen5 route reaches it is genuinely missing.
- **Axis-order risk mis-stated.** The owner already accepted SpA lower-left / SpD upper-left for Gen III and IV quick views and all editors. So for Gen V this is consistency, not a new open question.

**Not verified here**

Whether Gen V's three info rows plus the bottom hint fit at radarH=236 is not verified in code. The geometry matches Gen IV, which passed on hardware, but it still needs a hardware check.

**Missed generations or paths.**

**Missed paths**

1. **Gen VII-IX legacy save loaders and their per-game save-select entries are still in the tree.** These take the same else branch:
   - LGPE, Legends Arceus and Scarlet/Violet have exact capabilities (ExactSaveCapabilities.h:100-117).
   - SWSH, BDSP and Z-A have no exact entry.
   - Code: GetSaveFileContents.cpp:228-635 and SaveSelectScreen.cpp:181-188 and 501-520.

   I found no gate that makes them unreachable from the Box view. So the proposed "plain else / delete legacy block" changes their quick view too. Either scope the fix to Gen5, or first confirm in code that Gen VI+ cannot reach drawBoxSummaryPanel.

2. **Gen III RSE.** It is correct only because RSE records are Pokemon3FRLG with getGameGroup()==FRLG (FRLGReadOnlyTrainer.cpp:21,32; Pokemon3FRLG.h:59). The investigator did not call this out. Any condition rewrite must preserve it.

3. **Switch-native FRLG titles** (firered_switch*/leafgreen_switch*, GameIdentity.cpp:35-50) also rely on that FRLG game-group test. They are unaffected by the Gen5-only fix.

4. **Gen V routing.** Gen V Boxes mode reaches the shared panel via Gen5SharedReadOnlySurface.inc:2277 (drawGSCOverlay) and then TrainerViewScreenBase.inc:5202. Gen V has no separate quick-view renderer, so BoxPokemonPanel.cpp:323-324 is the only fix site.

**Generations**

- Affected: Gen V only, both BW and B2W2.
- Correct: I, II, III (RSE and FRLG) and IV.

</details>

### X-IMPEXP — X = Import/Export on box slots (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** This is a binding change in all five generations (I-V). Today X on a box slot is "Add on an empty slot" in Gen I, II, III and IV. In Gen V, X does nothing on box slots and the footer does not show it. Import and Export already work in every generation (Feature A), but only from the A Actions sheet; none of them puts Import or Export on X. So no generation already does what the owner asks.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Shared rule: include/UI/SharedPokemonEditorContract.h:154-162. boxActivation(occupied, pressA, pressX) returns Add when X is pressed on an empty slot, Actions when A is pressed, and None otherwise, so X on an occupied slot does nothing. The comment above it says "X is a direct Add shortcut only on an empty visual cell".

Each generation's box input, in the dispatch order of src/UI/TrainerViewScreenCompositeOverlay.cpp:387-458:
- Gen I: src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:1264-1267 (reached through the Foundation handleInputUX -> Cleanup3 handleInputUX:965-980 -> handleInputUXCleanup2). Empty slot: ux2BeginAdd. Occupied slot: shows the status "X Add is available on an empty writable Gen I slot". The same path also runs on the Storage save pane (line 1256 reads stSaveBox/stSaveSlot). An older copy at Gen1PokemonEditorOverlayUX.inc:1168-1174 is no longer called.
- Gen II: src/UI/Gen2SharedSurfaceParity.inc:256-279 calls SharedEditor::boxActivation. Add -> beginCreate(screen, box). Line 279 swallows X on an occupied slot ("occupied X is intentionally not a hidden edit/add action"). The Storage pane has no X (comment at 281). Fallback: when the parity path does not take X (canEditStagedCopy false, e.g. a Japanese save behind the gate), TrainerViewScreenGSCOverlay.inc:934-940 opens Pokemon Actions on X for any slot.
- Gen III: src/UI/Gen3SharedPokemonSurface.inc:2713-2728 calls Shared::boxActivation. Add -> beginCreate on both the Boxes view and the Storage save pane (storageEntrySurface). X on an occupied slot returns false, so it falls through to the inherited handlers.
- Gen IV: src/UI/Gen4SharedPokemonSurface.inc:2410-2416 handles X only on an empty slot in the Boxes view: beginActions, then actionRow=0 (Add), then beginCreate. It does not use boxActivation. X on an occupied slot or on the Storage pane returns false and falls through to the inherited legacy box handler (TrainerViewScreenBase.inc:4796-4797, X = Release). That handler is stopped by requireMutableWorkspace() because the source is read-only, so the save is safe; the exact status shown has not been confirmed on hardware.
- Gen V: src/UI/Gen5SharedReadOnlySurface.inc:1858-1883. The Boxes handler only reads L/R/ZL/ZR, the D-pad and A (beginActions); X is ignored. While a Pokemon is being carried, X is refused at ClassicPackedMoveOverlay.inc:1052.

Footers:
- Gen I-IV box grid: src/UI/ClassicPackedMoveOverlay.inc:1272-1297. Occupied: {Y Move / Hold Multi}. Gen IV Storage pane: {Y Hold Multi}. Otherwise {X Add, Y Hold Multi}. gridOwnsInput covers Gen I-IV (lines 122-161), so this footer is drawn over the older ones.
- Older footers that also show X Add: TrainerViewScreenCompositeOverlay.cpp:309-312 (drawGen2ClassicBoxFooter), TrainerViewScreenBase.inc:5329-5340 (Gen IV string "A: Actions | X: Add | B: Back"), and Gen1PokemonEditorOverlayUXCleanup2.inc:1205 "A: Actions | X: Add | L/R: Box | B: Back".
- Gen V Boxes: Gen5SharedReadOnlySurface.inc:2300-2302 {D-pad, L/R, ZL/ZR Jump 5, A Actions, Y Review, B Back}, with no X.

Help: include/UI/ClassicHelpPresentation.h:35-41 (Page::Pokemon, shared by Gen I-V and chosen at TrainerViewScreenBase.inc:5591). It does not mention X on boxes at all. Its rows are "A Actions: View, Edit, Move, Clone, Origin or Release" and "A On an empty slot: create a Pokemon draft"; neither lists Import or Export.

Existing Import/Export (Feature A): include/UI/PokemonFileMenus.h:19-24 and src/UI/PokemonFileMenus.inc:183-209. beginImport opens the modal PokemonFileDialog and stages one record; for Gen I/II it goes to the box's first empty slot, and for Gen III-V to the chosen slot. exportPokemon writes a new file under pokemon/genN, never replaces an existing file, and never changes the workspace. The Actions sheets already call these:
- Gen I: ClassicReleaseActionFix.inc:62-76
- Gen II: ClassicReleaseActionFix.inc:412-428, capabilities at 164-173
- Gen III: Gen3SharedPokemonSurface.inc:1721-1731
- Gen IV: Gen4SharedPokemonSurface.inc:958-960, 978-979, 1091-1101 (only when stagedAvailable)
- Gen V: Gen5SharedReadOnlySurface.inc:1675-1683
The shared action list puts ImportFile on empty slots and ExportFile on occupied slots, depending on canImportFile/canExportFile (SharedPokemonEditorContract.h:72-95).

**Root cause.**

Nothing is broken here. The current X-on-box behaviour was designed on purpose: "PR #280 parity" made X an Add shortcut on empty slots and left it unbound on occupied slots. That rule is written into SharedPokemonEditorContract.h:154-162 and pinned by tests. Feature A (Oct 28) later added Import and Export only as rows in the Actions sheets and never gave them a button.

The cross-generation differences come from each generation wiring X separately instead of through one shared function:
- Gen IV writes its own empty-slot check (Gen4SharedPokemonSurface.inc:2410) instead of calling boxActivation.
- Gen I uses its own handler (Gen1PokemonEditorOverlayUXCleanup2.inc:1264).
- Gen V never wired X on boxes (Gen5SharedReadOnlySurface.inc:1882).
- On the Storage pane, X adds in Gen I and Gen III, is inert in Gen II, and falls through in Gen IV.
- Footers are hard-coded in four places, and Help does not describe X on boxes.

**Shared owner.**

The decision belongs in include/UI/SharedPokemonEditorContract.h: replace boxActivation/BoxActivation with a version that knows about capabilities, plus a footer-hint helper. The footer should come from the shared ClassicBrowseFooter spec built in src/UI/ClassicPackedMoveOverlay.inc:1272-1297 for Gen I-IV, with Gen V's box navbar reusing the same helper. Help text belongs in include/UI/ClassicHelpPresentation.h (Page::Pokemon). The action itself should always go through UI::PokemonFileMenus::beginImport/exportPokemon (src/UI/PokemonFileMenus.inc). Each generation should only translate a key press into the shared decision, the same way it already does for the Actions rows.

**Proposed fix.**

1. Shared contract (SharedPokemonEditorContract.h:154-162):
   - Change the enum to BoxActivation { None, Actions, ImportFile, ExportFile }.
   - Change the function to boxActivation(bool occupied, bool pressA, bool pressX, ActionCapabilities caps): pressA -> Actions; pressX && !occupied -> ImportFile if caps.canImportFile, otherwise None; pressX && occupied -> ExportFile if caps.canExportFile, otherwise None.
   - Add `constexpr const char* boxXLabel(bool occupied, ActionCapabilities caps)` returning "Import" or "Export", or nullptr when X does nothing.
   - Use "Import"/"Export" (or "Import File"/"Export File") in the footer. Do not use "Export Copy": that label already means X on the Review screen (SharedReviewPresentation.h:33, ClassicHelpPresentation.h:21).
   - Add Pokemon stays on the A Actions sheet, which still lists Add first on empty slots in every generation (actionsForSlot line 76, Gen IV line 958, Gen V availableActions). Create therefore stays reachable.

2. Wire each generation to the shared decision and to PokemonFileMenus, closing any open sheet first as the Actions rows already do:
   - Gen I: Gen1PokemonEditorOverlayUXCleanup2.inc:1264-1267. Import -> PokemonFileMenus::beginImport(screen,1,box,slot); Export -> exportPokemon(screen,1,false,box,slot). Only when editorAvailable() is true (already checked at line 1254). Remove the "X Add is available..." status.
   - Gen II: Gen2SharedSurfaceParity.inc:258-279. Replace BoxActivation::Add -> beginCreate with Import/Export calls (generation 2) using releaseActionCapabilities(). Remove the X swallow on occupied slots at line 279. Decide whether the GSC fallback X -> openPokemonActions (TrainerViewScreenGSCOverlay.inc:937) stays for gated saves; recommendation: make it inert.
   - Gen III: Gen3SharedPokemonSurface.inc:2715-2728. Replace the Add branch with Import/Export (generation 3, box/slot taken from the Boxes view or the Storage pane).
   - Gen IV: Gen4SharedPokemonSurface.inc:2410-2416. Replace the own-coded empty-slot check with boxActivation, gated on stagedAvailable, and consume X on occupied slots so it no longer falls through to the legacy Release handler at TrainerViewScreenBase.inc:4796.
   - Gen V: Gen5SharedReadOnlySurface.inc:1882. Add X handling for Boxes, with occupancy taken from stagedPokemon().viewBox(box,slot) (as in PokemonFileMenus.inc:117), calling beginImport/exportPokemon with generation 5. Carry keeps priority, because ClassicPackedMove::handleGen5CarryInput already refuses X while carrying.

3. Footers:
   - ClassicPackedMoveOverlay.inc:1283-1288. Occupied: {X Export, Y Move / Hold Multi}. Empty: {X Import, Y Hold Multi}. Choose the label with boxXLabel, so the X hint is dropped when the staged editor or file capability is missing.
   - Apply the same change to the Gen V Boxes navbar (Gen5SharedReadOnlySurface.inc:2301).
   - Update or remove the stale "X: Add" strings in TrainerViewScreenCompositeOverlay.cpp:312, TrainerViewScreenBase.inc:5340 and Gen1PokemonEditorOverlayUXCleanup2.inc:1205.

4. Help (ClassicHelpPresentation.h:36-37):
   - Change the Actions row to "A   Actions: View, Edit, Move, Clone, Origin, Export or Release".
   - Change "A   On an empty slot: Add a Pokemon or Import from a file".
   - Add "X   Export the highlighted Pokemon to a file; on an empty slot: Import a file".
   - Keep each row starting with a real key (the Round 6 rule).

5. Storage save pane: this is the owner's call. Either remove X from the Storage pane in all generations (it currently adds in Gen I and Gen III only), or give it the same Import/Export behaviour. Recommendation: Boxes view only, for v1, which also removes today's Gen I/III vs II/IV mismatch.

**Risks.**

Save safety is low risk. Import only opens a modal dialog and stages into app memory; Review and Discard can undo it. Export writes only a new file under sdmc:/switch/PokeBank-NX/pokemon/genN, never replaces a file, and never touches the save or the workspace (PokemonFileMenus.h:8-12). Neither path writes to the source save.

UX risks:
(a) Export on X runs right away with no confirmation. A stray press creates an extra .pkN file each time (clutter, not data loss). This matches the existing Actions -> Export behaviour; a confirmation could be added if the owner wants one.
(b) "Export" next to Review's "X Export Copy" could confuse users, so use distinct labels.
(c) The owner previously signed off on (marked PASS) X = Add on empty slots ("PR #280 parity", Gen II "accepted Gen I" language at Gen2SharedSurfaceParity.inc:255-256). This change deliberately reverses that rule, and the owner's previous hardware passes for X Add will no longer apply.
(d) On Gen I/II, Import goes to the box's first empty slot, not the highlighted cell (packed lists), so X on a non-first empty cell will put the Pokemon somewhere other than the cursor. The status line already reports the real slot.
(e) Gen IV occupied X currently falls through to the legacy Release handler, which is blocked only by requireMutableWorkspace. Consuming X in the Gen IV surface makes that safer.
(f) Japanese Gen I/II saves (gate closed) have no staged editor. Gate X off when the capability is false so the footer does not show X there.
(g) X during carry, multi-select or Y-pending is still taken first by ClassicPackedMove (lines 1052, 1129, 1157) and must keep that priority.

**Open questions.**

Decisions for the owner:
1. Should X Import/Export also apply on the Storage save pane, or only in Boxes? Today the Storage pane differs: Gen I/III add, Gen II is inert, Gen IV falls through.
2. Should Export on X ask for confirmation, or run immediately like the Actions row?
3. Footer wording: "Import"/"Export" or "Import File"/"Export File", to avoid clashing with Review's "Export Copy".
4. Should Party slots get X Export too? The owner said "in the box", and today Party Export exists only in the Actions sheet (ClassicPartyActions.inc:32,105-108).
5. Japanese Gen I/II with the gate closed: should X be inert there? Today Gen II's GSC fallback opens Actions on X.

Needs hardware confirmation:
- What Gen IV currently shows for X on an occupied slot (fall-through to the legacy Release refusal).
- That the file dialog opened from a box X press draws over each generation's box grid with no leftover footer.

**Files to change:** `include/UI/SharedPokemonEditorContract.h`, `include/UI/ClassicHelpPresentation.h`, `include/UI/ClassicBrowseFooter.h (comment only: 'Gen I-III Boxes: X Add')`, `src/UI/ClassicPackedMoveOverlay.inc`, `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc`, `src/UI/Gen2SharedSurfaceParity.inc`, `src/UI/TrainerViewScreenGSCOverlay.inc`, `src/UI/Gen3SharedPokemonSurface.inc`, `src/UI/Gen4SharedPokemonSurface.inc`, `src/UI/Gen5SharedReadOnlySurface.inc`, `src/UI/TrainerViewScreenCompositeOverlay.cpp`, `src/UI/TrainerViewScreenBase.inc`

**Existing tests affected:**
- tests/test_shared_pokemon_editor_contract.cpp:110-113 (boxActivation Add/None truth table; must change deliberately to ImportFile/ExportFile)
- tests/test_gen3_hardware_surface_contract.cpp:33-40 (static_asserts of boxActivation, which state that X on an occupied slot does nothing)
- tests/test_gen4_hardware_surface_contract.cpp:66-73 (same static_asserts) and :486 (source-scan for 'A: Actions  |  X: Add  |  B: Back')
- tests/test_gsc_editor_surface_contract.cpp:136-141 (source-scan for BoxActivation::Add, 'if (down & HidNpadButton_X) return true', and composite '{"X", "Add"}')
- tests/test_classic_help_accuracy.cpp (Page::Pokemon rows; new X row must map to a handler)
- tests/test_footer_help_handlers.py (checks that each footer glyph has a handler reading HidNpadButton_X; the Gen V Boxes footer gains X)
- tests/test_qa_cross_gen_contract.py and tests/test_gen5_readonly_ui_contract.py (Gen V box footer / cross-gen footer scans; check they still match after the edit)
- tests/test_gen12_packed_move.cpp / tests/test_classic_packed_move_recovery_contract.cpp / tests/test_gen4_storage_carry.cpp (scan ClassicPackedMoveOverlay footer and X refusal while carrying; X-while-carrying must stay refused)

**New tests:**
- Shared contract truth table: boxActivation for every combination of occupied, A, X and caps. Empty+X+canImport -> ImportFile. Occupied+X+canExport -> ExportFile. No capability -> None. A always -> Actions. X never returns Add.
- boxXLabel: Import/Export/nullptr matches boxActivation for every input, so the footer can never show X while X does nothing.
- Per-generation source or behaviour contract (Gen I-V): the box X branch calls PokemonFileMenus::beginImport/exportPokemon with the correct generation number and never beginCreate/ux2BeginAdd. Gen IV consumes X on occupied slots (no fall-through to the legacy Release handler).
- Gen V Boxes: X on an occupied slot exports and X on an empty slot imports. X is still refused while carrying (handleGen5CarryInput).
- Footer parity test: the Gen I-V box footers name X Import/Export using the same helper, in the same position (after A, before Y).
- Help accuracy: the Page::Pokemon X row exists for gen 1-5, and Add is still described under A on empty slots.
- Regression: the 'Add Pokemon' row is still first in the empty-slot Actions sheet in Gen I-V (Create still reachable).

**Skeptic corrections.**

The core finding holds: this is a new request (3) plus a cross-generation inconsistency (2), not a bug. I re-opened every cited line and they match.
- The shared rule is at include/UI/SharedPokemonEditorContract.h:154-162.
- Gen I is at Gen1PokemonEditorOverlayUXCleanup2.inc:1263-1267. It is reached through Cleanup3:965-980 -> handleInputUXCleanup2, which is wired by the macro at TrainerViewScreenCompositeOverlay.cpp:88.
- Gen II is at Gen2SharedSurfaceParity.inc:258-279.
- Gen III is at Gen3SharedPokemonSurface.inc:2713-2728.
- Gen IV is at Gen4SharedPokemonSurface.inc:2410-2415.
- Gen V is at Gen5SharedReadOnlySurface.inc:1857-1882; X is never read there.
- The footer is at ClassicPackedMoveOverlay.inc:1283-1288.
- Help is at ClassicHelpPresentation.h:35-41.
- PokemonFileMenus signatures are in PokemonFileMenus.h:19-24.
- The dispatch order is at TrainerViewScreenCompositeOverlay.cpp:387-458. PokemonFileDialog::handleInput runs first, then Gen5, then ClassicPackedMove, then Gen4, Gen3, the Gen1/Gen2 release handlers, Gen2 final, Gen1 UX and the GSC overlay.

Things to correct or tighten:

1. A test that would break is missing from the list. The behavioural host test tests/test_ui_editor_input_host.cpp:468 (gen3CreatePokerus) presses X on an empty Gen III box slot to open Create. Lines 678-681 (the gen3 no-state-across-screens test) press X on an empty slot and expect "X opens a fresh Create draft". Both fail once X becomes Import, because X would open the modal PokemonFileDialog instead. They need to reach Create through A -> Actions -> Add. They are host-run behaviour tests, not source scans, so they must be on the list.

2. Gen III has a fall-through the report missed. It names Gen IV's occupied-X fall-through to the legacy Release handler (TrainerViewScreenBase.inc:4796-4797), but Gen III has the same path. Gen3SharedPokemonSurface.inc:2729 returns false for occupied X, and that input reaches the same legacy X = release-confirm block. It is safe today for the same reason. Gen I-V sources open as RetroArchLegacy or ExternalLegacy (UI.cpp:678-679, 785, 854, 941). SourceMutationPolicy.h:30-33 only allows BackupOrStaged and AppOwnedStorage, so requireMutableWorkspace() refuses.

3. The status shown on that refused path does not need hardware to identify. TrainerViewScreenBase.h:360-363 posts "<provider> source is read-only. Editing this file is disabled." The report lists it as hardware-unconfirmed. What remains to confirm on hardware is only whether that path is actually reached, not the text.

4. The Gen II fallback condition is wrong. TrainerViewScreenGSCOverlay.inc:922 gates the X -> openPokemonActions fallback on validatedGSC (line 916: GSC group and isGSCSource), not on the Japanese gate. It fires whenever the parity path did not consume X, i.e. canEditStagedCopy is false because stagedEditor is null (Gen2StagedAccess.h:11-13). The claim that this is "a Japanese save behind the gate" was not verified. The PokemonFileMenus.h:13 comment only says Japanese Gen I/II saves have no staged editor while the gate is closed.

5. The "stale X: Add strings" cleanup must stay narrow. "X Add" also means commit/Keep inside the Create draft editors:
   - Gen1PokemonEditorFoundationHardwareFix.inc:351
   - Gen1PokemonEditorOverlayUXCleanup3.inc:748
   - Gen2SharedPokemonSurface.inc:561
   - Gen2PokemonEditorFoundation.inc:1093
   - Gen3SharedPokemonSurface.inc:2396
   - Gen2WorkspaceLayers.h ("X", create ? "Add" : "Keep")
   Tests pin these: test_gsc_editor_surface_contract.cpp:124, test_gen2_workspace_layers.cpp:127, test_gen12_review_parity.cpp:163-164. They are Create-editor bindings, not box bindings, and must not change. Only the box-grid strings change: ClassicPackedMoveOverlay.inc:1287, TrainerViewScreenBase.inc:5340, TrainerViewScreenCompositeOverlay.cpp:312, Gen1PokemonEditorOverlayUXCleanup2.inc:1205, plus the dead Gen1PokemonEditorOverlayUX.inc:1139/1170.

6. Gen IV entry semantics change. Today X calls beginActions, sets actionRow=0 and then beginCreate (Gen4SharedPokemonSurface.inc:2411-2414), so the Actions sheet sits under Create. A direct beginImport skips that. The proposal is fine, but B-to-return behaviour changes and should be tested.

7. Gen V target. Gen V's existing Import/Export uses the state-machine target, state.target() at Gen5SharedReadOnlySurface.inc:1677-1683. The new X handler must take box and slot from selectedBoxIndex/selectedItemIndex, as the investigation says. Capability gating should reuse Gen5SharedScreenState::availableActions (include/Integration/Gen5/Gen5SharedScreenState.h:84) so the footer and the sheet cannot disagree.

8. Gen I side note. Cleanup2:1268-1271 also binds Y on an occupied slot to RemoveConfirm, but ClassicPackedMove consumes Y first in the dispatch. Nothing changes here, but a new Gen I X branch must keep the early-return order: X is checked before Y in that function.

Everything else (classification, the shared owner being SharedPokemonEditorContract + ClassicBrowseFooter + ClassicHelpPresentation + PokemonFileMenus, and save safety) is confirmed. Import only stages into memory and export only writes a new file (PokemonFileMenus.inc:183-209).

**Missed generations or paths.**

Missed tests:
- tests/test_ui_editor_input_host.cpp:468 and :678-681. Behavioural Gen III host tests that press X on an empty box slot to open Create. They will fail.

Missed path:
- Gen III occupied-slot X also falls through (Gen3SharedPokemonSurface.inc:2729 returns false) to the legacy release-confirm at TrainerViewScreenBase.inc:4796-4797. It is blocked only by requireMutableWorkspace (TrainerViewScreenBase.h:366, SourceMutationPolicy.h:30-33), the same as Gen IV. The fix should consume X on occupied slots in both Gen III and Gen IV.

Wrong condition:
- The Gen II fallback X -> Actions (TrainerViewScreenGSCOverlay.inc:937) is gated on validatedGSC (line 916/922) with a null staged editor, not specifically on the Japanese gate.

Bindings that must not change:
- Create-editor X Add/Keep bindings that share the label but are not box bindings: Gen1PokemonEditorFoundationHardwareFix.inc:351, Gen1PokemonEditorOverlayUXCleanup3.inc:748, Gen2SharedPokemonSurface.inc:561, Gen2PokemonEditorFoundation.inc:1093, Gen3SharedPokemonSurface.inc:2396, Gen2WorkspaceLayers.h. Pinned by test_gsc_editor_surface_contract.cpp:124, test_gen2_workspace_layers.cpp:127 and test_gen12_review_parity.cpp:163.

Dead code:
- Gen1PokemonEditorOverlayUX.inc:1139/1170 still carries "X: Add" text. Remove it or leave it, but do not let source-scan tests match it.

Not verified (needs reading or hardware):
- What X does on the Storage save pane for Gen II and Gen IV after their surfaces return without consuming it (the legacy storage handlers).

</details>

### QUIT — Quit dialog (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** This is not specific to any generation. The dialog is the app-level Product Home "leave decision" in SaveSelectScreen, and it opens before any Gen I-V save session exists, so Gen I, II, III, IV and V all see exactly the same dialog. The same overlay is also used for "Launch <game>?" (LeaveKind::LaunchTitle / LaunchLegacyInstance), so any change applies to the Launch dialog too. The editor's staged-change exit dialogs are separate and are not part of this complaint: the Gen I-IV four-choice panel (SharedEditorChrome.h:161-182, A Review / B Continue Editing / X Export Copy / Y Discard & Exit) and the Gen V and other two-button forms (SaveConfirmDialog.cpp:39-58, 81-140). They need no change.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Ways to open it: B at the Home root (src/UI/SaveSelectScreen.cpp:4502-4504) or Options > Exit (SaveSelectScreen.cpp:4280-4281). Both call openLeaveDecision(LeaveKind::Quit), which sets overlay = Overlay::QuitConfirm (SaveSelectScreen.cpp:2147-2153).

How it is drawn (SaveSelectScreen.cpp:5313-5344):
- Three glyph buttons, left to right:
  - `drawGlyphButton(..., "B", "Back")` at :5339
  - `drawGlyphButton(..., "A", "Stay")` at :5340
  - `drawGlyphButton(..., "Y", quitting ? "Quit" : "Launch", Colors::Accent, Colors::White)` at :5341-5342
- Only the Y button gets the Accent fill with white text, which is the "highlighted Y exit option" the owner saw. B and A use the default Colors::PanelAlt (include/UI/ScreenChrome.h:392-409).
- A footer that repeats the buttons: `SharedEditorChrome::drawFooter(fb, confirmFooter("Y", quitting ? "Quit" : "Launch", "A", "Stay"))` at :5343-5344. That renders "B Back | A Stay | Y Quit" (include/UI/SharedEditorPresentation.h:116-124).
- There is no cursor or focus state. The dialog is driven only by button presses, so the Accent fill only looks like a pre-selected default; nothing is actually focused.

Input (SaveSelectScreen.cpp:3499-3523, using include/UI/HomeQuitConfirm.h:26-32 decide()):
- A or B -> Decision::Stay, which restores leaveReturnOverlay.
- Y alone -> Decision::Quit, which sets appExitRequested.
- A+Y or B+Y -> Stay.
So "Back" (B) and "Stay" (A) produce the identical Decision::Stay, which confirms the owner's point that they are the same thing under two names.

Touch: every drawGlyphButton registers its own touch hit (ScreenChrome.h:395-397). Taps on the dialog buttons therefore keep working without the footer.

**Root cause.**

This is a design choice, not a defect. The R2-LAUNCH/P0 #320 implementation deliberately gave the dialog three buttons, Y styled as the primary action, and the shared confirm footer, and contract tests freeze that wording:
- SaveSelectScreen.cpp:5338-5344 draws the redundant "A Stay" button next to "B Back", even though HomeQuitConfirm.h:26-32 maps A and B to the same Decision::Stay.
- SaveSelectScreen.cpp:5342 passes Colors::Accent/White to the Y button only. That is the highlight.
- SaveSelectScreen.cpp:5343-5344 adds a confirmFooter that only repeats the three on-dialog buttons.
- The owner's "cancel" is the button labelled "Back" (B).

There is a related inconsistency across confirm dialogs:
- The Backup delete confirm uses "B Cancel / Y Delete" (src/UI/BackupSelectionScreen.cpp:424-427).
- The Release confirm uses "B Cancel" (TrainerViewScreenBase.inc:5376-5377).
- The quit dialog alone says "Back" plus "Stay".

**Shared owner.**

The Product Home leave-decision overlay renderer in src/UI/SaveSelectScreen.cpp (the Overlay::QuitConfirm draw block, around lines 5313-5344), with button order and labels described in include/UI/LeaveDecisionLayout.h:8. The input contract stays in include/UI/HomeQuitConfirm.h, which does not need to change. The dialog is app-level, so all generations, and the Launch variant, pick up the fix from this one place.

**Proposed fix.**

This is a small, draw-only change in SaveSelectScreen.cpp's QuitConfirm block. Input semantics do not change.

1. Remove the redundant button. Draw two buttons with G::buttonRect(layout, i, 2):
   - `drawGlyphButton(fb, cancel..., "B", "Cancel")` on the left
   - `drawGlyphButton(fb, commit..., "Y", quitting ? "Quit" : "Launch")` on the right
   - Use "Cancel" to match the owner's wording and the Backup-delete and Release confirms.
   - Keep HomeQuit::decide() unchanged, so A still maps to Stay without a label. A can never quit or launch, and a mashed A+Y still stays. This is the same pattern as BackupSelectionScreen, where A also cancels.

2. Remove the highlight. Drop the `Colors::Accent, Colors::White` arguments so Y uses the default PanelAlt fill like Cancel.
   - With no cursor, no button should look pre-selected.
   - The safe action (Cancel) then sits first, on the left, which gives the sensible default.
   - If the owner still wants the committing action to stand out, an outline-only accent would avoid a "selected" look; that is the owner's call.

3. Remove the redundant footer. Delete the confirmFooter call (:5343-5344). One caveat matters here:
   - The dim rect at :5332-5333 stops at `fb.getHeight() - kNavBarH`.
   - The covered Home footer ("... B Quit", :5304-5305), or the classic browser's footer, would then stay visible and undimmed, though not tappable after beginTopTouchLayer at :5312.
   - Either extend the dim over the full screen height or draw an empty bar with drawNavBar(fb, std::string()).
   - The empty bar is the better option because it keeps the "frame sits above the footer" geometry. The Bag Remove dialog already uses this footer-less pattern (TrainerViewScreenBase.inc:5242 `instructions.clear(); // dialog owns its A/B glyph buttons`).

4. Update the comments that describe the old layout: LeaveDecisionLayout.h:8, HomeQuitConfirm.h:7-8 and 34-35 (the touch comment that refers to footer badges should say the dialog buttons are the touch targets), and SaveSelectScreen.cpp:5315-5318.

Staged-change safety is not affected:
- Quit is only reachable from Product Home, where no editor session is open. Editor sessions with staged edits leave through the staged exit panels, and those are untouched.
- appExitRequested is still set at exactly one place, the Y branch at SaveSelectScreen.cpp:3519.

**Risks.**

- Save safety: low. Only drawing changes. decide() and the single appExitRequested and requestGameLaunch sites are unchanged, and the dialog only opens from Home, where no staged session exists. Staged-change exit panels are not touched.
- Owner-marked PASS: the P0 #320 behaviour "only Y quits, A/B stay, A+Y stays" is kept exactly. The Launch dialog also changes from "B Back / A Stay / Y Launch" to "B Cancel / Y Launch", so the CG07 Launch-menu hardware row needs a re-check.
- Footer removal: if the dim is not extended or an empty bar is not drawn, the covered Home footer shows "B Quit" under the dialog. It cannot be tapped, but it would look confusing.
- Unlabelled A: with A still cancelling but no longer shown, players may not discover it. That is acceptable because it can only cancel.

**Open questions.**

1. Should the A button still cancel without a label (recommended, safest), or do nothing (Decision::None)? Making A do nothing would require changing HomeQuitConfirm.h and the round-1 regression gate sentinels.
2. Should the label read "Cancel" (owner's word, matches the Backup delete confirm) or stay "Back"?
3. Should the Y Quit/Launch button be fully neutral, or keep a non-filled accent (outline) to mark it as the committing action?
4. Should the Launch dialog follow the same two-button form? It shares the code path, so yes by default.
5. Hardware check needed: after removing the footer, confirm the bottom bar looks right on both Product Home and the classic artwork browser, and that taps on the two buttons still work.

**Files to change:** `src/UI/SaveSelectScreen.cpp`, `include/UI/LeaveDecisionLayout.h`, `include/UI/HomeQuitConfirm.h`, `tests/test_game_hub_contract.py`, `tests/test_leave_decision_layout.cpp`, `docs/hardware/RETEST_ROUND2_TEMPLATE.md`

**Existing tests affected:**
- tests/test_game_hub_contract.py:807-820 pins exact strings that must be updated on purpose: 'drawGlyphButton(fb, back.x, ..., "B", "Back")', '"A", "Stay"', 'quitting ? "Quit" : "Launch", Colors::Accent, Colors::White);' and the confirmFooter(..."A", "Stay") call. Replace them with the two-button Cancel/Quit form, no Accent, and no confirmFooter. Keep lines 800-806, which require that only Y quits or launches and that appExitRequested and requestGameLaunch each have exactly one site.
- tests/test_leave_decision_layout.cpp:4 and 25-30 check three buttons from buttonRect(layout, i, 3). Change this to two buttons with buttonRect(layout, i, 2), each at least 150 wide, inside the frame and not overlapping. The decide() asserts at :69-70 stay unchanged.
- tests/test_home_input_guard.cpp:45-52, together with the test_hw_round1_regression_gate.py:48/579 and test_qa_hw_round1_gate_mutations.py:46 sentinels, pin A/B -> Stay. They stay unchanged, which is intended because A must still never quit.
- docs/hardware/RETEST_ROUND2_TEMPLATE.md:62 and RETEST_e10f307f.md:64 describe 'B Back / A Stay / Y Launch'. The template wording needs updating; the dated historical RETEST docs should be left as they are.

**New tests:**
- Extend test_game_hub_contract.py:
  - The QuitConfirm draw block contains exactly two drawGlyphButton calls (B "Cancel", Y Quit/Launch).
  - No "Stay" label appears.
  - No Colors::Accent appears in the block.
  - No confirmFooter or drawFooter call appears in the block.
  - An empty nav bar or a full-height dim covers the Home footer.
- Add to test_leave_decision_layout.cpp: buttonRect(layout, 0/1, 2) fits inside the LaunchConfirm frame, and the longest label ("Launch" / "Quit" with glyph) fits each button using the bundled font measurement already used in that test.
- Keep the existing guard that HomeQuit::decide(kButtonA) == Stay and add a comment that it is now unlabelled. If a host touch test exists for g_touchGlyphHits, add a case where tapping the Cancel rect maps to B and tapping the Quit rect maps to Y, with the footer absent.

**Skeptic corrections.**

The core diagnosis is correct. I checked every cited draw and input line in /tmp/claude-0/-home-user-PokeBank-NX/e3d26a85-4f48-5ea5-b234-ef4545c1ae50/scratchpad/wmain at HEAD 67b1b6a0:
- SaveSelectScreen.cpp:5337-5344 draws the three buttons B Back / A Stay / Y Quit-or-Launch. Only Y gets `Colors::Accent, Colors::White` (:5342), and the confirmFooter is drawn at :5343-5344.
- HomeQuitConfirm.h:26-32 maps both A and B to Decision::Stay.
- There is exactly one appExitRequested site (:3519).
- The classification "3 Newly requested behavior + 2 Cross-generation UI inconsistency" is right, and the dialog is app-level, so it is not tied to any generation. These parts of the investigation need correcting:

1. WRONG ENTRY PATH: "Options > Exit (SaveSelectScreen.cpp:4280-4281)" cannot be reached.
   - Nothing in src/ or include/ ever assigns `overlay = Overlay::Options`.
   - The only references are the input handler (:4271), the B close (:3548), touch index updates (:3645, :3677) and the draw block (:6182-6204).
   - `git log -S"overlay = Overlay::Options"` shows the last opener was removed in 9875d407 ("implement approved PokeBank NX product home").
   - So the only live way to quit is B at the Home root (:4501-4505). The Launch variant is still opened by ZR (:2222) and by the legacy instance A (:2142).
   - Quick Options (Theme / "Exit PokeBank NX" / "Cancel") is dead code. It is not the owner's "menu with cancel".

2. MISSED PINNING TEST: tests/test_game_hub_contract.py:846-855 splits all 20 Home modal branches, QuitConfirm included. It requires each branch to contain `drawNavBar(`, `drawProductHelpOverlay(`, `drawPickerFooter(` or `drawFooter(` ("must draw its own footer above the covered Home footer").
   - Simply deleting the confirmFooter call fails this test.
   - The proposed `drawNavBar(fb, std::string())` passes it, because the substring matches. That makes the empty bar mandatory, not just "the better option".
   - The comment at :836 ("every modal draws its OWN footer") then needs rewording.

3. TOUCH MECHANICS (proposal is still OK, but the stated reason is incomplete):
   - drawGlyphButton (ScreenChrome.h:392-409) pushes each hit into both g_touchGlyphAccum and g_touchGlyphHits.
   - drawNavHints (ScreenChrome.h:624-627) does `g_touchGlyphHits = g_touchGlyphAccum` and clears g_navHits.
   - beginTopTouchLayer (:461-467) already cleared the covered layer's targets.
   - So an empty nav bar republishes exactly the two dialog buttons and no nav badges, and a tap on Cancel/Quit still maps to B/Y through the touch-mapping code at ScreenChrome.h:518-523. Taps behave correctly.
   - With no footer call at all, the hits would still work, but the covered Home footer's pixels would show undimmed, as the investigator noted.

4. MISQUOTE AND INCONSISTENCY CLAIMS:
   - TrainerViewScreenBase.inc:5242 actually reads `instructions.clear(); // The explicit Bag Remove dialog owns its A/B glyph buttons.`
   - BackupSelectionScreen.cpp:424-427 is NOT a clean "B Cancel / Y Delete" model. It ALSO draws the redundant confirmFooter("Y","Delete","A","Cancel"), so its footer reads "B Back | A Cancel | Y Delete". It also fills Y with Colors::Red/White. That dialog has the same footer-redundancy and highlighted-Y pattern the owner objected to.
   - Making Quit neutral while Delete stays red is defensible (red means destructive), but it should be a stated choice.
   - The root of the "B Back" wording is the shared confirmFooter (SharedEditorPresentation.h:116-124 always pushes "B","Back"). That differs from kCancelLabel = "Cancel" (:89), which pickerFooter uses. Relabelling only the Quit button to "Cancel" leaves every other confirmFooter user saying "B Back". Those users are SaveSelectScreen.cpp:6100 and :6180, BackupSelectionScreen.cpp:424, StoragePanel.cpp:535, SharedPokemonEditorSurface.inc:1286, Gen3SharedPokemonSurface.inc:2429, Gen1PokemonEditorOverlayUX.inc:1108/1115, SharedBagTopUp.h:148 and Gen2WorkspaceLayers.h:136.

5. HomeQuitConfirm.h:34-35 is stale even today: it says the footer badges are "Y Quit, B Stay", but the footer actually shows "B Back, A Stay". The investigator flagged this comment, and it is worse than described.

The proposed fix itself is sound and safe:
- It is draw-only, and decide() is unchanged, so A/B still Stay and A+Y still Stays.
- appExitRequested and requestGameLaunch each keep exactly one site (test_game_hub_contract.py:800-806).
- No editor session or staged edits exist at Home.
- Required pinned updates: test_game_hub_contract.py:807-820 (exact strings) and :846-855 (must keep a nav-bar call); test_leave_decision_layout.cpp:1-6 comment and :25-30 (three-button loop); LeaveDecisionLayout.h:8; HomeQuitConfirm.h:7-8, 34-35; RETEST_ROUND2_TEMPLATE.md:62.
- test_home_input_guard.cpp:40-55 and the round-1 gate sentinels (test_hw_round1_regression_gate.py:48 and :579, test_qa_hw_round1_gate_mutations.py:46) stay untouched.

**Missed generations or paths.**

1. A Gen I-V-only "Stay" next to "Cancel" that was not mentioned. The Bank (Storage) exit prompt shows the same Stay/Cancel duplication the owner described:
   - src/UI/Panels/StoragePanel.cpp:671 has menu items {"Save & Exit", "Discard changes", "Cancel"}.
   - :679 is a footer `{"B", "Stay"}` that is drawn only for classic Gen I-V sessions (:673-679). TrainerViewScreenBase.inc:5374 carries the same "B: Stay" text.
   - It is the only other place in the app that says "Stay", and it is the only one where a "Cancel" option and "Stay" really sit in the same menu.
   - It is not the app quit dialog, but it should be relabelled "B: Cancel" in the same change for consistency. It affects Gen I, II, III, IV and V; Gen VI+ keeps its original presentation.

2. Dead Quick Options overlay. SaveSelectScreen.cpp:4271-4287 and :6182-6204 are unreachable since 9875d407. Remove it or note it, so nobody "fixes" the Options > Exit path, and so the claim that the dialog has two entry points gets corrected.

3. Sibling confirm dialogs with the same redundant-footer and highlighted-Y pattern:
   - BackupSelectionScreen.cpp:424-427: buttons plus a footer "B Back | A Cancel | Y Delete", with Y filled Red.
   - Home's EmulatorBackupDeleteConfirm (:6100) and RecoveryRemoveConfirm (:6180) also say "A Cancel", but they have NO glyph buttons. There the footer is the only touch target, so it must stay.
   - Any rule of the form "remove the footer when the dialog has its own buttons" has to be applied per dialog, not by editing confirmFooter globally.

4. Launch variant: ZR at :2222 and legacy instance A at :2142 share the same draw block, so they inherit the change. The CG07 Launch row (RETEST_ROUND2_TEMPLATE.md:62) needs a hardware re-check.

5. Interpretation risk. The Quit dialog has no "Cancel" label; its labels are Back / Stay / Quit. The owner's "we have cancel in that menu" is therefore most likely the B "Back" button. Relabelling Back to "Cancel" matches the owner's wording, but confirmFooter's "B Back" stays elsewhere unless that is changed deliberately.

</details>

### SPRITE — Species picker: one sprite with shiny preview (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** All of I, II, III, IV and V. Every generation's species picker calls the same SharedSpeciesPicker::drawContent, so all of them show two side-by-side sprites today: Gen I at src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:571, Gen II at src/UI/Gen2PokemonPickerOverlay.inc:165, Gen III at src/UI/Gen3SharedPokemonSurface.inc:2440, and Gen IV and V through src/UI/SharedPokemonEditorSurface.inc:854. Gen IV includes that file at src/UI/Gen4SharedPokemonSurface.inc:51 and Gen V at src/UI/Gen5SharedReadOnlySurface.inc:45. This is a design request, not a broken generation. Every generation already has the state the request needs: a previewShiny value that starts from the record and that Y toggles. Only the drawing needs to change.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

The preview column of include/UI/SharedSpeciesPicker.h::drawContent draws two labels and two artworks for the highlighted species.
- Lines 443-444 draw the captions "NORMAL" at previewX and "SHINY" at previewX+190. The active one is coloured using previewShiny.
- In the normal view, lines 460-465 draw getSprite(preview, form, false) at previewX (165x190 box) and getSprite(preview, form, true) at previewX+188. Each has its own "No sprite" / "No shiny sprite" fallback.
- In the search view (NEW-05), lines 446-452 draw the same pair at kSearchArtW x kSearchArtH (150x120), at previewX and previewX+190.
- Line 473-475 prints a "Shiny: Yes/No" line.
- The list-row icons at line 414 already follow the state with a single getIconSprite(species, form, previewShiny), so the rows react to Y but the large preview does not change.

Each owner already starts previewShiny from the record and toggles it with Y:
- Gen I: ux3OpenSpeciesPicker sets pickerPreviewShiny = shiny (Gen1PokemonEditorOverlayUXCleanup3.inc:175), called with the record/draft shiny at :344 and at Gen1PokemonEditorOverlayFoundation.inc:228. Y toggles at :266. The choice is applied to the draft at :206-207 and :228.
- Gen II: model.openSpecies(working.species, working.shiny) at Gen2PokemonPickerOverlay.inc:59 (Gen2PokemonPickerModel.h:45-46). Y toggles at :428. The choice is applied through applySpeciesAppearance at :77-78.
- Gen III: speciesPreviewShiny = p.shiny at Gen3SharedPokemonSurface.inc:855. Y toggles at :1339-1341. setShiny is called on A at :1345.
- Gen IV: speciesPreviewShiny = working->shiny() at Gen4SharedPokemonSurface.inc:579-580. Y toggles in shared code at SharedPokemonEditorSurface.inc:486-488. Applied at Gen4:758-759.
- Gen V: speciesPreviewShiny = p.shiny() at Gen5SharedReadOnlySurface.inc:396. Same shared Y toggle. Applied or staged at :744-763 and :795-802.

The footer already advertises "Y Normal/Shiny" (SharedPokemonEditorSurface.inc:868; hint text at SharedSpeciesPicker.h:330/338/341).

**Root cause.**

Nothing is malfunctioning. The two-sprite preview is a deliberate layout inherited from the "accepted Gen I visual picker" (comment at include/UI/SharedSpeciesPicker.h:315-317). The large preview ignores previewShiny for the artwork itself: it always asks for both getSprite(preview, form, false) and getSprite(preview, form, true) (SharedSpeciesPicker.h:447-452 when searching, :460-465 otherwise). previewShiny only changes the caption colour at :443-444 and the text at :474.

There is also a side effect in Gen II that a single sprite fixes. previewUnownForm(state, model.previewShiny, ...) (Gen2PokemonPickerOverlay.inc:162-163) returns the Unown letter for the shiny state you selected, but :447/:460 draw the non-shiny artwork with that same form. For Unown in shiny mode, the "NORMAL" image therefore shows a letter (I or V) that a non-shiny Unown would not have.

**Shared owner.**

include/UI/SharedSpeciesPicker.h, function drawContent (preview column, lines ~436-477). The owners need no changes because all five already pass the record-initialised, Y-toggled previewShiny to drawContent.

**Proposed fix.**

Make the change only in SharedSpeciesPicker::drawContent.
1. Replace the two NORMAL/SHINY captions (:443-444) with one state caption: "NORMAL" in Colors::Accent or "SHINY ★" in Colors::ShinyStar, depending on previewShiny. Alternatively, drop the caption and keep the existing "Shiny: Yes/No" line at :473-475.
2. Normal view: draw one artwork with `getSprite(preview, form, previewShiny)`, centred in the preview column, e.g. `fb.drawSpriteStaticContained(previewX + (kPreviewW-<w>)/2, y+152, <w>, 190, ...)`. It can be larger than 165 px now that there is only one. Keep a single fallback: "No shiny sprite" when previewShiny is set and "No sprite" otherwise. Do not silently show the normal artwork in place of a missing shiny one, because the image would then disagree with the Shiny state.
3. Search view (:446-452): draw the same single `getSprite(preview, form, previewShiny)`, centred, at kSearchArtW x kSearchArtH (or wider). Keep kSearchArtY/kSearchArtH so the key-grid spacing asserted in tests/test_species_search_gen15.cpp:211 still holds.
4. Keep the Y-toggle behaviour, the hint strings and footer.yLabel = "Normal/Shiny" unchanged. Y now visibly swaps the artwork, which is what the owner asked for.

The selected species and shiny state are already applied on A in each owner; this change does not touch that. The preview-only Y toggle does not mutate the record until A, which matches today's behaviour.

**Risks.**

Save safety: none. The change is in the renderer only. previewShiny's lifecycle stays the same: it starts from the record, Y toggles it, and A applies it in each owner (Gen1 :206-207/:228, Gen2 :77-78, Gen3 :1345, Gen4 :758-759, Gen5 :744-802). No write paths change.

PASS-marked behaviour that could be affected:
- The species picker layout and the search-mode artwork/key-grid spacing (NEW-05) the owner passed. Keeping kSearchArtY/kSearchArtH and the type-badge row at y+344 avoids moving the grid.
- Gen II Unown native-letter preview, which the owner accepted. The single sprite must keep passing the form from previewUnownForm.
- One small UX change: users can no longer compare normal and shiny side by side without pressing Y. The owner explicitly asked for this.

**Open questions.**

1. Caption: should the preview show a "NORMAL"/"SHINY ★" label, the existing "Shiny: Yes/No" line, or both? Keeping both is redundant.
2. Single-sprite size: enlarge it to use the freed width (e.g. about 260x190), or keep 165x190 centred?
3. Should touch get a way to toggle shiny (e.g. tapping the preview artwork, or a small Normal/Shiny chip), or is Y enough? Today the toggle is Y only, with no touch target in any generation.
4. Gen I has no native shininess. The app treats Gen I shiny as Gen II DV-shiny (Gen I Y toggle at Gen1PokemonEditorOverlayUXCleanup3.inc:266). The owner should confirm the Gen I preview should keep showing shiny artwork.
5. Hardware: someone needs to confirm on the Switch that the single sprite swaps immediately on Y in each of Gen I-V, and that it shows the record's current shiny state when the picker opens.

**Files to change:** `include/UI/SharedSpeciesPicker.h`, `tests/test_gsc_workspace_render.cpp`, `tests/test_gen2_unown_letter.cpp`

**Existing tests affected:**
- tests/test_gsc_workspace_render.cpp:357-366: asserts the shared renderer requests exactly {{dex,false},{dex,true}} for the hovered species. This must change deliberately to {{dex,previewShiny}} (one request). The call passes previewShiny=true, so the expectation becomes {{dex,true}}. Keep the 'Shiny: Yes' text assertion at :365 if that line stays.
- tests/test_gen2_unown_letter.cpp:119-122: a source-string contract that requires 'getSprite(preview, form, false)' and 'getSprite(preview, form, true)' in SharedSpeciesPicker.h. Update it to require 'getSprite(preview, form, previewShiny)' and to assert the two fixed-shiny calls are gone. The getIconSprite(species, form, previewShiny) clause is unchanged.
- tests/test_species_search_gen15.cpp:211: asserts the key grid sits below kSearchArtY+kSearchArtH+30. Unaffected if those constants are kept, but re-run it.
- tests/test_shared_species_picker_surface_contract.cpp, tests/test_gen3_hardware_surface_contract.cpp:153/325, tests/test_gen4_hardware_surface_contract.cpp:504, tests/test_shared_editor_unit.cpp:218: these pin footer 'Normal/Shiny' and the previewShiny plumbing. They should still pass unchanged.

**New tests:**
- Renderer test (extend the mocked SpriteManager harness in tests/test_gsc_workspace_render.cpp): for previewShiny in {false,true} and dex in {1,151,152,251,386,493}, drawContent with and without a Chooser in search mode requests exactly one full-size getSprite for the preview species, with shiny == previewShiny. No 'SHINY' caption is drawn when previewShiny is false, and no 'NORMAL' caption when it is true.
- Missing-artwork test: when getSprite returns nullptr with previewShiny=true, the text 'No shiny sprite' is drawn and the normal sprite is not requested as a fallback.
- Gen II Unown test (tests/test_gen2_unown_letter.cpp): with previewShiny=true the single preview getSprite is (201, form in {8,21}, true), and with false it is (201, previewUnownForm(false), false). This guards the form/shininess mismatch the old NORMAL slot had.
- Layout test: the single preview artwork rect lies inside the preview column [modal.x+kPreviewX, modal.x+kPreviewX+kPreviewW] and does not overlap the type badges at y+344 or the key grid at kKeyGridY in search mode.
- Cross-gen source contract (tests/test_shared_species_picker_surface_contract.cpp): SharedSpeciesPicker.h contains getSprite(preview, form, previewShiny) and no longer contains getSprite(preview, form, false) or getSprite(preview, form, true).

**Skeptic corrections.**

I checked every cited line against the read-only tree. The core finding holds. It is a new behaviour request (3) plus a UI inconsistency (2), and the change belongs in one place. include/UI/SharedSpeciesPicker.h:443-444 draws the fixed "NORMAL" and "SHINY" captions. Lines :447-452 (search view) and :460-465 (normal view) always request getSprite(preview, form, false) and getSprite(preview, form, true). previewShiny only changes the caption colour (:443-444) and the "Shiny: Yes/No" line (:473-475). The list-row icons at :414 already follow previewShiny. I confirmed each owner's previewShiny wiring: Gen I Cleanup3:175 and :266, Gen II Overlay:428, Gen III :855, :1339-1341 and :1345, Gen IV :579-580, shared toggle at SharedPokemonEditorSurface.inc:486-488, Gen V ReadOnlySurface:396. Exactly four call sites reach drawContent: Gen1 Cleanup3:570, Gen2 Overlay:164, Gen3 :2439 and SharedPokemonEditorSurface:853 (used by Gen IV and Gen V). No other generation has its own big-preview path.

Corrections and refinements:
1. The Gen II Unown mismatch happens in both directions, not only in shiny mode. Gen2PokemonPickerOverlay.inc:162-163 works out one form for model.previewShiny (Gen2PokemonPickerModel.h:129-134 applies the hypothetical appearance). SharedSpeciesPicker.h:447/:450 and :460/:463 draw both artworks with that one form. In shiny mode the NORMAL image shows I or V. In non-shiny mode the SHINY image shows whatever letter the non-shiny DVs give, which a shiny Unown can never have (the shiny form can only be I or V; test_gen2_unown_letter.cpp:115). A single getSprite(preview, form, previewShiny) fixes both cases.
2. Centring width. The proposal centres on kPreviewW (446, SharedSpeciesPicker.h:140). drawContent actually uses a local previewW = 353 (:441), and the type badges are centred on that width at :456 and :471. If the art is centred on 446 it will sit off-centre from the badges and the title. Centre it on the same 353 width, or change both widths together on purpose.
3. "Missing shiny art" fallback: SpriteManager::getSprite (src/UI/SpriteManager.cpp:129-173) never falls back from shiny to normal. It only falls back from form to base form, and caches nullptr. The proposed "No shiny sprite" test is therefore valid with a nullptr mock. Note that the Unown form path does fall back to the base species (:159-165).
4. Coverage gap in the cited test. tests/test_gsc_workspace_render.cpp:357-366 only calls drawContent with no Chooser and previewShiny=true. The search branch at :445-458 has no sprite-request assertion at all, so the new test must cover search mode explicitly. The other cited lines are correct: test_gen2_unown_letter.cpp:119-122, test_species_search_gen15.cpp:211, and test_shared_species_picker_surface_contract.cpp:78/81/88 (these pin the plumbing and the footer, not the sprites).
5. Small citation fix: the shared footer yLabel is at SharedPokemonEditorSurface.inc:867, not :868. Gen I (Cleanup3:734), Gen II (Overlay:185) and Gen III (:2459) also set yLabel "Normal/Shiny". All of these should stay as they are.
6. Not a save-safety issue, but a new performance point. Today the shiny and normal art for the hovered species are both decoded on hover, so Y swaps instantly. With one sprite, the first Y press on each species is a cache miss and decodes an HD PNG (SpriteManager.cpp:153-155). That can cause a visible one-frame hitch on hardware. Optional fix: peek or prefetch the other appearance without drawing it. That would put back a second getSprite call, so the new test should then count draws, not getSprite calls. Hardware should confirm whether the swap is instant.
Save safety: confirmed none. The change is render-only, and A still applies previewShiny in each owner.

**Missed generations or paths.**

No live generation or path was missed. One dead path should be noted so nobody edits it by mistake. Legacy Gen I ux2DrawPicker (src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:1054-1088) already draws one non-shiny "HOVER PREVIEW" sprite (:1084). It is unreachable. Cleanup2's drawOverlayUX is renamed to drawOverlayUXCleanup2 (src/UI/TrainerViewScreenCompositeOverlay.cpp:89-91), and nothing calls that name. The live path is Foundation.inc:830 -> drawOverlayUXCleanup3 -> ux3DrawPicker (Cleanup3:561, called at Cleanup3:1062 and FoundationHardwareFix.inc:378). The fix must not touch the dead code, but it is a cleanup candidate. tests/test_qa_cross_gen_contract.py:418-435 has a regex that checks getSprite call arity. It matches only 2-argument calls, so the 3-argument getSprite(preview, form, previewShiny) will not trip it. CG39 (:1960-1996) checks only the list and key-grid constants, so it is unaffected as long as kSearchArt* and keyGridRect stay unchanged. Detail screens (PokemonDetailsModal, the Gen II/Gen I details presentations) and the editor portraits are separate one-sprite paths that already follow shininess. They are out of scope.

</details>

### CREATE-MOVES — Create/species change must not auto-fill moves (P1 request)

- **Classification:** 3 Newly requested behavior, 2 Cross-generation UI inconsistency, 5 Safety concern  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** New Create draft has moves before the user picks any: Gen I (Thunder Shock and Growl, hard-coded) and Gen II (Thunder Shock and Growl, filled from the learnset table). Gen V also has one: it starts with Thunder Shock only. Gen III and Gen IV already start with no moves. Picking a different species fills in that species' level-up moves in all five generations (I, II, III, IV, V), in both Create and Edit. Picking the same species again leaves the moves alone in every generation, so that case is already correct.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Where a new Create draft gets its moves:
- Gen I: include/Integration/Gen1/Gen1StagedPokemonEditor.h:26-35. `BoxPokemonCreate` defaults to `moves{84,45,0,0}, pp{30,40,0,0}` (Thunder Shock and Growl). Every Add path resets the draft with `state.draft = BoxPokemonCreate{}`, for example src/UI/Gen1PokemonEditorOverlayUX.inc:367 and :537, Gen1PokemonEditorOverlayUXCleanup2.inc:187, Gen1PokemonEditorOverlayUXCleanup3.inc:328 and :393, and Gen1PokemonEditorOverlay.cpp:240.
- Gen II: src/UI/Gen2PokemonEditorFoundation.inc:182-184. `defaultCreateRecord` clears the moves. Then beginCreate at :208-210 calls `Rules::seedNativeMoves(state.working, screen.sourceGameId)`, which is defined at include/UI/Gen2PokemonSession.h:30-40. For Pikachu at level 5 this gives Thunder Shock and Growl. The comment at :208 claims Gen III-V do the same. That claim is wrong.
- Gen III: include/UI/Gen3SharedPokemonSession.h:95-108. beginCreate never sets moves, so the record starts with no moves. Already correct.
- Gen IV: src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:310-323. createBoxDraft passes `Pokemon4CreateDefaults`, and that struct has no move fields (include/Pokemon/Pokemon4Mutable.h:22-34), so the draft has no moves. Already correct.
- Gen V: include/Integration/Gen5/Gen5SharedScreenState.h:406 sets `r.moves[0]={kSharedCreateMove,...}`, and :823 defines `kSharedCreateMove=84` (Thunder Shock). It has to: `validateCreate5` at include/Integration/Gen5/Gen5CreateValidation.h:133-137 refuses a request with no first move ("First move required"). The draft is built as a real PK5 in the workspace straight away (Gen5SharedScreenState.h:424, `createInEmptyBox`).

Where picking a species fills in moves. Everything goes through the shared policy in include/UI/SpeciesChangeMoves.h:36-118 (`forSpecies`, `refreshArrays`, `writeEdit`, `setMoves`, `stageMoves`), which calls `PokeBank::Moves::defaultMoves` in include/Pokemon/SpeciesMoveDefaults.h:110-130.
- Gen I, Create and Edit: src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:196-202 (Create, `writeArrays`) and :219-223 (Edit, `writeEdit`), through `ux3SpeciesChangeMoves` at :185-188 and include/UI/Gen1SpeciesChangeMoves.h:16-24. The older overlays do the same at Gen1PokemonEditorOverlayUXCleanup2.inc:273 and :285, Gen1PokemonEditorOverlayUX.inc:396 and :421, and Gen1PokemonEditorOverlay.cpp:269 and :294.
- Gen II: include/UI/Gen2PokemonSession.h:73-97, inside `Session::setSpecies` (`refreshArrays` at :93).
- Gen III: include/UI/Gen3SharedPokemonSession.h:112-138, inside `setSpecies` (`refreshArrays` at :132).
- Gen IV: src/UI/Gen4SharedPokemonSurface.inc:758-768, in `applyGen4Picker` (`setMoves(forSpecies(...))`).
- Gen V Edit: src/UI/Gen5SharedReadOnlySurface.inc:779-791 (`stageMoves<Move,MovePPUps,MovePP>`).
- Gen V Create: include/Integration/Gen5/Gen5SharedScreenState.h:555-559. `recreateCreateSpecies` copies `defaultMoves(...)` into `r.moves` and then rebuilds the PK5 through `validateCreate5`.

On an existing Pokemon (Edit), changing species currently replaces all four moves with the new species' level-up moves. It does not keep the moves the new species can still learn. The H28 PASS wording, "Changing species clears incompatible moves", is met only because every old move is thrown away.

Whether each writer can store a Pokemon with no moves:
- Gen I: yes. `validPokemon` at src/Integration/Gen1/Gen1StagedPokemonEditor.cpp:57-63 only requires an empty slot to have PP 0 and PP Ups 0, and `stageAdd` (:425-470) writes whatever moves it is given.
- Gen II: yes. `stageAddBoxPokemon` (src/Integration/Gen2/Gen2StagedEditor.cpp:1327 onward) only range-checks the moves.
- Gen III: yes. The default Create record has no moves, as the comment at Gen3StagedPokemonEditor.cpp:911 notes.
- Gen IV: yes. Pokemon4Mutable.cpp:663-680 accepts move 0.
- Gen V: Edit accepts move 0 (`validNativeMove5(0)` is true, Gen5MovePP.h:46). Create refuses it: `validateCreate5` requires slot 1 to hold a move and the filled slots to be contiguous (Gen5CreateValidation.h:136-143).

Not found: anything in Gen I-IV that stops Keep or Add when a Pokemon has no moves.

**Root cause.**

This is deliberate behaviour from an earlier fix, not an accident. The R2-BUG-004 policy (include/UI/SpeciesChangeMoves.h:3-13, "On a REAL species change every editor replaces all four moves with the new species' native default moveset") is called from every species-change path listed above. The owner marked it PASS in round 3, and the tests pin it (CG19 at tests/test_qa_cross_gen_contract.py:1300-1365; R2-BUG-004 at tests/test_hw_round1_regression_gate.py:350-353).

The pre-filled Create moves have separate causes:
- Gen I: hard-coded defaults in `BoxPokemonCreate` (Gen1StagedPokemonEditor.h:34), added in commit 2d40753d.
- Gen II: `seedNativeMoves`, called from beginCreate (Gen2PokemonEditorFoundation.inc:210), added in commit 48fbf94b.
- Gen V: `kSharedCreateMove`, needed because `validateCreate5` insists on a first move (Gen5SharedScreenState.h:406 and :823; Gen5CreateValidation.h:136-137).

The owner now wants a blank start and no auto-fill. That contradicts the earlier accepted policy, so this is a new request plus a cross-generation inconsistency: Gen III and IV start blank while Gen I, II and V do not.

**Shared owner.**

include/UI/SpeciesChangeMoves.h, the shared `SpeciesMoveDefaults` species-change move policy. Every Gen I-IV species path already writes through it, and Gen V Edit does too. Gen V Create (Gen5SharedScreenState.h:555-559) calls `defaultMoves` directly and should be moved onto the same policy. A shared "Keep needs at least one move" check should sit next to it, for example in the same header or in SharedPokemonEditorContract.h, so all five generations get the same rule.

**Proposed fix.**

1. Change the shared policy in SpeciesChangeMoves.h so it never fills in level-up moves. Replace `forSpecies` (or add a new `forSpeciesChange`) with a function that returns the move slots to keep after a real species change:
   - Create draft: all four slots empty, PP 0, PP Ups 0.
   - Edit of an existing Pokemon: keep each old move the new species can learn, with its current PP and PP Ups, and clear the rest. Then shift the kept moves up so the empty slots are at the end. "Can learn" uses Pokemon/LearnsetTable.h `isLearnable` (the check Convert.cpp already uses), Integration/Gen3/Gen3LearnsetTable.h for Gen III, and Gen5MoveCompatibility.h for Gen V. If the owner prefers to clear everything on Edit too, it is a one-line policy change.
   Reselecting the same species stays a no-op. The writers (`writeArrays`, `writeEdit`, `setMoves`, `stageMoves`, `refreshArrays`) keep their signatures, so the call sites barely change and CG19 still finds a move writer in every species path.
2. Make new Create drafts blank:
   - Gen I: change Gen1StagedPokemonEditor.h:34 to `moves{}, pp{}`.
   - Gen II: delete the `seedNativeMoves` call at Gen2PokemonEditorFoundation.inc:208-210, and the helper too if nothing else uses it.
   - Gen V: let the pending Create draft exist with no moves. Either `validateCreate5` takes a draft mode that allows four empty slots (all PP and PP Ups 0) while still enforcing contiguous slots and other rules, or the PK5 is built with moves[0]=0 through a separate path. Then delete `r.moves[0]=kSharedCreateMove` at Gen5SharedScreenState.h:406, and in `recreateCreateSpecies` at :555-559 apply the step 1 policy instead of `defaultMoves`.
3. Add a safety gate at Keep/Add: refuse to commit while all four move slots are empty, with a message like "Choose at least one move before Keep". Apply it in all five generations, in Create and in Edit. Without it, a blank draft could be written to a save with no moves. The Gen I-IV writers would accept that, and how the games handle a moveless Pokemon has not been checked in this repo. For Gen V Create, keep the native first-move rule at the final Keep.
4. Update the R2-BUG-004 comments and the H28 description to the new policy: never auto-fill, Create starts blank, Edit keeps learnable moves (or clears all, if the owner picks that).

**Risks.**

1. Save safety: once new drafts are blank, a Pokemon with no moves could be written to a save unless the Keep/Add gate (fix step 3) is in place. The Gen I-IV writers accept it today (Gen1 validPokemon :57-63, Gen2 stageAdd, Gen3 Create, Gen4 setMove). Not checked in this repo: how R/B/Y, G/S/C, RSE/FRLG, DPPt/HGSS and BW/B2W2 handle a moveless Pokemon in battle or the summary screen, and whether the legality checker flags it.
2. Gen V Create builds a real PK5 in the workspace immediately, and `validateCreate5` requires a first move. Loosening that check for the draft stage must not let a moveless PK5 through at Keep.
3. Owner-marked PASS behaviour: R2-BUG-004 ("Species change resets moves", round 3 PASS) and H28 ("Changing species clears incompatible moves") must keep working. No invalid old moves may survive a species change. If Edit keeps learnable moves, the learnability check must be correct per game, or "Invalid" moves come back, which was the original R2-BUG-004 bug.
4. Review rows, the Create summary (Gen12CreateReview.h:24 prints no moves when there are none) and Gen V Review replay order (Move, then PP Ups, then PP) must stay consistent when slots are cleared rather than filled.
5. Several Gen I overlay files (UX, UXCleanup2, UXCleanup3, Overlay.cpp) each carry a species-apply path. Every one must change, or the old filling behaviour stays reachable.

**Open questions.**

1. On an existing Pokemon (Edit), should changing species clear all four moves (strict reading of "leave moves clear"), or keep moves the new species can learn and clear the rest (the literal H28 wording, "clears incompatible moves")? The owner has to choose.
2. Should Keep/Add require at least one move in every generation? Recommended: yes. Gen V's native Create rule already requires one, and the owner's own words ("i want the user to pick the moves") imply it.
3. Hardware: does each game (I-V) accept a Pokemon with zero moves without glitches? This can only be confirmed on a console or emulator. It decides whether the gate is a hard error or only a warning.
4. Does the H28 worksheet entry exist anywhere in the repo? Not found under docs/ or anywhere else in the repo.

**Files to change:** `include/UI/SpeciesChangeMoves.h`, `include/UI/Gen1SpeciesChangeMoves.h`, `include/Integration/Gen1/Gen1StagedPokemonEditor.h`, `src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc`, `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc`, `src/UI/Gen1PokemonEditorOverlayUX.inc`, `src/UI/Gen1PokemonEditorOverlay.cpp`, `src/UI/Gen2PokemonEditorFoundation.inc`, `include/UI/Gen2PokemonSession.h`, `include/UI/Gen3SharedPokemonSession.h`, `src/UI/Gen4SharedPokemonSurface.inc`, `src/UI/Gen5SharedReadOnlySurface.inc`, `include/Integration/Gen5/Gen5SharedScreenState.h`, `include/Integration/Gen5/Gen5CreateValidation.h`, `Keep/Add paths per generation (Gen I overlay keep, Gen II session createRequest/keep, Gen III/IV/V keep) for the at-least-one-move gate`

**Existing tests affected:**
- tests/test_species_change_moves_gen1.cpp (line 89 Create default Pikachu Thunder Shock/Growl; line 105-108 expects Fearow Peck/Growl/Leer after change)
- tests/test_species_change_moves_gen2.cpp (lines 65-74, 91-102 seedNativeMoves -> {84,45,0,0})
- tests/test_species_change_moves_gen3.cpp (lines 147, 170-179, 248-249 expect defaultMoves after change)
- tests/test_species_change_moves_gen4.cpp (line 209, 229-239 expects defaultMoves)
- tests/test_species_change_moves_gen5.cpp (line 140 expects defaultMoves)
- tests/test_species_change_moves_gen15.cpp (lines 140-179, 276: some pin the defaultMoves table itself and can stay as table tests; the parts that pin call-site behaviour must change)
- tests/test_gen5_party_box_parity.cpp (lines 336-343 expect defaults after species change)
- tests/test_hw_round1_regression_gate.py (line 350-353 R2-BUG-004 anchor 'Gen I Pikachu Lv5 = Create default'; line 658 anchor on seedNativeMoves {84,45,0,0})
- tests/test_qa_cross_gen_contract.py CG19 (lines 1300-1365): writer-call detection can stay, but rename/redescribe from 'resets moves' to the new policy
- tests/test_ui_editor_input_host.cpp line 866-900 (Gen V fixture with Thunder Shock; likely unaffected, verify)
- Any Gen V create tests relying on kSharedCreateMove / validateCreate5 'First move required' (test_gen5_create_edit_parity.cpp, Gen5 native create tests) need checking

**New tests:**
- Per generation, I to V, through the real editor or session code: a new Create draft has moves=={0,0,0,0} with PP 0 and PP Ups 0 (Gen I through the BoxPokemonCreate{} reset paths, Gen II beginCreate, Gen III Session::beginCreate, Gen IV createBoxDraft, Gen V beginCreateDraft or sharedCreateDefaults).
- Per generation, Create: changing species on a blank draft leaves all moves empty. Changing species on a draft where the user already chose moves applies the agreed rule (cleared, or only learnable moves kept) and never adds a level-up move the user did not pick.
- Per generation, Edit of an existing Pokemon (H28 regression): a move the new species cannot learn (for example Thunder Shock after Pikachu to Fearow) is removed. A move it can learn keeps its PP and PP Ups if that is the chosen rule. Remaining moves are shifted up, and no default moves appear.
- Reselecting the same species changes nothing (byte-identical draft).
- Keep/Add gate: Gen I-V refuse to commit a Pokemon with no moves, the staged and save bytes stay unchanged, and the user sees the message. Keep with one move succeeds.
- Gen V: rebuilding a pending Create draft for a new species works with a blank moveset, and the final Keep still meets the native first-move and contiguous-slot rules in validateCreate5.
- Contract test (CG19 replacement): no editor UI file calls PokeBank::Moves::defaultMoves or seedNativeMoves outside table tests.

**Skeptic corrections.**

I checked every cited file:line. The root cause is confirmed, and every citation I opened matches the code: Gen1StagedPokemonEditor.h:34 `moves{84,45,0,0}, pp{30,40,0,0}`; Gen2PokemonEditorFoundation.inc:208-210 calls `Rules::seedNativeMoves`, defined at Gen2PokemonSession.h:34-40; Gen2PokemonSession.h:93 is the `refreshArrays` call in setSpecies; Gen3SharedPokemonSession.h:95-108 (beginCreate sets no moves) and :132; Gen4StagedPokemonEditor.cpp:310-323 together with Pokemon4Mutable.h:22-34 (no move fields); Gen4SharedPokemonSurface.inc:758-768; Gen5SharedReadOnlySurface.inc:779-791; Gen5SharedScreenState.h:406, :555-559 and :823; Gen5CreateValidation.h:133-143; SpeciesChangeMoves.h:3-13 and :36-118. Both new-draft prefill and species-change auto-fill are deliberate policy (R2-BUG-004), not an accident.

Corrections and gaps:

1. An owner PASS was missed, and it conflicts directly with the request. tests/test_hw_round1_regression_gate.py:657-658 is entry C14, "Gen II Create starts with native level-up moves (Oct 26 PASS)". It anchors tests/test_species_change_moves_gen2.cpp with `fresh.moves == {84,45,0,0}`. The investigation only called this "line 658 anchor on seedNativeMoves". In fact the owner signed off on the opposite behaviour on Oct 26. The new request supersedes it. Retiring C14 (and the Gen I anchor at :350-353, "Gen I Pikachu Lv5 = Create default") needs an explicit note that the owner reversed the earlier PASS. It should not be a silent edit.

2. Gen I has more draft-reset sites than the list gives. The list misses Gen1PokemonEditorOverlayUX.inc:817, Gen1PokemonEditorOverlayUXCleanup2.inc:511 and :572, Gen1PokemonEditorOverlay.cpp:688, and Gen1PokemonEditorOverlayFoundation.inc:364 (which is #included at TrainerViewScreenCompositeOverlay.cpp:134). The default-member draft initialisers at UX.inc:78, Cleanup2.inc:58 and Overlay.cpp:69 (`BoxPokemonCreate draft{}`) are also missed. Changing the struct default covers all of these. A UI-only reset would not.

3. The Gen I struct-default change spreads into backend fixtures. BoxPokemonCreate is a backend type (Gen1StagedPokemonEditor.h), not UI state. Changing `moves{}` alters the result of `stageAdd(BoxPokemonCreate{})` in tests that the list leaves out:
   - tests/test_gen1_export_transaction.cpp:163-174 expects the review row "Moves: Thunder Shock, Growl".
   - tests/test_qa_matrix_gen12.cpp:1096-1130: creation cell `BoxPokemonCreate{}`.
   - tests/test_gen1_japanese_writer.cpp:233 and :409 use the default create.
   Either update these fixtures to set moves explicitly, or keep the backend default and clear moves in the UI reset path. The struct change is simpler and catches every reset site, but those fixtures must change with it.

4. Gen V "final Keep" is wrong. The fix says "keep the native first-move rule at the final Keep" through validateCreate5. validateCreate5 runs only when the PK5 is first built: createNative5 at Gen5NativeCreate.h:25 (reached through createInEmptyBox) and recreateCreateSpecies at Gen5SharedScreenState.h:565. After that, a pending Create is an ordinary staged edit. Its Keep is keepWithPresentation at Gen5SharedScreenState.h:650-665, and that function gates only on `createNeedsMetLocation()` (:351, :657).
   Field::Move staging (Gen5StagedPokemonRecord.h:422-434) accepts move 0 in any slot, including slot 0 and non-contiguous slots. So even today a user can clear Thunder Shock in a pending Gen V Create and Keep a moveless or gapped PK5. The pre-existing hole is also there for Gen V Edit.
   The right Gen V gate copies the existing met-location pattern: add `createNeedsMove()` beside :351, check it in keepWithPresentation beside :657, and label it in the Gen5SharedReadOnlySurface.inc:371 hint. Loosening validateCreate5 does not do this. For the draft build, a separate draft-mode flag passed to createNative5 is safer than weakening validateCreate5 globally, because createNative5 is the general native creation API.

5. Gen V has two species-rebuild call sites into recreateCreateSpecies: Gen5SharedReadOnlySurface.inc:762 and :888 (the pending.rebuildCreate path). Both route through :555-559, so one fix covers them, but both need tests.

6. There is a shared status string. kMovesRefreshedStatus at SpeciesChangeMoves.h:119 says "Moves reset to the new species' level-up moves" and becomes wrong under the new policy. Also check Gen5SharedReadOnlySurface.inc:790, whose error text reads "Could not reset moves for the new species".

7. The shared-owner claim holds, with one nuance. SpeciesChangeMoves.h is the right owner for the species-change rule. Create-draft defaults are not owned by any shared component: they live in four places (the Gen I struct, the Gen II defaultCreateRecord plus seedNativeMoves, Gen III beginCreate, and Gen V sharedCreateDefaults). The at-least-one-move Keep gate also has no shared home. Each generation's Keep is separate (Gen V keepWithPresentation, Gen IV session, Gen I overlay keep, and so on), so only a shared predicate, for example `SpeciesMoveDefaults::hasAnyMove(moves)`, can be shared. Each generation still has to call it.

8. Open question 1 has a factual basis. In Edit, the current code throws away every old move. So "keep learnable moves" is new logic and needs a per-game learnability source. Gen III has no general isLearnable use in the UI today. The proposal cites Gen3LearnsetTable.h and Gen5MoveCompatibility.h, but I did not verify that they cover every exact game (FRLG vs RSE, BW vs B2W2). "Clear all" is the lowest-risk option for R2-BUG-004, because no stale move can survive.

**Missed generations or paths.**

Generation coverage is correct. Gen I, II and V start with moves; Gen III and IV start blank. All five generations auto-fill moves on a real species change, and reselecting the same species is a no-op everywhere.

Missed paths:

- Gen I reset sites: Gen1PokemonEditorOverlayUX.inc:817, Gen1PokemonEditorOverlayUXCleanup2.inc:511 and :572, Gen1PokemonEditorOverlay.cpp:688, and Gen1PokemonEditorOverlayFoundation.inc:364. Also the `draft{}` member initialisers at UX.inc:78, Cleanup2.inc:58 and Overlay.cpp:69. Changing the struct default covers all of them.
- Gen V has two recreateCreateSpecies callers, Gen5SharedReadOnlySurface.inc:762 and :888. The pending Create Keep goes through keepWithPresentation (Gen5SharedScreenState.h:650-665), not validateCreate5. The PK5 is built through createNative5 (Gen5NativeCreate.h:25), which also calls validateCreate5, so any draft-mode loosening has to be threaded through that general creation API.
- Gen V Field::Move staging (Gen5StagedPokemonRecord.h:422-434) accepts move 0 in any slot. A moveless or non-contiguous Gen V Create or Edit is already keepable today. This is an existing save-safety gap that the proposed Gen V Keep gate must close.
- Gen II legacy prompt Add: TrainerViewScreenGSCOverlay.inc:364-374 builds a Gen2 BoxPokemonCreate with moves defaulting to {} (Gen2StagedEditor.h:68) and lets the user enter move 0 for all four slots. It is an existing moveless Add path, so the at-least-one-move gate must also cover it (or it must be confirmed unreachable).
- The shared status string kMovesRefreshedStatus (SpeciesChangeMoves.h:119) and the Gen V error text at Gen5SharedReadOnlySurface.inc:790 must be reworded.

Tests the investigation missed:

- tests/test_hw_round1_regression_gate.py:657-658 is C14 "Gen II Create starts with native level-up moves (Oct 26 PASS)". It is an owner PASS that the new request reverses. Retire or update it with a justification that cites the R10 H28 comment.
- tests/test_gen1_export_transaction.cpp:163-174 expects the "Moves: Thunder Shock, Growl" review row from a default BoxPokemonCreate.
- tests/test_qa_matrix_gen12.cpp:1096-1130 (stageAdd of BoxPokemonCreate{}) and tests/test_gen1_japanese_writer.cpp:233 and :409 use the default create. Their moves or bytes change once the struct default is blank.
- Also check tests/test_gen12_review_parity.cpp, tests/test_gsc_workspace_render.cpp, tests/test_qa_matrix_gen4.cpp and tests/test_gen5_staged_slot_actions.cpp, which reference Thunder Shock or move 84 or the Create defaults.

New tests needed:

- Gen V: Keep of a pending Create and of an Edit with slot 0 empty, or with non-contiguous slots, is refused. The workspace bytes stay unchanged.
- Gen II: the legacy prompt Add with four zero moves is refused (or the path is shown to be unreachable).

</details>

### CREATE-VALIDATION — Create validation: required fields per generation (P1 request)

- **Classification:** 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 5 Safety concern  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Correct today: Gen V only. On Create it shows "Choose a location" and Keep refuses until a location is picked. Affected: Gen III and Gen IV. Both have native met location fields, but Create silently commits a placeholder location id 0 and no check stops Keep or Add. Gen II Crystal has native caught data (location, level and time). Its Create stores caughtData=0, which means "no caught data", and nothing prompts the user to fill it in; whether to require it is the owner's call (see open questions). Not affected: Gen I and Gen II Gold/Silver. They have no met fields at all, so a met-location rule must not apply to them and none should be invented.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Gen V (the owner marked H07 PASS):
- include/Integration/Gen5/Gen5SharedScreenState.h:347-352. createNeedsMetLocation() returns createPending_ && !createMetLocationChosen_. noteCreateMetLocationChosen() sets the flag.
- :402 stages the placeholder kSharedCreateMetLocation=1 (:824).
- :442 resets the flag in beginCreateDraft.
- :657-660 keepWithPresentation refuses with "Choose a Met Location before Keep".
- src/UI/Gen5SharedReadOnlySurface.inc:371 shows the row as "Choose a location".
- :697 and :708-713 mark the choice as made, both when a location is picked and when the stored value is picked explicitly.
- :99-100 shows the refusal through screen.postStatus(error,360). This is the popup at the bottom of the screen the owner describes. It is the same postStatus that every generation uses.
- src/UI/SharedPokemonEditorSurface.inc:521-525 calls the optional adapter hook A::metLocationAccepted. Only the Gen V adapter defines it.

Gen IV:
- src/Integration/Gen4/Gen4StagedPokemonEditor.cpp:321 sets defaults.metLocation = 0, with the comment "UI must choose a real encounter location before legality claims".
- Nothing enforces that comment. include/UI/Gen4SharedPokemonSession.h:108-117 keepCreate and Gen4StagedPokemonEditor.cpp:420-480 stageCreateBoxPokemon check only the slot and a structural reparse.
- src/UI/Gen4SharedPokemonSurface.inc:368-374 keepWorkspace calls keepCreate directly.
- For HeartGold/SoulSilver, id 0 is not a native met location at all. include/Names/LocationNames.h:63-65 says HG/SS are valid only for >125 and <234. A HG/SS Create can therefore stage a non-native met location without any warning.

Gen III:
- include/UI/Gen3SharedPokemonSession.h:95-109 beginCreate never sets metLocation, so it stays 0. In src/Names/LocationNames.cpp:27-28, G3 id 0 is "Littleroot Town", a real Ruby/Sapphire/Emerald location. FireRed/LeafGreen Creates get that location from another region without being asked.
- Gen3SharedPokemonSession.h:337-346 add() and src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:747-776 only bound metLocation to 255 or less.
- src/UI/Gen3SharedPokemonSurface.inc:374-395 keepWorkspace calls session.add with no required-field check.

Gen II:
- src/UI/Gen2PokemonEditorFoundation.inc:188 creates with p.caughtData = 0.
- Crystal holds editor-only pending OT gender and met time (:80-84) and never invents caught history (src/UI/Gen2UnifiedPokemonWorkspace.inc:316-339).
- stageCreate (src/UI/Gen2PokemonEditorFoundation.inc:307-326) already blocks Add for one other reason, incompatible moves ("Add blocked: correct red incompatible move rows first"). There is no met check.

Gen I:
- src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:152-168 ux3StageAdd blocks only incompatible moves. Gen I has no met fields, which is correct.

Moves:
- Gen V checks "First move required" in validateCreate5 (include/Integration/Gen5/Gen5CreateValidation.h:132-145). That check runs on the initial create request (sharedCreateDefaults, Gen5SharedScreenState.h:407).
- I did not find a check that the final edited draft has at least one move in Gen III or Gen IV. Gen3StagedPokemonEditor.cpp:220-235 validPpForMoves accepts empty slots.

**Root cause.**

The required-met-location rule exists only as Gen V private state (Gen5SharedScreenState.h:351-352 and :657-660), with a Gen V-only adapter hook (Gen5SharedReadOnlySurface.inc:697, called through the optional `requires` at SharedPokemonEditorSurface.inc:524). The other generations' Create paths never got the rule. Gen IV even leaves a TODO-style comment that nothing enforces (Gen4StagedPokemonEditor.cpp:321). Gen III defaults to id 0 without any comment (Gen3SharedPokemonSession.h:95-109).

The shared contract that already knows each generation's native fields is not used for Create completeness. That contract is fieldAccessForGeneration in include/UI/SharedPokemonEditorContract.h:251-440: MetLocation is Hidden for Gen I and Gen II Gold/Silver, Editable for Crystal when crystalCaughtData is set (:306-308), and Editable for Gen III-V.

Side note: the Gen I branch of that table has the same crystalCaughtData ternary (:273-275). It is harmless, because Gen I never passes true, but it is misleading.

**Shared owner.**

include/UI/SharedPokemonEditorContract.h should own a new pure function, for example createRequiredFields(Generation, bool crystal) returning a list of FieldIdentity, plus missingCreateFieldsMessage(...). Any field that fieldAccessForGeneration reports as Hidden for that generation is excluded. The choice state ("has the user chosen this field?") should be lifted out of Gen5SharedScreenState into a small shared CreateCompleteness tracker (set of chosen FieldIdentity). The tracker is reset at each generation's beginCreate and checked at each generation's Create Keep/Add, before any staging. SharedPokemonEditorSurface.inc:521-525 should call the tracker for every adapter, not only through the Gen V-only metLocationAccepted hook. The message stays on screen.postStatus, which already produces the popup the owner likes in every generation.

**Proposed fix.**

1. In SharedPokemonEditorContract.h, add constexpr createRequiredFields(gen, crystal):
   - Gen I and Gen II Gold/Silver: {}. Species and moves already come from defaults.
   - Gen II Crystal: {MetLocation}, if the owner wants Crystal-caught origin. Otherwise {}. This is the owner's decision.
   - Gen III, IV and V: {MetLocation}.
   Add a formatter: "Can't add yet - choose: Met Location". It joins the missing labels so that later required fields (Met Level, Ball) show up in the same message without new UI.
2. Add a tiny shared struct CreateRequiredChoices { bitset chosen; void reset(); void note(FieldIdentity); missing(gen, crystal) }. Gen V's createMetLocationChosen_ becomes an instance of it. Keep the Gen V public API names (createNeedsMetLocation, noteCreateMetLocationChosen) as wrappers so existing tests still pass.
3. Wire it in each generation:
   - Gen IV: reset in beginCreate (Gen4SharedPokemonSurface.inc:584-614). Note it in the MetLocation applyPicker branch (:784-785) and on explicit acceptance of the stored value through the same SharedPokemonEditorSurface hook, made generic. Check it at the top of keepWorkspace before keepCreate (:368-371). Show "Choose a location" for the row during Create, as Gen V does.
   - Gen III: same at beginCreate (Gen3SharedPokemonSurface.inc:333-366), the picker apply (:822ff), and keepWorkspace before session.add (:383-385).
   - Gen II Crystal (only if the owner opts in): check in stageCreate (Gen2PokemonEditorFoundation.inc:307) next to the existing movesAllowCommit gate, using decodeCrystalCaughtData(state.working.caughtData).present.
   - Gen I: no change.
4. Moves: keep only the real native invariant, that move 1 must be non-empty. Gen V already has it (Gen5CreateValidation.h:137); add it as a required check at Keep for Gen III and IV if it is missing there. Do NOT implement "a level 100 Pokemon should have 1-2 moves". No game has that rule. In the native games a generated Pokemon gets its last up-to-four level-up moves at or below its level (pret FillMoves / GiveMonInitialMoveset, as the repo's own comments cite: include/UI/Gen2PokemonSession.h:32 and Gen3SharedPokemonSession.h:125-128). A level 100 Pokemon usually has 4 moves. Move legality is already covered by the incompatible-move gates (Gen I ux3StageAdd, Gen II movesAllowCommit).
5. Optional: in staging, refuse a non-native Gen IV met location (isGen4NativeMetLocation) on Create as defense in depth. This closes the HG/SS id-0 case even if the UI gate is bypassed.

**Risks.**

Save safety improves: the gate only refuses before staging, and every refusal path already leaves the staged bytes unchanged. The main risk is breaking flows the owner marked PASS:
- H07 (Gen V): changing its message or state API would break 4 tests. Keep the wrappers and the message.
- Gen III and Gen IV Create Keep currently pass hardware tests with the placeholder. After this change the owner must pick a location every time, so the new behaviour has to be confirmed again on hardware.
- Explicitly accepting the stored value must count as a choice in Gen III and Gen IV. Gen III uses its own picker code path (Gen3SharedPokemonSurface.inc:822), not SharedPokemonEditorSurface. Otherwise the "pick the shown value" case deadlocks.
- If the gate is put in the sessions (add/keepCreate), import and clone paths that reuse them could be blocked by mistake. Put it in the Create Keep entry points only.
- Do not apply any Create rule to Edit of existing Pokemon: real saves contain legitimate id-0 and odd met data.
- Inventing a move-count rule would refuse legal Pokemon. That is why it is excluded.

**Open questions.**

1. Gen II Crystal: caughtData=0 ("no caught data") is a normal native state, for example for Pokemon traded from Gold/Silver. Should a Crystal Create require a Met Location (it would claim a Crystal capture), or should it be optional? This needs the owner's decision.
2. Should Met Level and Ball also be required, or only Met Location? The owner named only Met Location. Ball and Met Level have sensible defaults (Poke Ball, met level equal to level).
3. The owner's "a level 100 Pokemon should have 1-2 moves" is not a game rule, and I recommend against it. Confirm the owner accepts the replacement: only "move 1 required" plus the existing incompatible-move gates.
4. On hardware, confirm that the postStatus popup position and duration (360 frames in Gen V) are what the owner means by "a popup in the middle bottom", and that it reads the same in Gen III and Gen IV.
5. I did not verify whether Gen III and Gen IV Keep accept a Create draft whose four move slots are all empty after editing. Check this with a host test before deciding whether the move-1 gate is needed there.

**Files to change:** `include/UI/SharedPokemonEditorContract.h`, `include/Integration/Gen5/Gen5SharedScreenState.h`, `src/UI/SharedPokemonEditorSurface.inc`, `src/UI/Gen5SharedReadOnlySurface.inc`, `src/UI/Gen4SharedPokemonSurface.inc`, `include/UI/Gen4SharedPokemonSession.h`, `src/UI/Gen3SharedPokemonSurface.inc`, `include/UI/Gen3SharedPokemonSession.h`, `src/UI/Gen2PokemonEditorFoundation.inc (Crystal only, if owner opts in)`, `src/Integration/Gen4/Gen4StagedPokemonEditor.cpp (optional non-native location refusal on Create)`

**Existing tests affected:**
- tests/test_qa_matrix_gen4.cpp:664-683 (Session Create -> keepCreate stages with the default metLocation 0; needs a location chosen first, a justified update)
- tests/test_qa_matrix_gen3.cpp:699-716 and :845 (Create add on the default draft; needs a location chosen first if the gate sits in the session; unaffected if the gate sits only in the surface keepWorkspace)
- tests/test_gen3_shared_pokemon_session.cpp:229, 271, 312, 357 (beginCreate followed by add, same consideration)
- tests/test_gen4_hardware_surface_contract.cpp:463, 522, 540 (source-text contracts on beginCreate/keepCreate; the order of the new check may need the contract extended)
- tests/test_qa_matrix_gen5.cpp:613-617, tests/test_gen5_party_box_parity.cpp:388-403, tests/test_ui_editor_input_host.cpp:229, tests/test_ui_layout_host.cpp:450 (pin Gen V behaviour and the exact message 'Choose a Met Location before Keep'; must keep passing unchanged, or the message change must be deliberate)
- tests/test_gsc_pokemon_session.cpp:269-336 and tests/test_qa_matrix_gen12.cpp:1502 (only if the Crystal requirement is adopted)

**New tests:**
- Contract unit test: createRequiredFields returns {} for Gen1, Gen2 non-Crystal (and Crystal if opted out), and {MetLocation} for Gen3/4/5; every required field is non-Hidden in fieldAccessForGeneration for that generation.
- Gen IV session/surface: Create then Keep without a location is refused with the shared message, staged bytes are byte-identical, and the draft stays open; after picking a location (or explicitly re-picking the stored value) Keep stages.
- Gen IV HG/SS: a Create cannot stage met location 0, which is non-native per isGen4NativeMetLocation.
- Gen III (RSE and FRLG): same refuse-then-accept pair; an FRLG Create can no longer silently commit Littleroot Town (id 0).
- Gen V regression: existing behaviour and message unchanged through the shared tracker.
- Gen I / Gen II G/S: Create Add is not blocked by any met requirement (guards against over-applying the rule).
- Gen II Crystal (if adopted): Add blocked until caught data is present; the pending OT gender and met time alone do not satisfy it.
- Message formatter: names every missing field, and is empty when nothing is missing.

**Skeptic corrections.**

I confirmed the met-location part of the root cause, but the investigation misses a second, larger defect behind the owner's "moves for current level" request, and some details are wrong. I checked out wmain HEAD 67b1b6a0 and ran one scratch probe.

CONFIRMED:
- The Gen V gate exists and matches the citations:
  - include/Integration/Gen5/Gen5SharedScreenState.h:351-352, :442, :657-660 and :824.
  - src/UI/Gen5SharedReadOnlySurface.inc:99-100, :371, :697 and :708-713.
  - src/UI/SharedPokemonEditorSurface.inc:521-525. The `requires` hook there is only reached by Gen IV and Gen V, because only Gen4SharedPokemonSurface.inc:51 and Gen5SharedReadOnlySurface.inc:45 include that file.
- Gen IV has no met gate:
  - Gen4StagedPokemonEditor.cpp:321 sets metLocation = 0.
  - Gen4SharedPokemonSession.h:108-117 keepCreate checks only the mode, then stages.
  - Gen4SharedPokemonSurface.inc:370-372 calls keepCreate directly.
- Gen III has no met gate:
  - Gen3SharedPokemonSession.h:95-109 never sets metLocation.
  - add() at :337-346 only calls stageAddBoxPokemon.
  - Gen3StagedPokemonEditor.cpp:774 only bounds the value to 255 or less.
  - Gen3SharedPokemonSurface.inc:383-385 has no gate.
  - G3 id 0 is "Littleroot Town" (LocationNames.cpp:28).
- Gen II: Gen2PokemonEditorFoundation.inc:188 creates with caughtData = 0. stageCreate at :307-317 gates only incompatible moves.
- Gen I: Gen1PokemonEditorOverlayUXCleanup3.inc:152-168 gates only incompatible moves.
- Contract: SharedPokemonEditorContract.h:302-308 makes Crystal's MetLocation Editable only when crystalCaughtData is set.

MISSED, highest impact: the default Gen III and Gen IV Create stages a Pokemon with zero moves.
- Gen III:
  - Gen3SharedPokemonSession.h:96 starts from `StagedPokemonRecord record{}`, and the moves array defaults to {} (include/Integration/Gen3/Gen3StagedPokemonEditor.h:24).
  - beginCreate (:95-109) never seeds moves.
  - setSpecies returns early when the species is unchanged (:115, `if (species == working.species) return true;`). So re-picking Pikachu does not seed moves either.
  - Probe (scratchpad/probe-skeptic-cv/p.cpp): beginCreate then createRequest() gives "moves 0 0 0 0 metLoc 0", and setSpecies(25) leaves moves[0] at 0.
  - stageAddBoxPokemon accepts this: validPpForMoves (Gen3StagedPokemonEditor.cpp:220-235) passes empty slots. tests/test_qa_matrix_gen3.cpp:699-716 shows `create.add` succeeding on exactly this default draft.
- Gen IV:
  - Pokemon4Mutable::createStored (src/Pokemon/Pokemon4Mutable.cpp:86-170) zero-fills 0x88 bytes and writes no move words.
  - Moves are seeded only when the species changes (Gen4SharedPokemonSurface.inc:760, `value != previousSpecies`).
  - tests/test_qa_matrix_gen4.cpp:679-683 keepCreate succeeds on the moveless default.
- The comment at Gen2PokemonEditorFoundation.inc:208-209 says "like ... Gen I / III-V Create (Pikachu Lv 5: Thunder Shock, Growl)". That is false for Gen III and Gen IV.
- The generations that seed moves correctly:
  - Gen I: BoxPokemonCreate moves {84,45,0,0} (include/Integration/Gen1/Gen1StagedPokemonEditor.h:34).
  - Gen II: seedNativeMoves at Gen2PokemonEditorFoundation.inc:210.
  - Gen V: move 84 at Gen5SharedScreenState.h:407.
- So this is a reproduced software bug (classification 1) and a cross-generation inconsistency in Gen III and Gen IV. The investigation listed it only as an unverified open question (Q5).
- Fix: seed native level-up moves in Gen III beginCreate, with the same refreshArrays call setSpecies already uses at Gen3SharedPokemonSession.h:132. Seed them in Gen IV beginCreate / createBoxDraft with SpeciesMoveDefaults::forSpecies(gen4TableForGroup...), as at Gen4SharedPokemonSurface.inc:764. Add a "move 1 required" check at Create Keep in both.

REFUTED:
- The side note "the Gen I branch has the same crystalCaughtData ternary (:273-275)" is false. Lines 270-276 are ReadOnly/Hidden cases. The only crystalCaughtData use in the table is at :308.
- Proposed fix step 5 does not close Gen IV Diamond/Pearl/Platinum. Their placeholder id 0 is "Mystery Zone" (LocationNames.cpp:289, G4 id 0), and isGen4NativeMetLocation returns true for 0 on D/P (<=111) and Pt (<=125) (LocationNames.h:58-62). So the "non-native" refusal catches only HeartGold/SoulSilver. The D/P/Pt picker (Gen4SharedPokemonSurface.inc:678-683) also lists id 0 as a choosable row. For HG/SS the picker list omits 0, so pickerRow falls to 0.
- The plan to copy Gen V's "picking the placeholder's own value counts as a choice" is unsafe as stated. Gen V's own placeholder kSharedCreateMetLocation = 1 is "Mystery Zone" (LocationNames.cpp:530, G5 id 1). Copying the rule to Gen IV would let a user confirm Mystery Zone on D/P/Pt. A Create gate should reject the placeholder id itself, not track "touched" state. Gen III is safe here only because kKeepStoredRow is suppressed when the stored id is 0 (include/UI/Gen3EncounterPickerModel.h:48).
- Minor line drift: the HG/SS case in LocationNames.h is :63-65, which matches the citation.

NEW RISK (missed): a deadlock in Gen III. Its Met Location picker is encounter-template driven. When the species has no exact-game template, the picker refuses to open: Gen3SharedPokemonSurface.inc:728-733 posts "No supported exact-game encounter templates for this Pokemon" and returns. A mandatory met gate in Gen III would make such species impossible to Add, with no way to satisfy the gate. Before gating, the fix needs either a full native location list for Create, or an explicit "no template, choose any native location" fallback. Gen IV and Gen V use full native lists and do not have this problem.

**Missed generations or paths.**

1. Gen III and Gen IV default Create has zero moves.
   - Gen III: Gen3SharedPokemonSession.h:95-109 plus the early return at :115. Confirmed by probe.
   - Gen IV: Pokemon4Mutable.cpp:86-170 with Gen4SharedPokemonSurface.inc:584-614, where moves are seeded only on a species change at :760.
   - Gen I (Gen1StagedPokemonEditor.h:34), Gen II (Gen2PokemonEditorFoundation.inc:210) and Gen V (Gen5SharedScreenState.h:407) already seed moves.
   - This is the concrete bug behind H28's "moves for current level". The investigator left it as open question 5.
   - Tests that pin the moveless default:
     - tests/test_qa_matrix_gen3.cpp:699-716
     - tests/test_qa_matrix_gen4.cpp:664-683
     - Possibly tests/test_gen3_shared_pokemon_session.cpp (the beginCreate cases).
     - These will see different staged bytes after seeding. That is a deliberate update.
2. Gen IV Diamond/Pearl/Platinum placeholder.
   - Id 0 is "Mystery Zone", and isGen4NativeMetLocation accepts it (LocationNames.h:58-62).
   - The proposed step 5 only fixes HeartGold/SoulSilver.
   - The Create gate must reject the placeholder id itself, not rely on native-range checks or on "user touched the picker".
3. Gen V's placeholder (id 1) is also "Mystery Zone" (LocationNames.cpp:530), and an explicit re-pick counts as a choice (Gen5SharedReadOnlySurface.inc:710; SharedPokemonEditorSurface.inc:521-525).
   - The PASS behaviour therefore lets a user confirm Mystery Zone.
   - Copying this pattern to Gen IV would be wrong. If the owner wants a "real location", the Gen V re-pick-placeholder rule also needs revisiting. That touches tests/test_gen5_party_box_parity.cpp:403 ("picked the shown value"), so it needs the owner's sign-off.
4. Gen III picker dead end. Gen3SharedPokemonSurface.inc:728-733 refuses to open for species with no exact-game encounter template, so a mandatory met gate would block Add for those species permanently. A fallback is needed first.
5. Gen III's Create entry points include the X/Add shortcut at Gen3SharedPokemonSurface.inc:2725, alongside :1720. Both go through beginCreate (:333), so resetting there covers both. Gen IV entry points are :1065 and :2414, and both use beginCreate (:584).
6. Wiring scope. SharedPokemonEditorSurface.inc is used only by Gen IV and Gen V. Gen III (its own picker at Gen3SharedPokemonSurface.inc:822-829), Gen II and Gen I do not use it. Making the SharedPokemonEditorSurface hook generic therefore does not reach Gen III. The pure rule belongs in SharedPokemonEditorContract.h; each Keep/Add entry point must call it explicitly.
7. Refuted side note: there is no crystalCaughtData ternary in the Gen I branch of SharedPokemonEditorContract.h. The only use is :308.

</details>

### ACTIONS — Actions menu consistency (P1)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 4 Known missing v1.0 functionality  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Box sheet, Move missing: Gen II and Gen III. The backend works (hold-Y carry beginGen2 / beginGen3), but the Actions menu never lists Move. Gen I (Boxes view only), Gen IV and Gen V already list Move.

Party sheet, Review Pending Changes missing: Gen I, II and III. Their party sheet hard-codes "no pending changes", so the row can never appear. The Gen IV and Gen V party sheets show Review when changes are pending.

Review on box sheets behaves the same in all five generations: it is listed only while changes are pending. If nothing was staged on hardware, the owner would not see it in any generation. That makes it a policy question (always show it, greyed when there is nothing to review?), not a per-generation bug.

Move to Vault does not exist in any generation (Known missing v1.0 functionality). Clone / Move / Release on party slots are not supported in Gen I-IV. Gen V party offers Clone only. This is a minor cross-generation inconsistency.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Live action lists, taken from the highest-priority handlers in src/UI/TrainerViewScreenCompositeOverlay.cpp:391,443-444,484,499-500. [X] means the row is shown only under condition X.

BOX, occupied slot
- Gen I (include/UI/Gen1PokemonEditorUIContract.h:40-61; built at src/UI/ClassicReleaseActionFix.inc:24-26 and 125-127): View, Edit, Clone, [Move only when boxMoveAvailable: Boxes view + staged editor, src/UI/Gen1PokemonEditorOverlayUX.inc:104-108], Release, Origin & Legality, [Export to file when staged], [Review Pending Changes when pending], Cancel. The Storage pane has no Move.
- Gen II (src/UI/ClassicReleaseActionFix.inc:166-176 caps; 339-340 input; 484-486 draw → SharedPokemonEditor::actionsForSlot, include/UI/SharedPokemonEditorContract.h:71-95): View, Edit, Clone, Release, Origin & Legality, Export to file, [Review when pending], Cancel. There is NO Move. The dispatcher says so explicitly: ClassicReleaseActionFix.inc:441 `case Action::Move: // Not offered by the Gen II action set.`
- Gen III (src/UI/Gen3SharedPokemonSurface.inc:1694-1696 input; 2516-2517 draw; same shared builder, caps {clone, remove, legality, import, export}): View, Edit, Clone, Release, Origin & Legality, Export to file, [Review when pending], Cancel. There is NO Move, and no `case Shared::Action::Move` in the switch at 1717-1762.
- Gen IV (src/UI/Gen4SharedPokemonSurface.inc:947-983, its own gen4Actions): View, Edit, Clone, Move, Release, Origin & Legality, [Export when staged], [Review when pending], Cancel. Edit / Clone / Move / Release are greyed when no staged editor exists.
- Gen V (include/Integration/Gen5/Gen5SharedEditorBridge.h:35-86, its own builder): View, Edit, [Clone if an empty box destination exists], [Move if box + destination], [Release if box], Origin & Legality, Export to file, [Review when pending], Cancel.

PARTY, occupied slot
- Gen I / II / III (one shared sheet, src/UI/ClassicPartyActions.inc:29-33): `withAction(actionsForSlot(true,{false,false,true},false), ExportFile)` gives exactly View, Edit, Origin & Legality, Export to file, Cancel. The third argument (hasPendingChanges) is the literal `false`, so Review can never appear. The dispatcher at 101-136 also has no Review branch.
- Gen IV (gen4Actions, Party target, Gen4SharedPokemonSurface.inc:970-982): View, Edit, Origin & Legality, [Export], [Review when pending], Cancel.
- Gen V (Gen5SharedEditorBridge.h:61-84, region Party): View, Edit, [Clone], Origin & Legality, Export to file, [Review when pending], Cancel.

Requested items not present anywhere:
- Move to Vault. Master Vault is "COMING SOON" (include/UI/AppShellModel.h:41-57). The Gen IV enum keeps a dead AddMasterVault that only posts "Master Vault is intentionally not started" (Gen4SharedPokemonSurface.inc:81, 1031-1032), and it is deliberately not listed (NarrowPokemonActionMenu.h:88-89).
- Party Move and party Release: not offered in any generation.

The shared vocabulary and order already exist: include/UI/NarrowPokemonActionMenu.h:84-134 ranks Add, Import, View, Edit, Clone, Move, Release, O&L, Export, Review, Cancel.

**Root cause.**

There is no single action-menu owner. Each generation builds its own list.

1. Gen II / III Move. The shared builder that Gen II and Gen III use, SharedPokemonEditor::actionsForSlot (include/UI/SharedPokemonEditorContract.h:71-95), has no Move capability. ActionCapabilities (52-60) has no `canMove` field, and the occupied branch (86-93) never appends Action::Move, even though the enum has it (line 45, "Gen IV/V shared box vocabulary"). Gen I got Move through its own contract (Gen1PokemonEditorUIContract.h:48, boxMove flag) plus an entry point, ClassicPackedMove::beginGen1FromAction (src/UI/ClassicPackedMoveOverlay.inc:1015-1025). Gen IV and Gen V hand-roll their own builders and entry points (beginGen4FromAction 1004, beginGen5FromAction 1027). The working single-slot carry backends for Gen II and Gen III, beginGen2 (ClassicPackedMoveOverlay.inc:312) and beginGen3 (390, sparse vector), have no FromAction entry point, so there is nothing to dispatch to.
   The parity was wrongly assumed in the docs. docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md:135-136 says Move was absent from Gen I "while Gen II-V list it in the shared E02 order", and docs/hardware/RETEST_62a2f8c3.md:108 says "for parity with Gen II–V". The code shows Gen II and Gen III never listed it.

2. Gen I-III party Review. src/UI/ClassicPartyActions.inc:32 passes a hard-coded `false` for hasPendingChanges, and handle() (101-136) has no Review dispatch. This was a deliberate earlier decision, recorded in tests/test_classic_party_review_path.cpp:1-4: "Party Actions keeps its hardware-accepted View / Edit / Origin & Legality / Cancel sheet (E02 / NEW-20)". It now conflicts with the owner's request and with Gen IV / V party behaviour.

3. Review is conditional everywhere: SharedPokemonEditorContract.h:81,92; Gen1PokemonEditorUIContract.h:56; Gen4SharedPokemonSurface.inc:961,980; Gen5SharedEditorBridge.h:83. That matches the existing E02 rule ("Review appears only while staged changes exist", GEN14_CAMPAIGN_PROGRESS.md:142-143). It is not a bug, but it explains why the owner sometimes does not see Review.

4. Move to Vault has no backend (docs/MASTER_VAULT_SPEC.md; AppShellModel "COMING SOON").

**Shared owner.**

The fix belongs in include/UI/SharedPokemonEditorContract.h (actionsForSlot plus ActionCapabilities), which becomes the ONE action-set builder for every generation and surface. include/UI/NarrowPokemonActionMenu.h stays the single label/order/presentation layer. Generation surfaces (Gen1PokemonEditorUIContract.h, the Gen4 gen4Actions builder, Gen5SharedEditorBridge::actions, ClassicPartyActions.inc) should only fill in capability flags.

Move dispatch for every generation belongs in src/UI/ClassicPackedMoveOverlay.inc: one beginFromAction(screen, gen, box, slot) that wraps beginGen1-5.

**Proposed fix.**

Minimal and safe, in order:

A. Gen II / III Move (the reproduced bug)
1. SharedPokemonEditorContract.h: add `bool canMove = false;` as the LAST field of ActionCapabilities. It must go last because callers use positional aggregate initialisation, e.g. Gen3SharedPokemonSurface.inc:1695 `{true,true,true,true,true}` and ClassicPartyActions.inc:32 `{false,false,true}`. In actionsForSlot, append Action::Move after Clone and before Remove when `capabilities.canMove`, matching the rank in NarrowPokemonActionMenu.h:125.
2. ClassicPackedMoveOverlay.inc: add beginGen2FromAction(screen, box, slot) and beginGen3FromAction(screen, box, slot), modelled on beginGen1FromAction (1015-1025). Each should check generation, Boxes view and slot bounds, refuse while a carry, selection or yPending is active, set the selected box/slot, and call beginGen2(state, box, slot) or beginGen3(state, box, {slot}).
3. Gen II (ClassicReleaseActionFix.inc): turn releaseActionCapabilities() into a function of the screen that sets canMove under the same predicate as Gen I's boxMoveAvailable (Boxes view, detailViewActive, staged editor). Use it in BOTH the input path (339) and the draw path (484) so rows, touch ids and dispatch stay aligned. Replace the `case Action::Move` no-op at 441 with: close the sheet, then beginGen2FromAction, else post "Move could not start from here".
4. Gen III (Gen3SharedPokemonSurface.inc:1694 and 2516): set canMove the same way in both calls and add `case Shared::Action::Move` calling beginGen3FromAction.

B. Party Review in Gen I-III (owner request; reverses the earlier E02 / NEW-20 decision)
In ClassicPartyActions.inc:29-33, make actions() take the screen and pass the real pending state per generation (Gen1 editor()->pendingChanges(), Gen2 stagedEditor()->pendingChanges(), Gen3 reviewRows()/editor hasPendingChanges). Add a Review branch to handle() that closes the sheet and calls screen.openStagedReview(false), which already exists and handles all three generations. Draw and handle must call the same actions(screen).

C. Consistency policy, needs the owner's decision (see open questions)
If the owner wants every sheet to have the same shape, show "Review Pending Changes" always and grey it when nothing is pending. NarrowPokemonMenu::Model already supports disabled rows (nativeIndex −1, NarrowPokemonActionMenu.h:18,25), and Gen IV already uses greyed rows. Do not add a "Move to Vault" row until the Vault backend exists: the documented product rule (NarrowPokemonActionMenu.h:88-89; test_classic_pokemon_action_menu.cpp "absent, not greyed") is to list only real commands. Party Move / Release / Clone stay as they are unless the owner asks for party↔box operations, which would be new behaviour.

D. Follow-up refactor, not needed for this fix: move Gen I, Gen IV (gen4Actions) and Gen V (Gen5SharedEditorBridge::actions) onto the same actionsForSlot builder with capability flags, so drift cannot happen again. Also correct the false parity statements in docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md:135-136 and docs/hardware/RETEST_62a2f8c3.md:108.

**Risks.**

Save safety: Move only starts the existing staged carry. Gen II beginPackedMove/placePackedGroupMove and Gen III beginSparseMove/placeSparseMove are already reachable via hold-Y and pinned by test_gen3_hardware_surface_contract.cpp:300-311, and nothing writes to the source save. The new risk is entry state. Starting a carry from a modal sheet must first close the sheet (the Gen I pattern: ux2Close + controllerNavigation.reset) and refuse when a carry, selection or yPending is already active; otherwise a stale sheet could swallow Y/B during the carry. Gen II packed boxes compact on removal, so the drop index semantics must match what the owner already accepted for hold-Y.

UI and input coupling: touch ids and dispatch use the native index (Gen II base 500, Gen III 31600, party 89000). Inserting Move or Review shifts the indices of later rows. The input and draw paths MUST call the builder with identical capabilities, or rows and dispatch go out of step; for Gen II there are four call sites (ClassicReleaseActionFix.inc 339 and 484, plus shadow layers Gen2SharedSurfaceParity.inc:115 and Gen2SharedPokemonSurface.inc:122, which use gen2ActionCapabilities with canRemove=false).

Initialisation: ActionCapabilities is positionally initialised in several places, so canMove must be appended as the last field.

Owner-accepted behaviour: Gen I box Move and the Gen IV/V lists are PASS references and must not change. The Gen I-III party sheet without Review was hardware-accepted (E02/NEW-20), so adding Review deliberately revises an accepted sheet and needs a hardware recheck. Showing an always-visible greyed Review would change every generation's accepted sheet height.

**Open questions.**

1. Review Pending Changes policy: always show it, greyed when nothing is pending, so every sheet has the same rows? Or keep the current E02 rule of showing it only while changes are pending (all generations) and just fix the Gen I-III party sheet? The owner's list implies always present.
2. Move to Vault: confirm it stays off the menu until the Vault v1.0 backend exists (Known missing v1.0 functionality), not shown as a greyed placeholder. The product rule so far forbids placeholders.
3. Party slots: the owner lists Clone / Move / Release for "all gens". Party Release / Move / Clone are unsupported in Gen I-IV, and Gen V party offers Clone only. Should party ↔ box Move/Clone be built (new behaviour, separate safety work)? Or should the shared model just list only what is supported, which means also deciding whether to drop Gen V party Clone or add party Clone to Gen I-IV?
4. Should Move appear in the Gen II/III Storage pane? Gen IV allows it there (beginGen4FromAction storage flag); Gen I does not.
5. Hardware: confirm on device that Actions → Move in Gen II (packed box compaction) and Gen III (sparse) drops exactly like the accepted hold-Y carry, and that B returns the box unchanged.

**Files to change:** `include/UI/SharedPokemonEditorContract.h`, `include/UI/NarrowPokemonActionMenu.h`, `src/UI/ClassicPackedMoveOverlay.inc`, `include/UI/ClassicPackedMove*.h (declarations of the new FromAction entry points, if declared in a header)`, `src/UI/ClassicReleaseActionFix.inc`, `src/UI/Gen3SharedPokemonSurface.inc`, `src/UI/ClassicPartyActions.inc`, `include/UI/ClassicPokemonActionMenu.h (comment at lines 4-8 claims Gen I/II have no Move)`, `docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md`, `docs/hardware/RETEST_62a2f8c3.md`

**Existing tests affected:**
- tests/test_classic_party_actions.cpp:111-116: pins the party sheet as exactly 5 rows (View, Edit, O&L, Export, Cancel). Unchanged without pending changes; needs new cases with pending changes (and a different count if Review is always shown).
- tests/test_classic_party_review_path.cpp:1-4: its header records the E02/NEW-20 decision that the party sheet has no Review. Needs a deliberate update that cites the owner's Round 10 request.
- tests/test_classic_pokemon_action_menu.cpp:60-100: pins Gen II/Gen I lists without Move. Still passes with canMove defaulting to false; add cases with canMove=true.
- tests/test_qa_matrix_gen12.cpp:1609-1630: pins Gen II occupied count == 6 and pending[5]==Review with caps lacking canMove. Unchanged with the default, but the comment 'releaseActionCapabilities() {clone, remove, legality}' should be updated to the live caps.
- tests/test_qa_matrix_gen3.cpp:856-882: Gen III action list with {true,true,true}. Unchanged with the default; should cover the Boxes-view canMove case.
- include/UI/NarrowPokemonActionMenu.h:212-225: static_asserts on counts 7 / 8 / 4. Unchanged with the default.
- tests/test_gsc_shared_editor_parity.cpp:210-212: designated-init caps. Unaffected.
- tests/test_gen12_packed_move.cpp:465-475 plus gate pins in tests/test_hw_round1_regression_gate.py:153,380 and tests/test_qa_hw_round1_gate_mutations.py:61: Gen I Move sheet literal. Must stay passing; Gen I is the reference.
- tests/test_gen4_hardware_surface_contract.cpp:496,743 and tests/test_gen5_party_box_parity.cpp:100-135, tests/test_gen5_staged_slot_actions.cpp:139-191, tests/test_gen5_readonly_ui_contract.py:1152: Gen IV/V Move pins. Unchanged unless refactor D is done.
- tests/test_gen3_hardware_surface_contract.cpp:300-311: carry backend string pins (beginSparseMove etc.). Must still pass.

**New tests:**
- Shared contract: actionsForSlot(true, {clone, remove, legality, import, export, canMove=true}, pending) returns View, Edit, Clone, Move, Release, O&L, Export, [Review], Cancel in the NarrowPokemonActionMenu rank order, and Move never appears for an empty slot. Add static_asserts in NarrowPokemonActionMenu.h for count 9 and geometry(9) fitting on screen.
- Gen II box Move: drive the real ClassicReleaseActionFix handler on a synthetic GSC save in Boxes view. Move is listed at the expected row and touch id (500+n). Selecting it starts the carry (beginGen2FromAction, state.generation==Gen2). Y drop stages exactly one packed move that strictly reparses. B cancels and leaves the staged box byte-identical. The Storage pane does NOT list Move.
- Gen III box Move: the same scenario through Gen3SharedPokemonSurface handleActions with touch base 31600. The sparse move places into an empty slot. B restores the box byte-identically. The draw-path and input-path action sets are identical.
- Cross-generation matrix test: for each generation I-V, box occupied and party occupied, the set of labels (ignoring conditional rows) equals the agreed shared set. This pins the parity the docs wrongly claimed.
- Gen I/II/III party Review: with a staged change, the ClassicPartyActions sheet lists 'Review Pending Changes' before Cancel, and A/touch opens the standalone shared Review (openStagedReview). Without changes it is absent, or greyed and not focusable if policy C is chosen.
- Mutation/gate entry: a mutation that drops canMove from the Gen II or Gen III call site must fail the gate. Mirror the existing 'round 3 E02 Gen I box sheet loses Move' gate.

**Skeptic corrections.**

I checked every citation and the root cause holds. Gen II and Gen III box sheets never list Move: include/UI/SharedPokemonEditorContract.h:52-60 has no canMove field, and the builder at 71-95 never appends Action::Move. Gen II leaves Move as a no-op at src/UI/ClassicReleaseActionFix.inc:441. Gen III falls into the `default:` branch at src/UI/Gen3SharedPokemonSurface.inc:1760-1762 ("not enabled in the current milestone"). The Gen I-III party sheet hard-codes hasPendingChanges=false at src/UI/ClassicPartyActions.inc:32, and handle() at 89-136 has no Review branch. This was a deliberate design choice, not drift. include/UI/ClassicPokemonActionMenu.h:7-8 says "the Gen I/II native action sets have no Move command". The false parity claims are confirmed at docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md:135-136 and docs/hardware/RETEST_62a2f8c3.md:108-109.

Corrections and gaps in the investigation:

1. Wrong return value in the proposed FromAction. beginGen2 (src/UI/ClassicPackedMoveOverlay.inc:312-349) and beginGen3 (390-424) return true even when they fail: no editor, empty slot, beginPackedMove or beginSparseMove refused. beginGen3 also returns true silently when the editor is null (line 401). The new FromAction wrappers must return state.active, as beginGen1FromAction does at 1015-1025 (`beginGen1(...); return state.active;`). "Call beginGen2 / beginGen3" alone is not enough, or the "Move could not start" status never fires.

2. Carry input liveness is not just "close the sheet". classicPackedMoveLayerAvailable (src/UI/TrainerViewScreenCompositeOverlay.cpp:250-270) only routes carry input when, for Gen II, `!editorOverlayState(screen).active` and Gen2 state.mode==None, and for Gen III, `!Gen3SharedEditorSurface::ownsFrame`. The Gen II dispatch must clear legacy.active AND legacy.pokemonActions, as the Close case at 435-438 does, before calling beginGen2FromAction. Otherwise the carry starts with Y/B swallowed by nobody: ClassicPackedMove::handleInput is never called at composite :439, and the carried Pokemon stays lifted, so the staged box is left with a pending packed move. Gen III must set state.actions=false first. The input order at composite :439-444 runs ClassicPackedMove before Gen3/Gen2 handlers, which is correct once the sheet is closed.

3. Native order must equal rank order. Rows are dispatched by NATIVE index: `actions[legacy.pokemonRow]` at ClassicReleaseActionFix.inc:369 and `actions[state.actionRow]` at Gen3SharedPokemonSurface.inc:1716. NarrowPokemonMenu::classic (NarrowPokemonActionMenu.h:136-146) reorders rows by rank for display but keeps the native index. Move must therefore be appended between Clone and Remove in the builder, not at the end. If it is appended at the end, the controller row and the drawn row drop out of step. The investigation states the placement but not this reason.

4. String-literal pins the investigation missed. The proposed edits will break each of these, so each needs a deliberate update:
- tests/test_classic_party_review_path.cpp:136-138 asserts the literal `actionsForSlot(true,{false,false,true},false)` is present ("Party Actions keeps the accepted four commands (no Review row)").
- tests/test_round4_shared_contract_mutations.py:56-57 (gate CG33) mutates that exact literal. The mutation finds nothing to mutate and the check fails.
- tests/test_qa_cross_gen_contract.py:1749 reports a violation if that literal is missing.
- tests/test_touch_appwide_surface_contract.py:183 pins `releaseActionCapabilities(),\n        !editor->pendingChanges().empty()`. Turning it into releaseActionCapabilities(screen), as proposed, breaks this assertion.
- tests/test_gen12_packed_move.cpp:509-517 pins the beginGen1FromAction signature and its call sites. Gen I must stay the reference if a generic beginFromAction is introduced.

5. Two live Gen I dispatchers. Gen I has one at ClassicReleaseActionFix.inc:50-57 and a second at src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:667/685, plus the actionsForSlot callers at Cleanup2:964/1216 and Cleanup3:874. Any refactor D that moves Gen I onto the shared builder must cover all of them. The legacy calls at Gen1PokemonEditorOverlayUX.inc:782/1002 omit boxMove.

6. Gen II shadow call sites are wider than reported. Gen2SharedPokemonSurface.inc has gen2ActionCapabilities() at :74 and actionsForSlot at :122 AND :326-327. Gen2SharedSurfaceParity.inc:115 draws with touch base 31600, not 500. These shadow sheets are reached only when handleReleaseActionInput returns false. The investigation listed only :122 and :115.

7. Storage-pane trap for Gen III. Its sheet opens from the Storage pane too (storageEntrySurface, Gen3SharedPokemonSurface.inc:2713). The canMove predicate for Gen III must explicitly require selectedMode==Boxes && detailViewActive, as Gen I's boxMoveAvailable does at Gen1PokemonEditorOverlayUX.inc:104-108. Otherwise Move is listed in Storage and refused there.

8. Static asserts. NarrowPokemonActionMenu.h:224 already static_asserts a 9-row Gen I sheet (count==9), so the 9-row geometry already fits. A new shared-builder assert is still worth adding: View, Edit, Clone, Move, Release, O&L, Export, Review, Cancel is 9 entries, within the std::array<Action,10> at SharedPokemonEditorContract.h:63.

9. Gen III fallback sheet. Without a staged editor, Gen III shows a source-only View/Cancel sheet (Gen3SharedPokemonSurface.inc:1660-1689). This is another per-generation divergence; it is correct, because nothing can be staged there.

10. Party Review targets differ. openStagedReview (TrainerViewScreenCompositeOverlay.cpp:184-213) exists for Gen I-IV as claimed. For Gen I it sets reviewReturn=Closed, so B from Review returns to the grid, not to the party sheet. That is acceptable but should be pinned. The pending-state sources differ per generation: Gen I editor()->pendingChanges(), Gen II stagedEditor()->pendingChanges(), Gen III reviewRows(). reviewRows is file-local to the Gen3 surface, so ClassicPartyActions may need an exported accessor.

**Missed generations or paths.**

- Gen II has four box action builders, not two:
  - ClassicReleaseActionFix.inc:339 (input) and :484 (draw), base 500
  - Gen2SharedPokemonSurface.inc:122 and :326, via gen2ActionCapabilities() at :74
  - Gen2SharedSurfaceParity.inc:115, base 31600

  The fix must update all of them, or retire the shadow ones, so they cannot diverge.
- Gen I has two live dispatchers that already route Move: ClassicReleaseActionFix.inc:55 and Gen1PokemonEditorOverlayUXCleanup2.inc:685.
- The Gen III Storage pane opens the same staged sheet (Gen3SharedPokemonSurface.inc:2713), so canMove needs a Boxes-view guard there.
- Gen III with no staged editor uses a separate View/Cancel sheet (Gen3SharedPokemonSurface.inc:1660-1689).
- Gen V party lists Clone (Gen5SharedEditorBridge.h:65-66 vs 67-68). Gen IV party drops Clone/Move/Release (Gen4SharedPokemonSurface.inc:965-969). Both confirmed.
- Source-string gates break under the proposed fix:
  - test_classic_party_review_path.cpp:137
  - test_round4_shared_contract_mutations.py:56 (CG33)
  - test_qa_cross_gen_contract.py:1749
  - test_touch_appwide_surface_contract.py:183
- Carry-liveness gate in classicPackedMoveLayerAvailable (TrainerViewScreenCompositeOverlay.cpp:250-270). If the Gen II legacy overlay is not fully deactivated before beginGen2FromAction, carry input is never delivered.
- The beginGen2/beginGen3 return value is always true (ClassicPackedMoveOverlay.inc:312-349, 390-424). The wrappers must return state.active.

</details>

### H27 — Gen V language display, Korean glyphs, keyboards (P2)

- **Classification:** 1 Reproduced software bug, 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 4 Known missing v1.0 functionality, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** Top-name refresh bug: Gen V only. Gen IV header decodes the full UTF-16 nickname (src/UI/Gen4SharedPokemonSurface.inc:1522, Utils::utf16ToUtf8(raw.nickname())). Gen III draws p.nickname (src/UI/Gen3SharedPokemonSurface.inc:2213). Both already follow Language. Gen I/II records have no language byte and no Language row, so they are not affected. Korean tofu: every generation that can show Hangul. Gen IV DP/PT/HGSS allow Korean in the Language cycle (include/Enums/LanguageID.h:44-48), so that is almost certainly where the owner saw "[][][][][]". Gen V shows English or "(native glyphs)" instead of Hangul, so it hides the problem rather than avoiding it. Korean GSC is unsupported. Gen III has no Korean. Keyboard: all generations. Only Latin, Symbols and Accents pages exist, plus kana pages for Japanese Gen I/II names. No Hangul page exists, and there are no Japanese pages for Gen III-V.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

Gen V Language row (src/UI/Gen5SharedReadOnlySurface.inc:446-453): pressing A steps through IDs 1,2,3,4,5,7,8 (include/UI/Gen5LanguageCycle.h:9) and stages Field::Language. The staged record handles this correctly (include/Integration/Gen5/Gen5StagedPokemonRecord.h:307-324). For an un-nicknamed, non-egg record it calls Names::defaultSpeciesName(species, lang, 5) and writes the result with writeUnicodeName5 (include/Integration/Gen5/Gen5NativeText.h:78-98). tests/test_gen5_language_default_name.cpp:63-75 checks that Pikachu becomes ピカチュウ, Pikachu and 피카츄 in the stored bytes.

The display does not use those bytes. The header comes from chrome() at Gen5SharedReadOnlySurface.inc:821-822, which sets info.nickname = asciiName5(...) or "". asciiName5 (Gen5NativeText.h:31-43) returns nullopt as soon as it sees a code unit outside 0x20-0x7E. The shared header (include/UI/SharedPokemonShell.h:48) then replaces an empty nickname with Names::getSpeciesName(species), and that function is always English (src/Names/SpeciesNames.cpp:9303-9306).

Measured on the repo's own name tables for species 1-649 (Python probe over src/Names/SpeciesNames.cpp, run read-only):
- JA: 649/649 non-ASCII, so the header falls back to English.
- KO: 649/649 non-ASCII, so the header falls back to English.
- IT and ES: 649/649 identical to English.
- FR: only 415 show a visible change; 105 have accents and fall back to English, e.g. 495 Vipélierre and 4 Salamèche.
- DE: 496 show a visible change.

So across the 7-step cycle the Gen V top name changes visibly only on French or German, and for many species never. For example, Pikachu reads "Pikachu" at every step. The Details Nickname row has the same fault (Gen5SharedReadOnlySurface.inc:355-357 shows "(native glyphs)"), and so do the review and summary rows (lines 2043, 2066-2068, 2113-2115 show "Native glyphs").

Fonts: src/UI/PKSEFramebuffer.cpp:87-106 loads romfs Nunito, NotoSansSymbols and NotoSansSymbols2, then only PlSharedFontType_Standard from the console (line 102). The Standard font covers Japanese but not Hangul, and PlSharedFontType_KO is never requested, so Korean is drawn as missing-glyph boxes.

Keyboard: src/UI/Dialogs/KeyboardDialog.cpp:192-245 has Letters (QWERTY plus ' - , . ?), Symbols (♂♀…× and so on), Accents (àé... ß) and Digits. Kana pages are at lines 130-190, but they are only enabled through request.japaneseKana (src/Utils/Keyboard.cpp:98-112, promptJapaneseName). Its only callers are Gen I/II Japanese names (include/UI/ClassicJapaneseNames.h:43, include/UI/ClassicTrainerSessionInput.h:113). There is no Hangul layout anywhere. Gen V name entry is ASCII-only by design (Gen5SharedReadOnlySurface.inc:401-404, Gen5NativeText.h:54-74).

There is no app display-language setting: grep finds no displayLanguage, uiLanguage or appLanguage, and getSpeciesName is hard-wired to English. Record language, stored nickname bytes and app UI language are separate concepts. Only the record language is editable.

H13: the owner says they did not open a Japanese save. Swapping the Language field only shows that the language-default name logic works on a Western save. It is not hardware evidence for Japanese save handling, and Japanese saves need an independent test.

**Root cause.**

1) Gen V top name (reproduced software bug): the stored data is correct, but the display uses an ASCII-only decoder meant for editing. Gen5SharedReadOnlySurface.inc:821 calls PokeVault::Integration::Gen5::asciiName5 (Gen5NativeText.h:39 `if(code<0x20 || code>0x7E)return std::nullopt;`). The empty result makes SharedPokemonShell.h:48 draw the English species name. JA and KO names, and accented FR names, therefore always show the English name. IT and ES default names equal the English ones. Gen IV avoids this because its chrome decodes the full UTF-16 string (Gen4SharedPokemonSurface.inc:1522). It is not a refresh problem, and the record is not nicknamed: test_gen5_language_default_name.cpp:71 checks that the nickname flag stays clear.

2) Korean boxes (known missing functionality): PKSEFramebuffer.cpp:102 loads only PlSharedFontType_Standard as a fallback. The libnx PlSharedFontType_KO (Hangul) font is never loaded.

3) Keyboards (new request / known missing): KeyboardDialog.cpp has no Hangul page. Kana pages are reachable only from promptJapaneseName (Keyboard.cpp:98). Gen III-V name writers do not accept non-ASCII or kana for free entry. Gen V says so in Gen5NativeText.h:19-21 and :76-77 ("free Unicode entry waits for the verified Gen V glyph rules").

**Shared owner.**

Display decoding: include/Integration/Gen5/Gen5NativeText.h should gain a display-only decoder (displayName5) used by every Gen V display site, matching Gen IV's raw.nickname()+Utils::utf16ToUtf8 path. That would bring the Gen V adapter in line with the Gen III/IV adapters that feed SharedPokemonShell::drawChrome. A better long-term owner is one shared "native UTF-16 to display UTF-8" helper in Utils used by Gen IV and Gen V. Glyph coverage: src/UI/PKSEFramebuffer.cpp, the single font loader, so every generation and every screen benefits. Keyboard layouts: src/UI/Dialogs/KeyboardDialog.cpp with Utils::KeyboardRequest (include/Utils/Keyboard.h:46). The per-generation writers (Gen5NativeText encode, Gen3/Gen4 encoders) must still decide which characters are storable.

**Proposed fix.**

Fix A (P2 bug, display only, minimal and save-neutral). Add to Gen5NativeText.h:
`std::string displayName5(span plain, bool nickname)`
It reads up to the capacity or the first 0x0000/0xFFFF, maps 0x246D to ♂ and 0x246E to ♀ (the inverse of writeUnicodeName5), converts with Utils::utf16ToUtf8, and replaces lone surrogates and other unrenderable units with U+FFFD or "?". Use it at Gen5SharedReadOnlySurface.inc:821 (chrome), :356 (Nickname/OT detail value), :2066-2068 (review detail) and :2113-2114 (summary rows). Keep asciiName5 for every editing and replay path: :400 prompt initial, Gen5SharedPokemonSession.h:150, Gen5SharedReview.h:205 and Gen5SharedScreenState.h:542/547. Those paths are the ASCII write gate and must not widen in this fix. When the name cannot be edited as ASCII, the Nickname prompt should start empty or show a "native name, ASCII replace only" guide.

Fix B (Korean glyphs, P2, all generations). In PKSEFramebuffer.cpp after line 106, also call plGetSharedFontByType(&ko, PlSharedFontType_KO) and nvgCreateFontMem(vg,"ko",...,0), then nvgAddFallbackFontId(vg, fontSans, fontKorean). It should be the last fallback, with the same freeData=0 and logging pattern. FONS_MAX_FALLBACKS is 20 (nanovg/fontstash.h:199), so there is room. Optionally load PlSharedFontType_NintendoExt for button glyphs. This change cannot touch saves.

Fix C (keyboards, owner question, v1.0+ feature, needs a design decision). Recommended answer to the owner: the NanoVG keyboard can carry any script once the font covers it (Fix B). Add a kana mode reachable from the Letters page whenever the record or game language is Japanese, and a Hangul jamo page with syllable composition for Korean. Composition is non-trivial, and the console's swkbd key (KeyboardKeyAction::SystemKeyboard, already in the bottom row) is a usable fallback. Choose the keyboard from the record's own language, not from a global switch: Gen III-V names must be stored in the record's language-specific character set, so the record language decides which characters are legal. A separate whole-app display-language setting (species, move and item names through getSpeciesNameLocalized) is a separate, larger feature. The search bar can then match localized names in the chosen display language, which is better than a per-search-bar language button. Each generation's writer must validate characters before any non-ASCII entry is enabled, because Gen V free Unicode entry is explicitly deferred (Gen5NativeText.h:76-77).

**Risks.**

Fix A is display-only and changes no save bytes. The main risk is accidentally widening the write gate by switching edit or replay paths from asciiName5 to the new decoder. That would let Keep/Review stage non-ASCII through encodeAsciiName5 (which refuses it), or invite a lossy re-encode. Keep the two functions distinct. Related latent issue seen in passing (not in scope, unverified on hardware): Gen5SharedScreenState.h:547 uses asciiName5 when rebuilding a record on species change, so a Japanese or Korean default name falls back to newDefaultName. Check that this picks the record-language default and not English. Fix B: the Korean shared font is large and adds glyph rasterisation into the fontstash atlas. Atlas growth is guarded per test_fontstash_allocation_contract.py, but hardware should confirm there is no memory or atlas-reset regression on screens with mixed scripts. If plGetSharedFontByType(KO) fails, behaviour must stay as today, which matches the Standard-font pattern. Owner-PASS items: the Gen IV and Gen III language cycles (header follows language) are untouched by Fix A. Fix B only adds a last fallback, so Latin and kana rendering that already passes is unchanged. H13 is marked PASS but does not cover Japanese saves.

**Open questions.**

1) Owner: should keyboard script follow the record's language (recommended, because legality and storage are per record), or should there be a global app display language that also localizes species, move and item names and search? These are separate features. A per-search-bar language button is not recommended. 2) Is Hangul nickname entry (jamo composition) wanted in v1.0, or is the swkbd fallback key enough? 3) When should Gen III-V non-ASCII and kana writers be enabled? That needs verified per-generation glyph tables, and Gen V free Unicode is explicitly deferred. 4) Hardware: confirm the PlSharedFontType_KO load works on retail firmware and renders Hangul, and re-run H13 with a real Japanese save. 5) Hardware only: which species and language the owner cycled. For IT and ES the Gen V name is identical to English for all 649 species, so 'no change' there is correct.

**Files to change:** `include/Integration/Gen5/Gen5NativeText.h (add displayName5)`, `src/UI/Gen5SharedReadOnlySurface.inc (lines 356, 821, 2066-2068, 2113-2114: display sites only)`, `src/UI/PKSEFramebuffer.cpp (load PlSharedFontType_KO as fallback; member in include/UI/PKSEFramebuffer.h)`, `tests/test_gen5_readonly_ui_contract.py (update the display-site expectations)`, `(feature, later) src/UI/Dialogs/KeyboardDialog.cpp, include/Utils/Keyboard.h, src/Utils/Keyboard.cpp`

**Existing tests affected:**
- tests/test_gen5_readonly_ui_contract.py: the source-text contract requires asciiName5/encodeAsciiName5 (lines ~1074-1082, ~1223). The edit-gate requirements must stay. Any assertion pinning asciiName5 at the chrome/detail display sites needs a deliberate update to displayName5.
- tests/test_gen5_language_default_name.cpp: pins the stored-byte D3 behaviour. It should stay unchanged and passing, which proves Fix A is display-only.
- tests/test_gen5_pk5_readonly.cpp, tests/test_gen5_readonly_save.cpp, tests/test_qa_matrix_gen5.cpp, tests/test_gen5_review_discard_export_trace.cpp: these reference asciiName5 or the 'Native glyphs' wording and should be re-checked for strings that would change.
- tests/test_ui_kana_keyboard_host.cpp, tests/test_gen12_kana_text.cpp: only affected if keyboard layouts change (Fix C).

**New tests:**
- Gen V displayName5 unit test: an un-nicknamed Pikachu staged to JA, KO, FR and DE gives a header string equal to UTF-8 of defaultSpeciesName(25, lang, 5). Also cover Nidoran♀ half-width U+246E mapping back to ♀, a 0xFFFF terminator mid-buffer, a full 10-char name with no terminator, and lone-surrogate replacement.
- Host UI test for Gen5 chrome(): after the Language stage, ChromeInfo.nickname is non-empty and differs from Names::getSpeciesName for JA, KO and FR-accent species (e.g. 495 Snivy to Vipélierre).
- Regression guard that the Gen V edit and replay paths still use the ASCII gate. A Japanese default name must not become editable free text, and Keep/Review replay of a language-only change must still succeed with the same bytes as test_gen5_language_default_name.
- Source-contract test (Python, like test_fontstash_allocation_contract.py) that PKSEFramebuffer requests PlSharedFontType_KO and registers it as a fallback of fontSans after the Standard font.
- Hardware checklist item: a Gen IV HGSS/DP Korean-language Pokemon and a Gen V Korean default name render Hangul instead of boxes. Plus H13 re-test with an actual Japanese Gen III/IV/V save.

**Skeptic corrections.**

The root cause is confirmed. I opened every cited line at HEAD 67b1b6a0 of wmain.

What checks out:
- The Gen V header uses the ASCII-only decoder. Gen5SharedReadOnlySurface.inc:821-822 calls asciiName5. Gen5NativeText.h:39 returns nullopt for any code unit outside 0x20-0x7E. SharedPokemonShell.h:48 then falls back to Names::getSpeciesName, which is hard-wired to English at SpeciesNames.cpp:9303-9306.
- The stored bytes are right. The Language stage writes the localized default name at Gen5StagedPokemonRecord.h:313-321 via writeUnicodeName5 (Gen5NativeText.h:78-98).
- Gen IV is correct. Gen4SharedPokemonSurface.inc:1522 calls utf16ToUtf8(raw.nickname()), and Pokemon4ReadOnly.cpp:99-101 decodes it through decodeGen4Field, whose table includes Hangul (Gen4Text.h:77, 0xAC00...).
- Gen IV can stage Korean. Gen4SharedPokemonSurface.inc:870-881 filters the cycle with groupHasLanguage, which allows Korean for DP/PT/HGSS (LanguageID.h:44-48). Gen III has no Korean: RS/E fall to `default: false` and FRLG returns false.
- Only PlSharedFontType_Standard is loaded (PKSEFramebuffer.cpp:102). There is no PlSharedFontType_KO anywhere in src/ or include/, so the Korean-boxes cause is confirmed.
- The other display sites are confirmed: :356-357, :2043, :2066-2069, :2113-2116.
- Replay safety is confirmed. Keep replays Language first (Gen5SharedPokemonSession.h:129-133). Review does the same (Gen5SharedReview.h:194-196). Name replay therefore only sees a real user edit, and those paths must keep asciiName5 (Session.h:150, Review.h:205).

Corrections:
1. A Gen V display-only Unicode decoder already exists and was missed: displayTrainerName in include/Integration/Gen5/Gen5TrainerName.h:25-63. It is already used by Gen5TrainerScreen.h:70, Gen5SaveInstanceAdapter.h:107, Gen5GameCardPreview.h:56, Gen5SaveDiagnostics.h:145 and Gen5ReadOnlyTrainer.cpp:33/102, and tested in tests/test_gen5_readonly_save.cpp. It handles surrogates and rejects control, private-use and noncharacter units. The fix should generalize it rather than add a parallel displayName5. For example: a displayName5(span, nickname) that reads the 0x48/0x68 field up to 0xFFFF or 0, maps U+246D/U+246E back to ♂/♀, and calls a shared length-parameterized core. Trainer names then inherit the same handling.
2. Latent gap in displayTrainerName: it does not map U+246D/U+246E. A Gen V OT or trainer name containing a half-width gender symbol would draw as ⑭/⑮. The shared core fixes this for both names.
3. Ordering risk: Fix A must ship together with Fix B, or after it. With Fix A alone, a record staged to Korean (ID 8) would change from showing the English species name to showing tofu boxes in the header. That is a visible regression of exactly what the owner reported.
4. The latent issue flagged at Gen5SharedScreenState.h:542/547 is narrower than described. recreateCreateSpecies runs only for a pending Create draft (lines 501-517). Line 542 also empties the OT name when the trainer's OT is non-ASCII, for example on a Japanese save. Whether validateCreate5 then refuses or writes an empty OT is unverified. It should be checked when H13 is re-run with a real Japanese Gen V save. This is not in scope for H27.
5. Fix A also changes H13-relevant behaviour, and the report understated it. Today any Gen V record with a Japanese or Korean nickname, i.e. any record on a JA/KO save, shows the English species name in the header and "(native glyphs)" in Details. After Fix A it shows the real nickname. That is what the owner expects, but it is new behaviour on screens nobody has hardware-tested.

The rest of the classification, the affected generations and the test inventory are accurate. The test_gen5_readonly_ui_contract.py:1074-1077 require list only checks that asciiName5/encodeAsciiName5 exist in Gen5NativeText.h. Nothing pins asciiName5 at the chrome site, so Fix A needs no edit to that contract, only new assertions.

**Missed generations or paths.**

- Shared component missed: include/Integration/Gen5/Gen5TrainerName.h:25 displayTrainerName is the existing Gen V display decoder. Reuse or generalize it. It has no U+246D/U+246E mapping today.
- Other screens already depending on it: include/Integration/Gen5/Gen5TrainerScreen.h:70 shows "(unsupported glyphs)"; Gen5SaveInstanceAdapter.h:107, Gen5GameCardPreview.h:56, src/Legacy/Gen5ReadOnlyTrainer.cpp:33/102 and src/Integration/Gen5/Gen5SourceDiscovery.cpp:537 also call it. All of them will show Hangul as tofu until PlSharedFontType_KO is loaded. Fix B therefore also covers Gen V trainer-name screens and the save list, not only Pokemon names.
- Gen5SharedReadOnlySurface.inc:2043 (Create review OT row) is in the investigation's evidence list but missing from its Fix A edit list. It should switch to the display decoder too.
- Gen5SharedScreenState.h:542 (Create species rebuild) turns a non-ASCII OT into an empty OT name. Unverified; a separate follow-up needing an H13 Japanese-save test.
- src/UI/TrainerViewScreenCompositeOverlay.cpp:103 has its own nickname-empty to English species fallback. It is worth checking whether the Gen V overlay draft nickname is fed from asciiName5. Not traced; not found to be Gen V specific.
- No other generation was missed. Gen I/II (no language byte) and Gen III (no Korean, and its header already uses p.nickname) are correctly marked as unaffected for the header bug.

</details>

### VAULT — Vault / Bank (H40-H43, H38 question) — separate v1.0 feature

- **Classification:** 4 Known missing v1.0 functionality, 2 Cross-generation UI inconsistency, 3 Newly requested behavior, 5 Safety concern, 6 Hardware test needing independent confirmation  ·  **Confidence:** high  ·  **Skeptic confirmed root cause:** yes
- **Generations:** The Vault is not implemented in any generation. What exists today is the inherited PKSE "Legacy Storage" bank (bank.dat).

In-save box rename (H40): blocked in all Gen I-V sessions that come from emulator, RetroArch or assigned-file sources. The block works differently per generation:
- Gen I: the box-name header cannot be focused. This is correct, because Gen I has no custom box names.
- Gen II: the header cannot be focused, although the save does store names and they are shown.
- Gen III (RetroArch FRLG): the header cannot be focused, and the save's own names are not even shown ("Box N").
- Gen IV: the header CAN be focused, then the rename is refused with "source is read-only".
- Gen V: Storage is refused entirely and Box names show "Box N".
The only place in-save rename works is the inherited PKSE backup-workspace path (Trainer3FRLG and the modern Switch games, BackupOrStaged). Legacy Storage box rename works in every generation that opens Storage (I-IV); this matches what the owner saw.

Deposit and withdraw between a save and the Bank (H41/H42): Gen I, II and IV do not offer it. Gen III over RetroArch does not either. Gen V refuses Storage. Bank record tags exist only for FRLG and Gen VII+, so Gen I, II, IV and V records cannot be stored in bank.dat at all. Gen III FRLG deposit and withdraw works only in the inherited Switch-backup workspace (DEVICE_TEST_FOLLOWUP 2026-09-03 records an Arbok persisting in Legacy Storage).

"Move to Vault/Bank" action (H43): absent in all Gen I-V action menus. The generic sheet defines it as NotYetSupported.

Cross-generation transfer (H38 question): none between Gen I/II/IV/V. Conversion exists only between FRLG and the Gen VII+ groups.

Search in Bank: absent in all generations.

<details><summary>Root cause, fix, tests, risks and skeptic notes</summary>

**Current behaviour.**

INVENTORY OF WHAT EXISTS

1. Storage format (Legacy Storage, not Vault): Trainer::Bank, in include/Trainer/Bank.h:1-126 and src/Trainer/Bank.cpp (673 lines).
- File: sdmc:/switch/PokeBank-NX/bank/bank.dat.
- 100 boxes x 30 slots (BankFormatPolicy.h:8, Bank.h:41-42). Each record is a fixed-size u32 group tag plus the native ENCRYPTED payload (Bank.cpp:46-47). A names section is appended after the records (Bank.cpp:231-240).
- Taggable groups are GG, SWSH, BDSP, PLA, SV, ZA and FRLG only (bankTagFor, Bank.cpp:92-114; record sizes at 122-128; makePokemon at 146-152). There are NO tags for RBY, GSC, DP/PT/HGSS or BW/B2W2.

2. Persistence and safety:
- load() classifies the file into a LoadOutcome enum (Bank.h:30-40): FirstRun, Loaded, Restored, ReadFailed, Quarantined, Truncated, DamagedRecords, NewerLayout, Blocked.
- Writes are blocked fail-closed (isWriteBlocked, Bank.h:100-103).
- save() goes through ensureLegacyBankRoot and then buildVerifiedImage. That function runs serialize(), verifyImage() (a per-slot round trip) and validateStorageImage (Bank.cpp:600-612). The durable write then runs a validator (Bank.cpp:626-656).
- Dirty tracking compares against savedImage (Bank.h:69, 112).
- Storage exit offers Save / Discard / Cancel (TrainerViewScreenBase.inc:4290-4345). Discard is bank-only, and withdrawn copies stay in the save (4330-4337).

3. Custody and crash recovery:
- A cross-store true Move between the Bank and the workspace uses the durable MoveTransaction engine: baseline digests, prepare, commit, journal, and a recovery lock (TrainerViewScreenBase.inc:820-1000).
- returnHeldToOrigin plans the whole rollback before moving anything (1047-1129).
- Cross-game true Move into the save pane is refused (879-881).
- Recovery files UI: include/UI/RecoveryManagementModel.h.
- There is NO Vault ID, no SHA-256 entity store and no provenance log. MASTER_VAULT_SPEC.md and TRANSFER_MODEL.md are design documents only.

4. Box naming:
- Bank boxes: renameBankBox (TrainerViewScreenBase.inc:1579-1598) is gated by requireMutableStoragePane(1, Rename). Pane 1 maps to AppOwnedStorage (MutationTargetPolicy.h:25-30), which is always mutable (SourceMutationPolicy.h:32). Names are UTF-8, at most 24 characters (Bank.h:76), and blank means "Bank N".
- Save boxes: renameBox (TrainerViewScreenBase.inc:1540-1572) first calls requireMutableWorkspace(). Header focus is gated on trainer.supportsBoxNames() at 2448 (storage pane 0), 4438 (storage touch), 4673 and 4751 (Boxes view).
- Native box-name support per generation:
  - Gen I: no custom names. RBYReadOnlyTrainer.cpp:39-42 uses "Box N" and does not override supportsBoxNames, so it inherits false (Trainer.h:399).
  - Gen II: native names are decoded (Gen2ReadOnlySave.cpp:134, 475; GSCReadOnlyTrainer.cpp:193-202). supportsBoxNames is not overridden, so it is false.
  - Gen III FRLG RetroArch: the save stores names, but FRLGReadOnlyTrainer.cpp:67-70 fills "Box N" and never decodes them. supportsBoxNames is false.
  - Gen IV: native names are decoded (Gen4ReadOnlySave.cpp:278-285, Gen4ReadOnlyTrainer.cpp:143-150). supportsBoxNames is true (Gen4ReadOnlyTrainer.h:38), but updateBoxNameBlock is a no-op (Gen4ReadOnlyTrainer.h:27).
  - Gen V: the save stores names, but Gen5ReadOnlyTrainer.cpp:48-50 fills "Box N". supportsBoxNames is false (Gen5ReadOnlyTrainer.h:37).
  - No Gen II-V native box-name WRITER exists. The only box-name hits in src/Integration are readers.

5. Deposit and withdraw:
- Implemented only in the inherited dual-pane Storage view (handleStorageInput, TrainerViewScreenBase.inc:2435 onward).
- For legacy sources, X/Y on the save pane are stripped (2437-2441). sourceKind is RetroArchLegacy or ExternalLegacy for every classic Gen I-V source (UI.cpp:678-679, 785, 854, 941), and canPerform is false for those (SourceMutationPolicy.h:30-33).
- The Gen IV carry grid keeps carries inside the save pane (ClassicPackedMoveOverlay.inc:141-155).
- Gen V routes the Storage tile to StorageRefused with the message "Legacy Storage (Bank) does not accept Gen V PK5 records" (Gen5HubRouting.h:20,46; Gen5SharedReadOnlySurface.inc:1534-1540).
- Gen II passive details deliberately exclude the Bank (Gen2SharedSurfaceParity.inc:17-20).

6. "Move to Vault / Add to Bank" action:
- PokemonAction::AddToMasterVault, AddToBank and TransferToGame all dispatch to NotYetSupported (ActionSheetModel.h:24-34, 83-89).
- The Gen IV narrow menu explicitly hides the Vault/Bank/Transfer placeholders (Gen4SharedPokemonSurface.inc:966-969).
- The Gen I menu labels (Gen1PokemonEditorOverlayUX.inc:1009-1010) contain no Bank or Vault entry.
- Product Home labels the pill "Legacy Storage ... app-owned / not Master Vault" (HomeMenuPanel.cpp:119; AppShellScreen.cpp:674).

7. Search:
- The Product Home "Search" section is a static preview: "Search will become available when the Master Vault index is ready" (AppShellScreen.cpp:673, 686-698; OrganizationPreviewModel.h:22-37).
- There is no search or highlight in StoragePanel.cpp or the Bank.
- PKSE trees in history (1133aedf v1.1.3, 70069625) contain no bank search code. The owner's "PKSE search with highlight" refers to an upstream version that is NOT in this repo.

8. Cross-generation transfer:
- Conversion::convert supports only FRLG <-> the GG/SWSH/BDSP/PLA/SV/ZA groups (Convert.cpp:53, 67, 1138-1238).
- No Gen I, II, IV or V converter exists.
- Standalone .pkN import refuses the wrong generation; the owner passed this as H38.

**Root cause.**

H40 (in-save box not selectable or renamable; only Legacy Storage boxes rename). This is intended behaviour, not a regression, but the UI is inconsistent across generations. There are two independent gates:

(a) Capability. Header focus requires trainer.supportsBoxNames() (TrainerViewScreenBase.inc:2448, 4438, 4673, 4751). The RBY, GSC and FRLG read-only adapters inherit the default false (Trainer.h:399), and Gen V returns false explicitly (Gen5ReadOnlyTrainer.h:37). The cursor therefore can never land on the save header. That is exactly the owner's "it doesn't allow me to select it".

(b) Source policy. Even where the header is focusable (Gen IV: Gen4ReadOnlyTrainer.h:38), renameBox calls requireMutableWorkspace() (TrainerViewScreenBase.inc:1541), which uses canPerform(RetroArchLegacy/ExternalLegacy, Rename) and gets false (SourceMutationPolicy.h:30-33). Rename is refused with "source is read-only".

Behind both gates there is no writer:
- Gen4ReadOnlyTrainer::updateBoxNameBlock is a no-op (Gen4ReadOnlyTrainer.h:27).
- No Gen II, III or V staged writer for box names exists.

The Legacy Storage pane renames because pane 1 maps to AppOwnedStorage (MutationTargetPolicy.h:25-30), which canPerform always allows. tests/test_source_mutation_policy.cpp:52-55 pins this.

H41/H42 (deposit and withdraw): there is no UI path from any classic Gen I-V source.
- Save-pane pickup is stripped for legacy sources (TrainerViewScreenBase.inc:2437-2441).
- The Gen I-IV carry overlays are confined to the save pane.
- Gen V refuses Storage (Gen5HubRouting.h:46).
- The Bank format has no record tags for RBY, GSC, Gen IV or Gen V (Bank.cpp:92-114).

H43: the "Move to Vault/Bank" actions are defined but return NotYetSupported (ActionSheetModel.h:83-89). The classic menus omit them on purpose (Gen4SharedPokemonSurface.inc:966-969).

H38 question: there is no Gen I-to-Gen V route, because no Gen I/II/IV/V converter exists (Convert.cpp handles only FRLG and Gen VII+).

Latent safety issue, found while investigating:
- Bank::serialize silently skips any occupied slot whose group has no tag ("unknown group -> can't tag it, skip", Bank.cpp:213-214).
- verifyImage then treats that slot as empty (Bank.cpp:497-498), so the verified save passes and the Pokemon disappears from bank.dat.
- The cross-store Move destination path for pane 1 clones without checking whether the Bank can store the group (TrainerViewScreenBase.inc:883-893), and prepareForPane returns useOriginal for the Bank with no eligibility check (1167-1172).
- I found no reachable path from a Gen I-V legacy source today, because of the gates above. However, any v1.0 deposit work, or a future source kind that is BackupOrStaged for RBY/GSC/Gen4/Gen5, would hit a silent loss.

**Shared owner.**

These pieces should own the work:
- Bank eligibility: Trainer::Bank (include/Trainer/Bank.h, src/Trainer/Bank.cpp). It should expose one canStore(GameVersion) predicate, built from bankTagFor, and use it to fail closed.
- Save-box rename availability: the Trainer capability interface (include/Trainer/Trainer.h:399-471, supportsBoxNames, getMaxBoxNameLength and canStoreBoxName), combined with the source policy, in one TrainerViewScreen predicate that every header-focus site uses (TrainerViewScreenBase.inc:2448, 4438, 4673, 4751 and renameBox at 1540).
- The future Vault: a new Vault module that implements MASTER_VAULT_SPEC.md. It should not be grown out of the PKSE Bank class or the per-generation surfaces.

**Proposed fix.**

REPAIR CAMPAIGN (minimal; does not implement the Vault)

R1. Make the save-header affordance honest and the same for every generation.
- Add one predicate in TrainerViewScreen: saveBoxRenameAvailable() = trainer.supportsBoxNames() && canPerform(sourceKind, Rename).
- Use it at TrainerViewScreenBase.inc:2448, 4438, 4673 and 4751 in place of the bare supportsBoxNames().
- Result: Gen IV legacy sources stop focusing a header that is always refused, which matches Gen I, II, III and V.
- Optionally, when a user presses A or Up toward an unavailable save header in Storage, post one status line: "Box names in this save are read-only in this version. Legacy Storage box names can be renamed." This explains the behaviour without enabling anything.
- No byte-level behaviour changes.

R2. Fail-closed Bank eligibility (safety hardening).
- Add a public static Bank::canStore(GameVersion), equal to bankTagFor(g) != BTAG_EMPTY.
- In serialize/verifyImage, count an occupied slot that cannot be tagged as a verify FAILURE rather than an empty slot. That way buildVerifiedImage and save() refuse instead of silently dropping it.
- In the cross-store Move with destPane==1 (TrainerViewScreenBase.inc:883) and in prepareForPane(destPane!=0) (1167), refuse with "Legacy Storage cannot hold <generation> records" when !Bank::canStore(mon.getGameGroup()).
- This changes nothing for currently storable groups.

OUT OF REPAIR SCOPE: SEPARATE v1.0 VAULT PLAN GAP LIST

1. Persistent storage: a Vault entity store with stable IDs, a SHA-256 per raw payload, a manifest and indexes (MASTER_VAULT_SPEC.md section 3). None of this exists. bank.dat is a fixed table of 100x30 slots.

2. Native record preservation for Gen I, II, IV and V:
- bank.dat cannot hold PK1/PK2/PK4/PK5. Choose a format-neutral, raw-bytes-plus-metadata entity rather than adding more PKSE tags.
- Define the canonical stored form for Gen I/II: a box record vs a party record, plus OT/nickname bytes, the language/region, and the Gen II Crystal tail.

3. Deposit and withdraw: Game -> Vault as COPY first (TRANSFER_MODEL.md "current alpha rule"). This needs:
- action-menu entries in every Gen I-V surface, through one shared action owner (NarrowPokemonActionMenu / ClassicPokemonActionMenu), not per-surface copies;
- withdraw to an empty destination only, with a preview, a cancel and a staged Review entry.

4. Custody: reuse the MoveTransaction journal and recovery lock for Vault<->workspace. True Move stays disabled until per-game writers are proven (#20).

5. Crash recovery: extend the Bank LoadOutcome taxonomy and the RecoveryManagement UI to the Vault store. Keep a quarantine for unreadable entities.

6. In-save box naming:
- Native writers for Gen II (8 characters plus terminator, Gen II charset; also JP), Gen III (decoded, then written in the Gen 3 charset via canStoreBoxName), Gen IV (40-byte UTF-16 field, Gen4ReadOnlySave.cpp:280) and Gen V. All must be staged and appear in Review.
- Display gaps to fix first: Gen III and V show "Box N" instead of the save's real names.
- Gen I has no box names; keep it read-only and say so.

7. Vault and Bank box naming: keep it app-owned, UTF-8 and with no game charset limit, as Legacy Storage does now.

8. Search: a name/species/item index with highlight in grid views. This is the owner's PKSE-style request. There is no PKSE reference code in this repo; it needs the upstream source or a spec from the owner.

9. Cross-generation transfer eligibility: encode the official one-way routes as a rule table:
- I <-> II (Time Capsule; only Gen I-compatible species, moves and items);
- III -> IV (Pal Park);
- IV -> V (Poké Transfer);
- I/II -> VII (Poké Transporter, Virtual Console).
- There is no official I/II -> III/IV/V route. A Gen I-to-Gen V request must be refused or explicitly marked non-canonical.
- Conversion today covers only FRLG and Gen VII+.

10. Conversion verification: dry-run preflight (Convert.h:80-88 preflightConvert exists for the modern/FRLG groups), round-trip fidelity reports (Fidelity.h, RouteEvidence.h), and provenance events per derived entity.

**Risks.**

Owner-PASS behaviour:
- H38 PASS (cross-generation import refusal) is untouched.
- H12/H44 recovery displays and Bank save/load (DEVICE_TEST_FOLLOWUP 2026-09-03, Legacy Storage persistence PASS) must not regress. R2 changes only the handling of groups that cannot be tagged, and those never round-tripped anyway.
- Inherited Gen VII+ and Switch-FRLG backup-workspace deposit, withdraw and rename keep working, because those groups are taggable and BackupOrStaged passes the rename policy.

R1 risks:
- R1 removes header focus in Gen IV legacy sessions. Any owner habit of seeing the Gen IV header highlight changes. Rename was always refused there, so no capability is lost.
- If R1 is applied without the status line, users still get no explanation. The status line is recommended.

Save safety:
- Enabling in-save rename is NOT a repair. It requires native writers and checksums, the staged Review path and Export Copy. Doing it ad hoc risks corrupting Gen II/III/IV/V box-name blocks and checksums.
- Do not give Gen4ReadOnlyTrainer a real updateBoxNameBlock outside the staged writer.

Vault scope:
- Do not grow the Vault out of bank.dat. Its fixed tagged table cannot represent Gen I/II/IV/V or provenance.

**Open questions.**

1. Which game and source did the owner use for H40? Behaviour differs: Gen IV focuses the header and then refuses, while Gen I, II, III and V never focus it. Ask for a hardware confirmation of the game, provider and Storage vs Boxes view (category 6).

2. Product decision: should in-save box renaming (Gen II-V, staged and exported via Export Copy) be part of v1.0, or stay read-only with an explanation? Gen I has no box names natively, so it stays read-only either way.

3. Should Gen III and Gen V box headers show the save's native box names (currently "Box N") before v1.0? This is display-only, but each needs a decoder plus tests.

4. Gen I-to-Gen V transfer (H38 question): there is no official route. The canonical paths are I<->II via Time Capsule and I/II -> VII via Poké Transporter; III -> IV -> V via Pal Park and Poké Transfer. Should PokeBank NX refuse it, or offer an explicitly non-canonical conversion that is marked in provenance?

5. Bank search: the owner cites a PKSE search with letter/item highlighting. That code is not present in any PKSE commit in this repo's history (checked 1133aedf v1.1.3 and 70069625). Please provide the upstream PKSE version, or confirm the expected behaviour (match on nickname/species/held item; highlight vs filter; current box vs all boxes).

6. Naming: the UI says "Legacy Storage (app-owned / not Master Vault)" while the owner says "bank/vault". Decide the v1.0 user-facing name, and whether bank.dat migrates into the Vault or stays a separate legacy area.

**Files to change:** `src/UI/TrainerViewScreenBase.inc (R1: lines 2448, 4438, 4673, 4751 use one saveBoxRenameAvailable() predicate; optional explanatory status. R2: refuse non-storable groups at 883-893 and 1167-1172)`, `include/UI/TrainerViewScreenBase.h (declare saveBoxRenameAvailable() next to requireMutableWorkspace at ~366)`, `include/Trainer/Bank.h (R2: public static canStore(GameVersion))`, `src/Trainer/Bank.cpp (R2: serialize 213-214 and verifyImage 497-498 count an untaggable occupied slot as a failure, not an empty slot)`, `Vault v1.0 (separate plan, not repair): new Vault module, Gen II/III/IV/V native box-name writers, Gen III/V box-name decoders (src/Legacy/FRLGReadOnlyTrainer.cpp:67-70, src/Legacy/Gen5ReadOnlyTrainer.cpp:48-50), shared action-menu entries`

**Existing tests affected:**
- tests/test_source_mutation_policy.cpp:20-55 pins that legacy sources cannot Rename while the Bank pane (pane 1) can. This must stay unchanged.
- tests/test_gen5_trainer.cpp:349,353 pins Storage routing to StorageRefused for Gen V. Unchanged.
- tests/test_gen5_readonly_ui_contract.py:107-113 (forbids renameBox( in the Gen V surface) and line 148 (StorageRefused case). Unchanged; R1 must not add renameBox to the Gen V surface.
- tests/test_gsc_readonly_bridge.cpp:174,283 pins that GSC native box names are decoded ('A'). Unchanged.
- tests/test_qa_matrix_gen4.cpp:579 and tests/test_gen4_readonly_foundation.cpp:338 pin Gen IV box-name decoding (18 names, u"BOX"). Unchanged.
- tests/test_bank_format_policy.cpp, test_bank_load_policy.cpp, test_bank_fault_injection.cpp, test_bank_record_validation.cpp and test_bank_recovery_contract.cpp pin Bank load/save/quarantine behaviour. R2 changes how verifyImage counts failures only for groups that cannot be tagged; re-run all of these. If any fixture deliberately places an untaggable mon and expects a silent skip, update it with justification that silent loss is a defect.
- tests/test_app_shell_model.cpp and tests/test_organization_preview_model.cpp pin Search as a 'Coming Soon' preview. Unchanged by the repair.

**New tests:**
- Host test for the header-rename capability matrix: for each adapter (RBY, GSC, FRLG-ReadOnly, Gen4-ReadOnly, Gen5-ReadOnly) crossed with sourceKind (RetroArchLegacy, ExternalLegacy, BackupOrStaged), saveBoxRenameAvailable() is false for every legacy source and the Gen IV legacy header is not focusable. The Bank pane stays renamable for every sourceKind.
- Static/contract test: every save-header focus site in TrainerViewScreenBase.inc uses saveBoxRenameAvailable(), with no bare trainer.supportsBoxNames() left at 2448/4438/4673/4751.
- Bank test: put an object whose group cannot be tagged (e.g. Pokemon1ReadOnly/RBY or Pokemon2ReadOnly/GSC) into bank->boxes. buildVerifiedImage() must return false, save() must return false, and bank.dat must stay byte-unchanged (today it silently drops the slot).
- Bank::canStore table test: true for GG, SWSH, BDSP, PLA, SV, ZA and FRLG; false for RBY, GSC, DP, PT, HGSS, BW and B2W2.
- Cross-store Move refusal test: Move to pane 1 with an RBY/GSC/Gen4/Gen5 held mon is refused before any destination is staged, no MoveTransaction is allocated, and custody is unchanged.
- Vault v1.0 (later): a transfer-route eligibility table test (I<->II, III->IV, IV->V, I/II->VII allowed; I/II->III/IV/V refused), per-generation box-name writer round-trip tests, and search index/highlight tests.

**Skeptic corrections.**

I checked the main citations and they hold.

**H40 rename gates are confirmed.**
- Header focus is gated by trainer.supportsBoxNames() at src/UI/TrainerViewScreenBase.inc:2448 (headerRenamable), 4438 (touch on the name pill), 4673 (canFocusHeader) and 4751 (tap 1002).
- The default is false at include/Trainer/Trainer.h:399. RBY, GSC and FRLG-ReadOnly do not override it (grep of include/Legacy finds overrides only in Gen4ReadOnlyTrainer.h:38, which returns true, and Gen5ReadOnlyTrainer.h:37, which returns false).
- Gen4ReadOnlyTrainer.h:27 and Gen5ReadOnlyTrainer.h:28 both have an empty updateBoxNameBlock.
- renameBox at TrainerViewScreenBase.inc:1539-1541 calls requireMutableWorkspace() first. That reaches requireMutationKind at include/UI/TrainerViewScreenBase.h:352-365, which posts "<provider> source is read-only."
- canPerform is in include/Safety/SourceMutationPolicy.h:30-33. The investigation cites a path, include/UI/SourceMutationPolicy.h, that does not exist; the file is under include/Safety/.
- renameBankBox at TrainerViewScreenBase.inc:1579-1580 uses pane 1, which mutationSourceForStoragePane (include/UI/MutationTargetPolicy.h:25-30) maps to AppOwnedStorage.
- tests/test_source_mutation_policy.cpp:48-55 pins this. Line 55 is the Rename assert; the investigation cites 52-55, which is correct.
- The legacy sourceKind assignments are confirmed at src/UI/UI.cpp:678-679, 785, 854 and 941.
- Gen V's fill is at src/Legacy/Gen5ReadOnlyTrainer.cpp:47-49, slightly off from the cited 48-50. FRLG's "Box N" fill is confirmed at FRLGReadOnlyTrainer.cpp:67-70.

**H41/H42 are confirmed.**
- In Storage, TrainerViewScreenBase.inc:2436-2441 strips X/Y for the save pane.
- pickupMulti (2102) and grabSelection (2121) both call requireMutableStoragePane(pane, DirectMove). That refuses pane 0 for legacy sources, so even a copy-mode pickup is impossible.
- ClassicCarryGrid (include/UI/ClassicCarryGrid.h:31-34, 46-60) keeps carries inside the save pane. The static_assert at line 63 reads "Bank pane never".
- Gen V StorageRefused is at Gen5SharedReadOnlySurface.inc:1535-1540. It sets selectedMode=Storage and posts a status; it does not open the view.
- Bank tags exist only for GG, SWSH, BDSP, PLA, SV, ZA and FRLG (src/Trainer/Bank.cpp:92-103).

**H43 is confirmed.** ActionSheetModel.h:83-88 returns NotYetSupported. Gen4SharedPokemonSurface.inc:966-969 hides the placeholders. The Gen I action labels at Gen1PokemonEditorOverlayUX.inc:1008-1010 have no Bank or Vault entry.

**The latent silent-loss problem is confirmed and is worse than reported.**
- Bank.cpp:213-214: serialize skips any occupied slot it cannot tag.
- Bank.cpp:496-501: verifyImage defines "occupied" with `bankTagFor(...) != BTAG_EMPTY`, so that slot counts as empty and verification passes.
- What the investigation missed: Bank::hasChanged() (Bank.cpp:670-672) is `serialize() != savedImage`. Placing a mon that cannot be tagged therefore never makes the Bank dirty. The Storage exit Save/Discard prompt (TrainerViewScreenBase.inc:4290 onward) would not appear at all, and the mon disappears on exit or reload with no prompt.
- R2 as written ("count as a verify failure in serialize/verifyImage") fixes the save path but not hasChanged(). The minimal correct change has three parts:
  1. In verifyImage, define occupied as `src && src->speciesID()!=0`, dropping the tag condition. groupForBankTag(BTAG_EMPTY) then fails and is counted as a failure.
  2. hasChanged() must return true, or the exit flow must refuse, when any occupied slot cannot be tagged.
  3. Refuse at the insertion points, not only at save.
- There is also a drop path the investigation did not name. In putDownBlock (TrainerViewScreenBase.inc:2256-2330), copy mode (`carriedCopyMode`, set at 2141) skips tryCommitCrossStoreMove and calls prepareForPane(..., pane). For pane 1 that returns useOriginal with no check (1167-1172), then calls commitNativeSwap. This copy/duplicate path to pane 1 needs the same Bank::canStore refusal as the cross-store Move clone path (883-893). The proposed guard at 1167 does cover it, but the new tests must include a copy-mode drop, not only a true Move.
- I agree that no legacy Gen I-V path can reach this today, because of the DirectMove gates above.

**R1 has a scope risk the investigation missed.**
- The proposed saveBoxRenameAvailable() = supportsBoxNames() && canPerform(sourceKind, Rename) also changes the modern games: SWSH, LA, BDSP, SV, ZA and Switch FRLG all return supportsBoxNames=true (Trainer8SWSH.h:146 and the others).
- Sessions with sourceKind InstalledGame or HistoricalBackup (UI.cpp:610-611) currently focus the header and are then refused by requireMutableWorkspace. Under R1 they would lose header focus too.
- This contradicts the claim that R1 only affects Gen IV legacy sessions. The owner's PASS items for installed-game or historical browsing need checking.
- BackupOrStaged is unaffected.
- Gen IV note: the classic carry grid owns input only while carrying or on Y/touch-drag (ClassicPackedMoveOverlay.inc:126-160). Up navigation to the header in Boxes and Storage still goes through the base code at 4673 and 2448, so Gen IV's "focus then refuse" is real.

**Search provenance is weaker than stated.**
- The investigation checked only PKSE 1133aedf (v1.1.3) and 70069625.
- docs/audits/PKSE_1_2_X_GEN15_GAP_MATRIX.md (commit 8194e6ee) pins upstream PKSE v1.2.0 (55039848), v1.2.1 (15b48b69) and version-1.2.2 (a79fe61a), and it has no Bank-search row.
- So the owner's PKSE search is either newer than 1.2.2 or was missed by that matrix. It is not just "not in this repo's history". The question to the owner should name these pins.

**Classification is correct:** 4 (main), 2 (Gen IV focuses the header then refuses; the others never focus it), 3 (search), 5 (latent Bank silent drop), 6 (which game was used for H40).

**Missed generations or paths.**

These paths and generations were not covered:

1. **hasChanged() goes through serialize()** (src/Trainer/Bank.cpp:670-672). A mon that cannot be tagged never marks the Bank dirty, so the Storage exit prompt is skipped entirely. The fix must cover hasChanged() and the exit flow, not just buildVerifiedImage and verifyImage.

2. **Copy-mode drop into the Bank pane.** putDownBlock (TrainerViewScreenBase.inc:2269-2325) with carriedCopyMode bypasses tryCommitCrossStoreMove, then reaches prepareForPane useOriginal (1167-1172) and commitNativeSwap. New tests must cover this as well as the true-Move clone path (883-893).

3. **R1 also hits Gen VII+ and Switch FRLG InstalledGame and HistoricalBackup sessions** (UI.cpp:610-611; supportsBoxNames=true in Trainer8SWSH.h:146, Trainer8LA.h:152, Trainer8BDSP.h:53, Trainer3FRLG.h:83, Trainer9SV.h:145 and Trainer9LZA.h:147). Header focus would disappear there too. Either restrict R1 to legacyReadOnlySource() (TrainerViewScreenBase.h:344-347), or confirm the owner has no PASS that depends on it.

4. **Bank::canStore is needed at every insertion point, not just two.** The other sites are storageSlot writes at TrainerViewScreenBase.inc:947 (Move commit), 2428 (sort/compaction, same pane only) and 2891 (creator buildDefaultMon into pane 1). The creator uses saveOriginVersion(trainer), so for a legacy trainer with creator.pane==1 the group cannot be tagged. That path is gated today only by whatever gates the creator for pane 1, which I did not verify. It needs a test.

5. **Gen II has its own box surfaces** (Gen2UnifiedPokemonWorkspace.inc, Gen2SharedSurfaceParity.inc). Any future in-save rename must check whether Gen II draws or focuses the header outside the four base sites. I did not find any focus site there, but did not exhaustively audit it.

6. **Upstream search provenance.** The PKSE v1.2.0, v1.2.1 and v1.2.2 pins in docs/audits/PKSE_1_2_X_GEN15_GAP_MATRIX.md were not cited. None of them contains a Bank-search row.

7. **Minor citation fixes:** the path is include/Safety/SourceMutationPolicy.h, not include/UI/; the Gen V box-name fill is at Gen5ReadOnlyTrainer.cpp:47-49.

</details>

