# Permanent Gen I–V regression matrix

Everything the owner has accepted on hardware, per generation, and how it is protected from silently regressing. Every accepted behaviour stays here permanently; a row is only removed by an explicit owner decision recorded in the ledger. A ✅ in *Accepted* is an owner hardware PASS on the named candidate, never a test or a commit.

**Status rules.** A status is never inferred from a passing test, a commit message or a PR description. *Hardware Accepted* means the most recent owner hardware result for that behaviour is PASS; *(recheck)* marks accepted behaviour whose code changed after the accepted candidate and must be re-verified on the current candidate. Current source: `6c7c8e99` (application tree identical to candidate `066e1db9`, NOT DEVICE TESTED).

## 1. Accepted behaviour by generation

| Gen | Feature | Latest owner PASS | Code changed since → recheck on current candidate | Evidence |
|---|---|---|---|---|
| I | Bag Add at x1 / quantity from row | R3 62a2f8c3 B01 | **yes** | B0458; ClassicInventoryOverlay.cpp changed by Top Up/Help commits after 62a2f8c3 |
| I | Box Actions -> Move starts Boxes carry | R3 62a2f8c3 R2-CHECK-009 | **yes** | 6f62b69e, dd69bc93 |
| I,II,III,IV | Classic Trainer Play Time and Pokedex shown | OCT24 c60d4da0 NEW-18 | **yes** | C0096 |
| I,II,III | Classic inventory + Options / - Help / B Back | PRE fc1e6de2 A0508-A0510 | **yes** | Gen I-III Bag + now opens Review; Help rewritten (R11 H03 PASS on 3634d1e6 covers Help) |
| I,II,III | Classic inventory X Add / Y Remove / A Edit Amount / L/R category / catalogs | PRE fc1e6de2 | **yes** | A0504-A0515 (superseded by later rounds) |
| I,II,III,IV | Consolidated Gen I-IV item editor incl. X Sort in Add picker | PRE b1fcbd3e (PR #208 comment) | **yes** | D0338 'nro tested and 100% pass' |
| I | Create default species/level: Pikachu L5 (moves part reversed in R11) | R3 62a2f8c3 E12 | **yes** | Gen1StagedPokemonEditor.h:28-35 species 25 level 5; moves now blank |
| I,II,III | Export refuses a changed original (source guard) | R6 26186464 C17 | no | C0336; ClassicSourceGuard.h unchanged |
| I | GB platform label for RBY | PRE 50dac31f | no | A0906 |
| I | Gen I Moves panel alignment | R3 62a2f8c3 M01 | **yes** | SharedMovesPanel.h 2-line change after R3 |
| I | Gen I Party/Storage passive View | PRE 5a2642ac | no | D0288 |
| I | Gen I Play Time editable | R11 3634d1e6 H10 | no | E0009 |
| I | Gen I Review presentation (shared panel only) | R6 26186464 D02 | **yes** | C0338 |
| I | Gen I Trainer Name/Money to Review | R6 26186464 B02 (R2-TRAINER) | **yes** | C0313, C0074 |
| I | Gen I boxed staged editor (DEVICE ACCEPTED) | R11 3634d1e6 H21 (accepted 69668bc8) | **yes** | A0540, E0020; Gen1 overlays changed f2cf5954/eb7fe8c9 -> R12 H13 |
| I | Gen I editor L/R panel navigation | PRE 69668bc8 A0546 | **yes** | Gen I editor overlays changed in 066e1db9 |
| I | Gen I hidden Move rows fit | R3 62a2f8c3 M08 | **yes** | Gen I overlay changed in R11 |
| I | Gen I semantic move-compatibility colours; picker ownership fixes | PRE 69668bc8 (A0547, A0552) | **yes** | Gen I overlay changed since |
| I | Gen I summary stats, five-axis radar, unfocusable calc cells | PRE 69668bc8 | **yes** | A0550/A0551/A0553; box radar routing changed 9a29863f -> R12 H18 |
| I,II,III,IV | Help names only controls that act there (no false + Review) | R6 26186464 A31 | **yes** | PBX-CTL-014 |
| I,II,III,IV,V | Inventory Controls help fits (same window Gen I-V) | R11 3634d1e6 H03 | **yes** | InventoryHelp.h ZR line lengthened (642a7931) |
| I,II,III,IV,V | Live species search | R11 3634d1e6 H11 / R3 62a2f8c3 R2-SEARCH | **yes** | SharedSpeciesPicker eb7fe8c9 |
| I,II,III,IV,V | Open one ordinary save per generation | R11 3634d1e6 H02 | **yes** | E0001 |
| I,II,III | Party A/tap opens Party Actions popup | R6 26186464 A21 (OCT24 c60d4da0 L06) | **yes** | ClassicPartyActions.inc dd69bc93 |
| I,II,III | Party Actions popup (NEW-20/A21) | R6 26186464 A21 | **yes** | dd69bc93 adds Review row |
| I | RBY Items/Bag/PC Items/navigation (ItemsFix) | PRE 50dac31f BUILD_RECORD | **yes** | A0895, A0902-A0905 |
| I | RBY Trainer page | PRE 50dac31f / d9077e2d | **yes** | A0897, A0907 |
| I | Red/Blue/Yellow read-only open (legacy milestone) | PRE 50dac31f | **yes** | A0892-A0896, D0279 |
| I,II,III | RetroArch launch via direct handoff | PRE 0760334f (owner '100% pass'; A0920/D0325 on 97f540e4) | **yes** | GameLauncher core-choice changes 7ed1cc16 |
| I,II,III,IV | Staged exit menu A Review / B Continue / Y Discard & Exit / X Export Copy | R6 26186464 A20 | no | C0300; ClassicStagedExit.h unchanged |
| I | Transactional existing-Pokemon Edit exit; session discard to pre-edit state | PRE 69668bc8 | **yes** | A0543, A0544 |
| II | Bag Add at x1 / quantity from row | R3 62a2f8c3 B02 | **yes** | B0459 |
| II | Crystal Met Location picker compact/names only | R3 62a2f8c3 L02 | **yes** | Crystal list rewritten R11 (03387c48) |
| II | Crystal Met Time cycles inline | R3 62a2f8c3 R2-CRYSTAL | **yes** | Gen2UnifiedPokemonWorkspace.inc changed |
| II | Crystal OT Gender inline | R6 26186464 A11 | no | C0291 |
| II | Gen II Create A/B controls | R3 62a2f8c3 C05 | **yes** | Gen II Create files changed in R11 |
| II | Gen II DATA pane layout | R3 62a2f8c3 L01 | no | B0516 |
| II | Gen II Moves panel matches Gen I | R3 62a2f8c3 M02 | **yes** | same |
| II | Gen II one Special DV/Stat Exp, separate SpA/SpD (R5 layout) | R5 1ca2faec R2-SPECIAL | **yes** | C0197 — later re-rejected R6/R11, see PBX-STA-002 |
| II | Gen II shared editor milestone | PRE cfb70357 | **yes** | D0303; Gen II workspace changed baada873/f2cf5954 |
| II | Gold/Silver/Crystal read-only open (legacy milestone) | PRE issue #6 | **yes** | D0281 |
| II,III,IV,V | Held Item search with Sort, docked keyboard | R6 26186464 A10 | no | PBX-SRC-003 |
| II | No Gold/Silver gender overlap | R6 26186464 B02 | **yes** | C0313 |
| II | PC box Move between slots and boxes | R11 3634d1e6 H23 | **yes** | 6f62b69e (A places) |
| II,III,IV | Trainer Play Time / Gender staged edit | R6 26186464 A30 | **yes** | C0310 |
| III | Bag Add at x1 / quantity from row | R3 62a2f8c3 B03 | **yes** | B0460 |
| III | FRLG/RSE read-only open (legacy milestone) | PRE a2df4c1a / 46e0c161 | **yes** | A0914-A0918, D0273, D0278 |
| III | FireRed HOME-forwarder shows correct assigned GBA party; party card wording 'Current save party' | PRE 2d2d7bc2 D0330/D0331 (0760334f 100% pass) | **yes** | Home party rule b3f88716 |
| III | FireRed current in-game save selected | PRE ea0b806b issue #6 | **yes** | D0267 |
| III | Gen III Create with Safari Ball, held item, export/reopen | R6 26186464 A23 (R2-REVIEWMOVE) | **yes** | Gen3StagedPokemonEditor.cpp changed in R11 |
| III | Gen III Language cycles inline | R3 62a2f8c3 E09 | no | B0511 |
| III | Gen III Moves panel | R3 62a2f8c3 M03 | **yes** | same |
| III | Gen III PID-linked fields and Unown editing | R3 62a2f8c3 E13 | no | B0515 |
| III | Gen III held-item policy; unreleased item labelled and kept | R6 26186464 C08/C15 (OCT26) | no | C0327, C0334 |
| III | Gen III location names cleaned | R3 62a2f8c3 L03 | **yes** | Gen III picker rebuilt R11 (01951ab7) |
| III | Gen III shared editor | PRE 996e6aa4; latest R11 3634d1e6 H24 Edit and discard | **yes** | D0306, E0023 |
| III,IV | Gen III/IV Trainer Name/Money reach Review + staged exit | R6 26186464 A05 | **yes** | C0285 |
| III,IV,V | Met Location rows fit compact 620 px window | R6 26186464 A26 (NEW-10) | **yes** | row content changed R11 |
| III,V | Pokérus and Held Item activation | R6 26186464 B01 | no | C0312 |
| III | RSE trainer read model | PRE 46e0c161 issue #6 | **yes** | D0271 |
| III | Ruby Pikachu Met Location keep-stored row | R3 62a2f8c3 R2-BUG-008 | **yes** | Gen III picker rebuilt R11 |
| III | Sapphire Super Rod pocket warning (R2-CHECK-010) | R3 62a2f8c3 R2-CHECK-010 | **yes** | B0546; catalog changed since |
| IV | Berry/TM/HM Add native ordering | R11 3634d1e6 H17 | no | E0016 |
| IV | G4-03 Party + Box staged View/Edit | PRE 84dae170 | **yes** | D0314 |
| IV | Gen IV Moves panel / Category spacing | R6 26186464 A03 (M04) | no | SharedEditorChrome.h / SharedMovesPanel.h unchanged since 26186464 |
| IV,V | Gen IV/V Add Item through shared flow | R3 62a2f8c3 B04 | **yes** | B0464; Gen5BagScreen.h changed (88f974da, 8915b018) |
| IV,V | Gen IV/V location choices context-backed | R6 26186464 A15 (NEW-12) | **yes** | EncounterContext45.h changed after R6 |
| IV,V | Gen IV/V location windows clean layout | R3 62a2f8c3 L04; R6 A26 | **yes** | picker rows changed R11 |
| IV | Malformed pocket safe refusal | R11 3634d1e6 H36 | no | E0035 |
| IV | Nintendo DS direct launch (DraStic) | PRE 0760334f (D0327 97f540e4, D0329 2d2d7bc2) | **yes** | GameLauncher changes since |
| IV | Remove keeps later items visible (Gen IV packing) | R11 3634d1e6 H16 | no | E0015 |
| IV | Y-carry display (no duplicated source sprite) and Y multi-move | R3 62a2f8c3 AUD-08 (display: 61ce32ac PR #278) | **yes** | Place key now A (6f62b69e) |
| V | Create / Clone / Move / Release work (S03, create-area record B0305) | R2 e10f307f S03 | **yes** | Gen V Move/Clone moved to shared carry (06a7dce2, 6f62b69e) |
| V | Create requires Met Location (Gen V) | R11 3634d1e6 H07 | **yes** | CreateRequiredFields.h generalised in R11 |
| V | Gen V Bag read/navigation | R11 3634d1e6 H37 | no | E0036 |
| V | Gen V Create header names exact game | R3 62a2f8c3 R2-GEN5HEADER | **yes** | test_gen5_create_edit_parity.cpp |
| V | Gen V Create/Edit details panel parity (no clipping) | R3 62a2f8c3 S03 | **yes** | test_gen5_create_edit_parity.cpp |
| V | Gen V Gender inline | R11 3634d1e6 H08 | **yes** | E0007; Gen V surface changed f2cf5954 |
| V | Gen V Home A opens the assigned save | R6 26186464 A18 | no | PBX-CTL-011 |
| V | Gen V Home A opens the assigned save; A selects and advances | R6 26186464 A18 / A29 | no | C0298, C0309 |
| V | Gen V Key Items / TMs editable (NEW-17) | R6 26186464 A19 | **yes** | C0299; Gen5BagScreen.h Top Up added |
| V | Gen V Language inline, no toast | R6 26186464 A27/C03 (OCT26) | **yes** | C0307, C0322; Gen V display names changed d1954551 -> R12 H14 |
| V | Gen V Learnable label OK treatment (7B) | R6 26186464 C02 | **yes** | Gen5SharedReadOnlySurface.inc changed in R11 |
| V | Gen V Moves panel / species-form move pools | R6 26186464 A24 (M05); H26 R11 | no | Gen5MoveCompatibility.h unchanged since 26186464 |
| V | Gen V PP/PP Ups captions | R3 62a2f8c3 S01 | **yes** | Gen V surface changed in R11 |
| V | Gen V Pokérus Keep keeps strain | R6 26186464 C12 | no | C0331 |
| V | Gen V Review / Discard / Export | R3 62a2f8c3 S04 (shared Review R6 26186464 D01) | **yes** | B0468, C0337 |
| V | Gen V Trainer Gender + Pokedex | R11 3634d1e6 H09 | **yes** | E0008; d1954551 display change |
| V | Gen V Trainer clutter / layout (R2-BUG-003) | R3 62a2f8c3 R2-BUG-003 | **yes** | B0532 |
| V | Gen V save assigned to profile | R3 62a2f8c3 AUD-07 | **yes** | B0576 |
| V | Gen V shows real saved data | R3 62a2f8c3 S02 | **yes** | B0528; box radar changed 9a29863f -> R12 H15 |
| V | Gen V species-restricted move chooser | R11 3634d1e6 H26 | no | Gen5MoveCompatibility.h unchanged |
| all | 7E save safety and backups audit vs PKSE | R6 26186464 C05 | **yes** | C0324 |
| all | Actions order consistent, Party View/Edit/Origin & Legality/Cancel (E02/A04) | R6 26186464 A04 | **yes** | Superseded by R11 Extra 7 FAIL; dd69bc93 |
| all | Backup deletion protected: Y confirms, B cancels, original undeletable (C07) | R6 26186464 A02 | **yes** | C0282; 55635234 changed SourceBackupClassification.h after |
| all | Backups list newest first, A opens exact copy as staged session (R2-EMUBACKUP) | R6 26186464 A28 | **yes** | C0308; 55635234 |
| all | Bag Remove confirmation and cancel | R11 3634d1e6 H33 | no | E0032 |
| all | Bag Review discoverable | R6 26186464 B03 / C07 | **yes** | C0314, C0326 |
| all | Ball picker scrollbar gutter | R6 26186464 A12 | no | C0292 |
| all | Clickable popup rows boxed | R3 62a2f8c3 R2-BUG-012 | no | B0678 |
| all | Coloured controller glyph badges in Help | R3 62a2f8c3 R2-BUG-013 | no | PBX-CTL-005 |
| all | Coloured move types | R3 62a2f8c3 R2-BUG-007 | no | move picker presentation unchanged since 62a2f8c3 |
| all | Compact item grid Add picker (R2-BUG-006) | R3 62a2f8c3 R2-BUG-006 | **yes** | B0536; e3c81745 balanced short lists after |
| all | Compact selection windows | R3 62a2f8c3 R2-MENUS | no | B0664 |
| all | Create Review lists the created details | R6 26186464 C13 | **yes** | Review entry paths changed in R11 (Caution notes) |
| all | Create defaults IVs/DVs 0 | R6 26186464 A17 (NEW-15) | **yes** | Gen5SharedScreenState.h:519-520 |
| all | Create, back out, keep (cancel leaves slot empty, Keep stages one, original untouched) | R11 3634d1e6 H30 | **yes** | Create entry points changed in R11 |
| all | Cross-generation .pkN import refused (H38) | R11 3634d1e6 H38 | **yes** | import/file code touched in R11 (src/UI/PokemonFileDialog.cpp, src/Integration); refusal strings unchanged |
| all | Cross-generation import refused | R11 3634d1e6 H38 | **yes** | E0037 |
| all | Editor border/cursor alignment | R3 62a2f8c3 L05 | no | B0523 |
| all | Emulator/provider identification in Save Instances | R11 3634d1e6 H20 | no | E0019 |
| all | Export Copy reload and compare (optional; method not recorded) | R11 3634d1e6 H46 | **yes** | E0045 (see PBX-EXP-006) |
| all | Export Copy writes a separate file; source unchanged | R11 3634d1e6 H14 | **yes** | E0013 |
| all | Export a Pokemon to file twice (' (2)') | R11 3634d1e6 H05 | **yes** | E0004 |
| all | Gender editing without huge picker | R3 62a2f8c3 E08 | no | B0510 |
| all | Import .pkX into an empty slot (staged) | R11 3634d1e6 H06 | **yes** | E0005 |
| all | Item sprites (incl. Gen V feathers/TM discs) | R3 62a2f8c3 AUD-06 | no | B0687 |
| all | L/R while searching (L Backspace, R inert) | R11 3634d1e6 H11 | no | PBX-SRC-004 |
| all | Make Shiny not a quick action (E01) | R3 62a2f8c3 E01 | **yes** | NarrowPokemonActionMenu.h:193-199 |
| all | Manual backup creation (explicit, never on open) | R4 c60d4da0 issue #322 item 4 | **yes** | D0203 'Copy creation works'; fe5c3ad7 and later backup changes |
| all | Move chooser details consistent (number/name/type/power/acc/PP) | R3 62a2f8c3 M06 | **yes** | per-gen surfaces changed in R11 |
| all | Move chooser extra rows | R3 62a2f8c3 R2-MOVECHOOSER | no | MovePickerPresentation.h/MovePickerRowUI.h unchanged since 62a2f8c3 |
| all | Multiple edits -> Review -> Discard restores | R11 3634d1e6 H45 | **yes** | E0044 |
| all | No 'Touch: Back' footer; touch Back kept | R6 26186464 A06 | no | PBX-CTL-013 |
| all | No row counters | R3 62a2f8c3 E04 | no | B0506 |
| all | One shared Review (names, before/after) | R6 26186464 D01 | **yes** | C0337; acc0b772 R7 |
| all | Origin & Legality wording / truthful panel | R6 26186464 A01 (E03) | **yes** | d347afad CG48 R11 |
| all | Original saves never change (S05) | R11 3634d1e6 H14 (also R6 26186464 A25) | **yes** | E0013, C0305; R11 staged editor changes |
| all | PP editing and PP Ups limits | R3 62a2f8c3 M07 | **yes** | per-gen surfaces changed in R11 |
| all | Pokérus cycles inline with A | R3 62a2f8c3 E07 | no | B0509 |
| all | Read-only fields gold | R3 62a2f8c3 R2-GOLD | no | B0666 |
| all | Regression smoke of earlier-round passes (navigation, open/exit, trainer, party/boxes, harmless staged edit) | R11 3634d1e6 H15 | **yes** | E0014; Round 11 repair lanes merged into 066e1db9 -> R12 H18 |
| all | Release requires Y confirmation; cancel preserves (C08) | R3 62a2f8c3 C08 | **yes** | ClassicReleaseActionFix.inc changed in R11 |
| all | Remove/Key Item warning dialogs A/B glyphs (NEW-03) | R6 26186464 A07 | no | C0287; SharedBagAddPicker.h unchanged since |
| all | Selected species name first | R6 26186464 A14 | no | C0294 |
| all | Shiny shows Yes/No (inline toggle) | R3 62a2f8c3 E06 | **yes** | B0508; shiny feedback changed f2cf5954 -> R12 H13 |
| all | Species change clears incompatible moves | R11 3634d1e6 H28 (also R3 R2-BUG-004) | **yes** | SpeciesChangeMoves.h rewritten in R11 (never adds) |
| all | Species search keyboard beside the list | R6 26186464 A09 | **yes** | eb7fe8c9 (R12 H16) |
| all | Stackable quantity editing within native range | R3 62a2f8c3 B06 | **yes** | B0461 |
| all | Switching saves never carries a draft over | R6 26186464 C16 | **yes** | C0335; R11 carry changes |
| all | TM/HM move information in Bag and held item picker | R11 3634d1e6 H35 | no | E0034 |
| all | Tap outside closes non-destructive popups; no tap-through | R6 26186464 C04/C10 | no | PBX-TCH-006 |
| all | Trainer 'A: Edit' only where editable (R2-BUG-001) | R3 62a2f8c3 R2-BUG-001 | **yes** | B0530 |
| all | Unique items (bikes, rods, Key Items) limited to one | R3 62a2f8c3 B05 | no | B0501; quantity policy guards CG09 |
| all | Y rerolls DVs/IVs/EVs immediately | R3 62a2f8c3 E10 | **yes** | B0512; reroll toast removed f2cf5954 -> R12 H13 |
| app | + contextual Options/Settings and - Help/Controls | PRE 3be4de6b A0007/A0008/A0039/A0040/A0124/A0125/D0254/D0255; Help B-close R3 62a2f8c3 NE… | **yes** | Help pages rewritten through R11 (ClassicHelpPresentation.h) |
| app | A selects and advances; no single button quits (P0 #320) | R1 7fea1797 C01-C09 (D0236); R3 62a2f8c3 C02/C04/C09 | **yes** | quit dialog redrawn aace6021 (R12 H02) |
| app | B back/cancel; B closes nested menus one layer at a time | R11 3634d1e6 H19 (PRE D0251) | no | E0018 |
| app | B on Home opens Quit decision; only Y quits | R3 62a2f8c3 C02 | **yes** | aace6021 (R12 H02) |
| app | Boot, 5-minute idle, repeated relaunch, no crash | R11 3634d1e6 H48 + H01 (PRE 3be4de6b A0000-A0079, D0247) | **yes** | new build 066e1db9 |
| app | D-pad single and held navigation (all four directions) | R11 3634d1e6 H18 (first PASS PRE 3be4de6b A0001/A0083-A0086/A0165/D0248) | no | NavigationRepeat.h unchanged since 3634d1e6 |
| app | Draggable scrollbars | R3 62a2f8c3 R2-SCROLLBAR | no | PBX-TCH-004 |
| app | Emulator/provider names on Home cards and the Trainer / Game Browser header (AUD-03) | R3 62a2f8c3 AUD-03 | no | include/UI/TrainerViewScreenBase.h:349-351 unchanged since 62a2f8c3; SaveProviderIdentity.h no commits since (PBX-HOM-005) |
| app | Exit and relaunch after discard | R11 3634d1e6 H48 | **yes** | new build |
| app | FAT32 durability / interruption gate | PRE cf1e390f issue #69 | **yes** | D0305; DurableFile.cpp changed 2 commits since |
| app | Generic Pokemon Action Sheet open / B / Cancel / ~100x stability (Switch-native titles) | PRE 3be4de6b 2026-09-03 (A0157/A0175) | **yes** | No longer used in Gen I-V (TrainerViewScreenBase.inc:1977); ActionSheetModel.h unchanged in R11 |
| app | HOME->resume, sleep->wake, Joy-Con reconnect, no stuck focus | PRE 3be4de6b A0158-A0163 | **yes** | PBX-LQT-002; never retested |
| app | Handheld text readability and focus visibility | PRE 3be4de6b (follow-up 2026-09-03) | **yes** | A0200/A0201; whole UI rewritten since (Product Home, shared editors); no later general handheld row |
| app | Install and open to Product Home | R11 3634d1e6 H01 | **yes** | HOME card c21d3747 (R12 H01) |
| app | Keyboard and numpad entry | PRE 2d2d7bc2 D0333 | **yes** | src/UI/Dialogs/KeyboardDialog.cpp changed in 066e1db9 (Hangul page db414ae7) |
| app | L/R change game on Product Home; ZL/ZR safe | R3 62a2f8c3 C04 (PRE 3be4de6b A0005/A0006/A0037/A0038/D0252/D0253) | no | PBX-CTL-002 |
| app | Launch requires confirmation (only Y launches) | R3 62a2f8c3 C03 / R2-LAUNCH | **yes** | aace6021 (R12 H02) |
| app | Left Stick navigation with held repeat | R11 3634d1e6 H18 (first PASS 0ea98cc1, issue #19 D0265) | no | PBX-CTL-001 |
| app | Legacy Storage persistence across games (Arbok) | PRE 3be4de6b 2026-09-03 (A0217) | **yes** | Bank load/save hardening 080d7c8b, 159b6c97 |
| app | Native Shield/SV/Z-A open | PRE 0760334f | **yes** | D0337 |
| app | Native card detection (Shining Diamond, Let's Go, FRLG Switch identities distinct, no false positives) | PRE 3be4de6b | **yes** | A0137-A0212 |
| app | Navigation torture: rapid mixed input, no focus corruption, stutter or crash | PRE 3be4de6b A0132-A0136 | **yes** | never re-run since first binary |
| app | No live installed-save write; original installed save unchanged after the exercised flow | PRE 3be4de6b (follow-up 2026-09-03) LIVE INSTALLED SAVE WRITE OBSERVED: NO | **yes** | A0215/A0216; WritePolicy.h unchanged since c618bd5e, but export/review paths changed many times since (R11 repairs) -> recheck via R12 H17 |
| app | PokeBank NX red shell identity | PRE af2acf04 (issue #13 comment 2026-09-06) | no | D0266 |
| app | Pokémon artwork in View/Summary | R3 62a2f8c3 AUD-02 | **yes** | A0230 (0ea98cc1), B0684; picker changed eb7fe8c9 -> R12 H16 |
| app | Popups never activate background controls; touch scroll + joystick together | R3 62a2f8c3 C06/N03 | **yes** | PBX-TCH-001 |
| app | Quick Games / Quick View opens and A on a game works | PRE 0760334f (D0332 on 2d2d7bc2) | **yes** | 727fad5d tap-outside |
| app | Quick View touch | R3 62a2f8c3 AUD-05 | **yes** | PBX-TCH-002 |
| app | Recovery files show latest copy protected; cancel deletes nothing (display only) | R11 3634d1e6 H44 | **yes** | E0043; 98b6deaf |
| app | Region backgrounds / RomFS artwork incl. Gen V covers | R3 62a2f8c3 AUD-10 | **yes** | B0691; HOME card scenery masking changed c21d3747 -> R12 H01 |
| app | Second controller (Joy-Con/Pro) inputs | R3 62a2f8c3 C09 | no | PBX-CTL-009 |
| app | Standalone PokeBank NX without PKSE installed | PRE issue #29 2026-09-12 (D0282) | **yes** | many builds since; implicitly R11 H01 |
| app | Themes OLED Black/Dark/Light, Action Sheets, persistence after restart/sleep | PRE 3be4de6b A0188-A0194, D0256-D0259 | **yes** | palettes changed d1a9aad6 |
| app | Touch and controls agreement (no layering, double activation, background selection) | R11 3634d1e6 H47 | **yes** | PBX-TCH-001 |
| app | X/Y/+/- on an empty game-save slot do nothing unsafe | PRE 3be4de6b A0122-A0125 | **yes** | Home rewritten since |
| III-V | Ability names and effects | R6 26186464 A16 | no | C0296 |
| I-IV | Narrow native Actions menus / touch | PR #278 d0a45909 2026-10-12 (A0939/D0340) | **yes** | dd69bc93, 1cdbe73c rebuilt sheets |
| III-V | Nature stat colours green/red | R6 26186464 A13 | no | C0293 |
| I-IV | Owner-accepted integrated Gen I–IV application build | PRE 0760334f (owner-accepted 2026-10-08) | **yes** | A0919 docs/BUILD_RECORD.md; superseded by the Gen I–V candidates |
| I-III | Party Actions popup with staged Edit | R6 26186464 A21 (actions-menu) | **yes** | C0301; Actions model rewritten dd69bc93 -> R12 H05 |
| III-V | View-only notice instead of silent no-op | R6 26186464 C11 | no | C0330 |
| III-V | View-only notice: A or a tap on a field in View explains instead of doing nothing (C11) | R6 26186464 C11 (Oct 26 PASS) | no | viewOnlyActivate (SharedPokemonEditorSurface.inc:643-671, Gen3SharedPokemonSurface.inc:525) unchanged since 26186464 per git log -L (PBX-VEW-009) |

## 2. Accepted issues (ledger rows with status Hardware Accepted)

| ID | Gen | Behaviour | Last owner PASS | Recheck needed |
|---|---|---|---|---|
| `PBX-ACT-001` | all | Pokemon Actions sheet basics: A opens a deliberate menu, B/Cancel close with zero mutation, unfinished actions inert, stable under repetiti… | R6 26186464 A04 (E02) / A21 (NEW-20): PASS | **yes** (H05) |
| `PBX-ACT-003` | all | Make Shiny removed from quick Actions (Shiny stays an editor field) | R3 62a2f8c3 E01: PASS | **yes** (H05) |
| `PBX-ACT-004` | I, II, III, IV | Pre-campaign cross-generation Action menu parity (Gen II empty-slot menu, Gen IV oversized dialog, picker/menu sizes) | PR #278 d0a45909 (2026-10-12): PASS | **yes** (H05) |
| `PBX-ACT-007` | I, II, III | Gen I-III Party Actions popup (View / Edit / Origin & Legality / Cancel, now + Review while staged) instead of jumping to View | R6 26186464 A21: PASS | **yes** (H05) |
| `PBX-BAG-001` | III | Gen III Bag/Items read in early browser builds (FRLG checklist unrun; RSE Items screen empty) | R3/62a2f8c3/B03: PASS (Gen III Add Item) | no |
| `PBX-BAG-002` | I | RBY Items: Invalid category / Medicines fallback defect (first RBY runtime) | PRE/50dac31f/RBY ItemsFix: PASS | no |
| `PBX-BAG-003` | I, II, III | Add Item picker size OK but item names / control hints too dim | R3/62a2f8c3/R2-BUG-006: PASS (compact grid, successor picker) | no |
| `PBX-BAG-004` | I, II, III, IV, V | Item editor screenshot defects: TM/HM art, wrong Ruby TM names, missing Gen IV machine names (+ X Sort request); TM/HM move detail | R11/3634d1e6/H35: PASS | no |
| `PBX-BAG-005` | I, II, III, all | Bag Add must place the item at x1 immediately (no numpad first), quantity edited afterwards from the row (B01/B02/B03/B06) | R3/62a2f8c3/B01,B02,B03,B06: PASS | **yes** (H18) |
| `PBX-BAG-006` | IV, V | Gen IV / V Add Item follows the shared Add flow (B04); Gen IV physical Items milestone | R3/62a2f8c3/B04: PASS (Gen IV ordering: R11/3634d1e6/H17 PASS) | **yes** (H18) |
| `PBX-BAG-007` | all | Bag Remove asks first; compact Remove / Key Item warning dialog with coloured A/B glyphs, A confirms, B cancels, no duplicate footer (B07 /… | R11/3634d1e6/H33: PASS | no |
| `PBX-BAG-008` | all | Bag changes appear in Review and in the exported copy; R3 'Export does nothing across generations' | R11/3634d1e6/H14,H46: PASS (export-copy area); R6/26186464/B03(B08): PASS | no |
| `PBX-BAG-010` | V | Gen V Key Items / TMs & HMs pockets were view-only; need native-safe staged Add/Remove (NEW-17) | R6/26186464/A19: PASS | **yes** (H18) |
| `PBX-BKP-002` | all | Backup deletion requires explicit Y confirmation; originals / last copy undeletable (C07) | R6/26186464/A02·C07: PASS | **yes** (not on R12) |
| `PBX-BKP-003` | all | Original source saves never change (S05 source-save immutability) | R11/3634d1e6/H14: PASS (R6/26186464/A25: PASS) | **yes** (H17) |
| `PBX-BKP-004` | all | Emulator / Switch Save Backups browser: full-screen list, newest first, explicit create, A opens exact copy as staged session, legacy backu… | R6/26186464/A28: PASS | **yes** (not on R12) |
| `PBX-BKP-006` | all | 7E: audit save safety and backups against PKSE principles | R6/26186464/C05·7E: PASS | **yes** (not on R12) |
| `PBX-BNK-001` | app | Legacy Storage (app-owned bank.dat) opens, browses and persists across games/restarts | PRE 3be4de6b (2026-09-03): PASS (A0218 PARTIAL) | **yes** (not on R12) |
| `PBX-CRT-001` | app | Inherited Create/mutation UI reachable on game saves (read-only alpha); Create must stage into a working copy and never write the original… | R11/3634d1e6/H30: PASS | **yes** (H17) |
| `PBX-CRT-002` | I, II, III, IV, V | Create opens one shared editor workspace (no multi-page wizard, persistent draft overview) from any applicable empty slot | R11/3634d1e6/H30: PASS | **yes** (H07) |
| `PBX-CRT-003` | II | Gen II empty-slot Create blocked by "RetroArch source is read-only" (staged copy must stay editable) | R3/62a2f8c3/C05: PASS | **yes** (H18) |
| `PBX-CRT-004` | I | Gen I Add/Create destination in packed boxes: refused destinations, then appended at packed position not cursor cell (needs product decisio… | PRE/cfb70357/PR #68 milestone: PASS | **yes** (not on R12) |
| `PBX-CRT-005` | II | Gen II post-Add auto-opens Pokemon Actions instead of returning to the box | PRE/cfb70357/PR #68 milestone: PASS | **yes** (H18) |
| `PBX-CRT-006` | IV | Gen IV real Create/Add; Create must enter the shared editor (not auto-open Species); fuller Gen IV fields | R11/3634d1e6/H30: PASS (gen not recorded) | **yes** (H07) |
| `PBX-CRT-007` | V | Gen V empty encrypted retail PK5 slots not recognised; Create showed only Cancel; uninitialised new-game trainer must be refused | R11/3634d1e6/H07: PASS | **yes** (H07) |
| `PBX-CRT-008` | V | Gen V Create/Edit/View parity with Gen I-IV: left-panel species text clipped; met/origin text overlapped graph | R3/62a2f8c3/S03: PASS | **yes** (H18) |
| `PBX-CRT-010` | II | Gen II Create A/B controls misroute (A/B "doesnt know what to press") | R3/62a2f8c3/C05: PASS | **yes** (H18) |
| `PBX-CRT-011` | V | Gen V Create header must name the exact game / staged source like Gen I-IV | R3/62a2f8c3/R2-GEN5HEADER: PASS | **yes** (H07) |
| `PBX-CRT-012` | all | New Pokemon IVs/DVs defaulted wrong (Gen V all 15); Create must start IVs/DVs at 0 without resetting existing Pokemon | R6/26186464/A17: PASS | **yes** (H07) |
| `PBX-CRT-013` | III | Gen III Create: every caught Ball incl. Safari, held item independent of Bag availability, named values in Review, export/reopen | R6/26186464/A23: PASS | **yes** (H07) |
| `PBX-CRT-017` | II | Gen II Add toast too implementation-facing ('Pokemon added to staged copy only; source .srm unchanged'); owner prefers concise wording such… | PRE/cfb70357/PR #68 milestone: PASS | **yes** (H18) |
| `PBX-CRY-002` | IV | Gen IV Y-carry pickup/placement display (duplicated pickup sprite, Summary showing held PK4) and Y multi-move | R3 62a2f8c3 AUD-08: PASS | **yes** (H03) |
| `PBX-CRY-004` | I | Gen I (Yellow) Box Actions -> Move reachable and starts the Boxes carry (R2-CHECK-009) | R3 62a2f8c3 R2-CHECK-009: PASS | **yes** (H03) |
| `PBX-CRY-005` | II | Gen II (Gold/Silver/Crystal) PC box Move pickup/drop/between boxes not completing (NEW-04) | R11 3634d1e6 H23: PASS | **yes** (H03) |
| `PBX-CTL-001` | app | Left Stick gives no navigation input; held stick repeat (issue #19); D-pad/stick parity | R11/3634d1e6/H18: PASS | no |
| `PBX-CTL-002` | app | Inherited L/R account switching and inert ZL/ZR (controller normalization #26); L/R Profile in full Games browser | R3/62a2f8c3/C04: PASS | no |
| `PBX-CTL-005` | app | Bottom controller-hint bar low contrast in OLED Black/Dark; hints should stand out (coloured button glyphs) | R3/62a2f8c3/R2-BUG-013: PASS (note-qualified) | no |
| `PBX-CTL-006` | I | Gen I editor step navigation confusing (R bumper as next step); L/R sections, A edit, B back | PRE/69668bc8: PASS | **yes** (not on R12) |
| `PBX-CTL-007` | I, II | Gen I/II Create footer 'X Stage Add' wording and Gen II box A/X control parity with Gen I | PRE/cfb70357: PASS | **yes** (H05) |
| `PBX-CTL-009` | app, II | P0 #320 global input regression: A exits without warning, R opens current save, Gen II Create A/B misrouted | R3/62a2f8c3/C09: PASS (C01-C09 all PASS R1/7fea1797) | no |
| `PBX-CTL-010` | all | E05 controller glyphs and footers consistent app-wide; Help/footer names only controls that act (incl. Inventory Help wording/title/fit) | R11/3634d1e6/H03: PASS (prior R6/26186464/B04: FAIL) | **yes** (H18) |
| `PBX-CTL-011` | V | Gen V Home A opens a Choose Save popup instead of the assigned save (C01 regression, dup NEW-16) | R6/26186464/A18 (NEW-16): PASS | no |
| `PBX-CTL-012` | app | Help closes with B; remove redundant '- Close/Help' footer everywhere | OCT24/c60d4da0/E05: PASS | no |
| `PBX-CTL-013` | all | Remove obsolete 'Touch: Back' footer text while keeping touch Back (NEW-02) | R6/26186464/A06: PASS | no |
| `PBX-CTL-014` | I, II, III, IV | 4D: Gen I-IV Help claims '+ Review' where + does something else; Help must name real controls | R6/26186464/A31: PASS | **yes** (not on R12) |
| `PBX-DSC-002` | I, II, III, IV, V | Emulator save discovery for RetroArch / mGBA / Tico / DraStic / melonDS without moving files (legacy GB/GBC/GBA discovery) | R11/3634d1e6/H20: PASS | no |
| `PBX-DSC-004` | III | FRLG opened an older save instead of the current in-game save | R11/3634d1e6/H20: PASS | no |
| `PBX-EDT-001` | app | Inherited PKSE mutation UI reachable; edits applied to save outside staged copy | R11 3634d1e6 H24: PASS | **yes** (H17) |
| `PBX-EDT-002` | I | Gen I boxed staged editor UX (one-screen workspace, Add/Edit one visual language, Left Stick lists) | R11 3634d1e6 H21: PASS | **yes** (H13) |
| `PBX-EDT-003` | I, II, III, IV | Party editing (Gen I-III staged editors were box-only; party mutation deferred) | R6 26186464 A21·NEW-20: PASS (actions-menu) | **yes** (H05) |
| `PBX-EDT-006` | V | Gen V editor parity (Nature/Gender/Shiny/Ability/PID duplicated, raw-ID controls, oversized menus) | R11 3634d1e6 H08: PASS | **yes** (H13) |
| `PBX-EDT-010` | II, all | Inline Shiny Yes/No and Pokérus A-cycle (Gen II campaign) | R3 62a2f8c3 E06/E07: PASS | **yes** (H13) |
| `PBX-EDT-011` | I, all | No 'Row x/y' counters; list editors draw whole rows above status line | R3 62a2f8c3 E04: PASS | no |
| `PBX-EDT-012` | II | Gen II (Crystal) OT Gender opens 3-option popup instead of inline A-cycle | R6 26186464 A11·NEW-07: PASS | no |
| `PBX-EDT-013` | all | Ball picker scrollbar crowds rows | R6 26186464 A12·NEW-08: PASS | no |
| `PBX-EDT-014` | III, IV, V | Ability picker shows 'Slot 1/2' instead of names and effects | R6 26186464 A16·NEW-13: PASS | no |
| `PBX-EDT-015` | III, V, all | Pokérus fails in Gen III/V; Held Item selection unreliable with A/touch | R6/OCT26 26186464 B01: PASS | no |
| `PBX-EDT-016` | III | Gen III held items follow PKHeX legality (Safari Ball / Berry Juice never held) | R6 26186464 C08: PASS | no |
| `PBX-EXP-001` | all | Export Copy writes a separate verified copy (original_backup + edited + manifest) in an app-owned folder; result visible | R11/3634d1e6/H14: PASS | **yes** (H17) |
| `PBX-EXP-002` | I, II, III, IV, V | Stale-export guard: refuse export when source changed since open (F09; Gen I-III G5) | R6/26186464/C17: PASS (Gen I-III) | no |
| `PBX-FIL-001` | I, II, III, IV, V | Standalone .pk1-.pk5 / .ek3-.ek5 export and import (staged, new file names, cross-gen refused) | R11/3634d1e6/H06: PASS | **yes** (H05) |
| `PBX-HOM-001` | app | Install and open to Product Home (standalone, no PKSE); HOME card renders correctly | R11/3634d1e6/H01: PASS | **yes** (H01) |
| `PBX-HOM-005` | app | Emulator/provider names: Home cards name the real provider (DraStic, melonDS, RetroArch (DeSmuME), mGBA), but the Trainer / Game Browser he… | R3/62a2f8c3/AUD-03: PASS | no |
| `PBX-LEG-001` | I, II, III, IV, V | "Origin & Legality" label and one truthful, unified legality/provenance panel in every generation (no fabricated clean verdict, no invented… | R6/26186464/A01: PASS | **yes** (H09) |
| `PBX-LNG-002` | V | Gen V Language: read-only, then modal chooser; must A-cycle inline, staged, no toast | R6 26186464 A27/C03: PASS | **yes** (H14) |
| `PBX-LNG-009` | all | Language shown only where the exact format stores it; never fabricated | R6 26186464 A27: PASS (Gen V); R3 62a2f8c3 E09: PASS (Gen III) | **yes** (H14) |
| `PBX-LQT-001` | app | Boot, idle stability, relaunch, exit-and-relaunch after discard | R11/3634d1e6/H48: PASS | **yes** (H01) |
| `PBX-LQT-002` | app | HOME button resume, sleep/wake, controller disconnect/reconnect lifecycle | PRE/3be4de6b: PASS | **yes** (not on R12) |
| `PBX-LQT-003` | app | Quick Games: pressing A on a game freezes the app (synchronous refreshHubPreview) | PRE/0760334f: PASS (owner '100% pass') | **yes** (not on R12) |
| `PBX-LQT-006` | I, II, III | Return host on default path: Gen I-III RetroArch crash on launch (698ea4d1, 2d2d7bc2) | PRE/0760334f: PASS | **yes** (not on R12) |
| `PBX-LQT-007` | app | B on Product Home must never quit without a visible Y-confirmed decision (Quit guard) | R3/62a2f8c3/C02: PASS | **yes** (H02) |
| `PBX-LQT-008` | app | Launch requires confirmation (ZR -> Y); launch menu resize and coloured A/B glyph buttons | R3/62a2f8c3/R2-LAUNCH: PASS | **yes** (H02) |
| `PBX-MET-001` | IV, V | Gen IV/V Met Location choices too broad (all 108/276 locations at Met Lv5); show source-backed wild context incl. level, preserve encounter… | R6/26186464/A15: PASS | **yes** (H07) |
| `PBX-MET-002` | II | Crystal Met Time: A cycles the value inline (no menu) | R3/62a2f8c3/R2-CRYSTAL: PASS | **yes** (H08) |
| `PBX-MET-003` | I, II | Crystal Met Location chooser proportional and names only (no raw IDs); Gen I has no met location | R3/62a2f8c3/L02: PASS | **yes** (H08) |
| `PBX-MET-004` | III | Gen III Met Location picker cleaned up: readable names without (RSE)/(FRLG)/ID numbers | R3/62a2f8c3/L03: PASS | **yes** (H07) |
| `PBX-MET-005` | III, IV, V | Met Location popup too tall / unused space; compact 620 px window for every row, no clipping (NEW-10, incl. L03/L04 notes) | R6/26186464/A26: PASS | **yes** (H07) |
| `PBX-MOV-001` | I, II, III, IV, V | Game/species-aware move compatibility (filtered move picker, visible compatibility status, exact-game native PP) across Gen I-V | R11/3634d1e6/H26: PASS (Gen V); R3/62a2f8c3/M06,M07: PASS | **yes** (H18) |
| `PBX-MOV-002` | I, II, III, IV, V | Shared Moves panel and move chooser across Gen I-V: alignment, number/name/type/power/accuracy/PP (Gen V showed name+PP only; Gen IV append… | R3/62a2f8c3/M06: PASS | **yes** (H18) |
| `PBX-MOV-003` | I | Gen I hidden-row appearance in Create/Moves (rows/cursor outlines outside the panel) | R3/62a2f8c3/M08: PASS | **yes** (H18) |
| `PBX-MOV-004` | V | Gen V PP / PP Ups captions too close together | R3/62a2f8c3/S01: PASS | **yes** (H18) |
| `PBX-MOV-005` | all | Move chooser should show 1-2 more rows instead of blank space (all gens) | R3/62a2f8c3/R2-MOVECHOOSER: PASS | no |
| `PBX-MOV-006` | IV, V | Gen IV/V move Category element crowds the selected move Power/Accuracy section | R6/26186464/A03 (M04): PASS | no |
| `PBX-MOV-007` | V | Gen V move picker listed all 559 moves (e.g. Pikachu) instead of the species/form learnset | R11/3634d1e6/H26: PASS | no |
| `PBX-MOV-008` | V | Gen V "Learnable" move label should use the same OK treatment as Gen I-IV (7B) | R6/26186464/C02: PASS | **yes** (H18) |
| `PBX-OPN-002` | V | Gen V Home A opens the assigned save directly (no second chooser; shared hub, never direct-to-Party) | R6/26186464/A18: PASS | no |
| `PBX-OPN-004` | app | Native Shield / Scarlet-Violet / Legends Z-A do not open from Product Home | PRE/0760334f: PASS | **yes** (not on R12) |
| `PBX-OTH-001` | all | Original/installed save never written: live installed-save writes hard-locked (no live-write safety regression) | PRE/3be4de6b/(follow-up 2026-09-03) LIVE INSTALLED SAVE WRITE OBSERVED: PASS (NO); relate… | **yes** (H17) |
| `PBX-OTH-014` | all | Previously owner-PASSed behaviour must not regress (smoke check of earlier passes every round) | R11/3634d1e6/H15: PASS | **yes** (H18) |
| `PBX-PCI-001` | I, II, III | PC Items view/edit in Gen I-III (Gen I foundation includes Bag/PC Items) | PRE/50dac31f/RBY PC Items: PASS (Gen II/III: implicit fc1e6de2 PASS) | **yes** (H10) |
| `PBX-REL-002` | all | Release/Remove needs a separate Y confirmation; A is not destructive; B/cancel preserves the Pokemon; no original-source write | R3 62a2f8c3 C08: PASS | **yes** (not on R12) |
| `PBX-REV-002` | I | Gen I Pending Changes panel content-aware sizing | R6/26186464/D02: PASS | **yes** (not on R12) |
| `PBX-REV-003` | V | Gen V Review / Discard / Export Copy parity and reachability (S04, Gen V Review not reachable) | R6/26186464/D01: PASS (Gen V-specific: R3/62a2f8c3/S04 PASS) | **yes** (H17) |
| `PBX-REV-004` | all | Staged working copy, Review lists only staged changes, Discard All restores original (Discard) | R11/3634d1e6/H45: PASS | **yes** (H17) |
| `PBX-REV-006` | III, IV, V | Review shows one row per Move and readable move/item names (R2-REVIEWMOVE) | R6/26186464/D01: PASS | **yes** (H17) |
| `PBX-REV-007` | I, II, III, IV | Gen I-IV staged exit menu: A Review, B Continue, Y Discard & Exit, X Export Copy (NEW-19; Gen V-like review flow for I-IV) | R6/26186464/A20: PASS | no |
| `PBX-REV-008` | all | One shared Gen I-V Review presentation (AUD-01); owner: do not redesign | R6/26186464/D01: PASS | **yes** (H17) |
| `PBX-REV-010` | all | Bag Review discoverability (B08 / 7G) | R6/26186464/B03: PASS | **yes** (not on R12) |
| `PBX-SET-001` | app | Themes OLED Black / Dark / Light (and variants) readable, switchable and persisted after restart | PRE/3be4de6b: PASS | **yes** (H01) |
| `PBX-SPC-001` | I, II | Gen I/II species picker presentation: `001 - Bulbasaur` rows, shared visual picker with normal/shiny (Gen II plain list) | PRE/cfb70357/PR #68 milestone: PASS | **yes** (H16) |
| `PBX-SPC-003` | I, II, III, IV, V | Species change (and form/gender/shiny) only when native; species mutation reconciles dependent state (Gen IV roadmap Phase 3) | R11/3634d1e6/H28: PASS | **yes** (H09) |
| `PBX-SPC-004` | all | Species change keeps old/incompatible moves (R2-BUG-004): clear incompatible moves on species change; since R11 never add/refill moves | R11/3634d1e6/H28: PASS | **yes** (not on R12) |
| `PBX-SRC-001` | I, II, III, IV, V | Live incremental species search in the shared Gen I-V species picker | R11/3634d1e6/H11: PASS (R3/62a2f8c3/R2-SEARCH: PASS) | **yes** (H16) |
| `PBX-SRC-002` | all | NEW-05: species search keyboard covers the list; dock keys under the art, keep list visible | R6/26186464/A09: PASS | **yes** (H16) |
| `PBX-SRC-003` | II, III, IV, V | NEW-06: Held Item picker needs search alongside Sort (docked keyboard, live filter) | R6/26186464/A10: PASS | no |
| `PBX-SRC-004` | all | L/R while searching: L deletes last letter, R inert, screen behind never moves | R11/3634d1e6/H11: PASS | no |
| `PBX-STA-001` | I | Gen I Summary/radar correctness (zero stats, modern rows, isShiny false, stray text, radar layout/scale) | R11 3634d1e6 H21: PASS | **yes** (H18) |
| `PBX-STA-003` | V | Gen V boxed Pokémon show HP/stats 0 | R3 62a2f8c3 S02: PASS | **yes** (H15) |
| `PBX-STA-005` | all | Y rerolls DVs/IVs/EVs immediately without confirmation | R3 62a2f8c3 E10: PASS | **yes** (H13) |
| `PBX-STA-006` | III, IV, V | Nature stat colours (raised green / lowered red) in Gen III–V View/Edit | R6 26186464 A13·NEW-09: PASS | no |
| `PBX-TCH-001` | all | Touch app-wide: popups/modals never activate background controls; touch scroll and joystick coexist; no accidental mutation | R11/3634d1e6/H47: PASS | **yes** (not on R12) |
| `PBX-TCH-002` | app | Quick Games / Quick View finger drag, vertical scroll, momentum, tap opens the right game | R3/62a2f8c3/AUD-05: PASS | **yes** (not on R12) |
| `PBX-TCH-003` | all | L06: Party / summary-panel taps must open the generation's Actions popup (not View) | R6/26186464/A21: PASS | **yes** (not on R12) |
| `PBX-TCH-004` | app | Scrollbars draggable / track-tappable for fast scrolling app-wide | R3/62a2f8c3/R2-SCROLLBAR: PASS | no |
| `PBX-TCH-006` | all | 7D: tap outside closes non-destructive popups (destructive confirmations stay); no tap-through | R6/26186464/C04+C10: PASS | no |
| `PBX-TRN-001` | III | Gen III trainer read model (FRLG trainer name/gender/TID/SID/money); FireRed trainer/item reads only PARTIAL | R6/26186464/A05: PASS | **yes** (H18) |
| `PBX-TRN-002` | II | Gen II Trainer screen read-only/unusable for staged editing | R6/26186464/A30: PASS | **yes** (H18) |
| `PBX-TRN-004` | V | Gen V Trainer: separate popup and list conventions (clamp, first-tap focus, focusable read-only rows); clutter (R2-BUG-003) | R11/3634d1e6/H09: PASS | **yes** (H14) |
| `PBX-TRN-005` | I, all | Gen I Trainer Name/Money editable; 'A: Edit' footer wherever a field is editable, all gens (R2-TRAINER / R2-BUG-001) | R6/26186464/B02 (R2-TRAINER): PASS | **yes** (H18) |
| `PBX-TRN-006` | I, II, III, IV | Trainer edits must reach Review; staged exit offers Review / Continue / Discard / Export Copy (Gen I-IV, R2-BUG-002) | R6/26186464/A05: PASS | **yes** (H17) |
| `PBX-TRN-007` | I, II, III, IV | Gen I-IV Trainer: native Play Time and Pokedex seen/caught like Gen V (NEW-18 data) | OCT24/c60d4da0/NEW-18: PASS | **yes** (H18) |
| `PBX-TRN-008` | I, II, III, IV, V | Trainer Info parity: badges, Trainer Gender visible where stored, editable Play Time (Gen II-IV) and Gender (Crystal, Gen III-IV) | R6/26186464/A30: PASS | **yes** (H18) |
| `PBX-TRN-009` | I, II | Gold/Silver Trainer Gender rows overlap ('Male' under new row); Gold/Silver/Gen I store no gender | R6/26186464/B02: PASS | **yes** (H18) |
| `PBX-TRN-011` | V | Gen V Trainer Gender editable (native writer) and Pokedex in the read-only section (H09) | R11/3634d1e6/H09: PASS | **yes** (H14) |
| `PBX-TRN-012` | I | Gen I Play Time editable (hours up to 255) and listed in Review (H10) | R11/3634d1e6/H10: PASS | no |
| `PBX-VEW-001` | all | Party / Boxes browsing and data sanity across saves | R11 3634d1e6 H02: PASS | **yes** (H18) |
| `PBX-VEW-002` | app | Summary should show Species and Type explicitly (tester read SPECIES as type) | PRE 5a2642ac: PASS | no |
| `PBX-VEW-004` | I | Read-only View reuses PKSE-style detail experience; keep five-stat radar | PRE 5a2642ac: PASS | no |
| `PBX-VEW-005` | II | Gen II Create/View native fields (Friendship, EXP, Egg, Crystal caught); no fabricated modern fields | R3 62a2f8c3 L01: PASS | no |
| `PBX-VEW-006` | IV | Platinum party-only save: no editable target; Party opened inherited read-only details (G4-03) | PRE 84dae170: PASS | no |
| `PBX-VEW-007` | II | Gen II DATA pane text wraps/clips inside its pane (SharedDataPane::layoutRows) | R3 62a2f8c3 L01: PASS | no |
| `PBX-VEW-008` | all | Selected species shows '25 Pikachu' instead of name first | R6 26186464 A14·NEW-11: PASS | no |
| `PBX-VIS-001` | app | PokeBank NX product identity: inherited PKSE branding/logo dominant | PRE af2acf04 (issue #13 comment 2026-09-06): PASS | no |
| `PBX-VIS-002` | app | View/Summary lacks a Pokémon image/sprite | R3 62a2f8c3 AUD-02: PASS | no |
| `PBX-VIS-005` | I | Yellow labelled as GBA (platform label wrong) | PRE 50dac31f: PASS | no |
| `PBX-VIS-006` | III | RSE game-card artwork missing | PRE (1c96df25 artifact): PASS | no |
| `PBX-VIS-007` | I | Gen I draft Pokémon has no live sprite preview while editing species | PRE 69668bc8: PASS (DEVICE ACCEPTED) | **yes** (H16) |
| `PBX-VIS-008` | II | Gen II modal/action-sheet shells show full accent outline (Empty Slot, Pokémon Actions) | PRE (issue #84 retest): PASS | no |
| `PBX-VIS-009` | V, app | Packaged RomFS artwork completeness (Gen V game cards, region backgrounds) | R3 62a2f8c3 AUD-10: PASS | no |
| `PBX-VIS-010` | all | Compact, uniform menus/pickers across generations (less blank space) | R3 62a2f8c3 R2-MENUS: PASS | no |
| `PBX-VIS-011` | I, II | Read-only fields (Gen I stats, derived HP DV) gold in both View and Edit | R11 3634d1e6 H21: PASS (R3 62a2f8c3 R2-GOLD: PASS) | no |
| `PBX-VIS-012` | V | Gen V species picker sprites (494-649) and shiny preview | R3 62a2f8c3 AUD-02: PASS | **yes** (H16) |
| `PBX-VIS-013` | V | Gen V item sprites missing (feathers 565-571, TM/HM disc art) | R3 62a2f8c3 AUD-06: PASS | no |

## 3. Regression guard coverage

`tests/test_hw_round1_regression_gate.py` pins owner PASS items from Round 1 (42), Round 2 (6), Round 3 (30 new), Oct 24 (27), Oct 25 (10) and Oct 26 (19) to host tests and contracts and runs on every PR. **It has no Round 11 table**, so Round 11 PASS items that are not on the Round 12 checklist are not protected:

- R11 H09 Gen V Trainer Gender and Pokédex (last PASS R11 (3634d1e6 inferred) H09 PASS)
- R11 H10 Gen I Play Time editable (to 255) (last PASS R11 (3634d1e6 inferred) H10 PASS)
- R11 H24 Gen III edit and discard (last PASS R11 (3634d1e6 inferred) H24 PASS)
- R11 H30 Create, back out, keep (last PASS R11 (3634d1e6 inferred) H30 PASS)
- R11 H36 Malformed Gen IV pocket safe refusal (optional) (last PASS R11 (3634d1e6 inferred) H36 PASS)
- R11 H38 Cross-generation import refusal (last PASS R11 (3634d1e6 inferred) H38 PASS)
- R11 H44 Recovery display: latest prior generation protected (last PASS R11 (3634d1e6 inferred) H44 PASS)
- R11 H46 Export Copy reload and compare (S06-like, optional) (last PASS R11 (3634d1e6 inferred) H46 PASS)
- R11 H48 Exit and relaunch after discard (last PASS R11 (3634d1e6 inferred) H48 PASS)
- R11 H05 Export same Pokémon twice -> distinct ' (2)' file (last PASS R11 (3634d1e6 inferred) H05 PASS)
- R11 H06 Import exported Pokémon into empty slot (last PASS R11 (3634d1e6 inferred) H06 PASS)
- R11 H13 Japanese Gen I/II read-only (last PASS R11 (3634d1e6 inferred) H13 PASS)

Known gate caveats: the Oct 26 table keys rows by Round 6 sheet number (its B01/C02 are not the Round 1 items of the same name); the E02 guard pins the pre-Round-11 Actions order; the E12/C14 guards were updated for the owner's Round 11 reversal (Create starts with no moves).

**Required next step:** add a Round 11 guard table (the 34 R11 PASS items) and a Round 12 table once the owner reports Round 12, so no accepted behaviour can leave a checklist unguarded.

## 4. Bag / PC item operations (mandatory feature 7)

| Gen | add | remove | quantity | sort | pc items view edit | top up add missing |
|---|---|---|---|---|---|---|
| I | exists · accepted: yes | exists · accepted: yes | exists · accepted: yes | picker-only · accepted: yes (picker sort) | exists · accepted: yes | exists · accepted: no |
| II | exists · accepted: yes | exists · accepted: yes | exists · accepted: yes | picker-only · accepted: yes (picker sort) | exists · accepted: implicit only | exists · accepted: no |
| III | exists · accepted: yes | exists · accepted: yes | exists · accepted: yes | picker-only · accepted: yes (picker sort) | exists · accepted: implicit only | exists · accepted: no |
| IV | exists · accepted: yes | exists · accepted: yes | exists · accepted: yes | picker-only · accepted: yes (picker sort, Gen IV in b1fcbd3e) | n/a · accepted: n/a | exists · accepted: no |
| V | exists · accepted: yes | exists · accepted: yes | exists · accepted: yes | n/a · accepted: n/a | n/a · accepted: n/a | exists · accepted: no |

## 5. Safety-critical functions never accepted on hardware

| Function | Gens | Evidence |
|---|---|---|
| Recovery-file deletion through both confirmations (Y on list -> Y on file-list confirmation), incl. B cancel at step 2 | I-V (app-wide) | Only R11 H44 PASS (display); R11 H12 FAIL (selection invisible). Repaired 98b6deaf; R12 H11 not device tested. Host: test_recovery_management_fs/model, test_ui_recovery_files_host. |
| Recovery files: nothing preselected / protected copies unselectable (owner clarification 3) | I-V | RecoveryManagementModel.h:254-262; host only; R12 H11 pending. |
| Gen V export-record deletion in Save Backups (Y confirm, last-copy and other-profile refusal) | V | Added 55635234 after the last C07 PASS (R6 26186464); test_gen5_backup_listing_host only. |
| Gen V exports listed/opened as staged copies in Save Backups (G1) | V | 55635234; R7 C02 never had results; not on R10-R12 checklists. |
| Verified reparse refusal path (corrupt/failed strict reparse blocks publish) | all | VerifiedStagedExport.cpp:179,259; exportCorruptionProbe host only; hardware only ever saw the success path (R11 H14). |
| Exclusive, race-safe export publish / repeat Export Copy gets new folder (QA-F3, QA-F3b) | all | fa9cfcea, 0bac3cf1; host tests only; never a checklist item. |
| Export/backup fault handling (no space, permission, partial output cleanup, power loss) | all | ExportFailurePoint host injection; DurableFile host tests; owner told not to simulate power loss. |
| Export byte whitelist / native checksum repair (F11) and no-op edit = no byte change | all | QA matrix + test_qa_export_contracts C3 only. |
| Exported copy loads in an emulator (S06 round-trip) | all | S06 NOT TESTED R1-R6; R11 H46 PASS does not record whether an emulator load was done (PBX-EXP-006). |
| Stale-export guard for Gen IV / Gen V | IV, V | Only Gen I-III owner PASS (R6 C17). |
| .pkX import into an occupied slot is refused (never overwrites) | III, IV, V (I/II append to first empty packed slot) | Gen3StagedPokemonEditor.cpp:1134+, Gen4StagedPokemonEditor.cpp:636-638, Gen5PokemonFile.h:27; R11 H32 NOT TESTED ('dont know how to test this'); host tests test_pokemon_file_gen3/4/5. |
| Import cancel ('-') leaves nothing staged | all | aace6021; test_pokemon_file_dialog_model only; R12 H06 pending. |
| Corrupt / truncated / wrong-game save refused without crash or write | I-V | Parser fuzzing host only; GEN14 C7 / GEN15 B14 never filled. |
| Cross-profile isolation of save assignments | III, V | ea0b806b profile bindings; no owner test with a second profile. |
| exFAT durability (FAT32 accepted only) | app | issue #69: exFAT PENDING / NOT TESTED. |
| Installed (native Switch) source has no save-changes path in current NRO (#23) | app | 0ea98cc1; af2acf04 checklist never filled. |

### Safety-critical ledger issues without hardware acceptance

- `PBX-BAG-011` [MANDATORY 7] Complete Bag operations: every native pocket, quantity bounds, Add/Remove, no lost edits (E04-E09) — Needs Investigation
- `PBX-BKP-001` Durability / interruption gate accepted on FAT32 only; exFAT physical acceptance never tested — Needs Investigation
- `PBX-BKP-005` No live emulator/installed writes, Inject Save or restore-backup-to-source — Intentionally Unsupported
- `PBX-BKP-007` Gen V exports were not listed in Save Backups (7E G1) — Fixed in Code — Awaiting Hardware
- `PBX-BKP-008` Recovery files view: selection invisible (H12 FAIL); nothing preselected; protections and both confirmations — Fixed in Code — Awaiting Hardware
- `PBX-BKP-009` Approved per-adapter source writeback pipeline (backup -> stage -> reparse -> explicit write -> readback -> rollback) — Missing Feature
- `PBX-BKP-010` Save Health Center / Disaster recovery / Save Time Machine — Missing Feature
- `PBX-BLK-001` Top Up / Add Missing only fills the current pocket; Bag categories not filled; 'add all' option wanted (H04) — Fixed in Code — Awaiting Hardware
- `PBX-BLK-002` Top Up does not fill PC Items; must work for every pocket and PC Items in all gens (Key Items excluded) (H34) — Fixed in Code — Awaiting Hardware
- `PBX-BLK-003` Gen I fill-all only fills the current category (not TMs / items); misleading on the shared 20-slot Bag — Fixed in Code — Awaiting Hardware
- `PBX-BNK-003` No Bank deposit/withdraw for Gen I-V saves (H41/H42); bank.dat cannot hold PK1/PK2/PK4/PK5 — Missing Feature
- `PBX-BNK-005` Legacy Storage save silently drops an occupied slot whose game group has no bank tag (verify treats it as empty) — Open Defect
- `PBX-CLN-001` Occupied/unavailable destination safety: Clone, Move and Import never silently overwrite a Pokemon (H32); Gen I/II carry must own the staged save — Fixed in Code — Awaiting Hardware
- `PBX-CLN-003` Clone to an available slot must be one flow in every generation (Gen II reopened Actions, Gen III-V numeric keypad that did not work, Gen IV defaulted to the o… — Fixed in Code — Awaiting Hardware
- `PBX-CRY-001` Inherited Y grab/Move/Multi mutation controls reachable in installed game-save views (read-only alpha contract) — Fixed in Code — Awaiting Hardware
- `PBX-CRY-003` Move controls: Y picks up, A places, B cancels in every generation with footer/Help updated (H23 request); Move is an atomic staged copy/remove with the source… — Fixed in Code — Awaiting Hardware
- `PBX-CTL-008` Gen V editor navigation differs from Gen I-IV; moving focus changes values (P1) — Fixed in Code — Awaiting Hardware
- `PBX-EXP-003` Export / backup fault handling: no space, read error, permission, existing folder, power loss; never claim success from partial file — Fixed in Code — Awaiting Hardware
- `PBX-EXP-004` Exports change only whitelisted bytes and repair native checksums (F11); no-op edits produce no byte change — Fixed in Code — Awaiting Hardware
- `PBX-EXP-005` Repeat export gets a fresh unique destination; race-safe publish (QA-F3 common/Gen V, QA-F3b Gen II) — Fixed in Code — Awaiting Hardware
- `PBX-EXP-006` S06: exported edited save loads in an emulator as a separate save (round-trip) — Needs Investigation
- `PBX-EXP-007` Verified strict reparse of the written edited save before publish (C2); checksum corruption detected — Fixed in Code — Awaiting Hardware
- `PBX-LNG-008` Japanese Gen I/II saves stay read-only (never hardware-tested with a real Japanese save) — Needs Investigation
- `PBX-OPN-003` Strict open/validate: exact game identity, ambiguous held back, corrupt input refused without crash; untouched round-trip — Fixed in Code — Awaiting Hardware
- `PBX-OTH-002` Final original-save integrity proof never performed: launch original game / pre-post SHA-256 / concurrent emulator writing the same save — Needs Investigation
- `PBX-OTH-003` Inherited PKSE mutation UI (Release/Create/Move-Multi/Edit/Save) reachable on installed-game sources (#23) — Fixed in Code — Awaiting Hardware
- `PBX-OTH-011` QA-F9: candidate-NRO audit defaulted to stale fa9cfcea artifact on pull_request — Fixed in Code — Awaiting Hardware
- `PBX-OTH-023` Owner approval needed for merges, deletions and retargets; legality #314 stays unmerged — Fixed in Code — Awaiting Hardware
- `PBX-REL-001` Inherited Release (X) reachable in installed game-save views — Fixed in Code — Awaiting Hardware
- `PBX-REV-001` Inherited apply/save-style mutation flow reachable in game-save views (#23) — Needs Investigation
- `PBX-SET-003` Second Switch profile: saves owned by another profile show 'belongs to another Switch profile' — Needs Investigation
- `PBX-SPC-006` H25: species change carried previous Pokemon's Met Location and Pokerus (stale editor state); Create drafts reset visibly, existing Pokemon keep history and ge… — Fixed in Code — Awaiting Hardware
- `PBX-VLT-001` Persistent PokeBank-owned Master Vault and named Banks (entity store, stable IDs, SHA-256, provenance, journal/recovery, Bank references) — Missing Feature
- `PBX-XFR-001` Gen I-V cross-generation transfer workflow (e.g. Gen I -> Gen V; 'through bank?') with explicit route rules, conversion preflight, provenance and destination-f… — Missing Feature
- `PBX-XFR-002` Cross-game True Move locked (installed saves read-only; COPY/import/staged only until per-adapter safety gates pass) — Intentionally Unsupported

## 6. Every round's hardware checklist must include

1. Every row in §1 with *recheck = yes* whose code changed since its last PASS.
2. Every ledger row with status Fixed in Code — Awaiting Hardware (its acceptance candidate).
3. Every safety function in §5 that an ordinary owner save can exercise.
4. No row may be removed from a checklist unless it is owner-PASSed or the owner records a decision; the ledger records the reason.
