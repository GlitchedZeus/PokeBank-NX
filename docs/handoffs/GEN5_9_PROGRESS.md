# Gen V production completion — progress checkpoint (Thread 2)

Branch `claude/gen5-integration-20261014` (draft PR #307), base
`claude/gen14-final-20261014` @ `4b9b9f65`. Work started from
`a17c503ad272cc55e69b8b3cbd925cec999567cc` after the coordinator confirmed
ownership. Forward commits only; no history rewritten.

Physical Switch status of every NRO built from this branch: **NOT DEVICE TESTED**.

## Hardware failure of candidate `fa9cfcea` and the repair (2026-10-15)

The owner tested `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe` on a Switch and Gen V **failed
hardware acceptance**. Gen V is **not** complete and **not** hardware-ready. Diagnosis and fixes:

| # | Failure seen on hardware | Root cause | Fix (commit) |
| --- | --- | --- | --- |
| 1 | Opened straight into Party, no hub | `UI.cpp handleGen5View` forced `selectedMode=Party; detailViewActive=true` | Opens on the shared loaded-game hub; `Gen5HubRouting.h` decides the only Gen V intercepts (`7123035`) |
| 2 | Create showed only Cancel | `emptyCloneDestination` required raw zero bytes; retail empty slots hold the *encrypted* blank (PID 0, checksum 0 → non-zero keystream; PKHeX `SaveFile.ClearBoxes` writes `BlankPKM`), so Add was never offered. The provider also lacked `PokemonCreation` | `nativeBlankSlot5` accepts zeros or the encrypted blank; Remove now writes the retail blank (`3cbf368`) |
| 2b | Create was a ~30-prompt wizard | Separate Gen V UI | Create opens the SAME Edit draft/three-panel workspace on Species; nothing persists until Keep; Discard restores the native blank (`80bfce6`) |
| 3 | Real Black 2 DraStic `.dsv`: "Primary copy / Party 0 / Boxed 0", name "(unsupported glyphs)", Money 0, TID/SID 0, Male, Dex 0 | The selected copy was checksum-valid but blank/initial (version byte only). The Save Instances/Home card path (`inspectSnapshot`) never ran the blank-copy gate, so it presented zeros. Synthetic reproduction in `test_gen5_save_diagnostics` | One gate (`unopenableCopyDiagnostic5`) for Save Instances, Home card and open: the copy is shown NOT OPENABLE with the reason, the other copy is offered explicitly, never swapped silently (`395e2a6`, `b9c1ea8`). **Not yet confirmed against the owner's file** – see on-device diagnostics below |
| 4 | Male portrait for a female trainer | Unknown/blank gender fell back to Hilbert/Nate; the blank copy (gender byte 0) fed the card | Gender only from an openable copy; unknown shows a neutral "Unova Trainer"; hilbert/hilda/nate/rosa are required RomFS portraits (`b9c1ea8`) |
| 5 | Separate "Trainer – Generation V" pop-up | Gen V-only modal | Trainer is drawn in the shared content rect/HOME card; read-only rows labelled; scrolling; focus memory; B returns to the hub (`7123035`) |
| 6 | General parity | Gen V owned every input | Hub, Pokémon Actions, shared Create/Edit, Items, Review/Export routes now follow Gen IV; Legacy Storage (Bank) is refused with a status because no PK5 bank conversion exists |

### On-device save diagnostics
Save Instances → Gen V candidates → **+ Save Diagnostics** writes
`sdmc:/switch/PokeBank-NX/diagnostics/gen5-<game>-<timestamp>.txt` (exclusive create, never
overwrites, source opened read-only). It lists per copy: block checksums valid/total, version
byte, gender byte, whether TID/SID and a name are present (no values), party count and record
validity, occupied / encrypted-blank / zero / invalid box slots, dex seen/caught, play time and the
copy state (`valid, populated` / `no trainer data (blank or initial copy)` / `checksums invalid` /
…). It contains no names, IDs or save bytes. This is how the Black 2 `.dsv` case is confirmed.

### Shared files touched (Gen V branches only; Gen I–IV behaviour unchanged)
`src/UI/SaveSelectScreen.cpp` (Gen V portrait fallback, Gen V card label, diagnostics key),
`src/UI/UI.cpp` (`handleGen5View` only), `tools/verify_nro_romfs.py` (+4 Gen V portraits).
No Gen IV source file was changed.

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

## What the user can do now (Gen V, all four titles) — as of the repair
- Open → the same hub as Gen I–IV: Pokémon (Boxes), Party, Storage (refused for Gen V), Items,
  Trainer. B on the hub exits through the staged-changes confirmation; + opens Review.
- A on a slot → Pokémon Actions. Add on an empty slot → shared editor (DETAILS / STATS / MOVES).
  L/R panel, Up/Down field, A picker/keypad, Left/Right step, X Keep/Add, Y Discard, B back.

### Earlier (pre-repair) notes

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
