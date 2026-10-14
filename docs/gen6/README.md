# Gen VI native save formats (X / Y / Omega Ruby / Alpha Sapphire)

Thread 3 lane, branch `claude/gen6-native-20261014`, started from PR #307 head
`a17c503ad272cc55e69b8b3cbd925cec999567cc`. New files only. The single allowed
shared edit is `include Makefile.host.gen6` in `Makefile.host`.

## What exists

| Layer | Files | Status |
|---|---|---|
| PK6 codec (232/260 bytes): block shuffle, LCG crypt, Add16 checksum, strict decode, byte-identical round trip | `include/Integration/Gen6/Gen6PK6.h` | done; differential vs PKSM-Core |
| SAV6 XY/ORAS identification (size + BEEF footer + all block CRC16-CCITT + MyStatus version), Trainer (read-only), Party, 31×30 Boxes | `Gen6SaveLayout.h`, `Gen6Save.h`, `src/.../Gen6Save.cpp` | done; tables differential vs PKSM-Core |
| Staged Edit / Clone / Remove on an app-owned copy, byte whitelist verifier, verified working-copy export via `Common::VerifiedStagedExport` | `Gen6StagedSession.h/.cpp` | done |
| Create | explicitly **unsupported**: EC/PID, personal data, met, handler, memory and geo fields are not verifiably derivable | by design |
| Shared-editor provider (Editable / ReadOnly / Derived / Hidden per field and per Box/Party region) | `Gen6ExactFormatEditorProvider.h` | done; registration requested |
| Save sources: Checkpoint layout, Dekopon (Switch), manual link; JKSM manual-only; Azahar unsupported | `Gen6SaveSources.h/.cpp`, `EMULATOR_SAVE_PROVIDERS.md` | done; registration requested |
| Host tests (GCC, Clang, ASan/UBSan) | `tests/test_gen6_*.cpp`, `tests/fixtures/gen6/*.h`, `Makefile.host.gen6`, `.github/workflows/gen6-focused-host.yml` | 4 binaries, 1552 checks |

## Pinned references for every layout constant

- PKHeX `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (the same pin as the Gen V work):
  - `PKM/Util/PokeCrypto.cs`: sizes, BlockPosition/Invert, Decrypt67/Encrypt67, CryptArray.
  - `PKM/Shared/G6PKM.cs` and `PKM/PK6.cs`: checksum, field offsets, name spans.
  - `Saves/Util/SaveUtil.cs`: SAV6 sizes and the BEEF footer.
  - `Saves/Access/SaveBlockAccessor6XY.cs`, `SaveBlockAccessor6AO.cs`: block tables.
  - `Saves/Blocks/BlockInfo3DS.cs`: metadata layout, `BlockInfo6` → CRC16_CCITT.
  - `Saves/SAV6*.cs`: Party/Box offsets, box count.
  - `Saves/Substructures/Gen6/MyStatus6.cs`: trainer fields.
  - `Game/Enums/GameVersion.cs`, `PKM/Util/EffortValues.cs`, `PKM/Strings/StringConverter6.cs`.
- PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c` (`vendor/PKSM-Core`):
  `include/utils/crypto.hpp`, `source/utils/crypto_checksums.cpp`,
  `include/sav/SavXY.hpp`, `include/sav/SavORAS.hpp`, `source/sav/Sav6.cpp`,
  and `source/utils/VersionTables.cpp` (max item, ball). The tests compile against
  these directly, so any disagreement fails CI.
- PR #301 (legality lane) was **not** read into or copied from. Nothing here derives from it.

## Field states (shared editor)

`Gen6EditorProvider::fieldState(field, region)`:

- **Editable (Box and Party):** Nickname, Held item, Friendship (OT; handler friendship
  unsupported), Pokérus, OT name, OT gender, TID, SID, Ball, Fateful encounter, IV, EV,
  Nature, Level (staged as the growth-rate minimum EXP).
- **Party records:** any IV/EV/Nature/Level edit recomputes the cached level and battle
  stats (`Gen6Stats.h`). The formula comes from PKHeX `PKM.LoadStats` and PKSM-Core
  `PK6::statImpl`. Base stats come from a table generated from PKHeX `personal_ao`
  (`docs/gen6/tools/gen_gen6_personal.py`), checked row by row against PKSM-Core
  `personal6`. Current HP becomes max HP, as in both references; status is left
  untouched. The verifier requires the exact recomputation.
- **Derived:** Shiny (PID^TID^SID < 16), and calculated stats on party records.
- **ReadOnly:** Species, Form, Gender, Language, EXP, Ability, PID, Origin game, Met/Egg
  data, Moves/PP/PP Ups, move compatibility, legality and provenance.
- **Hidden:** DV, Stat Exp, AV, Effort level, Stat nature.

The staged session enforces the same table. A test drives every field in both
regions and asserts that a field stages only if it is Editable.

## Serialization guarantees

- The source bytes are copied on parse and never modified (asserted in tests).
- A working copy may differ from the source only in:
  1. the staged record ranges, where the decrypted record may differ from its
     baseline only at the byte and bit mask of the editable fields; a Clone equals its
     donor's boxed form (plus later masked edits); a Remove is the encrypted blank;
  2. the u16 CRC of each block that owns a staged record (Party block 18, Box block
     53/56).
- The result must strictly reparse as the same exact game with the same party count.
  The exporter re-runs this verifier on the file read back from disk.
- On Switch the export root is locked to `exports/gen6`.

## Percentages (three separate columns)

| Area | Software | Automated verification | Hardware acceptance |
|---|---|---|---|
| PK6 format | 100% | 100% (host, GCC/Clang, ASan/UBSan, PKSM differential) | 0%: NOT DEVICE TESTED |
| SAV6 format (Trainer/Party/Boxes) | 90% (Bag, Pokédex and box names not parsed) | 90% | 0% |
| Staged Edit/Clone/Remove + export | 90% (Create unsupported by design; party Remove unsupported) | 90% | 0% |
| Shared editor plug-in | 60% (provider done; UI registration pending Thread 2) | provider 100%, UI 0% | 0% |
| Save sources | 60% (model and validation done; Save Instances registration pending) | 70% | 0% |

No NRO contains Gen VI yet: the native `Makefile` (Thread 2) does not list
`src/Integration/Gen6`. See `integration-requests/`.

## Still missing / next

- Shared registration and UI wiring (requests 01–02).
- Bag (MyItem block 1), Pokédex and box names. These need cited pouch tables.
- Real-save confirmation by the owner. Synthetic fixtures only so far.
