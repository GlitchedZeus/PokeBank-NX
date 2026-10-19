# Hardware round 1 results — PokeBank-NX-Gen1-5-Candidate-7fea1797

Authoritative record of the owner's round 1 sheet, copied verbatim from the coordinator's record
(`docs/hardware/ROUND1_OWNER_SHEET_7fea1797.md` on `claude/optimistic-euler-ph2z4f`, commit `432fe2cc`).
QA does not reclassify anything. Only the owner records acceptance.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-7fea1797.nro` |
| Code SHA | `7fea17971906cae557a7207bfb7b0bed3548cea2` |
| NRO SHA-256 | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` (175685297 bytes) |
| Build run / artifact | run `38946941097`, artifact `12066747065` |
| QA-F9 audit | app-qa-nro-audit run `38947904299`, job `117131594707`: PASS |
| Checklist | [RETEST_7fea1797.md](RETEST_7fea1797.md) |
| Regression gate | [`tests/test_hw_round1_regression_gate.py`](../../tests/test_hw_round1_regression_gate.py) maps each of the 42 PASS IDs to its automated guards |

---

# Owner hardware checklist — candidate 7fea1797 (round 1), as submitted

Recorded verbatim from the owner's interactive checklist. **42/54 verified working on the Switch.**
Unchecked = not verified on device, or a problem described in the owner's notes. Do not reclassify.

| ID | Item | Owner's note on the sheet | Result |
| --- | --- | --- | --- |
| C01 | A selects and advances normally | Home, Games, menus and editor | PASS |
| C02 | B never quits without warning | Home shows Quit confirmation; A/B stay, Y quits | PASS |
| C03 | Launch requires confirmation | ZR shows Launch dialog; Y launches, A/B cancel | PASS |
| C04 | L/R behave correctly | Home changes games, no accidental save opening | PASS |
| C05 | Gen II Create A/B fixed | A edits/selects, B backs out without wrong actions | PASS |
| C06 | Popups no longer activate background controls | Including touchscreen and controller input | PASS |
| C07 | Backup deletion requires Y confirmation | A alone never deletes | PASS |
| C08 | Pokémon Release / Remove requires Y confirmation | Cancellation preserves Pokémon | PASS |
| C09 | Second controller works properly | No A/B/R mix-ups on another Joy-Con or Pro Controller | PASS |
| M01 | Gen I Moves panel properly aligned | Four moves, PP, PP Ups and cursor | PASS |
| M02 | Gen II Moves panel matches Gen I | Same columns and selection layout | PASS |
| M03 | Gen III Moves panel matches | No shifted PP, clipped rows or hidden values | PASS |
| M04 | Gen IV Moves panel matches | Same clean shared layout | PASS |
| M05 | Gen V Moves panel matches | Same shared interface, appropriate native details | PASS |
| M06 | Move chooser consistent across generations | Move number, name, type, power, accuracy and PP where applicable | PASS |
| M07 | Move PP editing and PP Ups work | Correct generation-specific maximum PP | PASS |
| M08 | Gen I hidden-row appearance fixed | All visible rows and cursor outlines fit within the panel | PASS |
| B01 | Gen I Add Item workflow improved | Add is separate from editing a held item | **FAIL** (see notes) |
| B02 | Gen II Add Item workflow improved | Same controls and quantity behavior | **FAIL** (see notes) |
| B03 | Gen III item editing still works correctly | Clean baseline workflow preserved | **FAIL** (see notes) |
| B04 | Gen IV and V Add Item workflows match | Shared item picker and quantity editing | NOT VERIFIED |
| B05 | Bikes, rods, Key Items and other unique items limited to one | No nonsensical quantities; TM/HM behavior respects the game | PASS |
| B06 | Stackable item quantity editing works | Correct quantity range and no unwanted numpad | **FAIL** (see notes) |
| B07 | Removing items requires confirmation | Y Remove, B Cancel | PASS |
| B08 | Bag changes appear in Review and exported copy | No source-save overwrite | PASS |
| E01 | Make Shiny removed from quick Actions | Shiny remains an editor field | PASS |
| E02 | Actions menu order consistent | View, Edit, Clone, Move where supported, Release, Origin & Legality, Review | PASS |
| E03 | Origin & Legality heading matches in Gen I–V | No old Encounter Legality labels | NOT VERIFIED ("honestly i dont know") |
| E04 | Rows 1/15 counters removed | No text blocking the cursor | PASS |
| E05 | Green A Select / red B Cancel footers consistent | Species, moves, item pickers and confirmations | PASS |
| E06 | Shiny shows simply Yes / No | No DV-derived clutter; supported toggle works | PASS |
| E07 | Pokérus cycles inline with A | No unnecessary popup | PASS |
| E08 | Gender editing works without a huge picker | Where applicable and format-valid | PASS |
| E09 | Gen III language cycles inline | Only supported languages | PASS |
| E10 | Y immediately rerolls DVs / IVs / EVs | No repeated confirmation dialog | PASS |
| E11 | Rerolls preserve requested Shiny / Gender | Also Gen II Unown letter where applicable | NOT VERIFIED (owner doubts mechanics; see notes) |
| E12 | Gen I Create defaults to Pikachu Level 5 | Thunder Shock / Growl, correct PP | PASS |
| E13 | Gen III PID-linked fields and Unown editing work | No unexpected nature, gender, ability or form changes | PASS |
| L01 | Gen II DATA panel text stays inside its box | Caught history no longer overlaps the graph | PASS |
| L02 | Crystal location picker is compact and readable | No huge popup or raw IDs | PASS |
| L03 | Gen III location picker cleaned up | Readable names without (RSE), (FRLG) or ID numbers | PASS |
| L04 | Gen IV/V location windows use clean layout | No clipped entries, scroll and focus work | PASS |
| L05 | No editor panel cursor or row overlaps | Especially first and last visible rows | PASS |
| L06 | Party / summary taps open the right Actions screen | No unrelated generic View flash | NOT VERIFIED ("i dont know") |
| N01 | First scroll press responds immediately | D-pad and stick | PASS |
| N02 | Holding direction scrolls quickly and consistently | No quarter-second stall between each row | PASS |
| N03 | Touch scrolling and joystick focus behave properly | No menus bleeding input into panels behind | PASS |
| N04 | Long lists and keyboard repeat feel smooth | Species, moves, items and other menus | PASS |
| S01 | Gen V matches the shared Gen I–IV editor | Create, Edit, View, moves, fields and layout | **FAIL** (see notes) |
| S02 | Gen V party, boxes and trainer display correctly | Correct data instead of cached or blank information | PASS |
| S03 | Gen V Create, Clone, Move and Release work | Using disposable test save | **FAIL** (see notes) |
| S04 | Review / Discard / Export Copy work | Changes only staged until explicitly exported | NOT VERIFIED (owner suspects; "needs more work") |
| S05 | Original saves stay unchanged | Backup and edited copy exist separately | PASS |
| S06 | Exported edited save loads in emulator | Edits persist without corruption | NOT VERIFIED ("we havent added writing to the app yet") |

Totals: C 9/9, M 8/8, B 3/8, E 11/13, L 5/6, N 4/4, S 2/6 = **42/54**.

## Owner's notes — broken fixes / remaining bugs (verbatim)

> B01:vgen on still has the numpad pop up before its put in my bag, i shou have it in my bag and than i can add more quantity.  B02 same as B01, B03 same as B01 and B02, B06 also a part of the B01,02,03,  E03 honestly i dont know E11 i dont think  a  shiny and nonshiny can have the same stats this needs look into more.  L06 i dont know  S01 the pp and PP ups text are to close together so it look unreable/sloppy.  S03 gen 5 editor doesnt match gen 1-4 th left side menu text for the speices names is cut off and all that plkus awhole lot more.  S04 gen 5 needs more work i dont think this work properly like the others do. S06 we havent added writing to the app yet  i wouldnt know yet

Note: the owner's sheet title for S01 is "Gen V matches the shared Gen I–IV editor" and for S03 "Gen V Create, Clone, Move and Release work"; the owner's notes attach the PP/PP Ups caption spacing to S01 and the left-panel species clipping + parity to S03. Both notes are real defects; both are handled under Phase 9 (captions) and Phase 12 (parity). Whether Create/Clone/Move/Release themselves fail on Gen V is not stated — treat as part of the S04 trace, not as confirmed broken.

## Owner's new requests (verbatim)

> can we shrink box menu if they need it like in the bag menu can we have it so menu dont have so much blank space in the middle so we can easily see the item we wanna add and dont accidently add the item above or below it cause we cant see it properly cause they far away, now if a menu if change for one gen it has to ben constant for all gens, BUT one gen can have different sixed menus, like if to make them look good they have to all bedifferent sizes to fit all theyre stuff cool, but all gens must follow that exact setup, i want it to be uniform, so if the pokeball menu gets shrinked in gen 1 its would be the same size in the gen 5 editor, got it, for example the key item warning menu, all bag menus, dont make them super small but make them a nice size with no massive gaps with the name and than the pictures,sprites, the held item menu just needs a smidge the text and sprites closer and that should be used for like a example or some shit. can we add a search to the pokemon species picker, so we click on search or press a button like "+ or -" and than we can type what pokemon we want, but can we have it so the search is active like if i press C than all pokemon with C popup and then if i enter O than all pokemon that start with CO and so on and so on, and if i detlete a letter it reverts to just the pokemon with said letters, is called active search ??. gen 1 has in the view menu screen the stats that are uneditable are gold and in the editor menun they are white, change we change so both are gold, also can we make the HP DV gold in both since we cant edit that either ?? or would that be weird. gen 1 trainer menu doesnt allow me to edit the trainer and money stats, but in gen 2 i can we need to add a footer saying A to edit and also the ablity to edit in gen 1 and the footer in both gens and this needs done for all gens btw, in gen 1 the Special Stat has a number in gen 2 i doesnt seem to be readable it just has a  "--"  the launch menu also needs resizes and the A and B proper colored buttons added instead of just the text. "[ ] Backup deletion requires Y confirmation" this only works for the native switch games when i try to goin to my emulator backups its says they are read only source and i can open that menu (when i press + on a game card). SCROLL BARS if they are in a menu can i use them to fast scroll in stead of swiping with finger repeatly, this is for all scoll bar in the pokebank app. the moves sector menu look like it could have 1 or 2 more rows instead of that blank space, this is for all gens. in the bag item menu the delete confirmation menu thing needs shirinked down to a proper size. in some of the menu like "items" it still makes me pick a quanity beofre it goes into my bag, i should pick it it pops up in bag and from there i can edit the quanity. the reroll iv/ev i dont know if the info is correct cause i can have a shiny and non shiny pokemon with the exaclt same stats and i dont thnk that is possible, i think shinies have to have a certain amout of points this needs fixed for all gens. the met time in pokemon crystsal needs to be changed so i can press A on it and it changes instead of opening a menu to choose. gen 5 saves dont tell me in the create menu that im making a pokemon for what game like the rest in gens 1-4 do at the top of the screen where source save staged is.

## Request → Round 2 phase mapping

| Request | Phase / lane |
| --- | --- |
| Compact menus, same menu type = same size across gens, held-item picker as reference, key-item warning, bag menus, box menus, bag Remove confirmation size | Phase 2 (S) |
| Species active search (+/− or Search button, prefix filter, backspace reverts) | Phase 3 (P) |
| Gen I read-only stats gold in View and Edit; HP DV gold in both | Phase 4 (C, with S's SharedFieldState) |
| Gen II Special "--" | Phase 4 (C) |
| Gen I Trainer/Money editing; "A Edit" footer all gens | Phase 5 (C) |
| Launch menu resize + coloured A/B buttons (Y still launches) | Phase 6 (2R) |
| Emulator backups via + → "read only source" | Phase 7 (2R) |
| Draggable scrollbars everywhere | Phase 8 (P) |
| Move chooser 1–2 more rows, all gens | Phase 9 (S) |
| Add Item → in bag at qty 1 first, then edit quantity | Phase 1 (I) |
| Shiny vs non-shiny reroll stats | Phase 10 (R) — investigate against pret/PKHeX; report what is genuinely possible |
| Crystal Met Time: A cycles inline | Phase 11 (C) |
| Gen V Create shows the source game at the top like Gen I–IV | Phase 12 (S) |
