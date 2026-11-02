# Pinned sources and evidence ledger

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Repository revisions investigated

| Repository | Exact commit | Role |
|---|---|---|
| GlitchedZeus/PokeBank-NX main | `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad` | research branch base; current main fetched first |
| GlitchedZeus/PokeBank-NX #307 | `d104310da30d8095931ca16e097a2f6e2652ddfe` | current shared editor/product architecture |
| GlitchedZeus/PokeBank-NX #309 | `5e6836902555da4431be16348ef21abec0358d13` | PK6/SAV6 lane |
| GlitchedZeus/PokeBank-NX #311 | `2438b5d0b9294193a9de9c8f14736c59ebbb90a5` | PK7/SAV7 lane |
| GlitchedZeus/PokeBank-NX #312 | `2f065ee2b0768d01bf621f969aef01120c6231e3` | stacked lane audit/test checkout |
| GlitchedZeus/PokeBank-NX #301 | `809a31841fc2ad43dda02d4412cb9891f29a482f` | detached evidence adapters and failing wiring |
| GlitchedZeus/PokeBank-NX #314 | `1ceea5634d1bfc251087727aa9444ec0a1109305` | current legality architecture/CI |
| kwsch/PKHeX | `641c26ba100225f3605e5a44b61b3b16e15a3774` | freshly fetched HEAD; modern family source index |
| kwsch/PKHeX | `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` | existing local reference / lane source pin |
| FlagBrew/PKSM-Core | `aa22d7a4f87c0351baf7da5962ba5acd01039a7c` | unchanged NX submodule differential reference |
| FlagBrew/PKSM-Core | `d9bca6fcdf622760a0aa963565b102276e2ae2a3` | current upstream resign path cross-check |
| FlagBrew/memecrypto | `bd3b16ee2c12bd9ee353cf3098cb998e7880638c` | exact vendor implementation tested |
| PalindromicBreadLoaf/dekopon | `dfba28f5e5342310c1ccff5963363be8512862f6` | 6 hashed source files; provider/runtime facts |
| azahar-emu/azahar | `987f18405073960622e1043bb44005989135c6e1` | 3 hashed source files; provider/runtime facts |
| FlagBrew/Checkpoint | `740f221a853db784401458fe74276977afcfa8b0` | 2 hashed source files; provider/runtime facts |
| J-D-K/JKSM | `f883dff8fccbbb90851c3c6c2a16ea8bda250356` | 4 hashed source files; provider/runtime facts |
| FlagBrew/PKSM | `9e18c8068ede315f73a99f4c834819630393b5c5` | 3 hashed source files; provider/runtime facts |
| J-D-K/JKSV | `d39fd80a208b7c802d12edb12f362506a0addb47` | 8 hashed source files; provider/runtime facts |
| switchbrew/libnx | `20cbcfd90567f7bf84df7725f27e345e2164fa6b` | 3 hashed source files; provider/runtime facts |

`imneme/pcg-cpp@428802d1a5634f96bcd0705fab379ff0113bcf13` was initialized as a test dependency, not independently researched for save formats. No original Gen VI–IX game decomp source was verified. PKHeX forks found in search are not independent oracles merely because the repository name differs. Search discovery alone is not cited as a format fact.

## Concrete source/symbol map

- Record sizes/crypto: [SIZE_6*/SIZE_8*/SIZE_8A*, CryptArray, Decrypt67/8/8A](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/Util/PokeCrypto.cs); `PKM/PK6.cs`, PK7, PB7, G8PKM, PK8, PB8, PA8, PK9, PA9 properties/checksum/stat methods.
- Save size/identity: [SIZE_G*, IsSizeGen9SV, IsG8BDSP, IsG7LGPE](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Util/SaveUtil.cs); `Game/Enums/GameVersion.cs` values and each SAV*.IsVersionValid.
- 3DS/LGPE blocks: `Saves/Access/SaveBlockAccessor6XY/6AO/7SM/7USUM/7b.cs::BlockInfo`, `Saves/Blocks/BlockInfo3DS.cs::ChecksumOffset/BlockInfo6/7/7b`, `Checksums.cs`.
- Modern SC: `Saves/Encryption/SwishCrypto/SwishCrypto.cs::ComputeHash/Decrypt/Encrypt`, `SCBlock.cs`, `SCTypeCode.cs`, `SCXorShift32.cs`; family SaveBlockAccessor keys and required substructures.
- SAV7 signing: [SignInPlace/VerifyMemeData](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Encryption/MemeCrypto/MemeCrypto.cs), MemeKey and `SAV7.cs::ClearMemeCrypto/GetFinalData`; PKSM-Core SavSUMO/SavUSUM::resign; memecrypto.c::reverseCrypt/sign/verify and rsa.c/sha1.c. Vendor helper experiment is INDEPENDENT execution, not game-code evidence.
- BDSP: [constructor/HashOffset/SetChecksums/SaveRevision](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8BS.cs), Gem8Version; NX BDSPReadValidation/Trainer8BDSP/GetSaveFileContents persistence refusal.
- PLA: `SAV8LA.cs`, PA8 LoadStats/GetGanbaruStat/PurchasedRecord/MasteredRecord, SaveBlockAccessor8LA, PokedexSave8a, PlayerBag8a.
- SV/ZA: `SAV9SV.cs::SaveRevision`, `SAV9ZA.cs::GapBoxSlot/GetPartyOffset/WriteSlotBox`, PK9 TeraType*/StatAlignment and PA9 PlusFlags*/IsAlpha/Scale, respective accessors/personal/legality sources.
- Shared architecture: #307 `include/UI/ExactFormatEditorProvider.h`, `SharedPokemonEditorContract.h`, `SharedPokemonEditorWorkspace.h`, `SharedMenuGeometry.h`, `SharedFieldState.h`, `src/UI/SharedPokemonEditorSurface.inc`, Gen5SharedEditorBridge and THREAD_S_HANDOFF.

## Machine-readable provenance

`metadata/pkhex-index.json` has **907** file hashes/immutable URLs; extracted rows have file/symbol or property and source line. `metadata/provider-sources.json` records each actual inspected provider file hash. `evidence/github-snapshot.json` preserves PR bodies, heads and PR-triggered workflow first-page results, plus branch inventory/search. `evidence/failed-workflow-jobs.json` adds failed job/step details; it does not claim full log root causes.

Confidence: source extraction is PKHEX or ORIGINAL. Separate PKSM differential tests plus independent tools are INDEPENDENT/NX-TEST. Future integration recipes are INFERRED. Device acceptance/uncited title revisions remain UNKNOWN. An unimplemented adapter is UNSUPPORTED even if its format has good source references. Hashing a source file does not claim every line was manually audited.

## Official service references (live pages, no Git revision)

Retrieved 2026-10-18: [HOME movement](https://home.pokemon.com/en-ca/move/), [Z-A HOME notice](https://legends.pokemon.com/en-us/news/pokemon-home), [Bank/HOME connection instructions](https://support.pokemon.com/hc/en-us/articles/360038131072-About-connecting-Pok%C3%A9mon-HOME-to-different-games). These support service pathway restrictions, not save offsets or individual history. Refresh at implementation time; do not manufacture a source commit for a website.

`metadata/upstream-drift.json` compares selected file hashes at old/current PKHeX pins. PA9 and MemeCrypto files are byte-identical; SAV9ZA differs only by a blank-line removal in the inspected diff. SaveUtil and PokeCrypto hashes differ and need semantic review before changing lane pins. A changed hash alone is not a changed format. Quantity source facts include PlayerBag7SM/7USUM::GetMaxCount and PlayerBag9a::GetMaxCount; this audit found a missed per-item exception in NX Gen7Bag.
