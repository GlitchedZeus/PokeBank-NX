# PokeBank NX — Main Development checkpoint

Maintained by the Main Development coordinator. Only the coordinator edits this
file; threads report to it. GitHub is authoritative: re-fetch before relying on
any SHA below.

**Last updated:** 2026-10-15 13:15 UTC (QA-F9: independent NRO audit was stale; fix in progress).

## Active milestone

**Gen I–V COMPLETION CAMPAIGN** (owner directive, 2026-10-15): continuous work by Threads 2R/C/L/4 until all Gen I–V software scope is done and every gate is green on the final commit, then ONE NRO. #307 head at campaign start: `c952565cf878705726b2541546d085fa9724108d`, CI 8/9 green with GCC host still running; internal build only.

### Campaign progress (08:15 UTC tick)

| Item | State |
|---|---|
| #307 head | `96a446853e9f12574c98379499d1ce233ad82445`: 2R Gen V audit closed at `cedc2f8` (glyphs and overflow; exit guard, focus restore and popups verified) |
| QA-F2 | **Fixed** in `96a44685`. The three `one-time-*` workflows are re-indented and gated to `workflow_dispatch`; none deleted. |
| C | `cd2bbbff` (8 commits: Review discard, picker parity, export separation tests, Gen II Clone, Y Random DVs confirm, Gen IV bag layout). 2R is merging it. |
| L | `50491e88` (draft PR #317). The RetroArch Quit "crash" is upstream behaviour: no next load, so the loader goes to hbmenu or HOME. The DraStic return is its saved LauncherPath. Fixes: a bounded argv plan with libnx-exact quoting, the return selection restored on Home, and a return-on-launcher mode inside the **opt-in** return host. Default handoff is byte-identical. Merges cleanly; 2R is merging it and adding the quit-route notice. NOT DEVICE TESTED. |
| QA (4) | `c8c0f38c`: **Phase 5 matrix done**. 20 versions × 21 cells = 415 PASS, 5 N/A, 0 FAIL, identical under GCC and Clang (plain and ASan/UBSan). Hardware-only residue is listed in `docs/qa/GEN1_5_MATRIX.md`. 2R is merging it. Next: Phase 6 independent audit on the integrated head. |
| Merges on #307 | C → `a0b72203`; L → `181423a4`; QA `c8c0f38c` → `f9441070`. 2R reports host-test, sanitize and qa green under GCC and Clang. Pending merges: QA `4ae526f2`, L `95255c97` (draft PR #319). Head `c939cd4a`. |
| Phase 1 (2R lane) | Quit-route notice (`77ad7fbe`, SaveSelectScreen.cpp:2104), D-pad/stick/touch parity (test_shared_editor_contract navigation/touchParity), Left/Right never mutates (`c939cd4a`, test_shared_editor_contract.cpp:431). Verified in code. |
| QA-F7 / F8 (Low, C) | Gen IV Unown form not editable; clone→release leaves "(empty) (edited)" and different export bytes. Repros are on the QA branch. |
| Heads at 09:15 | #307 `2d8195d3` (QA `4ae526f2` and L `95255c97` merged; gates green per 2R). Pending: C `cea1da09`, L `ba6d63ba`, QA `d350cc2d`. |
| C since `cd2bbbff` | QA-F6 fixed; QA-F7 fixed (Unown 28 letters); QA-F8 review line fixed (export-bytes half still open); Gen III gender accessor `Gen3Gender.h`; Gen I HP; Gen II PP/OT/re-encode; retail upper-case nicknames; X Keep (div. 2); Gen I View navigation (div. 3); Gen II shared selectors (div. 5). Open: F8 export bytes, Castform/Cherrim stored forms restricted to form 0, div. 7/4/6. |
| L | Lane complete in software: opt-in return host with a per-stage log, return-selection validation, handoff-hygiene audit (no leaked services or handles on the default path). Awaiting the owner's A/B device test. |
| QA Phase 6 | fuzz_launch_plan (1.9M runs, no defect); export contract C6 Discard All on all 18 paths; SAVE_SAFETY_CHAIN.md; fuzz_home_cache. Open: stale home.party.* matrix facts. |
| Final NRO gate | Waits on: C's F8 bytes and Castform/Cherrim; Gen III gender wiring (2R); QA matrix facts and re-run on the integrated head. gen15-integrated-candidate workflow is ready; last runs green on c939cd4 and c933734. |
| 09:40 | #307 `d57e5bc4`: L `980a217b` and C `c7b0f978` merged, Gen III Home gender wired (HomePartyPreview.h:56/63), 139 PASS GCC/Clang plain and sanitize, qa 420/0. Pending merges: C `ce82066f` (all divergence items 1–7 done, Gen I Edit working-copy draft), QA `ce5919c` (Phase 6 complete, no new defects). |
| Decision: .dsv backups | Gen IV (and Gen V if affected): original_backup must be the byte-exact source file, container and footer included. The edited export stays the raw 0x80000 payload, and the manifest states the container. C does the transaction side, 2R the UI hand-off. |
| Decision: battle-only forms | Gen IV editor offers only stored form 0 for Castform and Cherrim (PKHeX FormInfo). Existing values are preserved. |
| 10:15 | #307 `f99e262c`. Gen V export now backs up the source file verbatim, .dsv footer included (test_gen5_source_e2e dsvBackupIsVerbatim; manifest ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE). C `ce82066f` merged. 143 PASS GCC/Clang plain and sanitize, qa 420/0. Pending merges: C `7dec0c78` (bag review lines, Gen III JP names, strain-0 Pokérus), QA `d6f34e4a` (Home facts derived; F6/F7/F8a are regressions). |
| Remaining before the final NRO | C: QA-F8b (Gen IV clone→release export bytes), Castform/Cherrim form 0, Gen IV .dsv stale-source check plus verbatim backup. 2R: VerifiedExportRequest backupBytes and container keys (Common), Gen IV UI hand-off. Then a full gate run and ONE NRO. |
| Ownership exception | C edited QA's tests/test_qa_matrix_gen4.cpp once, a stricter bag-review cell needed after 2b2be00f. QA is told to adopt it. |
| 11:15 | #307 `73f38d0a`: C `768c65fd` merged (F8b fixed `f883f4d0`; Castform/Cherrim form 0 `7e36220f`; Gen IV stale-source refusal), QA `2fa40e3`, Common verbatim-backup fields (`d9cdcf78`, test_verified_export_verbatim), Gen IV snapshot at Open (UI.cpp:678). All QA findings F1–F8 are fixed in code. Pending: C `afd96e3b` (Gen IV verbatim backup and manifest keys), the optional backup extension (2R), QA's F8b move to regressions and the strict cell. Then the ONE final NRO. |
| Note | PR #310 shows "merged by GlitchedZeus" at 10:50:42. That is GitHub auto-marking it merged when 2R pushed 93421d5d (the QA merge), not a separate owner action. |
| **FINAL TREE** | #307 `a0633b6d53a582aaae2be5bf91e9fde0c10d27ac`, tree `945df45d892aaa3082a57927ffbfa8f6d54365cc`. Verified by the coordinator: it contains QA `fad2523f`, C `bcd20dc5`, L `980a217b`, and the F8b, Castform/Cherrim, Gen IV/V verbatim .dsv backup and Common export units. `LIVE_SAVE_WRITES_ENABLED = false` (WritePolicy.h:18). 2R reports green on GCC and Clang host-test and host-sanitize (144 PASS), qa 420/0, qa-known-failures 0, contracts, diff-check, workflows and devkitA64. Waiting on exact-head CI and the gen15-integrated-candidate artifact. |
| Known limitation (decided) | Gen I–III Home cards have no STALE state: a failed source shows "Party preview unavailable". Gen IV/V keep the last data as STALE. Not changed, because it is hardware-accepted Legacy behaviour. |
| QA-F6 (Low, C) | `src/Legacy/RetroArchFRLGDiscovery.cpp:114`: identityHint substring-matches release names across the whole path. Confirmed in code and routed to C. |
| Divergence noted | Create default species is #1 for Gen I/IV and #25 for Gen II/III/V. Kept as is: hardware-accepted and documented. |

**URGENT: Gen V FAILED owner hardware acceptance** on candidate `fa9cfcea` (NRO `57b4ec2b…`). Thread 2 is repairing it on #307 (unfrozen). Gen VI/VII merges are on hold. Thread 3 is standing down so the usage quota goes to the repair.

**Stage B — complete Gen V (Black, White, Black 2, White 2)** on PR #307.
Stage A (Gen I–IV) is software-complete on #306 and waiting on hardware testing.
Stage C (Gen VI) is starting in parallel.

## Hardware acceptance

| Build | Code SHA | NRO SHA-256 | Status |
|---|---|---|---|
| **FINAL Gen I–V campaign candidate** (#307) | `a0633b6d53a582aaae2be5bf91e9fde0c10d27ac` (tree `945df45d892aaa3082a57927ffbfa8f6d54365cc`) | `55977952445e2b987044dca40b1cd58d681b901eeff5d2cfced61ff8caee68ea` (`PokeBank-NX-Gen1-5-Candidate-a0633b6d.nro`, 175,632,049 B; run 38732921202, artifact 11970313582, zip sha256 2675e35a…705d) | **NOT DEVICE TESTED**. CI 17/17 success on the exact head (coordinator-verified). Report: #307 comment 6192368775. |
| **QA-F9 (owner-reported, High)** | The green "Exact-head audit of the published candidate NRO" on `a0633b6d` (run 38732921392) audited the OLD `fa9cfcea` NRO (run 38673482742) via hard-coded defaults in `app-qa-nro-audit.yml`. It is INVALID as evidence: the candidate is NOT independently audited yet. Fix assigned to Thread 4 (identity file plus run/artifact binding plus staleness guard); 2R merges. The candidate binary is unchanged. |
| Integrated Gen I–IV / Product Home | `0760334fb850ca46b5171c55f690998d7680d4e7` | `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91` | **Owner accepted** (Oct 8) |
| Earlier #306 candidate | (earlier #306 head) | — | Owner confirmed: artwork restored, Gen IV launches directly into DraStic |
| #306 Gen I–IV candidate | `9bf9d492` (PR head `4b9b9f65` adds docs only) | recorded in a #306 PR comment | **NOT DEVICE TESTED** |
| **Integrated Gen I–V candidate** (#307) — **FAILED hardware** | `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe` (tree `5de23dc2`) | `57b4ec2bebf6f15cac0ee59218672fd6acc029c1e025abbe159a322aa1bccd23` (175,435,441 B; run 38673482742, artifact 11945232986) | **NOT DEVICE TESTED**. Owner checklist in the #307 comment. #307 is frozen during the test. |

## Generation and game coverage

Three separate columns: software / automated verification / hardware. These are
conservative estimates.

| Gen | Games | Software | Automated verification | Hardware |
|---|---|---|---|---|
| I–III | RBY, GSC, RSE/FRLG | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| IV | DPPt, HGSS | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| V | BW, B2W2 | **repair in progress** (UI does not match Gen I–IV; Create unusable; real B2 save decodes as empty) | CI green on `fa9cfcea`, but CI did not catch the failures | **FAILED** on `fa9cfcea` |
| VI | X, Y, OR, AS | ~60% (PK6/SAV6, staged Edit/Clone/Remove, party stat recompute, staged Bag, read-only box names/Pokédex, editor provider, sources; not wired into the app) | 7 Gen VI test binaries under GCC/Clang ± ASan/UBSan; differential vs PKSM-Core | — |
| VII | SM, USUM; LGPE as its own family | ~40% (read-only SAV7 + MemeCrypto verify; PK7 staged Edit/Clone/Remove plus staged Bag up to signing; party stats; read-only box names/Pokédex; export blocked on re-signing evidence; LGPE research only) | Gen VII focused CI green on #311 | — |
| VIII | SwSh, BDSP, PLA | ~8% (PKSE-derived read code; research note `docs/gen8/SWSH_RESEARCH.md`; installed-save path confirmed read-only; `Integration/Gen8` lane approved, SWSH first) | none yet | — |
| IX | SV, Legends: Z-A | ~2% (`BaseStatsGen89`, MyStatus validation) | none | — |

## Gen V: implemented vs missing (PR #307 `a17c503a`)

**Implemented in software:**
- Exact BW/B2W2 source/profile assignment.
- Pinned Primary/Backup copy selection, with fair per-provider scan budgets (#304).
- Native Trainer, Party and 24 Boxes read.
- Shared staged PK5 Edit, Review and app-owned export.
- Native empty-slot Create (#302; Erratic EXP fix live), plus box Clone and Remove.
- DraStic/melonDS launch with Unova ROM header identity (IRB/IRA/IRE/IRD).
- Gen V Bag backend: strict decode, staged quantity/Add/Remove, bag-aware export (`BAG_CHANGED`).

**Added by Thread 2 (`a17c503a..60c48c1b`, 10 commits):**
- Bag in the shared Items screen, with Review rows and Bag export.
- Product Home Items opens the Gen V Bag through the Save Instance chooser.
- Moving Pokémon between boxes at Gen IV parity, plus read-only origin fields.
- Trainer Name, Money and Play Time staging, with the export whitelist and a `TRAINER_CHANGED` manifest entry.
- Read-only Pokédex.
- End-to-end assign → reopen → copy → stage → export test.
- 572 randomized export validations.
- `patches/gen5-create` removed.
- `gen15-integrated-candidate.yml`: one Gen I–V NRO.

**Missing:**
- QA-F1 fixed (`3b35b3f`) and QA-F3 fixed (`fa9cfce`; Gen II's equivalent is Thread 1's).
- Hardware acceptance of `fa9cfcea`.
- Trainer fields beyond Name, Money and Play Time.
- The integrated Gen I–V candidate NRO.
- Automatic return to PokeBank NX after DraStic (shared issue with Gen IV).

## Active PRs and branches

| Lane | Branch | HEAD | CI | Owner |
|---|---|---|---|---|
| #306 Gen I–IV | `claude/gen14-final-20261014` | `4b9b9f65` | 9/9 green | Thread 1 |
| #307 Gen V integration | `claude/gen5-integration-20261014` | `fa9cfcea` (**frozen for hardware test**) | **9/9 green** | Thread 2 (standing by) |
| Gen VI native | `claude/gen6-native-20261014` (draft #309 → #307, CI only) | `5e683690` | see PR #309 | Thread 3 |
| Gen VII native | `claude/gen7-native-20261014` (draft #311 → Gen VI lane, CI only) | `08a4359b` | see PR #311 | Thread 3 |
| App QA | `audit/app-qa-20261014` (draft #310 → #307, CI only) | `a726610` | 12/13 green when last checked (GCC host still running) | Thread 4 (standing by) |
| Coordinator | `claude/optimistic-euler-ph2z4f` | docs only, on `a17c503a` | — | Coordinator |
| #300 / #304 / #302 | preserved source lanes | `58bd564b` / `4d8c267e` / `6e27bbd5` | green | Contained in #307 |
| #305 / #280 | `claude/gen4-capture-ball` / `fix/gen14-action-tap-empty-parity-20261011` | `b8eb006e` / `dcd855a7` | — | Rule confirmed by test / #280 tests change Gen III/IV surfaces → Thread 1 |

## Gen V hardware failures (owner test of `fa9cfcea`)

1. **Wrong first screen.** `UI.cpp` `handleGen5View()` forces Party mode with the detail view open, and `Gen5SharedReadOnlySurface.inc` (1,886 lines) intercepts all Gen V input. Gen IV opens the storage hub instead.
2. **Empty box → A shows only Cancel.** Add is gated by `emptyCloneDestination()`, the slot must be zero-filled, and the provider does not advertise the PokemonCreation capability.
3. **Real B2 DraStic `.dsv` decodes as empty.** Name shows unsupported glyphs; Money, TID and SID are 0; gender shows male for a female character; party, boxes and dex are all 0. Suspects: copy/counter selection, layout identification, decoding.
4. **Home card portrait assumes male** when the gender is unknown (`trainerPortraitForGame`). Name, party and dex are missing from the card.
5. **Separate "Trainer - Generation V" modal** with broken navigation.

## Gen V repair progress (Thread 2, #307, after `fa9cfcea`)

| Commit | Change |
|---|---|
| `395e2a6f` | Classify both copies; refuse a blank/stale copy with an explicit reason, never swapping copies on its own; on-device diagnostics report (no personal data) |
| `3cbf3683` | Create offered on retail encrypted-blank slots; Remove writes the retail blank |
| `b9c1ea8d` | One copy gate shared by Save Instances, Home card and open; neutral portrait when gender is unknown |
| `7123035d` | Opens on the shared loaded-game hub; Trainer uses the shared panel |
| `80bfce6c` | Create/View/Edit use the shared three-panel Pokémon workspace |
| `1b21022a`, `2ad496df` | Diagnosis docs |

- `35cbf163`: NewGameNoTrainer state, from the real-save findings.
- **Repaired candidate:** `PokeBank-NX-Gen1-5-Candidate-35cbf163.nro`, code `35cbf1637da41e9607f2d524354c51d70e401fb2`, tree `9cb8fde7`, 175,451,825 bytes, SHA-256 `16aa25333b4a8d0e07df3dc458f4695f170f2f08c7ab6c023bd07a5b839ec787` (run 38682361760, artifact 11948329433). **NOT DEVICE TESTED.**
- CI: 9/9 green. **FAILED owner hardware test (editor parity):** navigation mutates values, the middle panel is inaccessible, wide action menu, raw ID keypads, Gen V-specific move and species pickers, "cached party" Home card, "Linked save" provider labels, and Box Summary stats showing 0. Repair dispatched to Thread 2R (editor) and Thread 2H (Home card and providers).
- No Gen I–IV integration files touched.
- Consolidation is partial. Gen V opens on the shared hub and uses the shared Trainer panel and the shared three-panel Pokémon workspace. `Gen5SharedReadOnlySurface.inc` is still 1,848 lines (it was 1,886; 920 lines changed) and owns hub activation/Exit/Review and the entered panels. Storage is refused for Gen V.

## Gen V parity repair round 2 (after the `35cbf163` hardware failure)

- **Thread 2H done:** `claude/gen5-home-providers-20261015` @ `ece88e27` (draft #315).
  - Generation-neutral `HomeCardSnapshot` cache, refreshed only on stat or assignment change, with an explicit STALE state.
  - The Gen V card shows the "Current save party".
  - Real providers via `SaveProviderIdentity` ("Nintendo DS • DraStic"); "Linked save" is removed.
  - Width-measured label ellipsizing.
  - Contract tests replaced one-for-one and are stricter.
  - Not done: Gen IV card shiny badges (Gen IV output left unchanged); Dekopon/Azahar are recognised as labels only.
  - Thread 2R has been told to merge it forward.
- **Thread 2R in progress on #307:**
  - `888b886c`: cited BW/B2W2 move data table.
  - `2af25639`: PID-coordinated Gender/Shiny/Ability with exact replay.
  - `5d8b833e`: Gen IV editor workspace extracted into the shared `SharedPokemonEditorSurface`.
  - `b7c73365`: test source-path update (approved).
  - `de22e35f`: Gen V View/Edit/Create now run through the shared surface. The Gen V `adjustField` navigation is gone. The Actions menu uses the shared `NarrowPokemonMenu`.
  - `f0419926`: the owner's 14-point shared editor contract test for Gen I–V.
  - `ad95b2de`: Trainer and Items follow the Gen I–IV list conventions.
  - `6efd8c60` / `28a97b0c`: an explicit "Change PID" confirmation dialog, plus tests.
  - `9c038d91`: Thread 2H merged forward. GitHub then auto-marked #315 as merged, which was intended.
  - `Gen5SharedReadOnlySurface.inc` still holds Gen V's Bag, Trainer editor, Review, clone destination and the field adapter (1,942 lines).
  - The NRO is still pending.

## Owner real-save analysis (2026-10-15; files analysed only in the coordinator scratchpad, never committed)

- All four Gen V DraStic `.dsv` files (B/W/B2/W2) are 0x80000 bytes plus the standard 122-byte DeSmuME footer. Identification is correct: BW footers at 0x23F00/0x47F00, B2W2 at 0x25F00/0x4BF00, version bytes 20–23.
- **They are freshly initialized new-game saves:** empty name, TID/SID 0, play time 0:00, money 0, party 0, and all 720 box slots encrypted blanks. The Platinum control save has real progress.
- So the empty data PokeBank showed was **correct for these files**. The real defects are: (a) an uninitialized trainer is shown as "(unsupported glyphs)" / Male / male portrait instead of an explicit no-trainer state; (b) Create rejects every empty slot, because real empty slots are encrypted blanks, not zero-filled (the cause of "only Cancel").
- **Resolved:** the owner confirmed those Gen V files really were blank; there were no in-game saves yet. The decode was correct. Thread 2's `35cbf16` adds an explicit new-game state for such saves.

## QA results (Thread 4, final, 2026-10-15)

- **No crash, UB, out-of-bounds read or round-trip violation** in any Gen I–V parser, PK1–PK5 decoder, Bag decoder, Gen V copy selector or Save Instance identification.
  - 8 fuzz targets, plain and with ASan/UBSan, under GCC and Clang.
  - libFuzzer: 60 s per target, plus earlier 240 s soaks.
- Export contracts C1–C5 cover all 17 export paths.
- Candidate NRO `fa9cfcea` was audited in CI from artifact 11945232986: size and SHA-256 match, the build embeds `fa9cfcea`, there are 4,267 RomFS files, and every required asset is present (run 38675885758).
- Findings:

| ID | Severity | Status |
|---|---|---|
| QA-F1 | Low | Fixed in `3b35b3f` |
| QA-F2 | Medium | Open; owner decision on deleting 3 invalid workflows |
| QA-F3 | Low | Fixed for Common and Gen V in `fa9cfce` |
| QA-F3b | Low | Open; Gen II export transaction, Thread 1 |

## Safety audit (2026-10-15)

The PKSE-derived installed-save inject path (`restoreBackupToTitle`, used by `saveTrainerInfo*` with `injectToTitle`) refuses before mounting, via `Safety::canWriteTo(LiveGame)`. That checks the compile-time `LIVE_SAVE_WRITES_ENABLED = false`, and `test_write_policy`, `test_source_mutation_policy` and `test_gen4_safety_contract` guard it. `writeBlocksToSaveData7LGPE` only patches workspace images.

## Blockers

0. Usage limit: three threads plus the coordinator use up the 5-hour quota in about 1 hour. Thread 4 resumed to audit the candidate.

1. Gen I–III RetroArch exit and 2. DraStic "Quit to Launcher": reassigned to Thread L by the campaign directive. Findings and the opt-in fix are in `docs/handoffs/LAUNCH_RETURN_PROGRESS.md` (L branch). They need the owner's A/B device test.
3. Hardware test of the `9bf9d492` candidate is pending with the owner.
4. ~~#307 ownership~~: resolved. The legality session confirmed it stopped at `a17c503a` and left draft handoff `claude/gen5-handoff-from-legality-20261014` @ `fc519414` (Bag in the Items panel; unbuilt) for Thread 2 to review.
5. Native builds outside CI depend on pulling the devkitA64 Docker image, which can hit the Docker Hub rate limit.

## Owner decisions pending

1. Approve integrating Gen VI (later Gen VII) into #307 after the Gen V candidate. The integration requests are in `docs/gen6/integration-requests/01-02` on the Gen VI lane.
2. Gen VII re-signing: needs a "payload tail" diagnostic from one game-written SM/USUM save. PKHeX and PKSM-Core differ here, so SAV7 export stays unsupported until then.
3. Approve bumping the `memecrypto` submodule: `sha1.c` has undefined behavior in a signed shift, and `rsa.c` leaks memory.
5. QA-F2: fixed by gating to `workflow_dispatch` (`96a44685`). Deleting the stale one-time workflows still needs owner approval.
4. Confirm the Dekopon (`sdmc:/switch/dekopon/…`) and Checkpoint (`sdmc:/3ds/Checkpoint/saves/…`) paths on the real SD card. Azahar has no Switch build, so it is unsupported.

## Next implementation tasks

1. Thread 2: connect the Gen V Bag to the shared Items screen and Review, reach Gen V editor parity, add the Trainer and Pokédex work, then build the integrated Gen I–V NRO.
2. Thread 3: SAV6 (XY/ORAS) layout, checksums and block map from cited references; PK6 crypto; strict parse/serialize; staged PK6 edits with verified export; a Gen VI `ExactFormatEditorProvider`; Checkpoint/JKSV and manual-link sources; research into verified Azahar/Dekopon save locations.
3. Thread 4: parser fuzzing under ASan/UBSan; source-immutability tests for every export; an exact-head NRO audit tool.
