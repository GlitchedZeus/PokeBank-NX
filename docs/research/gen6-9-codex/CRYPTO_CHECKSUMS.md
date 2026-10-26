# Crypto, integrity and verification

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Pokémon entity crypto: PKHEX + independent existing tests

[Decrypt67 / Encrypt67 / Decrypt8 / Decrypt8A / CryptArray](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/Util/PokeCrypto.cs) defines the common little-endian LCG XOR and four-block permutation. LCG step is `seed = seed * 0x41C64E6D + 0x6073` modulo 2^32; XOR each u16 with high 16 bits. EC is the seed; shuffle selector uses bits 13..17, with the source's permutation table covering selectors 24..31 by repeating earlier entries. Use the existing codecs rather than reimplementing it.

| Record | Core end | Four block lengths | Party tail crypt | Checksum |
|---|---|---|---|---|
| PK6/PK7 | `0xE8` | `0x38` | starts `0xE8`, same EC | Add16 of `[8,0xE8)` |
| PB7 | generic Decrypt67 core `0xE8` | `0x38` | required 260-byte class | same core Add16 |
| PK8/PB8/PK9/PA9 | `0x148` | `0x50` | starts `0x148`, same EC | Add16 of `[8,0x148)` |
| PA8 | `0x168` | `0x58` | starts `0x168`, same EC | Add16 of `[8,0x168)` |

Checksum stored u16 at offset 6; sanity u16 at 4 must be zero in strict adapters. These are structural checks, not acquisition proof. PokeCrypto + record `CalculateChecksum` symbols in the pinned source ledger supply these constants. PB8/PK8 inherited implementations must be traced through G8PKM, not guessed from class name.

## Save integrity

- Gen VI: [BlockInfo6](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Blocks/BlockInfo3DS.cs) → [CRC16_CCITT](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Util/Checksums.cs); CCITT polynomial `0x1021`, initial `0xFFFF`, no final inversion. Verify each exact table block, not padding.
- SM/USUM: BlockInfo7 → CRC16Invert, reflected `0xA001`, initial `0xFFFF`, final complement; clear block 36 `[+0x100,+0x180)` for its CRC. Then bind the checksum table SHA256 to the recovered Meme payload. See signing report.
- LGPE: BlockInfo7b → CRC16NoInvert, reflected `0xA001`, initial 0, no complement. Do not reuse SM CRC or Meme signing.
- BDSP: [HashOffset / SetChecksums](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8BS.cs) hashes **the entire current revision's file** using MD5 after zeroing `[0xE9818,0xE9828)`. Hash is in the middle of later expanded saves. The research probe computes on a private copy and never modifies input.
- SWSH/PLA/SV/ZA: [ComputeHash / GetIsHashValid](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Encryption/SwishCrypto/SwishCrypto.cs) SHA256 over static intro salt, encrypted payload, static outro salt; digest is last 32 bytes. Then remove repeating **127-byte** xorpad; SCBlock keyed XorShift32 decrypts type, lengths, subtype and payload. Source constants are already in NX Gen8SwishCrypto; do not copy another salt table.

## Strict parser requirements

Bounds-check before reads/allocations, reject unknown types, negative/signed-invalid lengths, count×element-size overflow, truncated final blocks, duplicate keys, and mismatched required consumer geometry. Validate hash **before** untrusted block allocations. Preserve all unknown-but-valid blocks and order. `ChecksumsValid => true` in an already-decrypted PKHeX save means no secondary per-block checksum, not an unconditional valid input.

Existing `writeBlock` assumes a valid internal Block and truncates array data-size division if handed a non-multiple. Before exposing an encoder to user-created blocks, require known type/subtype and `data.size % typeSize == 0`, unique keys, bounded total length. This is an INFERRED API hardening need; existing parsed block round trips pass.

## Test evidence limits

Synthetic round trips and independent PKSM comparisons exist for Gen VI–VIII. `signature_probe` demonstrates verifier behavior, not actual game acceptance. No authentic retail saves are added. No fixed-vector corpus is claimed for all modern families. The JSON source hashes permit reproduction; positive **retail** fixtures for BDSP/PLA/SV/ZA are still UNKNOWN. New research test fixtures are deliberately labeled structural, not playable.
