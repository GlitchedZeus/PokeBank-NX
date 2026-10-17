# PokeBank NX — Main Development checkpoint

Maintained by the Main Development coordinator. Only the coordinator edits this
file; threads report to it. GitHub is authoritative: re-fetch before relying on
any SHA below.

**Last updated:** 2026-10-17 17:15 UTC (Round 2: #307 = 19dd0db; lanes stalled on the 5-hour limit 12:30–17:10, resumed).

## Active milestone: UNIFIED GEN I–V EDITOR CAMPAIGN (owner directive, 2026-10-16)

Goal: ONE consistent Gen I–V editor (generation differences only in the data adapters), Phase 0 controller repair first (#320), then ONE NRO for hardware testing. #307 stays open and draft and is not merged by any thread; #313 stays open; #314 is untouched; no Gen VI+ and no Master Vault. #307 head at campaign start: `c9e243343476a7abdbe43e69a1a1b1cf16b47734`.

| Lane | Session | Branch | Owns | Phases |
|---|---|---|---|---|
| 2R | session_01Gn6iaKx8qUJWbAydeQ6ST6 | `claude/gen5-integration-20261014` (#307) | SaveSelectScreen, UI.cpp, NavigationRepeat.h, ScreenChrome, Common, AppShell; integration and the single NRO | 0 (Home/global input, #320), 8 (shared scroll repeat), 14 build |
| C | session_012Qk8aPZvVGKZhMUyUC6rya | `claude/gen14-campaign-20261015` | Gen1*, Gen2*, Classic*, TrainerViewScreenGSCOverlay*, Gen I/II integration | 0 (Gen II Create, `ae099c77`), 1–7, 9–11 for Gen I/II, Gen I/II bags |
| S | session_019GeN9mKTkgfpfuqDGPzWhz | `claude/shared-editor-gen35-20261016` | Shared* headers, MovePicker*, NarrowPokemonActionMenu, SharedPokemonEditorSurface, Gen3/4/5 surfaces, LocationNames | Shared Moves panel/chooser/footer/"Origin & Legality"/inline toggle (first push), then 1–7, 9–12 for Gen III–V and their bags |
| I | session_01WwfzZtJWqDsNSrMV866RJE | `claude/item-quantity-policy-20261016` | include/src Inventory/ItemQuantityPolicy | 2 (source-backed per-game unique/max quantity policy) |
| QA (4) | session_01VXXPj238uqedMPANHRVViz | `audit/app-qa-20261014` | tests/test_qa_*, docs/qa, QA workflows | 13 (cross-gen contract suite, 20-game matrix), 14 exact-binary audit |

Status:

| Item | State |
|---|---|
| #320 Home | **Pushed `65cbd69a`** (a fast-forward of c9e24334). The coordinator verified it in code: requestGameLaunch and appExitRequested=true each have exactly one site, in the QuitConfirm Y branch (SaveSelectScreen.cpp:3221/3231). decide(): A/B stay, only Y alone quits. Root B, Options Exit, ZR and legacy A only open the dialog. 2R reports GCC/Clang host-test, sanitize and qa green (327 PASS each), plus devkitA64 OK. The stale-candidate audit check is red by design. Opt-in trace: `config/input_trace.enable` → `logs/input-trace.log`. NOT DEVICE TESTED.
| Gen II Create | C `ae099c77` **merged into #307 at `eb0feb0a`** (the coordinator verified both parents; the merge is clean). 2R reports 329 PASS on GCC/Clang host-test, sanitize and qa, plus devkitA64 OK. NOT DEVICE TESTED. |
| Phase 8 scroll | Root cause: NavigationRepeat counted frames (18/4), so on a slow screen it ran at 900/200 ms. Fix: a wall-clock HoldRepeatClock in the one shared engine (immediate first step, 250 ms delay, 67 ms repeat, one step per frame), with the keyboard on the same clock and a timing test at 16/33/50 ms frames. Pushing after gates. Trace lines now carry T=ms. |
| Input chain | No button remap (NpadStandard, logical A/B). Touch goes through the top layer only. No stale edges. Open owner question: in the full Games browser L/R switch PROFILE (documented). Kept for this round, with the footer to show "L/R Profile". |
| Quota | All lanes stopped on the shared usage limit 07:40–09:10 UTC. Resumed 2R, C, S and I at 09:12. QA is held until lane code lands. |
| 10:15 check-in | #307 `5150d061`, the coordinator verified the merge ancestry: Phase 8 `c7fd839e`, I `d08c5aa8`, C `dcd8770c`/`e59df023`, S `7fd8d0e4`. 2R reports one full gate green: GCC/Clang host-test, sanitize and qa 335 PASS, 24/24 py, devkitA64. 2R is gating the next batch, b2166f6 (L/R Profile footer, C `47cac826`, S `41271d76`). Queued after that: C `4203ce0d` (Phase 1/5 list fit, Gen I/II bag Remove confirm + a single quantity query, Origin & Legality labels, shared picker footers, shared Pokérus rule) and S `0017d186` (Gen IV/V bags on QuantityPolicy, SharedDataPaneLayout, Gen III PID label). QA `e71d5b69`: cross-gen contract suite CG01–CG10 (CG01b/06/07/10 enforced; the rest XFAIL until lanes land), candidate mode fatal in the exact-binary audit, 20-game UI matrix. Open: S to say which TrainerViewScreenBase dialogs are reachable from Gen I–V; S Phase 12 field-access parity (Gen III PID-derived fields, Gen V Shiny/Language); C ClassicInventoryCatalog → QuantityPolicy delegation (Makefile exception approved); the owner's answer on the hidden Gen I row. Usage: the 7-day limit shows a warning (resets ~09:00 UTC 10-17). |
| 11:20 | #307 `e4afc6fd` (coordinator verified the ancestry): 08c7453 Gen V qa order; C 6e900e27 (labels, footers, Pokérus rule, native fix), 225ac415 (Gen I/II on SharedMovesPanel) and 5d9dc8e5 (ClassicPokemonActionMenu → NarrowPokemonMenu adapter); S 37769b04 (Phase 12 field parity, Gen III Unown letter, Gen V Shiny), 54c713a7, 647917ed (TrainerViewScreenBase Gen I–V routing via genericUiAllowedForSession; SaveConfirm shared footer; Gen IV bag via Gen4BagQuantity/QuantityPolicy) and af42d637 (pret-derived Gen I/II move types, era-aware maxPP). 2R gate on e4afc6f: GCC/Clang 345 PASS (host, sanitize, qa), 24 py, 47 workflows, devkitA64. **S lane DONE** (Phase 9/12 closed). Documented limitations: Gen III Party is View-only ("move it to a box to edit"); a Gen IV-format Bank record is refused in a Gen IV session. QA: CG11 bound to genericUiAllowedForSession. Remaining (C): Gen I/II move chooser on drawChooserRow, ClassicInventoryCatalog→QuantityPolicy (Makefile exception approved), Gen II on SharedDataPane::layoutRows, Gen I/II Party edit-path check, Gen I default species 25. Then the NRO. |
| 16:20 | C FINAL `2d40753d` (Gen II on SharedDataPane::layoutRows, Gen I default #25), verified. GO was given at 12:30, but 2R and QA hit the usage limit and nothing was built. Coordinator pre-check: QA's cross-gen contracts in --candidate mode on e4afc6fd + 2d40753d FAIL 6 contracts (CG01/03/04/05/08/09, 61 lines; the full list is in the coordinator session). **Candidate HOLD** until the suite passes. Split: 2R (SaveSelectScreen/BackupSelectionScreen footers + delete-confirm touch layer), S (Gen III–V wording/footers/modals, LocationNames tags, Base dialogs, Gen IV :2008), C (Gen I/II wording/footers/modals, ClassicInventoryOverlay :229), QA (ActionSheetModel out of scope after CG11, enforce XPASSes, CG08 false positives). |
| 17:20 | #307 `4c9dd750`: 2R CG05/CG08 + Save Backups delete moved to Y (`8e2d05d3`); C contract fixes `ecad1a0a` + CI link fix `41358797`. S contract unit `e86741c`: CG01/03/04/05/08/09 in Gen III–V, LocationNames Gen III disambiguation → (Hoenn)/(Kanto)/(Sevii Islands), Base dialogs beginModal + footer in classic sessions only. Coordinator test-merge of 4c9dd750 + e86741c with QA suite 030d07a in --candidate mode: only 3 lines left. PickerDialog.cpp:202 (unreachable per CG11; QA to scope it conditionally). PokemonDetailsDispatcher (router; Gen2PokemonDetailsModal lacks beginModal, C fixing). Then GO. |
| 17:45 | C `72e73a99` (Gen2PokemonDetailsModal beginModal); S `c01e4fd` (strict CG08: 5 confirms beginModal + own footer, classic sessions only). QA `fcc8295`: strict CG08 (only beginModal/beginTopTouchLayer counts); CG01–05/09/11 enforced; ActionSheetModel/PickerDialog/generic details out of scope only while CG11 holds. **Coordinator run on #307 4c9dd750 + C 72e73a99 + S c01e4fd: 0 contracts failing in --candidate mode.** Last change before GO: the classic-session StoragePanel Release and ItemEdit Remove confirms move from A to Y (S). Save Backups delete moved to Y earlier (2R `8e2d05d3`). |
| 18:08 | **GO.** S `1082c6fc`: classic-session Bank Release and Item Remove confirms commit on Y only (A/B/Y+B cancel; Gen VI+ unchanged). Candidate inputs: #307 4c9dd750 + C 72e73a99 + S 1082c6fc, merged tree `29bd15749dd9ce70d6aa0a6304fd29f48155dece`; QA suite fcc82950 in --candidate mode: 0 failing (coordinator run). 2R: one full gate (incl. Clang sanitize and the candidate contracts), then push = THE candidate, then the QA-only follow-up (CANDIDATE_UNDER_AUDIT.env, RETEST doc, QA branch merge). QA verifies the QA-F9 audit line by line. |
| 18:40 | **CANDIDATE** `PokeBank-NX-Gen1-5-Candidate-7fea1797.nro`. Code `7fea17971906cae557a7207bfb7b0bed3548cea2`, tree `29bd15749dd9ce70d6aa0a6304fd29f48155dece` (= the coordinator's verified merge tree). Build run 38946941097 (success), artifact 12066747065 (zip digest 02677af4…c184), NRO SHA-256 `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5`, 175685297 bytes. QA-only follow-up `8587f20b` (QA fcc82950 merge, CANDIDATE_UNDER_AUDIT.env, docs/hardware/RETEST_7fea1797.md). **Exact-binary audit run 38947904299 / job 117131594707: PASS.** Coordinator read the log line by line: run binding to head_sha 7fea1797 / success / gen15 path / artifact 12066747065 not expired; tree check; staleness guard; download digest 02677af4…; sha256 + size + BUILD_IDENTITY (APPLICATION_SHA, TREE_SHA, RUN_ID, DEVICE_ACCEPTED=false, SOURCE_SAVE_WRITES=DISABLED) + SHA256SUMS; cross-gen contracts in CANDIDATE mode on candidate-src: 0 failing; nro_audit.py: embedded 7fea1797, NACP 0.1.0-alpha, RomFS 4267 files, NRO AUDIT: PASS. The 8587f20b candidate-workflow artifact is NOT the candidate. NOT HARDWARE ACCEPTED. |
| 19:05 | All 13 CI workflows on #307 8587f20b: success (Host Tests GCC and Clang, Native PR Gate, Fuzz, Contracts, NRO Audit, Integrated and Gen I-IV candidates, Gen V focused, packed move, touch gating, workflow hygiene). QA independently verified the QA-F9 audit (PASS); its 46c5a0b (XFAIL table empty, all CG enforced) is held on the audit branch until the hardware results. All lanes idle. Next: the owner's hardware test of 7fea1797 against docs/hardware/RETEST_7fea1797.md. |
| 21:55 | Daily check. Lane heads unchanged since 19:05: #307 8587f20b, C 72e73a99, S 1082c6fc, I d08c5aa8, QA 46c5a0b2 (held), main 336954e4. #307 still draft, unmerged, mergeable clean. No hardware result yet (#320 has no comments). No new blockers. |
| R2 start | Owner hardware round 1 on 7fea1797: 42 of 54 PASS. Confirmed FAIL: B01/B02/B03/B06 (Bag Add asks quantity first), S01 (Gen V PP captions), S03 (Gen V left-panel clipping/parity). Not verified: B04, E03, E11, L06, S04, S06. Round 2 campaign dispatched per docs/handoffs/ROUND2_LANE_PLAN.md (lanes I, C, S, P-new, R-new, 2R, QA). |
| R2 09:05 | #307 = 315ad61 (QA 46c5a0b2 merged). S pushed dddfe353: SharedMenuGeometry.h, SharedFieldState.h, NunitoMetrics.h, test_shared_menu_geometry.cpp, LocationDisplayNames Gen V id-3 fix. QA pushed ee0aaaab (round-1 results, round-2 status, phase-13 steps). 2R: Phase 7 emulator backups + L06 test implemented locally, not yet pushed. Lanes P, R, I, 2R hit the weekly usage limit at ~07:45 UTC (reset 09:00); resumed at 09:01. C running. P and R branches not yet on origin. |
| R2 12:15 | 2R pushed Phase 7 to #307: 9cc6caa9 (emulator-save Backups list; only app-owned export records deletable, Y-confirmed; original sources read-only). R pushed fe48d713 on claude/reroll-mechanics-20261017 (Gen III/IV PID construction preserves shininess). P branch still not on origin. All seven lanes hit the 5-hour session limit (~09:05–12:10 UTC); all resumed at 12:12. Pending merges into #307: S dddfe353, QA ee0aaaab, then I/C/P/R/S as reported. |
| R2 12:30 | #307 → 19dd0db: L06 test (test_party_summary_tap_routing.cpp), S dddfe353, QA ee0aaaab, Nunito pinned to google/fonts 47d54ca1. Gate: 25/25 contracts, CG candidate 0, devkitA64 OK, GCC+Clang host-test/sanitize/qa green. 2R local (next push): Phase 6 launch dialog 14a592b (LaunchConfirm geometry, glyph buttons, Y-only), QA b6da32e+85c6ac4 (round-1 regression gate, 116 guards), P 30b6ea4. P 5bea036 (all scrollbar call sites) waits on P full-gate report. I: B01–B04/B06 fix gated, SharedBagAddPicker follow-up drafted (4 QA literal edits approved). R fe48d713 unmerged pending handoff. |
| R2 17:15 | Lanes 2R, S, C, I, P, R hit the 5-hour session limit ~12:30–12:50 UTC (reset 17:10); all resumed 17:12. Since 12:30: P pushed 0867c9b8 (scrollbars on lists that had none; full-gate report still outstanding), R pushed 9c90eeb5 (REROLL_MECHANICS.md), QA merged 19dd0db forward (f8c0fecd). I and C have not pushed yet; 2R Phase 6 still local. #307 unchanged at 19dd0db. |
| Lane I | **Done:** `d08c5aa8` (ItemQuantityPolicy, 20 games, PKHeX PlayerBag* caps; key items/HMs/Gen V TMs unique). The coordinator built and ran its test: OK. Open findings routed to C: Gen III 99 cap in ClassicInventoryCatalog, Gen I id 50 duplicate, test_classic_inventory_catalog GS BALL. |
| Lane C | `dcd8770c`: Phase 3 (no Make Shiny/placeholders; order View, Edit, Clone, Release, Legality, Review, Cancel) and Phase 6 (instant Y reroll keeping shiny, Gen II gender and the Unown letter; 200-press tests). Verified in code. Next: Phase 10, then the shared unit for Phases 1/4/7. |
| Lane S | `7fd8d0e4`: shared unit (SharedMovesPanel, SharedMoveFacts, SharedEditorPresentation with footers, Origin & Legality, toggles and viewport, SharedEditorChrome beginModal). Rows counter and "commits to draft" removed; Gen III–V surfaces adopt it. |
| Lane C (cont.) | `e59df023` Phase 10 (Shiny Yes/No, inline Pokérus that restores the original byte; gender pin) + Phase 9 (Gen II DATA rows wrap inside the pane, with a bounds sweep over 128 Crystal landmarks). `47cac826` Phase 11 (Gen I has no met-location field, now pinned by a test; the Crystal Met Location chooser is proportional, with no raw IDs). |
| Lane S (cont.) | `1cdbe73` Phase 3/4 (shared order View, Edit, Clone, Move, Release, Origin & Legality, Review, Cancel; actionLabel → "Origin & Legality"). `41271d7` Phase 6/10/11 for Gen III–V (instant IV/EV reroll keeping the PID, tested 42×120 on real PK3/4/5; inline Pokérus/Language; LocationDisplayNames without (RSE)/(ID n)). |
| Combined check 2 | The coordinator test-merged eb0feb0a + I d08c5aa8 + C 47cac826 + S 41271d7. One conflict, in tests/test_touch_pokemon_editor_direct_rows_contract.py, resolved as the union. GCC host-test exit 0, all py contracts pass. The resolution was sent to 2R. Open items: two action-menu builders (C asked to make theirs an adapter over S's NarrowPokemonMenu); Phase 4 label leftovers in Gen I/II and ActionSheetModel.h. |

## Previous milestone

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
| **QA-F9 (owner-reported, High)** | The green "Exact-head audit of the published candidate NRO" on `a0633b6d` (run 38732921392) audited the OLD `fa9cfcea` NRO (run 38673482742) via hard-coded defaults in `app-qa-nro-audit.yml`. It is INVALID as evidence. **FIXED**: QA `cb03e27b` merged as #307 `c9e24334` (QA files only; empty non-QA diff vs `a0633b6d`). The **valid independent audit is run 38744338867, job 116453904424, success**. Its log, read by the coordinator, shows identity from CANDIDATE_UNDER_AUDIT.env; run 38732921202 head_sha a0633b6d, success, gen15 workflow; artifact 11970313582 (download digest 2675e35a…705d); tree 945df45d; staleness guard passed; BUILD_IDENTITY and SHA256SUMS match; NRO 175632049 B, SHA-256 55977952…68ea; embedded commit a0633b6d; RomFS 4267 files; NRO AUDIT: PASS. Audit record artifact 11977367025. Candidate binary unchanged. All 13 CI workflows on `c9e24334` completed with success (coordinator-verified via the Actions API). |
| **a0633b6d hardware result** | **FAILED** (owner, 2026-10-16): (1) Gen II Create: A/B "enter wrong"; (2) app-wide navigation: A/Settings "exits the app without warning", R "opened the current save", owner cannot navigate. Coordinator analysis: Home root bindings are unchanged from accepted 0760334f; B at root exits with no confirmation (campaign gap); Settings+A has no exit path in code, so a crash is suspected in focus-change code new since 35cbf163. Assigned: 2R (Home/global input contract, exit confirmation, crash hunt), C (Gen II Create input). Owner asked for crash reports and repro details. |
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
