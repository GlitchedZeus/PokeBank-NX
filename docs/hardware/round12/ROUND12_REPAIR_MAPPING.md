# Round 12 repair mapping (R12-001 to R12-012)

**Status: NOT DEVICE TESTED.** Every repair below is *Fixed in Code — Awaiting Hardware* until the owner
tests the Round 13 NRO. Host and CI results never count as hardware acceptance.

- Owner results: `OWNER_RESULTS_066e1db9.json` (byte-identical to the upload; never edited).
- Issue source: `ISSUE_LEDGER.csv` (QA intake, unchanged).
- Repair branch: `fix/round12-gen15-repairs` (from `fix/round11-gen15-repairs`). The frozen Round 12
  candidate `066e1db9` is unchanged.
- Owner checklist: `docs/hardware/RETEST_ROUND13_TEMPLATE.md`. Items are mapped by issue ID and title, not
  by H-number. The owner's custom 24-item Round 12 sheet numbers items differently from the repo's
  H01–H18.

| ID | Root cause (code) | Fix | Focused regressions | Round 13 item | Still unresolved / owner decision | Hardware evidence |
| --- | --- | --- | --- | --- | --- | --- |
| R12-001 | `ReviewEntry` mapped Gen V Boxes Review to Y; Gen V had its own A → Move path | Gen V Boxes use the shared `ClassicPackedMove::handleGen5BoxInput` (Y tap Move, Y hold Multi, A place, B cancel, + Review); group drop is one transaction (`dropCarriedGroup`, `sparseGroupLanding`); footer and Help are shared Gen I–V | `test_review_entry`, `test_classic_help_accuracy`, `test_ui_clone_carry_host` (Gen V keys), `test_gen5_staged_slot_actions`, `test_ui_layout_host` | H02 | Gen V touch-drag multi-select not added. Gen V Party keeps Y Review (by design). | None |
| R12-002 | Carry pickup, place, clone and cancel posted `EditFeedback` toasts and Y/X hints | `EditFeedback::Kind::Carry` is silent; `settled()` retires a refusal when the carry ends. Kept: "There is no Pokemon in this slot to move", capacity, occupied / protected / source-changed refusals, failures | `test_ui_clone_carry_host`, `test_classic_carry_rules`, `test_gen3_hardware_surface_contract`, `test_ui_pokemon_file_menus_host` | H03 | Owner to confirm cancel is silent too. | None |
| R12-003 | Packed Gen I/II carry drew a landing marker in the next box plus a free browse cursor | `ClassicCarryRules::packedFocusSlot`: the one cursor *is* the landing slot; the packed carry owns L/R and ZL/ZR; a full box is refused; B restores | `test_ui_clone_carry_host` (packed focus), `test_classic_carry_rules` | H04 | — | None |
| R12-004 | The default Met Location list was every native location, with unlisted ones only labelled "unverified" | `MetLocationPolicy.h`: default list = stored row + verified rows for the species and its pre-evolutions + markers + "Other location (unverified)…"; all other locations stay in the unverified list; the Other row is never written; "Met evidence" line in editor and Review | Gen III, Crystal and Gen IV/V picker tests (every species; never empty; stored row kept); export → reparse | H05 | No egg-group data: the egg marker shows for every species, labelled unverified. Gen IV markers 2000–2002 not choosable. Gen IV/V gift and static encounters not in the data. Crystal time rows merged and "via X" wording need owner confirmation. | None |
| R12-005 | The picker ignored the Pokémon's current level | Rows whose minimum level > current level move to the unverified list, marked "level too low"; Crystal Met Level capped at current level | Level-filter cases (Lv. 5 Pidgeotto, Lv. 5 Nidorino, Crystal) | H06 | Trade / event paths still unverified, never legal. | None |
| R12-006 | No cap existed. "About 20" was the native list size (RS Items, Gen II Items, Gen I shared Bag), but the UI never said so | `Plan::rooms` records slots / free / eligible / planned / not fitted per native list; the preview has one capacity line per list; the partial status names the full list and its native size | `test_bag_top_up_adapters` coverage section (12 games × every scope; plan → apply → export → strict reparse), `test_ui_bag_top_up_host` | H07 | — | None |
| R12-007 | `BulkItemPlan::storySensitive` treated HMs as Key Items | HMs added with quantity 1, unique across Bag + PC; only Key Items excluded ("Key Item (excluded: can gate story events)"); Emerald / FR/LG PC never gets HMs | `test_bulk_item_plan`, `test_bag_top_up_adapters` | H08 | An HM held in the PC is not re-added to the Bag (owner to confirm). | None |
| R12-008 | Gen III Japanese strict reparse compared Latin OT "RED" with the stored full-width "ＲＥＤ". The preview failed, and the surface drew the unbuilt draft (PID 0, stats "—"). No damaged record was staged | `gen3FoldWidth` / `gen3ToHalfWidth`; a language change re-stores the OT and nickname or is refused; the language cycle skips languages that cannot store the current name, with a status; Korean is not offered in Gen III | `test_gen3_staged_pokemon_editor` languageStability (all five Gen III games), `test_gen4_mutable_pokemon` guard | H09 | Latin names on Gen III Japanese show full-width (owner to confirm). | None |
| R12-009 | Gen V Create stored an explicit nickname, so its name never followed Language | `CreateRequest5::defaultNickname`: un-nicknamed Create follows Language; Gen III uses the same path as R12-008 | `test_gen5_party_box_parity` languageFollowsDefaultName5 | H10 | Gen III Korean is intentionally unsupported (no Korean Gen III release). | None |
| R12-010 | The export was correct. The status toast lasted 600 frames and was drawn over later modals | `TransientStatusScope.h`: a status is scoped to the layers present when it was posted; opening a new modal, Actions sheet or editor ends it | `test_ui_pokemon_file_menus_host` staleExportToast (6 checks fail on the Round 12 code, pass now) | H11 | Same-species, same-source Actions sheets share a scope key. | None |
| R12-011 | Not a regression: Vault is excluded from Gen I–V repair candidates | Not built | — | "Not in this build" | **v1.0 blocker: Missing Feature** (Move to Vault, deposit / withdraw / storage) | None |
| R12-012 | Not a regression: Vault is not built | Not built | — | "Not in this build" | **v1.0 blocker: Missing Feature** (Vault search) | None |

## Gates on the integrated head `9c301c48`

- 276 host test binaries pass.
- GCC ASan/UBSan, Clang, the Round 8 compatibility matrix, classic inventory, `make qa`, 42 Python test
  files and cross-gen candidate checks all pass.
- Real-save round trip: 15 / 15 pass.
- `git diff --check` is clean.

None of these results is hardware evidence.
