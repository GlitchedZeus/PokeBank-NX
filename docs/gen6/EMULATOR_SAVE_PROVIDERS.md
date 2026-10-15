# Gen VI save providers: verified layouts and open questions

Thread 3 (Gen VI native formats). Researched 2026-10-14 using only each
project's own source code at the pinned commit below. Nothing here comes from
forum posts, wikis or guesses. Community pages were used only to *find*
repositories.

The rule implemented in `include/Integration/Gen6/Gen6SaveSources.h`:

- A path is verified from source → provider descriptor `Verified`, discovery looks only
  at the exact files listed here.
- The layout is verified, but nothing writes it to the Switch SD card → `ManualLinkOnly`.
- No verified Switch path → `Unsupported`. The provider is never scanned, and the owner
  checklist below says what to look for.
- In every case the game identity (X / Y / Omega Ruby / Alpha Sapphire) comes from the
  validated save **contents**: save size, `BEEF` footer, all block CRCs, and the MyStatus
  version byte. A title ID in a folder name is only corroborating evidence. If it
  disagrees with the contents, the row becomes `AssignmentMismatch` and is never "fixed".

## Pinned references

| Project | Repository | Commit |
|---|---|---|
| Dekopon (Switch 3DS emulator, Azahar fork) | https://github.com/PalindromicBreadLoaf/dekopon | `dfba28f5e5342310c1ccff5963363be8512862f6` (HEAD, 2026-09-20); tag `v2.1.0-RC1` = `9b0f15f8867af1eaaba4e8de00ef9ef13ea0cbc9` (same paths re-checked) |
| Azahar | https://github.com/azahar-emu/azahar | `68202bc4ff7edf9d7280ec0dcd6572b7cd1ae26e` (HEAD, 2026-10-14) |
| Checkpoint | https://github.com/FlagBrew/Checkpoint | `740f221a853db784401458fe74276977afcfa8b0` |
| JKSM | https://github.com/J-D-K/JKSM | `f883dff8fccbbb90851c3c6c2a16ea8bda250356` |
| PKSM (title IDs, save file name) | https://github.com/FlagBrew/PKSM | `9e18c8068ede315f73a99f4c834819630393b5c5` |

## Game identity facts

- Title IDs come from PKSM `3ds/source/titles/loader.cpp` (`originalIDs`) and
  `common/source/ConfigurationFile.cpp` (version → title):
  X `0004000000055D00`, Y `0004000000055E00`, Omega Ruby `000400000011C400`,
  Alpha Sapphire `000400000011C500`. Version bytes are X=24, Y=25, AS=26, OR=27
  (PKHeX `GameVersion.cs` @ `6501f0ab`).
- The save archive file is `/main` (PKSM `3ds/source/titles/Title.cpp:157`,
  `loader.cpp:542`). Its size is `0x65600` (XY) or `0x76000` (ORAS), per PKHeX `SaveUtil.cs`.

## Dekopon: VERIFIED, Switch build exists

Dekopon has a real Switch frontend (`src/citra_switch/`, built as an NRO).

| Fact | Source (dekopon @ `dfba28f5`) |
|---|---|
| Default user dir `sdmc:/switch/dekopon/` under `__SWITCH__` | `src/common/file_util.cpp:971-976`; `src/citra_switch/config.cpp:34` (`kDefaultUserDir`) |
| User dir can be redirected by `sdmc:/switch/dekopon/user_dir.txt` (whitespace-stripped, trailing `/` added, default value means default) | `src/citra_switch/config.cpp:36-54` (`ReadUserDirPointer`), `src/common/string_util.cpp:58` (`StripSpaces`) |
| `SDMCDir = <user dir> + "sdmc/"` | `src/common/file_util.cpp:1035`, `src/common/common_paths.h` (`SDMC_DIR "sdmc"`) |
| SD save container `<sdmc>Nintendo 3DS/<ID0>/<ID1>/title/` | `src/core/file_sys/archive_source_sd_savedata.cpp:21-23` |
| `ID0 = ID1 = "00000000000000000000000000000000"` | `src/core/hle/service/fs/archive.h:21-24` |
| Per-title save dir `<high:08x>/<low:08x>/data/00000001/` | `archive_source_sd_savedata.cpp:25-29` |
| Built-in Checkpoint-style export to `sdmc:/3ds/Checkpoint/saves/0x%05X <title>/<backup>/` with unique ID `(program_id >> 8) & 0xFFFFF` | `src/citra_switch/save_manager.cpp:22,111-113,171-177` |

The resulting file for **X** under the default user dir is:

```
sdmc:/switch/dekopon/sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000/00055d00/data/00000001/main
```

(Y `00055e00`, OR `0011c400`, AS `0011c500`.) Implemented by
`Sources::emulatorSaveFile()` and `Sources::dekoponUserDir()`. A `user_dir.txt`
that points outside `sdmc:/` (for example USB storage, which Dekopon supports in
`usb_storage.cpp`) is treated as unreachable. The provider then returns nothing
and never guesses.

**Owner check on the SD card:**
1. After saving in-game in Dekopon, confirm that the `main` file above exists and is
   0x65600 (XY) or 0x76000 (ORAS) bytes.
2. Report whether `sdmc:/switch/dekopon/user_dir.txt` exists, and what it says.
3. Report the Dekopon NRO version you run. It was verified at `v2.1.0-RC1` and HEAD
   `dfba28f5`.

## Checkpoint layout: VERIFIED as a Switch-side location (written by Dekopon)

- Checkpoint (3DS) stores CTR backups at `/3ds/Checkpoint/saves/0x%05X <title>/<backup>/`.
  The unique ID is `(u32)titleId >> 8`. Sources: `3ds/source/paths.cpp:70-96`
  (`savesRoot`, `ctrFolderName`, `ctrSavePath`) and `3ds/source/titleprobe.cpp:70`.
- Dekopon's save manager writes the **same layout on the Switch SD card**
  (see the table above). That makes `sdmc:/3ds/Checkpoint/saves/` a verified Switch path.
- Discovery (`Sources::findCheckpointExports`) looks only at the four Gen VI
  prefixes `0x0055D `, `0x0055E `, `0x011C4 ` and `0x011C5 `, and at `<backup>/main` regular
  files. It never matches a title name.

**Owner check:** confirm a backup exported from Dekopon (or copied from a 3DS) appears
as `sdmc:/3ds/Checkpoint/saves/0x0055D <title>/<backup name>/main`.

## JKSM (3DS): MANUAL LINK ONLY

- JKSM runs on a 3DS. Its user-save backups live at `sdmc:/JKSM/User Saves/<path-safe title>/<backup>`
  (`JKSM/source/FS/FS.cpp:23-29`, `AppStates/BackupMenuState.cpp:50`).
- The folder carries a title **name**, not a title ID, so it is not identity evidence.
- With `ExportToZip` the backup is a `.zip` (`BackupMenuState.cpp:214-255`).
  ZIP containers are **unsupported**.
- Nothing writes this layout to a Switch SD card. The owner copies an uncompressed
  `<backup>/main` over and links it manually.
- "JKSV" is the Switch save manager for Switch titles. It is not a 3DS save source.

## Azahar: UNSUPPORTED on Switch

- Azahar @ `68202bc4` has no Switch frontend. Its `src/` holds `citra_qt`, `citra_cli`,
  `citra_libretro`, `android` and others, with no `citra_switch`, and
  `.github/workflows/libretro.yml` builds the libretro core only for android, linux,
  windows, macos, ios and tvos.
- The relative save layout is the same as Dekopon's (identical
  `archive_source_sd_savedata.cpp`, `SYSTEM_ID/SDCARD_ID`). The libretro core puts its user
  dir at `<frontend save dir>/Azahar/` (`src/citra_libretro/core_settings.cpp:1078-1108`).
  No official Switch build exists, so no Switch root can be verified.
- The provider descriptor stays `Unsupported`: it is never scanned, and probes return
  `Unsupported`.

**Owner check (only if you use an Azahar Switch port):** report the NRO or core name,
its version or source link, and the full path of the folder that contains `Nintendo 3DS/`.

## Manual linking: always available

The user picks any file and assigns X, Y, Omega Ruby or Alpha Sapphire. The probe
accepts it only when the validated contents name the same game.
`Sources::readCandidate` reads read-only and refuses everything except the three SAV6 sizes.
The ORAS Special Demo (`0x5A00`) reports `Unsupported`.
