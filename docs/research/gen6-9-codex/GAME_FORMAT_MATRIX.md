# Native game and format matrix

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Per-title identity and storage

Title IDs for 3DS are ORIGINAL PKSM `3ds/source/titles/loader.cpp::originalIDs` at `9e18c8068ede315f73a99f4c834819630393b5c5`. Switch IDs are ORIGINAL PokeBank [src/Games/GameIdentity.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/d104310da30d8095931ca16e097a2f6e2652ddfe/src/Games/GameIdentity.cpp); independent device enumeration not performed. IDs are corroboration; validated save contents decide title. No Switch 2 title ID is asserted.

Record sizes are stored / party entity sizes, **not always save slot strides**. All listed capacities and layouts are PKHEX; party maximum is six, with LGPE using references into common storage.

| Game | Platform | Title ID | Container class | Record | Entity bytes | Storage | Exact source |
|---|---|---|---|---|---|---|---|
| X | 3DS | `0004000000055D00` | SAV6XY | PK6 | 0xE8 / 0x104 | 31×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV6XY.cs) |
| Y | 3DS | `0004000000055E00` | SAV6XY | PK6 | 0xE8 / 0x104 | 31×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV6XY.cs) |
| Omega Ruby | 3DS | `000400000011C400` | SAV6AO | PK6 | 0xE8 / 0x104 | 31×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV6AO.cs) |
| Alpha Sapphire | 3DS | `000400000011C500` | SAV6AO | PK6 | 0xE8 / 0x104 | 31×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV6AO.cs) |
| Sun | 3DS | `0004000000164800` | SAV7SM | PK7 | 0xE8 / 0x104 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7SM.cs) |
| Moon | 3DS | `0004000000175E00` | SAV7SM | PK7 | 0xE8 / 0x104 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7SM.cs) |
| Ultra Sun | 3DS | `00040000001B5000` | SAV7USUM | PK7 | 0xE8 / 0x104 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7USUM.cs) |
| Ultra Moon | 3DS | `00040000001B5100` | SAV7USUM | PK7 | 0xE8 / 0x104 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7USUM.cs) |
| Let’s Go Pikachu | Switch | `010003F003A34000` | SAV7b | PB7 | 0x104 / 0x104 | 1000 shared; 40×25 UI | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7b.cs) |
| Let’s Go Eevee | Switch | `0100187003A36000` | SAV7b | PB7 | 0x104 / 0x104 | 1000 shared; 40×25 UI | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV7b.cs) |
| Sword | Switch | `0100ABF008968000` | SAV8SWSH | PK8 | 0x148 / 0x158 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8SWSH.cs) |
| Shield | Switch | `01008DB008C2C000` | SAV8SWSH | PK8 | 0x148 / 0x158 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8SWSH.cs) |
| Brilliant Diamond | Switch | `0100000011D90000` | SAV8BS | PB8 | 0x148 / 0x158 | 40×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8BS.cs) |
| Shining Pearl | Switch | `010018E011D92000` | SAV8BS | PB8 | 0x148 / 0x158 | 40×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8BS.cs) |
| Legends Arceus | Switch | `01001F5010DFA000` | SAV8LA | PA8 | 0x168 / 0x178 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8LA.cs) |
| Scarlet | Switch | `0100A3D008C5C000` | SAV9SV | PK9 | 0x148 / 0x158 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV9SV.cs) |
| Violet | Switch | `01008F6008C5E000` | SAV9SV | PK9 | 0x148 / 0x158 | 32×30 | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV9SV.cs) |
| Legends Z-A | Switch | `0100F43008C44000` | SAV9ZA | PA9 | 0x148 / 0x158 | 32×30; gaps required | [layout](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV9ZA.cs) |

Record crypt sizes: [SIZE_6*, SIZE_8*, SIZE_8A*](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/Util/PokeCrypto.cs). Record class source is `PKM/<Record>.cs` in the ledger. PB7 is a 260-byte native class; do not advertise its generic base stored length as a supported standalone 232-byte PB7 export.

## Container sizes and revisions

Every current `SaveUtil.SIZE_G6*` through `SIZE_G9*` numerical constant is in `metadata/pkhex-index.json`, with symbol, source line, SHA and URL. It includes calculated SV size variants. **Constants alone are not an allowlist**: [IsSizeGen9SV / IsSizeGen9ZA / IsG8BDSP](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Util/SaveUtil.cs) includes SV bounded ranges (+100 tolerance) and revision conditions. Preserve those distinctions; do not convert all numbers to unconditional valid sizes.

| Family | Full extracted file sizes | Validation |
|---|---|---|
| XY | `0x65600` | BEEF + 55 block CRCs + X/Y version |
| ORAS | `0x76000` | BEEF + 58 block CRCs + OR/AS version; demo `0x5A00` UNSUPPORTED |
| SM | `0x6BE00` | BEEF + 37 CRCs with cleared signature region + signature/table hash |
| USUM | `0x6CC00` | BEEF + 39 CRCs with cleared signature region + signature/table hash |
| LGPE | `0x100000`; active region `0xB8800` | BEEF at active footer, metadata discriminator, 21 CRCs; GP/GE version |
| SWSH | `0x1716B3`, `0x17195E`, `0x180B19`, `0x180AD0`, `0x1876B1`, `0x187693`, `0x187668`, `0x18764A` | salted SHA256 + complete SC parse + required key geometry + game/revision |
| BDSP | `0xE9828`, `0xEDC20`, `0xEED8C`, `0xEF0A4` | revision respectively `0x25`, `0x2C`, `0x32`, `0x34`; zero-field MD5 + title + record checks |
| PLA | `0x136DDE`, `0x13AD06` | salted SHA256 + LA-specific keys/revision + PA8 |
| SV | base/history constants and six DLC size ranges in JSON / SaveUtil | hash+SC schema+status; revision inferred from DLC blocks, not file size alone |
| ZA | `0x2F3284`, `0x2F3289`, `0x309FA6`, `0x309FB3` | hash+SC+ZA status and UInt64 revision key; unknown revisions fail closed |

All numeric entries above: [SaveUtil](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Util/SaveUtil.cs) and [Gem8Version](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Substructures/Gen8/BS/Gem8Version.cs). These describe this pinned parser's supported structures, not every possible retail patch.

## Files, blocks and exact access

3DS and LGPE use `main`; SWSH/PLA/SV/ZA use `main`; BDSP uses `SaveData.bin` with companion `Backup.bin`, as read by [src/Save/GetSaveFileContents.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/d104310da30d8095931ca16e097a2f6e2652ddfe/src/Save/GetSaveFileContents.cpp). Companion selection semantics remain UNKNOWN. Preserve companion names and bytes.

Complete XY/ORAS/SM/USUM/LGPE block offset/length/ID tables are generated in JSON. Metadata entry location is `metadata + 0x14 + ID*8`, checksum at entry+6; [BlockInfo3DS](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Blocks/BlockInfo3DS.cs). Gen VI/VII footer metadata is size minus `0x200`; LGPE uses active size minus `0x200`.

SC families have keyed blocks, **no stable whole-file field offsets**. Selected keys are generated with source symbols. BDSP flat offsets are listed in the Gen VIII report. Source name/type comments are not authoritative type proof: compare actual consumer and blank-block geometry.

## Common per-title editing contract

Trainer strings must use the family StringConverter and preserve termination/trash bytes according to a reviewed mask; TID16/SID16 are not display six-digit IDs. Trainer/game/language, money and playtime use family status/misc blocks. Bag sources are `Items/Bags/PlayerBag<family>.cs` and matching ItemStorage/pouch/item classes; never apply a universal quantity cap. Pokédex is `Zukan<family>` with game-specific form/state flags. Personal/experience, forms, abilities, ribbons, moves and version membership are in DATA_TABLES. Binary fields not audited for a title stay ReadOnly or Hidden. Export requires independent strict reparse, same identity/revision, semantic diff whitelist, full integrity checks and immutable original hash.

UNKNOWN: exhaustive retail full-file-size universe beyond this pinned parser, all Switch 2 variants, every post-pin patch, and game acceptance of new exports. Unsupported revisions must remain UNKNOWN/UNSUPPORTED, never guessed into the nearest family.
