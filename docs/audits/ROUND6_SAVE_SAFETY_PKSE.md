# Round 6 (7E): save safety compared with PKSE

**Scope:** an audit only. It compares PokeBank NX's backup, staged-edit, export and rollback behaviour with PKSE's, lists the gaps, and changes no write policy. Direct writing stays disabled.

**PKSE reference:** PKSE 1.1.3 at the pinned research commit `1133aedffeeffb5862f35162597e249233903597` (`docs/UPSTREAM_AUDIT.md` §1). That commit is in this repository's history, so every PKSE statement below cites `git show 1133aedf:<path>`.
- The README documents one save-safety feature: "Backup and restore save files, directly on the console."
- Everything else about PKSE's behaviour comes from its source at that commit.

**PokeBank reference:** #307 as merged forward into `claude/launch-return-20261015` on 2026-10-25.

## 1. Behaviour, side by side

| Area | PKSE 1.1.3 | PokeBank NX now |
|---|---|---|
| **Backup on open** | Every Switch-title open copies the save to a new `PKSE/<title>/<timestamp>/` folder. With Auto-Backup Off it copies into the reusable `PKSE/<title>/Working/` instead (`src/UI/UI.cpp:77`, `backupSaveData(..., g_autoBackupEnabled)`; `src/Utils/FileUtilities.cpp:208`). | **No backup on open.** Open refreshes the single Working copy through `WorkingCopy::refresh` (`include/UI/WorkingCopyRefresh.h`). An unchanged earlier copy is dropped; one holding edits is kept as a dated backup. Hub contract and CG60 pin this. |
| **Reopening with Working** | `copyDirectory("save:/", Working)` copies straight over the earlier Working folder (`FileUtilities.cpp:208-258`), so a previous session's saved edits can be overwritten without notice. | The earlier Working copy is moved aside first and compared. If it differs, it survives as a dated backup. A failed refresh puts it back exactly (host test `workingCopyRefresh`). |
| **Deliberate backups** | No separate action: a backup is a side effect of opening. | **Save Backups → "Create backup now"** is the only way a backup is made, for both Switch titles and emulator saves. Emulator copies are read back and compared, with the manifest written last (`SourceBackups::createBackup`). |
| **Copy integrity** | `copyFile` writes, then `fclose`. A partly copied backup folder stays in place and lists as a normal backup. | `copyDirectoryTransactional` copies into `.incomplete.<stamp>`, reads each file back byte for byte (`verifyCopiedFile`), and only then promotes it. Failures are quarantined as `.failed.`, and the replaced generation becomes `.previous.`. None of these three are ever listed. |
| **Where edits go** | The backup folder **is** the editing workspace: Save writes the game file straight into it with `fopen("wb")` and `fwrite` (`GetSaveFileContents.cpp:250-262`, Let's Go; other titles likewise). | Switch-title edits go to the Working copy or the isolated `Opened-Backup` session, never into a historical backup (round 5, #322). Writes use `DurableFile::replace`: validator, unique sibling generation, then promotion (`GetSaveFileContents.cpp:157`). Gen I–V emulator edits stay staged in memory. |
| **Opening an old backup** | A on any backup opens that folder as the workspace, so the next Save edits the historical backup in place. | A opens a **copy**: Switch backups via `openBackupSession`; emulator backups via `openBackupTrainer` from an immutable snapshot, Export Copy only. The backup stays byte- and time-identical (host tests `backupSessions`, `test_backup_snapshot_open`). |
| **Legacy title-only folders** | They are PKSE's native layout, so ownership isn't a concept. | `backups/<title>/` folders from before A08 are listed as "ownership unknown". They open only through an isolated copy, are never deleted, and are never assigned to the current profile. |
| **Writing back to the game** | Save destination "game save" (and every Save in a cart-loaded session) runs `restoreBackupToTitle`. It copies named files to `save:/`, stops before `fsdevCommitDevice` on any copy failure, then commits (`FileUtilities.cpp:293-380`). Gated by the `injectToGame` setting. | **Disabled at compile time.** `injectToGame` is read and ignored (`src/Utils/Settings.cpp`). There is no restore/inject path for Switch titles or emulator saves. Installed-title and emulator sources are opened read-only (`SourceMutationPolicy`). |
| **Emulator saves** | Not supported (Switch titles only). | The original file is opened `"rb"` and never written, moved or deleted. Export Copy (`VerifiedStagedExport`) writes into a temporary record under `exports/genN/`, reads it back and strict-parses it, then promotes it with a manifest naming the exact `SOURCE_PATH` and `original_backup.*`. Gen IV re-checks the source snapshot at export, and Gen V checks that the staged baseline still matches. |
| **Deleting a backup** | X, then **A** confirms (`BackupSelectionScreen.cpp:91-110`): one accidental A deletes the folder. | X/Y, then **only Y** confirms (`HomeQuit::decide`); A and B cancel, and the touch path needs a full tap. Each delete is re-checked on disk: <ul><li>Emulator records: location, manifest, no foreign files, never the last remaining copy.</li><li>Switch backups: last-owned guard and an unchanged directory snapshot.</li><li>Legacy folders: never deletable.</li></ul> |
| **Rollback** | Only by opening an older backup folder, which then becomes the workspace. | <ul><li>Older backups open as isolated copies.</li><li>Each durable save keeps the replaced generation as a `.previous.` sibling.</li><li>Interrupted Pokémon Moves are recovered by the A04b journal before any workspace opens, including the backup session (`openBackupCopySession`, round 5).</li></ul> |
| **Settings file** | `fprintf` straight into `settings.cfg`. | Verified sibling-file transaction (`AtomicTextFile`). A failed save keeps the old file. |

## 2. Where PokeBank keeps PKSE's guarantees

- **Backups stay on the console:** backups, staged edits and exports are all on the SD card under `sdmc:/switch/PokeBank-NX/`.
- **Safe handling of companion files:** PKSE copies only a title's named save files and skips missing optional companions such as Sword/Shield's `poke_trade` (`FileUtilities.cpp:260-292`). PokeBank keeps the same file sets for durable workspace saves, and adds validation before promotion.
- **Backup history is never pruned automatically,** as in PKSE. Deleting is always the user's decision.

## 3. Gaps

| # | Gap | Risk | Decision |
|---|---|---|---|
| G1 | **Gen V exports are not listed in Save Backups.** `Gen5WorkingCopyExport` writes `exports/gen5/account-<profile>/<label>/`, but `SourceBackups::list`, `historicalSources` and `openSnapshot` scan only `exports/genN/<record>/`. A Gen V export is safe on the SD card but can't be found, opened or deleted from the app. | Discoverability only; no data at risk. The record is outside every delete path, so it can't be removed by mistake either. | **Not changed in round 6.** It needs a profile-scoped record shape, matched through `SOURCE_PATH`, `GAME_ID` and the account component, in the listing, open and delete guards together. That is more than a small, clearly safe change; proposed as a follow-up for the backup owner. |
| G2 | **`.previous.`, `.failed.` and `.incomplete.` generations are kept indefinitely and are invisible.** They are recovery evidence by design, but there is no in-app way to recover from one or reclaim the space. | Could slowly use up the SD card; no data is lost. | **Not changed.** Deleting recovery evidence automatically would be less safe than PKSE. A visible "recovery copies" view is a follow-up. |
| G3 | **No restore of a backup to the game or emulator save.** PKSE has one (`restoreBackupToTitle`). | None: this is the deliberate policy ("No live emulator writes, installed-game writes, Inject Save or destructive writeback"). | **Intentional.** Users restore by exporting a copy and installing it with their own tools. |
| G4 | **An Export Copy made from a backup session records that backup as its `SOURCE_PATH`,** so it lists under the backup's recovery history, not under the live save. | None: the export is still listed and openable. Only its grouping differs. | **Documented.** It truthfully records which bytes were exported. |
| G5 | **Gen I–III emulator exports have no Open snapshot equivalent to Gen IV's.** Export rebuilds the original from the staged editor's baseline, and `VerifiedStagedExport` validates the result, but it doesn't re-check that the source file is unchanged since Open. | Low: the source is never written, and the export carries the bytes that were actually edited. | **Documented.** Adding the check would change three generations' exporters (lanes C/S) and isn't needed for safety. |

No gap above lets a live or historical save be written, and no small change was safe enough to make blind, so round 6 makes no save-path changes. Every row in §1 is pinned by an existing host or contract test named there.

## 4. What would change this audit

- Enabling any write to `save:/`, an installed title, or an emulator's own file. That needs a separate owner decision; nothing in round 6 does it.
- Changing the record shape under `exports/` or `backups/` (G1). The listing, open and delete guards must then change together, with the host tests in `tests/test_emulator_backup_access.cpp` and `tests/test_backup_snapshot_open.cpp`.
