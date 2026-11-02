# PokeBank NX — Gen I–V Complete Audit and Switch Acceptance Gate

**Owner directive (October 15, 2026):** Finish Gen V on the existing DraStic/melonDS/RetroArch workflows; audit and finalize the **single existing shared Gen I–V editor**; then execute a full Switch hardware torture test before doing ANY active Gen VI/new-emulator application integration. This gates application/release work, not preservation of separate research branches.

> This checklist is a **starting coverage matrix**. It is NOT a hardware pass and NOT proof that every UI action has already been identified. An independent source-code inspection must enumerate every actual reachable handler, input and modal, add missing rows, and attach NRO + test evidence. Do not sign off with sampled smoke tests alone.

## Mandatory milestone order

- [ ] A. Claude completes Gen V on active [integration PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307); close actual defects.
- [ ] B. Full Gen I–V shared-editor audit; every reachable UI action/controller/touch handler mapped to test case ID; same modal/menus across generations where applicable.
- [ ] C. Source-safe exports, save assignments and app architecture independently reviewed; per-generation native formats verified.
- [ ] D. One exact combined NRO: GCC/Clang host, sanitizers, native build, focused generation regression, fuzz, verified-export, embedded RomFS, source safety **all green on same SHA**.
- [ ] E. Every action below executed on a real Switch: handheld/touch, docked/controller, every supported provider and profile state, every game where applicable.
- [ ] F. Hardware failures fixed on forward-only branch; publish new NRO, rerun impacted + full regression matrix, and repeat hardware until all rows PASS or documented genuinely N/A.
- [ ] G. **Owner explicitly accepts** single exact NRO SHA-256. Only **after G** may Gen VI emulator/3DS integration begin.

**No early Gen VI active application work.** Preserve existing [Gen VI #309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309), [Gen VII #311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311), and [Gen VIII #312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) research branches as draft, unmerged and outside the release target.

## Game coverage (ALL 20, not one representative per generation)

| Gen | Releases (each independently smoke-tested) | Distinctive formats |
|---|---|---|
| I | Red, Blue, Yellow | RBY DVs, Stat Exp, no native Ability/Nature |
| II | Gold, Silver, Crystal | GSC gender/shiny/DVs/Pokérus |
| III | Ruby, Sapphire, Emerald, FireRed, LeafGreen | GBA save-sector checksums, game items, met details |
| IV | Diamond, Pearl, Platinum, HeartGold, SoulSilver | DS PK4, ball, party/boxes, DraStic/melonDS |
| V | Black, White, Black 2, White 2 | BW/B2W2 PK5, backup copies, Trainer/Bag/Zukan |

For each test row: record **PASS / FAIL / N/A**, applicable game(s), emulator/provider, active profile, input (controller or touch), expected/actual, error screenshot/log and exact NRO SHA. A supported action cannot be marked N/A because it failed. For save mutation tests, work on disposable emulator copies with an independent backup and a precomputed original-file hash.

## Every-action acceptance checklist

### A. BOOT & HOME (12 checkpoints)

- [ ] **A01** — Cold start from Hbmenu; app version, splash/logo, loading and no hang
- [ ] **A02** — Quit & Return/launcher; exit/back from each entry point; app state preserved
- [ ] **A03** — Foreground/resume, sleep/wake, reload game/profile and restart NRO
- [ ] **A04** — HOME card focus across all 20 games; no wrong-game return or freeze
- [ ] **A05** — All game covers, trainer portraits, shiny sprites, level/name, Party and Dex card counts
- [ ] **A06** — All 10 regional scenes and RomFS assets in actual NRO, not merely checkout
- [ ] **A07** — Male/female/neutral portrait and uninitialized trainer, never guessed gender
- [ ] **A08** — Switch user/profile selector and two offline/local profiles
- [ ] **A09** — Settings gear, About, diagnostics and every actual setting row
- [ ] **A10** — Light/dark/Poké Classic/theme variants, persistence after restart and contrast
- [ ] **A11** — Bottom Games/Items/Banks/Search/More icons; disabled placeholders honestly gray
- [ ] **A12** — D-pad/left stick/touch navigation, focus on screens with no active save

### B. GAMES & SAVE INSTANCES (22 checkpoints)

- [ ] **B01** — Open Games classic grid (A), Quick Games (Y); close B; Y never silently closes
- [ ] **B02** — Quick Games finger drag, scroll, momentum/reversal, tap correct tile, focus recovery
- [ ] **B03** — Source/Game File X; list provider name, actual emulator identity, copy and timestamp
- [ ] **B04** — Assign Save to THIS profile without launching; manual path link separately
- [ ] **B05** — Forget/unassign save with confirmation; B and A on confirmation inert
- [ ] **B06** — Switch profiles; first profile remains private, no accidental cross-claim
- [ ] **B07** — Restart app: same save remembered after another emulator in-game save
- [ ] **B08** — New profile assign, reassign, relink and conflicting existing profile binding
- [ ] **B09** — All Gen I–V correct ROM header identity; wrong game/manual link rejected
- [ ] **B10** — Gen I–III RetroArch plus mGBA/Tico bounded roots when actually applicable
- [ ] **B11** — Gen IV/V existing DraStic/melonDS raw/.dsv paths: no manual moving
- [ ] **B12** — RetroArch huge folder cannot starve DraStic/melonDS discovery budget
- [ ] **B13** — Same path varied case/slashes, duplicate physical aliases: no two profile owners
- [ ] **B14** — Invalid/truncated/wrong game, unreadable root and exhausted scan budget messages
- [ ] **B15** — Moved/missing/replaced save: never silently adopt another source
- [ ] **B16** — Gen V primary vs backup same/different, ambiguous, damaged and wrong title
- [ ] **B17** — Gen V real new-game save: Party 0/Boxed 0, neutral trainer and explicit notice
- [ ] **B18** — Gen V + Save Diagnostics: no trainer names/IDs/raw contents disclosed
- [ ] **B19** — Manual chooser: path casing/dot/parent/traversal rejection and B cancellation
- [ ] **B20** — App launch through RetroArch and DraStic/melonDS, correct game and error handling
- [ ] **B21** — Known RetroArch exit crash: reproduce/fix/retest Gen I–III
- [ ] **B22** — Known Gen IV/V Quit to Launcher DraStic handoff: reproduce/fix/retest

### C. EDITOR HUB & ACTION MENU (19 checkpoints)

- [ ] **C01** — Open shared game hub, never Gen V direct-to-Party regression
- [ ] **C02** — Party, Boxes, Trainer, Items, Storage availability and B Back routing
- [ ] **C03** — Party slot states 0–6, native HP/current stat vs PC summary
- [ ] **C04** — Box browse 1..native max; wrap, page, scroll; selected slot remembered
- [ ] **C05** — Occupied slot A opens correct narrow Action menu; empty slot A offers Add
- [ ] **C06** — Disabled View/Edit/Review/Legal/Provenance/Storage actions genuinely disabled
- [ ] **C07** — Every Action: View, Edit, Create/Add, Clone, Move, Remove/Release, Cancel
- [ ] **C08** — Separate confirmation for Remove/Release/Move/Discard; A not destructive
- [ ] **C09** — Gen IV Y Pickup and slot placement; B cancels and source remains
- [ ] **C10** — Gen V encrypted blank retail PK5 slot accepted for Create/Add
- [ ] **C11** — Gen V Create opens SAME three-panel draft; no old 30-prompt wizard
- [ ] **C12** — New-game Gen V uninitialized trainer: Create/Trainer edit safely refused
- [ ] **C13** — Summary and stat graph, ability/gender/native generation field display
- [ ] **C14** — Native species picker: Dex numbers, sprites/shiny previews, correct roster
- [ ] **C15** — Moves picker: species/generation availability, accuracy/power/PP/PP Ups
- [ ] **C16** — Met level inline with location, met date, egg date and egg flag display
- [ ] **C17** — Held item picker, compact nature picker, field/keypad focus restoration
- [ ] **C18** — D-pad/left stick cross-panel parity, L/R panel, stick edge navigation
- [ ] **C19** — Touch single/double tap, drag, press, scroll, back, tapped disabled row inert

### D. POKÉMON EDIT / CREATE (16 checkpoints)

- [ ] **D01** — Create from every applicable empty Party/Box slot with correct native blank
- [ ] **D02** — Species change and supported form/gender/shiny toggle only when native
- [ ] **D03** — Nickname text, clear, invalid chars, max length and cancel
- [ ] **D04** — Level/EXP keypad, derived stats, overflow/underflow/partial input
- [ ] **D05** — Nature/ability/gender/editable vs read-only per native generation
- [ ] **D06** — Gen I DVs/Stat Exp and HP DV derivation; no invented Nature/Ability
- [ ] **D07** — Gen II DVs, gender/shiny rule, Pokerus None/Cured/Infected
- [ ] **D08** — Gen III/IV/Gen V IV/EV edit, range and total guards, derived stats
- [ ] **D09** — Moves 1–4, move chooser, delete/replace, native PP and PP Ups
- [ ] **D10** — Held Item, Met, OT/ID, origin, capture Ball, ribbons/metadata where applicable
- [ ] **D11** — Edit Keep/Add vs Discard/Cancel with original unmodified
- [ ] **D12** — Clone to empty slot, full Party/Box failure, no dupes on cancel
- [ ] **D13** — Move Pokemon from occupied Box slot to empty, atomic staged exact copy/remove
- [ ] **D14** — Release/Remove with safe Y confirmation, no original-source write
- [ ] **D15** — Rapid B/plus/minus/triggers, focus and selection state stable after picker
- [ ] **D16** — Legal/provenance display only if applicable, no invented certainty

### E. TRAINER, BAG, DEX (10 checkpoints)

- [ ] **E01** — Trainer view native Name, TID/SID, gender, badges, version, language
- [ ] **E02** — Gen V editable Name/Money/Play Time with range and staging; readonly IDs
- [ ] **E03** — Trainer unknown/new-game shown neutral not male or unsupported glyphs
- [ ] **E04** — Product Home Items vs Trainer Y Bag same source selector and return
- [ ] **E05** — Every native Bag pocket and header L/R arrows, empty/full inventory
- [ ] **E06** — Quantity A/keypad and input bounds; X Add; Y twice Remove
- [ ] **E07** — Key Items/TMs-HMs view-only in Gen V; invalid item IDs refused
- [ ] **E08** — Gen III/IV Bag changes and Pokémon edits in same session: no lost edits
- [ ] **E09** — Gen V Bag: Items/Key Items/TMs-HMs/Medicine/Berries, native IDs and qty
- [ ] **E10** — Pokédex seen/caught, Gen V Zukan readonly; no unsupported editor

### F. REVIEW, EXPORT & DATA SAFETY (15 checkpoints)

- [ ] **F01** — + Review shows all and only staged Pokémon, Trainer and Bag changes
- [ ] **F02** — Review empty state appropriately grey; no fake pending provenance
- [ ] **F03** — Y then Y Discard All; A inert, B cancels and retains edits
- [ ] **F04** — Exit unsaved via B and spam B: confirmation, Keep/Cancel/Discard correct
- [ ] **F05** — X Export Copy backed by app-owned safe directory, no overwrite
- [ ] **F06** — Export manifest identifies game/copy, trainer/bag changes, source provenance
- [ ] **F07** — Original backup and edited save both present, readable, strict-reparse
- [ ] **F08** — Open edited export in corresponding emulator and verify field/Bag/Trainer effects
- [ ] **F09** — Wrong game/copy source, changed fingerprint: block stale export
- [ ] **F10** — Faults: no space, read error, permission denied, existing folder, power loss
- [ ] **F11** — All exports whitelist expected byte ranges and repair all native checksums
- [ ] **F12** — No direct original emulator/installed game writes, Inject Save remains gated
- [ ] **F13** — Pre/post original file SHA256 unchanged after edit/export/crash/restart
- [ ] **F14** — Repeat export produces fresh unique name, including Gen II race-safe publish
- [ ] **F15** — Normal/sanitizer/fuzz/CI/nro asset audit evidence links exact code SHA

### G. WHOLE APP / REGRESSION (12 checkpoints)

- [ ] **G01** — Handheld/touch vs docked/controller for ALL screen/modal/confirmation paths
- [ ] **G02** — Every reachable button A/B/X/Y/+/-/L/R/ZL/ZR and both sticks per context
- [ ] **G03** — Every actual touch target tap/double/long/drag/scroll, no accidental mutations
- [ ] **G04** — Full/empty list boundaries, scroll memory, reopened modal, screen overlays
- [ ] **G05** — All 20 titles smoke of assigned source/trainer/party/boxes/Items/return
- [ ] **G06** — Every actually enabled action per game (not only one per generation)
- [ ] **G07** — Multiple providers, multiple local profiles and missing provider scenarios
- [ ] **G08** — Hundreds of repeated open/edit/cancel cycles; no freeze/crash/leaks
- [ ] **G09** — No regression from Gen V into the owner-accepted Gen I–IV workflows
- [ ] **G10** — Independent line-by-line action inventory reconciled to every reachable code handler
- [ ] **G11** — Every failure retested on NEW same-head, full CI-green NRO
- [ ] **G12** — Owner explicitly signs exact NRO SHA-256 after every row PASS/N/A

## Previously observed Gen V hardware defects — required explicit retest

- [ ] Previous Gen I–V candidate `fa9cfcea` failed on Switch: Gen V opened directly into Party rather than the shared hub.
- [ ] Gen V empty encrypted retail PK5 slots were not recognized and Create showed only Cancel; native Create now follows the three-panel shared draft instead of a separate 30-prompt wizard.
- [ ] The owner-provided Black 2 DraStic file was a checksum-valid newly initialized save (trainer/Party empty). App must report **new game/no trainer**, not display fake male portrait, unsupported glyphs or manufactured progression; actual progressed save source discovery needs separate proof.
- [ ] Gen V Trainer no longer opens a separate generation-only popup; uses the shared hub/content area.
- [ ] Verify all 4 Gen V covers, Unova backdrop, trainer-gender portraits, Bag items and full-box sprites in **actual NRO**.
- [ ] Existing emulator source paths do not require the owner to move files. Both raw and footer-wrapped saves must be treated according to their native formats.
- [ ] Known Gen I–III RetroArch exit crash and Gen IV/V DraStic launcher-return issues get confirmed fix or explicitly blocking repro.

## Known QA findings requiring independent disposition

- [ ] **QA-F2:** three malformed historic `one-time-*` workflows flood Actions with irrelevant failures; review and remove old write-capable jobs from active integration **after checking no current workflow depends on them**.
- [ ] **QA-F3b:** Gen II exporter uses nonexclusive `rename()` publish; race-safe unique output proof and regression case required.
- [x] **QA-F1:** four Gen V covers added to packaged RomFS audit on Gen I–V integration.
- [x] **QA-F3:** Common/Gen V exporter publication race corrected in development candidate; revalidate after final integration.

## Exact-device artifact record (fill only with verified evidence)

| Required field | Value |
|---|---|
| Final application source commit (40 chars) | **PENDING** |
| Final tree SHA | **PENDING** |
| NRO filename / byte count | **PENDING** |
| SHA-256 of tested NRO | **PENDING** |
| CI links (GCC / Clang / ASan / UBSan / native / app QA / RomFS) | **PENDING** |
| Switch mode / firmware / emulator versions | **PENDING** |
| All 20 releases tested | **NO** |
| All reachable app actions mapped to checklist | **NO** |
| Total PASS / FAIL / genuinely N/A | **PENDING** |
| **Owner hardware acceptance** | **NO** |

## Governance and branch/PR cleanup

- **Active Gen I–V code owner:** Claude on [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307). Do not touch this branch while Claude is changing it, reset/rebase backwards, force-push, merge prematurely or delete any unique unmerged work.
- Retired duplicate PR tracking may be closed if its precise HEAD is proven an ancestor of #307 and that ancestry is recorded in a closure comment. Preserve source branches and original commit references until an explicit archival plan is approved.
- The 3DS/emulator split is a **real product boundary**: Gen VI begins only once all above gates have owner approval. Backend-only research for VI–VIII is not current application support.
- GitHub draft/CI-ready is not physical-device acceptance. Keep original emulator saves immutable; only separately confirmed app-owned exports may be written.

Related: [Gen V development handoff](https://github.com/GlitchedZeus/PokeBank-NX/blob/claude/gen5-integration-20261014/docs/handoffs/GEN5_9_PROGRESS.md) · [Gen I–IV hardware checklist](https://github.com/GlitchedZeus/PokeBank-NX/blob/claude/gen14-final-20261014/docs/handoffs/GEN14_HARDWARE_TEST_CHECKLIST.md) · [Independent QA findings](https://github.com/GlitchedZeus/PokeBank-NX/blob/audit/app-qa-20261014/docs/qa/FINDINGS.md).
