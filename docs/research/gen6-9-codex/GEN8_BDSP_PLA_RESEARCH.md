# BDSP and Legends: Arceus engineering research

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Family readiness

SWSH: existing strict native lane, tests executed; staging/export missing. BDSP: existing reader/layout guard and independent structural MD5 utility; multi-file acceptance unresolved. PLA: existing native reader and read validator, verified PKHeX PA8/SC source map; native staged implementation and independent retail evidence pending. None is newly hardware-accepted.

## BDSP: flat save, not SCBlock

[constructor / Initialize / SetChecksums](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8BS.cs) is the authoritative PKHEX layout at this pin. Expected extracted file names from NX read code are `SaveData.bin`, `Backup.bin`.

| Structure | File offset | Reference symbol |
|---|---|---|
| revision u32 | 0 | SaveRevision / Gem8Version |
| Items | `0x0563C` | MyItem8b |
| Party | `0x14098` | PartyInfo / Party8b |
| Box layout | `0x148AA`, length `0x64A` | BoxLayout8b |
| Boxes | `0x14EF4` | Initialize.Box; 40×30×0x158 |
| Trainer/status | `0x79BB4`, length `0x50` | MyStatus8b |
| Played | `0x79C04`, length 4 | PlayTime8b |
| Pokédex | `0x7A328`, length `0x30B8` | Zukan8b |
| hash | `0xE9818`, length 16 | HashOffset / CurrentHash |

PB8 entity size/crypt shares Decrypt8 with PK8, but native fields and legality remain PB8-specific. Party and boxes store full 0x158 records. Four revision/size pairs are in GAME_FORMAT_MATRIX. A valid hash with a wrong revision-size pair must be rejected before accessing revision additions. Later additions extend past the fixed hash location; hashing only the original length is wrong.

### Safe multi-file design: INFERRED, game selection UNKNOWN

NX `saveTrainerInfoBDSP` currently refuses persistence because a multi-file journal is absent. Keep that guard. PKHeX's single-buffer SAV8BS does not establish the game's two-file selection/rollback rules. No primary game implementation proving current-file authority was verified. Community rename/copy advice is not an engineering specification.

A future **export bundle** should snapshot both files, lengths, hashes and exact source identity; stage chosen content in memory; verify edited bytes and untouched companion; write a new temporary export directory; sync files + manifest; mark complete/rename only after all validators pass. This makes publication transactional but does **not** prove the pair is game-consistent. No symlink/path escape, overwrite of pre-existing export or source path, or fallback to rewriting one original.

Required private evidence: both files from repeated manual save and autosave cycles on each supported patch, per-file MD5/revision/status, which file changes and when, and controlled expendable-copy corruption tests to observe fallback behavior. Share only non-sensitive diagnostics/hashes, keep actual saves local. Do not assume timestamps decide authority or duplicate one file over the other.

### Research fixtures delivered

`format_probe.py::bdsp_integrity` checks exact size, matching revision and whole-file zero-field MD5 on a private buffer. Tests generate zero-filled **structural** envelopes for all four revisions and then tamper a byte, truncate and mismatch revision. These are not playable saves; no positive retail validity claim is made. A full positive record/container fixture should follow existing synthetic fixture conventions and be accepted by two independent parsers before being used as an export gate.

## PLA: distinct PA8 mechanics

[SAV8LA](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/SAV8LA.cs) uses SwishCrypto, two known sizes, DetectRevision and its own blocks. [accessor](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Saves/Access/SaveBlockAccessor8LA.cs) supplies **Box key `0x47E1CEAB`**, not SWSH's `0x0D66012C`; Party `0x2985FE5D`. PA8 stored 0x168, party 0x178; boxes use stored 0x168, unlike SWSH party-size boxes. 32 pastures×30 and party six. All constants point to source symbols and generated keys.

[PA8 / LoadStats / GetGanbaruStat / PurchasedRecord / MasteredRecord](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PKM/PA8.cs) exposes effort levels (IGanbaru), alpha/noble state, AlphaMove, purchased/mastered flags and scaling. Purchased flags at `0x155` length8 and mastered at `0x15D` length8 are source-backed; preserve unreached bits. Native stats are not ordinary SWSH EV formulas. Mastery/purchase legality needs species-specific move-shop tables and history, not a checkbox shared with TRs. An inherited binary Egg flag does not establish native breeding support.

Trainer `MyStatus8a`, inventory `PlayerBag8a`/`MyItem8a`, Pokédex `PokedexSave8a` and task/progress structures are in the source ledger. Research tasks and progress affect encounter/story state; keep them read-only initially. Use native form and gender presence from PersonalTable.LA, preserve alpha/scale/height consistency, and do not offer a SWSH species list.

### Minimal PLA prototype gates

Reuse tested SCBlock parser after proving hash and type/geometry parity on LA blocks. Add family-specific identification, strict per-record decode, quarantine invalid boxes, reject corrupt declared party. Verify record boundaries with 0x168 versus 0x178. Add stats oracle tests for effort levels and nature, mastery edge cases, malformed/missing item blocks and unknown revision. Export whitelist must compare **decrypted** touched PA8 records and metadata, preserving every unknown block and research field. Native build, copy-only source handling and save/reopen hardware tests remain required. PKSM-Core at both inspected revisions provides no independent PA8/SAV8LA implementation; this evidence is PKHEX + existing NX reader, not two independent authorities.
