# Gen V production completion — progress checkpoint (Thread 2)

Branch `claude/gen5-integration-20261014` (draft PR #307), base
`claude/gen14-final-20261014` @ `4b9b9f65`. Work started from
`a17c503ad272cc55e69b8b3cbd925cec999567cc` after the coordinator confirmed
ownership. Forward commits only; no history rewritten.

Physical Switch status of every NRO built from this branch: **NOT DEVICE TESTED**.

## Status by step

| Step | Software | Automated verification | Hardware |
| --- | --- | --- | --- |
| 1. Bag in shared Items screen + Review + export | Done | `test_gen5_bag`, `test_gen5_bag_screen`, UI contract | Not tested |
| 2. Party/Box parity with Gen IV + fields | Done (Move Pokemon, read-only origin fields) | `test_gen5_party_box_parity` | Not tested |
| 3. Trainer (Name / Money / Play Time) | Done; other rows read-only | `test_gen5_trainer` | Not tested |
| 4. Pokédex | Read-only view done | `test_gen5_trainer` (dex section) | Not tested |
| 5. Source assignment / copy choice e2e | Tests done | `test_gen5_source_e2e` | Not tested |
| 6. Native record validation | Tests done | `test_gen5_export_validation` (572 randomized exports) | n/a |
| 7. Housekeeping | `patches/gen5-create` removed (verified live) | — | n/a |
| 8. Integrated Gen I–V candidate | Workflow `gen15-integrated-candidate.yml` | CI on the exact head | NOT DEVICE TESTED |

## What the user can do now (Gen V, all four titles)

- **Items (Bag):** from the Trainer panel press **Y** (or tap the "Y Bag" footer), or use
  Product Home → Items, which runs the same explicit Save Instance chooser. Pockets: Items,
  Key Items, TMs & HMs, Medicine, Berries (L/R or the header arrows). **A** or a second tap on
  a row: quantity keypad. **X**: Add picker (native, released items not already held).
  **Y** then **Y**: Remove. Key Items and TMs/HMs are view-only. An unrecognized bag shows
  "Bag unavailable" and nothing is guessed. **+** opens Review.
- **Trainer:** **A** on the Trainer panel opens the field list. Name (1–7 printable ASCII),
  Money (0–9,999,999) and Play Time (to 999:59:59) are staged. TID, SID, gender, badges and
  language are read-only. The Pokédex row opens a read-only seen/caught list.
- **Boxes:** Pokémon Actions now include **Move Pokemon** (Gen IV parity), alongside View,
  Edit, Clone and Remove. Party keeps View/Edit/Clone.
- **Review** lists staged Pokémon, each Bag change (added / removed / quantity) and each
  Trainer change. Export writes `original_backup.sav`, `edited.sav` and a manifest that
  records `BAG_CHANGED` and `TRAINER_CHANGED`. The source save is never written.

## Citations

- Bag: PKHeX `6501f0ab` PlayerBag5BW/B2W2, ItemStorage5*, SaveBlockAccessor5* (block 25).
- Trainer: PKHeX `6501f0ab` SaveBlockAccessor5BW/B2W2 (PlayerData block 27, Misc block 52),
  PlayerData5.cs, Misc5.cs, SAV5.cs (`MaxStringLengthTrainer` 7), SaveFile.cs
  (`MaxMoney` 9,999,999), StringConverter5.cs. The 999-hour cap is app policy, not a format rule.
- Pokédex: PKHeX `6501f0ab` Zukan5.cs (BW block 55, B2W2 block 54; caught at 0x08, seen at
  0x5C, four 0x54-byte regions).
- PK5 origin fields: PKSM-Core `aa22d7a` `source/pkx/PK5.cpp` (0x78–0x7E, 0x85).

## Open items / limits

- If a source save's party stat cache or EXP disagree with its level (retail saves never do),
  staging a stat field and then reverting it makes that record fail export certification.
  This fails closed (export refused, nothing written); Discard restores it.
- PR #280's Gen III/IV empty-slot touch parity tests live in Thread 1 files
  (`tests/test_gen3_hardware_surface_contract.cpp`, `tests/test_gen4_hardware_surface_contract.cpp`,
  plus behaviour in the Gen III/IV surfaces); not ported here.
- Legality-session handoff `claude/gen5-handoff-from-legality-20261014` @ `fc519414`:
  reimplemented rather than merged. Its two changes (items from the staged/source bag; BW/B2W2
  pocket names) are covered by commit `b9c8d8f`.
- Gen V reuses the Gen IV DraStic launch path; automatic return to PokeBank NX is **not** claimed.
- The candidate NRO identity (code SHA, tree SHA, size, SHA-256, CI run IDs) is recorded in a
  PR #307 comment once CI finishes on the final head.
