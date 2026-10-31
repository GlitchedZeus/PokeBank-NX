# Gen VIII native lane (Sword/Shield first)

Thread 3 lane. Branch `claude/gen8-native-20261015`, created from `claude/gen7-native-20261014`
at `2438b5d`. The design and the read-only audit are in [SWSH_RESEARCH.md](SWSH_RESEARCH.md).

## Rules for this lane

- Input bytes come only from an app-owned copy, either the existing `backupSaveData`
  workspace copy or a manually linked file.
- No code here mounts a save, commits a device, calls a legacy `saveTrainerInfo*` writer or
  injects into an installed game. CI greps `src|include/Integration/Gen8` for those APIs and fails if any appear.
- Output goes only through `Integration/Common/VerifiedStagedExport`, rooted at `exports/gen8`.
- BDSP stays read-only and unsupported for export until the authoritative-file question is
  answered from cited evidence.

## Status

| Unit | State | Files |
|---|---|---|
| SwishCrypto + SCBlock container: strict decrypt, byte-identical encrypt | done | `Gen8SwishCrypto.h/.cpp` |
| PK8 codec (Decrypt8/Encrypt8, Add16 checksum, sanity) | done | `Gen8PK8.h` |
| Strict SWSH `main` parse: size set, hash, required blocks, version, party/boxes, trainer, battle teams | done | `Gen8SaveLayout.h`, `Gen8Save.h/.cpp` |
| Staged PK8 Edit/Clone/Remove, touched-block re-encryption, block-set verifier | next | |
| Legends: Arceus | after SWSH | |

## Cited references

| Reference | Revision | Used for |
|---|---|---|
| PKHeX | `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` | `SwishCrypto.cs`, `SCBlock.cs`, `SCXorShift32.cs`, `SCTypeCode.cs`, `SaveUtil.cs`, `SaveBlockAccessor8SWSH.cs`, `SAV8SWSH.cs`, `Party8.cs`, `MyStatus8.cs`, `BoxLayout8.cs`, `TeamIndexes8.cs`, `PokeCrypto.cs`, `G8PKM.cs`, `GameVersion.cs` |
| PKSM-Core | `aa22d7a4` (`vendor/PKSM-Core`) | Differential tests: `crypto_swsh.cpp` (`verify`, `sign`, `applyXor`, `getBlockList`) and the header-only PK8 crypt |

## Decisions recorded

- **Strictness.**
  - Unknown SCBlock type codes, an array subtype without a size, a length that runs past the payload, trailing bytes and duplicate keys are all rejected.
  - `Bool3` is accepted as a standalone type, as PKHeX does; PKHeX only asserts against it in debug builds.
- **Required blocks.** MyStatus, Party and Box must be byte payloads (`Object`, or an `Array` of `Byte`) and at least their cited minimum length.
- **Party slots.** Declared party slots must decode as occupied PK8.
- **Quarantine.** Undeclared party slots and box slots that fail sanity or checksum are counted in diagnostics, not fatal, and are never written over.
- **Battle teams.** Read from `TeamIndexes8` and `TeamLocks`. If either block is missing or references a slot outside storage, `teams()` is empty. The staged session will then refuse storage-changing operations (Clone/Remove), because it cannot prove they leave every team intact.
- **Companion files.** For the exported SWSH folder (coordinator default), `main` is exported, and `backup`/`poke_trade` are to be carried byte-identical. `VerifiedStagedExport` currently publishes one edited file plus the original backup. Carrying companion files needs a shared hook, requested in `integration-requests/01-export-companion-files.md`. Until it lands, their SHA-256 values are recorded in the manifest change lines and they are not copied.
