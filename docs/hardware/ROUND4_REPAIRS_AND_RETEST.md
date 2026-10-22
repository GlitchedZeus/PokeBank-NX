# Round 4 application repairs and device retest record

**Host validation complete; official candidate and hardware acceptance pending.**

Validated source checkpoint: `2ba52983bbe663966be25f2d74c2425657e7331d`.
Application integration target: [draft PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307).
Backup `44fb4b7eaa58f3e627ff981d84f2caeaed293667` is merged forward, retaining both parents.
The original application baseline `58a81207` is preserved. Legality PR #314 and main are untouched.

Owner hardware baseline stays **70 PASS / 37 FAIL / 3 NOT TESTED**. Every repair below needs the owner to retest the final audited NRO. S06 remains NOT TESTED; no live emulator save was overwritten.

## Original 37 failures

All 37 entries below are implemented and host-tested in the validated application snapshot. The campaign preserves the earlier 12 app58 repairs. Hardware status for every row is **pending**; application CI and exact-binary audit are separate gates.

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

## Validation at this checkpoint

- Focused backup/export tests: 36 executions across GCC/Clang and normal/ASan+UBSan passed; real separate outputs, reparses and unchanged source bytes/size/mtime were checked.
- Language/defaults/natures/native editor tests: 32 focused executions passed; unique Bag/inventory tests: another 12 passed.
- Shared Review: 16 executions passed; Gen V trace covers 400 checks across W/B/W2/B2 and all semantic Review pages.
- Met Location: four GCC/Clang normal/sanitized executions passed; all nine exact titles, native IDs, ancestry and unknown histories; pinned-source reproduction and corrupt-offset rejection passed.
- Party: 16 GCC/Clang normal/sanitized executions passed; real native I/II/III records and actual shared popup coordinator. The Gen III metadata-only cache-preservation follow-up passed the complete Clang normal/sanitizer suites; the GCC normal suite also passed.
- All sanitizer executions above used leak detection. Twenty broken-path shared guard controls passed before and after exact-source parser caching. Candidate source contracts have zero violations; all actionable Round 4 XFAIL markers are removed.
- The 70-owner-PASS gate (196 guards), repository safety and Gen V isolation guards passed at milestones. Both complete normal suites (197 binaries each), both complete ASan/UBSan suites with leak detection, and both frozen QA contract/matrix runs passed.
- Native checkpoint `c9a457a3`: [Switch build 39687900533](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39687900533) passed compile/link, asset checks and packaged RomFS. Party checkpoint `c81853da` [native run 39689447670](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39689447670) also passed compile/link, assets and packaging. Cache-preservation head `be3d965a` also passed native run 39690466629. Review/Language follow-up `629d6365` passed native run 39725096656. The later Gen II Party refresh preservation repair at `2753e17b` has passed focused Clang normal/sanitizer tests; native run 39726730148 passed all compile/link/assets/package steps. Both full normal suites passed all 197 tests at `2ba52983`; both complete sanitizer suites passed with leak detection. Smoke artifacts are not official candidates.

- Expanded Clang QA contracts + matrix passed normal and ASan/UBSan with leak detection: 469 matrix cells, zero problems, all 18 export paths satisfy C1–C6. The equivalent GCC run also passed all 469 cells and 18 export paths in normal and sanitized builds. Both complete normal suites passed all 197 binaries; both complete ASan/UBSan suites passed with leak detection.
- Fourteen current PR source guards passed; the generated-species contract also passed with its pinned source fetch enabled.

- Complete QA tools job passed (635.34 seconds under compiler load): all negative controls, NRO audit tooling, hardware-gate mutations, workflow parsing and header dependencies. Its CI time budget is twenty minutes; no check was skipped.

## Candidate / audit

**Pending.** The committed audit binding still identifies the older `62a2f8c3` NRO. The staleness guard is preserved. No official Round 4 artifact, final filename/hash/size/tree or QA-F9 PASS is claimed. These must come from one final integrated Gen I–V candidate after the gates pass.

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
