# v1.0 release feature gaps (Gen I–V)

**Verdict: PokeBank NX is NOT ready for v1.0.** 13 v1.0-required features are missing, 2 v1.0-relevant defects are open, and 35 safety-critical issues have no hardware acceptance. Nothing in this file counts a design document, an unused backend class, a model enum or a test as an implemented feature: a feature counts only if it is reachable from the shipped UI in the current source (`6c7c8e99` = candidate `066e1db9`).

**Status rules.** A status is never inferred from a passing test, a commit message or a PR description. *Hardware Accepted* means the most recent owner hardware result for that behaviour is PASS; *(recheck)* marks accepted behaviour whose code changed after the accepted candidate and must be re-verified on the current candidate. Current source: `6c7c8e99` (application tree identical to candidate `066e1db9`, NOT DEVICE TESTED).

## 1. The twelve mandatory features

| # | Feature | Overall | Issues (status) |
|---|---|---|---|
| 1 | Move to Vault in the shared Pokémon Actions menu | ❌ **Missing** | `PBX-ACT-002` Missing Feature |
| 2 | Vault deposit, withdrawal and persistent storage | ❌ **Missing** | `PBX-BNK-003` Missing Feature; `PBX-BNK-005` Open Defect; `PBX-VLT-001` Missing Feature |
| 3 | Gen I–V to later-generation transfer workflow | ❌ **Missing** | `PBX-XFR-001` Missing Feature; `PBX-XFR-002` Intentionally Unsupported |
| 4 | Vault Pokémon search | ❌ **Missing** | `PBX-VLT-002` Missing Feature |
| 5 | Consistent Actions menus across generations | ⏳ Not hardware-accepted on the current candidate | `PBX-ACT-001` Hardware Accepted; `PBX-ACT-003` Hardware Accepted; `PBX-ACT-004` Hardware Accepted; `PBX-ACT-005` Fixed in Code — Awaiting Hardware; `PBX-ACT-006` Fixed in Code — Awaiting Hardware; `PBX-ACT-007` Hardware Accepted |
| 6 | Clone, Move, Release and occupied-destination safety | ⏳ Not hardware-accepted on the current candidate | `PBX-CLN-001` Fixed in Code — Awaiting Hardware; `PBX-CLN-002` Fixed in Code — Awaiting Hardware; `PBX-CLN-003` Fixed in Code — Awaiting Hardware; `PBX-CLN-004` Needs Investigation; `PBX-CRY-001` Fixed in Code — Awaiting Hardware; `PBX-CRY-002` Hardware Accepted; `PBX-CRY-003` Fixed in Code — Awaiting Hardware; `PBX-CRY-004` Hardware Accepted; `PBX-CRY-005` Hardware Accepted; `PBX-CRY-006` Fixed in Code — Awaiting Hardware; `PBX-REL-001` Fixed in Code — Awaiting Hardware; `PBX-REL-002` Hardware Accepted |
| 7 | Complete Bag/PC item operations | ⏳ Not hardware-accepted on the current candidate | `PBX-BAG-005` Hardware Accepted; `PBX-BAG-006` Hardware Accepted; `PBX-BAG-009` Fixed in Code — Awaiting Hardware; `PBX-BAG-010` Hardware Accepted; `PBX-BAG-011` Needs Investigation; `PBX-BAG-012` Needs Investigation; `PBX-BAG-013` Missing Feature; `PBX-BLK-001` Fixed in Code — Awaiting Hardware; `PBX-BLK-002` Fixed in Code — Awaiting Hardware; `PBX-BLK-003` Fixed in Code — Awaiting Hardware; `PBX-BLK-004` Needs Investigation; `PBX-PCI-001` Hardware Accepted; `PBX-PCI-002` Needs Investigation · 6 accepted, unchanged since |
| 8 | Species-change level and history preservation | ❌ **Open defect** | `PBX-SPC-001` Hardware Accepted; `PBX-SPC-002` Fixed in Code — Awaiting Hardware; `PBX-SPC-003` Hardware Accepted; `PBX-SPC-004` Hardware Accepted; `PBX-SPC-005` Fixed in Code — Awaiting Hardware; `PBX-SPC-006` Fixed in Code — Awaiting Hardware; `PBX-SPC-007` Fixed in Code — Awaiting Hardware; `PBX-SPC-008` Open Defect |
| 9 | Correct move and encounter validation | ❌ **Missing** | `PBX-LEG-001` Hardware Accepted; `PBX-LEG-002` Open Defect; `PBX-LEG-003` Missing Feature; `PBX-LEG-004` Fixed in Code — Awaiting Hardware; `PBX-LEG-005` Needs Investigation; `PBX-MET-001` Hardware Accepted; `PBX-MET-002` Hardware Accepted; `PBX-MET-003` Hardware Accepted; `PBX-MET-004` Hardware Accepted; `PBX-MET-005` Hardware Accepted; `PBX-MET-006` Fixed in Code — Awaiting Hardware; `PBX-MET-007` Fixed in Code — Awaiting Hardware; `PBX-MET-008` Needs Investigation; `PBX-MOV-001` Hardware Accepted; `PBX-MOV-002` Hardware Accepted; `PBX-MOV-003` Hardware Accepted; `PBX-MOV-004` Hardware Accepted; `PBX-MOV-008` Hardware Accepted · 3 accepted, unchanged since |
| 10 | Language and name handling | ❌ **Missing** | `PBX-LNG-001` Fixed in Code — Awaiting Hardware; `PBX-LNG-002` Hardware Accepted; `PBX-LNG-003` Fixed in Code — Awaiting Hardware; `PBX-LNG-004` Fixed in Code — Awaiting Hardware; `PBX-LNG-005` Missing Feature; `PBX-LNG-006` Open Defect; `PBX-LNG-007` Missing Feature; `PBX-LNG-008` Needs Investigation; `PBX-LNG-009` Hardware Accepted; `PBX-LNG-010` Needs Investigation; `PBX-LNG-011` Needs Investigation |
| 11 | Original-save immutability, backups and recovery | ❌ **Missing** | `PBX-BKP-001` Needs Investigation; `PBX-BKP-002` Hardware Accepted; `PBX-BKP-003` Hardware Accepted; `PBX-BKP-004` Hardware Accepted; `PBX-BKP-005` Intentionally Unsupported; `PBX-BKP-006` Hardware Accepted; `PBX-BKP-007` Fixed in Code — Awaiting Hardware; `PBX-BKP-008` Fixed in Code — Awaiting Hardware; `PBX-BKP-009` Missing Feature; `PBX-BKP-010` Missing Feature; `PBX-EXP-001` Hardware Accepted; `PBX-EXP-003` Fixed in Code — Awaiting Hardware; `PBX-EXP-004` Fixed in Code — Awaiting Hardware; `PBX-EXP-005` Fixed in Code — Awaiting Hardware; `PBX-EXP-006` Needs Investigation; `PBX-EXP-007` Fixed in Code — Awaiting Hardware; `PBX-EXP-008` Needs Investigation; `PBX-OTH-001` Hardware Accepted; `PBX-OTH-002` Needs Investigation; `PBX-OTH-003` Fixed in Code — Awaiting Hardware · 1 accepted, unchanged since |
| 12 | Pokémon export/import and save serialization integrity | ⏳ Not hardware-accepted on the current candidate | `PBX-EXP-001` Hardware Accepted; `PBX-EXP-003` Fixed in Code — Awaiting Hardware; `PBX-EXP-004` Fixed in Code — Awaiting Hardware; `PBX-EXP-005` Fixed in Code — Awaiting Hardware; `PBX-EXP-006` Needs Investigation; `PBX-EXP-007` Fixed in Code — Awaiting Hardware; `PBX-EXP-008` Needs Investigation; `PBX-FIL-001` Hardware Accepted; `PBX-FIL-002` Fixed in Code — Awaiting Hardware; `PBX-FIL-003` Fixed in Code — Awaiting Hardware; `PBX-REV-001` Needs Investigation; `PBX-REV-002` Hardware Accepted; `PBX-REV-003` Hardware Accepted; `PBX-REV-004` Hardware Accepted; `PBX-REV-005` Fixed in Code — Awaiting Hardware; `PBX-REV-006` Hardware Accepted; `PBX-REV-008` Hardware Accepted; `PBX-REV-009` Fixed in Code — Awaiting Hardware; `PBX-REV-010` Hardware Accepted · 2 accepted, unchanged since |

## 2. Missing v1.0 features (status Missing Feature, v1.0-required)

### `PBX-ACT-002` Move to Vault / Add to Master Vault / Add to Bank / Transfer to Game absent from every Gen I-V Actions menu

- Evidence: No Gen I-V Actions sheet can reach AddToMasterVault/AddToBank/TransferToGame. The enum and labels exist only in include/UI/ActionSheetModel.h:24-34,48-56 (dispatch always NotYetSupported :83-89) and are used only by the generic Gen VI+ sheet (StoragePanel.cpp:464-483), which TrainerViewScreenBase.inc:1972-1990 refuses for Gen I-V sessions. Gen IV keeps dead enum values (src/UI/Gen4SharedPokemonSurface.inc:83, status strings :1036-1044) but gen4Actions() (:953-986) never appends them. The shared label table has no Vault row (NarrowPokemonActionMenu.h:97-114, allowed list :196-198); the greyed placeholders were removed in 1cdbe73c (comment :88-89). tests/test_action_sheet_model.cpp:12 only pins the generic enum. Product Home shows Master Vault as 'COMING SOON' (include/UI/AppShellModel.h:41-43). Engineering decision E0099/F0003: hidden until the Vault exists; owner request F0000 (Extra 7) wants 'move to vault' on every menu.
- Next action: Owner to confirm intake Q2 (stay hidden until the Vault exists). Implement only together with PBX-VLT-001: one action row from the shared owner (SharedPokemonEditorContract / NarrowPokemonActionMenu), Copy-to-Vault first.

### `PBX-BKP-009` Approved per-adapter source writeback pipeline (backup -> stage -> reparse -> explicit write -> readback -> rollback)

- Evidence: docs/V1_ROADMAP.md:41 v1 target 'only perform source writeback through individually approved transaction adapters'; Phase 12 (docs/V1_ROADMAP.md:211-222) all unchecked. No write-to-source code exists in current source (only app-owned exports; PBX-BKP-005). Gated by owner decision (CURRENT_STATUS.md:11) - deferred, not rejected.
- Next action: Owner decision needed: is source writeback in v1.0 scope (roadmap Phase 12) or explicitly post-1.0? Record the decision.

### `PBX-BKP-010` Save Health Center / Disaster recovery / Save Time Machine

- Evidence: docs/FEATURE_FEASIBILITY_MATRIX.md #18 Save Health Center v1.0/v1.x, #58 Disaster recovery v1.0/v1.x, #14 Time Machine v1.x. Current app has Save Backups + Recovery files only; no health-center or rescue-flow UI found in src/UI. v1 flag: target is 'v1.0/v1.x' (ambiguous).
- Next action: Owner to confirm whether any of #18/#58 is v1.0 scope.

### `PBX-BNK-003` No Bank deposit/withdraw for Gen I-V saves (H41/H42); bank.dat cannot hold PK1/PK2/PK4/PK5

- Evidence: Bank record tags exist only for GG/SWSH/BDSP/PLA/SV/ZA/FRLG (src/Trainer/Bank.cpp:81-116); there is no tag for RBY, GSC, DPPt/HGSS or BW/B2W2. Every classic Gen I-V source is RetroArchLegacy/ExternalLegacy, so save-pane X/Y are stripped (TrainerViewScreenBase.inc:2437-2441; canPerform false, include/Safety/SourceMutationPolicy.h canPerform). Gen V routes the Storage tile to a refusal (Gen5HubRouting.h:46; Gen5SharedReadOnlySurface.inc:1576-1580). Gen II excludes Bank from details (Gen2SharedSurfaceParity.inc:17-20). Gen III FRLG deposit/withdraw exists only in the inherited Switch-backup workspace, not for RetroArch FRLG. Net: deposit/withdraw is reachable only for Switch-native titles + FRLG backup workspaces.
- Next action: Implement inside the Vault plan (intake gaps 2-4: format-neutral raw-bytes entity, COPY-first deposit, withdraw to empty destination with preview/cancel/Review, MoveTransaction journal). Do not extend bank.dat tags.

### `PBX-BNK-004` In-save box names cannot be selected/renamed in Gen I-V (only Legacy Storage boxes rename); inconsistent per generation

- Evidence: No Gen II-V native box-name writer exists; header focus is gated on trainer.supportsBoxNames() (TrainerViewScreenBase.inc:2448) and save rename requires a mutable workspace (renameBox, TrainerViewScreenBase.inc ~1540). Gen IV can focus the header then refuses 'source is read-only'; Gen II/III/V show or hide names inconsistently (intake VAULT inventory 4). Bank box rename works (renameBankBox gated by requireMutableStoragePane(1, Rename), TrainerViewScreenBase.inc:1580). Product decision pending (intake Q2: in-save rename in v1.0 or read-only with explanation).
- Next action: Owner answers intake Q2; at minimum make the read-only state consistent and explained in every generation.

### `PBX-LEG-003` Complete legality engine / PKHeX-class encounter, move and ball legality not integrated (PRs #314/#327/#328 isolated, tracker #329)

- Evidence: The legality-engine PRs #314, #327, #328 are kept isolated and are not in this app (ROUND11_ENGINEERING_INTAKE.md section 5; RETEST_066e1db9.md: "no legality-engine code"). The app shows only neutral Origin & Legality / Incomplete states (CG48). docs/V1_ROADMAP.md:11 release order 3 requires finishing #314. Tracker: #329.
- Next action: Track on #329; integrate only after the app candidate is accepted (V1_ROADMAP release order 3-4).

### `PBX-LNG-005` Name entry for all languages: accented keys, Japanese kana and Korean Hangul keyboards, Gen V Unicode entry, app-wide display language

- Evidence: Partially built: Accents page and Gen I/II kana pages (src/UI/Dialogs/KeyboardDialog.cpp), Hangul page only for Korean-language Gen IV names (db414ae7, KeyboardDialog.cpp:193-195, Gen4SharedPokemonSurface.inc:819). Gen V name entry still ASCII-only ('1-10 ASCII characters', Gen5SharedReadOnlySurface.inc:440-445); no Japanese kana for Gen III-V; no app display-language setting. Owner questions in intake H27 (per-record vs global language, Hangul in v1) unanswered.
- Next action: Owner decision on the H27 questions (keyboard follows record language? global display language? Hangul/kana entry in v1?), then schedule Gen III-V kana and Gen V Unicode writers.

### `PBX-OTH-018` v1.0 release hardening items not implemented (Reduced Motion/accessibility, constrained memory, bounded caches, recovery UX, sleep/resume, privacy-safe diagnostics export)

- Evidence: No Reduced Motion setting anywhere in src/ or include/ (grep). Diagnostics exist (AppShellScreen.cpp:158-160 logs + read-only 'Diagnostics / Build Info') but no privacy-safe export. No sleep/resume/operation-mode handling found in src/main.cpp or src/UI/UI.cpp. NACP title is set (Makefile:46 APP_TITLE 'PokeBank NX', :74 icon). Release gates in docs/RELEASE_CHECKLIST.md (no unsupported parser crash, handheld+docked, sleep/resume) are unmet or untested.
- Next action: After #313, make a v1.0 hardening plan: Reduced Motion, sleep/resume test, diagnostics export, memory/cache bounds.

### `PBX-OTH-019` v1.0 game scope (40 mainline identities GB→Switch incl. 3DS, LGPE/SwSh/BDSP/PLA/SV/Z-A) not implemented

- Evidence: docs/PRODUCT_DECISIONS.md §1 makes 40 identities (incl. 3DS and Switch) the v1.0 target; README.md:79/:134 shows Gen VI–VIII native formats only on separate PRs #309/#311/#312 and modern Switch as 'general editor ❌ / Future'. No 3DS (Gen VI/VII) support exists in src/. Inherited Switch-native read paths exist, but no shared editor. Conflict to resolve: the current gate (F0187) is Gen I–V only, but v1.0 is still defined as all 40.
- Next action: Owner to confirm whether v1.0 still means 40 identities or Gen I–V only; update V1_ROADMAP/PRODUCT_DECISIONS to match.

### `PBX-OTH-022` Vault-driven National Pokédex (v1 milestone) not implemented — Home Pokédex is a placeholder

- Evidence: src/UI/AppShellScreen.cpp:786 renders 'Collection progress will appear here' for the Pokédex section; Master Vault is 'Coming Soon' (AppShellScreen.cpp:92, :672). Only per-save read-only Pokédex views exist (TrainerViewScreenBase.inc:482 pokedexProgress; Gen5SharedReadOnlySurface.inc:1333-1437 Gen V list). The spec requires Vault-authoritative ownership, which cannot exist without the Vault.
- Next action: Keep blocked on Master Vault; state explicitly in V1_ROADMAP whether the Pokédex milestone is v1.0 or v1.x.

### `PBX-VLT-001` Persistent PokeBank-owned Master Vault and named Banks (entity store, stable IDs, SHA-256, provenance, journal/recovery, Bank references)

- Evidence: No Vault exists in shipped code: no Vault entity store / Vault ID / SHA-256 manifest anywhere under include/ or src/ (no Vault directory; intake VAULT inventory 'There is NO Vault ID, no SHA-256 entity store and no provenance log'). Product Home 'Master Vault' and 'Banks' are FutureBackend placeholders (include/UI/AppShellModel.h:41-43 'COMING SOON', :50-52 'NO BANKS YET'); opening Master Vault shows only 'Master Vault is coming in a future PokeBank NX update' (src/UI/AppShellScreen.cpp:91-101). The only persistent store is the inherited PKSE Legacy Storage bank.dat (include/Trainer/Bank.h:1-10), explicitly labelled 'app-owned / not Master Vault' (src/UI/Panels/HomeMenuPanel.cpp:119, TrainerViewScreenBase.inc:5603). Deferred out of the Gen I-V gate (FUTURE_AFTER_GEN15_BACKLOG #3/#7/#27, F0038) and out of the Round 11 repair NRO (F0028); still required for v1.0 (V1_ROADMAP, README, MASTER_VAULT_SPEC §22). Unchanged between 3634d1e6 and 066e1db9.
- Next action: Owner to answer intake VAULT Q6 (user-facing name; bank.dat migrate or stay legacy) and approve the separate Vault plan/branch; build per MASTER_VAULT_SPEC §22 minimum milestone with a physical read-only Vault test.

### `PBX-VLT-002` Search in Bank/Vault with highlight of matching Pokemon/item (PKSE-style) before 1.0

- Evidence: No search/highlight in src/UI/Panels/StoragePanel.cpp or Trainer::Bank; Product Home 'Search' is FutureBackend 'COMING SOON' (include/UI/AppShellModel.h:53-55). Intake notes the PKSE search the owner remembers is not in this repo's PKSE history. v1.0 requirement also stated in search-area REQ records F0062-F0065 ([MANDATORY 4]).
- Next action: Owner to give the spec (intake VAULT: name/species/item index, highlight); schedule with the Vault index (PBX-VLT-001) or as a Legacy Storage search if wanted earlier.

### `PBX-XFR-001` Gen I-V cross-generation transfer workflow (e.g. Gen I -> Gen V; 'through bank?') with explicit route rules, conversion preflight, provenance and destination-first Move

- Evidence: No converter for Gen I, II, IV or V: Conversion::convert covers only FRLG <-> GG/SWSH/BDSP/PLA/SV/ZA (include/Conversion/Convert.h:1-33, Result::Unsupported :46-48; intake VAULT inventory 8). Standalone file import refuses other generations ('Only a .pk2 file can go into a Generation II save; Pokemon are not converted between generations', src/Integration/Gen2/Gen2StagedEditor.cpp ~1529), which the owner PASSed as H38. No Transfer to Game action in any Gen I-V sheet (PBX-ACT-002). Rule table (official routes; Gen I->V refused or marked non-canonical) is an open owner decision (intake Q4). Fidelity/RouteEvidence exist only for the FRLG/Switch converter (include/Conversion/Fidelity.h, RouteEvidence.h). Transfer issues were closed as deferred for the Gen I-V gate (F0059) but README/V1_ROADMAP keep them v1.0.
- Next action: Answer the owner's H38 question in plain words (no route exists today; official routes I<->II, III->IV, IV->V); get owner decision on intake Q4; plan the Transfer Lab after the Vault.

## 3. Other missing features (not marked v1.0-required)

- `PBX-BAG-013` Gen V Bag Add picker has no X Sort (Gen I-IV Add pickers have Native / A-Z / Z-A) — Owner decision: add X Sort to the Gen V Add picker for parity, or record that Bag 'sort' means picker sort only.
- `PBX-CRT-016` Create Pokemon / Quick Legal / advanced editor as permanent core feature: Quick Legal and explicit CREATED/USER-CREATED provenance labelling — Keep on the post-v1 backlog unless the owner promotes it; depends on the isolated legality engine (#329).
- `PBX-CTL-004` Right Stick has no action (future model rotation, #25) — Owner decision: keep deferred past v1.0 or schedule #25 (model rotation).
- `PBX-LNG-007` Gen I/II localized default species names — Schedule M2/M3 or record an owner decision.
- `PBX-OTH-021` Lineage/breeding parents: never invent parents the save did not store (lineage states) — Carry the rule into the Vault/lineage design when that work starts.
- `PBX-VEW-003` View/Pokédex with cry, Pokédex description and location — Owner to decide whether cry/Pokédex text is in v1 scope.
- `PBX-VIS-003` Optional Right Stick 3D model rotation in Summary — Owner to confirm it can be dropped from scope (would then be Intentionally Unsupported); not v1.

## 4. Open defects

- `PBX-BNK-002` Legacy Storage pane keeps inherited X/Y controls (Y cycles Menu/Move/Multi, X sorts/moves/duplicates) unlike the Y-pick/A-place box carry — Owner decision on intake Q4; if unified, route the Bank pane through ClassicCarryRules (Y pick, A place, B cancel).
- `PBX-BNK-005` ⚠️ Legacy Storage save silently drops an occupied slot whose game group has no bank tag (verify treats it as empty) — Apply intake repair R2: count an occupied untaggable slot as a verify FAILURE (fail closed) and add a host test, before any Vault/Bank deposit path is opened.
- `PBX-LEG-002` Gen V shiny toggle may change the Wurmple-line evolution value (PID high half rewritten) — Either constrain withShiny5 to keep (PID>>16)%10 class for Wurmple-line species, or record an owner decision accepting the limit; cite #329.
- `PBX-LNG-006` Gen II decodes text with the English table only (French/German/Italian/Spanish saves show wrong letters) — Implement M2 per-save European table choice for Gen II, with a French/German Crystal fixture.
- `PBX-OTH-005` Pre-NRO engineering gates incomplete: sanitizer/Clang/fuzz coverage gaps (host tests + ASan/UBSan required before every NRO) — Add sanitizer flags to Makefile.host.classic_inventory, add padded (0x00/0xFF) and Japanese seeds to fuzz_gen2_sav, and record the Clang/host-UI limitation as accepted or fix the ambiguous using-directives.
- `PBX-OTH-012` 🎯v1 Full Gen I–V 20-release, every-action, exact-NRO owner acceptance gate (#313) not met; no v1.0 — After R12 clears its FAILs, schedule the full G01–G12 20-game acceptance round on one exact NRO.
- `PBX-OTH-013` Hardware status records incomplete or stale: R7–R9 owner results never recorded; R10 row says 'owner testing now'; NRO archive missing R1/R2 (403) and R12; README stale — After #330 merges, run archive-candidate-nro.yml for R1, R2, R10 and 066e1db9 before Jan 2027 expiry. Update the R10 row and README to R11 results, and record that R7–R9 have no owner results.
- `PBX-OTH-015` Owner's original reports and screenshots not stored verbatim in the repository (Round 6 both reports) — Ask the owner to re-upload both Round 6 reports and screenshots; store them unchanged next to ROUND6_RESULTS_26186464.md.
- `PBX-OTH-024` Round 11's 34 owner-PASS items are not frozen in the regression gate — Add a GUARDS_ROUND11 block (34 IDs, count-checked against ROUND11_OWNER_WORKSHEET.md) to tests/test_hw_round1_regression_gate.py.
- `PBX-SPC-008` 🎯v1 Species change Level: whether a species change keeps the Pokemon's current Level has NO SPECIFICATION; generations behave differently today — Ask the owner for a rule (keep current Level vs reset) for Create and for Edit; then align Gen I-III with Gen IV/V or vice versa and add an R12/R13 checklist step.
- `PBX-VEW-009` View-only notice (L06 follow-up): A or a tap on a field in View must say why nothing changes - accepted for Gen III-V, never implemented for Gen I/II (silent dead taps); Gen IV lock rules lacked a harness fixture — Route Gen I and Gen II passive View A/tap through PokeBank::UIModel::ViewOnlyNotice (editHint/lockedHint) with host tests, then add a View-only notice item for Gen I/II to the next hardware checklist.

## 5. Contradictory requirements that need an owner decision

These come from the requirement extraction; each pair of documents states the requirement differently. Until the owner decides, neither version may be dropped silently.

1. **v1.0 game scope** — `PRODUCT_DECISIONS.md` §1 ("40 target v1.0 identities", GB to 3DS/Switch) and `README.md` (Gen VI–IX before 1.0) vs. `CURRENT_STATUS.md` ("Release scope remains Gen I–V") and `V1_ROADMAP.md` (Gen VI–VIII "not current release scope").
2. **Move to Vault presentation** — owner: in every generation's Actions menu; Round 11 intake: absent until the Vault exists; `UI_FLOW.md` / gate A11: greyed placeholder; `TRANSFER_MODEL.md`: only when verified.
3. **Actions order and Review row** — `UI_FLOW.md` target order (Add to Master Vault, Add to Bank, Transfer to Game, Make Shiny) differs from the owner's list; owner list implies Review is always shown, code shows it only with pending changes; gate F02 wants a greyed empty Review.
4. **Party actions** — `POKEMON_EDITOR_UI_CONTRACT.md` still says Party mutation is deferred; Gen I–III Party edit shipped and was accepted (R6 A21); owner wants Clone/Move/Release in Party too.
5. **Gen I → Gen V transfer** — owner expects a route (R11 H38); intake: refuse or mark non-canonical; `FEATURE_FEASIBILITY_MATRIX.md` puts Transfer Lab in v1.x; `README.md` needs transfer for v1.0.
6. **Vault timing** — `V1_ROADMAP.md`, intake and `RELEASE_CHECKLIST.md`: v1.0; `FUTURE_AFTER_GEN15_BACKLOG.md` #3 closed as "not planned for the current Gen I–V gate".
7. **Moves on species change** — `MAIN_ACTIVE_HANDOFF.md` R2-BUG-004 ("resets moves to the game's level-up moves") vs. owner Round 11 (never auto-fill moves). Current code follows the owner.
8. **Create validation** — owner Extra 6 "universal rule" and "level 100 → 1–2 moves" vs. the owner's own Oct 31 clarification (exact-game rules, never universal) and the intake ("not a game rule"). The 1–2-moves request is still unanswered by the owner.
9. **Clone semantics** — `UI_FLOW.md` / `MASTER_VAULT_SPEC.md`: Clone creates a new Vault ID and never duplicates into a live slot; gate D12 and the shipped app clone into the staged save.
10. **Japanese/Korean names** — owner wants native names and keyboards in every language (H27); `README.md` / consolidation keep Japanese Gen I/II saves read-only and their editing locked.
11. **Species-change Level** — no specification existed; the owner's Nov 1 list makes it mandatory. Gen I–III reset Level to the encounter minimum, Gen IV keeps it, Gen V refuses a different growth group (`PBX-SPC-008`).

## 6. Release blockers summary

- Missing v1.0 features: **13**
- Open defects: **11**; needs investigation: **25**
- Fixed in code but awaiting hardware: **72**
- Accepted behaviour changed since acceptance (R12 recheck): **94**
- Safety-critical issues without hardware acceptance: **35**; untested safety-critical functions: **16**

v1.0 must not be declared while any line above for missing v1.0 features is non-zero.

