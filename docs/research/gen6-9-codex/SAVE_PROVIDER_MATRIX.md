# Switch and 3DS save-provider evidence

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Rechecked upstream revisions

Current original-source commits, fetched this session:

| Provider | Commit | Switch result |
|---|---|---|
| Dekopon | `dfba28f5e5342310c1ccff5963363be8512862f6` | ORIGINAL Switch frontend/NRO source exists; game performance/device use untested |
| Azahar | `987f18405073960622e1043bb44005989135c6e1` | ORIGINAL documented desktop/Android; no verified Switch frontend/root |
| Checkpoint | `740f221a853db784401458fe74276977afcfa8b0` | 3DS exports; same path also written by Dekopon on Switch |
| JKSM | `f883dff8fccbbb90851c3c6c2a16ea8bda250356` | 3DS; manual copy/link only |
| PKSM | `9e18c8068ede315f73a99f4c834819630393b5c5` | title IDs and `/main` archive reference, not a Switch 3DS emulator |
| JKSV | `d39fd80a208b7c802d12edb12f362506a0addb47` | original Switch backup manager; configurable root, manual link preferred until per-title path adapter reviewed |

Exact source file hashes and immutable URLs: `metadata/provider-sources.json`. Existing descriptor/scan implementation: [include/Integration/Gen6/Gen6SaveSources.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/2f065ee2b0768d01bf621f969aef01120c6231e3/include/Integration/Gen6/Gen6SaveSources.h) and `Gen6SaveSources.cpp`; Gen7 sources reuse the title-ID generic layout. No source save was opened in this session.

## Paths and detection

| Provider | Root / relative filename | Detection and limitations |
|---|---|---|
| Dekopon default | `sdmc:/switch/dekopon/` + `sdmc/Nintendo 3DS/<ID0>/<ID1>/title/<high:08x>/<low:08x>/data/00000001/main` | `ID0=ID1=32 zeroes`; title high/low are format strings from original archive source, not a title-name guess |
| Dekopon configured | pointer `sdmc:/switch/dekopon/user_dir.txt` replaces user root | config ReadUserDirPointer strips whitespace, supplies trailing slash; roots inaccessible to app stay unsupported. Revalidate path and bounds; do not scan USB by guessing |
| Dekopon Checkpoint export | `sdmc:/3ds/Checkpoint/saves/0x%05X <title>/<backup>/main` | unique ID `(program_id>>8)&0xFFFFF`; literal ID prefix identifies candidate only |
| Checkpoint copied from 3DS | same `/3ds/Checkpoint/saves/...` | manual SD copy can produce same directory; cannot prove origin from folder name |
| JKSM | `sdmc:/JKSM/User Saves/<path-safe title>/<backup>/main` | original `JKSM/source/FS/FS.cpp`; title-name folder is not identity. ZIP exports UNSUPPORTED; manually link extracted `main` |
| JKSV | default `sdmc:/JKSV`, config at `sdmc:/config/JKSV/JKSV.json`, custom paths in `Paths.json` | current `source/config/ConfigContext.cpp`; backup subtree follows app metadata/config. Exact per-title layout adapter remains research pending; manual file linking avoids unverified scan conventions |
| Installed Switch title | mounted filesystem, **no SD path assumption** | libnx save metadata enumeration filtered by AccountUid/title ID, temporary read mount, copy-out, unmount. Never commit installed source |
| Azahar desktop/Android | no verified native Switch root | UNSUPPORTED auto-discovery; manually copy a valid 3DS extracted backup if supplied locally |

For X at default Dekopon root: `sdmc:/switch/dekopon/sdmc/Nintendo 3DS/00000000000000000000000000000000/00000000000000000000000000000000/title/00040000/00055d00/data/00000001/main`. Other 3DS IDs are in GAME_FORMAT_MATRIX. Gen VII prefixes derive from SN/MN/US/UM IDs, not guessed filenames.

ORIGINAL paths rechecked in `dekopon:src/common/file_util.cpp`, `src/citra_switch/config.cpp`, `src/citra_switch/save_manager.cpp`, `src/core/file_sys/archive_source_sd_savedata.cpp`, `src/core/hle/service/fs/archive.h`; `Checkpoint:3ds/source/paths.cpp` and `titleprobe.cpp`. Azahar's current `README.md`, `src/CMakeLists.txt` and libretro workflow establish advertised targets, not proof that no third-party port can ever exist. Such a port needs its own pinned source and root evidence.

## Save Instances and immutable reads

Every candidate is a separate Save Instance keyed by provider + canonical path/archive identity + title assignment + fingerprint. Do not coalesce a Dekopon live file, Checkpoint backup and copied manual file merely because their bytes match. Display provider/game/read-only status, retain source selection across modal return, and re-read/revalidate the fingerprint before exporting a staged copy. Assignment mismatch is a refusal, never a silent relabel.

Validate content size/footer/CRC/signature or SC hash/status before presenting a title as usable. Folder ID is useful candidate filtering, not final game identity. Never recursively scan arbitrary SD directories: enumerate exact configured roots and bounded backup depth, stat before reading, reject unsupported/oversized archives, cache immutable parse results and invalidate on changed fingerprint. Signature verification leaks in the existing vendor, so repeated rescans require careful lifetime/caching policy.

No emulator for Switch-native games is asserted to run on Switch. PC emulator directories are not Switch providers. Verified source paths do not establish any game's speed, completeness, or hardware acceptance.
