# Gen I–IV Hardware Test Checklist (one combined NRO)

Use the exact candidate NRO named in `GEN14_FINAL_COMPLETION_PROGRESS.md` (check its SHA-256
first). Hardware acceptance stays **PENDING** until you complete this list.

**Before you start:** copy your real saves to a PC as a safety net. Then, for one save per
generation, write down its SHA-256 or size+date. Section F re-checks them.

Mark each line ✅ / ❌ and add a note or screenshot for any ❌.

---

## A. Launch, artwork and Home (test once)

1. NRO starts. Home shows a **region backdrop** behind the selected game (not a flat colour).
2. Cycle the selected game across Kanto, Johto, Hoenn and Sinnoh titles. Each one shows its own **region scenery**, **cover art** and **trainer portrait**, with no blue box behind the portrait.
3. Party preview shows real Pokémon **sprites**, names and levels. Shiny Pokémon show the shiny sprite or marker.
4. Pokédex progress shows numbers for the save.
5. Settings → Diagnostics shows Version, Build, Theme and **Verified exports** folder.
6. Switch the theme between dark and light, and use any variants. Text stays readable. Restart the app and confirm the theme is remembered.

## B. Games / Quick Games / Profiles (test once)

1. **A** on Games opens the classic game list. **Y** opens Quick Games. **B** closes it. **Y** does not close it unexpectedly.
2. Quick Games:
   - drag the drawer with a finger, including reversing direction mid-drag; it settles smoothly
   - scroll the tiles vertically
   - tapping a tile opens the right game
3. Switch to a second Switch profile.
   - The first profile's assigned saves are **not** visible on it.
   - Assign a save to the second profile, switch back, and confirm the first profile is unchanged.
4. Exit an editor. The Home or Game list returns to the **same game** you left.

## C. Save Instances (sample per provider you actually use)

1. RetroArch: Gen I, II and III saves are listed with clear provider names.
2. DraStic or melonDS: Gen IV saves are listed.
   - If your RetroArch saves folder is large, Gen IV DraStic/melonDS saves must **still appear**. This is the new per-provider scan budget.
3. mGBA or Tico, if used: a Gen III save is listed.
4. If you keep a backup copy of a save, it is distinguishable from the original.
5. Assign Save to Profile, then reopen the app. The assignment is remembered.
6. Rename or move a save file on the SD card, then reopen. The app shows Missing/relink rather than opening a different save.
7. A corrupt or truncated file (a copied junk `.sav`) is rejected and not offered.

## D. Editors — one pass per generation

Run the full list on **one game per generation**:
- **Gen I:** Red/Blue, plus Yellow for the Pikachu/starter path.
- **Gen II:** Gold/Silver, plus Crystal for its native fields.
- **Gen III:** Emerald, plus FireRed.
- **Gen IV:** Platinum, plus HeartGold.

For the other games, just open the save and view Party, Boxes and Trainer.

1. View Party, Boxes and Trainer. Summary shows the right stats, moves, item and ball icon, plus the generation's native fields:
   - Gen I: DVs and Stat Exp; no Nature or Ability
   - Gen II: gender, Friendship, Pokérus, Held Item
   - Gen III/IV: Nature, Ability, IVs/EVs, Met Location and Met Level
2. **Create** in an empty box slot:
   - the editor opens without forcing the species picker
   - the species picker shows Dex numbers
   - **Y** toggles shiny where supported
   - Level is set through the numeric keyboard
3. **Edit** a Pokémon:
   - nickname
   - level/EXP
   - moves: the picker appears in front of the menu and shows PP/PP Ups
   - Held Item, using the compact selector
   - Nature (Gen III/IV), using the compact selector
4. Cancel any picker with **B**. Nothing changes, and the cursor returns to the same row.
5. Release: **A** does nothing in the confirmation. **Y** releases from the staged copy only. **B** cancels.
6. Bag/Items: change a quantity, add an item, cycle every pocket with **L/R**, including empty pockets.
7. Gen IV only: pick up a Pokémon with **Y**. The source slot looks empty, and **B** cancels the move.

## E. Staged Review, Export, Discard (once per generation you edit)

1. Open **Review**. Gen IV lists the changed slots with original → staged values.
2. Press **X — Export Copy**. A status message shows the folder:
   - `exports/gen3/...` or `exports/gen4/...` for Pokémon edits
   - the existing Gen I/II export paths
   - Bag exports through Items
3. On a PC, the export folder holds `original_backup`, `edited` and `EDIT_MANIFEST.txt`. Load `edited` in your emulator, and your change is present.
4. In Review press **Y** once. A *"Discard all staged changes?"* confirmation appears. **B** keeps your edits.
5. Press **Y** twice. Every staged edit is gone.
6. Spam **B** out of any editor. No staged edit is lost without a confirmation.

## F. Source safety (once at the end)

1. The original emulator saves have the **same SHA-256 or size+date** as before you started.
2. Launch a game from PokeBank:
   - RetroArch, DraStic/melonDS, and mGBA/Tico if used, each launch the **right** game
   - a ROM that is missing or renamed to a different Pokémon game gives a clear message and does not launch the wrong game
   - the app does not freeze when you select Launch

## G. Controls (spot-check while doing D)

- D-pad and left stick both move every list and grid.
- Touch works in each place below. A single tap never deletes, releases or overwrites anything:
  - Home, Games, Settings and Save Instances
  - Party, Boxes and Summary
  - every picker
  - numeric keyboards
  - confirmation dialogs
- Focus colour is teal, with no bright-red focus borders.
