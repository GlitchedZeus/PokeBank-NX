# Round 4 application repairs and device retest record

**Host-tested Round 4 candidate — NOT DEVICE TESTED.**

Candidate application source: `c60d4da0abc467aedac54aa886179c89fc89bc19`.
Application: [draft PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307).
Backup `44fb4b7eaa58f3e627ff981d84f2caeaed293667` is merged forward, retaining both parents.
The original application baseline `58a81207` is preserved. Legality PR #314 and main are untouched.

Owner hardware baseline stays **70 PASS / 37 FAIL / 3 NOT TESTED**. Every repair below needs the owner to retest the final audited NRO. S06 remains NOT TESTED; no live emulator save was overwritten.

## Owner hardware retest — October 23, 2026: Backup UI FAIL

**Candidate:** audited `c60d4da0` NRO. Owner-provided Switch screenshots confirm manual backup **creation succeeds** for emulator saves, including a timestamped app-owned copy. **R2-EMUBACKUP remains FAIL on real hardware**: emulator Backups uses a hybrid overlay instead of the full-screen PKSE backup picker; pressing A on created backups shows Details instead of opening the selected copy. Native Switch backups use the full-screen picker but show opaque `Edit backup workspace` / `LEGACY UNSCOPED / OWNERSHIP UNKNOWN` labels that obscure date order, and original historical PKSE backups cannot be opened. Track and fix all variants in **[#322](https://github.com/GlitchedZeus/PokeBank-NX/issues/322)**.

Do **not** promote R2-EMUBACKUP to PASS based on the previous host test; the 37/37 code-implemented count is a *prior candidate engineering statement*, not 37 hardware passes. C07 backup deletion and S05 original immutability remain **hardware unverified** until explicitly tested. Keep all previous owner PASS records, and S06 stays NOT TESTED. Historical unscoped backups must stay immutable and undeletable; safe validated browsing must not silently assign profile ownership or overwrite originals.

## Owner hardware retest — October 23, 2026: shared Review and Trainer Info FAIL

**Candidate:** audited `c60d4da0` NRO. Owner reports that each generation still has its own Review presentation and **Gen V Review cannot be reached in the tested UI path**. Also requests Trainer Info parity: native **Badges earned, Gender** visible in Gen I–IV where stored, and **editable Play Time and supported Trainer Gender** via staged, reviewed, export-only changes. The existing Gen V Play Time field is editable but Trainer Gender is currently read-only; Gen I / Gold / Silver have no freely editable player-gender field. Full source/format-specific acceptance and tests: **[#323](https://github.com/GlitchedZeus/PokeBank-NX/issues/323)**.

**Hardware status:** shared Review parity `AUD-01` **FAIL**; Gen V Review accessibility **FAIL**; expanded Trainer Info feature **NOT IMPLEMENTED/UNVERIFIED**. Prior host tests do not overrule owner's physical test. Preserve all other records and source immutability.

## Owner hardware retest — October 23, 2026: Gen II Special column / Gen IV stat-color FAIL

On the audited `c60d4da0` NRO: **Gen II (Gold/Silver/Crystal) Special Calculated cell was left blank** and SpA/SpD rendered detached below the grid, visually awkward. Owner requests SpA back in the Calculated column, SpD aligned immediately under it, while keeping one true Special DV and Stat Exp. **Gen IV View/Edit STATS labels are arbitrarily green/red according to nature**; user wants neutral stat labels there and colored stat effects only in the Nature picker (Move type/Ability colors retained). Track **[#324](https://github.com/GlitchedZeus/PokeBank-NX/issues/324)**.

Narrow source repair: Gen II helper + both VALUES painters now put `SpA <number>` in the Special Calculated cell and SpD on a second aligned Calculated-column line, without changing native DV/Stat Exp math. Shared STATS painter now uses neutral/selected theme text rather than `NatureStats::statColor`; original Nature picker retains +10%/−10% red/green. Updated Gen II numerical/layout contract and added neutral-label test. **Host CI and owner hardware retest still required; do not mark PASS on current NRO.**

## Owner hardware retest — October 23, 2026: Gen II Unown chooser sprites

The actual Unown form in Gen II derives from its four DVs; legitimate shiny Unown forms are I/V. The editor already renders a DV-derived letter, but the Species chooser drew form A for every Unown because it did not accept a form argument. The chooser now receives the exact Unown form computed on a **copy** of the current editor session using the same species+shiny routine as selecting A. Preview thumbnails and large artwork match the DV-derived form, and the title identifies the letter. Source DVs are unchanged. Unit/source regressions added to existing `test_gen2_unown_letter.cpp`. **Needs CI and new NRO physical retest; current c60d4da0 NRO still shows this bug.**

## Original 37 failures

All 37 entries below are implemented and host-tested in the validated application snapshot. The campaign preserves the earlier 12 app58 repairs. Hardware status for every row is **pending**. Exact-head CI and the independent binary audit are recorded in the PR checks linked below.

| ID | Repair / evidence | Current status |
| --- | --- | --- |
| E03 | One shared Origin panel; real stored facts; Not checked / Incomplete stay distinct from a clean verdict. (`20955b20`) | Host-tested; hardware pending |
| L06 | A and completed touch open one Party Actions popup; native staged Party Edit I–III. (`c81853da`) | Host-tested; hardware pending |
| R2-SPECIAL | One native Special DV; separate calculated Sp. Atk and Sp. Def labels. (`88f974da`) | Host-tested; hardware pending |
| R2-TRAINER | Trainer Review and four-choice staged exit; native name/money transactions. (`29084707`) | Host-tested; hardware pending |
| R2-EMUBACKUP | Explicit source/backups browser; deliberate backup creation; newest first; source/path classification. (`fe5c3ad7`) | Host-tested; hardware pending |
| R2-REROLL | DV/shiny/Unown rules checked; all 65,536 Gen II DV combinations tested; actual portrait receives derived form. (`88f974da`) | Host-tested; hardware pending |
| R2-REVIEWMOVE | Named moves/items and visible semantic differences; PC Move stays one grouped action. (`29084707 / e061f03c / c81853da`) | Host-tested; hardware pending |
| C01 | Gen V Home A directly opens the assigned source; chooser remains available for changing sources. (`app58 retained`) | Host-tested; hardware pending |
| C07 | Eligible app-owned backup deletion has explicit Y confirmation; originals and last-copy protection retained. (`fe5c3ad7`) | Host-tested; hardware pending |
| M04 | Move Category has its own detail line and a type-badge gutter. (`88f974da`) | Host-tested; hardware pending |
| M05 | Same shared Gen IV/V Category layout. (`88f974da`) | Host-tested; hardware pending |
| B08 | Real separate Gen I–V exports produced, verified and reparsed; result/path/errors visible; source bytes/size/mtime unchanged. (`app58 retained + export verification`) | Host-tested; hardware pending |
| E02 | Shared Party View / Edit / Origin & Legality / Cancel; boxed actions and PC Move retained. (`c81853da`) | Host-tested; hardware pending |
| E05 | Glyph choices in Bag dialogs; redundant Help footer removed; Release and Backup Delete stay Y-only. (`88f974da`) | Host-tested; hardware pending |
| S05 | Source read-only browsing, isolated working copies, recovery rollback, immutable originals and separate exports tested. (`fe5c3ad7`) | Host-tested; hardware pending |
| R2-BUG-002 | Gen III/IV Trainer Review is reachable; exit supports Review/Continue/Discard/Export Copy. (`29084707`) | Host-tested; hardware pending |
| R2-CHECK-011 | Default is stored plus species/ancestor wild context; X exposes other native histories as unverified. (`c9a457a3`) | Host-tested; hardware pending |
| NEW-02 | Obsolete Touch:Back text removed while touch Back remains. (`88f974da`) | Host-tested; hardware pending |
| NEW-03 | Explicit Bag Remove uses A/B glyph choices; B cancels first; no duplicate footer. (`88f974da`) | Host-tested; hardware pending |
| NEW-04 | Gen II packed Move uses display-only repaired bytes while carrying; final export still requires placement/cancel. (`app58 retained`) | Host-tested; hardware pending |
| NEW-05 | Species keyboard/list layout retained and regression-guarded. (`app58 retained`) | Host-tested; hardware pending |
| NEW-06 | Live Held Item search callback retained; real callback source path and broken-path guard verified. (`app58 retained / 20955b20`) | Host-tested; hardware pending |
| NEW-07 | Inline native Crystal OT Gender cycle retained and guarded. (`app58 retained / 20955b20`) | Host-tested; hardware pending |
| NEW-08 | Ball/list row, clip and touch widths reserve the full scrollbar touch slop plus gutter. (`88f974da / 20955b20`) | Host-tested; hardware pending |
| NEW-09 | All 25 nature effects; five neutral; green +10% / red −10%; HP never altered by nature. (`88f974da`) | Host-tested; hardware pending |
| NEW-10 | Compact native Met Location menus; bottom padding matches the Move menu. (`app58 retained / 20955b20`) | Host-tested; hardware pending |
| NEW-11 | Selected species name first, including missed Details fields; list numbering retained. (`app58 retained / 20955b20`) | Host-tested; hardware pending |
| NEW-12 | Pinned nine-title wild context and generation-specific ancestry; stored eggs/events/trades/transfers preserved as unverified histories. (`c9a457a3`) | Host-tested; hardware pending |
| NEW-13 | Ability names/effects retained; unavailable data no longer becomes a fake Slot label; native slot restrictions retained. (`app58 retained / 20955b20`) | Host-tested; hardware pending |
| NEW-14 | Native PK5 Language byte 0x17; IDs 1–5,7,8; Keep/Review/checksum/encryption/reparse/export; OT/nickname bytes unchanged. (`88f974da / 629d6365`) | Host-tested; hardware pending |
| NEW-15 | New I/II DVs and III–V IVs start at zero; existing records preserved. (`88f974da`) | Host-tested; hardware pending |
| NEW-16 | Same direct assigned-source open as C01. (`app58 retained`) | Host-tested; hardware pending |
| NEW-17 | Native Gen V Key Item/TM/HM add/remove; unique cap 1, correct pockets, warnings, Review/export; no quest/event-flag edits. (`88f974da`) | Host-tested; hardware pending |
| NEW-18 | Native I–IV Play Time and Seen/Caught counts; malformed clocks report Unavailable. (`20955b20`) | Host-tested; hardware pending |
| NEW-19 | One staged Review / Continue / Discard and Exit / Export Copy flow across I–IV. (`29084707`) | Host-tested; hardware pending |
| NEW-20 | One Party popup; native 44/48/100-byte staged Party edits; safe Keep/Discard/Continue; cache/status preservation. (`c81853da / be3d965a`) | Host-tested; hardware pending |
| AUD-01 | IV/V use one paged Review presenter, footer and Discard confirmation; every verified V field is reachable. (`e061f03c`) | Host-tested; hardware pending |

## Validation

- GCC and Clang complete normal host suites: all 197 binaries PASS each. Both complete ASan/UBSan suites PASS with leak detection enabled.
- Both frozen QA matrices: 469 cells, zero problems, normal and sanitized. The three reported cross-generation fact differences are retained. All 18 real export paths satisfy C1–C6, including strict reparsing, separate output publication, destination races, Discard refusal and original byte/size/mtime preservation.
- Native Party tests protect 44/48/100-byte formats, stored status/damage, cached stats, inactive slots, RTC and unchanged Party view objects. Actual shared popup tests cover A/touch parity, all four actions and B priority.
- Candidate source contracts: zero failures. Twenty shared broken-path controls PASS; the owner-PASS gate protects all 70 recorded passes with 196 guards. Repository safety, touch/artwork and source immutability checks PASS.
- Complete QA tools job PASS, including independent NRO-audit unit tests, hardware-gate mutations, workflow parsing and header dependencies. The CI time budget accommodates every expanded check.
- CI's compiler flagged copied structured bindings in new tests. Commit `0de3420f76c489baf25611a54dd5bfb2b9febeaf` binds three loops by reference. The affected normal/sanitized Bag and encounter tests and 469-cell GCC matrix passed again. No application file or warning setting changed.
- Native candidate build and packaged RomFS verification PASS: 4267 files, 156.6 MiB, content digest `bc09c152ac4232a2`; app icon, region artwork, item/Pokémon sprites and source-referenced assets are included.
- Current exact-head CI and independent QA-F9 conclusion: [PR #307 checks](https://github.com/GlitchedZeus/PokeBank-NX/pull/307/checks). Owner hardware results remain the original 70 PASS / 37 FAIL / 3 NOT TESTED.

## Bound official candidate

[Download the official candidate artifact](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39728794477/artifacts/12404690063) · [build run](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39728794477)

| Identity | Value |
| --- | --- |
| NRO filename | `PokeBank-NX-Gen1-5-Candidate-c60d4da0.nro` |
| Application source | `c60d4da0abc467aedac54aa886179c89fc89bc19` |
| Source tree | `b646118a299bbe531f87d948d7a1d55aff4e22de` |
| NRO SHA-256 | `6a150386e48f9a8ea075032de453bde036550dca7adfdf40ee3388f83092c456` |
| NRO size (bytes) | `176610993` |
| Build run | `39728794477` |
| Artifact ID | `12404690063` |

The docs-only binding in `docs/qa/CANDIDATE_UNDER_AUDIT.env` selects this exact published file. QA-F9 downloads it from its original run without rebuilding; it checks the NRO bytes/hash/size, embedded source identity, recorded tree, packaged assets, candidate ancestry and application-file freshness. [Audit workflow records](https://github.com/GlitchedZeus/PokeBank-NX/actions/workflows/app-qa-nro-audit.yml) contain the independent result. The staleness guard remains enforced.

This is the sole artifact selected for this hardware round. Other smoke or automatically produced artifacts are not the bound device candidate. No complete legality-engine or PKHeX-parity claim is made; PR #314 remains separate. S06 remains NOT TESTED; no live emulator save was overwritten.

## Device retest checklist

Use only the final audited candidate when supplied. Keep original emulator saves intact. Export Copy always writes a separate output.

| Steps | Expected result |
| --- | --- |
| Open a game → Save/Backups → inspect current save → Create Backup. | Explicit backup appears newest first with the correct provider/path; opening an unchanged save creates none. |
| Select an eligible app-owned backup → Delete → B; repeat → Y. | B keeps it; Y deletes only that eligible backup. Original saves cannot be deleted. |
| Open Crystal → PC Box → Move Pokémon → place in another box. | Pokémon moves successfully and appears once in Review; Gen I/III PC Move still works. |
| Open Red, Crystal and a Gen III game → Party → select a Pokémon with A, then by touch. | Same View / Edit / Origin & Legality / Cancel popup appears. |
| Party → Edit → change a level and move → B → Continue; then Keep → Review. | Continue preserves the draft; Keep stages it; Review shows named move changes and Party location. |
| Party → Edit → change a field → B → Discard. | That draft disappears; earlier staged work remains. Status/damage and other Pokémon remain correct. |
| Open any Gen I–IV Trainer → change Name/Money → Review Changes. | Changes are readable; Continue/Discard and Exit/Export Copy are available. |
| Make several Gen IV/V Pokémon and Bag changes → Review → L/R through every page. | Same layout and controls; every changed field is reachable; names replace raw IDs where available. |
| Open Gold/Silver/Crystal → edit a Pokémon → inspect Special. | One Special DV/Stat Exp; separately labelled calculated Sp. Atk and Sp. Def. |
| Open Crystal → Unown → change/reroll DVs. | Letter/art and shiny appearance follow the native DVs; no second Special DV exists. |
| Gen III–V → Nature → inspect neutral and raised/lowered examples. | Five neutral natures are labelled; raised stat is green +10%, lowered is red −10%; HP is unaffected. |
| Gen IV/V → Move → select a move with Power/Accuracy. | Category is readable without crowding the move details. |
| Gen IV/V → Pikachu or Walrein → Met Location → X Other histories → B. | Default list uses species/ancestor wild context plus stored history; other native histories say unverified; B changes nothing. |
| Gen V → edit Language → Keep → Review → Export Copy. | Correct language is reviewed/exported; OT/nickname stay unchanged. |
| Create a Pokémon in each generation → inspect DVs/IVs before editing. | Editable DVs start at 0 for I/II; IVs start at 0 for III–V. |
| Gen V → Key Items or TM/HM pocket → Add/Remove → Review. | Native unique-item limits and warnings work; no automatic story flag changes. |
| Open Help → B; inspect Bag Remove and Key Item Warning. | B closes Help; redundant footer text is gone; explicit Bag Remove uses A to confirm/B to cancel. |
| Check Trainer summaries I–IV, species/held-item search, Ball/Ability pickers and touch scrolling. | Real time/Dex counts and prior working navigation/artwork remain correct. |
| Make edits → Export Copy in I–V. | A verified separate output and visible destination/result appear; errors remain visible; originals stay unchanged. |
| Inspect Origin & Legality in every generation. | Shared facts layout; missing evidence stays Not checked/Incomplete/Unknown. A clean verdict is never invented. |

**S06 stays NOT TESTED.** Do not load or overwrite a live emulator save as part of this checklist. The owner has not authorized that test.

## Source provenance and limits

Wild-location context uses pinned [PKHeX EncounterArea4](https://github.com/kwsch/PKHeX/blob/6042efa241a9925ce57e645329ea7f976deabdd3/PKHeX.Core/Legality/Encounters/Templates/Gen4/EncounterArea4.cs), [EncounterArea5](https://github.com/kwsch/PKHeX/blob/6042efa241a9925ce57e645329ea7f976deabdd3/PKHeX.Core/Legality/Encounters/Templates/Gen5/EncounterArea5.cs), evolution resources and location marker constants. The committed manifest records source hashes. This is species-level UI context: form/season/time/lead/evolution/distribution requirements are unchecked. Absence of wild data cannot reject an egg, gift, static, trade, event or transfer history.

The separate complete legality engine is not integrated. These UI repairs do not establish PKHeX legality parity or device acceptance.
