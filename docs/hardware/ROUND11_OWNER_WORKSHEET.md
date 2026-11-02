> **Repository note (added October 31, 2026; the owner's worksheet below is unchanged, verbatim).**
> The worksheet records no NRO, SHA-256, commit or date. Its H01–H17 match the Round 10 checklist (`docs/hardware/RETEST_3634d1e6.md`) item for item, including H16/H17, which exist only in Round 10. The tested build is therefore **inferred** to be `PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro` (SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`). This is an inference, not a recorded identity. H18–H48 were added outside the repository checklist. No screenshots were attached. H13 is recorded as PASS by the owner but the note says no Japanese save was opened, so it is **not hardware evidence** for Japanese saves. Engineering intake: `docs/hardware/ROUND11_ENGINEERING_INTAKE.md`.

# PokeBank NX — Round 11 interactive hardware test results

**This is a user-completed hardware worksheet, not an automated CI report.**

## Candidate and session

- **NRO installed:** NOT RECORDED
- **SHA-256 calculated:** NOT RECORDED
- **Build commit or URL:** NOT RECORDED
- **Testing date:** NOT RECORDED
- **Games/emulators tested:** NOT RECORDED
- **Switch / Atmosphère:** NOT RECORDED

## Summary

| Scope | PASS | FAIL | NOT TESTED | UNMARKED | Total |
|---|---:|---:|---:|---:|---:|
| Essential H01–H17 | 15 | 2 | 0 | 0 | 17 |
| All H01–H48 | 34 | 13 | 1 | 0 | 48 |

## Failed tests requiring review

### H04 — Top Up and Add Missing preview
- **Severity:** NOT CLASSIFIED
- **Area:** Bag / Bulk
- **Game / emulator:** NOT RECORDED
- **Steps:** Bag → ZR → Top Up → This pocket → Amount 50 → Preview; cancel with B. Preview again and confirm with A. Open Review.
- **Expected:** Preview lists changes/skips; B makes no change; A stages only allowed edits; Key Items and HMs never change.
- **Actual / notes:** top up and add only fill the current menu, the bag categoties arnt filled which they should be it and option of add all i think
- **Screenshots:** NONE RECORDED

### H12 — Recovery files cancellation
- **Severity:** NOT CLASSIFIED
- **Area:** Recovery UI
- **Game / emulator:** NOT RECORDED
- **Steps:** Home → + → Backups for an emulator game → X Recovery files; select an older copy, press Y, then cancel with B.
- **Expected:** Newest save and Bank copies display as protected; B cancels and deletes NOTHING.
- **Actual / notes:** this doesnt work i cant see what is selected and nothing in the footer and nothing on screen changes for it, this needs worked on
- **Screenshots:** NONE RECORDED

### H22 — DV column and Unown sprite
- **Severity:** NOT CLASSIFIED
- **Area:** Gen II / Editor
- **Game / emulator:** NOT RECORDED
- **Steps:** Open Gen II editor; inspect DV column; compare Unown form in box card, picker and detail if one exists.
- **Expected:** DV row/column displays intact; Unown form/sprite consistent across views or discrepancy documented.
- **Actual / notes:** we still dont have a number in the calculated column for special calculated stat, add it when i swithc unowns i get a pop in middle bottom about stat changes and i dont need it, its like the language thing we fixed in another problem i also get the pop uo for rerolling the de/ev/iv/ev's can we fix this for all gens and while w are at it when im in gen 5 and go to choose shiny on a pokmon in the middle colum i get a pop up menu instead of a inline choose like we have for pokerus and language and gender,
- **Screenshots:** NONE RECORDED

### H25 — Nature stat colours and trainer fields
- **Severity:** NOT CLASSIFIED
- **Area:** Gen IV / Editor
- **Game / emulator:** NOT RECORDED
- **Steps:** Inspect Gen IV nature-influenced stat colours; edit OT/met level if supported; cancel.
- **Expected:** Stat colours match positive/negative nature modifiers; fields do not overlap; cancel does not stage changes.
- **Actual / notes:** gen leaves info in the left side panel ffrom the previous pokemon that was in the editor, like i had a pikachu in the editor and had its met location to trohpy arden and changed species to arbok and arbok has the same met location and pokerus stat aswell,
- **Screenshots:** NONE RECORDED

### H27 — Language chooser A-cycles
- **Severity:** NOT CLASSIFIED
- **Area:** Gen V / Editor
- **Game / emulator:** NOT RECORDED
- **Steps:** In a Gen V Pokémon editor, focus Language and press A repeatedly.
- **Expected:** Language cycles directly; a large modal does not open; displayed language is consistent. Do not commit unsafe edits.
- **Actual / notes:** language changes but the top names doesnt change like in gen 1-4, all gens need the proper names for their language and also we need to add the korean letters in cause they look like this "[][][][][]" also in our keyboards we need to add the proper letters and charcters for japenese and korean and all languges is that possible in the nanog kayboard and is it possible to add a new button in our custom search bar that would also change the letters or would it be easier to have when we can the language everyhting would get change to that language?? feedback ??
- **Screenshots:** NONE RECORDED

### H29 — Y shiny toggle
- **Severity:** NOT CLASSIFIED
- **Area:** Pokémon / Shiny
- **Game / emulator:** NOT RECORDED
- **Steps:** Where supported, press Y on a Pokémon details/editor view; inspect shiny state and Review, then discard.
- **Expected:** Shiny state toggles as intended without changing unrelated data; cancel or discard restores baseline.
- **Actual / notes:** gen 5 in the editor view doesnt have inline selcetion for shiny option, it opens a menu and warns about pid, this needs changed to be inline like eveery other gen (1-4)
- **Screenshots:** NONE RECORDED

### H31 — Clone to available slot
- **Severity:** NOT CLASSIFIED
- **Area:** Pokémon / Clone
- **Game / emulator:** NOT RECORDED
- **Steps:** On a dispensable Pokémon, stage Clone to an empty box slot; inspect source and destination; discard.
- **Expected:** Both staged copies are correct; no existing slot overwritten; discard restores baseline.
- **Actual / notes:** gen 1 works perfect, gen 2 when i clone a pokemon its creates it in the box and than revert back to the action menu with clone, edit,view,release,oring and legality, it shouldnt do that, it should clone the pokemon and no menu popup like gen 1, gen 3 has the numpad come up when clone is click on and that is completey wrong fix this so it behaves like gen 1 but with being able to select which slot it can on to now that it isnt locked like gen 1/2. ps even when i entered a number on the numpad on gen 3 it didnt work so yeah that broken. gen 4/5 has same problem as gen 3, when they are all fixed they should work like gen 1 but have the ability to select which slot cause it aint like gen 1/2 now, look at PKSE how they clone and also can we have the hover over box feature that PKSE has, so we go to the box/slot we want and than press A to confirm the clone and it pops the pokemon in the slot. 
- **Screenshots:** NONE RECORDED

### H34 — Add and pocket capacity
- **Severity:** NOT CLASSIFIED
- **Area:** Bag / Add
- **Game / emulator:** NOT RECORDED
- **Steps:** Add a safe regular item within capacity; if naturally full, try Add in that pocket without deleting data.
- **Expected:** Valid Add stages exactly once; naturally full pocket refuses safely, without replacing any item.
- **Actual / notes:** the pc items doesnt get filled with top up feature and also gen 1 doesnt seems to work, check out all gesn for that make sure they all work for every pocket and bag categotry and pc items, key items dont worrry about tho cause that could mess up users saves
- **Screenshots:** NONE RECORDED

### H39 — Standalone file browser selection/cancel
- **Severity:** NOT CLASSIFIED
- **Area:** Files / Repeat
- **Game / emulator:** NOT RECORDED
- **Steps:** Open file browser, enter/leave folders with A/B, cancel an import, reopen and select a valid file.
- **Expected:** Back navigation works, cancel causes no staged mutation, valid file loads in its correct format.
- **Actual / notes:** can we add a cancel feature, right now A opens stuff and confirms and B goes to the parent directory or "goes back" we need to add a buttoin that is cancel which closes the menu and cancel the import can we make it "-" add the footer for all needed spaces and gens and app wide
- **Screenshots:** NONE RECORDED

### H40 — Bank UI opens and persists safely (if supported)
- **Severity:** NOT CLASSIFIED
- **Area:** Bank / Smoke
- **Game / emulator:** NOT RECORDED
- **Steps:** If the Bank pane is available, open it, inspect boxes, then exit without changes; optionally rename a box, Save, reopen.
- **Expected:** No crash/corruption; bank boxes display; supported save/reload preserves exactly the accepted change.
- **Actual / notes:** can rename a box inside the save its doesnt allow me to select it, the only boxes i can rename are bank boxes or as their called right now in the app "legacy storage"
- **Screenshots:** NONE RECORDED

### H41 — Bank deposit (supported format only)
- **Severity:** NOT CLASSIFIED
- **Area:** Bank / Deposit
- **Game / emulator:** NOT RECORDED
- **Steps:** On a dispensable Pokémon from a supported game group, stage a Bank deposit if UI offers it; save Bank using intended action, reopen.
- **Expected:** Deposited Pokémon persists with correct species/OT/moves; original source save remains unchanged until Export Copy.
- **Actual / notes:** ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet
- **Screenshots:** NONE RECORDED

### H42 — Bank withdrawal and cancellation (if supported)
- **Severity:** NOT CLASSIFIED
- **Area:** Bank / Withdraw
- **Game / emulator:** NOT RECORDED
- **Steps:** From Bank, choose an empty supported destination, preview/stage a withdrawal; cancel once, then try again and inspect Review.
- **Expected:** Cancel leaves custody unchanged; accepted staged withdrawal preserves identity and never silently overwrites occupied destination.
- **Actual / notes:** not supported yet
- **Screenshots:** NONE RECORDED

### H43 — Gen I/II/IV/V Vault availability check
- **Severity:** NOT CLASSIFIED
- **Area:** Bank / Known limitation
- **Game / emulator:** NOT RECORDED
- **Steps:** On classic saves, inspect whether the UI actually offers native Vault/Bank deposit and withdrawal; do not convert formats to force a test.
- **Expected:** Record AVAILABLE or NOT IMPLEMENTED as an observation; a missing path is a tracked v1.0 gap, not a regression in this Gen I–V candidate.
- **Actual / notes:** gen 1 does not have "move to vault or bank or what ever" 
- **Screenshots:** NONE RECORDED

## Complete test ledger

### H01 — Install and open
- **Status:** PASS
- **Scope:** Essential
- **Area:** Core / Setup
- **Steps:** Back up the SD. Record the actual NRO filename and SHA-256; launch from the Homebrew Menu.
- **Expected:** App opens to Product Home without a crash, unexpected write request, or duplicate save instance.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H02 — One save per generation
- **Status:** PASS
- **Scope:** Essential
- **Area:** Core / Saves
- **Steps:** Open one ordinary Gen I, II, III, IV and V save if available. Inspect trainer, party and box; exit untouched.
- **Expected:** Names, party, boxes, badge count, money and trainer context are plausible and unchanged; unedited exit is clean.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H03 — Inventory Controls help fits
- **Status:** PASS
- **Scope:** Essential
- **Area:** Bag UI
- **Steps:** Open Bag in a Gen I–III save and a Gen IV/V save. Press − for Help.
- **Expected:** Inventory Controls dialog is legible with no clipped or overlapping lines.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H04 — Top Up and Add Missing preview
- **Status:** FAIL
- **Scope:** Essential
- **Area:** Bag / Bulk
- **Steps:** Bag → ZR → Top Up → This pocket → Amount 50 → Preview; cancel with B. Preview again and confirm with A. Open Review.
- **Expected:** Preview lists changes/skips; B makes no change; A stages only allowed edits; Key Items and HMs never change.
- **Game / emulator:** Not entered
- **Notes:** top up and add only fill the current menu, the bag categoties arnt filled which they should be it and option of add all i think
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H05 — Export same Pokémon twice
- **Status:** PASS
- **Scope:** Essential
- **Area:** Pokémon / Export
- **Steps:** On one Pokémon use A → Export to file; repeat on same Pokémon.
- **Expected:** Files appear under switch/PokeBank-NX/pokemon/genN; second has a distinct “ (2)” name; source save and Review unchanged.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H06 — Import exported Pokémon into an empty slot
- **Status:** PASS
- **Scope:** Essential
- **Area:** Pokémon / Import
- **Steps:** On an empty slot in a save of the SAME generation, A → Import from file → select H05 file; inspect Review; try B navigation.
- **Expected:** Imported Pokémon is staged and shown in Review; B navigates browser correctly; original source remains untouched.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H07 — Create requires Met Location
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen V
- **Steps:** In BW/B2W2 create in an empty box slot. Try Keep before choosing Met Location, then select a location and Keep.
- **Expected:** “Choose a location” appears; premature Keep refuses; picking a location permits staged creation.
- **Game / emulator:** Not entered
- **Notes:** this needs to be added for all gens
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H08 — Gender changes inline
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen V
- **Steps:** Edit a species capable of either gender. Select Gender; press A twice.
- **Expected:** Gender cycles without an extra Change PID dialog; status reports PID change; returning works.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H09 — Trainer Gender and Pokédex
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen V / Trainer
- **Steps:** Open Trainer, change Gender, inspect Review; open Pokédex row.
- **Expected:** Trainer / Gender appears in Review; Pokédex stays in its read-only section and opens its list.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H10 — Play Time editable
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen I / Trainer
- **Steps:** Open Red/Blue/Yellow Trainer and stage a different Play Time; inspect Review.
- **Expected:** Play Time allows supported hours up to 255 and appears as a staged change.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H11 — L/R while typing
- **Status:** PASS
- **Scope:** Essential
- **Area:** UI / Search
- **Steps:** Open species/item search, enter text, press L then R.
- **Expected:** L removes final character; R is inert; screen behind search does not move.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H12 — Recovery files cancellation
- **Status:** FAIL
- **Scope:** Essential
- **Area:** Recovery UI
- **Steps:** Home → + → Backups for an emulator game → X Recovery files; select an older copy, press Y, then cancel with B.
- **Expected:** Newest save and Bank copies display as protected; B cancels and deletes NOTHING.
- **Game / emulator:** Not entered
- **Notes:** this doesnt work i cant see what is selected and nothing in the footer and nothing on screen changes for it, this needs worked on
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H13 — Japanese Gen I/II read-only
- **Status:** PASS
- **Scope:** Essential
- **Area:** Languages / Safety
- **Steps:** If available, open a Japanese Gen I or II save and inspect names and available actions.
- **Expected:** Kana renders; editors/writes remain locked; no original save mutation. Otherwise mark NOT TESTED.
- **Game / emulator:** Not entered
- **Notes:** i guess this passes i didnt test a japenese save but when i swap languages it has the proper text
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H14 — Export Copy is separate
- **Status:** PASS
- **Scope:** Essential
- **Area:** Save / Export
- **Steps:** Stage one safe change → Review → Export Copy. Inspect exported path and original source.
- **Expected:** A new copy is produced; source save is byte-unchanged; no live overwrite.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H15 — Rounds 6–8 smoke check
- **Status:** PASS
- **Scope:** Essential
- **Area:** Regression
- **Steps:** Repeat a few previously passed actions: navigation, open/exit, trainer, party/boxes and a harmless staged edit.
- **Expected:** No regression compared with previously accepted behavior; note any difference.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H16 — Remove keeps later items visible
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen IV / Bag
- **Steps:** In DP/Pt/HGSS Bag pocket with 3+ stacks, remove first or middle item. Inspect staged pocket; optionally Export Copy and open in emulator.
- **Expected:** Removed stack gone; every later item remains visible, original order preserved; exported game sees all remaining stacks if tested.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H17 — Berry/TM/HM Add uses native ordering
- **Status:** PASS
- **Scope:** Essential
- **Area:** Gen IV / Bag
- **Steps:** Add lower-numbered Berry and TM to Gen IV pockets, then add an ordinary Items item.
- **Expected:** Berry/TM inserts in item-ID order; normal Items pocket appends; no unrelated stacks changed.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H18 — D-pad and joystick parity
- **Status:** PASS
- **Scope:** Expanded
- **Area:** UI / Navigation
- **Steps:** On Home, game selection, party, boxes and a long modal, navigate once with D-pad and once with joystick.
- **Expected:** Both inputs reach all selectable controls; focus remains visible and never skips beyond list.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H19 — B closes nested menus
- **Status:** PASS
- **Scope:** Expanded
- **Area:** UI / Modals
- **Steps:** Open species, move, nature or item chooser; navigate a nested list and press B repeatedly.
- **Expected:** B returns one layer at a time without hidden menus, focus traps or unintended edit.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H20 — Emulator/provider identification
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Save discovery
- **Steps:** Open Save Instances for games you have on RetroArch/mGBA/Tico/DraStic/melonDS. Check provider labels and saves.
- **Expected:** Correct game/provider shown, save instances distinct, no accidental duplicate or unrelated save attached.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H21 — Gen I DV/EV and shiny controls
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Gen I / Editor
- **Steps:** Edit a safe copy or stage an edit; inspect DV/EV, derived HP DV and Y shiny action; discard.
- **Expected:** HP DV derived correctly and read-only if intended; no malformed values; discard restores original.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H22 — DV column and Unown sprite
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Gen II / Editor
- **Steps:** Open Gen II editor; inspect DV column; compare Unown form in box card, picker and detail if one exists.
- **Expected:** DV row/column displays intact; Unown form/sprite consistent across views or discrepancy documented.
- **Game / emulator:** Not entered
- **Notes:** we still dont have a number in the calculated column for special calculated stat, add it when i swithc unowns i get a pop in middle bottom about stat changes and i dont need it, its like the language thing we fixed in another problem i also get the pop uo for rerolling the de/ev/iv/ev's can we fix this for all gens and while w are at it when im in gen 5 and go to choose shiny on a pokmon in the middle colum i get a pop up menu instead of a inline choose like we have for pokerus and language and gender,
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H23 — Move between boxes regression
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Gen II / Storage
- **Steps:** On Gen II only, stage a move from an occupied box slot to an empty slot in another box; inspect both. Discard.
- **Expected:** Destination holds correct Pokémon; source stage empty; both boxes remain consistent; discard restores.
- **Game / emulator:** Not entered
- **Notes:** it works, can we fix for all gens when we pick up a pokemon can we change the drop control to A, so it would be moving the pokemon to the spot you want and pressing and and that would set the pop there instead of pressing Y again to drop it, can we also fix the footer and all the help menu and anything else that would need to be changed for this to work 
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H24 — Edit and discard
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Gen III / Editor
- **Steps:** In Gen III stage species/held item/move/stat change on a dispensable Pokémon; inspect Review then discard.
- **Expected:** Only intended fields change in preview; original Pokémon returns exactly after discard.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H25 — Nature stat colours and trainer fields
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Gen IV / Editor
- **Steps:** Inspect Gen IV nature-influenced stat colours; edit OT/met level if supported; cancel.
- **Expected:** Stat colours match positive/negative nature modifiers; fields do not overlap; cancel does not stage changes.
- **Game / emulator:** Not entered
- **Notes:** gen leaves info in the left side panel ffrom the previous pokemon that was in the editor, like i had a pikachu in the editor and had its met location to trohpy arden and changed species to arbok and arbok has the same met location and pokerus stat aswell,
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H26 — Legal move chooser list
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Gen V / Moves
- **Steps:** On Pikachu or another familiar Gen V species, open move picker and inspect available moves and count.
- **Expected:** Chooser is species-restricted, not the entire ~559-move global list; move labels/type/PP legible.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H27 — Language chooser A-cycles
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Gen V / Editor
- **Steps:** In a Gen V Pokémon editor, focus Language and press A repeatedly.
- **Expected:** Language cycles directly; a large modal does not open; displayed language is consistent. Do not commit unsafe edits.
- **Game / emulator:** Not entered
- **Notes:** language changes but the top names doesnt change like in gen 1-4, all gens need the proper names for their language and also we need to add the korean letters in cause they look like this "[][][][][]" also in our keyboards we need to add the proper letters and charcters for japenese and korean and all languges is that possible in the nanog kayboard and is it possible to add a new button in our custom search bar that would also change the letters or would it be easier to have when we can the language everyhting would get change to that language?? feedback ??
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H28 — Changing species clears incompatible moves
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Pokémon / Species
- **Steps:** On a dispensable staged Pokémon, choose another in-game species; inspect resulting move slots.
- **Expected:** Old invalid moves are cleared or safely reconciled; out-of-game species absent; Review shows correct changes.
- **Game / emulator:** Not entered
- **Notes:** this passes but i wanna have it where when the editor is opened the new pokemon that hasnt been added yet doesnt come with its default moves and alson when we switch species i dont want it to come with moves auto filled in, i want the user to pick the moves and all the info first and also can we have it where a pokemon cant be added until it has all the proper info, met location, moves for current level, like a level 100 pokemon should have 1-2 moves unless its breed or however that would be legal, 
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H29 — Y shiny toggle
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Pokémon / Shiny
- **Steps:** Where supported, press Y on a Pokémon details/editor view; inspect shiny state and Review, then discard.
- **Expected:** Shiny state toggles as intended without changing unrelated data; cancel or discard restores baseline.
- **Game / emulator:** Not entered
- **Notes:** gen 5 in the editor view doesnt have inline selcetion for shiny option, it opens a menu and warns about pid, this needs changed to be inline like eveery other gen (1-4)
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H30 — Create, back out, keep
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Pokémon / Create
- **Steps:** Create in empty box slot, cancel before Keep; try again and Keep; inspect Review.
- **Expected:** Cancel leaves slot empty; Keep stages exactly one new Pokémon; original save not written.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H31 — Clone to available slot
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Pokémon / Clone
- **Steps:** On a dispensable Pokémon, stage Clone to an empty box slot; inspect source and destination; discard.
- **Expected:** Both staged copies are correct; no existing slot overwritten; discard restores baseline.
- **Game / emulator:** Not entered
- **Notes:** gen 1 works perfect, gen 2 when i clone a pokemon its creates it in the box and than revert back to the action menu with clone, edit,view,release,oring and legality, it shouldnt do that, it should clone the pokemon and no menu popup like gen 1, gen 3 has the numpad come up when clone is click on and that is completey wrong fix this so it behaves like gen 1 but with being able to select which slot it can on to now that it isnt locked like gen 1/2. ps even when i entered a number on the numpad on gen 3 it didnt work so yeah that broken. gen 4/5 has same problem as gen 3, when they are all fixed they should work like gen 1 but have the ability to select which slot cause it aint like gen 1/2 now, look at PKSE how they clone and also can we have the hover over box feature that PKSE has, so we go to the box/slot we want and than press A to confirm the clone and it pops the pokemon in the slot. 
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H32 — Occupied destination safety
- **Status:** NOT TESTED / NO FIXTURE
- **Scope:** Expanded
- **Area:** Pokémon / Full slots
- **Steps:** Attempt Import/Move/Clone toward an occupied or unavailable destination, without confirming destructive replacement.
- **Expected:** App blocks or explicitly handles collision; no silent overwrite or Pokémon loss.
- **Game / emulator:** Not entered
- **Notes:** dont know how to test this 
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H33 — Remove confirmation and cancel
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Bag / Remove
- **Steps:** On a save with 3+ items, choose Remove; cancel once, then confirm; inspect Review and discard.
- **Expected:** Cancel changes nothing; confirmed operation edits only intended stack; discard returns Bag to original.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H34 — Add and pocket capacity
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Bag / Add
- **Steps:** Add a safe regular item within capacity; if naturally full, try Add in that pocket without deleting data.
- **Expected:** Valid Add stages exactly once; naturally full pocket refuses safely, without replacing any item.
- **Game / emulator:** Not entered
- **Notes:** the pc items doesnt get filled with top up feature and also gen 1 doesnt seems to work, check out all gesn for that make sure they all work for every pocket and bag categotry and pc items, key items dont worrry about tho cause that could mess up users saves
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H35 — Held item and TM/HM details
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Bag / Held
- **Steps:** Inspect held item chooser and Bag TM/HM entries across available generations.
- **Expected:** Appropriate type, power/accuracy/PP shown where supported; no clipping; selection does not alter Bag until confirmed.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H36 — Malformed pocket safe refusal (OPTIONAL)
- **Status:** PASS
- **Scope:** Expanded, OPTIONAL
- **Area:** Gen IV / Edge
- **Steps:** Only with a *synthetic, separate, deliberately malformed* Gen IV save: attempt Bag Add/Remove.
- **Expected:** App refuses with a useful message and no byte changes. Never corrupt a personal save to run this.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H37 — Gen V Bag read/navigation
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Gen V / Bag
- **Steps:** Open BW/B2W2 Bag pockets; browse first/middle/last stacks. Do not rely on unverified native sorting.
- **Expected:** Every pre-existing stack appears correctly; navigation works; any unexpected gap/hiding documented.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H38 — Cross-generation import refusal
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Files / Boundaries
- **Steps:** On a *separate staging copy*, browse a Pokémon file from the wrong generation or unsupported extension.
- **Expected:** Unsupported format is refused clearly; no destination change or original-save write.
- **Game / emulator:** Not entered
- **Notes:** how are we suppose to move a gen 1 pokemon  to like gen 5 ?? does it have to go through bank first or how we doing that ??
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H39 — Standalone file browser selection/cancel
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Files / Repeat
- **Steps:** Open file browser, enter/leave folders with A/B, cancel an import, reopen and select a valid file.
- **Expected:** Back navigation works, cancel causes no staged mutation, valid file loads in its correct format.
- **Game / emulator:** Not entered
- **Notes:** can we add a cancel feature, right now A opens stuff and confirms and B goes to the parent directory or "goes back" we need to add a buttoin that is cancel which closes the menu and cancel the import can we make it "-" add the footer for all needed spaces and gens and app wide
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H40 — Bank UI opens and persists safely (if supported)
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Bank / Smoke
- **Steps:** If the Bank pane is available, open it, inspect boxes, then exit without changes; optionally rename a box, Save, reopen.
- **Expected:** No crash/corruption; bank boxes display; supported save/reload preserves exactly the accepted change.
- **Game / emulator:** Not entered
- **Notes:** can rename a box inside the save its doesnt allow me to select it, the only boxes i can rename are bank boxes or as their called right now in the app "legacy storage"
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H41 — Bank deposit (supported format only)
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Bank / Deposit
- **Steps:** On a dispensable Pokémon from a supported game group, stage a Bank deposit if UI offers it; save Bank using intended action, reopen.
- **Expected:** Deposited Pokémon persists with correct species/OT/moves; original source save remains unchanged until Export Copy.
- **Game / emulator:** Not entered
- **Notes:** ui doesnt allow it we havent added any menu or anything in to the legacy storage or "bank" yet
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H42 — Bank withdrawal and cancellation (if supported)
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Bank / Withdraw
- **Steps:** From Bank, choose an empty supported destination, preview/stage a withdrawal; cancel once, then try again and inspect Review.
- **Expected:** Cancel leaves custody unchanged; accepted staged withdrawal preserves identity and never silently overwrites occupied destination.
- **Game / emulator:** Not entered
- **Notes:** not supported yet
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H43 — Gen I/II/IV/V Vault availability check
- **Status:** FAIL
- **Scope:** Expanded
- **Area:** Bank / Known limitation
- **Steps:** On classic saves, inspect whether the UI actually offers native Vault/Bank deposit and withdrawal; do not convert formats to force a test.
- **Expected:** Record AVAILABLE or NOT IMPLEMENTED as an observation; a missing path is a tracked v1.0 gap, not a regression in this Gen I–V candidate.
- **Game / emulator:** Not entered
- **Notes:** gen 1 does not have "move to vault or bank or what ever" 
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H44 — Bank and save recovery display
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Recovery / Safety
- **Steps:** Inspect Recovery files only; verify latest prior generation is labelled protected. Cancel any deletion.
- **Expected:** No unintentional pruning; original files unchanged. Do NOT intentionally simulate power loss on hardware.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H45 — Multiple edits → Review → Discard
- **Status:** PASS
- **Scope:** Expanded
- **Area:** Staging / Review
- **Steps:** Make two safe staged edits in a disposable save/session; inspect Review; discard; reopen.
- **Expected:** Review has both changes and correct counts; discard restores both changes; no source mutation.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H46 — Export Copy reload and compare (OPTIONAL)
- **Status:** PASS
- **Scope:** Expanded, OPTIONAL
- **Area:** Save / Verify
- **Steps:** Export a harmless staged change to a separate file; if comfortable, load ONLY that copy in an emulator or compare on PC.
- **Expected:** Export reparses and reflects the intended change; original source remains byte-identical; no silent unrelated edits.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H47 — Touch and controls agreement
- **Status:** PASS
- **Scope:** Expanded
- **Area:** UI / Touch
- **Steps:** Open a long list and a modal; try touchscreen scrolling/selecting and controller equivalent.
- **Expected:** No modal layering, double-activation, dead zones or unintended background selection.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

### H48 — Exit and relaunch after discard
- **Status:** PASS
- **Scope:** Expanded
- **Area:** UI / Exit
- **Steps:** Discard staged edits, leave editor, exit Homebrew, relaunch and reopen same save.
- **Expected:** No crash; save instances load; discarded edits absent; no unsolicited save writes.
- **Game / emulator:** Not entered
- **Notes:** None
- **Severity if FAIL:** Not classified
- **Screenshot / video:** None

## Extra bugs and requests

Gen 1 bag items, fill all doesn't fill all bags, only does current bag not tm or items


leaf green and fire red on gba so gen 3 dont have a working met location for pokemon, i dont think the proper species for those games in in and i thik it is reading RSE for species, like i dont think i can get a rayquaza in leaf green or fire red.

gen 5 quick view graph or whatever we call it, the graph when you are in a save in the box and can see all the pokemon in the box moving between pokemon the graph on the quick view doesn't work and isn't updated with the graph that is in the editor and view mode, it also looks smaller in the quick view. 

is it possible to change the contorls in the save editored when in the box, can we get rid of the X to add and change it to import/export instead like if the cursor is on a pokemon X would say export but it its on a blank space it would say import, makes this app wide for all gens and change the footer as needed, and change the help menus app wide as need as well. 

The quit option in the nro has a highlighted option in the Y exit option, it also has a footer which isn't needed cause the buttons we need are the options in the menu, can we also get rid of "stay" we have cancel in that menu and those 2 options are the same just different names. In the species picker instead of having the 2 pictures one shiny and one normal can we have it where its 1 photo and if we can shiny or not it changes the photo, instead of the 2 photo thing. Gen 1/2 have moves when editor is opened, it should be blank, same as for gen 1-5 when a pokemon is selected it should leave moves clear not add or fill the moves just cause that pokemon was selected in species selector


gen 1/2 have moves when pokemon editor comes up, pikachu has growl and thunder shock i think, we need to have a blank canvas, so no moves, and same for when a pokemon/species is selected we shouldn't have the moves auto filled, that should be left for the user, also can we add the feature that gen 5 has where if we try to ADD a pokemon that doesn't have a met location it wont add and we have a popup in the middle bottom to tell us what we are missing so we can ad the pokemon this should be a universal rule.

gen 2/3 doesnt have 'move or review pending changes in the action menu can we make sure all gens have the same options in the actiion menu, they should all have "view, edit,clone,move,move to vault, release, origin and legality, export to file, reviewing pending changes, cancel"

pkse has a nice search function in the bank where it highlights the pokemon with the searched letter or the item  could we add just before 1.0.

for all these i wanna hear your take on them and not just right me a prompt check out the problems or fix and see how it would work or whats wrong or if we on the same page together, i dont just wanna fuck shit up i wanna hear your take

## Evidence notes

- This worksheet does not claim that legality PR #314 is present in the NRO.
- Not tested is not counted as a failure. Unmarked means no result selected.
- Original emulator saves should not have been overwritten.
