# PokeBank NX — Round 3 physical Switch hardware results

**Owner-tested current NRO, reported October 20, 2026.**
**Baseline:** NRO source commit \`62a2f8c34476062ece344ef2188ab6b385369a7d\`.
**Results:** **110 checks — 70 PASS, 37 FAIL, 3 NOT TESTED.**

This is the owner's authoritative hardware feedback, copied from the conversation. **PASS means the owner verified it on hardware, not permission to rewrite it. FAIL means reproduce/fix the described behavior. NOT TESTED means do not claim a pass or failure.** Treat note-qualified PASS entries as requests for minor polish. Do not merge separate legality engine PR #314, change original saves, or rewrite the 70 passes.

## 1. Old failures — retest the fixes

- **B01 — PASS — Gen I Add Item**
- **B02 — PASS — Gen II Add Item**
- **B03 — PASS — Gen III Add Item**
- **B06 — PASS — Item quantities**
- **S01 — PASS — Gen V PP labels**
- **S03 — PASS — Gen V Details panel**

## 2. Old tests never previously verified

- **B04 — PASS — Gen IV/V Add Item**
- **E03 — FAIL — Origin & Legality wording.** Owner: Gen I says legality not checked in View/Edit and Origin & Legality. Gold/Silver/Crystal party View sometimes says Legality OK but created Pokémon aren't checked. Gen III like Gen I. Gen IV separate PK4 structure PASS/Legality Incomplete popup with moves. Gen V richer popup including met location/level/date/encounter but incomplete. **Unify legality layout for every generation; full separate legality engine isn't integrated yet. Do not fabricate clean verdicts.**
- **E11 — NOT TESTED — Shiny/Gender rerolls.** Owner uncertain about Unown's odd displayed stats; has provided screenshot evidence, see additional findings below.
- **L06 — FAIL — Touch Party Actions.** Touch goes directly to View; expected Actions popup.
- **S04 — PASS — Gen V Review / Discard / Export.** Owner wants Gen V review workflow added to all Gen I–IV.
- **S06 — NOT TESTED — Exported save loads.** Owner explicitly declines live emulator-save write testing right now.

## 3. Old Round 2 changes

- **R2-MENUS — PASS — Compact selection windows**
- **R2-SEARCH — PASS — Species live search**
- **R2-GOLD — PASS — Read-only field colors**
- **R2-SPECIAL — FAIL — Gen II Special stats.** Sp. Atk/Sp. Def now numeric, but cramped as \`10/9\` in the calculated-stat column, looks wrong and potentially affects adjacent stat fields. Gen II shares one Special DV and one Stat Exp, with two calculated stats: display them cleanly, do not invent a second DV.
- **R2-TRAINER — FAIL — Gen I Trainer Name/Money.** No Review; Review needs adding to Gen I–IV.
- **R2-LAUNCH — PASS — Game Launch confirmation**
- **R2-EMUBACKUP — FAIL — Emulator backup safety.** Existing screen is weird and nonfunctional. Restore earlier PKSE-style save/backup browser: select a game, inspect available saves, browse current save and deliberately create backup. **Opening an unchanged save must not automatically create an unnecessary backup.**
- **R2-SCROLLBAR — PASS — Touch scrollbar dragging**
- **R2-MOVECHOOSER — PASS — Extra rows in Move list**
- **R2-REROLL — FAIL — Shiny reroll safety.** Owner questions whether shiny stats are game-legitimate; independently audit DV/PID/shiny/Unown form calculations without asserting an unproven defect.
- **R2-CRYSTAL — PASS — Crystal Met Time**
- **R2-GEN5HEADER — PASS — Gen V exact-game title**
- **R2-REVIEWMOVE — FAIL — One Review row per Move edit.** One row now works, but displayed move changes use numeric IDs instead of readable names; show old/new move and item names etc.

## 4. Previous passes — check for regressions

- **C01 — FAIL — A selection works.** Gen V Home A opens Choose Save popup instead of current already-assigned save directly; duplicate with NEW-16.
- **C02 — PASS — B doesn't accidentally quit**
- **C03 — PASS — Only Y launches games**
- **C04 — PASS — L/R game navigation**
- **C05 — PASS — Gen II Create controls**
- **C06 — PASS — Popup input stays inside popup**
- **C07 — FAIL — Backup deletion confirmation.** Current backup screen has no expected action; restore older PKSE-style browse/choose/save backup screen.
- **C08 — PASS — Pokémon Release confirmation safety**
- **C09 — PASS — Second controller inputs**
- **M01 — PASS — Gen I Moves layout**
- **M02 — PASS — Gen II Moves layout**
- **M03 — PASS — Gen III Moves layout**
- **M04 — FAIL — Gen IV Moves layout.** New category element crowds selected Move Power/Accuracy section.
- **M05 — FAIL — Gen V Moves layout.** Same category presentation issue as Gen IV.
- **M06 — PASS — Move picker details**
- **M07 — PASS — PP and PP Ups limits**
- **M08 — PASS — Gen I hidden Move rows**
- **B05 — PASS — Unique items cannot stack**
- **B07 — PASS — Bag removal safety**
- **B08 — FAIL — Bag Review and Export.** Owner reports Export does nothing across generations; investigate event handling, output path, error/reporting and safe export separately from source writes.
- **E01 — PASS — Shiny isn't a duplicate quick action**
- **E02 — FAIL — Pokémon Actions order.** Some expected Actions menus missing; related to L06/NEW-20.
- **E04 — PASS — No obstructive row counters**
- **E05 — FAIL — Colored selection footers.** Some windows and Help need updated glyphs/unused buttons removed.
- **E06 — PASS — Shiny toggle**
- **E07 — PASS — Pokérus cycles with A**
- **E08 — PASS — Gender editor**
- **E09 — PASS — Gen III Language**
- **E10 — PASS — Stat reroll with Y.** Owner still uncertain shiny-derived statistics are correct; audit while preserving working input.
- **E12 — PASS — Gen I starting Pikachu**
- **E13 — PASS — Gen III PID/Unown stability**
- **L01 — PASS — Gen II DATA layout**
- **L02 — PASS — Crystal location picker.** Further compactness wanted.
- **L03 — PASS — Gen III location names.** Unnecessary \`not stored\` filler; too much blank space in modal.
- **L04 — PASS — Gen IV/V location picker.** Scrolling/look good; compact unused space.
- **L05 — PASS — Editor border alignment**
- **N01 — PASS — First scroll input**
- **N02 — PASS — Hold to scroll**
- **N03 — PASS — Touch and joystick work together**
- **N04 — PASS — Long list search/repeat**
- **S02 — PASS — Gen V shows real saved data**
- **S05 — FAIL — Original save protection.** Owner cannot verify backup protection because backup flow isn't working; not evidence of actual original-save corruption.

## 5. Older screenshot bugs and checks

- **R2-BUG-001 — PASS — Trainer Edit button missing**
- **R2-BUG-002 — FAIL — Gen III/IV Trainer Name and Money.** No way to access Review; exiting shows Discard/Cancel only, even with staged changes.
- **R2-BUG-003 — PASS — Gen V Trainer clutter.** Owner wants Play Time and Pokédex on Gen I–IV too.
- **R2-BUG-004 — PASS — Species change keeps old Moves**
- **R2-BUG-005 — PASS — TM/HM move information**
- **R2-BUG-006 — PASS — Compact item grid.** Short lists (fewer than ~20 entries) should use better balanced rows/columns instead of awkward long sparse rows.
- **R2-BUG-007 — PASS — Colored Move types**
- **R2-BUG-012 — PASS — All clickable popup rows boxed**
- **R2-BUG-013 — PASS — Colored controller buttons in Help.** Some windows still need light cleanup.
- **R2-BUG-008 — PASS — Ruby Pikachu location investigation.** Owner sees only Level 26 Hoenn Safari Region option.
- **R2-CHECK-009 — PASS — Yellow Move Pokémon reachable.** Y move works in Gen I.
- **R2-CHECK-010 — PASS — Sapphire Super Rod pocket warning.** Warning needs correctly colored controller glyphs.
- **R2-CHECK-011 — FAIL — Black 2 VGE location names.** Black 2 Pikachu picker shows 276 locations, likely too broad; verify native location ID vs actual encounter viability.

## 6. NEW bugs found on the CURRENT NRO

- **NEW-01 — PASS — Help should close with B.** B works now; remaining minor fix is remove redundant \`- Close/Help\` footer everywhere.
- **NEW-02 — FAIL — Remove Touch:Back text.** Remove obsolete footer across View/Overview and all windows while retaining touch functionality.
- **NEW-03 — FAIL — Bag Remove and warning buttons.** Colored A/B glyphs inside dedicated Remove and Key Item Warning dialogs. Bag Remove confirmation should use A (not Y) to confirm and B to cancel. Don't repeat modal option buttons in bottom footer. Do not change other destructive-action safeguards.
- **NEW-04 — FAIL — Gen II Move Pokémon broken.** Gold/Silver/Crystal PC box pickup/drop/move between slots/boxes not completing. **Gen I and Gen III move work—do not rewrite them.**
- **NEW-05 — FAIL — Species keyboard blocks list.** Put search keyboard under species artwork/shift art upward, keep list visible.
- **NEW-06 — FAIL — Held Item needs search.** Add search alongside sorting for supported held-item generations.
- **NEW-07 — FAIL — Gen II OT Gender popup.** A cycles native-supported values inline rather than opening 3-option popup; do not fabricate absent native fields.
- **NEW-08 — FAIL — Ball picker scrollbar crowded.** Rebalance row padding/scrollbar space.
- **NEW-09 — FAIL — Nature stat changes missing.** For Gen III–V show green +stat, red -stat, neutral clearly; correct 25 nature mechanics.
- **NEW-10 — FAIL — Met Location popup too tall.** Remove unused middle spacing across generations while preserving scrolling.
- **NEW-11 — FAIL — Selected species number placement.** Selected left panel show Pikachu or Pikachu #25, not leading 25 Pikachu; keep Dex numbers in lists.
- **NEW-12 — FAIL — Gen IV/V location choices too broad.** E.g. Walrein can seemingly choose all 108/276 locations; distinguish native values from legal wild/egg/event/evolution/transfer histories, do not falsely reject legitimate origins.
- **NEW-13 — FAIL — Ability picker says Slot 1/2.** Show ability names and short accurate effect descriptions; green/red only when meaningful; respect slot restrictions.
- **NEW-14 — FAIL — Gen V Language cannot edit.** Add native PK5 staged Language editing.
- **NEW-15 — FAIL — New Pokémon IVs/DVs default wrong.** Gen V creates all IVs=15; owner wants Create defaults IVs=0 for Gen III–V and DVs=0 for Gen I–II. Don't reset existing Pokémon; preserve native shiny/gender constraints.
- **NEW-16 — FAIL — Gen V opens another save chooser.** Home A should directly open currently assigned/read save; change save via Game Card + → Source/Game File → Refresh/Choose Save or Backups.
- **NEW-17 — FAIL — Gen V Key Items/TM-HM view-only.** Add supported native-safe Add/Remove/Edit in staged workspace and remove blanket view-only footer once implemented.
- **NEW-18 — FAIL — Older Trainer screens missing stats.** Add native Play Time and Pokédex seen/caught summaries to Gen I–IV like Gen V.
- **NEW-19 — FAIL — Older gens missing staged exit menu.** Bring Gen V-like Review/Continue/Discard/Export Copy to Gen I–IV, safely and without duplicated controls.
- **NEW-20 — FAIL — Party Actions popup missing.** Gen I–III Party A/touch should show View/Edit/Origin & Legality/Cancel instead of jumping to View.

## 7. Optional old audits

- **AUD-01 — FAIL — Gen V matches shared editor.** Review screen differs from Gen IV and other parity gaps.
- **AUD-02 — PASS — Species sprites and restrictions**
- **AUD-03 — PASS — Emulator names**
- **AUD-04 — NOT TESTED — Game launch and return**
- **AUD-05 — PASS — Quick View touch**
- **AUD-06 — PASS — Item sprites**
- **AUD-07 — PASS — Gen V assigned to profile**
- **AUD-08 — PASS — Gen IV Y multi-move**
- **AUD-09 — PASS — Home Profile navigation**
- **AUD-10 — PASS — Region backgrounds included**

## 8. Additional original screenshot observations (supporting evidence; not new tests)

Owner supplied 14 Nintendo Switch screenshots after the checklist:

1. Pokémon Black Backups popup shows only \`Original DraStic save — READ ONLY\` and \`Staged edits — IN EDITOR\`, not usable backup history/management.
2. Silver created Lv5 Pikachu has DVs Attack/Defense/Speed/Special = 8, derived HP DV 0, displayed calculated \`Special 10/9\` with duplicate \`Sp. Atk 10\`, \`Sp. Def 9\` below. This is plausibly valid Gen II shared Special DV math; the **layout is wrong**, and Create defaults contradict owner's desired zero DV baseline.
3. Silver shiny Lv5 Unown shows Attack/Defense/Speed/Special DV 14/10/10/10, HP DV 0, Shiny Yes, similar displayed sprite to normal Unown.
4. Silver non-shiny Unown shows 15/8/8/3, HP DV 9, Shiny No, but appears same Unown form as above. **Investigate DV-derived Unown letter and sprite refresh** (these combinations imply distinct letters); do not assert shiny stats are fake without verification.
5. Gen V Black Bag Key Items and TMs & HMs pockets show \`Key Items/TMs view only\` without Add actions.
6. SoulSilver Walrein Met Location picker: \`1/108\` with New Bark Town, Cherrygrove City etc all displaying Met Lv5.
7. Diamond Pikachu Met Location picker: \`1/112\` with None, Twinleaf Town, Sandgem Town etc all displaying Met Lv5.
8. Crystal OT Gender popup displays Unknown/Male/Female; owner wants A inline cycling.
9. Ruby Tauros View/Edit shows \`Legality: Not checked\`, \`Encounter origin Unknown\`, \`Met Location None\`.
10. Gold Super Rod Key Item Warning dialogs show plain \`A Confirm\`/\`B Cancel\` boxes plus duplicate footer; Remove dialog shows \`Y Remove\`/\`B Cancel\` and duplicated footer.
11. Gold Gen II PC box Move/Hold state displays \`Place or cancel carried Pokémon before finalizing the Gen II staged save\`. Pickup apparently engages; placement/finalization is failing according to owner.

## 9. Priority and execution contract

**DO NOT CLAIM OWNER RESULTS ARE MISSING. THEY ARE IN THIS COMMITTED FILE.**

Work the actual failed cases in this order, deduplicating shared root causes:

P0: Save/backup browser (R2-EMUBACKUP, C07, S05); Gen II Move (NEW-04); Export Copy failures (B08); Gen V direct assigned-save open (C01, NEW-16).

P1: Gen I–IV Review/exit/accessibility and human-readable change rows (R2-TRAINER, R2-BUG-002, NEW-19, R2-REVIEWMOVE), Party Actions Gen I–III (L06, E02, NEW-20), Gen V Key Items/TM/HM editing (NEW-17).

P2: Shared Origin/Legality UI truthfulness (E03), shiny/Gen II Unown DV-form audit (R2-REROLL; E11 stays NOT TESTED until owner independently confirms), Gen II Special layout (R2-SPECIAL), Gen IV/V Move category layout (M04/M05), Gen IV/V Met Location context (NEW-12/R2-CHECK-011), Gen V Language/IV-DV defaults (NEW-14/NEW-15).

P3: App-wide footer/confirmation polish, Species search keyboard, Held Item search, OT Gender inline, Ball/Nature/Ability pickers, Trainer Play Time/Pokédex, compact location menus, species label, short-item grids, Gen V Review parity.

**Preserve all 70 owner-PASS tests.** An already green CI or audited old NRO does not prove a current FAIL has been fixed.

For each milestone: implement actual production C++, run available relevant tests, commit/push code, update docs/handoffs/MAIN_ACTIVE_HANDOFF.md with exact remote SHA, keep PR #307 draft. For unfinished WIP push a separate safe WIP branch. Never force-push/reset backward/overwrite concurrent work.

Do not merge separate legality PR #314. Never overwrite installed source saves or require owner live emulator write testing. Test exporters on disposable host fixtures and clearly distinguish host pass from hardware pending.

At end report concrete code commits, issues fixed vs pending, host/native test outcomes, and ONE audited NRO when ready—not just another plan or documentation pass.
