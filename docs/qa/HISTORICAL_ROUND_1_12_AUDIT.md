# Historical hardware QA audit, Rounds 1–12

Owner request (Nov 1): *"Stop losing bugs and features between hardware rounds."* This audit reads every hardware results document, checklist, worksheet, engineering intake and defect report from the first device tests (September) to Round 12, plus the owner's GitHub reports, and reconciles them against the current source. It changes no application code and no NRO; nothing is merged.

**Status rules.** A status is never inferred from a passing test, a commit message or a PR description. *Hardware Accepted* means the most recent owner hardware result for that behaviour is PASS; *(recheck)* marks accepted behaviour whose code changed after the accepted candidate and must be re-verified on the current candidate. Current source: `6c7c8e99` (application tree identical to candidate `066e1db9`, NOT DEVICE TESTED).

## 1. Method

1. **Extraction.** Every checklist item, owner result, owner comment, correction, request, decision, deferred item and requirement was extracted into one record each, with the owner's wording copied character for character (checked by script against the source file). 2,754 records from 6 slices: pre-round device tests (967), Rounds 1–3 (692), Round 4–6 + Oct 24–26 (429), Rounds 7–12 + all 332 GitHub issues/PRs and 800 comments (343), Round 11 worksheet + intake + Nov 1 QA findings (123), v1.0 requirements (200).
2. **Deduplication.** The 997 issue-like records (FAIL, NOT TESTED/NOT VERIFIED, PARTIAL, owner request, correction, decision, deferred item, requirement, engineering finding) were grouped by underlying problem into **276 unique issues** with stable `PBX-<AREA>-NNN` IDs. Coverage check: every one of the 997 records is listed under exactly one issue (or explicitly as noise with a reason); 0 missing, 0 duplicated.
3. **Tracing.** Each issue was traced through git history (repair commits), the automated tests, the current source at `6c7c8e99` (file:line evidence) and every later hardware result.
4. **Classification.** Exactly one of: Hardware Accepted · Fixed in Code — Awaiting Hardware · Open Defect · Missing Feature · Intentionally Unsupported · Needs Investigation.
5. **Checklist lineage.** Every checklist item was followed across rounds to find items dropped without acceptance (§5).

## 2. Rounds and candidates

| Round | Candidate | Owner hardware results | Records PASS / FAIL / NOT TESTED / PARTIAL / n/a |
|---|---|---|---|
| PRE | — | Sept–mid Oct device tests (multiple builds) | 287 / 128 / 27 / 6 / 601 |
| M1 | — | multilingual milestone defects | 0 / 0 / 0 / 0 / 8 |
| R1 | 7fea1797 | 42 PASS / 6 FAIL / 6 NOT VERIFIED | 169 / 24 / 30 / 1 / 62 |
| R2 | e10f307f | 6 PASS / 6 NOT VERIFIED | 13 / 0 / 6 / 0 / 143 |
| R3 | 62a2f8c3 | 70 PASS / 37 FAIL / 3 NOT TESTED | 101 / 37 / 3 / 0 / 129 |
| R4 | c60d4da0 | Oct 23 retests: 7 FAIL, 3 NOT TESTED (on c60d4da0) | 2 / 14 / 5 / 0 / 68 |
| OCT24 | c60d4da0 | 27 PASS / 6 FAIL / 4 unmarked + S06 not tested | 54 / 6 / 6 / 1 / 16 |
| R5 | 1ca2faec | 31 PASS / 8 FAIL / 3 NOT TESTED | 31 / 8 / 3 / 0 / 60 |
| OCT25 | — | 10 owner PASS items | 10 / 0 / 0 / 0 / 0 |
| R6 | 26186464 | 49 PASS / 9 FAIL / 1 NOT TESTED (see §4: second 54/4/1 report) | 49 / 9 / 1 / 0 / 92 |
| OCT26 | — | 19 owner PASS items | 19 / 0 / 0 / 0 / 1 |
| R7 | e88957f2 | **none recorded** | 0 / 0 / 0 / 0 / 67 |
| R8 | 6ec5ccb9 | **none recorded** | 0 / 0 / 0 / 0 / 16 |
| R9 | d6a20a2a | **none recorded** | 0 / 0 / 0 / 0 / 16 |
| R10 | 3634d1e6 | tested in R11 | 0 / 0 / 0 / 0 / 19 |
| R11 | 3634d1e6 | 34 PASS / 13 FAIL / 1 NOT TESTED (H01–H48) + 8 extra findings | 39 / 30 / 2 / 0 / 52 |
| GH | — | GitHub-only owner notes | 0 / 0 / 0 / 0 / 3 |
| R12 | 066e1db9 | **not device tested** | 0 / 0 / 0 / 0 / 22 |
| REQ | — | requirements | 0 / 0 / 0 / 0 / 200 |

Record counts include the same owner result repeated in several documents (results file, status page, regression gate); the unique-issue counts in §3 remove that duplication.

## 3. Totals

| Measure | Count |
|---|---|
| Historical unique defects (defect + safety issues) | 152 |
| All unique issues (incl. requests, requirements, decisions) | 276 |
| Hardware Accepted | 143 |
| Fixed in Code — Awaiting Hardware | 72 |
| Open Defect | 11 |
| Missing Feature | 20 |
| Intentionally Unsupported | 5 |
| Needs Investigation | 25 |
| Hardware Accepted but code changed since → R12 recheck | 94 |
| Missing v1.0 features | 13 |
| Safety-critical issues without hardware acceptance | 35 |
| Untested safety-critical functions (no owner hardware PASS ever) | 16 |

## 4. Record discrepancies the owner should know about

- **Rounds 7, 8 and 9 have no owner hardware results anywhere** (repository and GitHub). Their repairs were only exercised through the Round 11 worksheet on the Round 10 candidate `3634d1e6`.
- **Round 6 (`26186464`) has two owner reports.** The repository records 49 PASS / 9 FAIL / 1 NOT TESTED; `docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md` mentions a second report of 54 / 4 / 1 for the same build that is not in the repository, and `ROUND6_RESULTS_26186464.md` was rebuilt from the owner's Round 7 directive. The original reports and screenshots are missing.
- **Round numbering differs.** `tests/test_hw_round1_regression_gate.py` calls the Oct 24/25/26 results "Round 5/6/7". This audit uses candidate-based rounds (R5 = `1ca2faec`, R6 = `26186464`) and OCT24/OCT25/OCT26 for those lists. The gate's Oct 26 table also reuses R1 IDs (B01, C02 …) for different items.
- **Reused IDs changed meaning** (B08, R2-REVIEWMOVE); results under them must not be merged.
- **R11 H13 "Japanese saves read-only" is recorded PASS, but the owner wrote "i didnt test a japenese save".** It is not counted as hardware acceptance of Japanese saves.
- **R11 H46 "Export Copy reload and compare (OPTIONAL)" is PASS without saying whether an emulator load or a PC compare was done**; S06 (export loads in an emulator) is therefore Needs Investigation.
- **R1/R2 NROs are not archived** (GitHub HTTP 403); their artifacts expire 2027-01-14/16.

## 5. Requirements dropped from a newer checklist

31 checklist items or owner findings disappeared from every later checklist without ever being owner-PASSed; **21 are still needed**. Two events caused most drops: the **Round 8 split** (commit `66f8a53f`, owner Oct 28: "hardware acceptance must not depend on fixtures the owner does not own") replaced Round 7's 66 never-tested items with one "repeat a few favourite Round 7 checks" line and moved exotic cases to Group A automation, and the **Round 11 Vault/Bank items** were routed to the separate Vault plan (owner safety clarification 4).

| Item | Last round | Last result | Why it was dropped | Still needed | Owner words |
|---|---|---|---|---|---|
| PRE controller disconnect / reconnect (input works after reconnect) | PRE (DEVICE_TEST_CHECKLIST_SECOND K; GEN15 A03) | NOT TESTED | no reason recorded — DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md 'HOME/resume/sleep/controller reconnect' NOT TESTED (A0142); FOLLOWUP 2026-09-03 PASSed HOME->resume and sleep->wake only; no R1-R12 checklist item. R1 C09 is a second controlle… | **yes** |  |
| PRE Gen V Black 2 newly initialized save must report new game / no trainer (GEN15 B17) | PRE (GEN15 gate 2026-10-15) | FAIL | no reason recorded — GEN15_HARDWARE_ACCEPTANCE_GATE 'Previously observed Gen V hardware defects' row 3 (A0742); no R1-R12 item checks a new-game Gen V save. | **yes** |  |
| PRE save-discovery robustness: moved/missing/renamed save -> relink; junk/truncated .sav rejected; path aliases (GEN14 C6/C7, GEN15 B13-B15… | PRE (GEN14 / GEN15 checklists) | never tested (template rows) | moved to Group A automated — R8 Group A / ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md #7 'Deliberately corrupted saves' covers corrupted inputs only; moved/missing relink and alias rows are on no later checklist. | **yes** |  |
| AUD-04 Game launch and return (Gen I-III RetroArch exit crash; Gen IV/V DraStic Quit to Launcher) | R3 (62a2f8c3) | NOT TESTED | no reason recorded — Only mention after R3 is docs/hardware/ROUND3_RESULTS_62a2f8c3.md:132; not in R4 ROUND4_REPAIRS_AND_RETEST, OCT24, RETEST_1ca2faec .. RETEST_066e1db9; not in the gate (GUARDS_ROUND3 has AUD-02,03,05-10 only). Earlier: G… | **yes** | PR #92 2026-10-06 (quoted in docs/handoffs/LAUNCH_RETURN_PROGRESS.md:19): "RetroArch quit lifecycle: FAIL — quitting RetroArch drops/crashes to Nintendo HOME instead of returning to PokeBank NX." |
| E11 Rerolls keep the requested Shiny / Gender (and Gen II Unown letter) | R3 (62a2f8c3) | NOT TESTED | replaced by a broader item — docs/hardware/RETEST_e10f307f.md §3 'Rerolls (Phase 10) \| As E11 above'; ROUND4_REPAIRS_AND_RETEST R2-REROLL 'DV/shiny/Unown rules checked'; gate ROUND2_NOT_VERIFIED lists E11. The successor R2-REROLL was never owner-P… | **yes** | R1: "E11 i dont think  a  shiny and nonshiny can have the same stats this needs look into more."; R3: "Owner uncertain about Unown's odd displayed stats" |
| R2-REROLL Gen II Unown: letter, Shiny, DVs and Reroll agree (+ R6 C09 UNOWN-LETTER selector) | R7 B05 listed; last owner result R6 B07 / C09; R11 H22 FAIL (other sub-issues) | FAIL | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 B07: "Gen II Unown:<br>• Shiny restricts the available letters.<br>• Turning Shiny off and choosing a letter changes the DVs and the form.<br>• Turning Shiny on again changes the form.<br>• Reroll seems not to work, shiny or not.<br>• Choosing a letter doe… |
| R2-CHECK-011 Gen V event and Met Location rows distinguishable (+ R7 C09 focused event row detail line) | R7 B03 / C09 listed; last owner result R6 B05 | FAIL | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 B05: "The names now differ, but many rows still say "Event/special location; unverified". The owner wants to understand what each choice means and how it differs from the others." |
| NEW-18 / 7F Gen I-IV Trainer looks exactly like Gen V; Gen V Trainer Review button overlaps rows (R7 B04, C06) | R7 B04 / C06 listed; last owner result R6 B06 / C06 | FAIL | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 B06: "Gen I–IV Trainer still looks like a hybrid:<br>• yellow values where Gen V uses neutral text;<br>• awkward boxes and inconsistent row backgrounds;<br>• layout pieces that look patched together.<br>Gen V Trainer is the standard."; R6 C06: "Trainer loo… |
| E05 Inventory Help wording and glyphs Gen I-V (R6 B04; R7 B02 + R7 C01 'Bag + and Help agree') | R7 B02 / C01 listed; last owner result R6 B04 | FAIL | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 B04: "Problems with Inventory Help and glyphs:<br>• Gen I/II/III Inventory Help has stale wording such as "X Add remains available".<br>• Bag Help describes + as Options although + opens Review.<br>• Gen V Bag Help is titled "Game Browser Controls" but sho… |
| P1-RETURN (1) Gen II Unown Create -> Review shows an invisible screen; (2) one visible Review key per screen (R7 B06/B07) | R7 B06 / B07 listed; last owner result R6 B08 | FAIL | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 B08: "While creating Unown in Gen II, opening Review displays what appears to be an invisible screen. The application appears frozen, but pressing B exits the invisible screen and returns it to normal." / "Review also works by pressing + in the Party menu.… |
| R7 C02 Gen V export appears in Save Backups (7E audit gap G1) | R7 (e88957f2) C02 | never tested (owner reported it R6 C12 follow-up) | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 C12 follow-up: "**New bug:** after exporting an edited Gen V save, the exported copy does not appear in Save Backups, unlike the other generations." |
| R7 C03 Pokémon editor typography the same in Gen I-V | R7 (e88957f2) C03 | never tested (owner request R6) | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6: "**Pokémon editor typography is inconsistent.** Older-generation View/Edit screens use large white headings, while newer ones use smaller coloured section labels. |
| R7 C04 Quick Games (and other ordinary popups) close on a tap outside | R7 (e88957f2) C04 | never tested (owner follow-up to R6 C04 PASS) | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** | R6 C04 follow-up: "Also wants tap-outside dismissal on the **Quick Games** panel and other ordinary popups" |
| R7 C07 Gen I/II 'Origin & Legality' panel | R7 (e88957f2) C07 | never tested | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | **yes** |  |
| R8 H04 Bank storage: deposit one Pokémon, save Bank, restart | R8 (6ec5ccb9) H04 | never tested; R11 H41 FAIL | scope change — Dropped from R9 d6a20a2a Group H; ROUND11_ENGINEERING_INTAKE.md §5 'Vault / Bank (H38, H40–H43, bank search) ... a v1.0 feature with its own reviewed plan and branch'; owner safety clarification 4 'No unfinished Vault/B… | **yes** | R11 H41: "ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet" |
| R11 H40 In-save box rename (only Bank/'Legacy Storage' boxes rename) | R11 (3634d1e6 worksheet) H40 | FAIL | scope change — ROUND11_ENGINEERING_INTAKE.md §1 H40 routed to VAULT ('Separate Vault plan/branch'); intake :2977 'This is intended behaviour, not a regression, but the UI is inconsistent across generations' (source policy refuses rena… | **yes** | "can rename a box inside the save its doesnt allow me to select it, the only boxes i can rename are bank boxes or as their called right now in the app "legacy storage"" |
| R11 H41/H42 Bank deposit and withdrawal | R11 H41/H42 | FAIL | scope change — ROUND11_ENGINEERING_INTAKE.md §5 + owner safety clarification 4; RETEST_066e1db9.md intro 'no Vault'. | **yes** | H41: "ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet"; H42: "not supported yet" |
| R11 H43 Gen I/II/IV/V Vault availability | R11 H43 | FAIL | scope change — ROUND11_ENGINEERING_INTAKE.md §5; intake §4 'Move to Vault is not shown until the Vault exists'. | **yes** | "gen 1 does not have "move to vault or bank or what ever" " |
| R11 H38 question: how to move a Gen I Pokémon to Gen V (transfer path) | R11 H38 | PASS (question) | scope change — ROUND11_ENGINEERING_INTAKE.md §1 H38 priority Vault, routing VAULT. | **yes** | "how are we suppose to move a gen 1 pokemon  to like gen 5 ?? does it have to go through bank first or how we doing that ??" |
| R11 Extra 8 PKSE-style Bank search highlight before 1.0 | R11 Extra 8 | request | scope change — ROUND11_ENGINEERING_INTAKE.md §5 'Vault / Bank (H38, H40–H43, bank search)'. | **yes** | "pkse has a nice search function in the bank where it highlights the pokemon with the searched letter or the item  could we add just before 1.0." |
| R11 H27 keyboards: Japanese/Korean/all-language entry; language button in search bar | R11 H27 | FAIL | deferred by an owner decision — ROUND11_ENGINEERING_INTAKE.md H27 'Fix C (keyboards, owner question, v1.0+ feature, needs a design decision)' and open questions :2845; R12 H14 covers only name display/Hangul glyphs. | **yes** | "also in our keyboards we need to add the proper letters and charcters for japenese and korean and all languges is that possible in the nanog kayboard and is it possible to add a new button in our custom search bar that would also change the letters ... feedb… |
| PRE GEN15 F10 fault handling (no space, read error, power loss) and G08 hundreds of open/edit/cancel cycles | PRE (GEN15 gate) | never tested (template rows) | moved to Group A automated — Group A 'Bank missing ... interrupted save' covers the Bank only; R11 H44 'Do NOT intentionally simulate power loss on hardware'. Export fault injection and stress are on no later checklist. | no |  |
| PRE GEN15 A07 trainer-gender portraits / uninitialized trainer never guessed | PRE (GEN15 gate; GEN14 A2) | never tested (template rows) | no reason recorded — No R1-R12 item. | no |  |
| PRE live-write paths (installed-game / RetroArch direct writes) | PRE (DEVICE_TEST_CHECKLIST Test H) | NOT TESTED | deferred by an owner decision — RETEST_7fea1797.md §5 'No live emulator or installed-game writes; Inject Save stays disabled'; ROUND5_RESULTS 'S06 deferral is not write permission'. | no |  |
| PRE '# BALL' label (Gen II raw item ID 5) | PRE (DEVICE_TEST_CHECKLIST classic inventory, 2026-09-13) | FAIL | fixed and assumed fine — Record later_refs: 'Post-acceptance name-table cleanup with regression coverage; no new inventory hardware checkpoint required'. | no |  |
| PRE artificial idle / breathing sprite motion | PRE (DEVICE_TEST_CHECKLIST_SECOND prior feedback) | FAIL | fixed and assumed fine — Owner asked for removal; no later checklist row. | no | FAIL / REMOVE |
| R7 C08 Create Review rows look the same in Gen I-V | R7 (e88957f2) C08 | never tested | replaced by a broader item — docs/hardware/RETEST_6ec5ccb9.md 'How this round is split' + H10 'Repeat a few of your favourite round 7 checks'; commit 66f8a53f 'Round 8: owner checklist on ordinary saves; advanced cases move to an automated matrix'… | no |  |
| R8 H05 Bag quantity changed back -> Review shows no pending change | R8 (6ec5ccb9) H05 | never tested | no reason recorded — Not in RETEST_d6a20a2a.md / RETEST_3634d1e6.md / RETEST_066e1db9.md Group H; nearest R11 H33/H45 (PASS) do not include the revert-to-original case. | no |  |
| R8 H07 Gen II OT name length (7 characters) | R8 (6ec5ccb9) H07 | never tested | moved to Group A automated — docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md row #10 'Gen II OT name limit'; commit 66f8a53f ('also runs the Gen II OT prompt check no Makefile ran'). | no |  |
| R11 H28 request 'level 100 should have 1-2 moves' | R11 H28 | PASS (with request) | scope change — ROUND11_ENGINEERING_INTAKE.md §4 Required fields: 'The suggested "level 100 should have 1–2 moves" rule is **not** a game rule and is not invented' (engineering decision, 'owner may override'). | no | "a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves unless its breed or however that would be legal," |
| Gen I/II Create Met Location requirement ('this needs to be added for all gens' / 'universal rule') | R11 H07 / Extra 6 | PASS (with request) | scope change — ROUND11_ENGINEERING_INTAKE.md §4 Required fields: Met Location in Gen III–V only; 'Gen I has no met data and Gold/Silver have none; Crystal caught data stays optional' (engineering decision, owner may override; owner cl… | no | H07: "this needs to be added for all gens"; Extra 6: "also can we add the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add ... this should be a universal rule." |

### 5a. Omitted from the Round 12 checklist (`RETEST_066e1db9.md`)

| Item | Status before R12 | R12 coverage | Reason | Safety-critical |
|---|---|---|---|---|
| H32 Occupied destination safety (Import / Move / Clone toward an occupied or unavailable destination) | R11 H32 NOT TESTED ('dont know how to test this') | partial: H03 (Move) and H04 (Clone) include 'A on an occupied slot is refused'; Import into an occupied/unavailable slot and protected/full destinations only in Group A rows 4-5 (no owner action) | moved to Group A automated | **yes** |
| Gen I/II Create starts with no moves and needs at least one move (R11 Extra 5/6, H28 request; reverses accepted E12 / R6 C14) | R11 owner request (H28 PASS-with-request, Extra 5, Extra 6) | none for Gen I/II: H07 is 'Gen III–V' only; gate C14 now pins Gen II blank Create on host | no reason recorded (ROUND11_ENGINEERING_INTAKE §4 'Moves' decision says 'every generation') |  |
| R2-REROLL / UNOWN-LETTER Gen II Unown letter, Shiny, DVs and Reroll agree; 'Reroll seems not to work' | R6 B07 FAIL, R6 C09 FAIL, R11 H22 FAIL; never PASSed | partial: H13 checks only that no toast appears; H16 checks the species-picker picture | replaced by a broader item |  |
| E11 Rerolls keep the requested Shiny / Gender | R1/R2 NOT VERIFIED, R3 NOT TESTED; never PASSed | none | replaced by a broader item (R2-REROLL) |  |
| AUD-04 Game launch and return (Gen I-III RetroArch exit crash) | R3 NOT TESTED; PRE FAIL | none (H02 checks only the Launch question) | no reason recorded |  |
| S06 Exported edited save loads in an emulator | R1-R6 NOT VERIFIED / NOT TESTED; R11 H46 optional PASS without saying emulator or PC | none (H17 checks Export Copy writes a separate verified file) | deferred by an owner decision (ROUND5_RESULTS: 'S06 deferral is not write permission'; R6 D03 'Defe… | **yes** |
| R2-CHECK-011 Gen V event / Met Location rows distinguishable | R6 B05 FAIL; never PASSed after | partial: H07 marks Trade/Egg/Event separately, not row meaning | replaced by a broader item |  |
| NEW-18 / 7F Gen I-IV Trainer visual standard = Gen V; Gen V Trainer Review button overlap | R6 B06 FAIL, R6 C06 FAIL | none | replaced by a broader item |  |
| E05 Inventory Help wording/glyphs ('+' described as Options, stale 'X Add', Gen V title) | R6 B04 FAIL; R11 H03 PASS covers only fit/legibility | H18 smoke mentions 'Bag Help' only | replaced by a broader item |  |
| P1-RETURN (1) Gen II Unown Create -> Review invisible screen | R6 B08 FAIL | none (H05 checks Party Actions list Review) | replaced by a broader item |  |
| R7 C02 Gen V export appears in Save Backups | owner-reported R6; never tested | none | replaced by a broader item | **yes** |
| R7 C03 editor typography / C04 Quick Games tap-outside / C07 Gen I/II Origin & Legality panel | owner requests R6; never tested | none | replaced by a broader item |  |
| R11 H40-H43, H38, Extra 8 Bank/Vault (incl. in-save box rename) | R11 FAIL / question | none | scope change (owner safety clarification 4: Gen I–V only, no Vault) |  |
| R11 H27 keyboards for Japanese/Korean/all languages | R11 H27 FAIL (request part) | H14 covers name display and Hangul glyphs only | deferred by an owner decision (pending; intake Fix C questions) |  |
| R11 H13 Japanese Gen I/II save read-only | R11 PASS that is not evidence (no Japanese save opened) | optional sentence in H17 only | no reason recorded | **yes** |
| Group A rows named until R10 but not named in R12 (Gen III torn slots, Meteorite/unreleased held items, Gen II/III Bag byte fidelity, Egg names / PID… | Group A synthetic only | collapsed into 'Earlier Group A rows ... See RETEST_ROUND10_TEMPLATE.md' | moved to Group A automated | **yes** |
| Checked and NOT omitted: S05 (H17), C07/R2-EMUBACKUP successor Recovery files (H11), AUD-01 Review (H17), H04/H34 Top Up (H10), H12 (H11), H22 Specia… | various FAIL/request | present | n/a |  |
| Checked: R1 NOT VERIFIED items | B04 PASS R3; E03 PASS OCT24/R5/R6; L06 PASS OCT24, FAIL R5, PASS R6 B01; S04 PASS R3; E11 never; S06 never (R11 H46 optional PASS ambiguous) | E11 and S06 absent | see entries above |  |
| Checked: P1-GEN1 Gen I Review presentation | R5 NOT TESTED, R6 D02 PASS | not listed (accepted; guarded by GUARDS_ROUND7 D02) | n/a (accepted) |  |

### 5b. Owner-accepted items that left the checklist with no permanent guard

12 of 134 accepted items that are no longer on a checklist are not guarded by `tests/test_hw_round1_regression_gate.py` (it has no Round 11 table). See `PERMANENT_REGRESSION_MATRIX.md` §3.

- R11 H09 Gen V Trainer Gender and Pokédex — last PASS R11 (3634d1e6 inferred) H09 PASS
- R11 H10 Gen I Play Time editable (to 255) — last PASS R11 (3634d1e6 inferred) H10 PASS
- R11 H24 Gen III edit and discard — last PASS R11 (3634d1e6 inferred) H24 PASS
- R11 H30 Create, back out, keep — last PASS R11 (3634d1e6 inferred) H30 PASS
- R11 H36 Malformed Gen IV pocket safe refusal (optional) — last PASS R11 (3634d1e6 inferred) H36 PASS
- R11 H38 Cross-generation import refusal — last PASS R11 (3634d1e6 inferred) H38 PASS
- R11 H44 Recovery display: latest prior generation protected — last PASS R11 (3634d1e6 inferred) H44 PASS
- R11 H46 Export Copy reload and compare (S06-like, optional) — last PASS R11 (3634d1e6 inferred) H46 PASS
- R11 H48 Exit and relaunch after discard — last PASS R11 (3634d1e6 inferred) H48 PASS
- R11 H05 Export same Pokémon twice -> distinct ' (2)' file — last PASS R11 (3634d1e6 inferred) H05 PASS
- R11 H06 Import exported Pokémon into empty slot — last PASS R11 (3634d1e6 inferred) H06 PASS
- R11 H13 Japanese Gen I/II read-only — last PASS R11 (3634d1e6 inferred) H13 PASS

## 6. Evidence record (every unique issue, every occurrence)

Owner words are verbatim. Occurrences list every historical record (round · candidate · test ID · result · source); none were deleted.

### `PBX-ACT-001` Pokemon Actions sheet basics: A opens a deliberate menu, B/Cancel close with zero mutation, unfinished actions inert, stable under repetition, same semantics Party/Boxes/Storage

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** actions-menu · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R6 26186464 A04 (E02) / A21 (NEW-20): PASS
- **Basis:** Owner PASS on the original generic sheet (PRE 3be4de6b, A0080 '~100 open/close no lag', A0175) and on the Gen I-IV narrow menus (PR #278, d0a45909, A0939/D0340 'other than that its a pass'); Gen I-III Party popup and Boxes/Party order PASSed again R6 26186464 A04/A21. In current source the generic sheet (include/UI/ActionSheetModel.h) is never shown in a Gen I-V session: TrainerViewScreenBase.inc:1977 returns early only when genericUiAllowedForSession() is true, which is false for every classic source (include/UI/ClassicGenericUiGuard.h:34 genericUiAllowed = !classic); Gen I-V sheets are presentations of real staged commands only (include/UI/NarrowPokemonActionMenu.h:12-19, 84-92; onlySharedLabels :193-199). Sheet wiring changed after the last PASS (dd69bc93, 6f62b69e, f3959d67), so it needs R12 re-verification. C06/C08 (disabled rows truly inert, separate destructive confirmations) were never ticked on hardware as their own items.
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads; 85762adc fix(gen1): size actions to visible rows; 1cdbe73c feat(shared-editor): one action order for Gen III-V, no Make Shiny; "Origin & Legality" everywhere; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_action_sheet_model.cpp : generic 9-row sheet order, NotYetSupported dispatch, close/navigate (generic Gen VI+ sheet only); tests/test_classic_pokemon_action_menu.cpp : Gen I/II sheet rows are real commands in shared order; tests/test_hw_round1_regression_gate.py : frozen per-ID guards for owner-PASSed menu items
- **Hardware acceptance:** accepted PRE 3be4de6b / PR #278 d0a45909 / R6 26186464 A04; must re-verify on 066e1db9 R12 H05 · R12 item H05
- **Next action:** Run R12 H05 in every generation; also confirm B/Cancel leave Review empty (zero mutation) since H05 does not state it explicitly.
- **Owner** (A0080): > PASS / no lag / no stuck input
- **Owner** (D0250): > A ACTION SHEET: PASS
- **Occurrences (45):** PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b A ACTION SHEET PASS [issue #8 (comment 2026-09-02)]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE C05 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE C06 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE C08 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE G06 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE actions PASS [docs/qa/GEN1_5_MATRIX.md]; PRE actions NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; REQ Milestone B n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ C05-C08 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Action sheet behavior rules / acceptance n/a [docs/UI_FLOW.md]; REQ Adapter capability model n/a [docs/SAVE_SAFETY.md]

### `PBX-ACT-002` Move to Vault / Add to Master Vault / Add to Bank / Transfer to Game absent from every Gen I-V Actions menu

- **Status:** Missing Feature · **Gen:** I, II, III, IV, V · **Area:** actions-menu · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H43: FAIL ('gen 1 does not have "move to vault or bank or what ever"'); Extra 7 request
- **Basis:** No Gen I-V Actions sheet can reach AddToMasterVault/AddToBank/TransferToGame. The enum and labels exist only in include/UI/ActionSheetModel.h:24-34,48-56 (dispatch always NotYetSupported :83-89) and are used only by the generic Gen VI+ sheet (StoragePanel.cpp:464-483), which TrainerViewScreenBase.inc:1972-1990 refuses for Gen I-V sessions. Gen IV keeps dead enum values (src/UI/Gen4SharedPokemonSurface.inc:83, status strings :1036-1044) but gen4Actions() (:953-986) never appends them. The shared label table has no Vault row (NarrowPokemonActionMenu.h:97-114, allowed list :196-198); the greyed placeholders were removed in 1cdbe73c (comment :88-89). tests/test_action_sheet_model.cpp:12 only pins the generic enum. Product Home shows Master Vault as 'COMING SOON' (include/UI/AppShellModel.h:41-43). Engineering decision E0099/F0003: hidden until the Vault exists; owner request F0000 (Extra 7) wants 'move to vault' on every menu.
- **Automated tests:** tests/test_action_sheet_model.cpp : pins the generic sheet's AddToMasterVault/AddToBank/TransferToGame rows as NotYetSupported (not Gen I-V); tests/test_classic_pokemon_action_menu.cpp : Gen I/II sheets contain only real commands (no Vault row)
- **Hardware acceptance:** none (no Vault backend; future Vault build)
- **Next action:** Owner to confirm intake Q2 (stay hidden until the Vault exists). Implement only together with PBX-VLT-001: one action row from the shared owner (SharedPokemonEditorContract / NarrowPokemonActionMenu), Copy-to-Vault first.
- **Owner** (F0000): > gen 2/3 doesnt have 'move or review pending changes in the action menu can we make sure all gens have the same options in the actiion menu, they should all have "view, edit,clone,move,move to vault, release, origin and legality, export to file, reviewing pending changes, cancel"
- **Owner** (E0042): > gen 1 does not have "move to vault or bank or what ever"
- **Occurrences (13):** PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; R11 3634d1e6 H43 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H43 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ Pokémon action sheet n/a [docs/UI_FLOW.md]; REQ Action-sheet integration n/a [docs/TRANSFER_MODEL.md]; REQ A11 / F02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Extra 7 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ H43 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ ACTIONS n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ §4 Actions menu n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ VAULT gap 3 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-ACT-003` Make Shiny removed from quick Actions (Shiny stays an editor field)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** actions-menu · **Category:** request
- **Last owner result:** R3 62a2f8c3 E01: PASS
- **Basis:** Owner PASS R1 7fea1797 E01 and again R3 62a2f8c3 E01 (B0504). Current source: no Make Shiny label in the shared vocabulary (include/UI/NarrowPokemonActionMenu.h:97-114, onlySharedLabels :193-199). Actions sheet code changed after R3 (dd69bc93, 6f62b69e).
- **Repair commits:** dcd8770c feat(gen1/gen2): Actions sheet lists real commands only; Y re-rolls DVs at once; 1cdbe73c feat(shared-editor): one action order for Gen III-V, no Make Shiny; "Origin & Legality" everywhere
- **Automated tests:** tests/test_hw_round1_regression_gate.py : E01 guard; QA CG01 (docs/qa/GEN1_5_UI_AUDIT_MATRIX.md) : no Make Shiny quick action, all gens
- **Hardware acceptance:** R3 62a2f8c3 E01; re-verify on 066e1db9 R12 H05 · R12 item H05
- **Next action:** Covered implicitly by H05 row list; no separate action.
- **Occurrences (11):** PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE CG01 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE 3 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 E01 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E01 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 E01 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E01 PASS [tests/test_hw_round1_regression_gate.py]; R2 E01 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f E01 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 E01 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-ACT-004` Pre-campaign cross-generation Action menu parity (Gen II empty-slot menu, Gen IV oversized dialog, picker/menu sizes)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV · **Area:** actions-menu · **Category:** defect
- **Last owner result:** PR #278 d0a45909 (2026-10-12): PASS
- **Basis:** Fixed forward in cfb70357 / aff41765 / 85762adc; owner then PASSed the Gen I-IV narrow action menus on PR #278 (d0a45909, 2026-10-12, A0939/D0340). Menu builders replaced since (5d9dc8e5, 1cdbe73c, dd69bc93).
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions; aff41765 fix(gen4): align create editor with classic parity; 85762adc fix(gen1): size actions to visible rows
- **Automated tests:** tests/test_classic_pokemon_action_menu.cpp : Gen I/II rows/order
- **Hardware acceptance:** d0a45909 PR #278; re-verify 066e1db9 R12 H05 · R12 item H05
- **Next action:** None beyond H05.
- **Owner** (D0298): > - **Gen II Empty Slot menu parity:** current menu lacks `Legality & Provenance` and says `Close` instead of accepted Gen I `Cancel` terminology.
- **Owner** (D0340): > Owner's report: **"other than that its a pass"** for the Gen I–IV action menu/touch behavior, with a remaining **Gen IV move/pickup visual bug**.
- **Occurrences (5):** PRE 501093ae FAIL [PR #68 (comment 2026-09-15)]; PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]; PRE a86c8d03 FAIL [PR #92 (comment 2026-09-29)]; PRE  PASS [PR #278 (comment 2026-10-12)]; PRE d0a45909 PASS [docs/handoffs/GEN14_FINAL_COMPLETION_PROGRESS.md]

### `PBX-ACT-005` Gen V Actions parity: Actions/Add menu, Clone/Move/Release on created/cloned/edited slots, Origin & Legality row, Move via shared carry

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** actions-menu · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R6 26186464 A04 (order, all gens): PASS — not specific to staged slots
- **Basis:** Repro B0596 found eligibility based on originalOccupied(). Current: Gen V bridge fills the shared capability model (dd69bc93; include/Integration/Gen5/Gen5SharedEditorBridge.h:54 uses workspace.emptyCloneDestination) and Gen V Move/Clone run through the shared carry (src/UI/ClassicPackedMoveOverlay.inc:1110-1145 placeGen5, handleGen5CarryInput :1414-1430). R6 A04 PASS covered 'each generation' order but not the created/cloned/edited-slot case specifically, so no owner PASS for Part 9 #1 exists.
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; 06a7dce2 Part 9: test Gen V staged-slot Clone/Move/Release, carry and Origin & Legality; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_gen5_staged_slot_actions.cpp : Clone/Move/Release offered on created/cloned/edited slots; Origin & Legality; tests/test_ui_clone_carry_host.cpp : Gen V Move/Clone through the shared carry
- **Hardware acceptance:** 066e1db9 R12 H05 (+H03/H04 for Gen V) · R12 item H05
- **Next action:** In R12 H05 also open Actions on a Gen V created/cloned slot and confirm Clone/Move/Release/Origin & Legality rows.
- **Occurrences (4):** PRE 35cbf163 P3 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; R3 62a2f8c3 Part 9 #1 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 0faf7e9c Part 9 #1 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 0faf7e9c Part 9 #1 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-ACT-006` One Actions order for every generation (E02) and owner's uniform list: Gen II/III missing Move and Review Pending Changes (R11 Extra 7)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** actions-menu · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 Extra 7: FAIL (after R6 26186464 A04 PASS)
- **Basis:** E02 PASSed R1, FAILed R3 (B0505), PASSed OCT24/R5/R6 (26186464 A04), then the owner FAILed the menu content again in R11 3634d1e6 (Extra 7: Gen II/III lack Move and Review). dd69bc93 adds canMove to the shared ActionCapabilities and lists Move on Gen II/III Boxes (beginGen2FromAction/beginGen3FromAction; 82d92b69 drives the drop), one canonical order in include/UI/NarrowPokemonActionMenu.h:84-146 (sharedActionRank), Gen IV list src/UI/Gen4SharedPokemonSurface.inc:975-986. Residual policy gaps vs the owner's list: Review Pending Changes is shown only while changes are pending (Gen4SharedPokemonSurface.inc:985; intake Q1 open, owner list implies always), Party sheets never list Clone/Move/Release in Gen I-IV (intake Q3 open), Storage pane never lists Move, and Move to Vault is tracked separately (PBX-ACT-002).
- **Repair commits:** 5d9dc8e5 refactor(classic): Gen I/II Actions sheet is the shared NarrowPokemonMenu builder; 1dcf2cc3 WIP: feat(gen1): Box Actions lists Move and starts the Boxes carry (R2-CHECK-009); 1cdbe73c feat(shared-editor): one action order for Gen III-V, no Make Shiny; "Origin & Legality" everywhere; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V; 82d92b69 Round 11 review: drive the Gen II / III Actions -> Move drop
- **Automated tests:** tests/test_classic_pokemon_action_menu.cpp : exact Gen I/II lists and order; tests/test_gen12_packed_move.cpp : Gen I box sheet with/without Move; tests/test_shared_editor_contract (QA matrix 'actions') : exact per-gen lists; tests/test_hw_round1_regression_gate.py : E02 guard
- **Hardware acceptance:** 066e1db9 R12 H05 · R12 item H05
- **Next action:** Run H05; get owner answers to intake Q1 (Review always shown, greyed?) and Q3 (Party Clone/Move/Release) before calling the menu uniform.
- **Owner** (B0505): > Some expected Actions menus missing; related to L06/NEW-20.
- **Owner** (E0058): > gen 2/3 doesnt have 'move or review pending changes in the action menu can we make sure all gens have the same options in the actiion menu, they should all have "view, edit,clone,move,move to vault, release, origin and legality, export to file, reviewing pending changes, cancel"
- **Occurrences (28):** PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE C07 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE 5 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 E02 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E02 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 E02 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E02 PASS [tests/test_hw_round1_regression_gate.py]; R2 E02 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f E02 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 E02 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E02 (re-check) n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E02 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 E02 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E02 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E02 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec E02 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec E02 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A04 · E02 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A04 · E02 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A04 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 Extra 7 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 7 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; R12 066e1db9 H05 n/a [docs/hardware/RETEST_066e1db9.md]; REQ ACTIONS Q1/Q3/Q4 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-ACT-007` Gen I-III Party Actions popup (View / Edit / Origin & Legality / Cancel, now + Review while staged) instead of jumping to View

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III · **Area:** actions-menu · **Category:** defect
- **Last owner result:** R6 26186464 A21: PASS
- **Basis:** R3 FAIL fixed by c81853da/be3d965a; owner PASS OCT24, R5 1ca2faec and R6 26186464 A21. Current routing: TrainerViewScreenBase.inc:1990-2000 (Party + gen<=3 -> ClassicPartyActions::open). dd69bc93 added Review Pending Changes to the Gen I-III Party sheet after the last PASS.
- **Repair commits:** c81853da feat(editor): route classic Party Actions to native staged edits; be3d965a fix(gen3): preserve Party cache on metadata-only edits; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_classic_party_actions.cpp : Party popup rows/dispatch; QA CG33 / test_classic_party_review_path : Party review path
- **Hardware acceptance:** R6 26186464 A21; re-verify 066e1db9 R12 H05 · R12 item H05
- **Next action:** H05 Party step (Review row while staged).
- **Owner** (B0569): > Gen I–III Party A/touch should show View/Edit/Origin & Legality/Cancel instead of jumping to View.
- **Occurrences (9):** R3 62a2f8c3 NEW-20 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-20 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-20 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-20 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-20 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-20 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A21 · NEW-20 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A21 · NEW-20 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A21 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-BAG-001` Gen III Bag/Items read in early browser builds (FRLG checklist unrun; RSE Items screen empty)

- **Status:** Hardware Accepted · **Gen:** III · **Area:** bag-items · **Category:** defect
- **Last owner result:** R3/62a2f8c3/B03: PASS (Gen III Add Item)
- **Basis:** RSE Items FAIL on 46e0c161 (D0275); corrected artifact 1c96df25 (issue #48 Gen III accepted per later_refs). Gen III Items later owner PASS: fc1e6de2 classic inventory (A0504-A0515, gens I-III) and R3 62a2f8c3 B03 PASS (B0460). Current read path: src/UI/ClassicInventoryOverlay.cpp:209-219 (Gen3 inventoryEntries), pouch table src/Integration/Gen3/Gen3StagedInventoryEditor.cpp:34-52.
- **Repair commits:** 1c96df25 tests: expect RSE game-card artwork (corrected RSE artifact per issue #6/#48); fc1e6de2 ui: polish classic staged inventory footer and picker readability
- **Automated tests:** tests/test_gen3_staged_inventory.cpp : Gen III pouches decode/stage; tests/test_gen3_inventory_byte_fidelity.cpp : only edited slot + checksum change
- **Hardware acceptance:** 62a2f8c3 R3 B03 (also fc1e6de2 PRE classic inventory)
- **Next action:** None beyond R12 H18 smoke; close.
- **Owner** (D0275): > Items screen: **FAIL — empty**
- **Occurrences (6):** PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 46e0c161 FAIL [issue #6 (comment 2026-09-10)]

### `PBX-BAG-002` RBY Items: Invalid category / Medicines fallback defect (first RBY runtime)

- **Status:** Hardware Accepted · **Gen:** I · **Area:** bag-items · **Category:** defect
- **Last owner result:** PRE/50dac31f/RBY ItemsFix: PASS
- **Basis:** Fixed in ItemsFix 50dac31f; owner 'RBY Items DEVICE ACCEPTED' (A0895 'YES', A0902/A0903/A0905 PASS). Current Gen I categories: src/Inventory/ClassicInventoryCatalog.cpp:31-34 (kGen1Pockets), entries src/UI/ClassicInventoryOverlay.cpp:189-197.
- **Repair commits:** 50dac31f gen1: add read-only RBY inventory support (ItemsFix)
- **Automated tests:** tests/test_rby_inventory.cpp : RBY category mapping; tests/test_classic_inventory_catalog.cpp : exact-game catalogs
- **Hardware acceptance:** 50dac31f PRE (BUILD_RECORD RBY ItemsFix)
- **Next action:** None; close.
- **Owner** (D0280): > The first RBY runtime `d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7` was physically tested first. Trainer, Party, Boxes, Pokémon details and general browsing passed, but that exact binary exposed the Items/category defect and the incorrect GBA platform label for Yellow.
- **Occurrences (2):** PRE d9077e2d FAIL [docs/BUILD_RECORD.md]; PRE d9077e2d FAIL [PR #52 (body 2026-09-11)]

### `PBX-BAG-003` Add Item picker size OK but item names / control hints too dim

- **Status:** Hardware Accepted · **Gen:** I, II, III · **Area:** bag-items · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-BUG-006: PASS (compact grid, successor picker)
- **Basis:** fc1e6de2 'polish classic staged inventory footer and picker readability'; owner accepted fc1e6de2 'slim centered Add Item picker' (A0511) and 'visible picker controls' (A0512) PASS. Picker later replaced by the shared compact grid (R2-BUG-006, PASS R3).
- **Repair commits:** fc1e6de2 ui: polish classic staged inventory footer and picker readability
- **Automated tests:** tests/test_item_grid_picker.cpp : shared grid geometry
- **Hardware acceptance:** fc1e6de2 PRE A0511/A0512; 62a2f8c3 R3 R2-BUG-006
- **Next action:** None; close.
- **Owner** (D0304): > Latest hardware feedback also confirms the new narrow Add Item picker is a good size, but normal selectable item names and picker control hints are too dim/faded on the physical Switch.
- **Occurrences (1):** PRE  n/a [issue #59 (comment 2026-09-13)]

### `PBX-BAG-004` Item editor screenshot defects: TM/HM art, wrong Ruby TM names, missing Gen IV machine names (+ X Sort request); TM/HM move detail

- **Status:** Hardware Accepted · **Gen:** I, II, III, IV, V · **Area:** bag-items · **Category:** defect
- **Last owner result:** R11/3634d1e6/H35: PASS
- **Basis:** Owner 'nro tested and 100% pass' on b1fcbd3e (D0338), which contains picker X Sort 646c4f50 (ancestor of b1fcbd3e). TM/HM move detail R2-BUG-005 PASS R3 (B0535) and latest R11 3634d1e6 H35 'Held item and TM/HM details' PASS (E0034). include/Inventory/MachineMoves.h and src/Inventory/MachineMoves.cpp unchanged since 3634d1e6. X Sort: include/UI/ItemPickerSort.h:12-24, wired at src/UI/ClassicInventoryOverlay.cpp:909-925 and src/UI/Gen4SharedPokemonSurface.inc:1920-1923 (Gen V Bag Add has none, see PBX-BAG-014).
- **Repair commits:** 646c4f50 feat(ui): shared non-mutating Native/A-Z/Z-A item-picker sort retaining selected native ID; b1fcbd3e Run real Gen IV native bag add/edit bounds, rollback and source-integrity tests across DP Pt HGSS; 7ed1cc16 Merge ... (R2-BUG-005 machine data)
- **Automated tests:** tests/test_machine_moves_gen15.cpp : TM/HM move per game; tests/test_machine_display.cpp : HM labels (Gen V HM01-06); tests/test_item_picker_sort.cpp : Native/A-Z/Z-A presentation-only sort keeps selection
- **Hardware acceptance:** 3634d1e6 R11 H35 (sort: b1fcbd3e PRE)
- **Next action:** None; covered by R12 H18 smoke.
- **Owner** (D0339): > Screenshots: Gen I/II TM/HM legacy placeholder art, Gen III Ruby TM43 erroneously “Volt Switch” and TM49 erroneously “Scald”, TM/HM sprite fallbacks in PC storage, missing Gen IV machine move names and request for X Sort in Add/Held item menus.
- **Occurrences (2):** PRE ba628409 FAIL [PR #208 (comment 2026-10-11)]; R3 62a2f8c3 R2-BUG-005 / Part 9 #6 n/a [docs/hardware/RETEST_62a2f8c3.md]

### `PBX-BAG-005` Bag Add must place the item at x1 immediately (no numpad first), quantity edited afterwards from the row (B01/B02/B03/B06)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, all · **Area:** bag-items · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R3/62a2f8c3/B01,B02,B03,B06: PASS
- **Basis:** Owner PASS R2 e10f307f (B0300-B0303) and again R3 62a2f8c3 B01/B02/B03/B06 (B0458-B0461). Current code: shared decision UI/SharedBagAddFlow.h; Gen I-III stage x1 with no numpad src/UI/ClassicInventoryOverlay.cpp:549-560 (selectPickerItem), quantity only from a row :1100-1118 (decideRow + promptNumber). ClassicInventoryOverlay.cpp changed after 62a2f8c3 (8915b018 Top Up, d9105fae Help, 642a7931/153e5aca Top Up scope) so re-verify. NOTE: R11 H34 'Add and pocket capacity' was marked FAIL (E0033) but the owner's comment is entirely about Top Up/PC Items (tracked in PBX-BLK-002).
- **Repair commits:** 0b70f023 fix(bag): B01/B02/B03/B06 + B04 - one shared Gen I-V Add flow, item lands at x1 with no numpad; 74b55e88 feat(bag): B01-B06/B04 - one shared Add picker on SharedMenuGeometry for Gen I-V; 28707782 feat(inventory): classic quantity rule delegates to QuantityPolicy (Gen I-III); 3dcce1fd feat(classic-bag): Remove asks first; one per-item quantity query
- **Automated tests:** tests/test_bag_add_flow_gen15.cpp : Add stages x1 with no keypad, existing stack focused, invalid ids refused; tests/test_item_quantity_policy.cpp : per-game caps (CG09); tests/test_shared_bag_quantity.cpp : quantity editor bounds; tests/test_hw_round1_regression_gate.py : GUARDS_ROUND2 B01/B02/B03/B06 frozen
- **Hardware acceptance:** 62a2f8c3 R3 B01/B02/B03/B06; recheck on 066e1db9 R12 H18 smoke · R12 item H18
- **Next action:** In R12 H18 smoke, Add a new item in Gen I/II/III and confirm x1 with no numpad, then A edits quantity.
- **Owner** (B0017): > B01:vgen on still has the numpad pop up before its put in my bag, i shou have it in my bag and than i can add more quantity.
- **Owner** (B0064): > in some of the menu like "items" it still makes me pick a quanity beofre it goes into my bag, i should pick it it pops up in bag and from there i can edit the quanity.
- **Occurrences (43):** PRE 2 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE item 1 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE CG09 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE CG12 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE bag-add PASS [docs/qa/GEN1_5_MATRIX.md]; PRE bag-add NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; R1 7fea1797 B01 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B02 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B03 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B06 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B01 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B02 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B03 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B06 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 B01 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B02 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B03 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B06 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B01 FAIL [issue #313 (comment 2026-10-17)]; R1 7fea1797 B02 FAIL [issue #313 (comment 2026-10-17)]; R1 7fea1797 B03 FAIL [issue #313 (comment 2026-10-17)]; R1 7fea1797 B06 FAIL [issue #313 (comment 2026-10-17)]; R2 B01 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 B02 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 B03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 B06 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f B01 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f B02 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f B03 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f B06 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f B01 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 e10f307f B02 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 e10f307f B03 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 e10f307f B06 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R3 62a2f8c3 B01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B06 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; REQ Next app work n/a [CURRENT_STATUS.md]; REQ Repair priorities n/a [PROJECT_STATUS.md]; REQ Release order 1 n/a [docs/V1_ROADMAP.md]

### `PBX-BAG-006` Gen IV / V Add Item follows the shared Add flow (B04); Gen IV physical Items milestone

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** IV, V · **Area:** bag-items · **Category:** defect
- **Last owner result:** R3/62a2f8c3/B04: PASS (Gen IV ordering: R11/3634d1e6/H17 PASS)
- **Basis:** NOT VERIFIED in R1/R2 (B0020, B0306), owner PASS R3 62a2f8c3 B04 (B0464, guard B0662). Gen IV Add ordering later PASS R11 3634d1e6 H17 (E0016). Current: Gen IV X Add src/UI/Gen4SharedPokemonSurface.inc:2106-2131 -> stageGen4BagAdd :1731-1740; Gen V X Add src/UI/Gen5SharedReadOnlySurface.inc:1288-1290 -> include/Integration/Gen5/Gen5BagScreen.h:187 openAdd / :281 pickAdd. Gen5BagScreen.h changed after 62a2f8c3 (88f974da NEW-17, 8915b018 Top Up).
- **Repair commits:** 0b70f023 fix(bag): B01/B02/B03/B06 + B04 - one shared Gen I-V Add flow, item lands at x1 with no numpad; 52017cd1 test(gen5): B04 contract - Gen V Add goes through Gen5BagScreen::pickAdd (no keypad); 3634d1e6 fix(gen4): reconcile native Berry/TM Add sort with Remove packing; refuse damaged pockets
- **Automated tests:** tests/test_bag_add_flow_gen15.cpp : Gen IV stageBagAdd / Gen V pickAdd at x1; tests/test_gen5_bag_screen.cpp : Gen V Add/Remove/quantity screen model; tests/test_gen4_bag_remove_pack.cpp : Gen IV add/remove packing
- **Hardware acceptance:** 62a2f8c3 R3 B04 · R12 item H18
- **Next action:** Gen V Add on 066e1db9 during R12 H18 smoke (Gen5BagScreen changed since acceptance).
- **Occurrences (16):** PRE D6 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE  n/a [docs/GEN4_ITEMS_AND_PICKER_POLISH_2026-10-09.md]; PRE  n/a [docs/GEN4_ITEMS_AND_PICKER_POLISH_2026-10-09.md]; R1 7fea1797 B04 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 B04 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B04 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B04 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B04 NOT TESTED [issue #313 (comment 2026-10-17)]; R2 B04 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f B04 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f B04 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 B04 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R3 62a2f8c3 B04 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 B04 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 B04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B04 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-BAG-007` Bag Remove asks first; compact Remove / Key Item warning dialog with coloured A/B glyphs, A confirms, B cancels, no duplicate footer (B07 / NEW-03)

- **Status:** Hardware Accepted · **Gen:** all · **Area:** bag-items · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R11/3634d1e6/H33: PASS
- **Basis:** NEW-03 FAIL R3 (B0552) fixed 88f974da; owner PASS OCT24/R5/R6 A07 26186464 (C0287) and latest R11 3634d1e6 H33 'Remove confirmation and cancel' PASS (E0032). Current: Y -> confirm src/UI/ClassicInventoryOverlay.cpp:1091-1099, A/B handling :834-850, dialog :1143-1153 (SharedBagAddPicker::drawRemoveConfirm); Gen IV stageBagRemove src/UI/Gen4SharedPokemonSurface.inc:2064; Gen V include/Integration/Gen5/Gen5BagScreen.h:316/329. include/UI/SharedBagAddPicker.h unchanged since 26186464; remove path unchanged since 3634d1e6.
- **Repair commits:** 3dcce1fd feat(classic-bag): Remove asks first; one per-item quantity query; 88f974da Stage native PK5 language and unique inventory with zero creation stats and shared UI repairs (NEW-03 glyph dialogs)
- **Automated tests:** tests/test_classic_inventory_ui_model.cpp : classicRemoveConfirmStep A removes / B cancels; tests/test_gen5_bag_screen.cpp : ConfirmRemove; tests/test_hw_round1_regression_gate.py : B07 / NEW-03 guards (CG38)
- **Hardware acceptance:** 3634d1e6 R11 H33 (NEW-03: 26186464 R6 A07)
- **Next action:** None; close.
- **Owner** (B0063): > in the bag item menu the delete confirmation menu thing needs shirinked down to a proper size.
- **Owner** (B0552): > Colored A/B glyphs inside dedicated Remove and Key Item Warning dialogs. Bag Remove confirmation should use A (not Y) to confirm and B to cancel. Don't repeat modal option buttons in bottom footer. Do not change other destructive-action safeguards.
- **Owner** (B0547): > Warning needs correctly colored controller glyphs.
- **Occurrences (23):** R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B07 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B07 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B07 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B07 PASS [tests/test_hw_round1_regression_gate.py]; R2 B07 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f B07 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-CHECK-010 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-03 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B07 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 B07 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-010 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-010 PASS [tests/test_hw_round1_regression_gate.py]; R4 c60d4da0 NEW-03 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-03 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-03 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-03 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-03 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A07 · NEW-03 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A07 · NEW-03 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A07 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 H33 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-BAG-008` Bag changes appear in Review and in the exported copy; R3 'Export does nothing across generations'

- **Status:** Hardware Accepted · **Gen:** all · **Area:** bag-items · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R11/3634d1e6/H14,H46: PASS (export-copy area); R6/26186464/B03(B08): PASS
- **Basis:** R3 FAIL (B0503) was an app-wide Export Copy problem. Later owner PASS: R6 26186464 B03·B08 'Bag Review is discoverable' (C0314, review area) and R11 3634d1e6 H14 'Export Copy is separate' + H46 'Export Copy reload and compare' PASS (export-copy area). Bag Review entry: src/UI/ClassicInventoryOverlay.cpp:1058-1063 (+ opens shared Review); export src/UI/ClassicInventoryOverlay.cpp:393 exportStagedCopy.
- **Repair commits:** a7244a64 fix(gen1-4): Review opened outside Box Actions returns to its screen; Bag uses the shared Review (round 5 P1)
- **Automated tests:** tests/test_gen1_export_transaction.cpp : verified export of staged Bag; tests/test_qa_export_contracts.py : export contracts; tests/test_gen2_inventory_byte_fidelity.cpp / test_gen3_inventory_byte_fidelity.cpp : only edited slot + checksum
- **Hardware acceptance:** 3634d1e6 R11 H14/H46 · R12 item H17
- **Next action:** R12 H17 (Review/Keep/Discard/Export Copy) includes a Bag edit.
- **Owner** (B0503): > Owner reports Export does nothing across generations; investigate event handling, output path, error/reporting and safe export separately from source writes.
- **Occurrences (8):** R1 7fea1797 B08 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 B08 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 B08 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 B08 PASS [tests/test_hw_round1_regression_gate.py]; R2 B08 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f B08 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 B08 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 B08 n/a [docs/hardware/RETEST_62a2f8c3.md]

### `PBX-BAG-009` Compact item grid: short lists (<~20) should use balanced rows/columns

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** bag-items · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-BUG-006: PASS (with polish request)
- **Basis:** Grid itself PASS R3 (B0536). The balanced-short-list request is implemented in e3c81745: include/UI/SharedItemGridPicker.h:105-125 (kBalancedBelow=20, balancedGrid). No later owner result names balanced short grids.
- **Repair commits:** e3c81745 R2-BUG-006: short item grids use balanced rows and a narrower frame; 6e015b5b WIP: feat(ui): R2-BUG-006 shared compact grid item picker for Bag Add (Gen I-V)
- **Automated tests:** tests/test_item_grid_picker.cpp : grid geometry incl. balanced short lists
- **Hardware acceptance:** none (not on any checklist since R3)
- **Next action:** Add a one-line check (open Bag Add on a pocket with <20 choices, e.g. Gen I Balls) to R12/R13.
- **Owner** (B0537): > Short lists (fewer than ~20 entries) should use better balanced rows/columns instead of awkward long sparse rows.
- **Occurrences (4):** R3 62a2f8c3 R2-BUG-006 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-006 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-006 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-006 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-BAG-010` Gen V Key Items / TMs & HMs pockets were view-only; need native-safe staged Add/Remove (NEW-17)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** bag-items · **Category:** missing-feature · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R6/26186464/A19: PASS
- **Basis:** Implemented 88f974da; owner PASS OCT24 c60d4da0, R5 1ca2faec and R6 26186464 A19 (C0299). Current: mutablePocket covers every pocket include/Integration/Gen5/Gen5BagScreen.h:57-59; Key Item story warning :100; footer X/Y/ZR :80-95. Gen5BagScreen.h changed after 26186464 (8915b018 Top Up); R11 H37 PASS covered read/navigation only.
- **Repair commits:** 88f974da Stage native PK5 language and unique inventory with zero creation stats and shared UI repairs
- **Automated tests:** tests/test_gen5_bag_screen.cpp : Key Item/TM add/remove, unique cap 1, warning; tests/test_gen5_bag.cpp : native SAV5 pockets; tests/fuzz_gen5_bag.cpp : malformed pocket safety
- **Hardware acceptance:** 26186464 R6 A19; recheck on 066e1db9 R12 H18 · R12 item H18
- **Next action:** R12 H18 smoke: Gen V Key Items Add -> warning -> Remove.
- **Owner** (B0566): > Add supported native-safe Add/Remove/Edit in staged workspace and remove blanket view-only footer once implemented.
- **Occurrences (11):** PRE E07 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 62a2f8c3 NEW-17 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-17 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-17 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-17 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-17 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-17 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A19 · NEW-17 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A19 · NEW-17 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A19 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-BAG-011` [MANDATORY 7] Complete Bag operations: every native pocket, quantity bounds, Add/Remove, no lost edits (E04-E09)

- **Status:** Needs Investigation (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** bag-items · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none for E04/E08; see matrix for other ops
- **Basis:** Per-op state in item_ops_matrix. Accepted on hardware: add/remove/quantity Gen I-V (PBX-BAG-005/006/007), Gen V pockets (PBX-BAG-010, R11 H37). UNKNOWN: E04 (Product Home Items vs Trainer Y Bag open the same source and return) and E08 (Gen III/IV Bag + Pokemon edits in one session lose nothing) have no owner result in any round; Top Up part is not accepted (R11 H04/H34 FAIL, see PBX-BLK-001/002). Host-only PASS rows (A0810, A0855) are not hardware acceptance.
- **Automated tests:** tests/test_qa_matrix (bag / bag-add cells) : host PASS all 20 games; tests/test_inventory_ui_contract.cpp : shared A/X/Y availability
- **Hardware acceptance:** 066e1db9 R12 H10 (Top Up) + H17/H18; E04/E08 not on any checklist · R12 item H10
- **Next action:** Add E04 and E08 steps to the next hardware checklist; close after R12 H10 PASS.
- **Occurrences (8):** PRE E04 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E05 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E06 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E08 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE bag PASS [docs/qa/GEN1_5_MATRIX.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; REQ E04-E09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-BAG-012` Ordinary Bag edit: changing a quantity back to the original leaves no pending Review change (R8 H05)

- **Status:** Needs Investigation · **Gen:** all · **Area:** bag-items · **Category:** defect
- **Last owner result:** none
- **Basis:** Checklist only; R8 has no owner results and the item was not carried into R9/R10/R11/R12 checklists. Pending state comes from the staged editors' hasPendingChanges (src/UI/ClassicInventoryOverlay.cpp:265-279); not verified whether net-zero edits clear it for every generation on hardware.
- **Automated tests:** tests/test_gen3_staged_inventory.cpp / tests/test_gen5_bag_screen.cpp : staged restore cases (host)
- **Hardware acceptance:** none (not on any checklist after R8)
- **Next action:** Re-add H05 steps to the next owner checklist (it silently dropped after R8).
- **Occurrences (1):** R8 6ec5ccb9 H05 n/a [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]

### `PBX-BAG-013` Gen V Bag Add picker has no X Sort (Gen I-IV Add pickers have Native / A-Z / Z-A)

- **Status:** Missing Feature · **Gen:** V · **Area:** bag-items · **Category:** missing-feature
- **Last owner result:** none
- **Basis:** Gen V AddPicker input handles grid navigation, A and B only: src/UI/Gen5SharedReadOnlySurface.inc:1165-1200; footer include/Integration/Gen5/Gen5BagScreen.h:81-82 lists no Sort; no ItemPickerSort use in Gen5SharedReadOnlySurface.inc. Gen I-III: src/UI/ClassicInventoryOverlay.cpp:909-925; Gen IV: src/UI/Gen4SharedPokemonSurface.inc:1920-1923. No app supports reordering the Bag itself (include/UI/ItemPickerSort.h:12-13 'Never reorder a game inventory pouch').
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner decision: add X Sort to the Gen V Add picker for parity, or record that Bag 'sort' means picker sort only.
- **Occurrences (0):** 

### `PBX-BKP-001` Durability / interruption gate accepted on FAT32 only; exFAT physical acceptance never tested

- **Status:** Needs Investigation · **Gen:** app · **Area:** backups-recovery · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** PRE cf1e390f issue #69: PASS (FAT32 only); exFAT NOT TESTED
- **Basis:** Owner accepted FAT32 durability on cf1e390f (issue #69, 8/8 RESULT files) but explicitly recorded 'exFAT PHYSICAL ACCEPTANCE: PENDING / NOT TESTED'. No later record tests exFAT. DurableFile (src/Utils/DurableFile.cpp:237-372 rename/previous/failed protocol) changed 2 commits since cf1e390f. Unknown: whether the owner's SD card is exFAT and whether exFAT rename/fsync semantics hold on hardware.
- **Automated tests:** tests/test_backup_workspace_durability.cpp : DurableFile previous/failed/incomplete generations on host; tests/test_bank_recovery_contract.cpp : Bank recovery from .previous/.failed leftovers
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Ask owner which filesystem the SD card uses; if exFAT, add a non-destructive durability smoke item (no power-loss simulation) to the next checklist.
- **Owner** (D0305): > - tested filesystem: FAT32; - FAT32 PHYSICAL ACCEPTANCE: DEVICE ACCEPTED; - exFAT PHYSICAL ACCEPTANCE: PENDING / NOT TESTED.
- **Occurrences (1):** PRE cf1e390f PASS [issue #69 (body 2026-09-14)]

### `PBX-BKP-002` Backup deletion requires explicit Y confirmation; originals / last copy undeletable (C07)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** backups-recovery · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R6/26186464/A02·C07: PASS
- **Basis:** Latest owner result R6 26186464 A02·C07 PASS (C0282). Current source: Save Backups delete dialog only deletes on Y via HomeQuit::decide (src/UI/BackupSelectionScreen.cpp:164-197, include/UI/HomeQuitConfirm.h:26-32); emulator export-record delete EmulatorBackupDeleteConfirm (src/UI/SaveSelectScreen.cpp:3452-3468) with SourceBackups::deleteRecord refusals OriginalSource / StagedWorkingCopy / OnlyRemainingCopy (include/UI/SourceBackupClassification.h:356-404). Code changed after 26186464: 55635234 (Gen V exports listed/opened/deleted in Save Backups) touched SourceBackupClassification.h, so Gen V deletion has never been exercised on hardware.
- **Repair commits:** 9cc6caa9 feat(home): emulator-save Backups list; only app-owned export records are deletable (R2-EMUBACKUP); fe5c3ad7 (R4) eligible app-owned backup deletion with explicit Y confirmation, last-copy protection; 55635234 feat(backups): Gen V exports listed, opened and deleted in Save Backups (7E G1)
- **Automated tests:** tests/test_emulator_backup_access.cpp : original emulator file never deletable and byte-identical; only app export records deletable; tests/test_gen5_backup_listing_host.cpp : delete removes exactly one record of this profile; refuses other profile, foreign files, last copy; tests/test_hw_round1_regression_gate.py : C07 per-ID guards (round 1 / GUARDS_ROUND5)
- **Hardware acceptance:** 26186464 R6 A02 (accepted); recheck on 066e1db9 not itemised in R12 (closest: H17/H18 smoke)
- **Next action:** Add a C07 regression line (incl. a Gen V export record) to the R12/R13 checklist: B cancels, Y deletes, original and last copy refused.
- **Owner** (B0490): > Current backup screen has no expected action; restore older PKSE-style browse/choose/save backup screen.
- **Owner** (D0206): > **C07/S05 remain unverified** unless owner explicitly tests those behaviors.
- **Owner** (D0203): > 4. Copy creation works; retain current explicit-create / original read-only / Y-delete confirmation with last-copy guard.
- **Occurrences (14):** R1 7fea1797 C07 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C07 PASS [tests/test_hw_round1_regression_gate.py]; R3 62a2f8c3 C07 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 C07 NOT TESTED [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 C07 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 C07 NOT TESTED [issue #322 (body 2026-10-24)]; R4 c60d4da0 PASS [issue #322 (body 2026-10-24)]; OCT24 c60d4da0 C07 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 C07 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec C07 checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec C07 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A02 · C07 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A02 · C07 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A02 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-BKP-003` Original source saves never change (S05 source-save immutability)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** backups-recovery · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H14: PASS (R6/26186464/A25: PASS)
- **Basis:** Latest owner results: R6 26186464 A25·S05 PASS (C0305) and R11 3634d1e6 H14 PASS ('source save is byte-unchanged; no live overwrite', E0013) plus H46 PASS. History: R3 FAIL (could not verify because backup flow broken, not corruption), OCT24 FAIL, R5 PASS after 1eec6f18 (Switch backups opened through a copy). Current source: exports only write inside the claimed export directory (src/Integration/Common/VerifiedStagedExport.cpp:311-340; Gen2ExportTransaction.cpp:232-266); Gen I-III export re-checks the source snapshot (include/UI/ClassicSourceGuard.h:45). Code paths producing staged bytes changed in Round 11 (Gen I/II carry 53330345, species history ab072252), so R12 recheck needed.
- **Repair commits:** 1eec6f18 (R5) Switch backups are opened through a staged copy; 4fa35a60 feat(review): ... Gen I-III source guard
- **Automated tests:** tests/test_qa_export_contracts.cpp : contracts C1-C6, source bytes unchanged for every export path; tests/test_qa_matrix (immutability cell) : source bytes, size and ns mtime unchanged after discovery/link/edit/export in all 20 games; tests/test_hw_round1_regression_gate.py : S05 guards (round 1, GUARDS_ROUND6)
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** Owner runs R12 H17 and records original save size/date (and SHA-256 if possible) before and after.
- **Owner** (B0529): > Owner cannot verify backup protection because backup flow isn't working; not evidence of actual original-save corruption.
- **Owner** (D0207): > **C07/S05 remain unverified** unless owner explicitly tests those behaviors.
- **Occurrences (19):** R1 7fea1797 S05 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 S05 PASS [tests/test_hw_round1_regression_gate.py]; R3 62a2f8c3 S05 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 S05 NOT TESTED [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 S05 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 S05 NOT TESTED [issue #322 (body 2026-10-24)]; OCT24 c60d4da0 S05 FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec S05 checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec S05 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec S05 PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A25 · S05 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A25 · S05 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A24 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; REQ Permanent source safety / safety policy requirement [docs/GAME_SUPPORT_MATRIX.md]; REQ Permanent guardrails / transaction model requirement [README.md]; REQ Safety invariants requirement [docs/POKEMON_EDITOR_UI_CONTRACT.md]; REQ F10 / F12 / F13 requirement [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Governance requirement [CURRENT_STATUS.md]; REQ Safety/governance requirement [PROJECT_STATUS.md]

### `PBX-BKP-004` Emulator / Switch Save Backups browser: full-screen list, newest first, explicit create, A opens exact copy as staged session, legacy backups readable (R2-EMUBACKUP)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** backups-recovery · **Category:** request · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R6/26186464/A28: PASS
- **Basis:** Latest owner result R6 26186464 A28·R2-EMUBACKUP PASS (C0308), also R5 PASS (C0203). Earlier: R3 FAIL ('weird and nonfunctional'), R4 FAIL (hybrid modal; A only Details; legacy PKSE backups refused). Current source: createEmulatorBackup is the only place a backup is made, never on open (src/UI/SaveSelectScreen.cpp:3283-3300; include/UI/SourceBackupClassification.h:16,479); exact copy opened via openSnapshot (include/UI/SourceBackupOpen.h:38). Backup code changed after 26186464 by 55635234 (Gen V exports in the list) - never owner-tested.
- **Repair commits:** 9cc6caa9 feat(home): emulator-save Backups list (R2-EMUBACKUP); fe5c3ad7 (R4) explicit source/backups browser, deliberate creation, newest first; 80377153 feat(backups): open exact immutable historical copies in shared Gen I-V sessions; 1eec6f18 (R5) Switch backups open as isolated staged copy; 55635234 feat(backups): Gen V exports listed, opened and deleted in Save Backups (7E G1)
- **Automated tests:** tests/test_emulator_backup_access.cpp : RetroArch/DraStic/melonDS export records listed, originals read-only; tests/test_backup_snapshot_open.cpp : A opens the exact immutable historical copy; tests/test_emulator_backup_layout.cpp : full-screen layout, newest first; tests/test_round4_backup_contract_mutations.py : mutation tests of the R4 backup contract
- **Hardware acceptance:** 26186464 R6 A28 (accepted); not itemised in R12
- **Next action:** Include a Backups list/open regression (emulator + Switch + Gen V export row) in the next checklist; confirm no backup is created on plain open.
- **Owner** (B0060): > "[ ] Backup deletion requires Y confirmation" this only works for the native switch games when i try to goin to my emulator backups its says they are read only source and i can open that menu (when i press + on a game card).
- **Owner** (B0477): > Existing screen is weird and nonfunctional. Restore earlier PKSE-style save/backup browser: select a game, inspect available saves, browse current save and deliberately create backup. **Opening an unchanged save must not automatically create an unnecessary backup.**
- **Owner** (D0200): > 1. **Emulator games (Gen I–V):** Current Game > Backups uses a floating hybrid, four-row list (`Create backup now`, `Original RetroArch save`, `Backup • timestamp`, `Staged edits`) layered on Product Home, not the full-screen PKSE Save Backups browser. A on manual/app/export backup says **Details** and never opens the backup. The user wants A to open a selected **validated backup as a separate staged/read-only-source session**.
- **Occurrences (24):** R1 7fea1797 C07 owner-correction [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 owner-request [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 R2-EMUBACKUP (Phase 7) checklist-item [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f R2-EMUBACKUP (Phase 7) checklist-item [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-EMUBACKUP FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 engineering-finding [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 R2-EMUBACKUP FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 R2-EMUBACKUP checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 R2-EMUBACKUP FAIL [issue #322 (body 2026-10-24)]; R4 c60d4da0 R2-EMUBACKUP FAIL [issue #322 (body 2026-10-24)]; R4 c60d4da0 R2-EMUBACKUP FAIL [issue #322 (body 2026-10-24)]; R4 c60d4da0 owner-request [issue #322 (body 2026-10-24)]; R4 c60d4da0 owner-request [issue #322 (body 2026-10-24)]; R4 c60d4da0 requirement [issue #322 (body 2026-10-24)]; OCT24 c60d4da0 R2-EMUBACKUP NOT TESTED [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 2 checklist-item [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 deferred [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec R2-EMUBACKUP (unmarked) checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec R2-EMUBACKUP PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec R2-EMUBACKUP PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A28 · R2-EMUBACKUP checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A28 · R2-EMUBACKUP PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A27 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; REQ Owner request 6 requirement [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]

### `PBX-BKP-005` No live emulator/installed writes, Inject Save or restore-backup-to-source

- **Status:** Intentionally Unsupported · **Gen:** all · **Area:** backups-recovery · **Category:** decision · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** Recorded policy: R1 doc 'live emulator or installed-game writes; Inject Save stays disabled' (B0165); Round 6 save-safety audit G3 'Intentional policy: no live writes/Inject Save' (docs/audits/ROUND6_SAVE_SAFETY_PKSE.md:40); owner deferral of S06 'is not permission for direct writing or injection' (C0221); CURRENT_STATUS.md:11 'No source-save writeback ... before owner sign-off'. Current source: no restore path; include/Integration/Gen5/Gen5StagedSaveCandidate.h:20 'does not authorize Inject Save or emulator writes'.
- **Automated tests:** tests/test_qa_export_contracts.cpp : no export path opens a source path for writing
- **Hardware acceptance:** none (policy; not a hardware item)
- **Next action:** Keep; re-evaluate only with PBX-BKP-009 (approved per-adapter write pipeline) after owner sign-off.
- **Occurrences (4):** R1 7fea1797 deferred [docs/hardware/RETEST_7fea1797.md]; R6 G3 requirement [docs/audits/ROUND6_SAVE_SAFETY_PKSE.md]; REQ Non-negotiable rule requirement [docs/SAVE_SAFETY.md]; REQ User-facing confirmations requirement [docs/SAVE_SAFETY.md]

### `PBX-BKP-006` 7E: audit save safety and backups against PKSE principles

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** backups-recovery · **Category:** request · ⚠️ safety-critical
- **Last owner result:** R6/26186464/C05·7E: PASS
- **Basis:** Owner request R5 7E (C0217) -> audit docs/audits/ROUND6_SAVE_SAFETY_PKSE.md (G1-G5) -> R6 26186464 C05·7E PASS (C0324). G1 and G5 were later changed (55635234, 4fa35a60), so recheck.
- **Repair commits:** 55635234 (G1); 4fa35a60 (G5 Gen I-III source guard)
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND7 C05 entry
- **Hardware acceptance:** 26186464 R6 C05
- **Next action:** None beyond PBX-BKP-007/008 follow-ups.
- **Occurrences (5):** R5 1ca2faec 7E owner-request [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 C05 · 7E checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 C05 · 7E PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 C05 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A37 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-BKP-007` Gen V exports were not listed in Save Backups (7E G1)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** backups-recovery · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** Owner reported in R6 (C0348). Fixed in 55635234 (R7): SourceBackups lists/opens/deletes Gen V exports for the current profile (include/UI/SourceBackupClassification.h; exports/gen5/account-<profile>/...). R7 C02 checklist item exists but R7/R8/R9 have no owner results and R10/R11/R12 checklists do not include it. No owner PASS.
- **Repair commits:** 55635234 feat(backups): Gen V exports listed, opened and deleted in Save Backups (7E G1); ab46883e Merge lane L (... Gen V exports in Save Backups)
- **Automated tests:** tests/test_gen5_backup_listing_host.cpp : valid Gen V exports listed newest first, foreign-profile/malformed never listed, A opens exact export with edit present, originals byte-identical
- **Hardware acceptance:** none (not on any checklist since R7 C02, which has no results)
- **Next action:** Add 'Gen V export appears in Save Backups and opens as staged copy' to the R12/R13 checklist.
- **Owner** (C0348): > **New bug:** after exporting an edited Gen V save, the exported copy does not appear in Save Backups, unlike the other generations.
- **Occurrences (5):** R6 G1 engineering-finding [docs/audits/ROUND6_SAVE_SAFETY_PKSE.md]; R6 26186464 C12 · Gen V Pokérus (G1) owner-request [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 7E G1 deferred [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R6 26186464 C12 follow-up n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 C02 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-BKP-008` Recovery files view: selection invisible (H12 FAIL); nothing preselected; protections and both confirmations

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** backups-recovery · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H12: FAIL (H44: PASS for protected display)
- **Basis:** G2 (leftover .previous/.failed/.incomplete invisible) led to the Recovery files view 742df4dd. R11 3634d1e6 H12 FAIL: selection/footer invisible (E0011). Round 11 repair d9b10029 + 98b6deaf (merged 72fa9fa8): nothing starts selected (RecoveryManagementModel.h:254 emptySelection; SaveSelectScreen.cpp:3258), focus on opening game's group (RecoveryManagementModel.h:415 initialFocus), picked rows marked (row.picked, badge 'Selected' :365), footer A/Y(n)/-/B (:380-387). Two-step removal kept: Y on list builds plan and opens RecoveryConfirm (SaveSelectScreen.cpp:4214-4224); only Y there removes, A/B keep (:3472-3500, HomeQuit::decide); every item re-checked before removal (RecoveryManagementFs.h:376 removePlanned). H44 PASS (protected labels) on 3634d1e6 is the only owner pass; actual deletion and the cancel path were never PASSed.
- **Repair commits:** 742df4dd Recovery files view: review and remove older copies and leftovers, with confirmation; d9b10029 R11 H12: shared marked row style and structured Recovery files rows; 98b6deaf R11 H12: Recovery files shows selection, footer, Help and focus; 86218c18 Round 11 intake: owner safety clarifications (... nothing preselected in Recovery files ...)
- **Automated tests:** tests/test_recovery_management_model.cpp : classify/protect newest copy, Bank, unverified; A on protected selects nothing; randomized cases; tests/test_recovery_management_fs.cpp : enumerate -> select -> confirm plan -> remove on a mkdtemp tree; tests/test_ui_recovery_files_host.cpp : real screen: nothing selected on open, marked rows, footer hints, Help return
- **Hardware acceptance:** 066e1db9 R12 H11 · R12 item H11
- **Next action:** Owner runs R12 H11 including one real removal of a Removable copy through both confirmations, and a B-cancel at the second confirmation.
- **Owner** (E0011): > this doesnt work i cant see what is selected and nothing in the footer and nothing on screen changes for it, this needs worked on
- **Owner** (E0104): > **Recovery files.** Focus may start on the current game's backups, but **nothing starts selected for deletion**. All protection (newest copy, Bank, unverified) and both confirmation steps stay exactly as they are.
- **Owner** (E0121): > Recovery model tests passed 3,000 randomized cases. Prioritize the actual H12 rendering/footer defect rather than rewriting a model that currently passes.
- **Occurrences (13):** R6 G2 deferred [docs/audits/ROUND6_SAVE_SAFETY_PKSE.md]; R6 26186464 7E G2–G4 deferred [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R9 d6a20a2a H12 checklist-item [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H12 checklist-item [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H12 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H12 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 owner-decision [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; R11 3634d1e6 Owner safety clarification 3 owner-decision [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 owner safety clarifica…]; R11 3634d1e6 engineering-finding [owner message 2026-11-01 (independent Round 11 QA)]; R11 3634d1e6 H44 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R12 066e1db9 H11 checklist-item [docs/hardware/RETEST_066e1db9.md]; REQ Owner safety clarification 3 requirement [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ H12 requirement [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-BKP-009` Approved per-adapter source writeback pipeline (backup -> stage -> reparse -> explicit write -> readback -> rollback)

- **Status:** Missing Feature · **Gen:** all · **Area:** backups-recovery · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** docs/V1_ROADMAP.md:41 v1 target 'only perform source writeback through individually approved transaction adapters'; Phase 12 (docs/V1_ROADMAP.md:211-222) all unchecked. No write-to-source code exists in current source (only app-owned exports; PBX-BKP-005). Gated by owner decision (CURRENT_STATUS.md:11) - deferred, not rejected.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner decision needed: is source writeback in v1.0 scope (roadmap Phase 12) or explicitly post-1.0? Record the decision.
- **Occurrences (4):** REQ Future write pipeline requirement [docs/SAVE_SAFETY.md]; REQ Backup requirements requirement [docs/SAVE_SAFETY.md]; REQ Per-game enablement / Gate to remove the hard lock requirement [docs/SAVE_SAFETY.md]; REQ v1 target / Phase 12 / Phase 13 requirement [docs/V1_ROADMAP.md]

### `PBX-BKP-010` Save Health Center / Disaster recovery / Save Time Machine

- **Status:** Missing Feature · **Gen:** all · **Area:** backups-recovery · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** docs/FEATURE_FEASIBILITY_MATRIX.md #18 Save Health Center v1.0/v1.x, #58 Disaster recovery v1.0/v1.x, #14 Time Machine v1.x. Current app has Save Backups + Recovery files only; no health-center or rescue-flow UI found in src/UI. v1 flag: target is 'v1.0/v1.x' (ambiguous).
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner to confirm whether any of #18/#58 is v1.0 scope.
- **Occurrences (1):** REQ #14 / #18 / #58 requirement [docs/FEATURE_FEASIBILITY_MATRIX.md]

### `PBX-BLK-001` Top Up / Add Missing only fills the current pocket; Bag categories not filled; 'add all' option wanted (H04)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** bulk-topup · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H04: FAIL
- **Basis:** At 3634d1e6 Scope was the 2nd row, defaulting to 'This pocket' (git show 3634d1e6:include/UI/BagTopUpModel.h:28,103). Now Scope is the first row and defaults to All Bag pockets on a Bag tab: include/UI/BagTopUpModel.h:35-36,75-85,88-101,200-215; wired Gen I-III src/UI/ClassicInventoryOverlay.cpp:670-689,700-703; Gen IV src/UI/Gen4SharedPokemonSurface.inc:1786-1800; Gen V src/UI/Gen5SharedReadOnlySurface.inc:1087,1281-1287. Key Items/HMs stay excluded (include/Inventory/BulkItemPlan.h:131,193).
- **Repair commits:** 642a7931 Top Up / Add Missing: discoverable Scope, PC Items and Bag + PC scopes (Gen I-V); f565cc63 Bulk planner: explain full native lists, unique across Bag + PC, Gen I Max Elixer; 8915b018 wip(bag): Top Up / Add Missing on the Gen I-V Bag (checkpoint before full gates)
- **Automated tests:** tests/test_ui_bag_top_up_host.cpp : SETUP opens on Scope at '< All Bag pockets >' and stages via preview; tests/test_bag_top_up_adapters.cpp : model() scope order/cycle/default, All Bag plans per gen; tests/test_bulk_item_plan.cpp : bagPockets/pcPockets, Key/HM skipped
- **Hardware acceptance:** 066e1db9 R12 H10 · R12 item H10
- **Next action:** Owner runs R12 H10 on Gen I-V (Scope row, All Bag pockets, preview, A stages, Review).
- **Owner** (E0003): > top up and add only fill the current menu, the bag categoties arnt filled which they should be it and option of add all i think
- **Occurrences (7):** R9 d6a20a2a H04 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H04 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H04 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H04 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R12 066e1db9 H10 n/a [docs/hardware/RETEST_066e1db9.md]; REQ H04 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ H04-H34 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-BLK-002` Top Up does not fill PC Items; must work for every pocket and PC Items in all gens (Key Items excluded) (H34)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III · **Area:** bulk-topup · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H34: FAIL
- **Basis:** At 3634d1e6 planTopUp passed BI::bagPockets(pockets) so PC Items was never planned (diff 3634d1e6..066e1db9 src/UI/ClassicInventoryOverlay.cpp). Now PC Items / Bag + PC Items scopes exist where the save has PC storage: include/UI/BagTopUpModel.h:73,81-85,207-211; pcAvailable from src/UI/ClassicInventoryOverlay.cpp:687-688; adapters include PCItems include/Inventory/BulkItemAdaptersClassic.h:63,143,224; Key Items excluded include/Inventory/BulkItemPlan.h:131. Gen IV/V have no PC storage (BagTopUpModel.h:15; Gen4SharedPokemonSurface.inc:1797-1799).
- **Repair commits:** 642a7931 Top Up / Add Missing: discoverable Scope, PC Items and Bag + PC scopes (Gen I-V); f565cc63 Bulk planner: explain full native lists, unique across Bag + PC, Gen I Max Elixer
- **Automated tests:** tests/test_bag_top_up_adapters.cpp : PC Items scope fills Gen I 50-slot PC, Crystal PC, Gen III PC; Bag+PC; Gen IV/V !hasPcItems; tests/test_ui_bag_top_up_host.cpp : other scopes on the same screen
- **Hardware acceptance:** 066e1db9 R12 H10 · R12 item H10
- **Next action:** R12 H10: Gen I-III PC Items and Bag + PC Items scopes, preview, A, Review, Export reopen.
- **Owner** (E0033): > the pc items doesnt get filled with top up feature and also gen 1 doesnt seems to work, check out all gesn for that make sure they all work for every pocket and bag categotry and pc items, key items dont worrry about tho cause that could mess up users saves
- **Occurrences (3):** R11 3634d1e6 H34 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H34 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ H34 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-BLK-003` Gen I fill-all only fills the current category (not TMs / items); misleading on the shared 20-slot Bag

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I · **Area:** bulk-topup · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/Extra 1: FAIL
- **Basis:** Gen I Bag is one 20-slot list; scope now 'Whole Bag (20 slots shared)' (include/UI/BagTopUpModel.h:61-64,134-140; src/UI/ClassicInventoryOverlay.cpp:686), preview names full lists (:224-229) and partial results say 'Partly staged ... did not fit ... Nothing overflowed' (:303-318, used at src/UI/ClassicInventoryOverlay.cpp:758). Native limit remains: Add Missing cannot reach every category once 20 slots are used (fill order unresolved, PBX-BLK-004).
- **Repair commits:** 153e5aca R11 QA: Gen I/II clone wording is append-only; Gen I shared-Bag Top Up reports partial fits; 642a7931 Top Up / Add Missing: discoverable Scope, PC Items and Bag + PC scopes (Gen I-V); f565cc63 Bulk planner: explain full native lists, unique across Bag + PC, Gen I Max Elixer
- **Automated tests:** tests/test_bag_top_up_adapters.cpp : Gen I shared 20-slot skips ('Bag full: its 20 slots are shared'), Max Elixer in Medicines; tests/test_ui_bag_top_up_host.cpp : 'Whole Bag (20 slots shared)' label and '19 free' preview
- **Hardware acceptance:** 066e1db9 R12 H10 · R12 item H10
- **Next action:** R12 H10 Gen I step; tell owner TMs can be topped up but not all missing items fit in 20 slots.
- **Owner** (E0052): > Gen 1 bag items, fill all doesn't fill all bags, only does current bag not tm or items
- **Owner** (E0118): > Gen I "All Bag pockets" Add Missing hits the native shared 20-slot Bag capacity, filling the earliest categories first. Fix the misleading UI behavior and report capacity/partial completion accurately. Never overflow the native Bag.
- **Occurrences (4):** R11 3634d1e6 Extra 1 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 1 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [owner message 2026-11-01 (independent Round 11 QA)]; REQ Extra 1 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-BLK-004` Gen I Add Missing fill order on the 20-slot Bag (ascending item id) - owner priority decision pending

- **Status:** Needs Investigation · **Gen:** I · **Area:** bulk-topup · **Category:** decision
- **Last owner result:** none
- **Basis:** Deferred in PR #332: order unchanged (ascending item id), 'Choosing a different priority is an owner decision.' No owner decision recorded. Owner's Extra 1 wording asks for TMs and items, which ascending order may starve.
- **Hardware acceptance:** none (decision needed)
- **Next action:** Ask owner for Gen I fill priority (e.g. one per category first) before v1.
- **Occurrences (1):** R12 066e1db9 n/a [PR #332 (body 2026-11-01)]

### `PBX-BNK-001` Legacy Storage (app-owned bank.dat) opens, browses and persists across games/restarts

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** bank-storage · **Category:** requirement · ⚠️ safety-critical
- **Last owner result:** PRE 3be4de6b (2026-09-03): PASS (A0218 PARTIAL)
- **Basis:** Owner PASS on 3be4de6b (Arbok persisted across games, A0217). Only Switch-native/backup-workspace sources were exercised; A0218 (Storage -> other backup) was PARTIAL. R8 H04 Bank restart persistence checklist (D0069) has no recorded result. Bank load/save hardening landed later (159b6c97, 080d7c8b; include/Trainer/Bank.h:28-40 LoadOutcome, buildVerifiedImage :63), so the accepted behaviour needs re-verification. For Gen I-IV sessions the Legacy Storage pane opens (HomeMenuPanel.cpp:119) but holds no Gen I-V Pokemon; Gen V refuses Storage (include/Integration/Gen5/Gen5HubRouting.h:46).
- **Repair commits:** 159b6c97 Q01 Bank: only a genuinely missing Bank is a first run; every unusable one blocks saving
- **Automated tests:** tests/test_bank_load_policy.cpp : LoadOutcome classification; tests/test_bank_recovery_contract.cpp / test_bank_fault_injection.cpp : interrupted save recovery, fail-closed writes; tests/test_bank_format_policy.cpp : box-count migration policy
- **Hardware acceptance:** PRE 3be4de6b; R8 H04 (6ec5ccb9) never reported; not on R12 checklist
- **Next action:** Add a Legacy Storage open/save/restart smoke (old R8 H04) to the next checklist, or record it as out of Gen I-V scope.
- **Owner** (A0119): > APP STORAGE PERSISTENCE: PROVEN
- **Owner** (A0217): > PASS / Arbok visible after switching games
- **Owner** (A0218): > REACHABLE WHEN COMPATIBLE; final persistence not exhaustively tested here
- **Occurrences (14):** PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PARTIAL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b STORAGE PASS [issue #8 (comment 2026-09-02)]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE 6 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; R8 6ec5ccb9 H04 n/a [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]

### `PBX-BNK-002` Legacy Storage pane keeps inherited X/Y controls (Y cycles Menu/Move/Multi, X sorts/moves/duplicates) unlike the Y-pick/A-place box carry

- **Status:** Open Defect · **Gen:** app · **Area:** bank-storage · **Category:** defect
- **Last owner result:** PRE 3be4de6b: FAIL
- **Basis:** Still present: TrainerViewScreenBase.inc:2457-2462 (Y cycles Menu->Move->Multi) and :2561-2581 (X picks up / starts Multi duplicate / sorts the box). Save-pane X/Y are stripped for read-only sources (:2437-2441) but the Bank pane keeps them. Intake Q4 (ROUND11_ENGINEERING_INTAKE.md:1430) asks whether the Bank pickup key should be unified; unanswered.
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads (save pane only)
- **Automated tests:** tests/test_source_mutation_policy.cpp : save pane locked, AppOwnedStorage pane mutable
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner decision on intake Q4; if unified, route the Bank pane through ClassicCarryRules (Y pick, A place, B cancel).
- **Occurrences (1):** PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]

### `PBX-BNK-003` No Bank deposit/withdraw for Gen I-V saves (H41/H42); bank.dat cannot hold PK1/PK2/PK4/PK5

- **Status:** Missing Feature · **Gen:** I, II, III, IV, V · **Area:** bank-storage · **Category:** missing-feature · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H41/H42: FAIL ('ui doesnt allow it' / 'not supported yet')
- **Basis:** Bank record tags exist only for GG/SWSH/BDSP/PLA/SV/ZA/FRLG (src/Trainer/Bank.cpp:81-116); there is no tag for RBY, GSC, DPPt/HGSS or BW/B2W2. Every classic Gen I-V source is RetroArchLegacy/ExternalLegacy, so save-pane X/Y are stripped (TrainerViewScreenBase.inc:2437-2441; canPerform false, include/Safety/SourceMutationPolicy.h canPerform). Gen V routes the Storage tile to a refusal (Gen5HubRouting.h:46; Gen5SharedReadOnlySurface.inc:1576-1580). Gen II excludes Bank from details (Gen2SharedSurfaceParity.inc:17-20). Gen III FRLG deposit/withdraw exists only in the inherited Switch-backup workspace, not for RetroArch FRLG. Net: deposit/withdraw is reachable only for Switch-native titles + FRLG backup workspaces.
- **Automated tests:** tests/test_storage_custody_contract.cpp / test_storage_conversion_custody_contract.cpp : cross-store Move custody (Switch-native groups only)
- **Hardware acceptance:** none (Vault build)
- **Next action:** Implement inside the Vault plan (intake gaps 2-4: format-neutral raw-bytes entity, COPY-first deposit, withdraw to empty destination with preview/cancel/Review, MoveTransaction journal). Do not extend bank.dat tags.
- **Owner** (E0040): > ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet
- **Owner** (E0041): > not supported yet
- **Occurrences (7):** R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R11 3634d1e6 H41 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H42 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H41 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 H42 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ H41 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ H42 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-BNK-004` In-save box names cannot be selected/renamed in Gen I-V (only Legacy Storage boxes rename); inconsistent per generation

- **Status:** Missing Feature · **Gen:** I, II, III, IV, V · **Area:** bank-storage · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H40: FAIL
- **Basis:** No Gen II-V native box-name writer exists; header focus is gated on trainer.supportsBoxNames() (TrainerViewScreenBase.inc:2448) and save rename requires a mutable workspace (renameBox, TrainerViewScreenBase.inc ~1540). Gen IV can focus the header then refuses 'source is read-only'; Gen II/III/V show or hide names inconsistently (intake VAULT inventory 4). Bank box rename works (renameBankBox gated by requireMutableStoragePane(1, Rename), TrainerViewScreenBase.inc:1580). Product decision pending (intake Q2: in-save rename in v1.0 or read-only with explanation).
- **Automated tests:** tests/test_source_mutation_policy.cpp : Rename allowed only for BackupOrStaged / AppOwnedStorage
- **Hardware acceptance:** none (not on R12 checklist)
- **Next action:** Owner answers intake Q2; at minimum make the read-only state consistent and explained in every generation.
- **Owner** (E0039): > can rename a box inside the save its doesnt allow me to select it, the only boxes i can rename are bank boxes or as their called right now in the app "legacy storage"
- **Occurrences (4):** R11 3634d1e6 H40 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H40 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ H40 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ VAULT gap 6-7 / Q2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-BNK-005` Legacy Storage save silently drops an occupied slot whose game group has no bank tag (verify treats it as empty)

- **Status:** Open Defect · **Gen:** app · **Area:** bank-storage · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** Still in source: serialize() skips an occupied slot whose bankTagFor() is BTAG_EMPTY ('unknown group -> can't tag it, skip', src/Trainer/Bank.cpp:212-214) and verifyImage() counts such a slot as unoccupied, so it is not a verify failure (Bank.cpp:497-501). Proposed repair R2 (F0031) is not applied; Bank.cpp unchanged since 159b6c97 (in 3634d1e6). Latent for Gen I-V: no UI path puts an untaggable classic record into the Bank today (PBX-BNK-003), but any future deposit path would lose the Pokemon on save.
- **Automated tests:** tests/test_bank_record_validation.cpp : corrupt records rejected on load (does not cover untaggable occupied slots on save)
- **Hardware acceptance:** none (host-test fix; not hardware-observable today)
- **Next action:** Apply intake repair R2: count an occupied untaggable slot as a verify FAILURE (fail closed) and add a host test, before any Vault/Bank deposit path is opened.
- **Occurrences (2):** R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; REQ VAULT R2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-CLN-001` Occupied/unavailable destination safety: Clone, Move and Import never silently overwrite a Pokemon (H32); Gen I/II carry must own the staged save

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** clone · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H32: NOT TESTED
- **Basis:** Clone: decideCloneDrop refuses Occupied/Protected/BoxFull (include/UI/ClassicCarryRules.h:97-108, statuses :116-127). Move: Gen III refuses ('Move destination must be empty; no overwrite', src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:1368-1375), Gen V requires emptyCloneDestination (ClassicPackedMoveOverlay.inc:1124-1128), Gen IV single Move SWAPS losslessly with read-back verification (Gen4StagedPokemonEditor.cpp:743-777; overlay status 'moved/swapped' ClassicPackedMoveOverlay.inc:1100), Gen IV multi refuses occupied (:1066-1070), Gen I/II insert into the packed list (Gen1PackedMove.cpp:80-111). Latent Gen I/II defect from intake (other staged changes accepted mid-carry, discard() left carry active) fixed by 53330345: refuseWhileCarrying (Gen1StagedPokemonEditor.cpp:343-354; Gen2StagedEditor.cpp:730-735) and discard() clears the carry (Gen1 :623, Gen2 :1664-1666). Owner could not test H32 ('dont know how to test this'); R12 Group A row for occupied destinations is NOT TESTED / NO FIXTURE (D0168, other area). NOTE: R12 H03 expects 'A on an occupied slot is refused' for Move in every generation, but Gen IV swaps and Gen I/II insert by design (tests/test_ui_clone_carry_host.cpp:244 'Gen IV swaps losslessly; Gen I / II insert') — the checklist wording will produce a false FAIL.
- **Repair commits:** 53330345 H32: Gen I/II carry transaction owns the staged save until place/cancel; 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels
- **Automated tests:** tests/test_classic_carry_rules.cpp : occupied/protected/full clone destinations refused, packed next free slot; tests/test_ui_clone_carry_host.cpp : Clone/Move onto occupied slot never overwrites (Gen III/V refuse, Gen IV swap, Gen I/II insert); tests/test_gen12_packed_move.cpp:528-640 : H32 Gen I/II staging refused mid-carry; discard ends carry
- **Hardware acceptance:** 066e1db9 R12 H03 + H04 (occupied-slot steps) · R12 item H03
- **Next action:** Correct R12 H03 wording per generation (Gen III/V refuse, Gen IV swap, Gen I/II insert) or get an owner decision that Move onto occupied must be refused everywhere; give the owner explicit steps (Clone onto occupied, Import onto occupied).
- **Owner** (E0031): > dont know how to test this
- **Occurrences (5):** R11 3634d1e6 H32 NOT TESTED [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H32 NOT TESTED [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ Required automated tests n/a [docs/SAVE_SAFETY.md]; REQ H32 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ H32 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-CLN-002` Gen I Clone destination browsing should show cached sprites in occupied slots (Cleanup #3 item 4)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I · **Area:** clone · **Category:** request
- **Last owner result:** none for this item (R11 3634d1e6 H31 owner note: 'gen 1 works perfect')
- **Basis:** The separate Gen I clone destination picker was replaced by the shared visual Clone carry over the normal box grid (6f62b69e; ClassicPackedMoveOverlay.inc beginClone ~:1395-1402, ghost slot ClassicCarryRules.h:110-114), so occupied cells render the box's own sprites and the faded copy shows where it lands. No owner result for the Cleanup #3 item itself; in R11 (before 6f62b69e) the owner said 'gen 1 works perfect' for Clone (E0030).
- **Repair commits:** 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels; 153e5aca R11 QA: Gen I/II clone wording is append-only; Gen I shared-Bag Top Up reports partial fits
- **Automated tests:** tests/test_ui_clone_carry_host.cpp : clone ghost drawn over box grid, packed append slot
- **Hardware acceptance:** 066e1db9 R12 H04 · R12 item H04
- **Next action:** Confirm during H04 on Gen I that occupied slots show sprites and the copy ghost sits after the last Pokemon.
- **Occurrences (1):** PRE 6ecd9c28 4 n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]

### `PBX-CLN-003` Clone to an available slot must be one flow in every generation (Gen II reopened Actions, Gen III-V numeric keypad that did not work, Gen IV defaulted to the occupied source slot); PKSE-style hover-and-A placement

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** clone · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H31: FAIL
- **Basis:** 6f62b69e moves Clone for all five generations onto the shared carry: Actions -> Clone closes the menu and carries a ghost copy, nothing staged until A on an EMPTY slot, B cancels, no keypad, no Actions reopened (include/UI/ClassicCarryRules.h:14-17, 54-62, 97-108; src/UI/ClassicPackedMoveOverlay.inc:658-760 placeClone/clone target, :1148 place() -> placeClone). Gen I/II append-only wording (E0119) applied in 153e5aca (ClassicCarryRules.h:45-61, 116-118). Gen IV default-to-source-slot keypad (E0106) is gone with the keypad. Clone remains an in-save byte copy; the Vault 'new Vault ID + provenance' semantics (F0083/F0085) depend on PBX-VLT-001. Owner's R11 'gen 1 works perfect' was for the pre-6f62b69e Gen I flow, which has now been replaced by the shared carry.
- **Repair commits:** 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels; 153e5aca R11 QA: Gen I/II clone wording is append-only; Gen I shared-Bag Top Up reports partial fits; 91cf155e Round 11 integration: carry comments name A to place (H23)
- **Automated tests:** tests/test_ui_clone_carry_host.cpp : per-gen Clone pickup/no keypad/no menu reopen/A places/B cancels with nothing staged; tests/test_classic_carry_rules.cpp : decideCloneDrop, packed append, statuses; tests/test_gen2_clone_flow.cpp : Gen II clone backend
- **Hardware acceptance:** 066e1db9 R12 H04 · R12 item H04
- **Next action:** Run H04 in every generation, specifically re-checking Gen I because its accepted flow was replaced.
- **Owner** (E0030): > gen 1 works perfect, gen 2 when i clone a pokemon its creates it in the box and than revert back to the action menu with clone, edit,view,release,oring and legality, it shouldnt do that, it should clone the pokemon and no menu popup like gen 1, gen 3 has the numpad come up when clone is click on and that is completey wrong fix this so it behaves like gen 1 but with being able to select which slot it can on to now that it isnt locked like gen 1/2. ps even when i entered a number on the numpad on gen 3 it didnt work so yeah that broken. gen 4/5 has same problem as gen 3, when they are all fixed they should work like gen 1 but have the ability to select which slot cause it aint like gen 1/2 now, look at PKSE how they clone and also can we have the hover over box feature that PKSE has, so we go to the box/slot we want and than press A to confirm the clone and it pops the pokemon in the slot.
- **Owner** (E0119): > Gen I/II packed boxes only permit appending at the first empty slot. Their destination picker must not advertise arbitrary empty positions.
- **Occurrences (11):** PRE D12 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R11 3634d1e6 H31 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H31 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; R11 3634d1e6 n/a [owner message 2026-11-01 (independent Round 11 QA)]; R12 066e1db9 H04 n/a [docs/hardware/RETEST_066e1db9.md]; REQ D12-D14 / C09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ EXACT CLONE / DERIVED CLONE / COPY n/a [docs/POKEMON_IDENTITY_CUSTODY_CONTRACT.md]; REQ Clone n/a [docs/UI_FLOW.md]; REQ H31 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-CLN-004` Open owner decisions: Gen II egg-clone refusal; whether Gen V accepts a slot freed by a staged Release

- **Status:** Needs Investigation · **Gen:** II, V · **Area:** clone · **Category:** decision
- **Last owner result:** none
- **Basis:** Current behaviour: Gen II refuses egg cloning ('egg cloning is not enabled in this milestone', src/Integration/Gen2/Gen2StagedEditor.cpp:1297); Gen V refuses a destination with a pending change, including one freed by a staged Release (Gen5StagedPokemonWorkspace.h:383-388 pending_.count). Unknown: the owner's intended policy for both.
- **Automated tests:** tests/test_gen2_clone_flow.cpp : Gen II clone refusals
- **Hardware acceptance:** none (owner decision first)
- **Next action:** Ask the owner both questions; then pin the answer in tests.
- **Occurrences (1):** R12 066e1db9 n/a [PR #332 (body 2026-11-01)]

### `PBX-CRT-001` Inherited Create/mutation UI reachable on game saves (read-only alpha); Create must stage into a working copy and never write the original save

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** create · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R11/3634d1e6/H30: PASS
- **Basis:** Create is now an intended staged feature: every generation stages into the app-owned working copy (e.g. src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:229-234 "original RetroArch .srm unchanged"; include/UI/Gen4SharedPokemonSession.h:245-256 stageCreateBoxPokemon). Owner PASS R11 3634d1e6 H30 "Create, back out, keep" (Expected: original save not written). Create/Keep code changed in Round 11 (620ee717, ab072252), so R12 must recheck.
- **Repair commits:** 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07); ab072252 Round 11 follow-ups: explicit Met Location choice, H25 history flags, visible Create resets
- **Automated tests:** tests/test_species_change_history_policy.cpp : CreateRequiredFields refusal leaves draft/staged copy unchanged; tests/test_ui_gen2_create_review_host.cpp : Gen II Create stages into the working copy and appears in Review
- **Hardware acceptance:** 066e1db9 R12 H17 (Review/Keep/Discard/Export, original unchanged) · R12 item H17
- **Next action:** Owner runs R12 H17 on 066e1db9 including one Create; confirm original save SHA unchanged.
- **Occurrences (2):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]

### `PBX-CRT-002` Create opens one shared editor workspace (no multi-page wizard, persistent draft overview) from any applicable empty slot

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** create · **Category:** request
- **Last owner result:** R11/3634d1e6/H30: PASS
- **Basis:** Gen I three-panel workspace accepted PRE 69668bc8 (A0542/A0540, GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md); Gen V Create runs through the shared surface (de22e35f). Latest owner result R11 3634d1e6 H30 "Create, back out, keep" PASS (game not recorded on the worksheet). Create entry points changed in Round 11 (src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:177-237, src/UI/Gen2PokemonEditorFoundation.inc:200-215, include/Integration/Gen5/Gen5SharedScreenState.h:503-530).
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07)
- **Automated tests:** tests/test_qa_matrix_gen12.cpp / docs/qa/GEN1_5_MATRIX.md creation cell : host Create from production entry points on every version
- **Hardware acceptance:** 066e1db9 R12 H07 / H18 (smoke) · R12 item H07
- **Next action:** Recheck in R12 H07/H18; ask the owner to note which generation(s) H30-style Create was tried on.
- **Occurrences (8):** PRE 574d604b 1 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 574d604b 4 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE D2 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE creation PASS [docs/qa/GEN1_5_MATRIX.md]; PRE creation / fields / pickers NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE C11 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE D01 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R11 3634d1e6 H30 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-CRT-003` Gen II empty-slot Create blocked by "RetroArch source is read-only" (staged copy must stay editable)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** create · **Category:** defect
- **Last owner result:** R3/62a2f8c3/C05: PASS
- **Basis:** Owner accepted cfb70357 on hardware (PR #68 body, rid D0303: Gen I/II staged editing "look and work as intended"); later Gen II Create PASS R1 C05 and R3 C05. Gen II Create code changed in Round 11 (src/UI/Gen2PokemonEditorFoundation.inc:207-210).
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions
- **Automated tests:** tests/test_ui_gen2_create_review_host.cpp : Gen II Create stages and is listed in Review
- **Hardware acceptance:** cfb70357 PRE (PR #68); 066e1db9 R12 H18 smoke · R12 item H18
- **Next action:** Smoke-check Gen II Create in R12 (H07 for moves/H18).
- **Owner** (D0289): > - Gen II empty-slot Create is incorrectly blocked by `RetroArch source is read-only. Editing this file is disabled.` The source must stay immutable while the PokeBank staged copy remains editable.
- **Occurrences (1):** PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]

### `PBX-CRT-004` Gen I Add/Create destination in packed boxes: refused destinations, then appended at packed position not cursor cell (needs product decision)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** create · **Category:** decision · ⚠️ safety-critical
- **Last owner result:** PRE/cfb70357/PR #68 milestone: PASS
- **Basis:** Current code appends at the native packed slot: ux2BeginAdd uses e->appendSlot (src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:182-187); older overlay refuses non-first-empty with "Gen I boxes are contiguous; Add is available at the first empty slot only" (src/UI/Gen1PokemonEditorOverlay.cpp:237-240). Owner accepted cfb70357 (PR #68 body, D0303). No separate recorded owner product decision on visual-cell vs packed placement was found beyond that acceptance.
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions
- **Automated tests:** tests/test_rby_pokemon_editor.cpp : Gen I Create defaults and packed append
- **Hardware acceptance:** cfb70357 PRE (PR #68)
- **Next action:** No action unless the owner reopens; Gen I Create is exercised by R12 H18 smoke.
- **Owner** (D0295): > - **Gen I R/B/Y:** Create flow is now reachable from arbitrary empty visual cells, but `Stage Add` still serializes the new Pokémon at the native packed append position next to existing Pokémon, not at the exact empty visual cell the cursor was on. This needs an explicit product decision because native Gen I boxes are packed/contiguous and do not persist arbitrary sparse holes. Do not silently pretend native slot 14 exists if only two Pokémon are stored.
- **Owner** (D0294): > - Gen I R/B/Y Add/Create currently refuses destinations because of a global `next writable slot` / contiguous-box rule; the selected current box should be writable when native capacity exists, while still respecting the packed Gen I format and not fabricating persistent sparse holes.
- **Occurrences (2):** PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]; PRE 501093ae FAIL [PR #68 (comment 2026-09-15)]

### `PBX-CRT-005` Gen II post-Add auto-opens Pokemon Actions instead of returning to the box

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** create · **Category:** defect
- **Last owner result:** PRE/cfb70357/PR #68 milestone: PASS
- **Basis:** Owner accepted cfb70357 (PR #68 body, D0303). Gen II Create/Add path changed in Round 11 (Gen2PokemonEditorFoundation.inc:316 missingRequiredFields gate).
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions
- **Hardware acceptance:** cfb70357 PRE (PR #68) · R12 item H18
- **Next action:** Observe during R12 Gen II Create that Add returns to the box.
- **Owner** (D0299): > - **Gen II post-Add flow:** after `X Stage Add`, a `Pokemon Actions` modal opens automatically. Owner expects Add to return to the box with the new Pokémon present; Actions should appear only when the user explicitly interacts with an occupied Pokémon.
- **Occurrences (1):** PRE 501093ae FAIL [PR #68 (comment 2026-09-15)]

### `PBX-CRT-006` Gen IV real Create/Add; Create must enter the shared editor (not auto-open Species); fuller Gen IV fields

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** IV · **Area:** create · **Category:** request
- **Last owner result:** R11/3634d1e6/H30: PASS (gen not recorded)
- **Basis:** Gen IV Create exists: src/UI/Gen4SharedPokemonSurface.inc:586-610 beginCreate -> createBoxDraft(…,25) -> shared session (no forced species picker). Latest owner results that cover Create on all gens: R6 26186464 A17 (Create defaults) PASS, R6 C13 Create Review PASS, R11 3634d1e6 H30 PASS (generation not recorded). No Gen IV-specific Create PASS record exists.
- **Repair commits:** aff41765 fix(gen4): align create editor with classic parity; 66312f10 test(gen4): fix parity contract source assertion
- **Automated tests:** tests/test_gen4_create_species_change_history.cpp : Gen IV Create draft species change/history wiring
- **Hardware acceptance:** 066e1db9 R12 H07 (Gen IV) · R12 item H07
- **Next action:** R12 H07 explicitly includes Gen IV Create; record the Gen IV game used.
- **Owner** (D0316): > - Add/Create auto-opened Species instead of entering the shared editor like Gen I-III.
- **Owner** (D0315): > - real Gen IV Create/Add Pokémon; - fuller Gen IV field coverage; - visible read-only rows should remain inspectable; - remaining deferred fields/pickers need proper exact Gen IV implementations.
- **Occurrences (2):** PRE 84dae170 n/a [issue #86 (comment 2026-09-28)]; PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]

### `PBX-CRT-007` Gen V empty encrypted retail PK5 slots not recognised; Create showed only Cancel; uninitialised new-game trainer must be refused

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** create · **Category:** defect
- **Last owner result:** R11/3634d1e6/H07: PASS
- **Basis:** Owner PASS R11 3634d1e6 H07 (Gen V: create in an empty BW/B2W2 box slot, Keep after choosing a location). C12 (new-game uninitialised trainer refusal) has no owner result. Gen V Create code changed in Round 11 (include/Integration/Gen5/Gen5SharedScreenState.h:503-530, 455-465).
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface
- **Automated tests:** tests/test_gen5_create_edit_parity.cpp : Gen V Create header/rows; docs/GEN5_NATIVE_CREATE_VALIDATION.md validateCreate5 host tests
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** R12 H07 on a Gen V save; C12 (fresh new-game save) remains untested — add as optional fixture item if a fresh save exists.
- **Occurrences (3):** PRE C10 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE C12 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  FAIL [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-CRT-008` Gen V Create/Edit/View parity with Gen I-IV: left-panel species text clipped; met/origin text overlapped graph

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** create · **Category:** defect
- **Last owner result:** R3/62a2f8c3/S03: PASS
- **Basis:** Owner PASS R2 e10f307f S03 and R3 62a2f8c3 S03 ("Gen V Details panel"); guarded by tests/test_gen5_create_edit_parity.cpp. A0935 (P6 met text overlaps graph on 35cbf163) was fixed by de22e35f before R1 and the panel layout is covered by the S03 PASS. Gen V surface changed in Round 11.
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; b57ffe03 S03 + Phase 4: one DETAILS column for Gen III-V, locked rows gold, exact game; 4f030aaa S03: update the Gen V DETAILS label pin to the shared label set
- **Automated tests:** tests/test_gen5_create_edit_parity.cpp : S03 labels fit the column; locked rows gold
- **Hardware acceptance:** 62a2f8c3 R3 S03 · R12 item H18
- **Next action:** R12 H18 smoke on a Gen V Pokemon editor.
- **Owner** (D0228): > **Six unchecked items with reported problems:** B01/B02/B03/B06 (Gen I–III Bag Add-before-quantity workflow), S01 (Gen V PP/PP Ups crowded), S03 (Gen V Create/Edit clipped field text and parity).
- **Owner** (B0050): > S03 gen 5 editor doesnt match gen 1-4 th left side menu text for the speices names is cut off and all that plkus awhole lot more.
- **Occurrences (11):** PRE 35cbf163 P6 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; R1 7fea1797 S03 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 S03 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 S03 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 S03 FAIL [issue #313 (comment 2026-10-17)]; R2 S03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f S03 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f S03 PASS [tests/test_hw_round1_regression_gate.py]; R2 e10f307f S03 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R3 62a2f8c3 S03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 S03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-CRT-009` Gen I Create default draft: Pikachu L5 with Thunder Shock/Growl (owner request) -> Gen II Create seeded level-up moves (C14) -> REVERSED R11 H28/Extra 5/Extra 6: new drafts start with NO moves and a species choice never auto-fills moves

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** create · **Category:** decision · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H28: PASS with request (blank moves); earlier R6/26186464/C14: PASS for the now-reversed behaviour
- **Basis:** Owner history: E12 PASS R1/R3 (Pikachu with Thunder Shock/Growl) and C14 PASS R6 26186464 (Gen II Create seeds level-up moves) were reversed by the owner in R11 (H28 note, Extra 5, Extra 6: "we need to have a blank canvas, so no moves"). Latest owner decision = blank. Verified in current source: Gen I BoxPokemonCreate{} moves{} (include/Integration/Gen1/Gen1StagedPokemonEditor.h:28-35); Gen II defaultCreateRecord with no moves (src/UI/Gen2PokemonEditorFoundation.inc:207-210); Gen III beginCreate sets no moves (include/UI/Gen3SharedPokemonSession.h:130-144); Gen V r.moves={} draftWithoutMoves (include/Integration/Gen5/Gen5SharedScreenState.h:521-523); species change never adds a move in any gen (include/UI/SpeciesChangeMoves.h:5-12). No owner hardware result after the change.
- **Repair commits:** 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07); 160e5c6b Update tests that pinned auto-filled Create moves (owner reversed, R11 H28); b155e174 docs(handoff): Lane C checkpoint, Gen II Create moves (the reversed C14 behaviour)
- **Automated tests:** tests/test_species_change_moves_gen2.cpp : "Create starts with no moves (C14 reversed)" (guard in tests/test_hw_round1_regression_gate.py C14); tests/test_species_change_history_policy.cpp : a blank draft stays blank after a species change; tests/test_species_change_moves_gen1.cpp .. _gen5.cpp : per-gen no-auto-fill
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** Owner verifies R12 H07 ("The new draft starts with no moves") in Gen III-V and also checks Gen I/II Create (H07 only names Gen III-V) and a species switch in Create leaves moves empty.
- **Owner** (E0069): > … when the editor is opened the new pokemon … doesnt come with its default moves and alson when we switch species i dont want it to come with moves auto filled in … a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves …
- **Owner** (E0057): > gen 1/2 have moves when pokemon editor comes up, pikachu has growl and thunder shock i think, we need to have a blank canvas, so no moves, and same for when a pokemon/species is selected we shouldn't have the moves auto filled, that should be left for the user, also can we add the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add and we have a popup in the middle bottom to tell us what we are missing so we can ad the pokemon this should be a universal rule.
- **Owner** (E0050): > this passes but i wanna have it where when the editor is opened the new pokemon that hasnt been added yet doesnt come with its default moves and alson when we switch species i dont want it to come with moves auto filled in, i want the user to pick the moves and all the info first and also can we have it where a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves unless its breed or however that would be legal,
- **Occurrences (19):** PRE — n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE create.default.species n/a [docs/qa/GEN1_5_MATRIX.md]; R1 7fea1797 E12 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E12 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 E12 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E12 PASS [tests/test_hw_round1_regression_gate.py]; R2 E12 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f E12 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 E12 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E12 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R6 26186464 C14 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C14 · Gen II Create moves PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 C14 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A44 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 H28 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 Extra 6 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 H28 PASS [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; REQ H28 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-CRT-010` Gen II Create A/B controls misroute (A/B "doesnt know what to press")

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** create · **Category:** defect
- **Last owner result:** R3/62a2f8c3/C05: PASS
- **Basis:** Owner PASS R1 7fea1797 C05 and R3 62a2f8c3 C05 ("Gen II Create controls"); guarded by tests/test_hw_round1_regression_gate.py C05. Gen II Create input/surface files changed after R3 (Round 11: src/UI/Gen2PokemonEditorFoundation.inc, Gen2SharedPokemonSurface.inc).
- **Automated tests:** tests/test_hw_round1_regression_gate.py : C05 per-ID guard
- **Hardware acceptance:** 62a2f8c3 R3 C05 · R12 item H18
- **Next action:** Smoke-check Gen II Create A/B in R12 H18.
- **Owner** (D0215): > gen 2 cretae controls are bugged when i try and press A or B on something it doesnt know what to press and enters wrong, that needs looked at and fixed.
- **Occurrences (10):** PRE a0633b6d FAIL [issue #320 (body 2026-10-16)]; R1 7fea1797 C05 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C05 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 C05 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C05 PASS [tests/test_hw_round1_regression_gate.py]; R2 C05 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C05 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C05 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C05 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-CRT-011` Gen V Create header must name the exact game / staged source like Gen I-IV

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** create · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-GEN5HEADER: PASS
- **Basis:** Owner PASS R3 62a2f8c3 R2-GEN5HEADER; guarded by tests/test_gen5_create_edit_parity.cpp ("S03 Create header names the game") via the regression gate. Gen V surface changed in Round 11 (src/UI/Gen5SharedReadOnlySurface.inc).
- **Repair commits:** b57ffe03 S03 + Phase 4: one DETAILS column for Gen III-V, locked rows gold, exact game
- **Automated tests:** tests/test_gen5_create_edit_parity.cpp : Create header names the exact game
- **Hardware acceptance:** 62a2f8c3 R3 · R12 item H07
- **Next action:** Glance at the header during R12 H07 Gen V Create.
- **Owner** (B0067): > gen 5 saves dont tell me in the create menu that im making a pokemon for what game like the rest in gens 1-4 do at the top of the screen where source save staged is.
- **Occurrences (6):** R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 Gen V Create header (Phase 12) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f Gen V Create header (Phase 12) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-GEN5HEADER PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-GEN5HEADER PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CRT-012` New Pokemon IVs/DVs defaulted wrong (Gen V all 15); Create must start IVs/DVs at 0 without resetting existing Pokemon

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** create · **Category:** defect
- **Last owner result:** R6/26186464/A17: PASS
- **Basis:** Owner PASS OCT24 c60d4da0, R5 1ca2faec, R6 26186464 A17. Current source: Gen V r.ivs={} r.evs={} (include/Integration/Gen5/Gen5SharedScreenState.h:519-520); Gen I/II BoxPokemonCreate dvs{} (include/Integration/Gen1/Gen1StagedPokemonEditor.h:34, include/Integration/Gen2/Gen2StagedEditor.h:71). Create-draft code in every generation changed in Round 11 (moves/required fields), so recheck.
- **Repair commits:** 88f974da Stage native PK5 language and unique inventory with zero creation stats and shared UI repairs
- **Automated tests:** tests/test_hw_round1_regression_gate.py : NEW-15 GUARDS_ROUND5 entry
- **Hardware acceptance:** 26186464 R6 A17 · R12 item H07
- **Next action:** During R12 H07 confirm a new draft shows IVs/DVs 0.
- **Owner** (B0564): > Gen V creates all IVs=15; owner wants Create defaults IVs=0 for Gen III–V and DVs=0 for Gen I–II. Don't reset existing Pokémon; preserve native shiny/gender constraints.
- **Occurrences (9):** R3 62a2f8c3 NEW-15 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-15 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-15 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-15 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-15 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-15 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A17 · NEW-15 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A17 · NEW-15 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A17 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-CRT-013` Gen III Create: every caught Ball incl. Safari, held item independent of Bag availability, named values in Review, export/reopen

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** III · **Area:** create · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** R6/26186464/A23: PASS
- **Basis:** Owner FAIL OCT24 c60d4da0, repaired 7130bff3; owner PASS OCT25 1ca2faec and R6 26186464 A23 ("Created a Gen III Pikachu with Safari Ball, exported it, then reopened the export ... with the edits intact"). Gen III Create path (src/Integration/Gen3/Gen3StagedPokemonEditor.cpp, include/UI/Gen3SharedPokemonSession.h) changed in Round 11 (required fields, Met Location list).
- **Repair commits:** 7130bff3 fix(gen3): validate Create held items independently of Bag availability
- **Automated tests:** tests/test_hw_round1_regression_gate.py : R2-REVIEWMOVE GUARDS_ROUND6 entry (all 12 caught balls)
- **Hardware acceptance:** 26186464 R6 A23 · R12 item H07
- **Next action:** R12 H07 FR/LG Create + H17 export; optionally re-run the Safari Ball export/reopen.
- **Owner** (C0303): > Created a Gen III Pikachu with Safari Ball, exported it, then reopened the export from Save Backups with the edits intact
- **Occurrences (9):** R4 c60d4da0 R2-REVIEWMOVE n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-REVIEWMOVE FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 6 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec R2-REVIEWMOVE n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec R2-REVIEWMOVE PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec R2-REVIEWMOVE PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A23 · R2-REVIEWMOVE n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A23 · R2-REVIEWMOVE PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A22 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-CRT-014` Create requires the native required fields before Keep/Add: explicit Met Location choice (Gen III-V, exact-game list, never "value != 0") and at least one move (app policy) — owner asked for it "for all gens"

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** create · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H07: PASS (Gen V only) with request "this needs to be added for all gens"
- **Basis:** Shared rule include/UI/CreateRequiredFields.h:1-76 (Met Location only for gens 3-5 at :46-48, at least one move everywhere, flag-based "chosen", no move-count rule). Wired: Gen I src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:223-227 (move only); Gen II include/UI/Gen2PokemonSession.h:221-243 (move only; Crystal caught data optional); Gen III include/UI/Gen3SharedPokemonSession.h:264-270,465,479; Gen IV include/UI/Gen4SharedPokemonSession.h:200-256; Gen V include/Integration/Gen5/Gen5SharedScreenState.h:455-465. Gen V H07 PASS (R11) predates the cross-gen change. The owner's "level 100 should have 1-2 moves" idea (H28 note) was NOT implemented (intake section 4 E0098: not a game rule); owner clarification 1 (E0102) frames the one-move rule as app policy — the level-based rule has no explicit owner sign-off.
- **Repair commits:** 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07); e86d2a2c CreateRequiredFields: one-move rule is a PokeBank app policy, not legality; ab072252 Round 11 follow-ups: explicit Met Location choice, H25 history flags, visible Create resets
- **Automated tests:** tests/test_species_change_history_policy.cpp : CreateRequiredFields Met Location Gen III-V only, one move everywhere, one message, no move-count rule; tests/test_gsc_pokemon_session.cpp : Gen II Add refused without a move; R12 Group A row "Gen III / IV Create Met Location counts only an explicit pick (never value != 0)" : NOT TESTED / NO FIXTURE on hardware
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** Owner runs R12 H07 in Gen III (FR/LG), IV and V; also try Add with no move in Gen I/II (not on H07). Ask the owner to confirm dropping the "level 100 = 1-2 moves" idea.
- **Owner** (E0102): > **Create validation is exact-game / format-aware.** "At least one move" is an **app policy** for a usable Pokémon, not a legality proof, and is labelled as such. Met Location is required as an **explicit choice** in the exact game's own location list. Never require a *nonzero* value: a real native Pokémon can legitimately carry location 0 or no recorded location (for example Gen III RSE index 0, Gen IV/V special or "----" entries, traded or event records). The check must use each game's native semantics, never one universal rule.
- **Owner** (F0111): > gen 1/2 have moves when pokemon editor comes up, pikachu has growl and thunder shock i think, we need to have a blank canvas, so no moves, and same for when a pokemon/species is selected we shouldn't have the moves auto filled, that should be left for the user, also can we add the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add and we have a popup in the middle bottom to tell us what we are missing so we can ad the pokemon this should be a universal rule.
- **Owner** (E0085): > gen 1/2 have moves when pokemon editor comes up, pikachu has growl and thunder shock … blank canvas … the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add … universal rule.
- **Occurrences (13):** R9 d6a20a2a H07 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H07 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H07 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H07 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H07 PASS [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 Extra 6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; R11 3634d1e6 Owner safety clarification 1 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 owner safety clarifica…]; R12 066e1db9 H07 n/a [docs/hardware/RETEST_066e1db9.md]; REQ Extra 6 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ H07 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ §4 Moves / Required fields n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ Owner safety clarification 1 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-CRT-015` Create origin game: Create always records the save's own game as origin; non-native origin (e.g. FR/LG Rayquaza with R/S/E origin) is an open owner v1.0 decision

- **Status:** Needs Investigation · **Gen:** I, II, III, IV, V · **Area:** create · **Category:** decision · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Current source: Gen III Create origin forced to the save game (Gen3StagedPokemonEditor.cpp create path; beginCreate(trainer, originGame) include/UI/Gen3SharedPokemonSession.h:130-144); Gen V origin = trainer.gameVersion (Gen5SharedScreenState.h:686). Intake FRLG section open question 1 and PR #332 body: "a non-native origin for Create is an owner v1.0 decision". Unknown: the owner's decision. Related: owner's Extra 2 belief that FR/LG reads RSE species is incorrect (all 386 selectable; intake FRLG skeptic note g).
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Ask the owner to decide whether v1.0 Create offers a non-native origin picker; record the decision in PRODUCT_DECISIONS.md.
- **Occurrences (1):** R12 066e1db9 n/a [PR #332 (body 2026-11-01)]

### `PBX-CRT-016` Create Pokemon / Quick Legal / advanced editor as permanent core feature: Quick Legal and explicit CREATED/USER-CREATED provenance labelling

- **Status:** Missing Feature · **Gen:** all · **Area:** create · **Category:** missing-feature
- **Last owner result:** none
- **Basis:** No Quick Legal entry point or CREATED/USER-CREATED provenance label exists in src/ or include/ (grep "Quick Legal", "USER-CREATED" returns nothing). docs/CREATE_POKEMON_VISION.md header marks it future-only; docs/PRODUCT_DECISIONS.md §7 says "should eventually provide". Guided Create itself exists.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Keep on the post-v1 backlog unless the owner promotes it; depends on the isolated legality engine (#329).
- **Occurrences (2):** REQ §7 n/a [docs/PRODUCT_DECISIONS.md]; REQ Header / Product decision n/a [docs/CREATE_POKEMON_VISION.md]

### `PBX-CRT-017` Gen II Add toast too implementation-facing ('Pokemon added to staged copy only; source .srm unchanged'); owner prefers concise wording such as 'Added to Box 1 (staged)'

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** create · **Category:** defect
- **Last owner result:** PRE/cfb70357/PR #68 milestone: PASS
- **Basis:** Owner FAIL on 501093ae (D0300). 36a89388 (2026-09-14 21:10, after 501093ae) replaced the toast with '<name> added to Box N' (git show 36a89388: Gen2PokemonEditorFoundation.inc); current source src/UI/Gen2PokemonEditorFoundation.inc:333-337 posts displayName + ' added to Box ' + N. 36a89388 is an ancestor of cfb70357, which the owner accepted on hardware (PR #68 body, D0303: '... staged editing, and the hardware-retst fixes all look and work as intended'). The Gen II Add path changed again in Round 11 (required-fields gate before state.add, Gen2PokemonEditorFoundation.inc:316-328), so it needs re-verification.
- **Repair commits:** 36a89388 ui: return Gen II Add directly to packed box selection; cfb70357 fix: address Gen I and II hardware retest regressions
- **Automated tests:** tests/test_gsc_editor_surface_contract.cpp:130 (49d88330) : Gen II Add calls closeToBoxAfterAdd (returns to the packed box); no test asserts the toast wording
- **Hardware acceptance:** accepted cfb70357 PRE (PR #68); recheck on 066e1db9 R12 H18 · R12 item H18
- **Next action:** During R12 Gen II Create/Add, confirm the status line reads '<name> added to Box N' with no implementation wording.
- **Owner** (D0300): > - **Gen II staged toast:** `Pokemon added to staged copy only; source .srm unchanged` is too implementation-facing/noisy. Prefer concise PokeBank wording such as `Added to Box 1 (staged)` while source immutability remains persistent elsewhere in UI.
- **Occurrences (1):** PRE 501093ae FAIL [PR #68 (comment 2026-09-15)]

### `PBX-CRY-001` Inherited Y grab/Move/Multi mutation controls reachable in installed game-save views (read-only alpha contract)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** move-carry · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** PRE 3be4de6b: FAIL
- **Basis:** 0ea98cc1 locks installed-source UI: canPerform() allows mutations only for BackupOrStaged/AppOwnedStorage (include/Safety/SourceMutationPolicy.h canPerform); Storage save-pane X/Y are stripped for read-only sources (src/UI/TrainerViewScreenBase.inc:2437-2441). The second device checklist rows that would verify it (A0264-A0282 range, 'NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE' F0090) were never filled. For Gen I-V the Y carry is now an intended staged-copy feature (PBX-CRY-003).
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads
- **Automated tests:** tests/test_source_mutation_policy.cpp : installed/legacy sources refuse DirectMove/Edit/Rename
- **Hardware acceptance:** none (Switch-native titles are outside the Gen I-V R12 checklist)
- **Next action:** Verify on an installed Switch-native title when Gen VI+ is tested.
- **Owner** (A0172): > grab/move behavior
- **Occurrences (3):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-CRY-002` Gen IV Y-carry pickup/placement display (duplicated pickup sprite, Summary showing held PK4) and Y multi-move

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** IV · **Area:** move-carry · **Category:** defect
- **Last owner result:** R3 62a2f8c3 AUD-08: PASS
- **Basis:** Owner PASS 61ce32ac ('ok thats a pass for me', D0342), d0a45909 Gen IV Y-carry accepted (A0940), and R3 62a2f8c3 AUD-08 Gen IV Y multi-move PASS. Placement key changed from Y to A for all generations in 6f62b69e (ClassicCarryRules.h:36-42), so re-verification needed.
- **Repair commits:** 61ce32ac Fix Gen IV Y-carry ghost source and stale Summary without touching staged PK4; 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels
- **Automated tests:** test_gen4_hardware_surface_contract : Gen IV Boxes carry; tests/test_hw_round1_regression_gate.py : AUD-08 guard (GUARDS_ROUND3)
- **Hardware acceptance:** R3 62a2f8c3 AUD-08; re-verify 066e1db9 R12 H03 · R12 item H03
- **Next action:** H03 Gen IV, including a Y-multi rectangle drop.
- **Owner** (D0341): > the remaining issue is Gen IV Diamond **duplicated pickup/source sprite and Summary showing the still-held PK4**, unlike Gen III Emerald.
- **Owner** (D0342): > **"ok thats a pass for me"**
- **Occurrences (7):** PRE D7 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE C09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  FAIL [PR #278 (comment 2026-10-12)]; PRE 61ce32ac PASS [PR #278 (comment 2026-10-12)]; PRE d0a45909 PASS [docs/handoffs/GEN14_FINAL_COMPLETION_PROGRESS.md]; R3 62a2f8c3 AUD-08 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 AUD-08 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CRY-003` Move controls: Y picks up, A places, B cancels in every generation with footer/Help updated (H23 request); Move is an atomic staged copy/remove with the source kept until the destination is verified

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** move-carry · **Category:** request · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H23: PASS (with this request)
- **Basis:** Shared rules: carryKey() B=Cancel, A=Place, Y/X refused with hint and consumed (include/UI/ClassicCarryRules.h:36-52, static_asserts :133-136); footer 'A Place / L/R Box / B Cancel' (:74-77); used by Gen I-IV handleInput and Gen V handleGen5CarryInput (src/UI/ClassicPackedMoveOverlay.inc:1414-1430). Atomicity: each backend stages on a copy and rolls back on failed read-back (Gen4StagedPokemonEditor.cpp:743-777; Gen3StagedPokemonEditor.cpp:1378-1389; Gen IV overlay restores backup on presentation failure ClassicPackedMoveOverlay.inc:1086-1094; Gen V dropCarriedPokemon re-checks the carried record :1129-1140). No owner result yet for A-to-place.
- **Repair commits:** 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels; 91cf155e Round 11 integration: carry comments name A to place (H23); 3cacfa98 Round 11 integration: Pokemon Help keeps 10 rows; carry contract follows the shared footer model
- **Automated tests:** tests/test_classic_carry_rules.cpp : key mapping, footer, refuse hints; tests/test_ui_clone_carry_host.cpp : per-gen Y pick, Y refused, B restores exact staged state, A places, no Actions after move; tests/test_classic_packed_move_recovery_contract.cpp : packed move recovery
- **Hardware acceptance:** 066e1db9 R12 H03 · R12 item H03
- **Next action:** Run H03 per generation; check Help pages mention A to place.
- **Owner** (E0049): > it works, can we fix for all gens when we pick up a pokemon can we change the drop control to A, so it would be moving the pokemon to the spot you want and pressing and and that would set the pop there instead of pressing Y again to drop it, can we also fix the footer and all the help menu and anything else that would need to be changed for this to work
- **Occurrences (9):** PRE D13 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R11 3634d1e6 H23 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H23 PASS [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; R12 066e1db9 H03 n/a [docs/hardware/RETEST_066e1db9.md]; REQ MOVE transaction n/a [docs/POKEMON_IDENTITY_CUSTODY_CONTRACT.md]; REQ H23 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ §4 Box controls / §2 carry n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-CRY-004` Gen I (Yellow) Box Actions -> Move reachable and starts the Boxes carry (R2-CHECK-009)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** move-carry · **Category:** defect
- **Last owner result:** R3 62a2f8c3 R2-CHECK-009: PASS
- **Basis:** Owner PASS R3 62a2f8c3 R2-CHECK-009 ('Y move works in Gen I.'). Drop key changed from Y to A (6f62b69e) and Gen I sheet rebuilt (dd69bc93).
- **Repair commits:** 1dcf2cc3 WIP: feat(gen1): Box Actions lists Move and starts the Boxes carry (R2-CHECK-009); 9d4b8b71 Merge audit/app-qa-20261014 (d98d250, C 114a22f3 R2-CHECK-009/R2-BUG-008 + E02 pins; every contract enforced) into the Gen V integration branch
- **Automated tests:** tests/test_gen12_packed_move.cpp : Gen I sheet with/without Move; Move -> drop stages one verified move; B leaves box byte-identical
- **Hardware acceptance:** R3 62a2f8c3; re-verify 066e1db9 R12 H03/H05 · R12 item H03
- **Next action:** H03 Gen I via Actions -> Move and via Y.
- **Owner** (B0544): > Y move works in Gen I.
- **Occurrences (4):** R3 62a2f8c3 R2-CHECK-009 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-009 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-009 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-009 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CRY-005` Gen II (Gold/Silver/Crystal) PC box Move pickup/drop/between boxes not completing (NEW-04)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** move-carry · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** R11 3634d1e6 H23: PASS
- **Basis:** R3 FAIL; fixed (c3807c63 grid refresh mid-carry); owner PASS OCT24, R5 1ca2faec, R6 26186464 A08 and again R11 3634d1e6 H23 ('it works'). Place key changed to A after R11 (6f62b69e) and Gen II Actions now offer Move (dd69bc93).
- **Repair commits:** c3807c63 WIP: fix(gen2): PC box Move refreshes the grid mid-carry (NEW-04); 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels
- **Automated tests:** tests/test_gen2_box_move_presentation.cpp : Gen II move presentation (CG56); tests/test_gen12_packed_move.cpp : Gen II packed move/cancel
- **Hardware acceptance:** R11 3634d1e6 H23; re-verify 066e1db9 R12 H03 · R12 item H03
- **Next action:** H03 Gen II between boxes with A to place.
- **Owner** (B0553): > Gold/Silver/Crystal PC box pickup/drop/move between slots/boxes not completing. **Gen I and Gen III move work—do not rewrite them.**
- **Owner** (E0022): > it works, can we fix for all gens when we pick up a pokemon can we change the drop control to A, so it would be moving the pokemon to the spot you want and pressing and and that would set the pop there instead of pressing Y again to drop it, can we also fix the footer and all the help menu and anything else that would need to be changed for this to work
- **Occurrences (11):** R3 62a2f8c3 NEW-04 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-04 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-04 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-04 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-04 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-04 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A08 · NEW-04 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A08 · NEW-04 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A08 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 H23 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-CRY-006` Gen IV Storage view save pane: Y carry stripped, drop at Boxes-tab cursor, carry not drawn (Part 9 #8)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV · **Area:** move-carry · **Category:** defect
- **Last owner result:** none
- **Basis:** Current overlay drops in the grid the Pokemon came from, Boxes or the Storage save pane (src/UI/ClassicPackedMoveOverlay.inc:1015-1017 cursorBox/cursorSlot). Fixed on lane S (83e3d004/60243fcf). No owner result for Part 9 #8 in R3 results or later; not on R12 checklist.
- **Repair commits:** 83e3d004 Part 9 item 8: test the Gen IV Storage save-pane Move / Y carry (130 checks); 60243fcf Thread S handoff: Part 9 item 8, pin folds, CG13 hand-off to P
- **Automated tests:** tests/test_gen4_storage_carry.cpp : Storage save-pane pickup, drop at visible cursor, drawn carry
- **Hardware acceptance:** none (not on R12 checklist)
- **Next action:** Add a Gen IV Storage-view carry step (Y pick, A place, B cancel) to the next checklist.
- **Occurrences (3):** R3 62a2f8c3 Part 9 #8 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 0faf7e9c Part 9 #8 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 0faf7e9c Part 9 #8 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-CTL-001` Left Stick gives no navigation input; held stick repeat (issue #19); D-pad/stick parity

- **Status:** Hardware Accepted · **Gen:** app · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R11/3634d1e6/H18: PASS
- **Basis:** Owner reported Left Stick working on 0ea98cc1 (issue #19 comment 2026-09-06, D0265; #19 closed). Latest owner result R11 3634d1e6 H18 'D-pad and joystick parity' PASS (E0017) and H19 'B closes nested menus' PASS (E0018). Current source: include/UI/NavigationRepeat.h:41-158 (AnalogNavigation converts left stick with hysteresis into the shared D-pad repeat engine, CG10). NavigationRepeat.h has no commits between 3634d1e6 and 066e1db9.
- **Repair commits:** 16ecd415 fix(ui): use PokeBank analog navigation in keyboard; 98f57b82 fix: restore Gen III analog menu navigation; 57fb0ace fix(gen1): pass held keys and the stick to the Gen I read-only View
- **Automated tests:** tests/test_navigation_repeat.cpp : first move immediate, held repeat after INITIAL_DELAY_MS then every REPEAT_INTERVAL_MS, never a burst (CG10 single repeat engine)
- **Hardware acceptance:** 3634d1e6 R11 H18 (D-pad and joystick parity)
- **Next action:** None required; keep H18 parity in the R12/R13 smoke set. Gen I long-list stick repeat request (A0534, 6ecd9c28) is subsumed by H18.
- **Owner** (A0058): > FAIL — no input/action
- **Owner** (D0265): > User report: Left Stick now works on hardware. Right Stick remains intentionally deferred and is not part of this issue.
- **Occurrences (78):** PRE 3be4de6b FAIL [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b LEFT STICK + HOLD FAIL [issue #8 (comment 2026-09-02)]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 6ecd9c28 1 n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; PRE G-1 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE A12 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE C18 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE D15 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE G02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE navigation NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE CG10 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE navigation PASS [docs/qa/GEN1_5_MATRIX.md]; PRE Controller n/a [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE 0ea98cc1 PASS [issue #19 (comment 2026-09-06)]; PRE 3 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; R1 7fea1797 N01 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 N02 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 N04 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 N01 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 N02 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 N04 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 N01 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 N02 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 N04 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 N01 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 N02 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 N04 PASS [tests/test_hw_round1_regression_gate.py]; R3 62a2f8c3 N01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 N02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 N04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R8 6ec5ccb9 H02 n/a [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]; R11 3634d1e6 H18 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H19 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-CTL-002` Inherited L/R account switching and inert ZL/ZR (controller normalization #26); L/R Profile in full Games browser

- **Status:** Hardware Accepted · **Gen:** app · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R3/62a2f8c3/C04: PASS
- **Basis:** R1 7fea1797 C04 'L/R behave correctly' PASS (Home changes games, no accidental save opening) and R3 62a2f8c3 C04 PASS (B0487). Current Product Home footer src/UI/SaveSelectScreen.cpp:5314-5315 'L/R Change Game'; Games browser keeps 'L/R Profile' only when >1 user (SaveSelectScreen.cpp:4855, 4892), documented limitation B0164. Guard: tests/test_home_input_guard.cpp (L/R/ZL/ZR never quit). R11 SaveSelectScreen commits (aace6021, c21d3747, 98b6deaf) do not touch L/R routing.
- **Repair commits:** (issue #26 normalization; P0 #320 Home input guard in 7fea1797)
- **Automated tests:** tests/test_home_input_guard.cpp : A,B,R,L,ZL,ZR,X, D-pad never quit; only Y in the visible decision; tests/test_hw_round1_regression_gate.py : C04 frozen guards
- **Hardware acceptance:** 62a2f8c3 R3 C04
- **Next action:** None; optional: confirm Games-browser L/R Profile on a 2-profile console (never explicitly checked).
- **Owner** (A0128): > switches accounts
- **Owner** (A0072): > account switching in tested context
- **Occurrences (26):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b n/a [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; R1 7fea1797 C04 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C04 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 C04 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C04 PASS [tests/test_hw_round1_regression_gate.py]; R2 C04 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C04 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C04 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 0faf7e9c Part 9 #9 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-09 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 AUD-09 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CTL-003` Inherited Boxes face buttons: X sorts by Dex, Y opens Menu/Move/Multi; X/Y inert on Select Game

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** PRE/3be4de6b: FAIL (inherited behaviour); no owner result for the current X mapping
- **Basis:** Boxes X/Y have been redefined twice; the inherited Dex-sort X is gone. Current: X = Export (occupied) / Import (empty) via SharedPokemonEditorContract boxActivation (dd69bc93, Round 11 Extra 4), Y = pick up to move in Gen I-IV (include/UI/ClassicHelpPresentation.h Page::Pokemon rows, Round 11 H23). The Y pick-up passed R11 H23 (move-carry area) but X semantics are new in 066e1db9 and have no owner result.
- **Repair commits:** dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V; 6f62b69e H31 / H23: one shared visual Clone carry for Gen I-V; A places, B cancels
- **Automated tests:** tests/test_classic_help_accuracy.cpp : Help names only controls that act; Pokemon page names X Export/Import in every generation; tests/test_footer_help_handlers.py : every footer key token has a handler in that screen's input layer
- **Hardware acceptance:** 066e1db9 R12 H05 (X Export/Import) and H03 (Y pick-up) · R12 item H05
- **Next action:** Owner runs R12 H05 (X on occupied/empty slot in each generation) and H03.
- **Owner** (A0070): > Dex-sort behavior
- **Owner** (A0071): > inherited Menu / Move / Multi
- **Occurrences (12):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]

### `PBX-CTL-004` Right Stick has no action (future model rotation, #25)

- **Status:** Missing Feature · **Gen:** app · **Area:** help-footer-controls · **Category:** request
- **Last owner result:** PRE/3be4de6b: no action observed (not a failure)
- **Basis:** No right-stick handling exists outside the left-stick AnalogNavigation path (include/UI/NavigationRepeat.h:73-158 samples the left stick only). Owner: 'Right Stick remains intentionally deferred' (issue #19, D0265) - deferral, not a recorded won't-support decision.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner decision: keep deferred past v1.0 or schedule #25 (model rotation).
- **Owner** (D0265): > User report: Left Stick now works on hardware. Right Stick remains intentionally deferred and is not part of this issue.
- **Occurrences (4):** PRE 3be4de6b n/a [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]

### `PBX-CTL-005` Bottom controller-hint bar low contrast in OLED Black/Dark; hints should stand out (coloured button glyphs)

- **Status:** Hardware Accepted · **Gen:** app · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R3/62a2f8c3/R2-BUG-013: PASS (note-qualified)
- **Basis:** Coloured controller glyph badges replaced plain text (R2-BUG-013: 11b185d4, 678627f, 796ba212). Owner R3 62a2f8c3 R2-BUG-013 'Colored controller buttons in Help' PASS with note 'Some windows still need light cleanup' (B0540; polish tracked under PBX-CTL-010). Contrast in OLED Black/Dark specifically was not re-scored after the PRE PARTIAL. Also holds the PRE '+ / -' and Help-readability acceptance rows (A0024, A0401-A0404): + Options and - Help PASSed on 3be4de6b (A0007/A0008/A0039/A0040).
- **Repair commits:** 11b185d4 feat(ui): GlyphInstruction splits help rows and instruction prose into button badges (R2-BUG-013); 678627f fix(ui): R2-BUG-013 Gen I-III Inventory Help draws button glyphs; 796ba212 feat(ui): button badges in help and modal text; provider names in sessions; profile focus kept
- **Automated tests:** tests/test_glyph_instruction.cpp : every controller button named in Help/modal text is a coloured badge, never plain A/B/X/Y text
- **Hardware acceptance:** 62a2f8c3 R3 R2-BUG-013
- **Next action:** Optional: owner glance at footer contrast in OLED Black and Dark during R12 (H01 invites two themes).
- **Owner** (A0195): > NEEDS POLISH
- **Owner** (B0540): > Some windows still need light cleanup.
- **Occurrences (19):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 3be4de6b PARTIAL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; R3 62a2f8c3 R2-BUG-013 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-013 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-013 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-013 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CTL-006` Gen I editor step navigation confusing (R bumper as next step); L/R sections, A edit, B back

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** PRE/69668bc8: PASS
- **Basis:** Owner accepted 'L/R panel navigation' on 69668bc8 (GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14, A0546). Gen I editor overlay changed in Round 11 (src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc 121 lines, Cleanup3, UX.inc in 3634d1e6..066e1db9).
- **Repair commits:** (Gen I UX cleanup series, PR #66, accepted at 69668bc8)
- **Automated tests:** tests/test_gen1_view_navigation.cpp : Gen I View navigation keys
- **Hardware acceptance:** 69668bc8 (PRE Gen I acceptance); recheck on 066e1db9
- **Next action:** Include a Gen I Edit L/R panel pass in the R12 smoke (H18).
- **Occurrences (2):** PRE 574d604b 2 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]

### `PBX-CTL-007` Gen I/II Create footer 'X Stage Add' wording and Gen II box A/X control parity with Gen I

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** PRE/cfb70357: PASS
- **Basis:** Accepted on cfb70357 (PR #68 body: 'cfb70357 accepted'). The X key on the box grid was redefined in 066e1db9 (dd69bc93: X = Export/Import, no longer Add), so the accepted footer wording is superseded.
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions; dd69bc93 Round 11 X-IMPEXP + ACTIONS (supersedes X Add)
- **Automated tests:** tests/test_footer_help_handlers.py : footer tokens match handlers
- **Hardware acceptance:** cfb70357 (PRE); recheck 066e1db9 R12 H05 · R12 item H05
- **Next action:** Verify Gen I/II box footers in R12 H05.
- **Owner** (D0296): > - **Create footer wording:** `X Stage Add` should become user-facing `X Add` (or equivalent concise PokeBank wording). Keep staging semantics under the hood.
- **Owner** (D0297): > - **Gen II box/footer parity:** live GSC box screen still does not use the same A/X control language as Gen I. Empty-slot entry is still behaviorally different.
- **Occurrences (2):** PRE 501093ae n/a [PR #68 (comment 2026-09-15)]; PRE 501093ae FAIL [PR #68 (comment 2026-09-15)]

### `PBX-CTL-008` Gen V editor navigation differs from Gen I-IV; moving focus changes values (P1)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** help-footer-controls · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** PRE/35cbf163/P1: FAIL
- **Basis:** Gen V View/Edit/Create runs through the one shared editor surface (5d8b833e, de22e35f, both in 7fea1797). No owner result records this specific behaviour (focus movement not changing values) as PASS; later Gen V edit PASSes (e.g. R6 C12, R11 H24) are indirect. Flagged safety-critical because an unintended value change would alter a Pokemon on Keep.
- **Repair commits:** 5d8b833e refactor(ui): extract the Gen IV editor workspace into one shared editor surface; de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface
- **Automated tests:** (shared editor contract tests; no test named for focus-without-mutation found)
- **Hardware acceptance:** none (not on any checklist); verify on 066e1db9
- **Next action:** Add an explicit Gen V Edit step: move focus across every field with D-pad/stick, then Review shows no change.
- **Occurrences (1):** PRE 35cbf163 P1 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]

### `PBX-CTL-009` P0 #320 global input regression: A exits without warning, R opens current save, Gen II Create A/B misrouted

- **Status:** Hardware Accepted · **Gen:** app, II · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R3/62a2f8c3/C09: PASS (C01-C09 all PASS R1/7fea1797)
- **Basis:** Owner marked C01-C09 PASS on R1 7fea1797 (issue #320 comment 2026-10-17, D0236). R3 62a2f8c3: C09 PASS (B0492), C04 PASS; C01 FAIL only for the Gen V Choose-Save behaviour (tracked as PBX-CTL-011). Home guard source: include/UI/HomeQuitConfirm.h:19-37 (decide(): only Y commits, A/B stay, A+Y stays).
- **Repair commits:** (P0 #320 repairs landed in 7fea1797; Home input guard)
- **Automated tests:** tests/test_home_input_guard.cpp : no single non-Y button quits; opt-in input trace; tests/test_hw_round1_regression_gate.py : C01-C09 frozen guards
- **Hardware acceptance:** 7fea1797 R1 C01-C09; 62a2f8c3 R3 C02/C04/C09
- **Next action:** None; C01 regression item for Gen V tracked separately.
- **Owner** (D0216): > the controls for the whole app now have changed and they get fucked up and A exits now without warning whenit should never do that hell a advances in the app its does go back to quit so it makes no sense this is bugged, hell i just pressed the R trigger and it opened the current save, like what the hell i cant find bugs when i cant even navigate the app
- **Owner** (D0236): > Owner explicitly marked C01–C09 PASS, including A selection, B guarded quit, launch confirmation, L/R navigation, Gen II Create A/B, blocked background popup input, deliberate Y confirmation and second controller.
- **Occurrences (21):** PRE a0633b6d FAIL [issue #320 (body 2026-10-16)]; PRE a0633b6d FAIL [PR #307 (comment 2026-10-16)]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; R1 7fea1797 C01–C09 PASS [issue #320 (comment 2026-10-17)]; R1 7fea1797 C01 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C01 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 C01 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C01 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 C09 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C09 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 C09 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C09 PASS [tests/test_hw_round1_regression_gate.py]; R2 C01 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C01 n/a [docs/hardware/RETEST_e10f307f.md]; R2 C09 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C09 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C01 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C09 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C09 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R7 e88957f2 A28 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-CTL-010` E05 controller glyphs and footers consistent app-wide; Help/footer names only controls that act (incl. Inventory Help wording/title/fit)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R11/3634d1e6/H03: PASS (prior R6/26186464/B04: FAIL)
- **Basis:** Owner FAILs: R3 B0507, R5 C0168 (Game Browser Controls, Party footers, stale Review lines), R6 26186464 B04 (Inventory Help: 'X Add remains available', + described as Options, Gen V titled 'Game Browser Controls'). R7 repairs: src/UI/TrainerViewScreenBase.inc:5593-5594 Bag Help titled InventoryHelp::kTitle 'Inventory Controls' in every generation; include/UI/InventoryHelp.h:7-9,28. Latest owner result R11 3634d1e6 H03 'Inventory Controls help fits' PASS (E0002). CAVEAT: the non-inventory screens named in R5 C0168 have no explicit later item-level PASS (R6 B04 FAIL text lists inventory problems only). Recheck needed: InventoryHelp.h ZR row text lengthened in 642a7931 and ClassicHelpPresentation.h Pokemon page rewritten (3cacfa98, dd69bc93, 6f62b69e) after 3634d1e6.
- **Repair commits:** 88f974da (R4: glyph choices in Bag dialogs; redundant Help footer removed); edff856e Round 5 4D: Gen I-V Help names only controls that act there; 642a7931 Top Up / Add Missing: discoverable Scope (InventoryHelp ZR row); 3cacfa98 Round 11 integration: Pokemon Help keeps 10 rows
- **Automated tests:** tests/test_classic_help_accuracy.cpp : each Help claim tied to the code that handles it; tests/test_footer_help_handlers.py : footer/Help key tokens have handlers; tests/test_glyph_instruction.cpp : coloured badges; CG05/CG36/CG29/CG63 (tests/test_qa_cross_gen_contract.py)
- **Hardware acceptance:** 3634d1e6 R11 H03; recheck 066e1db9 R12 H18 (smoke includes Bag Help) · R12 item H18
- **Next action:** R12 H18: open Bag Help in Gen I-III and IV/V and confirm the longer ZR line fits; glance at Game Browser Controls and Party footers (R5 C0168 screens) to close the caveat.
- **Owner** (B0507): > Some windows and Help need updated glyphs/unused buttons removed.
- **Owner** (C0168): > Controller glyphs are missing or inconsistent: Game Browser Controls, Party footers, and stale Review lines in Game Browser Help.
- **Owner** (C0315): > Problems with Inventory Help and glyphs:<br>• Gen I/II/III Inventory Help has stale wording such as "X Add remains available".<br>• Bag Help describes + as Options although + opens Review.<br>• Gen V Bag Help is titled "Game Browser Controls" but should be "Inventory Controls".<br>• Gen IV/V Inventory is closer to the preferred layout.
- **Occurrences (29):** PRE CG05 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 E05 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E05 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 E05 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E05 PASS [tests/test_hw_round1_regression_gate.py]; R2 E05 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f E05 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 E05 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E05 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 E05 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E05 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 4 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E05 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec E05 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec E05 FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P4 n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B04 · E05 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 B04 · E05 FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B04, B03 follow-up n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B03 · B08 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 B02 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 C01 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R9 d6a20a2a H03 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H03 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H03 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; REQ Global controls n/a [docs/UI_FLOW.md]

### `PBX-CTL-011` Gen V Home A opens a Choose Save popup instead of the assigned save (C01 regression, dup NEW-16)

- **Status:** Hardware Accepted · **Gen:** V · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R6/26186464/A18 (NEW-16): PASS
- **Basis:** Same defect as NEW-16 (open-save area): owner PASS OCT24 c60d4da0 NEW-16, R5 1ca2faec NEW-16 PASS, R6 26186464 A18 'Gen V Home A opens the assigned save' PASS. R7 A28 (D0027) retest had no recorded result. Guard CG57.
- **Repair commits:** (app58 retained; assigned Gen V direct open, Oct 24 repair 8804f8f8)
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG57 Gen V Home A opens the assigned source
- **Hardware acceptance:** 26186464 R6 A18
- **Next action:** None; owned jointly with open-save NEW-16.
- **Owner** (B0484): > Gen V Home A opens Choose Save popup instead of current already-assigned save directly; duplicate with NEW-16.
- **Occurrences (1):** R3 62a2f8c3 C01 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-CTL-012` Help closes with B; remove redundant '- Close/Help' footer everywhere

- **Status:** Hardware Accepted · **Gen:** app · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** OCT24/c60d4da0/E05: PASS
- **Basis:** R3 62a2f8c3 NEW-01 PASS (B works) with polish request; redundant Help footer removed in 88f974da (R4 C0026); owner OCT24 c60d4da0 E05 'Footer glyphs and Help close on B' PASS (C0079/C0368). Current Help close: src/UI/TrainerViewScreenBase.inc kCloseHelp 'B / -   Close this Help' (include/UI/ClassicHelpPresentation.h:24); no '{"-","Close"}' footer token in src/include.
- **Repair commits:** 88f974da (R4) redundant Help footer removed
- **Automated tests:** tests/test_hw_round1_regression_gate.py : NEW-01 and E05 (Oct 24) guards
- **Hardware acceptance:** c60d4da0 OCT24 E05
- **Next action:** None.
- **Owner** (B0550): > B works now; remaining minor fix is remove redundant \`- Close/Help\` footer everywhere.
- **Occurrences (3):** R3 62a2f8c3 NEW-01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-01 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-01 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-CTL-013` Remove obsolete 'Touch: Back' footer text while keeping touch Back (NEW-02)

- **Status:** Hardware Accepted · **Gen:** all · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R6/26186464/A06: PASS
- **Basis:** R3 FAIL (B0551) -> 88f974da -> OCT24 PASS, R5 1ca2faec PASS, R6 26186464 A06 PASS (C0286). No 'Touch: Back' / 'Touch:Back' string in current src/ or include/.
- **Repair commits:** 88f974da (R4) Obsolete Touch:Back text removed
- **Automated tests:** tests/test_hw_round1_regression_gate.py : NEW-02 guard (CG37)
- **Hardware acceptance:** 26186464 R6 A06
- **Next action:** None.
- **Owner** (B0551): > Remove obsolete footer across View/Overview and all windows while retaining touch functionality.
- **Occurrences (9):** R3 62a2f8c3 NEW-02 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-02 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-02 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-02 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-02 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-02 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A06 · NEW-02 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A06 · NEW-02 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A06 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-CTL-014` 4D: Gen I-IV Help claims '+ Review' where + does something else; Help must name real controls

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV · **Area:** help-footer-controls · **Category:** defect
- **Last owner result:** R6/26186464/A31: PASS
- **Basis:** Fixed edff856e (Round 5); owner R5 1ca2faec 4D-HELP PASS and R6 26186464 A31 PASS (C0311). include/UI/ClassicHelpPresentation.h Pokemon/Party pages changed after 3634d1e6 (Actions order, X Export/Import, Y pick-up/carry rows; 3cacfa98, dd69bc93, 6f62b69e), so the R12 build needs a Help recheck.
- **Repair commits:** edff856e Round 5 4D: Gen I-V Help names only controls that act there
- **Automated tests:** tests/test_classic_help_accuracy.cpp : every Help claim tied to its handler; no vague 'Tools'
- **Hardware acceptance:** 26186464 R6 A31; recheck 066e1db9
- **Next action:** Add 'open Help on Pokemon/Party page per generation' to R12 smoke (not explicit on the R12 list).
- **Occurrences (6):** R5 1ca2faec 4D Help n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec 4D-HELP PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec 4D-HELP PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A31 · 4D-HELP n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A31 · 4D-HELP PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A30 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-CTL-015` Extra 4: X = Export on a Pokemon / Import on an empty slot, app-wide footers and Help

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** help-footer-controls · **Category:** request
- **Last owner result:** none
- **Basis:** dd69bc93: boxActivation(occupied, A, X, caps) returns Actions / ImportFile / ExportFile in Gen I-V; footers name 'X Export / Import' only where X works; Help Pokemon page row 'X   Export to file; on an empty slot: Import from file' (include/UI/ClassicHelpPresentation.h Page::Pokemon). Only in 066e1db9, never device tested.
- **Repair commits:** dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_classic_help_accuracy.cpp : Pokemon page names X Export/Import in every generation
- **Hardware acceptance:** 066e1db9 R12 H05 · R12 item H05
- **Next action:** Owner runs R12 H05.
- **Owner** (E0055): > is it possible to change the contorls in the save editored when in the box, can we get rid of the X to add and change it to import/export instead like if the cursor is on a pokemon X would say export but it its on a blank space it would say import, makes this app wide for all gens and change the footer as needed, and change the help menus app wide as need as well.
- **Occurrences (2):** R11 3634d1e6 Extra 4 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 4 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]

### `PBX-DSC-001` Full installed-game / FRLG identity detection matrix left incomplete (native discovery PARTIAL)

- **Status:** Needs Investigation · **Gen:** app · **Area:** save-discovery · **Category:** request
- **Last owner result:** PRE/3be4de6b: PARTIAL
- **Basis:** PRE 3be4de6b: native Switch discovery 'PASS / PARTIAL based on sampled titles'; full matrix NOT TESTED. No later round tests native-title discovery. Unknown: whether the full matrix matters for the Gen I-V v1 scope.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Decide scope; otherwise close as out of scope.
- **Occurrences (6):** PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PARTIAL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; R8 6ec5ccb9 n/a [docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md (Known gaps)]

### `PBX-DSC-002` Emulator save discovery for RetroArch / mGBA / Tico / DraStic / melonDS without moving files (legacy GB/GBC/GBA discovery)

- **Status:** Hardware Accepted · **Gen:** I, II, III, IV, V · **Area:** save-discovery · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H20: PASS
- **Basis:** PRE 'NOT IMPLEMENTED' (A0213) -> implemented (issue #6/#85); owner PASSes for RSE (46e0c161), DraStic Platinum .dsv, multi-provider Save Instances (d49efd0c/00ee7a6e), and R11 3634d1e6 H20 'Emulator/provider identification' PASS (E0019). No discovery code changed after 3634d1e6 (git log src/Legacy, Gen4/Gen5 SourceDiscovery: 0 commits).
- **Automated tests:** QA matrix discovery/linking/provider/identity cells (20 games)
- **Hardware acceptance:** 3634d1e6 R11 H20
- **Next action:** None.
- **Owner** (A0213): > NOT IMPLEMENTED / not detected
- **Occurrences (13):** PRE 3be4de6b deferred [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  requirement [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE discovery PASS [docs/qa/GEN1_5_MATRIX.md]; PRE linking PASS [docs/qa/GEN1_5_MATRIX.md]; PRE provider PASS [docs/qa/GEN1_5_MATRIX.md]; PRE identity PASS [docs/qa/GEN1_5_MATRIX.md]; PRE discovery / relaunch NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE 46e0c161 PASS [issue #6 (comment 2026-09-10)]; PRE 46e0c161 PASS [issue #6 (comment 2026-09-10)]; PRE  PASS [issue #84 (body 2026-09-28)]; PRE d49efd0c PASS [issue #85 (comment 2026-09-28)]; PRE 00ee7a6e PASS [issue #85 (comment 2026-09-28)]; R11 3634d1e6 H20 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-DSC-003` Gen III discovery release hint taken from a raw substring of the whole path (QA-F6)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** III · **Area:** save-discovery · **Category:** defect
- **Last owner result:** none
- **Basis:** Fixed 82032a9 (docs/qa/FINDINGS.md); host-only.
- **Repair commits:** 82032a9 (QA-F6)
- **Hardware acceptance:** none (host-only)
- **Next action:** None.
- **Occurrences (3):** PRE QA-F6 engineering-finding [docs/qa/FINDINGS.md]; PRE QA-F6 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE identity.source n/a [docs/qa/GEN1_5_MATRIX.md]

### `PBX-DSC-004` FRLG opened an older save instead of the current in-game save

- **Status:** Hardware Accepted · **Gen:** III · **Area:** save-discovery · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** R11/3634d1e6/H20: PASS
- **Basis:** 5d3e5e23 FAIL; ea0b806b removed the arbitrary Main Save guess; owner 'FIRERED CURRENT IN-GAME SAVE SELECTION: PHYSICAL PASS' (D0267); later R11 H20 PASS (save instances distinct, no unrelated save attached).
- **Repair commits:** ea0b806b legacy: bind FRLG sources to profiles and expose diagnostics
- **Hardware acceptance:** 3634d1e6 R11 H20
- **Next action:** None.
- **Owner** (A0889): > DEVICE ACCEPTED: NO
- **Occurrences (3):** PRE 5d3e5e23 FAIL [docs/history/CURRENT_STATUS_PRE_LEAFGREEN_2026-09-09.md]; PRE 5d3e5e23 1 FAIL [docs/history/CURRENT_STATUS_PRE_LEAFGREEN_2026-09-09.md]; PRE ea0b806b PASS [issue #6 (comment 2026-09-09)]

### `PBX-DSC-005` Profile-scoped save assignment: FRLG sources appeared under both profiles; LeafGreen assignment could not be saved

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** III, V · **Area:** save-discovery · **Category:** defect
- **Last owner result:** R3/62a2f8c3/AUD-07: PASS (assignment); isolation: none
- **Basis:** ea0b806b introduced profile-scoped bindings (src/Legacy/LegacySourceBindings.cpp:118,145,306-310). Assignment later accepted (Gen III read-only #48 PRE; R3 62a2f8c3 AUD-07 Gen V assigned to profile PASS), but no owner record ever tests a SECOND profile (GEN14 B3 / GEN15 B06 unfilled), so the cross-profile isolation fix is unverified on hardware.
- **Repair commits:** ea0b806b legacy: bind FRLG sources to profiles
- **Automated tests:** tests/test_save_instance_model, test_gen5_source_e2e (per B0602)
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add a two-profile isolation check if the owner has a second profile.
- **Owner** (D0269): > LEAFGREEN PROFILE ASSIGNMENT: PHYSICAL FAIL
- **Occurrences (7):** PRE 5d3e5e23 2 FAIL [docs/history/CURRENT_STATUS_PRE_LEAFGREEN_2026-09-09.md]; PRE ea0b806b FAIL [issue #6 (comment 2026-09-09)]; PRE B3 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE B06 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 0faf7e9c Part 9 #7 engineering-finding [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-07 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 AUD-07 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-EDT-001` Inherited PKSE mutation UI reachable; edits applied to save outside staged copy

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** pokemon-edit · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H24: PASS
- **Basis:** Gen I staged-only source-safe editing accepted 69668bc8 (A0554); later every generation edits a staged draft (POKEMON_EDITOR_UI_CONTRACT: 'Create/Edit operate on staged copies/drafts'). Latest owner R11 3634d1e6 H24 'Edit and discard' PASS (E0023). R11 changed editor surfaces (f2cf5954, d1954551) so re-verify.
- **Repair commits:** #23 lock inherited live-save mutation UI; 69668bc8 Gen I staged editor
- **Automated tests:** tests/test_qa_cross_gen_contract.py : staged-only edit contracts; docs/qa/SAVE_SAFETY_CHAIN.md Edit stage (qa-matrix reparse)
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** R12 H17 (Review/Discard/Export Copy; original unchanged).
- **Owner** (A0107): > opens inherited editable view
- **Owner** (A0179): > IV edit saved in app flow, reopened, restored; inherited editor must be blocked or copy-based.
- **Occurrences (7):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; R11 3634d1e6 H24 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-EDT-002` Gen I boxed staged editor UX (one-screen workspace, Add/Edit one visual language, Left Stick lists)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** pokemon-edit · **Category:** request
- **Last owner result:** R11 3634d1e6 H21: PASS
- **Basis:** DEVICE ACCEPTED on 69668bc8 (A0540, D0287: capability-driven three-panel workspace); latest Gen I editor owner PASS R11 3634d1e6 H21 (E0020). R11 f2cf5954 changed Gen1PokemonEditorOverlay*.inc (toasts) and eb7fe8c9 the picker.
- **Repair commits:** 69668bc8 Gen I editor acceptance; f2cf5954 Gen I-V: no toast after routine edits
- **Automated tests:** tests/test_edit_feedback.cpp : routine edits post no toast; tests/test_gen12_dv_reroll.cpp : Gen I DV reroll
- **Hardware acceptance:** 066e1db9 R12 H13 · R12 item H13
- **Next action:** R12 H13 on a Gen I save (reroll/shiny without toast).
- **Owner** (A0520): > FAIL / RETEST REQUIRED
- **Owner** (D0284): > The one-screen Gen I editor is functionally working on real hardware, but the exact production artifact `PokeBank-NX-Gen1-Editor-c3ad2be6.nro` is **not DEVICE ACCEPTED**.
- **Occurrences (11):** PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 574d604b FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 574d604b 7 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE  n/a [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE  n/a [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 6ecd9c28 n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; PRE  n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; PRE c3ad2be6 PARTIAL [issue #62 (comment 2026-09-13)]; PRE 6ecd9c28 n/a [issue #62 (comment 2026-09-13)]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; R11 3634d1e6 H21 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-EDT-003` Party editing (Gen I-III staged editors were box-only; party mutation deferred)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV · **Area:** pokemon-edit · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R6 26186464 A21·NEW-20: PASS (actions-menu)
- **Basis:** Gen IV Party+Box staged View/Edit accepted 84dae170 (D0314); Gen I-III Party routed to native staged edits by c81853da (include/UI/ClassicPartyActions.h); owner PASS R6 26186464 A21·NEW-20 'Gen I–III Party Actions popup' (actions-menu record C0301). R11 dd69bc93 rewrote the box/Actions capability model. NOTE: docs/POKEMON_EDITOR_UI_CONTRACT.md:99 still says 'Party mutation remains deferred' — stale vs code.
- **Repair commits:** c81853da feat(editor): route classic Party Actions to native staged edits; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model
- **Automated tests:** host-ui party actions tests (NEW-20 contracts)
- **Hardware acceptance:** 066e1db9 R12 H05 · R12 item H05
- **Next action:** R12 H05 plus a Party edit -> Keep -> Export Copy in H17; update the stale contract line.
- **Owner** (B0161): > Documented limitation (not a regression): rty in Gen I–III opens the native read-only View; the Gen I and Gen II staged editors are box-only. To edit, move the Pokémon into a box.
- **Occurrences (5):** PRE 574d604b n/a [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 69668bc8 n/a [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 84dae170 PASS [issue #86 (comment 2026-09-28)]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; REQ Safety invariants n/a [docs/POKEMON_EDITOR_UI_CONTRACT.md]

### `PBX-EDT-004` Shared Create/Edit/View editor across Gen I–V (owner rule: one PKSE interface, one editor, capability-gated fields)

- **Status:** Fixed in Code — Awaiting Hardware (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** pokemon-edit · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H29: FAIL (Gen V parity gap)
- **Basis:** Gen I/II accepted cfb70357 (D0303), Gen III 996e6aa4 (D0306), Gen IV 84dae170 (D0314); Gen V ran on shared surface since de22e35 but the last cross-gen parity gap (Gen V Shiny via PID dialog, R11 H29 FAIL) was fixed only in f2cf5954 (src/UI/Gen5SharedReadOnlySurface.inc:632-650). Host matrix PASS rows are not hardware acceptance.
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; f0419926 test(ui): owner's 14-point shared Pokemon editor contract; f2cf5954 Gen V Shiny toggles inline
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG01b/CG02/CG11/CG16/CG17 cross-gen UI contract; tests/test_ui_editor_input_host.cpp : shared editor input
- **Hardware acceptance:** 066e1db9 R12 H13 + H18 · R12 item H13
- **Next action:** After R12 H13/H18, record a per-generation shared-editor acceptance line.
- **Owner** (A0928): > Only generation-specific data, restrictions and capabilities may differ.
- **Owner** (D0302): > - **Shared future field architecture:** owner likes the richer PKSE-style left/middle panel organization (Ability, Friendship, PID, EXP, Egg, met/origin, Ball, Nature, IV/EV/stat, etc.) as a reference for the universal editor, but fields must remain capability-gated by exact generation/game. For Gen I/II, retain DVs/Stat Exp rather than IV/EV and hide unsupported modern fields.
- **Occurrences (13):** PRE 501093ae n/a [PR #68 (comment 2026-09-15)]; PRE 35cbf163 n/a [docs/handoffs/GEN5_9_PROGRESS.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE fields PASS [docs/qa/GEN1_5_MATRIX.md]; PRE CG11 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE CG16 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE CG17 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE Edit n/a [docs/qa/SAVE_SAFETY_CHAIN.md]; PRE CG01b PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE cfb70357 PASS [PR #68 (body 2026-09-14)]; PRE 996e6aa4 PASS [PR #77 (body 2026-09-19)]; REQ Permanent implementation rule n/a [docs/POKEMON_EDITOR_UI_CONTRACT.md]; REQ Current priority n/a [PROJECT_STATUS.md]

### `PBX-EDT-005` Gen IV OT / Trainer ID should be editable metadata (SID read-only)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** PRE acfca273: FAIL
- **Basis:** aff41765/66312f10 on PR #92; current src/UI/Gen4SharedPokemonSurface.inc:460-516 lists OT and Trainer ID as controls, SID 'visible but not a control' (line 516). The generic Oct 8 '100% pass' on 0760334f (contains aff41765) did not name this row, so not counted.
- **Repair commits:** aff41765 fix(gen4): align create editor with classic parity; 66312f10 test(gen4): fix parity contract source assertion
- **Automated tests:** Gen IV parity contract test (66312f10)
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add a Gen IV OT/TID edit -> Review check to a future round (R11 H25 touched OT only incidentally).
- **Owner** (D0321): > - OT / Trainer ID should follow classic editor behavior as editable Pokémon metadata; SID remains read-only.
- **Occurrences (1):** PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]

### `PBX-EDT-006` Gen V editor parity (Nature/Gender/Shiny/Ability/PID duplicated, raw-ID controls, oversized menus)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R11 3634d1e6 H08: PASS
- **Basis:** Repaired de22e35/2af2563/6efd8c6/f041992. Latest Gen V editor owner results: R6 26186464 A16 NEW-13 ability names PASS, R11 3634d1e6 H08 Gen V Gender inline PASS (E0007). Gen V surface changed in R11 (f2cf5954 Shiny inline, d1954551 names).
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; 2af25639 PID-coordinated Gender, Shiny and Ability; 6efd8c60 Change PID dialog; f0419926 owner 14-point contract
- **Automated tests:** tests/test_qa_cross_gen_contract.py : Gen V shared editor contract
- **Hardware acceptance:** 066e1db9 R12 H13/H14 · R12 item H13
- **Next action:** R12 H13+H14 on a Gen V save.
- **Owner** (A0927): > Owner tested 35cbf163 on Switch; Gen V failed hardware acceptance on editor parity.
- **Occurrences (6):** PRE 35cbf163 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE 35cbf163 P2 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE 35cbf163 P4 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE 35cbf163 P6 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE 35cbf163 P7 n/a [docs/handoffs/GEN5_9_PROGRESS.md]; R11 3634d1e6 H08 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-EDT-007` Gen V PID replay protects gender link only when ability link holds (QA-F4)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** none
- **Basis:** bc838152 'PID replay protects the gender and ability links independently'; Gen V Gender/Shiny now also re-verified by verifiedPidProbe (src/UI/Gen5SharedReadOnlySurface.inc:625-650). No owner row targets PID replay.
- **Repair commits:** bc838152 fix(gen5): PID replay protects the gender and ability links independently (QA-F4)
- **Automated tests:** QA-F4 regression in tests (docs/qa/FINDINGS.md)
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Covered indirectly by R12 H13 (Gen V Shiny keeps gender/nature/ability); no dedicated row needed.
- **Owner** (A0788): > Application QA finding (Thread 4), severity Low.
- **Occurrences (2):** PRE QA-F4 n/a [docs/qa/FINDINGS.md]; PRE QA-F4 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]

### `PBX-EDT-008` Gen IV Unown letter (form) cannot be edited (QA-F7)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** none
- **Basis:** f17926f9 'Unown's 28 stored letters are editable' (ancestor of all R1+ candidates). No Gen IV Unown owner row exists.
- **Repair commits:** f17926f9 fix(gen4): Unown's 28 stored letters are editable (QA-a)
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Optional: add Gen IV Unown form edit to a later checklist.
- **Owner** (A0791): > Application QA finding (Thread 4), severity Low.
- **Occurrences (2):** PRE QA-F7 n/a [docs/qa/FINDINGS.md]; PRE QA-F7 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]

### `PBX-EDT-009` Gen II Create/Edit: one input owner per frame; exit dialog footer its own

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II · **Area:** pokemon-edit · **Category:** request
- **Last owner result:** none
- **Basis:** ae099c77 'Create/Edit footer and input follow the layer that owns the press'. No owner row isolates it (R1 L05 'no editor panel cursor overlaps' PASS is a different behaviour).
- **Repair commits:** ae099c77 fix(gen2): Create/Edit footer and input follow the layer that owns the press
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Fold into R12 H18 Gen II smoke.
- **Owner** (A0941): > Owner campaign (Gen I/II lane) unit; implemented with host tests, not device-tested here.
- **Occurrences (1):** PRE #320 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]

### `PBX-EDT-010` Inline Shiny Yes/No and Pokérus A-cycle (Gen II campaign)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II, all · **Area:** pokemon-edit · **Category:** request
- **Last owner result:** R3 62a2f8c3 E06/E07: PASS
- **Basis:** Owner PASS R3 62a2f8c3 E06 'Shiny toggle' and E07 'Pokérus cycles with A' (B0508/B0509). Shiny paths rewritten in R11 f2cf5954 (src/UI/Gen2PokemonEditorFoundation.inc:424-469 EditFeedback).
- **Repair commits:** e59df023 feat(gen2): inline Shiny/Pokerus/Gender; Data pane text stays inside its pane; f2cf5954 no toast after routine edits
- **Automated tests:** tests/test_edit_feedback.cpp : Shiny applied posts no toast
- **Hardware acceptance:** 066e1db9 R12 H13 · R12 item H13
- **Next action:** R12 H13.
- **Owner** (A0944): > Owner campaign (Gen I/II lane) unit; implemented with host tests, not device-tested here.
- **Occurrences (3):** PRE 10 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R3 62a2f8c3 E06 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 E07 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-EDT-011` No 'Row x/y' counters; list editors draw whole rows above status line

- **Status:** Hardware Accepted · **Gen:** I, all · **Area:** pokemon-edit · **Category:** request
- **Last owner result:** R3 62a2f8c3 E04: PASS
- **Basis:** Owner PASS R3 62a2f8c3 E04 'No obstructive row counters' (B0506).
- **Repair commits:** 9d1d03f9 fix(gen1): list editors draw only whole rows above the status line; no row counters
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG02 no row counters
- **Hardware acceptance:** R3 62a2f8c3 E04
- **Next action:** None.
- **Owner** (A0947): > Owner campaign (Gen I/II lane) unit; implemented with host tests, not device-tested here.
- **Occurrences (3):** PRE 1/5 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE CG02 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R3 62a2f8c3 E04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-EDT-012` Gen II (Crystal) OT Gender opens 3-option popup instead of inline A-cycle

- **Status:** Hardware Accepted · **Gen:** II · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R6 26186464 A11·NEW-07: PASS
- **Basis:** Owner PASS OCT24 c60d4da0, R5 1ca2faec and R6 26186464 A11 (C0291).
- **Hardware acceptance:** R6 26186464 A11
- **Next action:** None.
- **Owner** (B0556): > A cycles native-supported values inline rather than opening 3-option popup; do not fabricate absent native fields.
- **Occurrences (3):** R3 62a2f8c3 NEW-07 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R6 26186464 A11 · NEW-07 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-EDT-013` Ball picker scrollbar crowds rows

- **Status:** Hardware Accepted · **Gen:** all · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R6 26186464 A12·NEW-08: PASS
- **Basis:** Owner PASS OCT24, R5 and R6 26186464 A12 (C0292).
- **Hardware acceptance:** R6 26186464 A12
- **Next action:** None.
- **Owner** (B0557): > Rebalance row padding/scrollbar space.
- **Occurrences (2):** R3 62a2f8c3 NEW-08 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R6 26186464 A12 · NEW-08 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-EDT-014` Ability picker shows 'Slot 1/2' instead of names and effects

- **Status:** Hardware Accepted · **Gen:** III, IV, V · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R6 26186464 A16·NEW-13: PASS
- **Basis:** Owner PASS OCT24, R5, R6 26186464 A16 (C0296). include/UI/SharedAbilityPicker.h / AbilityEffects.h unchanged since 26186464.
- **Hardware acceptance:** R6 26186464 A16
- **Next action:** None.
- **Owner** (B0562): > Show ability names and short accurate effect descriptions; green/red only when meaningful; respect slot restrictions.
- **Occurrences (2):** R3 62a2f8c3 NEW-13 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R6 26186464 A16 · NEW-13 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-EDT-015` Pokérus fails in Gen III/V; Held Item selection unreliable with A/touch

- **Status:** Hardware Accepted · **Gen:** III, V, all · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R6/OCT26 26186464 B01: PASS
- **Basis:** Owner PASS R6 26186464 B01 'Gen III/V Pokérus and Held Item activation work' (C0312) and OCT26 (C0398).
- **Hardware acceptance:** R6 26186464 B01
- **Next action:** None.
- **Owner** (C0165): > Pokérus works in Gen II and IV but not in Gen III or V. Held Item selection is unreliable: in Gen III, neither A nor touch selected an item, and some taps do nothing.
- **Occurrences (3):** R5 1ca2faec L06 FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P1 n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B01 · L06 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-EDT-016` Gen III held items follow PKHeX legality (Safari Ball / Berry Juice never held)

- **Status:** Hardware Accepted · **Gen:** III · **Area:** pokemon-edit · **Category:** decision
- **Last owner result:** R6 26186464 C08: PASS
- **Basis:** Fix 15c77226; owner PASS R6 26186464 C08 on LeafGreen (C0327) and C15 unreleased label kept (C0334).
- **Repair commits:** 15c77226 fix(gen3): never offer or write unreleased held items (Safari Ball, Berry Juice)
- **Hardware acceptance:** R6 26186464 C08
- **Next action:** None.
- **Owner** (C0327): > On LeafGreen:<br>• Safari Ball and Berry Juice cannot be chosen as new held items.<br>• Safari Ball can be chosen as a caught Ball.
- **Occurrences (5):** R5 1ca2faec n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R6 26186464 C08 · Held-item policy PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C15 · Unreleased label PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; REQ Native restrictions n/a [README.md]

### `PBX-EDT-017` Routine successful edits (Unown letter, Y reroll, Shiny) post bottom-centre toasts in every generation

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** pokemon-edit · **Category:** request
- **Last owner result:** R11 3634d1e6 H22: FAIL
- **Basis:** Shared rule include/UI/EditFeedback.h:29-34 (routine Applied outcomes post nothing; static_asserts) used by Gen1 (Gen1PokemonEditorOverlayFoundation.inc:216-221), Gen2 (Gen2PokemonEditorFoundation.inc:424-469), Gen III/IV/V reroll and Gen V Shiny.
- **Repair commits:** f2cf5954 Gen I-V: no toast after routine edits; Gen V Shiny toggles inline (H22, H29)
- **Automated tests:** tests/test_edit_feedback.cpp : postsStatus rule per kind/outcome; tests/test_ui_unown_letter_host.cpp : Unown letter silent; tests/test_ui_editor_input_host.cpp : reroll silent
- **Hardware acceptance:** 066e1db9 R12 H13 · R12 item H13
- **Next action:** Owner runs R12 H13 in every generation.
- **Owner** (E0065): > we still dont have a number in the calculated column for special calculated stat, add it when i swithc unowns i get a pop in middle bottom about stat changes and i dont need it … i also get the pop uo for rerolling the de/ev/iv/ev's can we fix this for all gens … when im in gen 5 and go to choose shiny … i get a pop up menu instead of a inline choose
- **Occurrences (2):** R11 3634d1e6 H22 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]

### `PBX-EDT-018` Gen V Shiny opens 'Change PID?' popup instead of inline toggle

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** pokemon-edit · **Category:** defect
- **Last owner result:** R11 3634d1e6 H29: FAIL
- **Basis:** src/UI/Gen5SharedReadOnlySurface.inc:632-650: ShinyRow toggles in place through verifiedPidProbe, no offer()/PendingPidChange5 dialog; withShiny5 keeps low PID half (gender), ability bit.
- **Repair commits:** f2cf5954 Gen I-V: no toast after routine edits; Gen V Shiny toggles inline (H22, H29)
- **Automated tests:** tests/test_ui_editor_input_host.cpp : Gen V Shiny inline, no PID dialog; tests/test_qa_cross_gen_contract.py : H29 contract
- **Hardware acceptance:** 066e1db9 R12 H13 · R12 item H13
- **Next action:** Owner runs R12 H13 Gen V part.
- **Owner** (E0028): > gen 5 in the editor view doesnt have inline selcetion for shiny option, it opens a menu and warns about pid, this needs changed to be inline like eveery other gen (1-4)
- **Occurrences (4):** R11 3634d1e6 H29 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H29 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]

### `PBX-EXP-001` Export Copy writes a separate verified copy (original_backup + edited + manifest) in an app-owned folder; result visible

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H14: PASS
- **Basis:** Latest owner result R11 3634d1e6 H14 PASS (E0013); OCT24 B08 'export outcome visible' PASS (C0077). Current source: VerifiedStagedExport.cpp:311-340 publishes into an exclusively claimed new directory after strict reparse; same protocol in Gen2ExportTransaction.cpp:232-266 and Gen5WorkingCopyExport.cpp:292-333; manifest SOURCE_PATH written (VerifiedStagedExport.cpp:267). Export callers changed in Round 11 (staged editors: Gen I/II carry, species-history), so recheck in R12.
- **Repair commits:** 24e730a0 test(qa): Gen I-V parser fuzzing and export-safety contracts; fa9cfcea (QA-F3 publish fix); 0bac3cf1 fix(gen2): publish verified exports with an exclusive mkdir (QA-F3b)
- **Automated tests:** tests/test_qa_export_contracts.cpp : C1 source unchanged, C2 strict reparse, C3 whitelist, C4 never overwrites, C5 no partial output, C6 Discard All then export refused; tests/test_classic_verified_export.cpp : Gen I/II exact original backup, checksum-valid edited save, fault cleanup; tests/test_gen5_export_validation.cpp : Gen V candidates strict-reparse with same title/copy
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** Owner runs R12 H17 (Export Copy part).
- **Occurrences (17):** PRE E2 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE E3 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE F05 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F06 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE export PASS [docs/qa/GEN1_5_MATRIX.md]; PRE export NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE Export requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; R1 7fea1797 checklist-item [docs/hardware/RETEST_7fea1797.md]; R4 c60d4da0 B08 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 B08 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 B08 PASS [tests/test_hw_round1_regression_gate.py]; R8 6ec5ccb9 H09 checklist-item [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]; R9 d6a20a2a H14 checklist-item [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H14 checklist-item [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H14 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ F01-F09 / F11 / F14 requirement [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Export requirement [docs/TRANSFER_MODEL.md]

### `PBX-EXP-002` Stale-export guard: refuse export when source changed since open (F09; Gen I-III G5)

- **Status:** Hardware Accepted · **Gen:** I, II, III, IV, V · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R6/26186464/C17: PASS (Gen I-III)
- **Basis:** R6 26186464 C17 PASS for Gen I-III (C0336). Current source: ClassicSourceGuard::unchangedSinceOpen (include/UI/ClassicSourceGuard.h:45) called before export (src/UI/ClassicInventoryOverlay.cpp:399-400, src/UI/Gen1PokemonEditorOverlayUX.inc:718); ClassicSourceGuard.h unchanged since 26186464. Gen IV/V rely on their own open-snapshot checks; no owner test of the Gen IV/V stale refusal.
- **Repair commits:** 4fa35a60 feat(review): ... Gen I-III source guard
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND7 C17
- **Hardware acceptance:** 26186464 R6 C17
- **Next action:** Optional: add a Gen IV/V stale-source refusal item.
- **Occurrences (6):** PRE F09 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R6 G5 engineering-finding [docs/audits/ROUND6_SAVE_SAFETY_PKSE.md]; R6 26186464 C17 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 C17 · Source guard PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 C17 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A47 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-EXP-003` Export / backup fault handling: no space, read error, permission, existing folder, power loss; never claim success from partial file

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Implemented and host-tested only: ExportFailurePoint injection and cleanup in VerifiedStagedExport.cpp:300-340 (destination appears, partial publish -> unpublish); DurableFile previous/failed generations (src/Utils/DurableFile.cpp:237-372). Owner instructed not to simulate power loss on hardware (R11 H44 note). No hardware result.
- **Repair commits:** fa9cfcea (QA-F3); 0bac3cf1 (QA-F3b)
- **Automated tests:** tests/test_qa_export_contracts.cpp : C5 no partial output under injected failures; tests/test_backup_workspace_durability.cpp : interrupted writes leave recoverable generations
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Keep host-only; optionally a safe 'SD nearly full' manual check.
- **Occurrences (3):** PRE F10 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Low-space / crash recovery journal requirement [docs/SAVE_SAFETY.md]; REQ Error/dialog rules requirement [docs/UI_FLOW.md]

### `PBX-EXP-004` Exports change only whitelisted bytes and repair native checksums (F11); no-op edits produce no byte change

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Host-verified: qa-matrix export cell checks only whitelisted record/block/checksum bytes differ (docs/qa/GEN1_5_MATRIX.md); QA-F8b Gen IV release zero-fill fixed f883f4d0. Not directly observable on hardware; no owner result.
- **Repair commits:** f883f4d0 (QA-F8b Gen IV release blank fix)
- **Automated tests:** tests/test_qa_export_contracts.cpp : C3 whitelist; tests/test_verified_export_verbatim.cpp : unchanged regions verbatim
- **Hardware acceptance:** none (host-only)
- **Next action:** None (host gate).
- **Occurrences (5):** PRE F11 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE QA-F8b engineering-finding [docs/qa/FINDINGS.md]; PRE QA-F8b n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; REQ Staging / change-budget requirement [docs/SAVE_SAFETY.md]; REQ Beta / RC gates requirement [docs/RELEASE_CHECKLIST.md]

### `PBX-EXP-005` Repeat export gets a fresh unique destination; race-safe publish (QA-F3 common/Gen V, QA-F3b Gen II)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Fixed: exclusive mkdir claim, EEXIST -> 'nothing was replaced' (VerifiedStagedExport.cpp:311-321, Gen2ExportTransaction.cpp:239-248, Gen5WorkingCopyExport.cpp:321-333). Never owner-tested (repeat Export Copy twice is not a checklist item; R11 H05 'export twice' is the .pkX file feature).
- **Repair commits:** fa9cfcea (QA-F3); 0bac3cf1 fix(gen2): publish verified exports with an exclusive mkdir (QA-F3b)
- **Automated tests:** tests/test_qa_export_contracts.cpp : DestinationAppearsAsEmptyDirectory/AsFile -> refused, nothing replaced; tests/test_gsc_export_transaction.cpp : Gen II publish
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add 'Export Copy twice -> two separate folders' to a future checklist.
- **Occurrences (6):** PRE F14 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE QA-F3b engineering-finding [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE QA-F3 PASS [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE QA-F3 engineering-finding [docs/qa/FINDINGS.md]; PRE QA-F3b engineering-finding [docs/qa/FINDINGS.md]; PRE QA-F3b n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]

### `PBX-EXP-006` S06: exported edited save loads in an emulator as a separate save (round-trip)

- **Status:** Needs Investigation · **Gen:** all · **Area:** export-copy · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H46: PASS (method unrecorded); S06 R6/26186464/D03: NOT TESTED
- **Basis:** S06 NOT TESTED in R1-R6 (owner deferred on purpose, C0175/C0339). R11 3634d1e6 H46 'Export Copy reload and compare (OPTIONAL)' is recorded PASS (E0045) but its steps allowed 'load ONLY that copy in an emulator OR compare on PC' and the worksheet records no game/emulator, so it is unknown whether an emulator ever loaded an export. v1 roadmap release order requires 'Test exported edited.sav with the corresponding emulator' (F0143).
- **Automated tests:** tests/test_qa_export_contracts.cpp : C2 strict reparse as same game; QA matrix export.checksum=detects-corruption (20/20) : reparse catches a corrupted checksum byte
- **Hardware acceptance:** none (not on R12 checklist; R12 header says emulator load is optional)
- **Next action:** Ask the owner what H46 actually did (emulator load or PC compare, which games). If no emulator load, schedule an optional per-generation emulator load of an export copy (never overwriting the original) before v1.0.
- **Owner** (B0053): > S06 we havent added writing to the app yet  i wouldnt know yet
- **Owner** (B0470): > Owner explicitly declines live emulator-save write testing right now.
- **Owner** (C0339): > Deferred on purpose. Not permission for direct writing.
- **Occurrences (26):** PRE F08 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R1 7fea1797 S06 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 S06 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 S06 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 S06 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 S06 NOT TESTED [issue #313 (comment 2026-10-17)]; R1 7fea1797 owner-request [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 S06 checklist-item [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f S06 checklist-item [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f S06 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 S06 checklist-item [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R3 62a2f8c3 S06 checklist-item [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 S06 checklist-item [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 S06 NOT TESTED [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 S06 NOT TESTED [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 S06 NOT TESTED [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 S06 NOT TESTED [PR #307 (body, updated 2026-10-24)]; R5 1ca2faec S06 checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec S06 NOT TESTED [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec S06 owner-decision [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 D03 · S06 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 D03 · S06 NOT TESTED [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 D01 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 H46 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ Release order 2 / 5 requirement [docs/V1_ROADMAP.md]; REQ Owner request 11 requirement [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]

### `PBX-EXP-007` Verified strict reparse of the written edited save before publish (C2); checksum corruption detected

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** export-copy · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none (refusal path)
- **Basis:** Implemented: strict staged reparse before write and of written file (VerifiedStagedExport.cpp:179,259; Gen2ExportTransaction.cpp:160,221; Gen5WorkingCopyExport.cpp:292-296). Success path ran on device whenever an export succeeded (R11 H14 PASS) but the refusal path (corrupt/failed reparse) is host-only; no owner item exercises it.
- **Repair commits:** 9d4b8b7 (R3 exportCorruptionProbe in QA matrix)
- **Automated tests:** QA matrix export.checksum (exportCorruptionProbe) : one corrupted checksummed byte stops reparse, all 20 games; tests/test_qa_export_contracts.cpp : C2
- **Hardware acceptance:** none (host-only)
- **Next action:** None (host gate); keep in untested-safety list.
- **Occurrences (6):** PRE F07 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE Verify requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; R3 S06 PASS [docs/qa/GEN1_5_MATRIX.md]; R3 62a2f8c3 S06 host probe checklist-item [docs/hardware/RETEST_62a2f8c3.md]; R8 6ec5ccb9 n/a [docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md (Known gaps)]; REQ Staging requirements requirement [docs/SAVE_SAFETY.md]

### `PBX-EXP-008` Export Copy made from a backup session records the backup as SOURCE_PATH (7E G4)

- **Status:** Needs Investigation · **Gen:** I, II, III, IV, V · **Area:** export-copy · **Category:** engineering
- **Last owner result:** none
- **Basis:** Still true in current source: manifest SOURCE_PATH=request.sourcePath (VerifiedStagedExport.cpp:267, Gen2ExportTransaction.cpp:124, Gen5WorkingCopyExport.cpp:140); such an export groups under the backup's history, and Recovery files groups protection by SOURCE_PATH (include/UI/RecoveryManagementModel.h:86). Audit called it 'Documented; grouping only'; no owner decision recorded. Unknown: whether the owner accepts this grouping, and whether it can make a live save's newest export look removable in Recovery files.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner decision on grouping; add a host test that a backup-session export never lowers protection of the live save's copies.
- **Occurrences (1):** R6 G4 engineering-finding [docs/audits/ROUND6_SAVE_SAFETY_PKSE.md]

### `PBX-FIL-001` Standalone .pk1-.pk5 / .ek3-.ek5 export and import (staged, new file names, cross-gen refused)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** import-export-file · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H06: PASS
- **Basis:** R11 3634d1e6 H05 (export twice, ' (2)' name) and H06 (import into empty slot, staged, Review lists it) PASS; H38 cross-gen refusal PASS. Import refuses occupied slots (Gen3StagedPokemonEditor.cpp:1134+, Gen4StagedPokemonEditor.cpp:636-638 'refuses to overwrite an occupied box slot', Gen5PokemonFile.h:27) and Gen I/II append to first empty packed slot (src/UI/PokemonFileMenus.inc:27,46). Changed after 3634d1e6: '-' Cancel (aace6021) and X binding (dd69bc93) -> recheck. Occupied-slot refusal never owner-tested (R11 H32 NOT TESTED).
- **Repair commits:** f3959d67 Pokemon Actions: Import from file / Export to file in every Gen I-V sheet; 4be9bbd7, bf9ba508, 1a6e6a74, 8579c5c4 Feature A per-gen import/export
- **Automated tests:** tests/test_pokemon_file_gen12.cpp / gen3 / gen4 / gen5 : import into empty slot, refuse occupied/out-of-range/wrong-gen, non-mutating export; tests/test_ui_pokemon_file_menus_host.cpp : Actions -> Import/Export through the real screen
- **Hardware acceptance:** 066e1db9 R12 H05/H06 · R12 item H05
- **Next action:** R12 H05/H06; add an occupied-slot import refusal check (H32 was NOT TESTED).
- **Owner** (E0037): > how are we suppose to move a gen 1 pokemon  to like gen 5 ?? does it have to go through bank first or how we doing that ??
- **Occurrences (8):** R9 d6a20a2a H05 checklist-item [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R9 d6a20a2a H06 checklist-item [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H05 checklist-item [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R10 3634d1e6 H06 checklist-item [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H05 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H06 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H38 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ Pokémon and inventory requirement [README.md]

### `PBX-FIL-002` SD file browser needs a Cancel ('-') that closes and cancels import (H39)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** import-export-file · **Category:** request · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H39: FAIL
- **Basis:** R11 FAIL/request. aace6021: Key::Cancel closes from any folder/Favourites with nothing imported (include/UI/PokemonFileDialogModel.h:12-14,130-134); '-' mapped first (src/UI/PokemonFileDialog.cpp:204-206); Link Game File browser B/- close (SaveSelectScreen.cpp). Footer shows both. Not yet owner-tested.
- **Repair commits:** aace6021 Round 11 H39 + QUIT: '-' cancels SD browsers; two-button quit dialog
- **Automated tests:** tests/test_pokemon_file_dialog_model.cpp:136-152 : '-' (Key::Cancel) closes from any folder or list without changing state
- **Hardware acceptance:** 066e1db9 R12 H06 · R12 item H06
- **Next action:** Owner runs R12 H06.
- **Owner** (E0038): > can we add a cancel feature, right now A opens stuff and confirms and B goes to the parent directory or "goes back" we need to add a buttoin that is cancel which closes the menu and cancel the import can we make it "-" add the footer for all needed spaces and gens and app wide
- **Occurrences (5):** R11 3634d1e6 H39 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H39 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 engineering-finding [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; R12 066e1db9 H06 checklist-item [docs/hardware/RETEST_066e1db9.md]; REQ H39 requirement [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-FIL-003` X = Import on an empty slot / Export on an occupied slot, app-wide (Extra 4)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** import-export-file · **Category:** request · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** dd69bc93: SharedPokemonEditorContract::boxActivation / boxXLabel (include/UI/SharedPokemonEditorContract.h:168-188) returns ImportFile/ExportFile, wired in Gen I-V Boxes grids; footers and Help updated. Not yet owner-tested.
- **Repair commits:** dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_ui_pokemon_file_menus_host.cpp : X on occupied exports, X on empty imports
- **Hardware acceptance:** 066e1db9 R12 H05 · R12 item H05
- **Next action:** Owner runs R12 H05.
- **Owner** (F0159): > is it possible to change the contorls in the save editored when in the box, can we get rid of the X to add and change it to import/export instead like if the cursor is on a pokemon X would say export but it its on a blank space it would say import, makes this app wide for all gens and change the footer as needed, and change the help menus app wide as need as well.
- **Occurrences (2):** REQ Extra 4 requirement [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ X-IMPEXP requirement [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-HOM-001` Install and open to Product Home (standalone, no PKSE); HOME card renders correctly

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** install-home · **Category:** requirement
- **Last owner result:** R11/3634d1e6/H01: PASS
- **Basis:** R11 3634d1e6 H01 'Install and open' PASS (E0000). HOME card scenery rewritten in 066e1db9 (c21d3747: region scenery masked with rounded corners; include/UI/CardScenery.h, RoundedArtwork.h) and covered by R12 H01.
- **Repair commits:** c21d3747 HOME card: mask region scenery with the card's rounded corners (Round 11)
- **Automated tests:** tests/test_ui_home_card_scenery_host.cpp : scenery painted as rounded shapes concentric with the card, no rectangular clip; tests/test_home_card_scenery_contract.py
- **Hardware acceptance:** 066e1db9 R12 H01 · R12 item H01
- **Next action:** Owner runs R12 H01.
- **Owner** (D0282): > Standalone PokeBank NX has now been physically tested on real Switch hardware with PKSE removed: app launches, does not recreate `/PKSE/`, and native supported games remain usable.
- **Occurrences (10):** PRE  PASS [issue #29 (comment 2026-09-12)]; PRE A04 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE A11 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE B01 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R8 6ec5ccb9 H01 n/a [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]; R9 d6a20a2a H01 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H01 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H01 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R12 066e1db9 H01 n/a [docs/hardware/RETEST_066e1db9.md]

### `PBX-HOM-002` Native Shield / Scarlet-Violet / Legends Z-A Home cards show 'Party preview unavailable.'

- **Status:** Needs Investigation · **Gen:** app · **Area:** install-home · **Category:** defect
- **Last owner result:** PRE/2d2d7bc2: FAIL
- **Basis:** Owner FAIL on 2d2d7bc2. Current src/UI/SaveSelectScreen.cpp:1856-1907: installed Switch titles preview only when Save::validateTrainerSaveForOpen succeeds, else 'Party preview unavailable.'. The 0760334f '100% pass' (D0337) is scoped to Gen I-IV; no later owner result for SwSh/SV/Z-A cards, and Round 11/12 builds are Gen I-V scope. Unknown whether these titles now produce a party preview.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner states whether Gen VIII/IX Home party previews are in v1 scope; if yes, retest on 066e1db9.
- **Owner** (D0336): > - Shield / Scarlet-Violet / Legends Z-A still show `Party preview unavailable.`
- **Occurrences (1):** PRE 2d2d7bc2 FAIL [PR #92 (body, updated 2026-10-12)]

### `PBX-HOM-003` Home party strip previews drop form (Gen V) / pass form 0, Gen III level 0, shiny only for Gen V

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** install-home · **Category:** defect
- **Last owner result:** none
- **Basis:** b3f88716 (7fea1797) 'one Gen I-V Home party rule (species/level/form/shiny/gender)' via Integration/Common/HomePartyPreview.h; SaveSelectScreen.cpp:1814-1815 fills form and shiny; include/Integration/Gen5/Gen5GameCardPreview.h:27 PK5 form. No owner result names Home party form/shiny/level.
- **Repair commits:** b3f88716 feat(home): one Gen I-V Home party rule (species/level/form/shiny/gender); ca967840 feat(home): Gen III Home party gender from the PID
- **Automated tests:** tests/test_home_party_preview.cpp : Gen I-III adapters fill species/level/form/shiny/gender only where the format defines them; tests/test_home_card_gen4_snapshot.cpp / test_home_card_gen5_snapshot.cpp
- **Hardware acceptance:** none (not on any checklist); observable on 066e1db9 R12 H01
- **Next action:** During R12 H01 note whether a shiny/Unown/forme party member shows correctly on the Home card.
- **Occurrences (4):** PRE QA-F5 n/a [docs/qa/FINDINGS.md]; PRE  n/a [docs/qa/FINDINGS.md]; PRE QA-F5 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE home.party.level / form / shiny / gender n/a [docs/qa/GEN1_5_MATRIX.md]

### `PBX-HOM-004` Home refresh (save size/mtime watch, party sprite drawing) never device tested

- **Status:** Needs Investigation · **Gen:** I, II, III, IV, V · **Area:** install-home · **Category:** requirement
- **Last owner result:** none
- **Basis:** Host qa-matrix 'home-refresh' PASS only (docs/qa/GEN1_5_MATRIX.md); listed as hardware-only residue NOT DEVICE TESTED. Unknown: whether Home refreshes after an emulator writes the save, on hardware.
- **Automated tests:** tests/test_qa_matrix_home.h : home-refresh cell (host)
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add a step: play/save in emulator, return, confirm Home card party updates.
- **Occurrences (4):** PRE home-refresh PASS [docs/qa/GEN1_5_MATRIX.md]; PRE home-refresh (Gen I–III) NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE home-refresh (Gen IV/V) NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE home.refresh n/a [docs/qa/GEN1_5_MATRIX.md]

### `PBX-HOM-005` Emulator/provider names: Home cards name the real provider (DraStic, melonDS, RetroArch (DeSmuME), mGBA), but the Trainer / Game Browser header showed raw or generic labels (RETROARCH, EXTERNAL, Source) (Part 9 #3, CG26, AUD-03)

- **Status:** Hardware Accepted · **Gen:** app · **Area:** install-home · **Category:** defect
- **Last owner result:** R3/62a2f8c3/AUD-03: PASS
- **Basis:** QA repro on 0faf7e9c: legacyProviderLabel() returned sourceProviderLabel raw, so the Trainer / Game Browser header showed 'RetroArch/DeSmuME', 'Source', 'Remembered' or hard-coded 'RETROARCH'/'EXTERNAL' (docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md:23-27). Fixed by aeccd13a (2026-10-19), an ancestor of R3 candidate 62a2f8c3; owner PASS R3 62a2f8c3 AUD-03 'Emulator names' (B0572, frozen guard B0685). Current source: include/UI/TrainerViewScreenBase.h:349-351 legacyProviderLabel() returns PokeVault::Source::providerDisplayName(sourceProviderLabel), identical to 62a2f8c3; include/Source/SaveProviderIdentity.h:67-86 has no commits in 62a2f8c3..HEAD. The label feeds the header (src/UI/TrainerViewScreenBase.inc:5170) and the Gen II/IV/V source banners (src/UI/Gen2SharedSurfaceParity.inc:129, src/UI/Gen4SharedPokemonSurface.inc:1618, src/UI/Gen5SharedReadOnlySurface.inc:1985).
- **Repair commits:** 796ba212 feat(ui): button badges in help and modal text; provider names in sessions; profile focus kept; aeccd13a fix(home): same-profile guard lives in setUser; header calls providerDisplayName directly (CG26/CG28)
- **Automated tests:** tests/test_qa_cross_gen_contract.py:1526-1540 CG26-trainer-provider-display-name : legacyProviderLabel() must go through providerDisplayName(), never raw sourceProviderLabel / RETROARCH / EXTERNAL; tests/test_home_card_providers.cpp : Home cards name real providers Gen I-V; tests/test_session_provider_names.cpp : each provider (mGBA, RetroArch + DeSmuME core, melonDS, DraStic, Switch title) named in sessions; tests/test_hw_round1_regression_gate.py:397-400 GUARDS_ROUND3['AUD-03'] : frozen guard for the R3 owner PASS
- **Hardware acceptance:** accepted 62a2f8c3 R3 AUD-03 (not on the R12 checklist)
- **Next action:** No change needed; include provider names in the next smoke check of earlier passes (code unchanged since 62a2f8c3).
- **Occurrences (3):** R3 62a2f8c3 AUD-03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 0faf7e9c Part 9 #3 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-03 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-LEG-001` "Origin & Legality" label and one truthful, unified legality/provenance panel in every generation (no fabricated clean verdict, no invented certainty)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** legality · **Category:** defect
- **Last owner result:** R6/26186464/A01: PASS
- **Basis:** R1/R2 NOT VERIFIED, R3 62a2f8c3 FAIL ("Unify legality layout ... Do not fabricate clean verdicts"); repaired 20955b20; owner PASS OCT24 c60d4da0, R5 1ca2faec and R6 26186464 A01 ("Origin & Legality uncertainty is correct"). Gen V now has the row (include/Integration/Gen5/Gen5SharedScreenState.h:67; src/UI/Gen5SharedReadOnlySurface.inc:270). Round 11 changed legality wording for Gen I-V sessions in the generic details modal (d347afad, src/UI/Modals/PokemonDetailsModal.cpp; include/UI/SharedEditorPresentation.h:45-59), so recheck.
- **Repair commits:** 4203ce0d feat(gen1/gen2): Origin & Legality labels, shared picker footers and Pokerus rule; 20955b20 Share truthful Origin panels and native classic Trainer summaries with picker regressions guarded; d347afad Round 11 CG48: no green clean legality verdict in Gen I-V sessions
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG03 Origin & Legality heading; CG48 truthful state; tests/test_hw_round1_regression_gate.py : E03 GUARDS_ROUND5
- **Hardware acceptance:** 26186464 R6 A01 · R12 item H09
- **Next action:** During R12 H09 (Origin & Legality is opened) confirm the panel wording; no separate step needed.
- **Owner** (C0281): > Origin & Legality uncertainty is correct
- **Owner** (B0465): > Owner: Gen I says legality not checked in View/Edit and Origin & Legality. Gold/Silver/Crystal party View sometimes says Legality OK but created Pokémon aren't checked. Gen III like Gen I. Gen IV separate PK4 structure PASS/Legality Incomplete popup with moves. Gen V richer popup including met location/level/date/encounter but incomplete. **Unify legality layout for every generation; full separate legality engine isn't integrated yet. Do not fabricate clean verdicts.**
- **Owner** (D0230): > **Six other unchecked/uncertain, not failed:** B04 (Gen IV/V Bag flow), E03 (legality label), E11 (shiny reroll relations, mechanics need verification), L06 (Party/summary touch), S04 (Gen V Review/Discard/Export suspected, needs repro), S06 (edited export not reopened in emulator).
- **Occurrences (27):** PRE CG03 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE D16 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE 4/7 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 E03 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 E03 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E03 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 E03 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E03 NOT TESTED [issue #313 (comment 2026-10-17)]; R2 E03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f E03 n/a [docs/hardware/RETEST_e10f307f.md]; R2 E03 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R2 e10f307f E03 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R3 62a2f8c3 E03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 E03 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 E03 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 E03 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 E03 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E03 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 E03 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec E03 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec E03 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A01 · E03 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A01 · E03 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A01 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 C07 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-LEG-002` Gen V shiny toggle may change the Wurmple-line evolution value (PID high half rewritten)

- **Status:** Open Defect · **Gen:** V · **Area:** legality · **Category:** defect
- **Last owner result:** none (E11 NOT VERIFIED R1/R3, stats area)
- **Basis:** include/Integration/Gen5/Gen5PidCoordination.h:54-68 withShiny5 rewrites PID bits 17-31 with no Wurmple-evolution constraint, whereas Gen III and Gen IV PID searches preserve it (src/Pokemon/Pokemon3FRLG.cpp:218-221, src/Pokemon/Pokemon4Mutable.cpp:833-848 want.wurmpleEvo). Documented limit deferred to #314 (detection) / #329; owner E11 never verified.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Either constrain withShiny5 to keep (PID>>16)%10 class for Wurmple-line species, or record an owner decision accepting the limit; cite #329.
- **Occurrences (1):** R1 7fea1797 E11 n/a [docs/qa/ROUND2_STATUS.md]

### `PBX-LEG-003` Complete legality engine / PKHeX-class encounter, move and ball legality not integrated (PRs #314/#327/#328 isolated, tracker #329)

- **Status:** Missing Feature · **Gen:** all · **Area:** legality · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** The legality-engine PRs #314, #327, #328 are kept isolated and are not in this app (ROUND11_ENGINEERING_INTAKE.md section 5; RETEST_066e1db9.md: "no legality-engine code"). The app shows only neutral Origin & Legality / Incomplete states (CG48). docs/V1_ROADMAP.md:11 release order 3 requires finishing #314. Tracker: #329.
- **Automated tests:** tests/test_r11_legacy_legality_wording.py : Gen I-V clean report shows Incomplete, never green
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Track on #329; integrate only after the app candidate is accepted (V1_ROADMAP release order 3-4).
- **Occurrences (6):** R4 c60d4da0 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §5 separate from the repa…]; REQ Release order 3 n/a [docs/V1_ROADMAP.md]; REQ §11 n/a [docs/PRODUCT_DECISIONS.md]; REQ PKHeX Oracle / legality assistant n/a [docs/CREATE_POKEMON_VISION.md]

### `PBX-LEG-004` Misleading legacy green "no problems found" legality wording in Gen I-V sessions (generic details modal / Bank Pokemon)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** legality · **Category:** defect
- **Last owner result:** none
- **Basis:** src/UI/Modals/PokemonDetailsModal.cpp now maps a Gen I-V clean report to the shared Incomplete state in the warning colour (checked by tests/test_r11_legacy_legality_wording.py:24-37); include/UI/SharedEditorPresentation.h:57-59 kUnverifiedForbiddenClaims. No owner result.
- **Repair commits:** d347afad Round 11 CG48: no green clean legality verdict in Gen I-V sessions
- **Automated tests:** tests/test_r11_legacy_legality_wording.py : Gen I-V clean verdict gated on partialLegalityCoverage; tests/test_qa_cross_gen_contract.py : CG48
- **Hardware acceptance:** none (not on any checklist) — needs 066e1db9 add-on
- **Next action:** Add an R12/R13 step: open a Bank Pokemon details modal from a Gen I-V save and confirm "Incomplete", not green.
- **Occurrences (1):** R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]

### `PBX-LEG-005` Independent legality leads: Gen IV EV/EXP reported clean when it should be flagged; incomplete coverage promoted to "Complete"

- **Status:** Needs Investigation · **Gen:** IV, all · **Area:** legality · **Category:** defect
- **Last owner result:** none
- **Basis:** Leads relayed by the owner (issue #329 comment 2026-10-31), not reproduced; they concern the isolated engine (#314/#327/#328), which is not in this app. In this app, Gen I-V clean reports are forced to "Incomplete" (d347afad; tests/test_r11_legacy_legality_wording.py). Unknown: whether either lead reproduces in the engine branches.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Reproduce on the engine branches and record the result on #329.
- **Owner** (D0240): > 2. **Coverage promotion.** Legality coverage that is actually incomplete may be promoted to **Complete**.
- **Owner** (D0239): > 1. **Gen IV EV/EXP false clean.** A Gen IV record whose EV/EXP combination should be flagged was reported clean.
- **Occurrences (3):** R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; GH  n/a [issue #329 (comment 2026-10-31)]; GH  n/a [issue #329 (comment 2026-10-31)]

### `PBX-LNG-001` Gen II raw item 5 shown as '# BALL'

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II · **Area:** language-names · **Category:** defect
- **Last owner result:** PRE fc1e6de2: FAIL
- **Basis:** src/Integration/Gen2/Gen2ReadOnlyInventory.cpp:40 table entry 5 = 'Poké Ball' (GS BALL stays 115, :54/:99). No owner row re-checks this label.
- **Automated tests:** tests/test_gsc_inventory.cpp / tests/test_classic_inventory_ui_model.cpp : Poké Ball label
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Glance at a Gen II Poké Ball in R12 H18 Bag smoke.
- **Owner** (A0516): > Reported label was ordinary Poké Ball; shared name table now shows `Poké Ball`; GS BALL stays ID 115.
- **Occurrences (1):** PRE fc1e6de2 FAIL [docs/DEVICE_TEST_CHECKLIST.md]

### `PBX-LNG-002` Gen V Language: read-only, then modal chooser; must A-cycle inline, staged, no toast

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** language-names · **Category:** request
- **Last owner result:** R6 26186464 A27/C03: PASS
- **Basis:** Owner PASS R5 1ca2faec NEW-14, R6 26186464 A27 and C03 'No Gen V Language toast' (C0307, C0322), OCT26 (C0402). Current src/UI/Gen5SharedReadOnlySurface.inc:491-496 stage(Field::Language, Gen5LanguageCycle::next) without toast. Gen V surface name display changed in R11 d1954551.
- **Repair commits:** 1241c2b8 cycle Gen V Pokémon language inline (#325); 64fe8f9c CG79: Gen V Language cycles inline without a status toast
- **Automated tests:** tests/test_gen5_language_cycle.cpp : 1,2,3,4,5,7,8 wrap
- **Hardware acceptance:** 066e1db9 R12 H14 · R12 item H14
- **Next action:** R12 H14.
- **Owner** (D0213): > Owner hardware feedback: in the Gen V shared Pokémon Edit details, pressing A on Language currently opens a modal chooser. Requested: A cycles directly through the native Pokémon language IDs (1 Japanese, 2 English, 3 French, 4 Italian, 5 German, 7 Spanish, 8 Korean), wrapping after Korean;
- **Owner** (C0215): > Follow-up request attached to PASS item NEW-14.
- **Occurrences (8):** R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R3 62a2f8c3 NEW-14 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-14 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 n/a [issue #325 (body 2026-10-24)]; OCT24 c60d4da0 NEW-14 FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec 7C n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A27 · NEW-14 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C03 · 7C PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-LNG-003` Multilingual naming defects M1 D1–D4 (Gen III language+nickname encoding, Gen III/IV Create English defaults, Gen V language default name, Gen I/II accents)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** language-names · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Merged 8c63957a (ancestor of 3634d1e6 and HEAD); include/Names/DefaultSpeciesName.h used by Gen3StagedPokemonEditor.cpp, Pokemon4Mutable.cpp, Gen5StagedPokemonRecord.h. D3 display was masked in R11 by the ASCII-only header (see PBX-LNG-004). R11 H13 note ('when i swap languages it has the proper text') is not item-specific acceptance.
- **Repair commits:** 8c63957a Merge feature/multilingual-gen15; e6bdbebb M1 D4 Gen I/II: international names accept European accents
- **Automated tests:** tests/test_default_species_names.cpp : per-language defaults; tests/test_gen5_language_default_name.cpp : Pikachu -> ピカチュウ/피카츄 bytes
- **Hardware acceptance:** 066e1db9 R12 H14 (D3 only); D1/D2/D4 none
- **Next action:** Add Gen III/IV Create in non-English language and Gen I/II accented nickname rows to a later round.
- **Owner** (A0757): > Fixed with regression test on feature/multilingual-gen15; NOT DEVICE TESTED (host only, no NRO built).
- **Occurrences (4):** M1 D1 n/a [docs/multilingual/M1_NAMING_DEFECTS.md]; M1 D2 n/a [docs/multilingual/M1_NAMING_DEFECTS.md]; M1 D3 n/a [docs/multilingual/M1_NAMING_DEFECTS.md]; M1 D4 n/a [docs/multilingual/M1_NAMING_DEFECTS.md]

### `PBX-LNG-004` Gen V header name does not follow Language; Korean names render as '[][][][][]'

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV, V · **Area:** language-names · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H27: FAIL
- **Basis:** Header uses displayName5 (src/UI/Gen5SharedReadOnlySurface.inc:880-883) instead of asciiName5; Korean shared font registered as last fallback (src/UI/PKSEFramebuffer.cpp:95-121, include/UI/SystemFontFallbacks.h:14-16). Write gate still ASCII (:440-445).
- **Repair commits:** d1954551 H27: Gen V names show their stored language; Hangul renders via the Korean system font
- **Automated tests:** tests/test_gen5_display_name.cpp : decoded native names; tests/test_korean_font_contract.py : PlSharedFontType_KO requested; tests/test_system_font_fallbacks.cpp : font order
- **Hardware acceptance:** 066e1db9 R12 H14 · R12 item H14
- **Next action:** R12 H14 incl. a Korean Gen IV/V Pokémon; watch for atlas/memory regressions on mixed-script screens.
- **Owner** (E0026): > language changes but the top names doesnt change like in gen 1-4, all gens need the proper names for their language and also we need to add the korean letters in cause they look like this "[][][][][]" also in our keyboards we need to add the proper letters and charcters for japenese and korean and all languges is that possible in the nanog kayboard and is it possible to add a new button in our custom search bar that would also change the letters or would it be easier to have when we can the language everyhting would get change to that language?? feedback ??
- **Occurrences (3):** R11 3634d1e6 H27 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; REQ H27 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-LNG-005` Name entry for all languages: accented keys, Japanese kana and Korean Hangul keyboards, Gen V Unicode entry, app-wide display language

- **Status:** Missing Feature · **Gen:** I, II, III, IV, V · **Area:** language-names · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H27: FAIL
- **Basis:** Partially built: Accents page and Gen I/II kana pages (src/UI/Dialogs/KeyboardDialog.cpp), Hangul page only for Korean-language Gen IV names (db414ae7, KeyboardDialog.cpp:193-195, Gen4SharedPokemonSurface.inc:819). Gen V name entry still ASCII-only ('1-10 ASCII characters', Gen5SharedReadOnlySurface.inc:440-445); no Japanese kana for Gen III-V; no app display-language setting. Owner questions in intake H27 (per-record vs global language, Hangul in v1) unanswered.
- **Repair commits:** db414ae7 H27: Hangul keyboard page for Korean-language Gen IV names; Gen V stays ASCII
- **Automated tests:** tests/test_hangul_composer.cpp; tests/test_ui_hangul_keyboard_host.cpp
- **Hardware acceptance:** none (R12 H14 checks display only)
- **Next action:** Owner decision on the H27 questions (keyboard follows record language? global display language? Hangul/kana entry in v1?), then schedule Gen III-V kana and Gen V Unicode writers.
- **Owner** (E0068): > language changes but the top names doesnt change like in gen 1-4 … korean letters … look like this "[][][][][]" … keyboards … japenese and korean and all languges … feedback ??
- **Occurrences (4):** M1  n/a [docs/multilingual/M1_NAMING_DEFECTS.md]; M1  n/a [docs/multilingual/M1_NAMING_DEFECTS.md]; R11 3634d1e6 H27 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ H27 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-LNG-006` Gen II decodes text with the English table only (French/German/Italian/Spanish saves show wrong letters)

- **Status:** Open Defect · **Gen:** II · **Area:** language-names · **Category:** defect
- **Last owner result:** none
- **Basis:** include/Utils/Gen2Text.h has only GEN2_EN (:30) and GEN2_JP (:65) tables; gen2ToChar(b, japanese) (:100) — no per-save European table. M2 never landed.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Implement M2 per-save European table choice for Gen II, with a French/German Crystal fixture.
- **Owner** (A0761): > Known limit carried into M2.
- **Occurrences (1):** M1  n/a [docs/multilingual/M1_NAMING_DEFECTS.md]

### `PBX-LNG-007` Gen I/II localized default species names

- **Status:** Missing Feature · **Gen:** I, II · **Area:** language-names · **Category:** missing-feature
- **Last owner result:** none
- **Basis:** Names::defaultSpeciesName is only called from Gen III/IV/V paths (Gen3StagedPokemonEditor.cpp, Pokemon4Mutable.cpp, Gen5StagedPokemonRecord.h); no Gen I/II caller.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Schedule M2/M3 or record an owner decision.
- **Owner** (A0762): > Known limit carried into M2.
- **Occurrences (1):** M1  n/a [docs/multilingual/M1_NAMING_DEFECTS.md]

### `PBX-LNG-008` Japanese Gen I/II saves stay read-only (never hardware-tested with a real Japanese save)

- **Status:** Needs Investigation · **Gen:** I, II · **Area:** language-names · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H13: PASS note, not evidence (no Japanese save)
- **Basis:** Gate include/Integration/Common/JapaneseEditingGate.h exists; owner marked R11 H13 PASS but wrote 'i didnt test a japenese save' (intake: recorded as NOT TESTED). Unknown: real Japanese-save behaviour on hardware; also Gen5SharedScreenState.h:542 empties a non-ASCII OT on Create species rebuild (intake H27 note, unverified).
- **Automated tests:** Group A Japanese kit-save rows (synthetic; not hardware)
- **Hardware acceptance:** 066e1db9 R12 H17 (Japanese save if owner has one) · R12 item H17
- **Next action:** Owner tests a real Japanese Gen I/II save in R12 H17; if none available, keep NOT TESTED.
- **Owner** (F0126): > i guess this passes i didnt test a japenese save but when i swap languages it has the proper text
- **Occurrences (3):** R11 3634d1e6 H13 PASS [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ H13 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ Added since Round 6 n/a [README.md]

### `PBX-LNG-009` Language shown only where the exact format stores it; never fabricated

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** language-names · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R6 26186464 A27: PASS (Gen V); R3 62a2f8c3 E09: PASS (Gen III)
- **Basis:** Gen I/II have no Language row (intake H27: 'Gen I/II records have no language byte and no Language row'); Gen III Language PASS R3 E09 (B0511), Gen V Language PASS R6 A27 (C0307). Gen V language display changed in R11.
- **Hardware acceptance:** 066e1db9 R12 H14 · R12 item H14
- **Next action:** R12 H14.
- **Owner** (F0128): > "If the exact current format stores the field, show it ... If the exact current format does not store it, hide it." "Language/origin information that is not independently stored by an old format may still be displayed from trusted source/provenance metadata, but must not be presented as a fabricated editable field in that old save."
- **Occurrences (2):** R3 62a2f8c3 E09 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; REQ Language / Provenance rules n/a [docs/POKEMON_EDITOR_UI_CONTRACT.md]

### `PBX-LNG-010` Nickname entry bounds (clear, invalid chars, max length, cancel); trainer native name/language display

- **Status:** Needs Investigation · **Gen:** I, II, III, IV, V · **Area:** language-names · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Limits exist in source (Gen V '1-10 ASCII' guide, Gen5SharedReadOnlySurface.inc:443; Gen II encodable-accent refusal from M1 D4) but no owner hardware result exists for D03 nickname entry (checklist A0689 never answered) — acceptance unknown.
- **Automated tests:** tests/test_default_species_names.cpp
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add a nickname-entry row (max length, invalid char, cancel) per generation.
- **Owner** (F0129): > D03 "Nickname text, clear, invalid chars, max length and cancel"; E01 "Trainer view native Name, TID/SID, gender, badges, version, language"; E03 "Trainer unknown/new-game shown neutral not male or unsupported glyphs".
- **Occurrences (1):** REQ D03 / E01 / E03 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-LNG-011` No Japanese Gen V save is tested: Gen V species/PID gender and names are checked only at record level, never through a Japanese Black/White/B2W2 save

- **Status:** Needs Investigation · **Gen:** V · **Area:** language-names · **Category:** defect
- **Last owner result:** none
- **Basis:** docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md:93 records the gap. Current tests still have no Japanese Gen V save fixture: Japanese coverage exists for Gen I/II (.pk1/.pk2 lists, tests/test_native_pokemon_file.cpp:49-118; real-save round trip refuses Japanese Gen I/II, tests/test_real_save_roundtrip.cpp:9), while Gen V language handling is tested on records only (tests/test_gen5_language_default_name.cpp:63-77, added 2f9745e3). d1954551 (H27) changed Gen V name decoding after R11. Unknown: whether a real Japanese Gen V save opens, shows names in Japanese script, keeps PID gender and exports byte-correctly. No owner hardware result exists.
- **Repair commits:** 2f9745e3 M1 D3 Gen V: a language change renames an unnicknamed Pokemon to its default name in that language; d1954551 H27: Gen V names show their stored language; Hangul renders via the Korean system font
- **Automated tests:** tests/test_gen5_language_default_name.cpp:63-77 : record-level Gen V default names per language (no Japanese save)
- **Hardware acceptance:** 066e1db9 R12 H14 (only if the owner has a Japanese Gen V save) · R12 item H14
- **Next action:** Add a Japanese Gen V save fixture (open, names, PID gender, export + strict reparse) to the host suite; ask the owner to note in R12 H14 whether a Japanese Gen V save was used.
- **Occurrences (1):** R8 6ec5ccb9 n/a [docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md (Known gaps)]

### `PBX-LQT-001` Boot, idle stability, relaunch, exit-and-relaunch after discard

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** launch-quit · **Category:** requirement
- **Last owner result:** R11/3634d1e6/H48: PASS
- **Basis:** Owner PASS boot/idle/5 relaunches on 3be4de6b (A0000-A0079, A0154-A0156); latest owner result R11 3634d1e6 H48 'Exit and relaunch after discard' PASS (E0047) and H01 'Install and open' PASS. 066e1db9 is a new build (92 src/include files changed) so boot must be re-verified.
- **Automated tests:** host qa-matrix 'relaunch' cell (docs/qa/GEN1_5_MATRIX.md)
- **Hardware acceptance:** 3634d1e6 R11 H48/H01; recheck 066e1db9 R12 H01 · R12 item H01
- **Next action:** R12 H01 covers boot; no separate exit/relaunch item on R12.
- **Owner** (D0263): > CRASHES: NONE
- **Occurrences (27):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE Return path n/a [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE B4 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE A01 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE A02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE relaunch PASS [docs/qa/GEN1_5_MATRIX.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE 3be4de6b CRASHES PASS [issue #8 (comment 2026-09-02)]; R11 3634d1e6 H48 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-LQT-002` HOME button resume, sleep/wake, controller disconnect/reconnect lifecycle

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** launch-quit · **Category:** requirement
- **Last owner result:** PRE/3be4de6b: PASS
- **Basis:** A0142 NOT TESTED in the extended pass, then PASS on the same 3be4de6b binary (DEVICE_TEST_FOLLOWUP_2026-09-03: HOME->resume, sleep->wake, Joy-Con detach/reconnect, input after reconnect). Never re-tested on any later candidate; the app has been substantially rewritten since.
- **Hardware acceptance:** 3be4de6b (PRE); needs recheck on 066e1db9
- **Next action:** Add a lifecycle item (HOME/resume, sleep/wake, Joy-Con reconnect, ideally with a staged edit open) to the R12/R13 checklist.
- **Occurrences (13):** PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE A03 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-LQT-003` Quick Games: pressing A on a game freezes the app (synchronous refreshHubPreview)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** launch-quit · **Category:** defect
- **Last owner result:** PRE/0760334f: PASS (owner '100% pass')
- **Basis:** Quick Games A made cache-only (PR #92); owner '- Quick View / Quick Games still works' on 2d2d7bc2 (D0332) and '100% pass' on 0760334f (D0337). Quick Games overlay changed later (727fad5d tap-outside close, e88957f2; Quick Games touch commits 498c0687/55204e21) without an explicit later owner Quick Games A item.
- **Repair commits:** 498c0687 Open validated game on Quick Games clean tap/A; preserve live drag ownership; 55204e21 Contract Quick Games clean tap to native validated game open
- **Automated tests:** tests/fuzz_home_cache.cpp : Home cache handling
- **Hardware acceptance:** 0760334f (PRE)
- **Next action:** Include 'Y Quick Games -> A on a game' in R12 smoke.
- **Owner** (D0324): > **DEVICE REJECTED**. On physical Switch, opening **Quick Games** and pressing **A** on a game freezes the app.
- **Occurrences (3):** PRE adc92fd0 FAIL [PR #92 (comment 2026-10-05)]; PRE 1 n/a [docs/hardware/2026-10-05-hotpath-validation.md]; PRE 2d2d7bc2 PASS [PR #92 (body, updated 2026-10-12)]

### `PBX-LQT-004` Gen I-III RetroArch quit drops/crashes to Nintendo HOME instead of returning to PokeBank NX

- **Status:** Needs Investigation · **Gen:** I, II, III · **Area:** launch-quit · **Category:** defect
- **Last owner result:** R3/62a2f8c3/AUD-04: NOT TESTED (last explicit quit result PRE/97f540e4: FAIL)
- **Basis:** Diagnosis (docs/handoffs/LAUNCH_RETURN_PROGRESS.md:27-37): after RetroArch's normal Quit the outer loader picks hbmenu or ends the process (NSP forwarder) - not a PokeBank code path. Returning to PokeBank needs the experimental return host, which runs only when <configRoot>/experimental_return_host.enable exists (src/UI/GameLauncher.cpp:41) and crashed on launch when put on the default path (698ea4d1). Owner accepted 0760334f with the direct handoff ('100% pass'), but PR #307 (2026-10-15) still lists the 'Gen I-III RetroArch exit crash' as open and R3 AUD-04 'Game launch and return' was NOT TESTED. Unknown: (a) whether the owner accepts 'quit returns to Homebrew Menu/HOME' (no recorded decision, unlike DraStic), (b) whether the launch dialog's quit-route notice matches device behaviour, (c) the 698ea4d1 return-host crash cause (LAUNCH_RETURN_PROGRESS.md:48 'still open').
- **Repair commits:** 8803c9ea fix(launch): restore hardware-safe RetroArch startup; (returnHostReadiness() preview/launch consistency fix, docs/handoffs/LAUNCH_RETURN_PROGRESS.md:110-112)
- **Automated tests:** tests/test_game_launch_model.cpp : libnx argvSetup tokenisation of every argv PokeBank/return host builds; tests/test_qa_cross_gen_contract.py : CG27 retroarch core-from-save evidence
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Ask owner for a decision (accept return-to-hbmenu as for DraStic, or require return host); if required, add R3-4 style check to a checklist.
- **Owner** (D0326): > - RetroArch quit lifecycle: FAIL — quitting RetroArch drops/crashes to Nintendo HOME instead of returning to PokeBank NX. Treat this as a separate lifecycle issue, not content resolution.
- **Owner** (A0921): > RetroArch quit lifecycle: FAIL — quitting RetroArch drops/crashes to Nintendo HOME instead of returning to PokeBank NX.
- **Occurrences (10):** PRE 97f540e4 FAIL [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE 97f540e4 FAIL [PR #92 (comment 2026-10-06)]; PRE  n/a [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE B20 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE B21 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  FAIL [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F2 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; R3 62a2f8c3 AUD-04 NOT TESTED [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 0faf7e9c Part 9 #4 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 0faf7e9c n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-LQT-005` Gen IV/V DraStic 'Quit to Launcher' returns to DraStic, not PokeBank NX

- **Status:** Intentionally Unsupported · **Gen:** IV, V · **Area:** launch-quit · **Category:** decision
- **Last owner result:** PRE/97f540e4: decision
- **Basis:** Owner decision PR #92 comment 2026-10-06 (D0328/A0922): 'DraStic return-to-PokeBank is no longer an acceptance requirement ... quit behavior follows DraStic's launcher semantics.' Engineering (LAUNCH_RETURN_PROGRESS.md:40-46) rejects changing DraStic's stored launcher path.
- **Hardware acceptance:** n/a (decision)
- **Next action:** Close the stale PR #307 open-bug entry citing the decision.
- **Owner** (D0328): > - DraStic return-to-PokeBank is no longer an acceptance requirement: DraStic exposes Quit to Launcher rather than a return-to-caller contract. DS acceptance should be direct exact-ROM boot with no freeze/chooser; quit behavior follows DraStic's launcher semantics.
- **Occurrences (4):** PRE 97f540e4 n/a [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE 97f540e4 n/a [PR #92 (comment 2026-10-06)]; PRE  n/a [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE B22 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-LQT-006` Return host on default path: Gen I-III RetroArch crash on launch (698ea4d1, 2d2d7bc2)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III · **Area:** launch-quit · **Category:** defect
- **Last owner result:** PRE/0760334f: PASS
- **Basis:** 8803c9ea restored the direct handoff; owner accepted 0760334f ('100% pass', D0337; A0924 'Owner-accepted with the direct handoff'). GameLauncher.cpp changed after 0760334f (core-from-save choice 7ed1cc16, returnHostReadiness), and no later owner RetroArch launch result exists.
- **Repair commits:** 8803c9ea fix(launch): restore hardware-safe RetroArch startup
- **Automated tests:** tests/test_retroarch_launch_resolution.cpp : cartridge header identity, Red vs FireRed guards; tests/test_game_launch_model.cpp : argv tokenisation
- **Hardware acceptance:** 0760334f (PRE)
- **Next action:** Add one Gen I-III RetroArch launch to R12 smoke (launch path untested since 0760334f).
- **Owner** (A0923): > crash on launch
- **Owner** (D0334): > - Red / Blue / Yellow, Gold / Silver / Crystal, Ruby / Sapphire / FireRed / LeafGreen / Emerald now crash when launched through RetroArch
- **Occurrences (8):** PRE 698ea4d1 FAIL [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE 2d2d7bc2 FAIL [PR #92 (body, updated 2026-10-12)]; PRE 0760334f PASS [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE 3 n/a [docs/hardware/2026-10-05-hotpath-validation.md]; PRE 5 n/a [docs/hardware/2026-10-05-hotpath-validation.md]; PRE 97f540e4 PASS [docs/handoffs/LAUNCH_RETURN_PROGRESS.md]; PRE 97f540e4 PASS [PR #92 (comment 2026-10-06)]; PRE 0760334f PASS [PR #92 (body, updated 2026-10-12)]

### `PBX-LQT-007` B on Product Home must never quit without a visible Y-confirmed decision (Quit guard)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** launch-quit · **Category:** safety
- **Last owner result:** R3/62a2f8c3/C02: PASS
- **Basis:** R1 7fea1797 C02 PASS; R3 62a2f8c3 C02 PASS (B0485). Decision logic unchanged (include/UI/HomeQuitConfirm.h decide()), but the dialog drawing changed in aace6021 (two buttons B Cancel / Y Quit, empty nav bar; src/UI/SaveSelectScreen.cpp:5321-5352).
- **Repair commits:** aace6021 Round 11 H39 + QUIT: '-' cancels SD browsers; two-button quit dialog
- **Automated tests:** tests/test_home_input_guard.cpp : only Y quits; tests/test_game_hub_contract.py : two-button B Cancel / Y leave decision, empty nav bar; tests/test_leave_decision_layout.cpp : buttons from buttonRect(..,2); text fits
- **Hardware acceptance:** 62a2f8c3 R3 C02; recheck 066e1db9 R12 H02 · R12 item H02
- **Next action:** Owner runs R12 H02.
- **Occurrences (13):** PRE CG07 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; R1 7fea1797 C02 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C02 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 C02 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C02 PASS [tests/test_hw_round1_regression_gate.py]; R2 C02 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C02 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C02 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-LQT-008` Launch requires confirmation (ZR -> Y); launch menu resize and coloured A/B glyph buttons

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** launch-quit · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-LAUNCH: PASS
- **Basis:** R3 62a2f8c3 R2-LAUNCH 'Game Launch confirmation' PASS (B0476) and C03 PASS (B0486). Launch dialog redrawn in aace6021 (B Cancel / Y Launch, no A Stay).
- **Repair commits:** (R2-LAUNCH Phase 6, e10f307f); aace6021 two-button leave decision
- **Automated tests:** tests/test_leave_decision_layout.cpp : all four quit-route notices and long game names fit; tests/test_game_hub_contract.py : CG07 single launch site behind Y
- **Hardware acceptance:** 62a2f8c3 R3 R2-LAUNCH; recheck 066e1db9 R12 H02 · R12 item H02
- **Next action:** Owner runs R12 H02 including the Launch question.
- **Owner** (B0059): > the launch menu also needs resizes and the A and B proper colored buttons added instead of just the text.
- **Occurrences (18):** R1 7fea1797 C03 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C03 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 C03 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C03 PASS [tests/test_hw_round1_regression_gate.py]; R2 Launch menu (Phase 6) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 C03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f Launch menu (Phase 6) n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f C03 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-LAUNCH PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 C03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-LAUNCH PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-LQT-009` melonDS/DeSmuME DS save must launch the matching RetroArch core (core chosen from game ID only)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV, V · **Area:** launch-quit · **Category:** defect
- **Last owner result:** none
- **Basis:** Source defect confirmed (B0599; LAUNCH_RETURN_PROGRESS.md:89 'defaultRetroArchCore() chose DeSmuME first from the game ID alone'), corrected on #307 at 7ed1cc16 (in 62a2f8c3). Launch dialog notice from gameLaunchDialogNotice() (src/UI/GameLauncher.cpp:1445, used at src/UI/SaveSelectScreen.cpp:2160). R3 checklist B0438 has no owner result.
- **Repair commits:** 7ed1cc16 Merge claude/gen14-campaign-20261015 (core-from-save evidence, CG27)
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG27-retroarch-core-from-save-evidence
- **Hardware acceptance:** none (not on any checklist; R3-1..R3-3 steps in LAUNCH_RETURN_PROGRESS.md:116-118)
- **Next action:** Owner with a melonDS-core DS save runs R3-1/R3-3 steps.
- **Occurrences (2):** PRE 4 n/a [docs/hardware/2026-10-05-hotpath-validation.md]; R3 62a2f8c3 Part 9 #4 n/a [docs/hardware/RETEST_62a2f8c3.md]

### `PBX-LQT-010` Extra 5: quit dialog cleanup - no highlighted Y, no redundant footer, drop 'Stay' (Cancel only)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** launch-quit · **Category:** request
- **Last owner result:** none
- **Basis:** aace6021: Product Home leave decision draws two neutral buttons B Cancel / Y Quit\|Launch, no accent fill, empty nav bar (src/UI/SaveSelectScreen.cpp:5328-5352); Bank/Storage exit prompt B relabelled Cancel. Note: E0056 also carries SPRITE (species picker single picture, eb7fe8c9, R12 H16) and CREATE-MOVES requests owned by species-change/create.
- **Repair commits:** aace6021 Round 11 H39 + QUIT: '-' cancels SD browsers; two-button quit dialog
- **Automated tests:** tests/test_game_hub_contract.py : leave-decision pins B Cancel / Y with empty nav bar; Storage B Cancel; tests/test_leave_decision_layout.cpp : two buttons
- **Hardware acceptance:** 066e1db9 R12 H02 · R12 item H02
- **Next action:** Owner runs R12 H02.
- **Owner** (E0056): > The quit option in the nro has a highlighted option in the Y exit option, it also has a footer which isn't needed cause the buttons we need are the options in the menu, can we also get rid of "stay" we have cancel in that menu and those 2 options are the same just different names. In the species picker instead of having the 2 pictures one shiny and one normal can we have it where its 1 photo and if we can shiny or not it changes the photo, instead of the 2 photo thing. Gen 1/2 have moves when editor is opened, it should be blank, same as for gen 1-5 when a pokemon is selected it should leave moves clear not add or fill the moves just cause that pokemon was selected in species selector
- **Occurrences (3):** R11 3634d1e6 Extra 5 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 5 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R12 066e1db9 H02 n/a [docs/hardware/RETEST_066e1db9.md]

### `PBX-MET-001` Gen IV/V Met Location choices too broad (all 108/276 locations at Met Lv5); show source-backed wild context incl. level, preserve encounter filtering, never falsely reject legitimate origins

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** IV, V · **Area:** encounters-met · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R6/26186464/A15: PASS
- **Basis:** Owner FAIL R3; repaired c9a457a3; PASS OCT24, R5 1ca2faec and R6 26186464 A15 with decision "Preserve encounter filtering". Owner reversal/extension: the Nov 1 owner message (E0117) asks for full native choices distinguished from verified encounters; R12 H07 expects catches first then every other location marked "Not a listed catch; unverified". include/Integration/Encounter/EncounterContext45.h changed after 26186464.
- **Repair commits:** aff41765 fix(gen4): align create editor with classic parity (met level context); c9a457a3 fix(editor): show source-backed Gen IV/V location context
- **Automated tests:** tests/test_gen45_encounter_context.cpp : Catalogue context rows for Gen IV/V
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** R12 H07 on Gen IV/V: verify catches-first ordering still honours "Preserve encounter filtering".
- **Owner** (C0295): > Preserve encounter filtering
- **Owner** (B0561): > E.g. Walrein can seemingly choose all 108/276 locations; distinguish native values from legal wild/egg/event/evolution/transfer histories, do not falsely reject legitimate origins.
- **Owner** (D0320): > - Met Location rows lacked level context.
- **Occurrences (15):** PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]; PRE C16 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE D10 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 62a2f8c3 NEW-12 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-12 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-12 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-12 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-12 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-12 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A15 · NEW-12 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A15 · NEW-12 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A15 · NEW-12 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A15 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-MET-002` Crystal Met Time: A cycles the value inline (no menu)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** encounters-met · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-CRYSTAL: PASS
- **Basis:** Owner PASS R3 62a2f8c3 R2-CRYSTAL; tests/test_crystal_met_time_inline.cpp. Gen II workspace (src/UI/Gen2UnifiedPokemonWorkspace.inc) changed after 62a2f8c3 including Round 11.
- **Repair commits:** e824fc97 feat(gen2): real Special Attack/Defense in the VALUES column; Crystal Met Time cycles inline
- **Automated tests:** tests/test_crystal_met_time_inline.cpp : A cycles Met Time in place
- **Hardware acceptance:** 62a2f8c3 R3 · R12 item H08
- **Next action:** Observe Met Time during R12 H08.
- **Owner** (B0066): > the met time in pokemon crystsal needs to be changed so i can press A on it and it changes instead of opening a menu to choose.
- **Occurrences (8):** PRE met-time PASS [docs/qa/GEN1_5_MATRIX.md]; PRE met-time NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 Crystal Met Time (Phase 11) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f Crystal Met Time (Phase 11) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-CRYSTAL PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-CRYSTAL PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-MET-003` Crystal Met Location chooser proportional and names only (no raw IDs); Gen I has no met location

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II · **Area:** encounters-met · **Category:** request
- **Last owner result:** R3/62a2f8c3/L02: PASS
- **Basis:** Owner PASS R1 7fea1797 and R3 62a2f8c3 L02. Crystal picker rewritten in Round 11 to an open native list (03387c48; src/UI/Gen2UnifiedPokemonWorkspace.inc:579-609, include/UI/Gen2PokemonPickerModel.h:42-75). The R3 PASS note "Further compactness wanted" (B0518) is tracked separately.
- **Repair commits:** 47cac826 feat(gen2): proportional Crystal Met Location chooser with names only; 03387c48 Crystal Met Location: same open native list as Gen III (R11 FRLG, Crystal path)
- **Automated tests:** tests/test_gen2_location_picker.cpp : picker geometry fits the screen
- **Hardware acceptance:** 066e1db9 R12 H08 · R12 item H08
- **Next action:** Owner runs R12 H08.
- **Owner** (B0517): > Further compactness wanted.
- **Occurrences (10):** PRE 11 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 L02 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 L02 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 L02 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 L02 PASS [tests/test_hw_round1_regression_gate.py]; R2 L02 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f L02 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 L02 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 L02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MET-004` Gen III Met Location picker cleaned up: readable names without (RSE)/(FRLG)/ID numbers

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** III · **Area:** encounters-met · **Category:** request
- **Last owner result:** R3/62a2f8c3/L03: PASS
- **Basis:** Owner PASS R1 7fea1797 and R3 62a2f8c3 L03 (note B0520 tracked in the compact-popup issue). Gen III picker rows were rebuilt in Round 11 (01951ab7; include/UI/Gen3EncounterPickerModel.h:100-175).
- **Repair commits:** 01951ab7 Gen III Met Location: exact-origin native list, never fail closed (R11 FRLG)
- **Automated tests:** tests/test_gen3_exact_game_met_locations.cpp : exact-origin native rows
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** Check row names during R12 H07 (FR/LG).
- **Owner** (B0519): > Unnecessary \`not stored\` filler; too much blank space in modal.
- **Occurrences (8):** R1 7fea1797 L03 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 L03 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 L03 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 L03 PASS [tests/test_hw_round1_regression_gate.py]; R2 L03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f L03 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 L03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 L03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MET-005` Met Location popup too tall / unused space; compact 620 px window for every row, no clipping (NEW-10, incl. L03/L04 notes)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** III, IV, V · **Area:** encounters-met · **Category:** defect
- **Last owner result:** R6/26186464/A26: PASS
- **Basis:** Owner FAIL R3 and OCT24; PASS OCT25 1ca2faec and R6 26186464 A26 with decision "Keep the compact popup". Row content changed in Round 11 (unverified native rows added, include/UI/Gen3EncounterPickerModel.h:137-139; src/UI/SharedPokemonEditorSurface.inc changed after 3634d1e6).
- **Repair commits:** d361d86f Round 5 NEW-10: no clipped Met Location rows in the 620 px window
- **Automated tests:** tests/test_met_location_fit.cpp : 620 px window, detail fitted beside name; tests/test_met_location_rows_distinct.cpp
- **Hardware acceptance:** 066e1db9 R12 H07 · R12 item H07
- **Next action:** Confirm the compact popup still fits in R12 H07.
- **Owner** (C0306): > Keep the compact popup
- **Owner** (B0559): > Remove unused middle spacing across generations while preserving scrolling.
- **Owner** (B0521): > Scrolling/look good; compact unused space.
- **Occurrences (21):** R1 7fea1797 L04 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 L04 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 L04 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 L04 PASS [tests/test_hw_round1_regression_gate.py]; R2 L04 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f L04 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 L04 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 L03 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 L04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 L04 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-10 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-10 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-10 FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-10 n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R5 1ca2faec NEW-10 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-10 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec NEW-10 PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A26 · NEW-10 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A26 · NEW-10 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A26 · NEW-10 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A25 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-MET-006` Gen III (all five games, reported on FR/LG) and Crystal Met Location pickers fail closed / too narrow (1,267 species-game combos with no row; Ruby Pikachu only one option); owner also suspected FR/LG species roster reads RSE

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II, III · **Area:** encounters-met · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/Extra 2: FAIL
- **Basis:** Owner FAIL R11 3634d1e6 Extra 2. Verified in current source: Gen III picker offers catch templates of the record's origin game, then every other native location, then trade/egg/event markers (include/UI/Gen3EncounterPickerModel.h:100-139; include/UI/Gen3SharedPokemonSession.h:213-260 metLocationPicker/metLocationChoosable); the "No supported exact-game encounter templates" refusal no longer exists in src/include. Crystal uses the same open list (src/UI/Gen2UnifiedPokemonWorkspace.inc:579-609). Species roster: intake FRLG skeptic note — no RSE species list is read; all 1-386 were and are selectable (not a defect; non-native origin is the open Create-origin decision). R2-BUG-008 keep-stored row PASS R3 is preserved.
- **Repair commits:** 01951ab7 Gen III Met Location: exact-origin native list, never fail closed (R11 FRLG); 03387c48 Crystal Met Location: same open native list as Gen III (R11 FRLG, Crystal path); 479170d0 WIP: fix(gen3): Met Location picker keeps a stored non-template origin (R2-BUG-008)
- **Automated tests:** tests/test_gen3_exact_game_met_locations.cpp : every species in R/S/E/FR/LG and Crystal has a choosable row; export -> reparse; tests/test_gen3_hardware_surface_contract.cpp : rowValuesWithNative wiring; tests/test_gen3_encounter_keep_stored.cpp : R2-BUG-008 keep-stored row
- **Hardware acceptance:** 066e1db9 R12 H07 + H08 · R12 item H07
- **Next action:** Owner runs R12 H07 (FR/LG Rayquaza or Charizard) and H08 (Crystal).
- **Owner** (E0117): > Gen III and Crystal Met Location pickers are incomplete. Independent tests found 1,267 species/game combinations with no picker row. The encounter-filtered list is too narrow for ordinary editing. Provide the appropriate native Met Location choices and distinguish format-valid choices from historically verified encounters. Do not invent encounter legality.
- **Owner** (E0081): > leaf green and fire red on gba so gen 3 dont have a working met location for pokemon … i thik it is reading RSE for species … i dont think i can get a rayquaza in leaf green or fire red.
- **Owner** (E0053): > leaf green and fire red on gba so gen 3 dont have a working met location for pokemon, i dont think the proper species for those games in in and i thik it is reading RSE for species, like i dont think i can get a rayquaza in leaf green or fire red.
- **Occurrences (11):** R3 62a2f8c3 R2-BUG-008 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-008 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-008 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-008 PASS [tests/test_hw_round1_regression_gate.py]; R11 3634d1e6 Extra 2 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [owner message 2026-11-01 (independent Round 11 QA)]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; R12 066e1db9 H08 n/a [docs/hardware/RETEST_066e1db9.md]; REQ Extra 2 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ FRLG n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-MET-007` Gen V (Black 2) event/location rows indistinguishable (276 locations, repeated POKEPARK/Festa, "Event/special location; unverified")

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** encounters-met · **Category:** defect
- **Last owner result:** R6/26186464/B05: FAIL
- **Basis:** Latest owner result R6 26186464 B05 FAIL ("many rows still say Event/special location; unverified"). Round 7 repair: sourced event/met-location kinds and focused-row detail line (include/Names/SpecialMetLocationNotes.h, 149 lines, added after 26186464). Round 7-10 checklists (B03/C09) have no owner results and the item is not on the R11 worksheet or R12 checklist.
- **Repair commits:** b421c1c6 feat(B05): Gen IV / V event and marker met locations say what pinned tables show; a273d88e Merge lane P e52b7d97: sourced event/met-location kinds and focused-row detail (B05)
- **Automated tests:** tests/test_special_met_location_notes.cpp : each special/event row has a sourced description; tests/test_event_year_source.cpp
- **Hardware acceptance:** none (not on any checklist) — needs 066e1db9 R12 add-on
- **Next action:** Add a Black 2 event Met Location step to the R12 checklist (focus a POKEPARK/Festa row, read its detail line).
- **Owner** (C0316): > The names now differ, but many rows still say "Event/special location; unverified". The owner wants to understand what each choice means and how it differs from the others.
- **Owner** (C0169): > Gen V historical event and location choices cannot be told apart: several rows read POKÉPARK, and Festa rows differ only by number.
- **Owner** (B0548): > Black 2 Pikachu picker shows 276 locations, likely too broad; verify native location ID vs actual encounter viability.
- **Occurrences (14):** R3 62a2f8c3 R2-CHECK-011 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-CHECK-011 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 R2-CHECK-011 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-CHECK-011 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 9 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-CHECK-011 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec R2-CHECK-011 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec R2-CHECK-011 FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P5 n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B05 · R2-CHECK-011 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 B05 · R2-CHECK-011 FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B05 (A15/A26 preserved) n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 B03 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 C09 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-MET-008` Crystal Met Location picker: owner wants further compactness (note on R3 L02 PASS)

- **Status:** Needs Investigation · **Gen:** II · **Area:** encounters-met · **Category:** request
- **Last owner result:** R3/62a2f8c3/L02: PASS with request
- **Basis:** NEW-10 compaction (620 px) covered Gen III-V only; tests/test_met_location_fit.cpp:45 still calls Crystal's 640 px window "approved". Round 11 made the Crystal list longer (every landmark). Unknown: whether the owner still wants the Crystal popup made smaller; no later Crystal-specific owner result.
- **Automated tests:** tests/test_met_location_fit.cpp : 620 px window, <= Crystal 640 px
- **Hardware acceptance:** 066e1db9 R12 H08 (comment) · R12 item H08
- **Next action:** Ask the owner in R12 H08 comment whether the Crystal Met Location popup size is acceptable.
- **Owner** (B0518): > Further compactness wanted.
- **Occurrences (1):** R3 62a2f8c3 L02 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MOV-001` Game/species-aware move compatibility (filtered move picker, visible compatibility status, exact-game native PP) across Gen I-V

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** moves · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H26: PASS (Gen V); R3/62a2f8c3/M06,M07: PASS
- **Basis:** Gen I semantic compatibility accepted 69668bc8 (A0547); Gen II filtered move picker + empty-move handling accepted cfb70357 (D0303); M06/M07 PASS R3 62a2f8c3; Gen V species-restricted chooser PASS R11 3634d1e6 H26. Compatibility predicates are also used by the Round 11 species-change code (include/UI/SpeciesChangeMoves.h:12).
- **Repair commits:** c7355c57 feat(gen1): Gen I moves on the shared Moves panel; ddd52f55 feat(gen2): Gen II moves on the shared Moves panel; 83247b81 fix(gen5): filter Move picker by Pikachu/species learnsets, transfer and preserved histories (#326)
- **Automated tests:** tests/test_gen5_move_compatibility.cpp : BW/B2W2 species pools; tests/test_hw_round1_regression_gate.py : M06/M07 guards
- **Hardware acceptance:** 3634d1e6 R11 H26 · R12 item H18
- **Next action:** No new action beyond R12 smoke.
- **Owner** (D0293): > - visible move-status/compatibility summary needs to be restored while remaining distinct from encounter legality.
- **Occurrences (9):** PRE 6ecd9c28 2 n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]; PRE C15 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE D09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Phase 2 n/a [docs/V1_ROADMAP.md]; REQ C15 / C16 / D09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Generation IV G4-04 n/a [docs/GAME_SUPPORT_MATRIX.md]; REQ Design goals n/a [README.md]

### `PBX-MOV-002` Shared Moves panel and move chooser across Gen I-V: alignment, number/name/type/power/accuracy/PP (Gen V showed name+PP only; Gen IV appended raw IDs)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** moves · **Category:** defect
- **Last owner result:** R3/62a2f8c3/M06: PASS
- **Basis:** Owner PASS R1 7fea1797 M01-M07 and R3 62a2f8c3 M01-M03, M06, M07 (M04/M05 regressed separately, see category issue). include/UI/SharedMovesPanel.h changed after 62a2f8c3 (2 lines) and per-gen surfaces changed in Round 11.
- **Repair commits:** 888b886c data(gen5): cited BW/B2W2 move type, category, power, accuracy and PP table; de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface; aff41765 fix(gen4): align create editor with classic parity; 307976f3 feat(gen1/gen2): Gen I/II move choosers use the shared Move chooser rows
- **Automated tests:** tests/test_hw_round1_regression_gate.py : M01-M07 per-ID guards
- **Hardware acceptance:** 62a2f8c3 R3 · R12 item H18
- **Next action:** R12 H18 smoke of the Moves panel.
- **Owner** (D0319): > - Gen IV Move picker appended raw move IDs
- **Occurrences (59):** PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]; PRE  n/a [docs/GEN4_ITEMS_AND_PICKER_POLISH_2026-10-09.md]; PRE 35cbf163 P5 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE 1 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE 1 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 M01 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M02 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M03 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M04 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M05 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M06 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M07 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M01 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M02 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M03 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M04 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M05 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M06 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M07 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 M01 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M02 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M03 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M04 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M05 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M06 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M07 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M01 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M02 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M03 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M04 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M05 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M06 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 M07 PASS [tests/test_hw_round1_regression_gate.py]; R2 M01 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M02 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M04 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M05 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M06 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 M07 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f M01 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M02 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M03 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M04 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M05 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M06 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f M07 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 M01 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M02 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M05 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M06 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M07 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 M02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 M03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 M06 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 M07 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MOV-003` Gen I hidden-row appearance in Create/Moves (rows/cursor outlines outside the panel)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** moves · **Category:** defect
- **Last owner result:** R3/62a2f8c3/M08: PASS
- **Basis:** A0956 (awaiting owner detail on the Gen I Create "hidden row") was answered by M08 "Gen I hidden-row appearance fixed": owner PASS R1 7fea1797 and R3 62a2f8c3. Gen I overlay files changed in Round 11.
- **Automated tests:** tests/test_hw_round1_regression_gate.py : M08 guard (closest list-fit / moves-panel tests; device-only)
- **Hardware acceptance:** 62a2f8c3 R3 M08 · R12 item H18
- **Next action:** R12 H18 smoke on Gen I.
- **Occurrences (9):** PRE  n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; R1 7fea1797 M08 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 M08 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 M08 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 M08 PASS [tests/test_hw_round1_regression_gate.py]; R2 M08 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f M08 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 M08 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M08 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MOV-004` Gen V PP / PP Ups captions too close together

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** moves · **Category:** defect
- **Last owner result:** R3/62a2f8c3/S01: PASS
- **Basis:** Owner FAIL R1, PASS R2 e10f307f and R3 62a2f8c3 S01. Gen V surface changed in Round 11.
- **Repair commits:** b1d34f96 S01 + Phase 9: PP / PP Ups captions apart; Move chooser shows 10 rows
- **Automated tests:** tests/test_hw_round1_regression_gate.py : S01 GUARDS_ROUND2
- **Hardware acceptance:** 62a2f8c3 R3 S01 · R12 item H18
- **Next action:** R12 H18 smoke.
- **Owner** (D0227): > **Six unchecked items with reported problems:** B01/B02/B03/B06 (Gen I–III Bag Add-before-quantity workflow), S01 (Gen V PP/PP Ups crowded), S03 (Gen V Create/Edit clipped field text and parity).
- **Owner** (B0048): > S01 the pp and PP ups text are to close together so it look unreable/sloppy.
- **Occurrences (10):** R1 7fea1797 S01 FAIL [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 S01 FAIL [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 S01 FAIL [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 S01 FAIL [issue #313 (comment 2026-10-17)]; R2 S01 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f S01 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f S01 PASS [tests/test_hw_round1_regression_gate.py]; R2 e10f307f S01 PASS [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R3 62a2f8c3 S01 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 S01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-MOV-005` Move chooser should show 1-2 more rows instead of blank space (all gens)

- **Status:** Hardware Accepted · **Gen:** all · **Area:** moves · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-MOVECHOOSER: PASS
- **Basis:** Owner PASS R3 62a2f8c3 R2-MOVECHOOSER (10-row chooser). Move picker presentation headers (include/UI/MovePickerPresentation.h, MovePickerRowUI.h) unchanged since 62a2f8c3.
- **Repair commits:** b1d34f96 S01 + Phase 9: PP / PP Ups captions apart; Move chooser shows 10 rows
- **Automated tests:** tests/test_hw_round1_regression_gate.py : R2-MOVECHOOSER GUARDS_ROUND3
- **Hardware acceptance:** 62a2f8c3 R3
- **Next action:** None.
- **Owner** (B0062): > the moves sector menu look like it could have 1 or 2 more rows instead of that blank space, this is for all gens.
- **Occurrences (6):** R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 Move chooser rows (Phase 9) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f Move chooser rows (Phase 9) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-MOVECHOOSER PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-MOVECHOOSER PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-MOV-006` Gen IV/V move Category element crowds the selected move Power/Accuracy section

- **Status:** Hardware Accepted · **Gen:** IV, V · **Area:** moves · **Category:** defect
- **Last owner result:** R6/26186464/A03 (M04): PASS
- **Basis:** Owner FAIL R3 62a2f8c3 M04/M05; repaired 88f974da (own Category line + type-badge gutter); owner PASS OCT24 c60d4da0, R5 1ca2faec, R6 26186464 A03 (Gen IV); Gen V Moves M05 PASS R5 and R6 A24. include/UI/SharedEditorChrome.h / SharedMovesPanel.h unchanged since 26186464.
- **Repair commits:** 88f974da Stage native PK5 language and unique inventory with zero creation stats and shared UI repairs
- **Automated tests:** tests/test_hw_round1_regression_gate.py : M04 GUARDS_ROUND5
- **Hardware acceptance:** 26186464 R6 A03
- **Next action:** None.
- **Owner** (B0497): > Same category presentation issue as Gen IV.
- **Owner** (B0496): > New category element crowds selected Move Power/Accuracy section.
- **Occurrences (12):** R3 62a2f8c3 M04 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 M04 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 M05 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 M04 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 M05 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 M04 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 M04 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec M04 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec M04 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A03 · M04 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A03 · M04 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A03 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-MOV-007` Gen V move picker listed all 559 moves (e.g. Pikachu) instead of the species/form learnset

- **Status:** Hardware Accepted · **Gen:** V · **Area:** moves · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H26: PASS
- **Basis:** Owner FAIL R4 c60d4da0 and OCT24 (M05); repaired 83247b81 (#326); owner PASS OCT25 1ca2faec, R6 26186464 A24, and R11 3634d1e6 H26 "Legal move chooser list". include/Integration/Gen5/Gen5MoveCompatibility.h unchanged since 26186464.
- **Repair commits:** 83247b81 fix(gen5): filter Move picker by Pikachu/species learnsets, transfer and preserved histories (#326)
- **Automated tests:** tests/test_gen5_move_compatibility.cpp : species/form pools, Learnable/Transfer/Preserved
- **Hardware acceptance:** 3634d1e6 R11 H26
- **Next action:** None.
- **Owner** (D0214): > Owner hardware feedback: Gen V Pikachu Move chooser shows ~559 native moves instead of only species-appropriate moves (Gen I–IV already filter).
- **Occurrences (11):** R4 c60d4da0 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 FAIL [issue #326 (body 2026-10-24)]; OCT24 c60d4da0 M05 FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 7 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec M05 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec M05 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec M05 PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A24 · M05 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A24 · M05 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A23 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R11 3634d1e6 H26 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-MOV-008` Gen V "Learnable" move label should use the same OK treatment as Gen I-IV (7B)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** moves · **Category:** request
- **Last owner result:** R6/26186464/C02: PASS
- **Basis:** Owner PASS R6 26186464 C02 (OCT26 guard). Gen V surface (src/UI/Gen5SharedReadOnlySurface.inc) changed in Round 11.
- **Automated tests:** tests/test_hw_round1_regression_gate.py : C02 GUARDS_ROUND7
- **Hardware acceptance:** 26186464 R6 C02 · R12 item H18
- **Next action:** R12 H18 smoke.
- **Occurrences (5):** R5 1ca2faec 7B n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 C02 · 7B n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C02 · 7B PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 C02 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A34 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-OPN-001` Older Legends: Arceus save crashes on open (#24)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** open-save · **Category:** defect
- **Last owner result:** PRE/3be4de6b: FAIL
- **Basis:** Hardening 0ea98cc1 (bounded SC decoding, block validation; docs/SESSION2_6_SAFETY_IMPLEMENTATION.md:40-54) explicitly 'DEVICE FIXED is not claimed', #24 left open. The exact failing save was never retested. Out of the current Gen I-V release scope (CURRENT_STATUS.md:5).
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads
- **Automated tests:** Session 2.6 synthetic PLA/container host suites
- **Hardware acceptance:** none (not on any checklist; Gen VI-IX out of scope)
- **Next action:** Retest the exact old PLA save when Switch-native titles return to scope.
- **Owner** (A0081): > REPRODUCIBLE CRASH
- **Occurrences (10):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b PARTIAL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE af2acf04 checklist-item [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 checklist-item [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 checklist-item [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE 2 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 2 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]

### `PBX-OPN-002` Gen V Home A opens the assigned save directly (no second chooser; shared hub, never direct-to-Party)

- **Status:** Hardware Accepted · **Gen:** V · **Area:** open-save · **Category:** defect
- **Last owner result:** R6/26186464/A18: PASS
- **Basis:** fa9cfcea FAIL (direct-to-Party), R3 FAIL (extra chooser, B0565); R5 and R6 26186464 A18·NEW-16 and A29·C01 PASS (C0298, C0309); R11 H02 (open one save per generation) PASS. Round 11 diff of SaveSelectScreen.cpp touches only recovery, '-' browser cancel, HOME card and quit dialog, not the open route.
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND5/6 NEW-16, C01 (CG57)
- **Hardware acceptance:** 26186464 R6 A18
- **Next action:** None (H18 smoke covers opening).
- **Owner** (B0565): > Home A should directly open currently assigned/read save; change save via Game Card + → Source/Game File → Refresh/Choose Save or Backups.
- **Occurrences (17):** PRE fa9cfcea FAIL [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE Assignment checklist-item [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE C01 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 62a2f8c3 NEW-16 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 C01 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 NEW-16 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-16 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 C01 NOT TESTED [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec NEW-16 checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec C01 (unmarked) checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-16 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec C01 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A18 · NEW-16 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A29 · C01 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A18 · NEW-16 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A29 · C01 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A18 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-OPN-003` Strict open/validate: exact game identity, ambiguous held back, corrupt input refused without crash; untouched round-trip

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** open-save · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** none (refusal path); R11/3634d1e6/H02 PASS for ordinary open
- **Basis:** Host gates (qa-matrix identity/discovery, parser fuzzing 24e730a0). Hardware: ordinary saves open (R11 H02 PASS) but corrupt/truncated refusal was never owner-tested (GEN14 C7, GEN15 B14 unfilled).
- **Repair commits:** 24e730a0 test(qa): Gen I-V parser fuzzing and export-safety contracts
- **Automated tests:** QA matrix identity cell; parser fuzz suites
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Host gate; optional hardware check with a copied truncated file.
- **Occurrences (5):** PRE Open requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; PRE Validate requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; PRE C7 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE B14 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Round-trip invariant requirement [docs/SAVE_SAFETY.md]

### `PBX-OPN-004` Native Shield / Scarlet-Violet / Legends Z-A do not open from Product Home

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** open-save · **Category:** defect
- **Last owner result:** PRE/0760334f: PASS
- **Basis:** 2d2d7bc2 FAIL; owner reported '100% pass' on 0760334f (D0337, 2026-10-08). Large code churn since; native titles outside current Gen I-V release scope.
- **Hardware acceptance:** 0760334f PRE (accepted); not in Gen I-V checklists
- **Next action:** Recheck when native titles return to scope.
- **Owner** (D0335): > - Shield / Scarlet-Violet / Legends Z-A do not open from Product Home or Games; the attempted native open returns immediately to the card
- **Occurrences (1):** PRE 2d2d7bc2 FAIL [PR #92 (body, updated 2026-10-12)]

### `PBX-OTH-001` Original/installed save never written: live installed-save writes hard-locked (no live-write safety regression)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** other · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** PRE/3be4de6b/(follow-up 2026-09-03) LIVE INSTALLED SAVE WRITE OBSERVED: PASS (NO); related R11/3634d1e6/H14: PASS (export-copy area)
- **Basis:** include/Safety/WritePolicy.h:18 LIVE_SAVE_WRITES_ENABLED=false (constexpr, unchanged since c618bd5e 2026-09-01); canWriteTo() :25 refuses SaveDestination::LiveGame. Owner hardware: PRE 3be4de6b 'LIVE INSTALLED SAVE WRITE OBSERVED: NO' and 'ORIGINAL INSTALLED SAVE CHANGED: NO' (A0215/A0216). Latest owner evidence that originals stay unchanged is in other areas (R6 26186464 S05 PASS; R11 3634d1e6 H14 'Export Copy is separate' PASS). Host-only PASS cells (A0819, A0858) are NOT hardware evidence. Export/Review code changed in the Round 11 repairs, so re-verify.
- **Repair commits:** c618bd5e safety: hard-lock live game save writes
- **Automated tests:** tests/test_write_policy.cpp : LiveGame destination refused while LIVE_SAVE_WRITES_ENABLED is false; tests/test_ci_repository_safety_contract.py : repository safety contract
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** In R12 H17 record the original save's SHA-256 before and after Export Copy (A0589 source-integrity proof) instead of a visual 'unchanged'.
- **Owner** (A0215): > NO — tester reports unchanged in exercised flow
- **Owner** (A0216): > NO
- **Occurrences (8):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE fc1e6de2 NOT TESTED [docs/DEVICE_TEST_CHECKLIST.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b PASS [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE immutability PASS [docs/qa/GEN1_5_MATRIX.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]

### `PBX-OTH-002` Final original-save integrity proof never performed: launch original game / pre-post SHA-256 / concurrent emulator writing the same save

- **Status:** Needs Investigation · **Gen:** all · **Area:** other · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** PRE/3be4de6b/LIVE INSTALLED SAVE WRITE: NOT TESTED ('NOT PROVEN')
- **Basis:** No owner record anywhere of launching the original game after a PokeBank session, nor of a pre/post SHA-256 of the original save (the GEN5_SAVE_INSTANCE_HARDWARE_SMOKE rule, A0589). docs/qa/GEN1_5_MATRIX.md lists 'emulator running concurrently against the same save' as hardware-only residue (A0845). Owner rule in R3 (B0595) says never require owner live emulator write testing, so the concurrent-emulator part may be intentionally out of owner scope; unknown: whether SD FAT semantics / concurrent writes are ever verified. R12 H17 checks only that the original 'is unchanged', with no SHA step.
- **Automated tests:** tests/test_qa_export_contracts.cpp : export writes a separate file, source bytes unchanged (host synthetic)
- **Hardware acceptance:** 066e1db9 R12 H17 (partial: no SHA or game-launch step) · R12 item H17
- **Next action:** Owner decision: either add a pre/post SHA-256 + launch-in-original-game step to R12 H17, or record that concurrent-emulator and game-launch verification are out of owner scope.
- **Owner** (A0120): > LIVE INSTALLED SAVE WRITE: NOT PROVEN
- **Occurrences (7):** PRE 3be4de6b NOT TESTED [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE  n/a [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE immutability NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE  NOT TESTED [docs/qa/SAVE_SAFETY_CHAIN.md]

### `PBX-OTH-003` Inherited PKSE mutation UI (Release/Create/Move-Multi/Edit/Save) reachable on installed-game sources (#23)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** PRE/3be4de6b/USER-FACING READ-ONLY CONTRACT: FAIL
- **Basis:** include/Safety/SourceMutationPolicy.h:30-33 canPerform() allows only View on InstalledGame/HistoricalBackup/legacy sources; src/UI/UI.cpp:611 opens cart/installed saves as SourceKind::InstalledGame; src/UI/TrainerViewScreenBase.inc:5226/:5401 gate Edit through canPerform; trace tag INSTALLED_READ_ONLY at :646. The PRE second-device checklist rows (E. Installed-game mutation safety, A0271–A0278) were never filled in, and no later round re-tested installed (Switch-native) sources, which are outside the Gen I–V gate.
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads
- **Automated tests:** tests/test_source_mutation_policy.cpp : every mutation except View is refused for InstalledGame/legacy/historical sources
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add one R12 or later row: open an installed Switch-native save, confirm Release/Create/Move/Edit/Save are absent or refused. Otherwise #23 stays unverified.
- **Owner** (A0121): > USER-REACHABLE MUTATION UI: PROVEN
- **Owner** (A0219): > FAIL / inherited mutation UI remains
- **Occurrences (5):** PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 1 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 1 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]

### `PBX-OTH-004` Handheld and docked UI pass (docked mode never owner-tested as a whole)

- **Status:** Needs Investigation · **Gen:** app · **Area:** other · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** PRE/3be4de6b/Docked mode: NOT TESTED (handheld readability/focus: PASS)
- **Basis:** Handheld readability/focus PASS on PRE 3be4de6b (A0200/A0201). Docked was NOT TESTED there, and no later round records a general docked pass (only feature-specific docked checks such as Oct 24 NEW-06 docked Species keyboard). v1 gates require it: docs/RELEASE_CHECKLIST.md 'handheld + docked UI passes' (F0173), docs/V1_ROADMAP.md Phase 13 (F0171). Unknown: whether R11 testing was handheld or docked. R12 H01 mentions docked only for the HOME card.
- **Hardware acceptance:** 066e1db9 R12 H01 (HOME card only) · R12 item H01
- **Next action:** Add a docked whole-app pass row (TV output, text/focus/footers on every main screen) to the #313 full acceptance round.
- **Owner** (A0202): > NOT TESTED
- **Occurrences (4):** PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-OTH-005` Pre-NRO engineering gates incomplete: sanitizer/Clang/fuzz coverage gaps (host tests + ASan/UBSan required before every NRO)

- **Status:** Open Defect · **Gen:** all · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Partially honoured. Makefile.host.host-ui:8-13 links the app only under GCC (Clang skipped), so .github/workflows/host-tests-clang.yml does not cover the host-UI harness. Makefile.host.classic_inventory has no -fsanitize flags; its tests (test_rby_staged_inventory, test_gen3_staged_inventory, test_gsc_universal_inventory, test_classic_inventory_catalog) run only in inventory-hardware-retest.yml without ASan/UBSan. tests/fuzz_gen2_sav.cpp:62-70 seeds still contain no 128 KiB padded or Japanese image (last changed cd5eabfe 2026-10-27), while padded/Japanese Gen II saves are shipped behaviours (Group A rows Q02/Q03).
- **Automated tests:** .github/workflows/host-tests-clang.yml : host-test + host-sanitize under clang++ (excludes host-UI harness)
- **Hardware acceptance:** none (engineering-only)
- **Next action:** Add sanitizer flags to Makefile.host.classic_inventory, add padded (0x00/0xFF) and Japanese seeds to fuzz_gen2_sav, and record the Clang/host-UI limitation as accepted or fix the ambiguous using-directives.
- **Occurrences (4):** PRE 5 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; R6 26186464 n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R8 6ec5ccb9 n/a [docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md (Known gaps)]; R8 6ec5ccb9 n/a [docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md (Known gaps)]

### `PBX-OTH-006` One exact, SHA-identified, audited NRO per round (no single-fix NROs; record app SHA + NRO SHA-256)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Honoured. docs/qa/CANDIDATE_UNDER_AUDIT.env binds CODE_SHA 066e1db9…, TREE_SHA, NRO_SHA256 ba6aea09… and size (commit c761964f); docs/hardware/RETEST_066e1db9.md:22 shows the SHA-256 and :35 asks the owner to check it. Held-item fix was not shipped alone (C0422): shipped in Round 6 26186464 per MAIN_ACTIVE_HANDOFF. One candidate per round R1–R12 in NRO_ARCHIVE.md.
- **Repair commits:** c761964f Bind the Round 11 repair candidate 066e1db9 for QA-F9 and the Round 12 hardware checklist
- **Automated tests:** tests/test_qa_nro_audit_workflow.py : one identity source, exact key formats (CODE_SHA/TREE_SHA/NRO_SHA256)
- **Hardware acceptance:** 066e1db9 R12 (owner SHA check step, RETEST_066e1db9.md:35)
- **Next action:** Owner writes the downloaded NRO's SHA-256 into the R12 sheet; keep batching fixes into one candidate per round.
- **Occurrences (4):** PRE 6 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 7 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R5 1ca2faec n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]

### `PBX-OTH-007` Packaged RomFS completeness: NRO builds must not drop artwork (Gen V covers, device asset preflight)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** defect
- **Last owner result:** none
- **Basis:** eb2aa8b3 added tools/verify_nro_romfs.py; tools/check_device_assets.py preflight; recovery check reports 'verified baseline: 3260 HD / 1025 base / 18 types / 3 fonts'. QA-F1 (A0749) added the four Gen V covers to the audit. Artwork visibility on hardware is tracked in visuals-sprites; no owner row checks packaging as such.
- **Repair commits:** eb2aa8b3 fix(release): stop NRO builds from dropping Product Home artwork; verify packaged RomFS
- **Automated tests:** tests/test_verify_nro_romfs.py : packaged RomFS contains required art incl. Gen V covers
- **Hardware acceptance:** 066e1db9 R12 H01 (indirect: HOME card art) · R12 item H01
- **Next action:** None beyond keeping verify_nro_romfs in the candidate workflow.
- **Occurrences (2):** PRE 5 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE QA-F1 PASS [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-OTH-008` Never infer a PASS: green CI / host PASS / an old audited NRO is not hardware acceptance; a PASS with requested changes does not close the request

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Honoured: every candidate is marked NOT DEVICE TESTED until owner results (PR #330/#332 bodies D0242/D0243; NRO_ARCHIVE 'not an approved release'); host cells say 'NOT DEVICE TESTED'; tests/test_hw_round1_regression_gate.py carries the Oct 26 'A PASS does not close the request' rule. Exception: README.md:44 still calls Round 6 the latest recorded result although R11 results are in the repo (see the record-keeping issue).
- **Repair commits:** b6cc08b0 test(qa): hardware round 1 regression gate for the owner's 42 PASS IDs
- **Automated tests:** tests/test_hw_round1_regression_gate.py : owner PASS items guarded; PASS with requested change stays an open request
- **Hardware acceptance:** none (process)
- **Next action:** Keep the rule; fix the stale README status line.
- **Owner** (C0417): > A PASS does not close the request
- **Occurrences (3):** PRE  n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; OCT26 26186464 n/a [tests/test_hw_round1_regression_gate.py]

### `PBX-OTH-009` Gen VI–IX application work deferred and isolated until exact Gen I–V owner acceptance

- **Status:** Intentionally Unsupported · **Gen:** app · **Area:** other · **Category:** decision
- **Last owner result:** none
- **Basis:** Owner directive 2026-10-15 (handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) and owner decision Oct 31 (docs/MAIN_CONSOLIDATION_20261031.md; ROUND11_ENGINEERING_INTAKE.md §4 'Gen I–V only… no Gen VI–IX code'). Honoured: CURRENT_STATUS.md:5, README.md:241 development/gen6-plus is separate; git log of the current branch has no gen6-plus merge. Note: inherited PKSE 1.1.3 Switch-native read code (src/Trainer/Trainer8SWSH.cpp, Trainer8LA.cpp, Trainer9SV.cpp, Trainer9LZA.cpp; SaveSelectScreen.cpp:184-191, :506-518) predates the directive (25f248ab 2026-09-01) and is still in the app; 02595853 (2026-10-28) touched Trainer7LGPE/Trainer8LA as inherited-bug fixes. Deferral is temporary, not a permanent 'no'.
- **Hardware acceptance:** none (decision)
- **Next action:** Keep isolated; confirm with the owner that the inherited Switch-native browse paths in the Gen I–V NRO are acceptable under the 'no Gen VI–IX code' rule.
- **Owner** (E0105): > **Scope of the Round 11 repair NRO:** Gen I–V only. No unfinished Vault/Bank features and no Gen VI–IX code are integrated.
- **Occurrences (11):** PRE  n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R11 3634d1e6 Owner safety clarification 4 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 owner safety clarifica…]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §5 separate from the repa…]; REQ Owner directive / Mandatory milestone order n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Release order 6 n/a [docs/V1_ROADMAP.md]; REQ Gen VI–IX row n/a [docs/GAME_SUPPORT_MATRIX.md]; REQ Header / Not deferred n/a [docs/FUTURE_AFTER_GEN15_BACKLOG.md]; REQ Active owner gate n/a [CURRENT_STATUS.md]; REQ Owner decision (Oct 31) development/gen6-plus n/a [docs/MAIN_CONSOLIDATION_20261031.md]; REQ Owner safety clarification 4 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-OTH-010` QA-F2: malformed one-time-* workflows failed on every push

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** defect
- **Last owner result:** none
- **Basis:** 96a44685 repaired the 3 YAML files and gated them to manual dispatch; python3 tools/qa/check_workflows.py on current source: 'PASS all 48 workflow files parse with a trigger and jobs'; .github/workflows/app-qa-workflow-hygiene.yml enforces it on PRs. No hardware component. Deletion of the 20+ one-time-* workflows still waits for owner approval (A0784), and they are still present in .github/workflows.
- **Repair commits:** 96a44685 ci: repair invalid one-time workflow YAML and gate to manual dispatch (QA-F2); 1b101ed5 tools(qa): workflow parse checker for QA-F2
- **Automated tests:** tools/qa/check_workflows.py (app-qa-workflow-hygiene.yml) : every workflow parses with on:/jobs:
- **Hardware acceptance:** none (CI-only; owner approval for deletion)
- **Next action:** Ask the owner to approve or decline deleting the one-time-* workflows (docs/qa/WORKFLOW_CLEANUP_PROPOSAL.md).
- **Occurrences (4):** PRE QA-F2 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE QA-F2 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE  n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE QA-F2 n/a [docs/qa/FINDINGS.md]

### `PBX-OTH-011` QA-F9: candidate-NRO audit defaulted to stale fa9cfcea artifact on pull_request

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** defect · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** cb03e27b removed defaults; python3 tests/test_qa_nro_audit_workflow.py on current source: PASS. Identity file docs/qa/CANDIDATE_UNDER_AUDIT.env bound to 066e1db9. Later audit PASSes for 7fea1797 (run 38947904299) and e10f307f (run 39122863086) are exact-binary audits, not hardware. Safety-critical because an audit of the wrong binary breaks identity integrity of what the owner tests.
- **Repair commits:** cb03e27b ci(qa): QA-F9 - the candidate NRO audit can no longer audit a stale artifact
- **Automated tests:** tests/test_qa_nro_audit_workflow.py : no identity literals/defaults, API run/artifact binding, staleness guard, tree SHA check
- **Hardware acceptance:** none (CI-only)
- **Next action:** Confirm an app-qa-nro-audit run PASS exists for 066e1db9 before the owner tests R12.
- **Occurrences (5):** PRE QA-F9 n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE QA-F9 n/a [docs/qa/FINDINGS.md]; PRE 7fea1797 QA-F9 PASS [docs/qa/CAMPAIGN_CHECKPOINT.md]; PRE fa9cfcea PASS [docs/qa/FINDINGS.md]; R2 e10f307f QA-F9 PASS [docs/qa/CAMPAIGN_CHECKPOINT.md]

### `PBX-OTH-012` Full Gen I–V 20-release, every-action, exact-NRO owner acceptance gate (#313) not met; no v1.0

- **Status:** Open Defect · **Gen:** all · **Area:** other · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6: 34 PASS / 13 FAIL / 1 NOT TESTED (not a full G01–G12 run)
- **Basis:** Requirement unmet (tracked as Open because no fix can exist until the owner signs the exact NRO, G12). CURRENT_STATUS.md:11 '#313 all-20-release acceptance OPEN'; README.md:44 'no v1.0 release or complete Gen I–V hardware acceptance yet'. Rounds 7–9 have no owner results, R11 was 34/13/1, and R12 066e1db9 is NOT DEVICE TESTED. Hardware-only rows (controller/touch parity, picker/modal rendering, leave dialog, SD export, emulator launch/return; A0879) need an owner full pass.
- **Automated tests:** tests/test_qa_cross_gen_contract.py : host cross-gen UI contracts (not hardware)
- **Hardware acceptance:** first candidate after R12 that clears every FAIL (full #313 round)
- **Next action:** After R12 clears its FAILs, schedule the full G01–G12 20-game acceptance round on one exact NRO.
- **Occurrences (7):** PRE  NOT TESTED [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; REQ G01-G12 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Owner-locked release order n/a [docs/V1_ROADMAP.md]; REQ Important / Roadmap n/a [README.md]; REQ §4 issues #29 etc. n/a [docs/MAIN_CONSOLIDATION_20261031.md]; REQ v1 target bullets n/a [docs/V1_ROADMAP.md]; REQ §5 n/a [docs/PRODUCT_DECISIONS.md]

### `PBX-OTH-013` Hardware status records incomplete or stale: R7–R9 owner results never recorded; R10 row says 'owner testing now'; NRO archive missing R1/R2 (403) and R12; README stale

- **Status:** Open Defect · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. docs/hardware/NRO_ARCHIVE.md: R1 7fea1797 and R2 e10f307f 'not archived: release creation refused (HTTP 403)' (artifacts expire 2027-01-14/16); R7–R9 'results not recorded'; R10 3634d1e6 still 'owner testing now' though ROUND11_OWNER_WORKSHEET.md has its results; no row for 066e1db9 (R12) at all. archive-candidate-nro.yml (bd4aa878) can only be dispatched from the default branch. README.md:44 says the latest recorded result is Round 6 and the owner is testing Round 10. R12 066e1db9 is NOT DEVICE TESTED (PR #332).
- **Repair commits:** bd4aa878 Repository organisation: NRO archive workflow, project board definition, full audit ledger
- **Hardware acceptance:** none (record-keeping)
- **Next action:** After #330 merges, run archive-candidate-nro.yml for R1, R2, R10 and 066e1db9 before Jan 2027 expiry. Update the R10 row and README to R11 results, and record that R7–R9 have no owner results.
- **Occurrences (13):** R1 7fea1797 n/a [docs/hardware/NRO_ARCHIVE.md]; R2 e10f307f n/a [docs/hardware/NRO_ARCHIVE.md]; R3 62a2f8c3 n/a [docs/hardware/NRO_ARCHIVE.md]; R5 1ca2faec n/a [docs/hardware/NRO_ARCHIVE.md]; R6 26186464 n/a [docs/hardware/NRO_ARCHIVE.md]; R7 e88957f2 n/a [docs/hardware/NRO_ARCHIVE.md]; R8 6ec5ccb9 n/a [docs/hardware/NRO_ARCHIVE.md]; R9 d6a20a2a n/a [docs/hardware/NRO_ARCHIVE.md]; R10 3634d1e6 n/a [docs/hardware/NRO_ARCHIVE.md]; R10 3634d1e6 n/a [PR #330 (body 2026-10-31)]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]; GH 3634d1e6 n/a [PR #330 (body 2026-10-31)]; R12 066e1db9 n/a [PR #332 (body 2026-11-01)]

### `PBX-OTH-014` Previously owner-PASSed behaviour must not regress (smoke check of earlier passes every round)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** other · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H15: PASS
- **Basis:** R11 3634d1e6 H15 'Rounds 6–8 smoke check' owner PASS (E0014). tests/test_hw_round1_regression_gate.py PASS on current source: 302 guards covering R1, R2, R3 (70), Oct 24, Oct 25 and Oct 26 PASS items. The Round 11 repair lanes (066e1db9) changed carry, species change, footers and encounters, so recheck in R12 H18. GEN15 gate G09 (F0194) requires the same.
- **Repair commits:** b6cc08b0 test(qa): hardware round 1 regression gate for the owner's 42 PASS IDs
- **Automated tests:** tests/test_hw_round1_regression_gate.py : per-ID guards for every owner PASS from R1 through Oct 26
- **Hardware acceptance:** 066e1db9 R12 H18 · R12 item H18
- **Next action:** Owner runs R12 H18.
- **Occurrences (2):** R2 e10f307f n/a [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R11 3634d1e6 H15 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-OTH-015` Owner's original reports and screenshots not stored verbatim in the repository (Round 6 both reports)

- **Status:** Open Defect · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Not honoured. docs/hardware/ROUND6_RESULTS_26186464.md:18-23 still says the original PokeBank-NX-Round6-Results-26186464.md and round6-screenshots/ are not in the repository. The second 54/4/1 report is referenced only in docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md, never added. Two conflicting Round 6 tallies (49/9/1 vs 54/4/1) remain unreconciled.
- **Hardware acceptance:** none (owner must supply files)
- **Next action:** Ask the owner to re-upload both Round 6 reports and screenshots; store them unchanged next to ROUND6_RESULTS_26186464.md.
- **Occurrences (2):** R6 26186464 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 n/a [docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md]

### `PBX-OTH-016` Owner wants an engineering assessment of every finding before changes, not just a prompt or blind fix

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Honoured for Round 11: docs/hardware/ROUND11_ENGINEERING_INTAKE.md gives a per-finding assessment, shared root causes (§2) and owner safety clarifications (§4) before the repair lanes were merged (066e1db9). The owner has not confirmed the assessment format meets the request.
- **Repair commits:** 132f941d Round 11 intake: owner worksheet, engineering intake, NRO archive record
- **Hardware acceptance:** none (process; owner confirmation)
- **Next action:** Keep giving an intake-style assessment for every owner round before building.
- **Owner** (E0060): > for all these i wanna hear your take on them and not just right me a prompt check out the problems or fix and see how it would work or whats wrong or if we on the same page together, i dont just wanna fuck shit up i wanna hear your take
- **Occurrences (2):** R11 3634d1e6 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; REQ Closing note n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-OTH-017` RECOVERY means a repository/workspace recovery operation (not save recovery)

- **Status:** Needs Investigation · **Gen:** app · **Area:** other · **Category:** request
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Tooling exists (tools/recover_workspace.py, tools/check_recovery_contract.py reports 'snapshot status: COMMITTED'). But docs/RECOVERY_CONTRACT.md is stale ('Last updated 2026-09-22', names PR #77 as the active candidate). Unknown: which branch RECOVERY should restore after the Oct 31 consolidation (main vs fix/round11-gen15-repairs).
- **Automated tests:** tools/check_recovery_contract.py : asset baseline + snapshot state
- **Hardware acceptance:** none (process)
- **Next action:** Update RECOVERY_CONTRACT.md to name the current authoritative branch.
- **Occurrences (1):** REQ The rule n/a [docs/RECOVERY_CONTRACT.md]

### `PBX-OTH-018` v1.0 release hardening items not implemented (Reduced Motion/accessibility, constrained memory, bounded caches, recovery UX, sleep/resume, privacy-safe diagnostics export)

- **Status:** Missing Feature · **Gen:** app · **Area:** other · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** No Reduced Motion setting anywhere in src/ or include/ (grep). Diagnostics exist (AppShellScreen.cpp:158-160 logs + read-only 'Diagnostics / Build Info') but no privacy-safe export. No sleep/resume/operation-mode handling found in src/main.cpp or src/UI/UI.cpp. NACP title is set (Makefile:46 APP_TITLE 'PokeBank NX', :74 icon). Release gates in docs/RELEASE_CHECKLIST.md (no unsupported parser crash, handheld+docked, sleep/resume) are unmet or untested.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** After #313, make a v1.0 hardening plan: Reduced Motion, sleep/resume test, diagnostics export, memory/cache bounds.
- **Occurrences (2):** REQ Phase 13 n/a [docs/V1_ROADMAP.md]; REQ Stable v1.0 release gates n/a [docs/RELEASE_CHECKLIST.md]

### `PBX-OTH-019` v1.0 game scope (40 mainline identities GB→Switch incl. 3DS, LGPE/SwSh/BDSP/PLA/SV/Z-A) not implemented

- **Status:** Missing Feature · **Gen:** app · **Area:** other · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** docs/PRODUCT_DECISIONS.md §1 makes 40 identities (incl. 3DS and Switch) the v1.0 target; README.md:79/:134 shows Gen VI–VIII native formats only on separate PRs #309/#311/#312 and modern Switch as 'general editor ❌ / Future'. No 3DS (Gen VI/VII) support exists in src/. Inherited Switch-native read paths exist, but no shared editor. Conflict to resolve: the current gate (F0187) is Gen I–V only, but v1.0 is still defined as all 40.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner to confirm whether v1.0 still means 40 identities or Gen I–V only; update V1_ROADMAP/PRODUCT_DECISIONS to match.
- **Occurrences (2):** REQ Phases 8-10 n/a [docs/V1_ROADMAP.md]; REQ §1 n/a [docs/PRODUCT_DECISIONS.md]

### `PBX-OTH-020` Post-v1 / non-goal scope: Android companion, link-cable Trade Station, internet trading/GTS, Trainer Plaza, Event Library, Stadium/Colosseum/XD

- **Status:** Intentionally Unsupported · **Gen:** app · **Area:** other · **Category:** decision
- **Last owner result:** none
- **Basis:** Recorded product decisions: docs/PRODUCT_DECISIONS.md §2 (Android v2.0), §4 (link-cable Trade Station a non-goal), §8 (Event Library provenance rule); docs/FEATURE_FEASIBILITY_MATRIX.md 'not required for 1.0'; docs/V2_ROADMAP.md; docs/TRAINER_PLAZA_VISION.md 'must not block v1.0'; docs/FUTURE_AFTER_GEN15_BACKLOG.md (#35 cries/audio, #46 Mystery Gift, #39 Android deferred). None of these exist in src/ (consistent).
- **Hardware acceptance:** none (decision)
- **Next action:** None for v1.0.
- **Occurrences (7):** REQ §2 n/a [docs/PRODUCT_DECISIONS.md]; REQ §4 n/a [docs/PRODUCT_DECISIONS.md]; REQ §8 n/a [docs/PRODUCT_DECISIONS.md]; REQ Not required for 1.0 n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]; REQ Release principle / Not required for v2.0 / Can ship before 2.0 n/a [docs/V2_ROADMAP.md]; REQ No release hostage n/a [docs/TRAINER_PLAZA_VISION.md]; REQ  n/a [docs/V2_PLATFORM_VISION.md]

### `PBX-OTH-021` Lineage/breeding parents: never invent parents the save did not store (lineage states)

- **Status:** Missing Feature · **Gen:** all · **Area:** other · **Category:** requirement
- **Last owner result:** none
- **Basis:** No lineage feature exists: only placeholder copy (src/UI/AppShellScreen.cpp:814 'Clone Lineage', SaveSelectScreen.cpp:5253 'Transfer & lineage' Vault tile). Nothing in src/ fabricates parents, so the constraint is met only because the feature does not exist.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Carry the rule into the Vault/lineage design when that work starts.
- **Occurrences (1):** REQ §3 n/a [docs/PRODUCT_DECISIONS.md]

### `PBX-OTH-022` Vault-driven National Pokédex (v1 milestone) not implemented — Home Pokédex is a placeholder

- **Status:** Missing Feature · **Gen:** all · **Area:** other · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** src/UI/AppShellScreen.cpp:786 renders 'Collection progress will appear here' for the Pokédex section; Master Vault is 'Coming Soon' (AppShellScreen.cpp:92, :672). Only per-save read-only Pokédex views exist (TrainerViewScreenBase.inc:482 pokedexProgress; Gen5SharedReadOnlySurface.inc:1333-1437 Gen V list). The spec requires Vault-authoritative ownership, which cannot exist without the Vault.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Keep blocked on Master Vault; state explicitly in V1_ROADMAP whether the Pokédex milestone is v1.0 or v1.x.
- **Occurrences (2):** REQ Minimum Pokédex v1 milestone n/a [docs/POKEDEX_SPEC.md]; REQ Core rule n/a [docs/POKEDEX_SPEC.md]

### `PBX-OTH-023` Owner approval needed for merges, deletions and retargets; legality #314 stays unmerged

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** other · **Category:** request · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** PROCESS REQUIREMENT. Status mapping for process items: 'Fixed in Code — Awaiting Hardware' = honoured in current tooling/docs but never owner-confirmed; 'Open Defect' = not honoured. Honoured as of 6c7c8e99: no #314/legality-engine merge in history (only inherited include/Legality/Legality.h); one-time workflows not deleted pending approval; docs/MAIN_CONSOLIDATION_20261031.md §1 holds every deletion/closure for approval; Japanese Gen I/II editing stays locked (R11 H13 PASS). Flagged safety because unreviewed merges have truncated trees before (a5515493). B0134/B0595 carry the same rule and are listed under the issues for their other content.
- **Automated tests:** tests/test_ci_repository_safety_contract.py : repository safety contract
- **Hardware acceptance:** none (process)
- **Next action:** Get explicit owner approval before merging #330/#332 or deleting branches/workflows.
- **Occurrences (1):** REQ §1 / §8 Rules n/a [docs/MAIN_CONSOLIDATION_20261031.md]

### `PBX-OTH-024` Round 11's 34 owner-PASS items are not frozen in the regression gate

- **Status:** Open Defect · **Gen:** all · **Area:** other · **Category:** defect
- **Last owner result:** R11/3634d1e6: 34 PASS (unguarded)
- **Basis:** docs/hardware/ROUND11_ENGINEERING_INTAKE.md:43 requires every repair to keep the 34 R11 PASS results (H01–H03, H05–H11, H13–H21, H23, H24, H26, H28, H30, H33, H35–H48 subset) and add regressions. tests/test_hw_round1_regression_gate.py has GUARDS for R1/R2/R3/Oct24/Oct25/Oct26 only (lines 45, 260, 302, 438, 547, 607; summary ends at Oct 26). There is no GUARDS_ROUND11 block, so the gate will not catch a regression of an R11 PASS. R11 is referenced only for reversals (C14 at :661, R2-BUG-004 at :350).
- **Hardware acceptance:** 066e1db9 R12 H18 (smoke only) · R12 item H18
- **Next action:** Add a GUARDS_ROUND11 block (34 IDs, count-checked against ROUND11_OWNER_WORKSHEET.md) to tests/test_hw_round1_regression_gate.py.
- **Occurrences (1):** REQ §1 note n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-OTH-025` No developer / implementation wording in user-visible strings (CG04) - host contract only; Gen I edit status texts still say '... staged; source .srm unchanged'

- **Status:** Needs Investigation · **Gen:** I, II, III, IV, V · **Area:** other · **Category:** requirement
- **Last owner result:** none
- **Basis:** CG04 (tests/test_qa_cross_gen_contract.py:882-888) only rejects '(DV-derived)', 'commits to draft', '(RSE)/(FRLG)/...' suffixes and '(ID nn)' raw IDs; host PASS with 0 violations, NOT DEVICE TESTED (A0864). The owner rejected implementation-facing wording ('source .srm unchanged') for Gen II (D0300, PBX-CRT-017), but src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc:215,304,392,420,434,448,524,538,546 still pass '... staged; source .srm unchanged' texts to EditFeedback::report, and include/UI/EditFeedback.h:29-33 posts Kind::Field Applied messages. Unknown: whether these Gen I UX2 commit paths are still reachable on 066e1db9 (Gen I Edit became a Keep/Discard working-copy draft in 74d4b16f) and whether CG04 should cover this wording. No owner hardware item for CG04 itself.
- **Repair commits:** 36a89388 ui: return Gen II Add directly to packed box selection (Gen II toast reworded); f2cf5954 Gen I-V: no toast after routine edits; Gen V Shiny toggles inline (H22, H29)
- **Automated tests:** tests/test_qa_cross_gen_contract.py:882-888 CG04-no-developer-wording : no '(DV-derived)', 'commits to draft', game-group or raw-ID suffixes in user-visible literals; tests/test_hw_round1_regression_gate.py:210-213 : CG04 is part of the frozen guards for owner-PASSed picker items
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Trace whether Gen I ux2CommitEdit success texts reach the screen on 066e1db9; if so, reword them and extend CG04 to reject 'source .srm unchanged'-style implementation wording.
- **Occurrences (1):** PRE CG04 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]

### `PBX-PCI-001` PC Items view/edit in Gen I-III (Gen I foundation includes Bag/PC Items)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III · **Area:** pc-items · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** PRE/50dac31f/RBY PC Items: PASS (Gen II/III: implicit fc1e6de2 PASS)
- **Basis:** Gen I explicit PASS 'RBY ItemsFix: PC Items' 50dac31f (A0904). Gen II/III only implicit: fc1e6de2 classic inventory PASS (L/R category, exact-game catalogs, A0507/A0515, gens I-III) and b1fcbd3e '100% pass' (D0338); no item-level PC Items PASS for Gen II or III. Current: PC Items is a category in src/Inventory/ClassicInventoryCatalog.cpp:31-42; staged via src/UI/ClassicInventoryOverlay.cpp:135,150,237-257. Catalog/adapters changed since (f565cc63, 642a7931 PC Top Up).
- **Repair commits:** 50dac31f gen1: add read-only RBY inventory support; fc1e6de2 ui: polish classic staged inventory footer and picker readability
- **Automated tests:** tests/test_classic_inventory_catalog.cpp : PC Items catalogs per game; tests/test_bag_top_up_adapters.cpp : Gen I/II/III PC Items state (hasPcItems)
- **Hardware acceptance:** 50dac31f PRE (Gen I); Gen II/III should be explicit in 066e1db9 R12 H10 (PC Items scope) · R12 item H10
- **Next action:** During R12 H10 also open PC Items in Crystal and a Gen III game, edit one quantity, Review.
- **Occurrences (3):** PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE 50dac31f PASS [docs/BUILD_RECORD.md]; REQ Generation I n/a [docs/GAME_SUPPORT_MATRIX.md]

### `PBX-PCI-002` R2-CHECK-010: PC Items catalogs (R/S no Devon Goods/Letter/Orbs, Emerald corrections) and Sapphire Super Rod pocket warning

- **Status:** Needs Investigation (recheck in R12) · **Gen:** III · **Area:** pc-items · **Category:** defect
- **Last owner result:** R3/62a2f8c3/R2-CHECK-010: PASS (Super Rod warning only)
- **Basis:** R3 PASS row for R2-CHECK-010 (B0546/B0682) is titled 'Sapphire Super Rod pocket warning'; no owner result explicitly confirms the R/S/E PC Items catalog changes. Current catalog: src/Inventory/ClassicInventoryCatalog.cpp:300 (gen3Pc), changed since R3 (cfcc5f98 Meteorite, f565cc63).
- **Repair commits:** cfcc5f98 Q06 FRLG: Meteorite (280) is a valid FireRed/LeafGreen Key Item
- **Automated tests:** tests/test_classic_inventory_catalog.cpp : Gen III PC catalogs
- **Hardware acceptance:** none (not on any checklist after R3)
- **Next action:** Owner: Ruby/Sapphire/Emerald PC Items -> X Add, confirm story items are absent.
- **Owner** (B0546): > Warning needs correctly colored controller glyphs.
- **Occurrences (1):** R3 62a2f8c3 R2-CHECK-010 n/a [docs/hardware/RETEST_62a2f8c3.md]

### `PBX-REL-001` Inherited Release (X) reachable in installed game-save views

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** release · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** PRE 3be4de6b: FAIL
- **Basis:** 0ea98cc1 source-mutation lock (canPerform false for installed/legacy sources). In Gen III/IV the Boxes grid now consumes X so it cannot fall through to the inherited X=Release handler (dd69bc93); intake notes the legacy handler is still stopped by requireMutableWorkspace() when reached. 'NO RELEASE ON INSTALLED SOURCE' (F0090) never received an owner result.
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_source_mutation_policy.cpp : installed source refuses mutations
- **Hardware acceptance:** none (Switch-native titles outside Gen I-V R12)
- **Next action:** Verify X on an installed Switch-native title when Gen VI+ is tested; R12 H05 X steps cover Gen I-V.
- **Occurrences (3):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; REQ Second physical Switch matrix n/a [docs/RELEASE_CHECKLIST.md]

### `PBX-REL-002` Release/Remove needs a separate Y confirmation; A is not destructive; B/cancel preserves the Pokemon; no original-source write

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** release · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R3 62a2f8c3 C08: PASS
- **Basis:** Owner PASS R1 7fea1797 C08 and R3 62a2f8c3 C08 (B0491). Current: Y confirms (src/UI/ClassicReleaseActionFix.inc:173 confirmRelease, :319 Y -> confirmRelease; Gen III confirmRelease state Gen3SharedPokemonSurface.inc:119). The Release sheet wiring in ClassicReleaseActionFix.inc changed after R3 (dd69bc93, 6f62b69e, 91cf155e).
- **Repair commits:** c81853da feat(editor): route classic Party Actions to native staged edits; dd69bc93 Round 11 X-IMPEXP + ACTIONS: one shared box/Actions capability model, Gen I-V
- **Automated tests:** tests/test_hw_round1_regression_gate.py : C08 guard; test_shared_editor_unit / CG06 / test_touch_pokemon_editor_direct_rows_contract.py : Release confirmation
- **Hardware acceptance:** R3 62a2f8c3 C08; re-verify on 066e1db9 (no explicit R12 item; H05 only lists the row)
- **Next action:** Add a one-line Release Y-confirm / B-cancel regression step to R12 (per generation).
- **Occurrences (11):** PRE D5 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE D14 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R1 7fea1797 C08 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C08 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 C08 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C08 PASS [tests/test_hw_round1_regression_gate.py]; R2 C08 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C08 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C08 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C08 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; REQ Permanent guardrails n/a [README.md]

### `PBX-REV-001` Inherited apply/save-style mutation flow reachable in game-save views (#23)

- **Status:** Needs Investigation · **Gen:** app · **Area:** review-keep-discard · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** PRE/3be4de6b: FAIL
- **Basis:** PRE 3be4de6b FAIL; #23 hardening 0ea98cc1 'safety: lock installed-source UI'. The second-device checklist (af2acf04, A0271-A0278, 'NO SAVE-CHANGES PATH TO INSTALLED SOURCE') was never filled in. Gen I-V emulator saves later get a staged/export-only model (S05 PASS R6, H14 PASS R11), but no owner result specifically confirms native installed-title saves have no save-changes path in the current NRO. TrainerViewScreenBase.h:301 still has a 'Save & Exit' storage exit (app-owned Storage).
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Confirm with owner whether native Switch titles are in v1 scope; if yes, add an installed-source no-write check.
- **Occurrences (2):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b engineering-finding [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]

### `PBX-REV-002` Gen I Pending Changes panel content-aware sizing

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** review-keep-discard · **Category:** request
- **Last owner result:** R6/26186464/D02: PASS
- **Basis:** Superseded by the shared paged Review: R6 26186464 D02·P1-GEN1 PASS 'Only the shared Review panel ... a swipe turns one page' (C0338). Gen I UX files changed in R11.
- **Repair commits:** 8f0e1210 (R5 Gen I shared Review panel)
- **Automated tests:** tests/test_gen12_review_parity.cpp
- **Hardware acceptance:** 26186464 R6 D02
- **Next action:** None.
- **Occurrences (6):** PRE 6ecd9c28 5 owner-request [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; R5 1ca2faec P1 Gen I Review checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec P1-GEN1 NOT TESTED [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 D02 · P1-GEN1 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 D02 · P1-GEN1 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 D02 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-REV-003` Gen V Review / Discard / Export Copy parity and reachability (S04, Gen V Review not reachable)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** review-keep-discard · **Category:** defect · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R6/26186464/D01: PASS (Gen V-specific: R3/62a2f8c3/S04 PASS)
- **Basis:** R3 62a2f8c3 S04 PASS (B0468); R4 Gen V Review not reachable FAIL (C0004); OCT24 repair 'Gen V Trainer Y/Review accessible'; R6 26186464 D01 AUD-01 Review consistency PASS across Gen I-V. Gen V surface changed heavily in R11 (src/UI/Gen5SharedReadOnlySurface.inc 222 lines), so recheck.
- **Repair commits:** 0f132ebc S04 host trace; e061f03c (R4) IV/V one paged Review presenter; 42bf612d fix(review): the Gen V save hub footer names its + Review
- **Automated tests:** tests/test_gen5_review_discard_export_trace.cpp : 372 checks W/B/W2/B2 Review/Discard/Export
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** R12 H17 on a Gen V save.
- **Owner** (B0051): > S04 gen 5 needs more work i dont think this work properly like the others do.
- **Owner** (B0468): > Owner wants Gen V review workflow added to all Gen I–IV.
- **Occurrences (16):** PRE Review checklist-item [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; R1 7fea1797 S04 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 S04 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 S04 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 S04 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 S04 NOT TESTED [issue #313 (comment 2026-10-17)]; R2 S04 checklist-item [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f S04 checklist-item [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f S04 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 S04 checklist-item [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R3 62a2f8c3 S04 checklist-item [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 S04 checklist-item [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 S04 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 S04 PASS [tests/test_hw_round1_regression_gate.py]; R4 c60d4da0 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 AUD-01 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]

### `PBX-REV-004` Staged working copy, Review lists only staged changes, Discard All restores original (Discard)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** review-keep-discard · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H45: PASS
- **Basis:** Latest owner result R11 3634d1e6 H45 PASS 'discard restores both changes; no source mutation' (E0044); also R11 H24/H48 PASS (other areas). Discard confirmation is the shared Y-then-Y path (include/UI/Gen12ReviewDiscardConfirm.h, SharedEditorChrome.h:179-181 'Discard & Exit'). Round 11 changed staged editors (Gen I/II carry owns staged save; Discard ends a carry 53330345), so recheck in R12 H17.
- **Repair commits:** 29084707 (R4 staged exit I-IV); 53330345 H32: Gen I/II carry transaction owns the staged save until place/cancel
- **Automated tests:** tests/test_qa_export_contracts.cpp : C6 Discard All restores; later export refused; QA matrix working-copy/review cells (20 games)
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** R12 H17 (Discard B then Y).
- **Occurrences (18):** PRE E1 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE E4 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE E5 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE E6 checklist-item [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE D11 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F01 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F02 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F03 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE F04 checklist-item [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE working-copy PASS [docs/qa/GEN1_5_MATRIX.md]; PRE review PASS [docs/qa/GEN1_5_MATRIX.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE Working copy requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; PRE Review requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; PRE Exit requirement [docs/qa/SAVE_SAFETY_CHAIN.md]; R8 6ec5ccb9 H08 checklist-item [docs/hardware/RETEST_6ec5ccb9.md (same items in RETEST_ROUND8_TEMPLAT…]; R11 3634d1e6 H45 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R12 066e1db9 H17 checklist-item [docs/hardware/RETEST_066e1db9.md]

### `PBX-REV-005` Gen IV net no-op clone + release appeared in Review as an edit (QA-F8a)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV · **Area:** review-keep-discard · **Category:** defect
- **Last owner result:** none
- **Basis:** Fixed c7b0f97 (docs/qa/FINDINGS.md). Host-only; no owner item.
- **Repair commits:** c7b0f97 (QA-F8a)
- **Automated tests:** QA matrix review cell
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** None.
- **Occurrences (2):** PRE QA-F8a engineering-finding [docs/qa/FINDINGS.md]; PRE QA-F8a n/a [docs/qa/CAMPAIGN_CHECKPOINT.md]

### `PBX-REV-006` Review shows one row per Move and readable move/item names (R2-REVIEWMOVE)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** III, IV, V · **Area:** review-keep-discard · **Category:** defect
- **Last owner result:** R6/26186464/D01: PASS
- **Basis:** R3 FAIL numeric IDs (B0483); OCT24 repair 'Review rows retain named moves/items'; R6 26186464 D01 PASS 'same pages, names and before/after values' (C0337). Note: the ID R2-REVIEWMOVE was later reused for 'Gen III Create with Safari Ball' (create area). Review row code changed in R11 (Gen4ExportTransaction.cpp:225-236 caution rows).
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND7 D01
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** Covered by R12 H17.
- **Owner** (B0483): > One row now works, but displayed move changes use numeric IDs instead of readable names; show old/new move and item names etc.
- **Occurrences (4):** R2 Review: one row per Move checklist-item [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f Review: one row per Move checklist-item [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-REVIEWMOVE FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; OCT24 8804f8f8 1 checklist-item [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]

### `PBX-REV-007` Gen I-IV staged exit menu: A Review, B Continue, Y Discard & Exit, X Export Copy (NEW-19; Gen V-like review flow for I-IV)

- **Status:** Hardware Accepted · **Gen:** I, II, III, IV · **Area:** review-keep-discard · **Category:** request · ⚠️ safety-critical
- **Last owner result:** R6/26186464/A20: PASS
- **Basis:** R6 26186464 A20 PASS 'Four-choice staged exit' (C0300). include/UI/ClassicStagedExit.h and SharedEditorChrome.h:179-181 unchanged since 26186464.
- **Repair commits:** 29084707 (R4) one staged Review/Continue/Discard and Exit/Export Copy flow I-IV; a7244a64 (R5) Review returns to the screen underneath
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND5 NEW-19
- **Hardware acceptance:** 26186464 R6 A20
- **Next action:** None.
- **Owner** (B0469): > Owner wants Gen V review workflow added to all Gen I–IV.
- **Owner** (B0568): > Bring Gen V-like Review/Continue/Discard/Export Copy to Gen I–IV, safely and without duplicated controls.
- **Occurrences (10):** R3 62a2f8c3 S04 owner-request [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-19 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-19 checklist-item [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-19 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-19 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-19 checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-19 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A20 · NEW-19 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 A20 · NEW-19 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A20 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-REV-008` One shared Gen I-V Review presentation (AUD-01); owner: do not redesign

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** review-keep-discard · **Category:** request
- **Last owner result:** R6/26186464/D01: PASS
- **Basis:** R4 FAIL (C0003/D0208); R6 26186464 D01 PASS (C0337) with owner decision 'Do not redesign it' (C0344). ReviewEntry.h changed after R6 (acc0b772 R7) -> recheck.
- **Repair commits:** 4e1532d3, 9f034431 (R5 shared paged Review I-IV); acc0b772 fix(review): one Review key per screen
- **Automated tests:** tests/test_gen12_review_parity.cpp; tests/test_classic_party_review_path.cpp
- **Hardware acceptance:** 066e1db9 R12 H17 · R12 item H17
- **Next action:** Keep presentation frozen per owner decision.
- **Owner** (D0208): > **Owner verdict: FAIL / needs implementation.** Review presentation changes from generation to generation, and owner cannot reach a functioning Review screen in Gen V.
- **Owner** (C0344): > The shared Review is much improved. Do not redesign it.
- **Owner** (B0570): > Review screen differs from Gen IV and other parity gaps.
- **Occurrences (11):** R3 62a2f8c3 AUD-01 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 AUD-01 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 FAIL [issue #323 (body 2026-10-24)]; OCT24 c60d4da0 AUD-01 NOT TESTED [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec AUD-01 (unmarked) checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec AUD-01 NOT TESTED [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 D01 · AUD-01 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 D01 · AUD-01 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 D01 · AUD-01 owner-decision [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 D01 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A48 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-REV-009` Review easy to find from every screen and returns where opened; Gen II Create Review invisible; + opened Review on Party (P1-RETURN, P3)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** review-keep-discard · **Category:** defect
- **Last owner result:** R6/26186464/B08·P1-RETURN: FAIL
- **Basis:** R5 FAIL (C0172) and R6 FAIL (C0319: Gen II Unown Create Review invisible; + opens Review in Party). Fixes R7: 146511e2 (Gen II Create/Edit Review drawn), acc0b772 (one Review key per screen, the one its footer shows), 698314e6/7152178b (visible Review entry everywhere), CG81 enforced (2b8e9791). R7-R9 checklists have no results; R10/R11/R12 have no specific item. No owner PASS since the R6 FAIL.
- **Repair commits:** 146511e2 fix(review): Gen II Create / Edit Review is drawn, not hidden under the workspace; acc0b772 fix(review): one Review key per screen, the one its footer shows; 698314e6 feat(review): visible Review entry from every editor screen, Gen I-V; 2b8e9791 test(qa): CG81 enforced (one Review key per screen)
- **Automated tests:** tests/test_ui_review_entry_host.cpp : real TrainerViewScreen, every gen's Trainer/Party/Boxes/Bag opens Review with its key, B returns; tests/test_ui_gen2_create_review_host.cpp : Gen II Create Review drawn
- **Hardware acceptance:** none (not on any checklist since R7 B06/B07, which have no results)
- **Next action:** Add to next checklist: Gen II Create Unown -> Review visible; + in Party does not open Review; footer key opens Review on each screen.
- **Owner** (C0172): > Review is hard to find from Bag and the other editor screens, and there is no visible control prompt for it.
- **Owner** (C0319): > Two separate defects:<br>(1) "While creating Unown in Gen II, opening Review displays what appears to be an invisible screen. The application appears frozen, but pressing B exits the invisible screen and returns it to normal."<br>(2) "Review also works by pressing + in the Party menu. This needs fixed to just the Y button, or whatever the correct button is in the current menu or window."
- **Occurrences (11):** R5 1ca2faec P1 Review return checklist-item [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec P1-RETURN FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P3 requirement [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B08 · P1-RETURN checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 B08 · P1-RETURN FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B08 (1) n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B08 (2) n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A43 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 B06 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 B07 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 C08 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-REV-010` Bag Review discoverability (B08 / 7G)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** review-keep-discard · **Category:** defect
- **Last owner result:** R6/26186464/B03: PASS
- **Basis:** R5 FAIL (C0167); R6 26186464 B03 and C07 PASS (C0314, C0326). Bag Help wording follow-up (C0346) belongs to help-footer-controls. ReviewEntry.h changed after R6 -> recheck. Note: ID B08 earlier meant 'export outcome visible' (OCT24), reused.
- **Repair commits:** 698314e6 feat(review): visible Review entry ...
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND7 B03, C07
- **Hardware acceptance:** 26186464 R6 B03
- **Next action:** None.
- **Owner** (C0167): > Bag Review is not directly discoverable. It is buried in the `+` options and the save-exit flow.
- **Occurrences (10):** R5 1ca2faec B08 FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec 7G owner-request [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B03 · B08 checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 C07 · 7G checklist-item [docs/hardware/RETEST_26186464.md]; R6 26186464 B03 · B08 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C07 · 7G PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 B03 PASS [tests/test_hw_round1_regression_gate.py]; OCT26 26186464 C07 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A33 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 A38 checklist-item [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-SET-001` Themes OLED Black / Dark / Light (and variants) readable, switchable and persisted after restart

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** settings · **Category:** requirement
- **Last owner result:** PRE/3be4de6b: PASS
- **Basis:** Owner PASS on 3be4de6b (A0009-A0012, A0041-A0044, D0256-D0259; theme torture A0188-A0194 incl. persistence after restart/sleep). Never re-scored since; palettes changed in d1a9aad6 (Dark/OLED/Light values, 7 new themes) and 18 ThemeModes now exist (include/Utils/Settings.h:21; include/UI/Common.h:181-198).
- **Repair commits:** d1a9aad6 fix: finish Gen III polish and expand readable themes
- **Automated tests:** tests/test_theme.cpp : theme palettes / persistence parsing
- **Hardware acceptance:** 3be4de6b (PRE); R12 H01 optionally uses two themes · R12 item H01
- **Next action:** Ask owner to switch theme and restart once during R12 H01.
- **Occurrences (33):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE af2acf04 n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE A5 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE A6 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE A09 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE A10 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]

### `PBX-SET-002` Dark theme ~10% darker; keep Light as-is; OLED Black preferred dark Action Sheet

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** settings · **Category:** request
- **Last owner result:** none
- **Basis:** d1a9aad6 (first owner-tested in 0760334f): DARK_PALETTE background 24,27,38 -> 20,23,32 and surface 35,39,53 -> 31,35,47 (~15% darker; include/UI/Common.h:50-55). Light background unchanged (246,247,252) but text roles darkened and comment changed to 'bright high-contrast' - check it still matches 'Light should remain substantially as-is'. No owner result on the darker Dark.
- **Repair commits:** d1a9aad6 fix: finish Gen III polish and expand readable themes
- **Automated tests:** tests/test_theme.cpp
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner compares Dark and Light on 066e1db9 (can be done in R12 H01).
- **Occurrences (3):** PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-SET-003` Second Switch profile: saves owned by another profile show 'belongs to another Switch profile'

- **Status:** Needs Investigation · **Gen:** app · **Area:** settings · **Category:** requirement · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** Source message at src/UI/SaveSelectScreen.cpp:2562 (validated saves belonging to another Switch profile) and current-profile assignment at :2674. Device check listed (ROUND3_PART9_REPRO_0faf7e9c) but never run; no owner result. Safety: opening another profile's save would risk cross-profile writes.
- **Repair commits:** aeccd13 fix(home): same-profile guard lives in setUser
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner with two profiles checks profile switching and foreign-save messaging.
- **Occurrences (2):** PRE A08 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 0faf7e9c n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-SPC-001` Gen I/II species picker presentation: `001 - Bulbasaur` rows, shared visual picker with normal/shiny (Gen II plain list)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II · **Area:** species-change · **Category:** defect
- **Last owner result:** PRE/cfb70357/PR #68 milestone: PASS
- **Basis:** Owner accepted 69668bc8 (A0552 picker ownership fixes) and cfb70357 (PR #68 body D0303: Gen I/II presentation accepted). Later species-picker owner PASSes R6 26186464 A09/A14 (NEW-05/NEW-11, other areas). Picker changed in Round 11 to one sprite (eb7fe8c9, include/UI/SharedSpeciesPicker.h).
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions; eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state
- **Hardware acceptance:** cfb70357 PRE · R12 item H16
- **Next action:** Covered by R12 H16.
- **Owner** (D0291): > - Gen II Species picker still uses a plain list instead of the accepted shared Gen I visual picker family with normal/shiny presentation where valid.
- **Occurrences (4):** PRE 574d604b 5 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]; PRE 6 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]

### `PBX-SPC-002` Gen IV/V species picker parity: Normal/Shiny preview, classic Pokedex-number labels, sprites (Gen V text-only/overbroad modal rejected)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV, V · **Area:** species-change · **Category:** defect
- **Last owner result:** none
- **Basis:** Gen IV fixed aff41765/66312f10; shared picker now one sprite with Normal/Shiny toggle for every gen (eb7fe8c9, include/UI/SharedSpeciesPicker.h). No owner PASS that explicitly covers Gen IV/V sprite + shiny preview + Dex labels (R6 A09/A14 PASSes cover keyboard and name order only).
- **Repair commits:** aff41765 fix(gen4): align create editor with classic parity; eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state
- **Automated tests:** docs/qa/GEN1_5_MATRIX.md pickers cell : host PASS only
- **Hardware acceptance:** 066e1db9 R12 H16 · R12 item H16
- **Next action:** Owner runs R12 H16 on a Gen IV and a Gen V save.
- **Owner** (D0220): > The owner previously rejected the text-only/overbroad species modal and asked for the **same Gen I–IV artwork/shiny previews and game-correct availability**.
- **Owner** (D0318): > - Species labels/presentation did not match the classic Pokédex-number style closely enough.
- **Owner** (D0317): > - Gen IV Species picker lacked Y Normal/Shiny selection.
- **Occurrences (5):** PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]; PRE acfca273 FAIL [issue #95 (comment 2026-09-28)]; PRE pickers PASS [docs/qa/GEN1_5_MATRIX.md]; PRE C14 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE 888b886c n/a [PR #307 (comment 2026-10-15)]

### `PBX-SPC-003` Species change (and form/gender/shiny) only when native; species mutation reconciles dependent state (Gen IV roadmap Phase 3)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** species-change · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H28: PASS
- **Basis:** Native guards: Gen III 1..386 (include/UI/Gen3SharedPokemonSession.h:151), Gen IV setSpecies base form + PID-linked nature/gender/ability reconciliation with rollback (src/Pokemon/Pokemon4Mutable.cpp:418-500), Gen V speciesRefusal (include/Integration/Gen5/Gen5SharedScreenState.h:592-618). Owner PASS R11 3634d1e6 H28 (expected "out-of-game species absent; Review shows correct changes"). Species apply code changed in Round 11.
- **Repair commits:** 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07)
- **Automated tests:** tests/test_species_change_moves_gen4.cpp / _gen5.cpp : species change reconciles moves and refuses non-native changes
- **Hardware acceptance:** 3634d1e6 R11 H28 · R12 item H09
- **Next action:** Recheck through R12 H09/H16.
- **Occurrences (3):** PRE D02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; REQ Phase 3 n/a [docs/V1_ROADMAP.md]; REQ D02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-SPC-004` Species change keeps old/incompatible moves (R2-BUG-004): clear incompatible moves on species change; since R11 never add/refill moves

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** species-change · **Category:** defect · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H28: PASS
- **Basis:** Owner PASS R3 62a2f8c3 R2-BUG-004 (that build REFILLED native level-up moves) and R11 3634d1e6 H28 PASS. Owner reversal: H28/Extra 5/Extra 6 asked that species change never auto-fill moves; Round 11 changed keepLearnable to keep only learnable moves and add none (include/UI/SpeciesChangeMoves.h:5-50; Gen II include/UI/Gen2PokemonSession.h:87-94; Gen III include/UI/Gen3SharedPokemonSession.h:162-169; Gen IV include/UI/Gen4SharedPokemonSession.h:145-153; Gen V src/UI/Gen5SharedReadOnlySurface.inc:840-851). Code changed after the last PASS, so recheck.
- **Repair commits:** 8a84128e Merge audit/app-qa-20261014 (R2-BUG-004 refill, round 3); 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07)
- **Automated tests:** tests/test_hw_round1_regression_gate.py : R2-BUG-004 guard (CG19, test_species_change_moves_gen1 "Thunder Shock cleared, Growl kept with its PP and PP Ups, nothing added"); tests/test_species_change_moves_gen1.cpp .. _gen5.cpp
- **Hardware acceptance:** 3634d1e6 R11 H28; recheck on 066e1db9
- **Next action:** Add an explicit R12 step: Edit species change keeps learnable moves with PP/PP Ups and adds none (no current Hxx covers it; H09 only checks history).
- **Owner** (E0027): > this passes but i wanna have it where when the editor is opened the new pokemon that hasnt been added yet doesnt come with its default moves and alson when we switch species i dont want it to come with moves auto filled in, i want the user to pick the moves and all the info first and also can we have it where a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves unless its breed or however that would be legal,
- **Occurrences (4):** R3 62a2f8c3 R2-BUG-004 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-004 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-004 PASS [tests/test_hw_round1_regression_gate.py]; R11 3634d1e6 H28 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-SPC-005` Gen II Unown chooser sprites always showed form A (letter must follow DVs)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II · **Area:** species-change · **Category:** defect
- **Last owner result:** R4/c60d4da0: FAIL
- **Basis:** Picker computes the DV-derived letter on a hypothetical copy (include/UI/Gen2PokemonPickerModel.h:130-162 applySpeciesAppearance(hypothetical, 201,…)). Owner FAIL R4 c60d4da0; the related R5 R2-REROLL item (C0157 chooser portrait/letter agree) was unmarked and C0171 R5 FAIL concerns letter vs shiny (stats area). No owner PASS for the chooser portrait.
- **Repair commits:** fd30e0fc fix(gen2): Unown sprite and draft letter follow the DVs (R2-REROLL, round 3 screenshots 3-4)
- **Automated tests:** tests/test_gen2_unown_letter.cpp : chooser/draft letter follows the DVs; tests/test_gen2_unown_letter_selector.cpp
- **Hardware acceptance:** 066e1db9 R12 H16 ("Gen II: try Unown") · R12 item H16
- **Next action:** Owner runs R12 H16 with Crystal/Gold Unown and compares chooser picture to the kept letter.
- **Occurrences (1):** R4 c60d4da0 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]

### `PBX-SPC-006` H25: species change carried previous Pokemon's Met Location and Pokerus (stale editor state); Create drafts reset visibly, existing Pokemon keep history and get a Caution flag (never silently deleted)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II, III, IV, V · **Area:** species-change · **Category:** safety · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H25: FAIL
- **Basis:** Shared policy include/UI/SpeciesChangeHistoryPolicy.h:58-64 (Create resets Pokerus and an invalid chosen Met Location; Edit resets nothing) and include/UI/HistoryConsistency.h:42-56 (Caution flags, never delete). Applied: Gen II include/UI/Gen2PokemonSession.h:95-129; Gen III include/UI/Gen3SharedPokemonSession.h:170-212; Gen IV include/UI/Gen4SharedPokemonSession.h:155-197; Gen V include/Integration/Gen5/Gen5SharedScreenState.h:441-453,684-695; Gen III Review caution src/Integration/Gen3/Gen3StagedPokemonEditor.cpp:711-715. Gen I stores neither field. Owner FAIL R11 3634d1e6 H25; no later owner result. Caveat: the R12 Group A byte-for-byte Edit-species-change round trip is NOT TESTED / NO FIXTURE, and see PBX-SPC level issue (Gen I-III Edit species change also resets Level/EXP, which then triggers the "Met Level is above the current level" Caution).
- **Repair commits:** 620ee717 Shared species-change and Create-draft policy for Gen I-V (R11 H25, H28, H07); ab072252 Round 11 follow-ups: explicit Met Location choice, H25 history flags, visible Create resets
- **Automated tests:** tests/test_species_change_history_policy.cpp : Create resets Pokerus/invalid Met Location, Edit keeps history, flags never delete; tests/test_gen4_create_species_change_history.cpp; tests/test_species_change_moves_gen2.cpp / _gen4.cpp : HistoryConsistency flags
- **Hardware acceptance:** 066e1db9 R12 H09 · R12 item H09
- **Next action:** Owner runs R12 H09 (existing Pokemon with Pokerus/Met Location, unrelated species, check Caution and Review; then a Create draft species change status). Engineering: run the Group A byte-for-byte round trip with a real save.
- **Owner** (E0120): > H25 is genuine preserved Pokémon history. Species editing must not silently clear Pokérus, origin or Met Location. Flag suspect combinations in Editor/Review.
- **Owner** (E0103): > **H25 history vs stale editor state.** On an existing Pokémon, recorded history (Met Location, Met Level, Pokérus, ball, egg/fateful data) is **preserved**. When it is incompatible with a newly chosen species, it is **flagged** in the editor and in Review, never silently deleted. New drafts reset properly and never inherit another record's state; the editor panels always re-read the record being edited, so nothing stale is shown.
- **Owner** (E0067): > gen leaves info in the left side panel ffrom the previous pokemon … i had a pikachu … met location to trohpy arden and changed species to arbok and arbok has the same met location and pokerus stat aswell
- **Occurrences (11):** R11 3634d1e6 H25 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H25 FAIL [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 decisions]; R11 3634d1e6 Owner safety clarification 2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §4 owner safety clarifica…]; R11 3634d1e6 n/a [owner message 2026-11-01 (independent Round 11 QA)]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §2 shared root causes]; R12 066e1db9 H09 n/a [docs/hardware/RETEST_066e1db9.md]; REQ H25 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ Owner safety clarification 2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ §4 H25 Edit vs Create n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ H09 n/a [docs/hardware/RETEST_066e1db9.md]

### `PBX-SPC-007` Species picker: one picture that switches Normal/Shiny instead of two pictures (owner Extra 5)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** species-change · **Category:** request · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** eb7fe8c9 "SPRITE: species picker shows one sprite in the current shiny state" (include/UI/SharedSpeciesPicker.h changed in Round 11). Other parts of Extra 5 (quit dialog; blank Gen I/II moves) are tracked in launch-quit and in the PBX-CRT blank-moves issue.
- **Repair commits:** eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state
- **Hardware acceptance:** 066e1db9 R12 H16 · R12 item H16
- **Next action:** Owner runs R12 H16.
- **Owner** (F0110): > The quit option in the nro has a highlighted option in the Y exit option, it also has a footer which isn't needed cause the buttons we need are the options in the menu, can we also get rid of "stay" we have cancel in that menu and those 2 options are the same just different names. In the species picker instead of having the 2 pictures one shiny and one normal can we have it where its 1 photo and if we can shiny or not it changes the photo, instead of the 2 photo thing. Gen 1/2 have moves when editor is opened, it should be blank, same as for gen 1-5 when a pokemon is selected it should leave moves clear not add or fill the moves just cause that pokemon was selected in species selector
- **Occurrences (2):** R12 066e1db9 H16 n/a [docs/hardware/RETEST_066e1db9.md]; REQ Extra 5 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-SPC-008` Species change Level: whether a species change keeps the Pokemon's current Level has NO SPECIFICATION; generations behave differently today

- **Status:** Open Defect · **Gen:** I, II, III, IV, V · **Area:** species-change · **Category:** requirement · 🎯 v1.0 required
- **Last owner result:** none
- **Basis:** Reclassified at assembly from Needs Investigation: the owner's 2026-11-01 reconciliation brief lists "Species-change level and history preservation" as a mandatory feature. Gen I–III Edit still resets an existing Pokémon's Level/EXP to the encounter minimum on a species change (evidence below), so the requirement is not met. The exact rule (keep Level always? keep Level, recompute EXP?) still needs the owner's wording. Original basis: No spec (F0106 "NO SPECIFICATION FOUND"). Current code per gen: Gen I — Create AND Edit reset Level to SpeciesChangeLevelPolicy::defaultLevel (exact-game minimum encounter level, fallback 5) and drop EXP (src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:198-201 AddDraft, :222-224 Edit). Gen II — Create AND Edit reset Level/EXP to defaultLevel (include/UI/Gen2PokemonSession.h:75-86). Gen III — Create AND Edit reset Level/EXP to defaultLevel (include/UI/Gen3SharedPokemonSession.h:162; test tests/test_gen3_shared_pokemon_session.cpp:194-211 asserts an Edit at Level 80 becomes the encounter minimum and "!= 80"). Gen IV — keeps the current Level, rewriting EXP for the new growth rate (src/Pokemon/Pokemon4Mutable.cpp:426,452-454). Gen V — Edit refuses a species with a different EXP growth group ("its level would change", include/Integration/Gen5/Gen5SharedScreenState.h:603-607), so Level is kept; Create rebuild keeps the draft Level (:665-667). Policy origin: include/UI/SpeciesChangeLevelPolicy.h ("never inherited from the previous species"), commit 63ce8877 (2026-09-20). Unknown: the owner's intended rule; Gen I-III Edit behaviour silently changes an existing Pokemon's Level/EXP, inconsistent with Gen IV/V and with the H25 "history kept" spirit.
- **Repair commits:** 63ce8877 editor: add shared species change level policy
- **Automated tests:** tests/test_gen3_shared_pokemon_session.cpp:194-211 : Edit species change resets Level to encounter minimum (asserts != 80); tests/test_species_change_moves_gen1.cpp:145,191 / tests/test_species_change_moves_gen2.cpp:86 : Level == defaultLevel after species change
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Ask the owner for a rule (keep current Level vs reset) for Create and for Edit; then align Gen I-III with Gen IV/V or vice versa and add an R12/R13 checklist step.
- **Occurrences (1):** REQ  n/a [n/a (searched docs/ and root status files)]

### `PBX-SRC-001` Live incremental species search in the shared Gen I-V species picker

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** search · **Category:** request
- **Last owner result:** R11/3634d1e6/H11: PASS (R3/62a2f8c3/R2-SEARCH: PASS)
- **Basis:** 2876ffe2 (e10f307f); owner R3 62a2f8c3 R2-SEARCH PASS (B0472); search typing also exercised by R11 3634d1e6 H11 PASS. include/UI/SharedSpeciesPicker.h changed after 3634d1e6 (eb7fe8c9 single sprite) - layout around the search changed.
- **Repair commits:** 2876ffe2 feat(ui): R2-SEARCH live incremental species search in the shared Gen I-V chooser (Phase 3)
- **Automated tests:** tests/test_species_search_gen15.cpp : prefix filter narrows per character, Backspace widens, all Gen I-V (CG15)
- **Hardware acceptance:** 3634d1e6 R11 H11; recheck with R12 H16 · R12 item H16
- **Next action:** During R12 H16 also type a few letters in the species search.
- **Owner** (B0055): > can we add a search to the pokemon species picker, so we click on search or press a button like "+ or -" and than we can type what pokemon we want, but can we have it so the search is active like if i press C than all pokemon with C popup and then if i enter O than all pokemon that start with CO and so on and so on, and if i detlete a letter it reverts to just the pokemon with said letters, is called active search ??.
- **Occurrences (8):** PRE CG15 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 R2-SEARCH (Phase 3) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f R2-SEARCH (Phase 3) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-SEARCH PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-SEARCH PASS [tests/test_hw_round1_regression_gate.py]; REQ Owner request 2 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]

### `PBX-SRC-002` NEW-05: species search keyboard covers the list; dock keys under the art, keep list visible

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** search · **Category:** defect
- **Last owner result:** R6/26186464/A09: PASS
- **Basis:** R3 FAIL -> OCT24 PASS, R5 PASS, R6 26186464 A09 PASS (C0289). Species picker art area changed in 066e1db9 (eb7fe8c9 one centred sprite), which is where the keyboard docks.
- **Repair commits:** eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state (layout change after acceptance)
- **Automated tests:** CG39 (tests/test_qa_cross_gen_contract.py) species keyboard beside list
- **Hardware acceptance:** 26186464 R6 A09; recheck 066e1db9 R12 H16 · R12 item H16
- **Next action:** R12 H16: open the search keyboard with the new single picture.
- **Owner** (B0554): > Put search keyboard under species artwork/shift art upward, keep list visible.
- **Occurrences (9):** R3 62a2f8c3 NEW-05 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-05 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-05 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-05 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-05 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-05 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A09 · NEW-05 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A09 · NEW-05 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A09 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-SRC-003` NEW-06: Held Item picker needs search alongside Sort (docked keyboard, live filter)

- **Status:** Hardware Accepted · **Gen:** II, III, IV, V · **Area:** search · **Category:** request
- **Last owner result:** R6/26186464/A10: PASS
- **Basis:** R3 FAIL -> OCT24 PASS, R5 PASS, R6 26186464 A10 'Held Item search and Sort' PASS (C0290). include/UI/SharedHeldItemSearch.h has no commits after 26186464.
- **Repair commits:** 20955b20 (live Held Item search callback); (Oct 24 8804f8f8 docked Held Item keyboard)
- **Automated tests:** tests/test_item_picker_search.cpp : Held Item search alongside sort in Gen II/III/IV/V pickers; native list never changed; CG40
- **Hardware acceptance:** 26186464 R6 A10
- **Next action:** None.
- **Owner** (B0555): > Add search alongside sorting for supported held-item generations.
- **Occurrences (9):** R3 62a2f8c3 NEW-06 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-06 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-06 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-06 PASS [tests/test_hw_round1_regression_gate.py]; OCT24 c60d4da0 NEW-06 n/a [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-06 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec NEW-06 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A10 · NEW-06 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A10 · NEW-06 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-SRC-004` L/R while searching: L deletes last letter, R inert, screen behind never moves

- **Status:** Hardware Accepted · **Gen:** all · **Area:** search · **Category:** request
- **Last owner result:** R11/3634d1e6/H11: PASS
- **Basis:** b4bfcc93 (d6a20a2a); owner R11 3634d1e6 H11 PASS (E0010). include/UI/SharedSearchInput.h:210-216 (L = Backspace) unchanged after 3634d1e6.
- **Repair commits:** b4bfcc93 Owner UI batch: ... L/R in search, folder preferences
- **Automated tests:** tests/test_search_shoulder_keys.cpp : with the key grid open L is Backspace, R consumed, L/R never page the list behind
- **Hardware acceptance:** 3634d1e6 R11 H11 (R12 H18 smoke repeats it) · R12 item H18
- **Next action:** None beyond R12 H18 smoke.
- **Occurrences (3):** R9 d6a20a2a H11 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H11 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H11 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-STA-001` Gen I Summary/radar correctness (zero stats, modern rows, isShiny false, stray text, radar layout/scale)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** stats · **Category:** defect
- **Last owner result:** R11 3634d1e6 H21: PASS
- **Basis:** Owner accepted real Gen I summary stats, five-axis radar, unfocusable calc cells, shiny presentation on 69668bc8 (A0549-A0553); Pokemon1ReadOnly::isShiny delegates to ShinyDVs (include/Pokemon/Pokemon1ReadOnly.h:89-90); R11 H21 PASS. Box quick view routing rewritten in 9a29863f (BoxSummaryRadar Route::Gen1Special, src/UI/Panels/BoxPokemonPanel.cpp:315-326).
- **Repair commits:** PR #66 Gen I summary/radar correction; 9a29863f G5-RADAR: Gen V box quick view draws the editor's radar
- **Automated tests:** tests/test_gen1_radar_render.cpp : radar layout/scale; tests/test_box_summary_radar.cpp : per-generation radar route
- **Hardware acceptance:** 066e1db9 R12 H18 · R12 item H18
- **Next action:** Include a Gen I box quick-view glance in R12 H18.
- **Owner** (A0541): > Owner asked whether very low DVs produce displayed stats; authentic formulas give HP 24/Atk 13/Def 13/Spe 13/Spc 15, matching device.
- **Occurrences (10):** PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 1 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 2 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 3 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 4 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 5 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 6 n/a [docs/GEN1_SUMMARY_RADAR_RETEST.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]

### `PBX-STA-002` Gen II Special: calculated Sp. Atk / Sp. Def presentation (cramped '10/9', blank cell, detached block)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II · **Area:** stats · **Category:** defect
- **Last owner result:** R11 3634d1e6 H22: FAIL
- **Basis:** ACCEPTED THEN RE-REJECTED: R5 1ca2faec R2-SPECIAL PASS (C0197/C0388) -> R6 26186464 A22/C01 FAIL ('do not squeeze them into the numeric STATS column') -> R7 0e58157b emptied the cell -> R11 H22 FAIL ('we still dont have a number in the calculated column'). Current: include/UI/Gen2WorkspacePresentation.h:37-39 calculatedCell(row 4) = SpA + gap + SpD; src/UI/Gen2UnifiedPokemonWorkspace.inc:930 loop row <= kSpecialRow; View modal src/UI/Modals/Gen2PokemonDetailsModal.cpp:232-234; lower 'CALCULATED SPECIAL STATS' block kept (:938-941).
- **Repair commits:** baada873 Gen II: show calculated Sp. Atk and Sp. Def in the Special row's Calculated cell (H22); 0e58157b feat(gen2): Special shown like Gen I, plain STATS columns (round 7 A22 / C01)
- **Automated tests:** tests/test_gen2_special_stat.cpp : Special cell == SpA + '  ' + SpD, no '/' (lines 58-65); tests/test_gsc_editor_surface_contract.cpp; tests/test_ui_layout_host.cpp
- **Hardware acceptance:** 066e1db9 R12 H12 · R12 item H12
- **Next action:** R12 H12. Risk: the R12 cell again holds two numbers in the numeric column, which the owner rejected in R3 ('cramped 10/9') and R6; get an explicit owner verdict on this exact layout.
- **Owner** (E0021): > we still dont have a number in the calculated column for special calculated stat, add it when i swithc unowns i get a pop in middle bottom about stat changes and i dont need it, its like the language thing we fixed in another problem i also get the pop uo for rerolling the de/ev/iv/ev's can we fix this for all gens and while w are at it when im in gen 5 and go to choose shiny on a pokmon in the middle colum i get a pop up menu instead of a inline choose like we have for pokerus and language and gender,
- **Owner** (C0302): > The calculated Special values are still in the wrong place. Do not squeeze them into the numeric STATS column. Use the Gen I calculated-Special presentation as the reference, in the lower-middle of the centre STATS panel.
- **Owner** (D0210): > ### 1. Gen II Special Values layout — FAIL The owner rejects the Round 4 change that left **Special's Calculated cell blank** and moved both derived Sp. Atk / Sp. Def numbers to an unrelated block below the DV/Stat Exp/Calculated grid. Wants the calculated stat **back inside the correct column**, with readable aligned Special Defense alongside/below, no cramped `10/9` and no empty cell or duplicate unrelated readouts.
- **Occurrences (15):** PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]; PRE special-stat PASS [docs/qa/GEN1_5_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R3 62a2f8c3 R2-SPECIAL FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 R2-SPECIAL FAIL [issue #324 (body 2026-10-24)]; OCT24 c60d4da0 R2-SPECIAL FAIL [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec 7A n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec R2-SPECIAL PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A22 · R2-SPECIAL FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C01 · 7A FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A22, C01 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R11 3634d1e6 H22 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-STA-003` Gen V boxed Pokémon show HP/stats 0

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** stats · **Category:** defect
- **Last owner result:** R3 62a2f8c3 S02: PASS
- **Basis:** Fixed de22e35; owner R3 62a2f8c3 S02 'Gen V shows real saved data' PASS (B0528). Box quick view for Gen V switched to shared radar in 9a29863f.
- **Repair commits:** de22e35f feat(gen5): run Gen V View/Edit/Create through the one shared editor surface
- **Automated tests:** tests/test_box_summary_radar.cpp
- **Hardware acceptance:** 066e1db9 R12 H15 · R12 item H15
- **Next action:** R12 H15.
- **Owner** (A0934): > Owner finding from 35cbf163 hardware test; fixed in editor-parity repair.
- **Occurrences (2):** PRE 35cbf163 P6 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; R3 62a2f8c3 S02 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-STA-004` Shiny vs non-shiny reroll stat legitimacy (E11 / R2-REROLL general)

- **Status:** Needs Investigation · **Gen:** all · **Area:** stats · **Category:** defect
- **Last owner result:** R3 62a2f8c3 R2-REROLL: FAIL
- **Basis:** Owner never PASSed E11 (NOT VERIFIED R1/R2/R3) and FAILed R3 R2-REROLL ('questions whether shiny stats are game-legitimate'). Engineering (docs/REROLL_MECHANICS.md, B0368): Gen I/II shiny is a DV pattern (enforced), Gen III-V shiny is PID^TID^SID<8 so identical IVs are valid; real Gen III rerollPreservingShiny / Gen IV rerollPid defects fixed in 951a8993 (src/Pokemon/Pokemon3FRLG.cpp:239). Unknown: whether the owner accepts the mechanics explanation; E11 has not been on any checklist since R3 and no hardware row checks shiny/gender/nature preservation after reroll.
- **Repair commits:** 951a8993 Merge reroll-mechanics lane (Gen III rerollPreservingShiny, Gen IV rerollPid fixes)
- **Automated tests:** tests/test_reroll_mechanics_gen15.cpp : 100 rerolls per case keep shiny/gender/nature/Unown per gen; tests/test_shared_stat_reroll.cpp; tests/test_gen12_dv_reroll.cpp
- **Hardware acceptance:** none (not on R12 checklist; H13 only checks no toast)
- **Next action:** Send owner the REROLL_MECHANICS explanation and add a row: reroll shiny Gen III/IV/V Pokémon 5x, confirm Shiny/Gender/Nature/Ability kept.
- **Owner** (B0065): > the reroll iv/ev i dont know if the info is correct cause i can have a shiny and non shiny pokemon with the exaclt same stats and i dont thnk that is possible, i think shinies have to have a certain amout of points this needs fixed for all gens.
- **Owner** (B0480): > Owner questions whether shiny stats are game-legitimate; independently audit DV/PID/shiny/Unown form calculations without asserting an unproven defect.
- **Occurrences (14):** PRE reroll PASS [docs/qa/GEN1_5_MATRIX.md]; PRE reroll NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; R1 7fea1797 E11 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 E11 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 E11 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 E11 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E11 n/a [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 E11 NOT TESTED [issue #313 (comment 2026-10-17)]; R2 e10f307f E11 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R3 62a2f8c3 E11 NOT TESTED [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-REROLL FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 E10 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-STA-005` Y rerolls DVs/IVs/EVs immediately without confirmation

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** stats · **Category:** request
- **Last owner result:** R3 62a2f8c3 E10: PASS
- **Basis:** Owner PASS R1 and R3 62a2f8c3 E10 (B0512, with a mechanics note tracked in PBX-STA-004); Gen I Y DV randomization accepted 69668bc8 (A0545). Reroll feedback changed in R11 f2cf5954 (no success toast).
- **Repair commits:** dcd8770c feat(gen1/gen2): Y re-rolls DVs at once; f2cf5954 no toast after routine edits
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG06 no confirmation on reroll; tests/test_edit_feedback.cpp
- **Hardware acceptance:** 066e1db9 R12 H13 · R12 item H13
- **Next action:** R12 H13.
- **Owner** (A0943): > Owner campaign (Gen I/II lane) unit; implemented with host tests, not device-tested here.
- **Occurrences (4):** PRE 6 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE CG06 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]; R3 62a2f8c3 E10 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-STA-006` Nature stat colours (raised green / lowered red) in Gen III–V View/Edit

- **Status:** Hardware Accepted · **Gen:** III, IV, V · **Area:** stats · **Category:** decision
- **Last owner result:** R6 26186464 A13·NEW-09: PASS
- **Basis:** Owner correction (issue #324, D0212) withdrew the neutral-colour complaint; owner PASS R6 26186464 A13 'Keep the green/red Nature colours' (C0293). include/UI/NatureStatEffect.h unchanged since 26186464.
- **Repair commits:** 1241c2b8 fix(ui): retain nature-tinted stats (#324)
- **Automated tests:** tests/test_nature_stat_effect.cpp : 25 natures +/- stat; tests/test_nature_picker_layout.cpp
- **Hardware acceptance:** R6 26186464 A13
- **Next action:** None; do not neutralize colours.
- **Owner** (D0212): > Owner correction (Oct 23): Gen IV nature-linked green/red stat text in View/Edit is intentional and ACCEPTED; restore and keep it as an at-a-glance raised/lowered Nature effect. Revert only the stat-neutralization patch; keep the separate Gen II Special-column repair. Do not treat nature coloring as a bug.
- **Owner** (C0340): > Keep the green/red Nature colours
- **Occurrences (7):** R3 62a2f8c3 NEW-09 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 FAIL [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 FAIL [issue #324 (body 2026-10-24)]; R4 c60d4da0 PASS [issue #324 (comment 2026-10-24)]; R6 26186464 A13 · NEW-09 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A13 · NEW-09 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-STA-007` Gen II Unown: letter, shiny state, DVs and sprite disagree; letter selector confusing

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** II · **Area:** stats · **Category:** defect
- **Last owner result:** R6 26186464 B07·R2-REROLL / C09: FAIL
- **Basis:** fd30e0fc/e0d5102f (sprite follows DVs), 8613b13d letter selector, 2dfad968+47b8097e (R7 B07/C09 letter grid, shiny questions, explained reroll) and f2cf5954 (silent Unown letter apply). Last owner verdict R6 26186464 B07/C09 FAIL. R7-R9 had no owner results; R11 H22 listed Unown comparison in its steps but the owner commented only on the toast — not a PASS.
- **Repair commits:** fd30e0fc fix(gen2): Unown sprite and draft letter follow the DVs; 8613b13d feat(gen2): Unown letter selector; 2dfad968 wip(gen2): Unown Letter grid, Shiny/letter questions, Reroll status (R7 B07/C09); f2cf5954 no toast after routine edits
- **Automated tests:** tests/test_gen2_unown_letter.cpp; tests/test_gen2_unown_letter_selector.cpp : letter/shiny/DV agreement; tests/test_ui_unown_letter_host.cpp
- **Hardware acceptance:** 066e1db9 R12 H13 (letter change) + H16 (Gen II Unown in picker) · R12 item H13
- **Next action:** Ask owner explicitly in R12 H13 whether letter/shiny/DV behaviour is now clear (not just toast-free).
- **Owner** (C0318): > Gen II Unown:<br>• Shiny restricts the available letters.<br>• Turning Shiny off and choosing a letter changes the DVs and the form.<br>• Turning Shiny on again changes the form.<br>• Reroll seems not to work, shiny or not.<br>• Choosing a letter does not explain whether the result should be shiny.<br>Overall it is confusing and feels broken.
- **Owner** (C0171): > Gen II Unown: making it shiny changed A to something that looks like I or V. It is unclear how letter, shiny state and DVs relate.
- **Occurrences (8):** R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; OCT24 c60d4da0 R2-REROLL NOT TESTED [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec R2-REROLL FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P6 n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B07 · R2-REROLL FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C09 · UNOWN-LETTER FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B07, C09 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-STA-008` Gen V box quick-view stat graph not updated / smaller than editor radar

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** stats · **Category:** defect
- **Last owner result:** R11 3634d1e6 Extra 3: FAIL
- **Basis:** src/UI/Panels/BoxPokemonPanel.cpp:315-326 now routes every Gen I-V record through UI/BoxSummaryRadar.h shared radar; Gen V no longer falls into the inherited drawStatHexagon branch (kept only for later formats, :345-356).
- **Repair commits:** 9a29863f G5-RADAR: Gen V box quick view draws the editor's radar
- **Automated tests:** tests/test_box_summary_radar.cpp : route + scale per generation
- **Hardware acceptance:** 066e1db9 R12 H15 · R12 item H15
- **Next action:** Owner runs R12 H15.
- **Owner** (E0054): > gen 5 quick view graph or whatever we call it, the graph when you are in a save in the box and can see all the pokemon in the box moving between pokemon the graph on the quick view doesn't work and isn't updated with the graph that is in the editor and view mode, it also looks smaller in the quick view.
- **Occurrences (3):** R11 3634d1e6 Extra 3 FAIL [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 3 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §3 leads]

### `PBX-TCH-001` Touch app-wide: popups/modals never activate background controls; touch scroll and joystick coexist; no accidental mutation

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** touch · **Category:** requirement · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11/3634d1e6/H47: PASS
- **Basis:** R1 C06/N03 PASS, R3 62a2f8c3 C06/N03 PASS, latest R11 3634d1e6 H47 'Touch and controls agreement' PASS (E0046). Test gap B0612 closed: tests/test_touch_scroll_state.cpp is now built by Makefile.host.pickers. Recheck: new touch targets in 066e1db9 (quit dialog buttons aace6021, X Export/Import box handlers dd69bc93, Recovery files 98b6deaf). Safety: G-2 requires a single tap never deletes/releases/overwrites. V1_ROADMAP Phase 5 (F0170) lists controller+touch coexistence and physical touch-only acceptance.
- **Repair commits:** (P0 #320 top touch layer; beginTopTouchLayer in src/UI/SaveSelectScreen.cpp:5321)
- **Automated tests:** tests/test_touch_modal_list_contract.py : contact and release on same hit rect; modal lists; tests/test_touch_appwide_surface_contract.py : app-wide touch surfaces; tests/test_touch_scroll_state.cpp : horizontal swipe never owns a vertical list; tests/test_gen5_box_touch.cpp : Gen V box touch (first tap focuses, second opens Actions)
- **Hardware acceptance:** 3634d1e6 R11 H47; recheck 066e1db9 (no explicit R12 item)
- **Next action:** Fold a touch-only pass of the new R12 dialogs (H02 quit buttons, H05 X/A actions, H11 Recovery) into those items.
- **Occurrences (27):** PRE Box touch n/a [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE Unsupported touch n/a [docs/GEN5_SAVE_INSTANCE_HARDWARE_SMOKE.md]; PRE G-2 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE C19 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE G01 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE G03 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE CG08 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 C06 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 N03 PASS [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 C06 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 N03 PASS [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 C06 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 N03 PASS [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 C06 PASS [tests/test_hw_round1_regression_gate.py]; R1 7fea1797 N03 PASS [tests/test_hw_round1_regression_gate.py]; R2 C06 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 N03 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f C06 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f N03 n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 C06 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 N03 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 C06 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 N03 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 0faf7e9c Part 9 #5 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R11 3634d1e6 H47 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ Phase 5 n/a [docs/V1_ROADMAP.md]

### `PBX-TCH-002` Quick Games / Quick View finger drag, vertical scroll, momentum, tap opens the right game

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** app · **Area:** touch · **Category:** requirement
- **Last owner result:** R3/62a2f8c3/AUD-05: PASS
- **Basis:** R3 62a2f8c3 AUD-05 'Quick View touch' PASS (B0574). Part 9 #5 found no host repro (B0600). Quick Games overlay changed afterwards by 727fad5d (tap-outside close, e88957f2).
- **Repair commits:** d0a45909 Fix Quick Games vertical finger scrolling without breaking draggable drawer; 498c0687 Open validated game on Quick Games clean tap/A; preserve live drag ownership
- **Automated tests:** tests/test_touch_scroll_state.cpp : swipe direction ownership
- **Hardware acceptance:** 62a2f8c3 R3 AUD-05
- **Next action:** Smoke Quick Games drag/tap with PBX-TCH-007 on 066e1db9.
- **Occurrences (6):** PRE B2 n/a [docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md]; PRE B02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 62a2f8c3 AUD-05 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 0faf7e9c Part 9 #5 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 0faf7e9c n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-05 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-TCH-003` L06: Party / summary-panel taps must open the generation's Actions popup (not View)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** touch · **Category:** defect
- **Last owner result:** R6/26186464/A21: PASS
- **Basis:** R1/R2 NOT VERIFIED, R3 62a2f8c3 FAIL 'Touch goes directly to View; expected Actions popup' (B0467). Fixed c81853da (c60d4da0); owner OCT24 c60d4da0 L06 PASS ('Touch Party / summary opens Actions'), R6 26186464 A21 NEW-20 'Gen I-III Party Actions popup' (A or tap) PASS. src/UI/ClassicPartyActions.inc changed in 066e1db9 (dd69bc93 adds Export / Review rows).
- **Repair commits:** c81853da feat(editor): route classic Party Actions to native staged edits
- **Automated tests:** tests/test_party_summary_tap_routing.cpp : Boxes summary-panel / Party tap opens native Actions (Gen III-V); Gen I/II only CG11; CG11/CG33 (tests/test_qa_cross_gen_contract.py)
- **Hardware acceptance:** 26186464 R6 A21
- **Next action:** Tap a Party Pokemon in Gen I-III during R12 H05 to confirm the new Export/Review rows.
- **Owner** (B0467): > Touch goes directly to View; expected Actions popup.
- **Owner** (B0043): > L06 i dont know
- **Occurrences (14):** R1 7fea1797 L06 NOT TESTED [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R1 7fea1797 L06 NOT TESTED [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 L06 NOT TESTED [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R1 7fea1797 n/a [docs/hardware/RETEST_7fea1797.md]; R1 7fea1797 L06 NOT TESTED [docs/qa/ROUND2_STATUS.md]; R1 7fea1797 L06 NOT TESTED [issue #313 (comment 2026-10-17)]; R2 L06 n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f L06 n/a [docs/hardware/RETEST_e10f307f.md]; R2 e10f307f L06 NOT TESTED [docs/hardware/ROUND2_RESULTS_e10f307f.md]; R2 L06 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS.md]; R3 62a2f8c3 L06 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 L06 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 L06 n/a [docs/hardware/PHASE13_UNVERIFIED_STEPS_ROUND3.md]; R3 62a2f8c3 L06 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-TCH-004` Scrollbars draggable / track-tappable for fast scrolling app-wide

- **Status:** Hardware Accepted · **Gen:** app · **Area:** touch · **Category:** request
- **Last owner result:** R3/62a2f8c3/R2-SCROLLBAR: PASS
- **Basis:** 5bea036d (e10f307f) shared draggable scrollbar; owner R3 62a2f8c3 R2-SCROLLBAR PASS (B0478).
- **Repair commits:** 5bea036d feat(ui): R2-SCROLLBAR draggable scrollbars on every existing scroll surface
- **Automated tests:** tests/test_shared_scrollbar.cpp : drag/track behaviour; selection never touched (CG14)
- **Hardware acceptance:** 62a2f8c3 R3 R2-SCROLLBAR
- **Next action:** None.
- **Owner** (B0061): > SCROLL BARS if they are in a menu can i use them to fast scroll in stead of swiping with finger repeatly, this is for all scoll bar in the pokebank app.
- **Occurrences (7):** PRE CG14 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 R2-SCROLLBAR (Phase 8) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f R2-SCROLLBAR (Phase 8) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-SCROLLBAR PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-SCROLLBAR PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-TCH-005` Games-browser profile avatar touch rect pushed but never hit-tested

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** touch · **Category:** defect
- **Last owner result:** none
- **Basis:** 796ba212 (in 62a2f8c3): src/UI/SaveSelectScreen.cpp:4412-4425 Games browser hit-tests headerRects idx 0 on touch-down/tap and opens the Profile picker. No owner result for this tap.
- **Repair commits:** 796ba212 feat(ui): button badges in help and modal text; provider names in sessions; profile focus kept
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner with >1 Switch profile taps the avatar in the full Games browser.
- **Occurrences (1):** R3 0faf7e9c Part 9 #9 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-TCH-006` 7D: tap outside closes non-destructive popups (destructive confirmations stay); no tap-through

- **Status:** Hardware Accepted · **Gen:** all · **Area:** touch · **Category:** request · ⚠️ safety-critical
- **Last owner result:** R6/26186464/C04+C10: PASS
- **Basis:** 55f31d27/c6e3aef6 (26186464); owner R6 26186464 C04 and C10 PASS (C0323, C0329). SharedPopupDismiss unchanged since 26186464. Safety: tap-through could open/move a slot under a popup.
- **Repair commits:** 55f31d27 feat(pickers): Round 6 L06 held-item tap/A reliability, 7D tap-outside dismissal; efb687eb test(7D): Gen I Species and Gen II Move pickers close on an outside tap
- **Automated tests:** tests/test_hw_round1_regression_gate.py : GUARDS_ROUND7 C04/C10
- **Hardware acceptance:** 26186464 R6 C04/C10
- **Next action:** None.
- **Occurrences (8):** R5 1ca2faec 7D n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 C04 · 7D n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C10 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C10 · Tap-through PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 C04 PASS [tests/test_hw_round1_regression_gate.py]; OCT26 26186464 C10 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A36 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 A40 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TCH-007` Tap-outside dismissal also on the Quick Games panel and other ordinary Home/browser popups

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** touch · **Category:** request
- **Last owner result:** none (R6 C04 PASS carried this as a follow-up request)
- **Basis:** 727fad5d (first in e88957f2): src/UI/SaveSelectScreen.cpp:3533-3538 a completed tap outside an ordinary open overlay (Quick Games, ...) closes it via SharedPopupDismiss::tappedOutside; Quick Games drawer publishes its rect (SaveSelectScreen.cpp:5406). R7 D0059 retest has no recorded result (NRO archive) and the item is absent from R8-R12 lists.
- **Repair commits:** 727fad5d feat(C04): Quick Games and other ordinary overlays close on an outside tap
- **Automated tests:** CG80-CG89 round 7 contracts (tests/test_qa_cross_gen_contract.py, a52d5c5b/8b92d716)
- **Hardware acceptance:** none (not on any checklist); verify on 066e1db9
- **Next action:** Add 'Quick Games -> tap outside, including on a game card' to the R12 smoke.
- **Owner** (C0347): > Also wants tap-outside dismissal on the **Quick Games** panel and other ordinary popups
- **Occurrences (4):** R6 26186464 C04 · 7D PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C04 · 7D n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C04 follow-up n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 C04 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TRN-001` Gen III trainer read model (FRLG trainer name/gender/TID/SID/money); FireRed trainer/item reads only PARTIAL

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** III · **Area:** trainer · **Category:** defect
- **Last owner result:** R6/26186464/A05: PASS
- **Basis:** FRLG reads PASS/PARTIAL on ea0b806b (D0268); RSE trainer PASS 46e0c161 (D0271); Gen III trainer later accepted with staged edits R6 26186464 A05 (C0285). Trainer page painter changed after R6 (7eab05cf).
- **Repair commits:** ea1a7a4a feat(gen3): stage Trainer Name / Money for Ruby, Sapphire, Emerald, FireRed, LeafGreen (R2-BUG-002)
- **Automated tests:** tests/test_gen3_trainer_edit.cpp : Gen III trainer fields stage/round-trip
- **Hardware acceptance:** 26186464 R6 A05 · R12 item H18
- **Next action:** Gen III Trainer page in R12 H18 smoke.
- **Owner** (D0268): > FIRERED TRAINER/ITEM READS: PHYSICAL PASS/PARTIAL
- **Occurrences (2):** PRE 5d3e5e23 n/a [docs/DEVICE_TEST_CHECKLIST.md]; PRE ea0b806b PARTIAL [issue #6 (comment 2026-09-09)]

### `PBX-TRN-002` Gen II Trainer screen read-only/unusable for staged editing

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** II · **Area:** trainer · **Category:** defect
- **Last owner result:** R6/26186464/A30: PASS
- **Basis:** cfb70357 accepted (PR #68); Gen II trainer edits later PASS R6 26186464 A30 (C0310, 'Supported Trainer editing works. Keep it.'). Current: include/Integration/Gen2/Gen2StagedEditor.h:92-116. Painter changed after R6 (7eab05cf).
- **Repair commits:** cfb70357 fix: address Gen I and II hardware retest regressions
- **Automated tests:** tests/test_trainer_playtime_gender_rows.cpp : Gen II Play Time / Crystal Gender rows
- **Hardware acceptance:** 26186464 R6 A30 · R12 item H18
- **Next action:** Gen II Trainer page in R12 H18 smoke.
- **Owner** (D0290): > - Gen II Trainer screen is effectively read-only/unusable for staged editing.
- **Occurrences (1):** PRE 91a42970 FAIL [PR #68 (comment 2026-09-14)]

### `PBX-TRN-003` Gen V newly initialized save showed a fake male trainer portrait; must report new game / no trainer, never guess gender

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** V · **Area:** trainer · **Category:** defect
- **Last owner result:** PRE/(gate doc)/Black 2 new-game: FAIL
- **Basis:** 35cbf163 adds explicit new-game state: include/Integration/Gen5/Gen5TrainerName.h:16 newGameNoTrainer5 used at include/Integration/Gen5/Gen5SharedScreenState.h:478 and Gen5GameCardPreview.h:58. No later owner result on a new-game Gen V save.
- **Repair commits:** 35cbf163 fix(gen5): explicit new-game save state; never show an uninitialized trainer as Male
- **Automated tests:** tests/test_gen5_trainer_footer.cpp : new-game trainer shows neutral; tests/test_gen5_save_diagnostics.cpp : new-game diagnostics
- **Hardware acceptance:** none (not on any checklist since the GEN15 gate)
- **Next action:** Owner opens a fresh Black 2 DraStic save on 066e1db9: expect new game / no trainer, neutral portrait.
- **Occurrences (3):** PRE A07 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E03 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE  FAIL [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]

### `PBX-TRN-004` Gen V Trainer: separate popup and list conventions (clamp, first-tap focus, focusable read-only rows); clutter (R2-BUG-003)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** trainer · **Category:** defect
- **Last owner result:** R11/3634d1e6/H09: PASS
- **Basis:** 7123035d (shared panel) + ad95b2de (conventions); R2-BUG-003 PASS R3 62a2f8c3 (B0532); Gen V Trainer used in R11 3634d1e6 H09 PASS (E0008). Current: include/Integration/Gen5/Gen5TrainerScreen.h:25-47. Gen5TrainerName.h display changed after 3634d1e6 (d1954551).
- **Repair commits:** 7123035d fix(gen5): open on the shared loaded-game hub; Trainer is the shared panel; ad95b2de fix(gen5): Trainer and Items follow the Gen I-IV list conventions; 4f549775 feat(gen5): Gen V Trainer on the one Trainer page model; CG83 / CG84 / CG90 enforced
- **Automated tests:** tests/test_gen5_trainer.cpp : rows, editability, Up/Down wrap; tests/test_gen5_trainer_footer.cpp : A: Edit only on editable rows
- **Hardware acceptance:** 3634d1e6 R11 H09 · R12 item H14
- **Next action:** R12 H14 (Gen V names) exercises the Gen V trainer name display change.
- **Occurrences (7):** PRE  FAIL [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE 35cbf163 P6 FAIL [docs/handoffs/GEN5_9_PROGRESS.md]; PRE E01 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; PRE E02 n/a [docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md]; R3 62a2f8c3 R2-BUG-003 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-003 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-003 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-TRN-005` Gen I Trainer Name/Money editable; 'A: Edit' footer wherever a field is editable, all gens (R2-TRAINER / R2-BUG-001)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, all · **Area:** trainer · **Category:** request
- **Last owner result:** R6/26186464/B02 (R2-TRAINER): PASS
- **Basis:** R2-BUG-001 PASS R3 62a2f8c3 (B0530); Gen I Name/Money PASS OCT24 c60d4da0 R2-TRAINER (C0074) and R6 26186464 B02·R2-TRAINER (C0313). Current: include/Integration/Gen1/Gen1StagedTrainerEditor.h:59-67,161-181; editability include/UI/ClassicTrainerEditability.h; page rows include/UI/ClassicTrainerPage.h:55-71. Page painter changed after R6 (7eab05cf, 4f549775).
- **Repair commits:** 6d6c55d5 feat(gen1): staged Trainer Name / Money editing; honest A Edit Trainer footer for every generation; cccec27b WIP: fix(trainer): editable Trainer page keeps its A: Edit footer on a read-only source (R2-BUG-001); 06329b5a R2-BUG-001 parity: Gen V Trainer footer shows A: Edit only when the focused row is editable
- **Automated tests:** tests/test_gen1_trainer_edit.cpp : Gen I name/money stage + export; tests/test_classic_trainer_page.cpp : rows/editability; tests/test_hw_round1_regression_gate.py : R2-BUG-001 guard (CG18)
- **Hardware acceptance:** 26186464 R6 B02; 62a2f8c3 R3 R2-BUG-001 · R12 item H18
- **Next action:** Gen I Trainer Name/Money edit in R12 H18 smoke (new painter).
- **Owner** (B0057): > gen 1 trainer menu doesnt allow me to edit the trainer and money stats, but in gen 2 i can we need to add a footer saying A to edit and also the ablity to edit in gen 1 and the footer in both gens and this needs done for all gens btw,
- **Occurrences (11):** PRE trainer-name PASS [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-edit PASS [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-edit NOT TESTED [docs/qa/GEN1_5_MATRIX.md]; PRE CG18 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R2 R2-TRAINER (Phase 5) n/a [docs/hardware/RETEST_ROUND2_TEMPLATE.md]; R2 e10f307f R2-TRAINER (Phase 5) n/a [docs/hardware/RETEST_e10f307f.md]; R3 62a2f8c3 R2-BUG-001 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-001 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-001 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-TRN-006` Trainer edits must reach Review; staged exit offers Review / Continue / Discard / Export Copy (Gen I-IV, R2-BUG-002)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV · **Area:** trainer · **Category:** safety · ⚠️ safety-critical
- **Last owner result:** R6/26186464/A05: PASS
- **Basis:** FAILs R3 (B0475, B0531) fixed 29084707 / a7244a64; owner PASS OCT24 c60d4da0, R5 1ca2faec, R6 26186464 A05 (C0285). Gen I Review also exercised R11 3634d1e6 H10 PASS (E0009). Current session/exit: include/UI/ClassicTrainerSession.h; include/UI/ClassicStagedExit.h. Trainer page code changed after R6 (7eab05cf).
- **Repair commits:** 29084707 Open classic Trainer Review and stage-safe exit choices with named changes; a7244a64 fix(gen1-4): Review opened outside Box Actions returns to its screen; Bag uses the shared Review (round 5 P1); ea1a7a4a feat(gen3): stage Trainer Name / Money for Ruby, Sapphire, Emerald, FireRed, LeafGreen (R2-BUG-002); e5dec3a2 WIP: feat(gen4): stage Trainer Name / Money in the Gen IV workspace (R2-BUG-002)
- **Automated tests:** tests/test_gen3_trainer_edit.cpp : Gen III name/money stage, Review, export; tests/test_hw_round1_regression_gate.py : GUARDS_ROUND5 R2-TRAINER / R2-BUG-002
- **Hardware acceptance:** 26186464 R6 A05 · R12 item H17
- **Next action:** Include a Trainer edit in R12 H17 Review/Keep/Discard/Export pass for Gen III/IV.
- **Owner** (B0475): > No Review; Review needs adding to Gen I–IV.
- **Owner** (B0531): > No way to access Review; exiting shows Discard/Cancel only, even with staged changes.
- **Occurrences (15):** R3 62a2f8c3 R2-BUG-002 n/a [docs/hardware/RETEST_62a2f8c3.md]; R3 62a2f8c3 R2-TRAINER FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 R2-BUG-002 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 R2-TRAINER n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 R2-BUG-002 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-TRAINER PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-BUG-002 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 R2-TRAINER PASS [tests/test_hw_round1_regression_gate.py]; OCT24 c60d4da0 R2-BUG-002 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec R2-TRAINER n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec R2-BUG-002 n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec R2-BUG-002 PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 A05 · R2-BUG-002 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A05 · R2-BUG-002 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A05 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TRN-007` Gen I-IV Trainer: native Play Time and Pokedex seen/caught like Gen V (NEW-18 data)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV · **Area:** trainer · **Category:** request
- **Last owner result:** OCT24/c60d4da0/NEW-18: PASS
- **Basis:** 20955b20 native classic summaries; owner PASS OCT24 c60d4da0 NEW-18 (C0096). Rows: include/UI/ClassicTrainerPage.h:30,64-71 (Play Time, Badges, Pokedex). The later NEW-18 FAILs (R5/R6) are about visual design and are tracked in PBX-TRN-010.
- **Repair commits:** 20955b20 Share truthful Origin panels and native classic Trainer summaries with picker regressions guarded
- **Automated tests:** tests/test_native_trainer_badges.cpp : native badges incl. Johto/Kanto; tests/test_classic_trainer_page.cpp : Play Time / Pokedex rows
- **Hardware acceptance:** c60d4da0 OCT24 NEW-18 · R12 item H18
- **Next action:** Covered by the visual retest in PBX-TRN-010.
- **Owner** (B0533): > Owner wants Play Time and Pokédex on Gen I–IV too.
- **Owner** (B0567): > Add native Play Time and Pokédex seen/caught summaries to Gen I–IV like Gen V.
- **Occurrences (7):** PRE pokedex PASS [docs/qa/GEN1_5_MATRIX.md]; R3 62a2f8c3 R2-BUG-003 n/a [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R3 62a2f8c3 NEW-18 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R4 c60d4da0 NEW-18 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-18 PASS [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 c60d4da0 NEW-18 PASS [tests/test_hw_round1_regression_gate.py]; R5 1ca2faec NEW-18 n/a [docs/hardware/RETEST_1ca2faec.md]

### `PBX-TRN-008` Trainer Info parity: badges, Trainer Gender visible where stored, editable Play Time (Gen II-IV) and Gender (Crystal, Gen III-IV)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II, III, IV, V · **Area:** trainer · **Category:** request
- **Last owner result:** R6/26186464/A30: PASS
- **Basis:** f775db01 stages Play Time (Gen II-IV) and Gender (Crystal, Gen III-IV); owner PASS R5 1ca2faec P3-TRAINER and R6 26186464 A30 ('Supported Trainer editing works. Keep it.'). Current: include/Integration/Gen2/Gen2StagedEditor.h:113-116; include/Integration/Gen3/Gen3StagedTrainerEditor.h:180-190; include/Integration/Gen4/Gen4StagedTrainerEditor.h:50-54; rows include/UI/ClassicTrainerPage.h:55-71. Painter changed after R6 (7eab05cf). Badges row display-only (ClassicTrainerPage.h:26-27,70).
- **Repair commits:** f775db01 P3: stage Play Time (Gen II-IV) and Trainer Gender (Crystal, Gen III-IV); 20955b20 Share truthful Origin panels and native classic Trainer summaries with picker regressions guarded
- **Automated tests:** tests/test_trainer_playtime_gender_rows.cpp : editable rows only with native writers; tests/test_native_trainer_badges.cpp : badges; tests/test_hw_round1_regression_gate.py : GUARDS_ROUND6 P3-TRAINER
- **Hardware acceptance:** 26186464 R6 A30 · R12 item H18
- **Next action:** Gen II-IV Play Time / Gender edit in R12 H18 smoke (new painter).
- **Owner** (D0209): > The owner requests badges + Trainer Gender on Gen I–IV Trainer Info and editable Trainer Gender / Play Time where the native save permits.
- **Owner** (C0310): > Supported Trainer editing works. Keep it.
- **Occurrences (11):** R4 c60d4da0 n/a [docs/hardware/ROUND4_REPAIRS_AND_RETEST.md]; R4 c60d4da0 n/a [issue #323 (body 2026-10-24)]; OCT24 8804f8f8 3 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; OCT24 8804f8f8 n/a [docs/hardware/OCT24_REPAIRS_AND_RETEST.md]; R5 1ca2faec P3 Trainer n/a [docs/hardware/RETEST_1ca2faec.md]; R5 1ca2faec P3-TRAINER PASS [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; OCT25 1ca2faec P3-TRAINER PASS [tests/test_hw_round1_regression_gate.py]; R6 26186464 A30 · P3-TRAINER n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 A30 · P3-TRAINER n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 A30 · P3-TRAINER PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A29 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TRN-009` Gold/Silver Trainer Gender rows overlap ('Male' under new row); Gold/Silver/Gen I store no gender

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I, II · **Area:** trainer · **Category:** defect
- **Last owner result:** R6/26186464/B02: PASS
- **Basis:** d2a7f7d9 Gold/Silver report no gender; eb0ae8ee rebuilt layout; owner PASS R6 26186464 B02 ('The Gold/Silver gender overlap is gone.'). Current: no Gender row when not stored include/UI/ClassicTrainerPage.h:24,63-66. Painter changed after R6 (7eab05cf).
- **Repair commits:** d2a7f7d9 fix(gen2): Gold / Silver report no trainer gender (CG72, round 6 P2); eb0ae8ee feat(gen1-4): Trainer Info in the Gen V design, gated (round 6 P2)
- **Automated tests:** tests/test_hw_round1_regression_gate.py : B02 guard 'no Gold / Silver gender overlap'; tests/test_classic_trainer_page.cpp : no Gender row for Gold/Silver/Gen I
- **Hardware acceptance:** 26186464 R6 B02 · R12 item H18
- **Next action:** Recheck Gold/Silver Trainer during PBX-TRN-010 visual retest.
- **Owner** (C0166): > The Gold/Silver Trainer Gender rows overlap: an old "Male" label sits under the new row. Older Trainer layouts are inconsistent.
- **Occurrences (12):** PRE trainer-gender PASS [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-gender n/a [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-gender n/a [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-gender n/a [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-gender n/a [docs/qa/GEN1_5_MATRIX.md]; PRE trainer-gender n/a [docs/qa/GEN1_5_MATRIX.md]; PRE trainer.gender n/a [docs/qa/GEN1_5_MATRIX.md]; R5 1ca2faec R2-TRAINER FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B02 · R2-TRAINER n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 B02 · R2-TRAINER PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; OCT26 26186464 B02 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A32 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TRN-010` Gen I-IV Trainer must match the Gen V visual standard (neutral values, consistent rows, no patched boxes); Y Review Changes button too large / overlaps rows

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** I, II, III, IV, V · **Area:** trainer · **Category:** defect
- **Last owner result:** R6/26186464/B06,C06: FAIL
- **Basis:** Last owner result R6 26186464 B06 and C06 FAIL. Fix: one shared Trainer painter include/UI/SharedTrainerPanel.h:9-27 (neutral valueColor :22) and page model include/UI/ClassicTrainerPage.h:87-141 (measured Review slot kReviewW=200,kReviewH=36 :96, fits check :141). In e88957f2 (R7) and every later candidate; R7 B04/C06 had no owner results and the item was not on R8-R12 checklists. R11 H09/H10 PASS touched the Trainer page but did not test the visual standard.
- **Repair commits:** 7eab05cf feat(ui): Gen I-IV Trainer painted by Gen V's Trainer painter (round 7 B06 / C06); 4f549775 feat(gen5): Gen V Trainer on the one Trainer page model; CG83 / CG84 / CG90 enforced; 8c5d89af Merge lane C 73248a15: one shared Trainer painter Gen I-V incl. Gen V measured Review slot (B06/C06)
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG83/CG84/CG90 Trainer page layout, Review slot below last row, no overlap; tests/test_classic_trainer_page.cpp : page geometry
- **Hardware acceptance:** none (not on any checklist since R7 e88957f2 B04/C06, which have no owner results)
- **Next action:** Put 'Trainer Gen I-IV side by side with Gen V; Review button placement' back on the R12/R13 checklist.
- **Owner** (C0317): > Gen I–IV Trainer still looks like a hybrid:<br>• yellow values where Gen V uses neutral text;<br>• awkward boxes and inconsistent row backgrounds;<br>• layout pieces that look patched together.<br>Gen V Trainer is the standard.
- **Owner** (C0325): > Trainer looks inconsistent with Gen V. The Y Review Changes button is too large or badly placed and overlaps rows.
- **Occurrences (11):** R5 1ca2faec NEW-18 FAIL [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec P2 n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R5 1ca2faec 7F n/a [docs/hardware/ROUND5_RESULTS_1ca2faec.md]; R6 26186464 B06 · NEW-18 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C06 · 7F n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 B06 · NEW-18 FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 C06 · 7F FAIL [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B02 · R2-TRAINER n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 B06, C06, B02/A30 follow-ups n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 B04 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]; R7 e88957f2 C06 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-TRN-011` Gen V Trainer Gender editable (native writer) and Pokedex in the read-only section (H09)

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** trainer · **Category:** request
- **Last owner result:** R11/3634d1e6/H09: PASS
- **Basis:** edb7bda1; owner PASS R11 3634d1e6 H09 (E0008). Current include/Integration/Gen5/Gen5TrainerScreen.h:19,25-35. Gen5TrainerName.h changed after 3634d1e6 (d1954551, display text only).
- **Repair commits:** edb7bda1 Gen V: Trainer Gender staged with a native writer, Pokedex with the read-only rows (#5); inline Pokemon Gender (#6)
- **Automated tests:** tests/test_gen5_trainer.cpp : Gender editable/focusable, value 'Female' after stage
- **Hardware acceptance:** 3634d1e6 R11 H09 · R12 item H14
- **Next action:** None beyond R12 H14/H18.
- **Occurrences (3):** R9 d6a20a2a H09 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H09 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H09 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-TRN-012` Gen I Play Time editable (hours up to 255) and listed in Review (H10)

- **Status:** Hardware Accepted · **Gen:** I · **Area:** trainer · **Category:** request
- **Last owner result:** R11/3634d1e6/H10: PASS
- **Basis:** b4bfcc93; owner PASS R11 3634d1e6 H10 (E0009). include/Integration/Gen1/Gen1StagedTrainerEditor.h:67,181; no change to Gen I trainer editor / ClassicTrainerPage.h / ClassicTrainerSessionInput.h since 3634d1e6.
- **Repair commits:** b4bfcc93 Owner UI batch: Gen V Create Met Location choice, Gen I Play Time, L/R in search, folder preferences
- **Automated tests:** tests/test_gen1_trainer_edit.cpp : Play Time stage bounds
- **Hardware acceptance:** 3634d1e6 R11 H10
- **Next action:** None; close.
- **Occurrences (3):** R9 d6a20a2a H10 n/a [docs/hardware/RETEST_d6a20a2a.md (same items in RETEST_ROUND9_TEMPLAT…]; R10 3634d1e6 H10 n/a [docs/hardware/RETEST_3634d1e6.md (same items in RETEST_ROUND10_TEMPLA…]; R11 3634d1e6 H10 PASS [docs/hardware/ROUND11_OWNER_WORKSHEET.md]

### `PBX-TRN-013` Gen IV trainer-name propagation: Product Home / Games cards show the real Gen IV trainer name read from the save (V1 roadmap Phase 4, checked)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** IV · **Area:** trainer · **Category:** requirement
- **Last owner result:** none
- **Basis:** docs/V1_ROADMAP.md:101 '[x] Gen IV trainer-name propagation'; docs/GAME_SUPPORT_MATRIX.md:99. Implemented by b20b91ba (2026-09-29, 'ui: show real Pokedex progress and Gen IV trainer names'): SaveSelectScreen reads the trainer from the opened Gen IV save (Utils::utf16ToUtf8(opened.save->trainer().name)); current source copies it from the assigned-source snapshot into the card at src/UI/SaveSelectScreen.cpp:1174-1180 and previews at :1205-1211. tests/test_home_card_gen4_snapshot.cpp:160-166 asserts the snapshot trainerName equals the save's own trainer name ('ASH') and gender. No owner hardware item names the Gen IV trainer name on Home; broad Home PASSes are not counted.
- **Repair commits:** b20b91ba ui: show real Pokedex progress and Gen IV trainer names
- **Automated tests:** tests/test_home_card_gen4_snapshot.cpp:160-166 : Home card snapshot trainerName/gender equal the Gen IV save's trainer
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Add 'Gen IV Home card shows the save's trainer name and gender' to the next hardware checklist (can be observed during R12 H01).
- **Occurrences (1):** REQ Phase 4 n/a [docs/V1_ROADMAP.md]

### `PBX-VEW-001` Party / Boxes browsing and data sanity across saves

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** all · **Area:** pokemon-view · **Category:** requirement
- **Last owner result:** R11 3634d1e6 H02: PASS
- **Basis:** Repeated owner PASS since 3be4de6b (A0013/A0014); latest R11 3634d1e6 H02 'One save per generation' PASS (open-save record E0001) and H15 smoke PASS. R11 changed box/Actions/carry code.
- **Automated tests:** docs/qa/GEN1_5_MATRIX.md party/boxes host cells (not hardware)
- **Hardware acceptance:** 066e1db9 R12 H18 · R12 item H18
- **Next action:** R12 H18 smoke.
- **Owner** (A0139): > Left mostly incomplete in the extended pass.
- **Occurrences (8):** PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b NOT TESTED [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE party PASS [docs/qa/GEN1_5_MATRIX.md]; PRE boxes PASS [docs/qa/GEN1_5_MATRIX.md]; PRE  PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b PASS [docs/DEVICE_TEST_REPORT_2026-09-01.md]

### `PBX-VEW-002` Summary should show Species and Type explicitly (tester read SPECIES as type)

- **Status:** Hardware Accepted · **Gen:** app · **Area:** pokemon-view · **Category:** request
- **Last owner result:** PRE 5a2642ac: PASS
- **Basis:** Editors/View label 'Species' rows (e.g. src/UI/Gen1PokemonEditorFoundationHardwareFix.inc:212, src/UI/Gen2HardwareFinalFix.inc:99) and draw type icons (src/UI/Panels/BoxPokemonPanel.cpp drawTypeIcon). Gen I passive View accepted 5a2642ac (D0288).
- **Hardware acceptance:** 5a2642ac PRE
- **Next action:** None.
- **Owner** (A0186): > Tester read SPECIES as type; future Summary should show Species and Type explicitly.
- **Occurrences (1):** PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-VEW-003` View/Pokédex with cry, Pokédex description and location

- **Status:** Missing Feature · **Gen:** app · **Area:** pokemon-view · **Category:** request
- **Last owner result:** none
- **Basis:** No cry playback or Pokédex flavour text in Gen I-V View (grep cry/flavorText/dexEntry: only SV/SWSH regional dex tables, not shown in Gen I-V). Visual part delivered (PBX-VIS-002).
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner to decide whether cry/Pokédex text is in v1 scope.
- **Owner** (A0187): > Tester wants these in future View/Pokédex; product work, not a blocker.
- **Occurrences (1):** PRE 3be4de6b n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]

### `PBX-VEW-004` Read-only View reuses PKSE-style detail experience; keep five-stat radar

- **Status:** Hardware Accepted · **Gen:** I · **Area:** pokemon-view · **Category:** request
- **Last owner result:** PRE 5a2642ac: PASS
- **Basis:** Gen I Party/Storage passive View accepted 5a2642ac (D0288); five-axis radar accepted 69668bc8 (A0551).
- **Hardware acceptance:** 5a2642ac PRE
- **Next action:** None.
- **Owner** (A0536): > Focused Cleanup #3 requirement.
- **Occurrences (2):** PRE 6ecd9c28 3 n/a [docs/GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md]; PRE 5a2642ac PASS [PR #67 (comment 2026-09-14)]

### `PBX-VEW-005` Gen II Create/View native fields (Friendship, EXP, Egg, Crystal caught); no fabricated modern fields

- **Status:** Hardware Accepted · **Gen:** II · **Area:** pokemon-view · **Category:** request
- **Last owner result:** R3 62a2f8c3 L01: PASS
- **Basis:** Gen II shared editor milestone accepted cfb70357 (D0303); Gen II DATA layout PASS R3 62a2f8c3 L01 (B0516).
- **Hardware acceptance:** cfb70357 PRE (PR #68)
- **Next action:** None.
- **Owner** (D0301): > - **Gen II Create/View data:** current dedicated GEN II DATA/BATTLE STATS layout is much better. Continue this layout and add only native fields that actually exist in Gen II. Candidate requested references include Friendship and EXP (native), Egg status where truly represented, and Crystal caught/met data. Do **not** fabricate Ability, PID, Nature, Ball, dates, or other later-gen fields in Gen I/II.
- **Occurrences (2):** PRE 501093ae n/a [PR #68 (comment 2026-09-15)]; R3 62a2f8c3 L01 PASS [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]

### `PBX-VEW-006` Platinum party-only save: no editable target; Party opened inherited read-only details (G4-03)

- **Status:** Hardware Accepted · **Gen:** IV · **Area:** pokemon-view · **Category:** defect
- **Last owner result:** PRE 84dae170: PASS
- **Basis:** Owner PASS for G4-03 Party + Box staged View/Edit on 84dae170 (D0314).
- **Hardware acceptance:** 84dae170 PRE
- **Next action:** None.
- **Owner** (D0313): > Result: **HARDWARE FAIL / DO NOT ACCEPT**
- **Occurrences (1):** PRE 5e79e9f8 FAIL [issue #86 (comment 2026-09-28)]

### `PBX-VEW-007` Gen II DATA pane text wraps/clips inside its pane (SharedDataPane::layoutRows)

- **Status:** Hardware Accepted · **Gen:** II · **Area:** pokemon-view · **Category:** request
- **Last owner result:** R3 62a2f8c3 L01: PASS
- **Basis:** Owner PASS R1 7fea1797 and R3 62a2f8c3 L01 'Gen II DATA layout' (B0516).
- **Repair commits:** e59df023 Data pane text stays inside its pane
- **Hardware acceptance:** R3 62a2f8c3 L01
- **Next action:** None.
- **Owner** (A0945): > Owner campaign (Gen I/II lane) unit; implemented with host tests, not device-tested here.
- **Occurrences (2):** PRE 9 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]; PRE item 6 n/a [docs/handoffs/GEN14_CAMPAIGN_PROGRESS.md]

### `PBX-VEW-008` Selected species shows '25 Pikachu' instead of name first

- **Status:** Hardware Accepted · **Gen:** all · **Area:** pokemon-view · **Category:** defect
- **Last owner result:** R6 26186464 A14·NEW-11: PASS
- **Basis:** Owner PASS OCT24, R5 and R6 26186464 A14 (C0294).
- **Hardware acceptance:** R6 26186464 A14
- **Next action:** None.
- **Owner** (B0560): > Selected left panel show Pikachu or Pikachu #25, not leading 25 Pikachu; keep Dex numbers in lists.
- **Occurrences (3):** R3 62a2f8c3 NEW-11 FAIL [docs/hardware/ROUND3_RESULTS_62a2f8c3.md]; R6 26186464 A14 · NEW-11 PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R7 e88957f2 A14 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-VEW-009` View-only notice (L06 follow-up): A or a tap on a field in View must say why nothing changes - accepted for Gen III-V, never implemented for Gen I/II (silent dead taps); Gen IV lock rules lacked a harness fixture

- **Status:** Open Defect · **Gen:** I, II, III, IV, V · **Area:** pokemon-view · **Category:** defect
- **Last owner result:** R6/26186464/C11 (Gen III-V only): PASS; Gen I/II: none
- **Basis:** Gen III-V part: 552658dc (2026-10-26, in 26186464) added include/UI/SharedViewOnlyNotice.h ('One wording for Gen III, IV and V', :4-5), posted by viewOnlyActivate in src/UI/SharedPokemonEditorSurface.inc:643-671 (Gen IV/V adapters) and src/UI/Gen3SharedPokemonSurface.inc:525; owner PASS R6 26186464 C11 'View-only notice' (C0330, Oct 26 C0408); git log -L shows no change to either function since 26186464. Gen I/II part (C0425, still open): the R6/R7 checklists state 'Gen I/II use different screens and are not covered' (C0271, D0040); Gen I View (src/UI/Gen1PokemonEditorPassiveView.inc:12-31) and Gen II View (src/UI/Gen2SharedPokemonSurface.inc:505-509) only move focus and handle B/Back, so A or a tap on a field posts nothing. No commit after 552658dc adds the notice to Gen I/II. Gen IV harness gap (C0426) is resolved in tests only: a6eb6242 adds gen4ViewOnly (tests/test_ui_editor_input_host.cpp:1212-1256) exercising pidRowLocked (src/UI/Gen4SharedPokemonSurface.inc:1413-1418) through the real screen.
- **Repair commits:** 552658dc L06 View: A or a tap on a field says why nothing changes (Gen III / IV / V); a6eb6242 Harness: Gen IV View lock rules through the real screen
- **Automated tests:** tests/test_ui_editor_input_host.cpp:1145-1256 : Gen III/IV/V View A/tap posts 'View only: choose Edit to change ...' or '... is read-only for this Pokemon', opens no prompt and stages nothing (no Gen I/II case)
- **Hardware acceptance:** Gen III-V accepted 26186464 R6 C11; Gen I/II: none (not on any checklist)
- **Next action:** Route Gen I and Gen II passive View A/tap through PokeBank::UIModel::ViewOnlyNotice (editHint/lockedHint) with host tests, then add a View-only notice item for Gen I/II to the next hardware checklist.
- **Occurrences (6):** R6 26186464 C11 n/a [docs/hardware/RETEST_26186464.md]; R6 26186464 C11 · View-only notice PASS [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; R6 26186464 n/a [docs/handoffs/MAIN_ACTIVE_HANDOFF.md]; OCT26 26186464 C11 PASS [tests/test_hw_round1_regression_gate.py]; R7 e88957f2 A41 n/a [docs/hardware/RETEST_e88957f2.md (same items in RETEST_ROUND7_TEMPLAT…]

### `PBX-VIS-001` PokeBank NX product identity: inherited PKSE branding/logo dominant

- **Status:** Hardware Accepted · **Gen:** app · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE af2acf04 (issue #13 comment 2026-09-06): PASS
- **Basis:** Owner accepted the red PokeBank NX shell on af2acf04 (D0266, issue #13 comment 2026-09-06: 'UI is good enough for now; the permanent left-edge bar is gone'). No later identity complaint in R1-R11. HOME card scenery changed in R11 (c21d3747) but that is tracked as PBX-VIS-013.
- **Repair commits:** af2acf04 (visual shell candidate, issue #13)
- **Hardware acceptance:** af2acf04 PRE (issue #13)
- **Next action:** None; closed. Keep identity in the R12 H01 HOME-card smoke.
- **Owner** (D0264): > The user reports the visible UI remains essentially identical to inherited PKSE, including PKSE branding/logo. The most obvious PokeBank NX identity is the `+` Options menu showing `Exit PokeBank NX`.
- **Owner** (A0052): > FAIL / incomplete
- **Occurrences (11):** PRE 3be4de6b FAIL [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE  n/a [docs/DEVICE_TEST_REPORT_2026-09-01.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-01.md]; PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 3be4de6b FAIL [issue #8 (comment 2026-09-02)]; PRE af2acf04 PASS [issue #13 (comment 2026-09-06)]; PRE 4 n/a [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 4 n/a [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE  n/a [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]

### `PBX-VIS-002` View/Summary lacks a Pokémon image/sprite

- **Status:** Hardware Accepted · **Gen:** app · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** R3 62a2f8c3 AUD-02: PASS
- **Basis:** Owner PASS 'POKEMON ARTWORK' / 'ARTWORK QUALITY: GOOD' on 0ea98cc1 (A0230/A0231); later R3 62a2f8c3 AUD-02 'Species sprites and restrictions' PASS (B0684). Sprites drawn via containSprite in src/UI/PKSEFramebuffer.cpp:655-658.
- **Repair commits:** 0ea98cc1 (issue #25 artwork candidate)
- **Automated tests:** tests/test_item_sprite_coverage.py : item sprite coverage (species art covered by NRO RomFS gate)
- **Hardware acceptance:** R3 62a2f8c3 AUD-02
- **Next action:** None.
- **Owner** (A0178): > FAIL / missing in artifact
- **Occurrences (5):** PRE 3be4de6b FAIL [docs/SESSION_LOG_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md]; PRE 3be4de6b FAIL [docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md]; PRE 0ea98cc1 PASS [docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md]; R3 62a2f8c3 AUD-02 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-VIS-003` Optional Right Stick 3D model rotation in Summary

- **Status:** Missing Feature · **Gen:** app · **Area:** visuals-sprites · **Category:** request
- **Last owner result:** none
- **Basis:** No 3D model or right-stick rotation code anywhere (grep 'model rotation'/'3D model' in src/include: none). Owner framed it as optional QoL; 'strong sprite/artwork Summary is sufficient' (issue #25). Not reachable from UI.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner to confirm it can be dropped from scope (would then be Intentionally Unsupported); not v1.
- **Owner** (A0076): > Later optional QoL idea only if practical; strong sprite/artwork Summary is sufficient.
- **Occurrences (1):** PRE 3be4de6b n/a [docs/SESSION_LOG_2026-09-02.md]

### `PBX-VIS-004` Artificial idle/breathing motion on static Pokémon artwork (must be static, centred, aspect-preserving)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE 0ea98cc1: FAIL / REMOVE
- **Basis:** PKSEFramebuffer::drawSpriteIdle (src/UI/PKSEFramebuffer.cpp:661-675, bob + squash/stretch) still exists but has NO callers in src/ (grep 'drawSpriteIdle(' finds only the definition); artwork goes through the static containSprite path (PKSEFramebuffer.cpp:655-658). The af2acf04 checklist row 'STATIC ARTWORK REMAINS STILL/UNDISTORTED' (A0238) was never given an owner result; af2acf04 shell acceptance (D0266) does not name it.
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Ask owner to confirm artwork is static during R12 H16/H18 smoke, or add an explicit row; optionally delete dead drawSpriteIdle.
- **Owner** (A0232): > FAIL / REMOVE
- **Occurrences (3):** PRE 0ea98cc1 FAIL [docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md]; PRE 0ea98cc1 n/a [docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md]; PRE 0ea98cc1 FAIL [docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md]

### `PBX-VIS-005` Yellow labelled as GBA (platform label wrong)

- **Status:** Hardware Accepted · **Gen:** I · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE 50dac31f: PASS
- **Basis:** Owner PASS 'RBY ItemsFix: GB platform label' on 50dac31f (A0906).
- **Repair commits:** 50dac31f gen1: add read-only RBY inventory support (ItemsFix candidate)
- **Hardware acceptance:** 50dac31f PRE
- **Next action:** None.
- **Owner** (A0913): > Yellow identity detection not weakened; platform behaviour corrected later.
- **Occurrences (2):** PRE d9077e2d FAIL [docs/BUILD_RECORD.md]; PRE 50dac31f PASS [docs/BUILD_RECORD.md]

### `PBX-VIS-006` RSE game-card artwork missing

- **Status:** Hardware Accepted · **Gen:** III · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE (1c96df25 artifact): PASS
- **Basis:** Owner 'RSE artwork physical = PASS' (D0277) on corrected artifact; tests expect RSE card art (1c96df25).
- **Repair commits:** 1c96df25 tests: expect RSE game-card artwork
- **Automated tests:** tests/test_game_hub_contract.py : game hub card art contract
- **Hardware acceptance:** 1c96df25 PRE (issue #48)
- **Next action:** None.
- **Owner** (D0276): > game-card artwork: **FAIL — missing**
- **Occurrences (2):** PRE 46e0c161 FAIL [issue #6 (comment 2026-09-10)]; PRE  PASS [issue #6 (comment 2026-09-10)]

### `PBX-VIS-007` Gen I draft Pokémon has no live sprite preview while editing species

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** I · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE 69668bc8: PASS (DEVICE ACCEPTED)
- **Basis:** Gen I staged editor DEVICE ACCEPTED on 69668bc8 incl. 'generation-correct shiny presentation' (A0549, A0540). Species picker sprite code changed in R11 (eb7fe8c9 touches src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc and include/UI/SharedSpeciesPicker.h).
- **Repair commits:** 69668bc8 Gen I editor acceptance candidate; eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state
- **Automated tests:** tests/test_shared_species_picker_surface_contract.cpp : picker draws one sprite in current shiny state
- **Hardware acceptance:** 066e1db9 R12 H16 · R12 item H16
- **Next action:** Recheck Gen I species picker/draft sprite in R12 H16.
- **Owner** (A0524): > Show sprite as soon as draft species changes; draft-only, no staging.
- **Occurrences (2):** PRE 574d604b 3 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE 69668bc8 PASS [docs/GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md]

### `PBX-VIS-008` Gen II modal/action-sheet shells show full accent outline (Empty Slot, Pokémon Actions)

- **Status:** Hardware Accepted · **Gen:** II · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** PRE (issue #84 retest): PASS
- **Basis:** Owner retest 'works' (D0310, issue #84 comment 2026-09-28).
- **Hardware acceptance:** issue #84 candidate (PRE)
- **Next action:** None.
- **Owner** (D0308): > 1. Gen II still shows full theme-accent/yellow outlines on some modal/action-sheet shells at runtime:    - Empty Slot    - Pokemon Actions    These must match the quiet neutral outer-shell style used by Gen I / Gen III. The selected row may remain theme-accented, but the full modal border must not be.
- **Occurrences (2):** PRE  FAIL [issue #84 (body 2026-09-28)]; PRE  PASS [issue #84 (comment 2026-09-28)]

### `PBX-VIS-009` Packaged RomFS artwork completeness (Gen V game cards, region backgrounds)

- **Status:** Hardware Accepted · **Gen:** V, app · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** R3 62a2f8c3 AUD-10: PASS
- **Basis:** QA-F1 fixed 3b35b3f9 'guard the Gen V covers in the packaged-RomFS gate' (ancestor of 62a2f8c3). Owner R3 62a2f8c3 AUD-10 'Region backgrounds included' PASS (B0691).
- **Repair commits:** 3b35b3f9 fix(release): guard the Gen V covers in the packaged-RomFS gate (QA-F1)
- **Automated tests:** tests/test_qa_regressions_assets.py : QA-F1 Gen V covers in manifest; tests/test_verify_nro_romfs.py : RomFS paths present in NRO
- **Hardware acceptance:** R3 62a2f8c3 AUD-10
- **Next action:** None.
- **Owner** (B0605): > Verdict: Does not reproduce for Gen I-V; the packaged NRO needs CI; owning lane: —. 10. **RomFS.** The referenced regions are covered by the NRO gate (1268 paths). hisui and paldea (Switch-only titles) are fetched at release. `test_qa_regressions_assets.py` (QA-F1), `test_verify_nro_romfs.py` and `test_device_asset_png_validation.py` pass.
- **Occurrences (3):** PRE QA-F1 n/a [docs/qa/FINDINGS.md]; R3 0faf7e9c Part 9 #10 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-10 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-VIS-010` Compact, uniform menus/pickers across generations (less blank space)

- **Status:** Hardware Accepted · **Gen:** all · **Area:** visuals-sprites · **Category:** request
- **Last owner result:** R3 62a2f8c3 R2-MENUS: PASS
- **Basis:** Owner R3 62a2f8c3 R2-MENUS 'Compact selection windows' PASS (B0664).
- **Hardware acceptance:** R3 62a2f8c3 R2-MENUS
- **Next action:** None.
- **Owner** (B0054): > can we shrink box menu if they need it like in the bag menu can we have it so menu dont have so much blank space in the middle so we can easily see the item we wanna add and dont accidently add the item above or below it cause we cant see it properly cause they far away, now if a menu if change for one gen it has to ben constant for all gens, BUT one gen can have different sixed menus, like if to make them look good they have to all bedifferent sizes to fit all theyre stuff cool, but all gens must follow that exact setup, i want it to be uniform, so if the pokeball menu gets shrinked in gen 1 its would be the same size in the gen 5 editor, got it, for example the key item warning menu, all bag menus, dont make them super small but make them a nice size with no massive gaps with the name and than the pictures,sprites, the held item menu just needs a smidge the text and sprites closer and that should be used for like a example or some shit.
- **Occurrences (5):** PRE 574d604b 6 FAIL [docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md]; PRE CG13 PASS [docs/qa/GEN1_5_UI_AUDIT_MATRIX.md]; R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R3 62a2f8c3 R2-MENUS PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-VIS-011` Read-only fields (Gen I stats, derived HP DV) gold in both View and Edit

- **Status:** Hardware Accepted · **Gen:** I, II · **Area:** visuals-sprites · **Category:** request
- **Last owner result:** R11 3634d1e6 H21: PASS (R3 62a2f8c3 R2-GOLD: PASS)
- **Basis:** Owner R3 62a2f8c3 R2-GOLD 'Read-only field colours' PASS (B0666); R11 H21 Gen I DV/EV controls PASS (E0020). Gen II Special part of B0124 tracked in PBX-STA-002.
- **Automated tests:** tests/test_classic_stat_field_state.cpp : locked/derived stat cells use read-only style
- **Hardware acceptance:** R3 62a2f8c3 R2-GOLD
- **Next action:** None.
- **Owner** (B0056): > gen 1 has in the view menu screen the stats that are uneditable are gold and in the editor menun they are white, change we change so both are gold, also can we make the HP DV gold in both since we cant edit that either ?? or would that be weird.
- **Occurrences (3):** R1 7fea1797 n/a [docs/hardware/ROUND1_RESULTS_7fea1797.md]; R1 7fea1797 n/a [docs/hardware/OWNER_RETEST_7fea1797_20261017.md]; R3 62a2f8c3 R2-GOLD PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-VIS-012` Gen V species picker sprites (494-649) and shiny preview

- **Status:** Hardware Accepted (recheck in R12) · **Gen:** V · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** R3 62a2f8c3 AUD-02: PASS
- **Basis:** Owner R3 62a2f8c3 AUD-02 'Species sprites and restrictions' PASS (B0684). Picker reworked in R11 eb7fe8c9 (include/UI/SharedSpeciesPicker.h: one sprite in current shiny state).
- **Repair commits:** eb7fe8c9 SPRITE: species picker shows one sprite in the current shiny state
- **Automated tests:** tests/test_shared_species_picker_surface_contract.cpp : single sprite / shiny caption
- **Hardware acceptance:** 066e1db9 R12 H16 · R12 item H16
- **Next action:** Run R12 H16 on a Gen V save incl. a 494-649 species.
- **Owner** (B0597): > Verdict: Does not reproduce in source; sprites for 494-649 need a device check; owning lane: —. 2. **Species picker.** It is the shared picker, species 1-649, with a Y toggle for Normal/Shiny (`SharedSpeciesPicker.h:389-430`). Ineligible picks are refused after the choice (`Gen5SharedScreenState.h:399-419`), the same as Gen III/IV. `tools/check_device_assets.py` checks shiny sprites only for Gen I (lines 178-179), so the Gen V shiny art is unchecked.
- **Occurrences (1):** R3 0faf7e9c Part 9 #2 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]

### `PBX-VIS-013` Gen V item sprites missing (feathers 565-571, TM/HM disc art)

- **Status:** Hardware Accepted · **Gen:** V · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** R3 62a2f8c3 AUD-06: PASS
- **Basis:** Fixed c3b12d2b (ancestor of 62a2f8c3); owner R3 62a2f8c3 AUD-06 'Item sprites' PASS (B0687). CG25 enforced.
- **Repair commits:** c3b12d2b fix(items): Gen V TM/HM labels and disc art, Gen V feather sprites; one machine table (QA Part 9)
- **Automated tests:** tests/test_item_sprite_coverage.py : feather -> *-wing aliases (lines 181-182); tests/test_hw_round1_regression_gate.py : CG25-gen5-machine-and-feather-art
- **Hardware acceptance:** R3 62a2f8c3 AUD-06
- **Next action:** None.
- **Owner** (B0601): > Verdict: **REPRODUCES (Gen V only)**; owning lane: I. 6. **Item sprites.** `test_item_sprite_coverage.py` passes for Gen I-IV: 124/221/307/514 eligible, 0 missing. Gen V is not covered by that test. Findings for Gen V: - 7 feathers have no icon (ids 565-571). The pinned art exists as `*-wing`, so only the aliases are missing. - All 101 TMs/HMs have no icon. `MachineDisplay.cpp:32-112` has no BW/B2W2 case, and `ClassicMachineArtwork.h:109` returns type 0. - HM01-06 (ids 420-425) are labelled "TM93".."TM98", the BDSP names, so they read as duplicate TM93-98 (`ItemNames.cpp:429-434`). A host probe printed `game=BW id=420 label='TM93'`. **Fix:** add BW/B2W2 machine mapping (TM01-92 = 328-419, HM01-06 = 420-425, TM93-95 = 618-620) and the feather to wing aliases, and extend coverage to Gen V. Citation to verify against the pinned PKHeX item tables before landing.
- **Occurrences (2):** R3 0faf7e9c Part 9 #6 n/a [docs/qa/ROUND3_PART9_REPRO_0faf7e9c.md]; R3 62a2f8c3 AUD-06 PASS [tests/test_hw_round1_regression_gate.py]

### `PBX-VIS-014` Pokémon editor typography inconsistent across generations (headings)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** all · **Area:** visuals-sprites · **Category:** request
- **Last owner result:** R6 26186464 (other request): FAIL/request
- **Basis:** d88056cc 'one shared panel heading style' (R7, in e88957f2 and 3634d1e6, not in 26186464); include/UI/SharedPanelHeading.h used by Gen1/Gen2/Gen3/Shared surfaces; CG89 enforced. R7 C03 checklist never got owner results; R11 worksheet had no typography item (H15 smoke PASS is not item-specific).
- **Repair commits:** d88056cc R7 P2: one shared panel heading style for the Pokemon editor, Gen I-V; b9e8e517 CG89 heading misses through the shared helper
- **Automated tests:** tests/test_qa_cross_gen_contract.py : CG89-one-editor-heading-style
- **Hardware acceptance:** none (not on R12 checklist; R7 C03 never tested)
- **Next action:** Add an explicit typography check to R12 H18 smoke or next round (compare Gen I/II vs Gen IV/V headings).
- **Owner** (C0349): > **Pokémon editor typography is inconsistent.** Older-generation View/Edit screens use large white headings, while newer ones use smaller coloured section labels. Affected labels include:   - DETAILS   - STATS   - MOVES   - Origin & Legality   - supporting field labels    The shared Gen IV/V editor is the reference. Keep the green/red Nature colours.
- **Occurrences (2):** R6 26186464 n/a [docs/hardware/ROUND6_RESULTS_26186464.md]; R6 26186464 Other n/a [docs/hardware/ROUND6_RESULTS_26186464.md]

### `PBX-VIS-015` HOME card region artwork pokes past rounded corners (rectangular clip)

- **Status:** Fixed in Code — Awaiting Hardware · **Gen:** app · **Area:** visuals-sprites · **Category:** defect
- **Last owner result:** none
- **Basis:** c21d3747 masks scenery with card's rounded path (include/UI/RoundedArtwork.h, src/UI/SaveSelectScreen.cpp).
- **Repair commits:** c21d3747 HOME card: mask region scenery with the card's rounded corners (Round 11)
- **Automated tests:** tests/test_rounded_artwork.cpp : rounded path geometry; tests/test_home_card_scenery_contract.py : no rectangular clip; tests/test_ui_home_card_scenery_host.cpp : host render
- **Hardware acceptance:** 066e1db9 R12 H01 · R12 item H01
- **Next action:** Owner runs R12 H01 handheld+docked.
- **Owner** (E0122): > Preserve the HOME region-artwork rounded-corner clipping regression discovered by Sonnet.
- **Occurrences (1):** R11 3634d1e6 n/a [owner message 2026-11-01 (independent Round 11 QA)]

### `PBX-VLT-001` Persistent PokeBank-owned Master Vault and named Banks (entity store, stable IDs, SHA-256, provenance, journal/recovery, Bank references)

- **Status:** Missing Feature · **Gen:** all · **Area:** vault · **Category:** missing-feature · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H41/H42/H43: FAIL (not implemented)
- **Basis:** No Vault exists in shipped code: no Vault entity store / Vault ID / SHA-256 manifest anywhere under include/ or src/ (no Vault directory; intake VAULT inventory 'There is NO Vault ID, no SHA-256 entity store and no provenance log'). Product Home 'Master Vault' and 'Banks' are FutureBackend placeholders (include/UI/AppShellModel.h:41-43 'COMING SOON', :50-52 'NO BANKS YET'); opening Master Vault shows only 'Master Vault is coming in a future PokeBank NX update' (src/UI/AppShellScreen.cpp:91-101). The only persistent store is the inherited PKSE Legacy Storage bank.dat (include/Trainer/Bank.h:1-10), explicitly labelled 'app-owned / not Master Vault' (src/UI/Panels/HomeMenuPanel.cpp:119, TrainerViewScreenBase.inc:5603). Deferred out of the Gen I-V gate (FUTURE_AFTER_GEN15_BACKLOG #3/#7/#27, F0038) and out of the Round 11 repair NRO (F0028); still required for v1.0 (V1_ROADMAP, README, MASTER_VAULT_SPEC §22). Unchanged between 3634d1e6 and 066e1db9.
- **Automated tests:** none for a Vault (tests/test_bank_*.cpp cover Legacy Storage bank.dat only)
- **Hardware acceptance:** none (not on any checklist; needs a separate Vault build)
- **Next action:** Owner to answer intake VAULT Q6 (user-facing name; bank.dat migrate or stay legacy) and approve the separate Vault plan/branch; build per MASTER_VAULT_SPEC §22 minimum milestone with a physical read-only Vault test.
- **Occurrences (36):** R11 3634d1e6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §5 separate from the repa…]; REQ §1 Core invariants n/a [docs/MASTER_VAULT_SPEC.md]; REQ §3 On-disk layout n/a [docs/MASTER_VAULT_SPEC.md]; REQ §18 Transaction model n/a [docs/MASTER_VAULT_SPEC.md]; REQ §20 Schema migration n/a [docs/MASTER_VAULT_SPEC.md]; REQ §14 Duplicate detection n/a [docs/MASTER_VAULT_SPEC.md]; REQ §15 Banks n/a [docs/MASTER_VAULT_SPEC.md]; REQ §21 Performance targets n/a [docs/MASTER_VAULT_SPEC.md]; REQ §22 Minimum Vault v1 milestone n/a [docs/MASTER_VAULT_SPEC.md]; REQ §23 Non-goals n/a [docs/MASTER_VAULT_SPEC.md]; REQ Add to Master Vault n/a [docs/UI_FLOW.md]; REQ Add to Bank… n/a [docs/UI_FLOW.md]; REQ Vault Home / Master Vault browsing / Banks n/a [docs/UI_FLOW.md]; REQ §9 Clone semantics n/a [docs/MASTER_VAULT_SPEC.md]; REQ Add to Master Vault n/a [docs/TRANSFER_MODEL.md]; REQ Stable v1.0 release gates (Vault) n/a [docs/RELEASE_CHECKLIST.md]; REQ Beta / RC gates n/a [docs/RELEASE_CHECKLIST.md]; REQ Core invariant / MOVE n/a [docs/POKEMON_IDENTITY_CUSTODY_CONTRACT.md]; REQ v1 target n/a [docs/V1_ROADMAP.md]; REQ Progressive release contract n/a [docs/V1_ROADMAP.md]; REQ Phase 6 n/a [docs/V1_ROADMAP.md]; REQ Critical path: Future product layers n/a [docs/V1_ROADMAP.md]; REQ Milestone progress: Master Vault, Banks, Search n/a [PROJECT_STATUS.md]; REQ #3 / #7 / #27 n/a [docs/FUTURE_AFTER_GEN15_BACKLOG.md]; REQ v1.0 target philosophy n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]; REQ #1 / #17 n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]; REQ Project status: Vault/Banks, v1.0 n/a [README.md]; REQ §5 / VAULT n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ VAULT gap 1-2 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ VAULT gap 3-5 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ VAULT Q6 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ §19 Crash recovery n/a [docs/MASTER_VAULT_SPEC.md]; REQ §1 invariants 10-11 n/a [docs/MASTER_VAULT_SPEC.md]; REQ Required provenance event classes n/a [docs/POKEMON_IDENTITY_CUSTODY_CONTRACT.md]; REQ §5 Raw entity payload n/a [docs/MASTER_VAULT_SPEC.md]; REQ #57 n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]

### `PBX-VLT-002` Search in Bank/Vault with highlight of matching Pokemon/item (PKSE-style) before 1.0

- **Status:** Missing Feature · **Gen:** all · **Area:** vault · **Category:** missing-feature · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 Extra 8: request (no result)
- **Basis:** No search/highlight in src/UI/Panels/StoragePanel.cpp or Trainer::Bank; Product Home 'Search' is FutureBackend 'COMING SOON' (include/UI/AppShellModel.h:53-55). Intake notes the PKSE search the owner remembers is not in this repo's PKSE history. v1.0 requirement also stated in search-area REQ records F0062-F0065 ([MANDATORY 4]).
- **Hardware acceptance:** none (not on any checklist)
- **Next action:** Owner to give the spec (intake VAULT: name/species/item index, highlight); schedule with the Vault index (PBX-VLT-001) or as a Legacy Storage search if wanted earlier.
- **Owner** (E0059): > pkse has a nice search function in the bank where it highlights the pokemon with the searched letter or the item  could we add just before 1.0.
- **Owner** (F0062): > pkse has a nice search function in the bank where it highlights the pokemon with the searched letter or the item  could we add just before 1.0.
- **Occurrences (8):** R11 3634d1e6 Extra 8 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md (Extra bugs and requests)]; R11 3634d1e6 Extra 8 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ Extra 8 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ VAULT gap 8 / Q5 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ v1 target / Phase 10 n/a [docs/V1_ROADMAP.md]; REQ #47 Smart Banks / advanced search n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]; REQ §10 n/a [docs/PRODUCT_DECISIONS.md]; REQ Android companion n/a [docs/V2_ROADMAP.md]

### `PBX-XFR-001` Gen I-V cross-generation transfer workflow (e.g. Gen I -> Gen V; 'through bank?') with explicit route rules, conversion preflight, provenance and destination-first Move

- **Status:** Missing Feature · **Gen:** all · **Area:** transfer · **Category:** missing-feature · ⚠️ safety-critical · 🎯 v1.0 required
- **Last owner result:** R11 3634d1e6 H38: PASS (import refusal) with question 'how are we suppose to move a gen 1 pokemon to like gen 5'
- **Basis:** No converter for Gen I, II, IV or V: Conversion::convert covers only FRLG <-> GG/SWSH/BDSP/PLA/SV/ZA (include/Conversion/Convert.h:1-33, Result::Unsupported :46-48; intake VAULT inventory 8). Standalone file import refuses other generations ('Only a .pk2 file can go into a Generation II save; Pokemon are not converted between generations', src/Integration/Gen2/Gen2StagedEditor.cpp ~1529), which the owner PASSed as H38. No Transfer to Game action in any Gen I-V sheet (PBX-ACT-002). Rule table (official routes; Gen I->V refused or marked non-canonical) is an open owner decision (intake Q4). Fidelity/RouteEvidence exist only for the FRLG/Switch converter (include/Conversion/Fidelity.h, RouteEvidence.h). Transfer issues were closed as deferred for the Gen I-V gate (F0059) but README/V1_ROADMAP keep them v1.0.
- **Automated tests:** tests/test_conversion_entity_golden.cpp / test_conversion_fidelity_golden.cpp / test_conversion_evidence.cpp : FRLG <-> Switch conversion only
- **Hardware acceptance:** none (future Vault/transfer build)
- **Next action:** Answer the owner's H38 question in plain words (no route exists today; official routes I<->II, III->IV, IV->V); get owner decision on intake Q4; plan the Transfer Lab after the Vault.
- **Owner** (E0051): > how are we suppose to move a gen 1 pokemon  to like gen 5 ?? does it have to go through bank first or how we doing that ??
- **Occurrences (20):** R11 3634d1e6 H38 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; R11 3634d1e6 H38 PASS [docs/hardware/ROUND11_ENGINEERING_INTAKE.md §1 finding table]; REQ Transfer to Game… n/a [docs/UI_FLOW.md]; REQ §12 Conversion semantics n/a [docs/MASTER_VAULT_SPEC.md]; REQ Product goal n/a [docs/TRANSFER_MODEL.md]; REQ Generation conversion n/a [docs/TRANSFER_MODEL.md]; REQ Loss/change reporting; Destination compatibility n/a [docs/TRANSFER_MODEL.md]; REQ True Move: Game -> Vault / Vault -> Game / Game -> Game n/a [docs/TRANSFER_MODEL.md]; REQ Minimum pre-live-write transfer milestone n/a [docs/TRANSFER_MODEL.md]; REQ Transfer journal n/a [docs/TRANSFER_MODEL.md]; REQ HOME Bridge n/a [docs/TRANSFER_MODEL.md]; REQ  n/a [docs/HOME_BRIDGE_HISTORICAL_TRANSFER_RESEARCH_2026-09-03.md]; REQ CONVERSION IS NOT CLONING n/a [docs/POKEMON_IDENTITY_CUSTODY_CONTRACT.md]; REQ Phase 11 n/a [docs/V1_ROADMAP.md]; REQ #10 / #19 / #41 n/a [docs/FEATURE_FEASIBILITY_MATRIX.md]; REQ #10 / #20 / #40 / #47 n/a [docs/FUTURE_AFTER_GEN15_BACKLOG.md]; REQ Roadmap step 6 / guardrails n/a [README.md]; REQ H38 n/a [docs/hardware/ROUND11_OWNER_WORKSHEET.md]; REQ VAULT gap 9 / Q4 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]; REQ VAULT gap 10 n/a [docs/hardware/ROUND11_ENGINEERING_INTAKE.md]

### `PBX-XFR-002` Cross-game True Move locked (installed saves read-only; COPY/import/staged only until per-adapter safety gates pass)

- **Status:** Intentionally Unsupported · **Gen:** all · **Area:** transfer · **Category:** decision · ⚠️ safety-critical
- **Last owner result:** none
- **Basis:** Recorded permanent safety policy: docs/GAME_SUPPORT_MATRIX.md 'cross-game True Move LOCKED; source injection LOCKED' (F0061), docs/TRANSFER_MODEL.md current-alpha rule and enablement gate (F0046/F0051, tracking #20), README guardrail 'Cross-game True Move is not broadly enabled' (F0060). Enforced in code: a cross-store Move into the save pane of a different game group is refused 'Cross-game true Move is locked until conversion preservation is proven.' (src/UI/TrainerViewScreenBase.inc:879-881); every classic Gen I-V source is legacy read-only (SourceMutationPolicy canPerform). The eventual v1.0 true-Move requirement is tracked under PBX-XFR-001.
- **Repair commits:** 0ea98cc1 safety: lock installed-source UI and harden PLA reads
- **Automated tests:** tests/test_storage_conversion_custody_contract.cpp : cross-game true Move refused; tests/test_source_mutation_policy.cpp : legacy/installed sources cannot mutate
- **Hardware acceptance:** none (policy; not a checklist item)
- **Next action:** Keep locked; revisit only per adapter after Vault v1 and write/readback verification (TRANSFER_MODEL enablement gate).
- **Occurrences (3):** REQ Current alpha rule n/a [docs/TRANSFER_MODEL.md]; REQ True Move enablement gate n/a [docs/TRANSFER_MODEL.md]; REQ Permanent safety policy n/a [docs/GAME_SUPPORT_MATRIX.md]
