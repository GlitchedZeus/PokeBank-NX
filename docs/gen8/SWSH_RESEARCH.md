# Gen VIII (Sword/Shield, BDSP, Legends: Arceus): audit and staged/export design

Status: **research only, no code.** Thread 3, 2026-10-15, written for the coordinator.
Every format constant cites the pinned PKHeX revision
`6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (*PKHeX*, paths under `PKHeX.Core/`), the
pinned PKSM-Core `aa22d7a4` (`vendor/PKSM-Core`), or code already in this repository
at `claude/gen7-native-20261014` (based on `claude/gen5-integration-20261014`).
Repository line numbers are as of that head. No file outside `docs/gen8/` was changed.

## 1. How installed Switch saves are read today (and whether that is read-only)

| Step | Code | Effect on the installed save |
|---|---|---|
| Enumerate | `SaveSelectScreen::scanSaveSpace` (`src/UI/SaveSelectScreen.cpp:1334-1385`): `fsOpenSaveDataInfoReader` on `FsSaveDataSpaceId_All` (fallback `User`, `SdUser`), filtered to `FsSaveDataType_Account` + the selected `AccountUid`, then `getGameVersion(titleId)` and `findSwitchGame(titleId)` | **None.** The info reader lists save metadata only; nothing is mounted. Title identity comes from the system's `application_id`, not a guess. |
| Preview | `src/UI/SaveSelectScreen.cpp:1666-1718`: `fsdevMountSaveData("pbpreview", …)`, then `Save::validateTrainerSaveForOpen` / `readTrainerInfo` on `pbpreview:`, then `fsdevUnmountDevice` | **Reads only.** The device is mounted and read. No write, and no `fsdevCommitDevice`. |
| Back up | `backupSaveData` (`src/Utils/FileUtilities.cpp:280-346`): `fsdevMountSaveData("save", …)`, then `copyDirectoryTransactional("save:/", <workspace backup dir>)`, then unmount | **Reads only.** Files are copied *out* of `save:/` into an app-owned folder from `PokeBank::Paths::workspaceBackupPath`. |
| Persist edits | `saveTrainerInfoSwSh` / `…LA` (`src/Save/GetSaveFileContents.cpp:789-830`, `924-960`): re-serialize, `encrypt`, then `persistWorkspaceFile` (`DurableFile::replace` with `validateSCWorkspace` / `validatePLAWorkspace`) | Writes to the **app-owned backup folder** only. |
| Inject | `restoreBackupToTitle` (`src/Utils/FileUtilities.cpp:375`), the only `fsdevCommitDevice` call site (`:456`), reached only when `injectToTitle` is true | **Blocked.** Line 378 returns false before mounting: `canWriteTo(SaveDestination::LiveGame)` is false because `LIVE_SAVE_WRITES_ENABLED = false` is a compile-time constant (`include/Safety/WritePolicy.h:18`). `test_write_policy.cpp` and `test_source_mutation_policy.cpp` guard it. |
| BDSP persist | `saveTrainerInfoBDSP` (`GetSaveFileContents.cpp:909-922`) | Fails closed. `SaveData.bin` and `Backup.bin` need a multi-file journal, so nothing is written. |

**Conclusion.** The installed-save path is read-only:
- Every mount that can reach an installed save either reads or copies out.
- `rg 'fsdevCommitDevice|fsdevMountSaveData|fsOpenSaveDataFileSystem'` over `src include` finds no other mount or commit site.
- The single commit site sits behind the policy constant.

Two residual risks for a Gen VIII lane:
1. The legacy `saveTrainerInfo*` writers still *accept* an `injectToTitle` argument. A new lane must not call them. It must build its own working copy instead (section 4).
2. The legacy writers replace the file in the workspace backup folder in place (`DurableFile::replace` keeps the previous generation). In a native lane, the original app-owned copy must stay byte-identical and the output must be a separate export, as in the Gen VI/VII lanes.

## 2. Formats (cited)

### 2.1 SWSH: SwishCrypto + SCBlock (`main`)

| Fact | Value | Source |
|---|---|---|
| File | `main` (plus `backup`, `poke_trade` copied alongside by the legacy flow) | `GetSaveFileContents.cpp:183-186`, `:821` |
| Accepted sizes | `0x1716B3` (1.0), `0x17195E`, `0x180B19`, `0x180AD0`, `0x1876B1`, `0x187693`, `0x187668`, `0x18764A` (patch histories) | PKHeX `Saves/Util/SaveUtil.cs` `SIZE_G8SWSH*`, `IsSizeGen8SWSH` |
| Identity | size in the set above **and** `SwishCrypto.GetIsHashValid` | PKHeX `SaveUtil.cs` `IsG8SWSH` |
| Integrity | SHA-256 over `IntroHashBytes ‖ payload ‖ OutroHashBytes`, stored as the last `0x20` bytes | PKHeX `Saves/Encryption/SwishCrypto/SwishCrypto.cs` (`SIZE_HASH`, `ComputeHash`, `GetIsHashValid`) |
| Payload crypt | `StaticXorpad` applied in strides of `xp.Length - 1` (0x7F) | PKHeX `SwishCrypto.cs` `CryptStaticXorpadBytes` |
| Blocks | Each block is a `u32` key, then a type byte XOR'd with an `SCXorShift32(key)` stream. `Object` adds a u32 length; `Array` adds a u32 count and a subtype; value types are fixed size. | PKHeX `SCBlock.cs` `GetTotalLength`, `WriteBlock` |
| Keys used for staging | MyStatus `0xF25C070E`, Party `0x2985FE5D`, Box `0x0D66012C`, BoxLayout `0x19722C89`, Item `0x1177C2C4`, Misc `0x1B882B09`; Zukan + R1/R2 | PKHeX `Saves/Access/SaveBlockAccessor8SWSH.cs` |
| Record | PK8: stored `0x148` (8 + 4×0x50), party `0x158`. `Decrypt8` shuffles by `(EC>>13)&31 % 24` with the Gen VI/VII LCG; party stats are crypted after `0x148`. Checksum is Add16 over `[8, 0x148)` at `0x06`. | PKHeX `PKM/Util/PokeCrypto.cs` (`SIZE_8STORED`, `SIZE_8PARTY`, `Decrypt8`), `PKM/PK8.cs` |
| Second reference | PKSM-Core implements SWSH independently: `include/sav/Sav8.hpp`, `SavSWSH.hpp`, `source/utils/crypto_swsh.cpp`, `include/pkx/PK8.hpp` | vendor tree |

There is **no** per-block CRC and **no** signature. The whole-file SHA-256 is salted with
constants published in both references. So unlike SAV7, a SWSH export is fully
re-derivable from cited material.

### 2.2 Legends: Arceus (`main`)

| Fact | Value | Source |
|---|---|---|
| Sizes | `0x136DDE`, `0x13AD06` | PKHeX `SaveUtil.cs` `SIZE_G8LA*` |
| Integrity | same SwishCrypto as SWSH | PKHeX `SaveUtil.cs` `IsG8LA` |
| Keys | Party `0x2985FE5D`, Box **`0x47E1CEAB`**, BoxLayout `0x19722C89`, items `0x9FE2790A`, `0x59A4D0C3`, `0x8E434F0D`, `0xF5D9F4A5`, favourites `0x00EF4BAE` | PKHeX `Saves/Access/SaveBlockAccessor8LA.cs` |
| Record | PA8: stored `0x168` (8 + 4×0x58), party `0x178`, `Decrypt8A`, Add16 over `[8, 0x168)` | PKHeX `PokeCrypto.cs`, `PKM/PA8.cs` |
| Second reference | **none** in PKSM-Core `aa22d7a4` | vendor tree |

### 2.3 BDSP (`SaveData.bin` and `Backup.bin`)

| Fact | Value | Source |
|---|---|---|
| Sizes / revision | `0xE9828` (1.0), `0xEDC20`, `0xEED8C`, `0xEF0A4`; u32 at offset 0 must equal the matching `Gem8Version` | PKHeX `SaveUtil.cs` `SIZE_G8BDSP_*`, `IsG8BDSP` |
| Integrity | MD5 of the **whole** file with the 16-byte hash at `0xE9818` zeroed first | PKHeX `Saves/SAV8BS.cs` `SetChecksums` (`HashOffset = SIZE_G8BDSP_0 - 16`) |
| Layout | flat, no blocks: Items `0x0563C`, Party `0x14098`, BoxLayout `0x148AA` (`0x64A`), Box `0x14EF4`, MyStatus `0x79BB4`, Zukan `0x7A328` | PKHeX `SAV8BS.cs` constructor, `Initialize` |
| Record | PB8 = PK8 sizes (`0x148`/`0x158`) and crypto | PKHeX `PKM/PB8.cs` |
| Second reference | none in PKSM-Core | vendor tree |

## 3. What the repository already has (PKSE-derived, other owners)

| Area | Files | Notes for a native lane |
|---|---|---|
| SwishCrypto | `src/Encryption/Encryption.cpp` (`cryptStaticXorpadBytes`, `tryDecrypt` with `HashMismatch`/`MalformedBlocks`, `encrypt`, `computeHash`), `include/Save/Block.h`, `src/Save/Block.cpp`, `src/Enums/SCTypeCode.cpp` | The decrypt is strict: the hash is checked first and must consume the whole payload. It has not yet been tested against PKSM-Core `crypto_swsh.cpp`. |
| PK8/PB8/PA8 crypt | `include/Encryption/Encryption8SWSH.h` (sizes match PKHeX), `Encryption8BDSP.h`, `Encryption8LA.h` | Reuse as-is, or differential only. |
| Records | `include/Pokemon/Pokemon8SWSH.h`, `Pokemon8BDSP.h`, `Pokemon8LA.h` | Accessor classes; the stat model needs its own audit (BDSP/LA cached stats). |
| Trainers | `include/Trainer/Trainer8SWSH.h` (keys match PKHeX), `Trainer8BDSP.h` (flat offsets + whole-file MD5 `recomputeHash`), `Trainer8LA.h`, `Inventory8*` | These mutate in place and feed the legacy writers. |
| Read validation | `include/Save/SCReadValidation.h` (`validateSWSH`: required keys, types, minimum sizes, per-slot decrypt + checksum + domain, unique keys), `PLAReadValidation.h`, `BDSPReadValidation.h` (`hashOffset 0xE9818`) | Read gates for the legacy open. They are **not** a byte whitelist for export. |
| Tests | `tests/test_sc_read_validation.cpp`, `test_pla_read_validation.cpp`, `test_bdsp_layout_guard.cpp`, `test_write_policy.cpp`, `test_source_mutation_policy.cpp` | No differential test against PKSM-Core SWSH yet. |

## 4. Proposed Gen VIII staged/export design (Gen VI/VII pattern)

The lane would be `include|src/Integration/Gen8/**`, `tests/test_gen8_*`,
`tests/fixtures/gen8/**`, `Makefile.host.gen8`, `.github/workflows/gen8-focused-host.yml`
and `docs/gen8/**`. It does not touch shared UI, `Integration/Common` or the legacy
`Save/`, `Trainer/` or `Encryption/` code. Hooks would be requested under
`docs/gen8/integration-requests/`.

1. **Source.** The input is the app-owned copy produced by `backupSaveData`
   (`copyDirectoryTransactional` out of `save:/`) or a manually linked file. The lane
   never mounts `save:/` itself. It receives bytes from the existing read-only path,
   and the original copy is never rewritten.
2. **Strict parse (SWSH first).**
   - Check the size against the `SIZE_G8SWSH*` set, then the SHA-256 (`computeHash`).
   - Decrypt and require that block parsing consumes the whole payload and that keys are unique.
   - Require the staging keys with their exact types and lengths.
   - Decode every party/box PK8 with checksum and sanity checks.
   - Run this as a differential test against PKSM-Core `Sav8`/`crypto_swsh.cpp` on a synthetic save built from the published salts and xorpad. No real `main` is committed.
3. **Staged session.** Same API as Gen VI/VII: `SlotRef`, `stageValue`, `stageText`,
   `cloneToBox`, `removeFromBox`, `bag()`. Edits are applied to a decrypted block list;
   the source byte vector stays intact.
4. **Working copy.** Serialize only the touched blocks (Party, Box, Item, plus BoxLayout
   if names are staged) and re-encrypt them with `SCXorShift32`. Every other block is
   kept **byte-identical, including its encrypted bytes**. Then re-apply the xorpad and
   recompute the SHA-256.
5. **Verifier.** Strict reparse of the candidate, then compare **decrypted** block sets:
   - Same keys, order, types and lengths.
   - Untouched blocks are byte-equal.
   - In touched blocks, only bytes within the staged record ranges may change, under the per-record editable-field mask; bag slot ranges are verified as in `bagDiffWithinSlots`.
   - Party stats are verified only as an exact recomputation.

   Because SwishCrypto has no per-block checksums, the whitelist works on decrypted
   content. The SHA-256 is verified by the reparse.
6. **Export.** Output only through `Integration/Common/VerifiedStagedExport`, root locked
   to `exports/gen8` under `__SWITCH__`. There is no inject path, and the lane never calls
   `saveTrainerInfoSwSh` or `restoreBackupToTitle`.
7. **Editor.** A `Gen8ExactFormatEditorProvider` registers SWSH in the shared editor
   (identity rows already exist: `sword_switch`/`shield_switch`, `src/Games/GameIdentity.cpp:39-40`). No new shell.
8. **Order and scope.**
   - **SWSH:** strict parse → staged party/box → bag → box names/Zukan readouts.
   - **LA:** same container. PA8 has no second reference, so it is PKHeX-only and
     should land behind SWSH.
   - **BDSP:** last. Whole-file MD5 is simple, but the save is two files
     (`SaveData.bin` and `Backup.bin`). A *verified export* of a single file avoids the
     multi-file journal problem, because the export never replaces the originals. Which
     of the two files is authoritative is not documented in either reference and stays
     **unsupported** until confirmed.

## 5. Open questions (owner/coordinator)

1. Approve the Gen VIII lane and the file ownership above, and say whether it belongs to Thread 3 or another thread.
2. Confirm that the lane takes bytes from the existing `backupSaveData` copy (so the lane needs no mount code), or name another hand-off.
3. For SWSH, confirm that exporting `main` alone is sufficient. The legacy flow also copies `backup` and `poke_trade` (`GetSaveFileContents.cpp:821`); their roles are not documented in PKHeX `6501f0ab`, so they are carried through untouched and never regenerated.
4. BDSP authoritative-file question (section 4, item 8).
