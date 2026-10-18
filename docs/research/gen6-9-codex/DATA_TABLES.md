# Catalogs, mechanics tables and licensing

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Data sources and schema

All paths below refer to PKHeX `641c26ba100225f3605e5a44b61b3b16e15a3774` and are hashed/linked in `metadata/pkhex-index.json`; existing generated lane tables cite the earlier pinned `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`. Keep their revision labels separate. Re-generating against current source is a data migration, not a harmless refresh.

| Family | Personal / stats | Item catalog / pocket source | Other native requirements |
|---|---|---|---|
| XY | `PersonalTable.XY`; PK6 generic personal uses AO | `ItemStorage6XY`, `PlayerBag6XY`, InventoryPouch4 | XY form/move membership, Amie/affection, Super Training/ribbons |
| ORAS | `PersonalTable.AO` | `ItemStorage6AO`, `PlayerBag6AO` | ORAS forms/Mega/primal availability, contest ribbons |
| SM | `PersonalTable.SM`; game-specific learn source | `ItemStorage7SM`, `PlayerBag7SM` | SM availability distinct from USUM; Hyper Training, Z-Crystals |
| USUM | `PersonalTable.USUM`, native PK7 uses USUM table | `ItemStorage7USUM`, `PlayerBag7USUM` | added species/forms/moves; no SM backport assumptions |
| LGPE | `PersonalTable.GG`, PB7 LoadStats/CP | `ItemStorage7GG`, `PlayerBag7b` | AVs instead of EVs, friendship stat multiplier, partner restrictions |
| SWSH | `PersonalTable.SWSH` | `ItemStorage8SWSH`, `PlayerBag8` | Dynamax/Gigantamax, TR flags, DLC presence |
| BDSP | `PersonalTable.BDSP` | `ItemStorage8BDSP`, `PlayerBag8b`, InventoryItem8b | Gen IV-remake move pools, seals/contests, separate record semantics |
| PLA | `PersonalTable.LA`, PA8 Ganbaru stats | `ItemStorage8LA`, `PlayerBag8a`, InventoryPouch8a | effort levels, alpha/noble state, move shop/mastery, research tasks |
| SV | `PersonalTable.SV` | `ItemStorage9SV`, `PlayerBag9`, InventoryItem9 | Tera original/override, TM records, raid/met data, DLC dexes |
| ZA | `PersonalTable.ZA` | `ItemStorage9ZA`, `PlayerBag9a`, InventoryItem9a | alpha/scale, Plus move flags, Mega-related forms, revision-specific DLC data |

Personal factory names are navigation labels: use [PersonalTable](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/PersonalInfo/PersonalTable.cs) and the corresponding PersonalTable*/PersonalInfo* definitions. Move learn sources live under `Legality/LearnSource`, encounters under `Legality/Encounters`; resource loaders reveal exact embedded resource names. **Do not use species or move numeric ceilings as exact-game membership.** Forms often use additional personal rows; a national-ID row alone loses form stats/abilities/growth.

## Existing reusable generated metadata

Gen6PersonalData/Gen7PersonalData contain base stats/growth/form-index rows, generated from PKHeX personal_ao/personal_uu, with differential tests against pinned PKSM. Gen6BagCatalog/Gen7BagCatalog contain per-family valid/addable item lists. Existing shared `BaseStatsGen7`, `BaseStatsGen89`, SWSHDexTable, PLADexTable, SVDexTable and tools `gen_personal.py`, `gen_learnsets.py`, `gen_moveinfo.py`, `gen_itempouches.py`, `gen_itempresence.py`, `gen_movepresence.py` must be audited before generating duplicates. None was regenerated on this branch.

[tools/pkhex_source.py](https://github.com/GlitchedZeus/PokeBank-NX/blob/d104310da30d8095931ca16e097a2f6e2652ddfe/tools/pkhex_source.py) pins download behavior for existing asset/data pipelines; do not let build-time HEAD requests silently change accepted metadata. Full new-game species/form/move/item availability tables are still pending. The research index supplies source locations, not a completed legal acquisition database.

## Quantities: exact game and pocket

Gen VI PlayerBag6XY/AO defines Items/Medicine/Berries max999, KeyItems and TMHMs max1; ORAS Medicine/Berries starts shift relative to XY (0x970/0xA70 vs 0x968/0xA68). Source: [PlayerBag6XY](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Items/Bags/PlayerBag6XY.cs) and [PlayerBag6AO](https://github.com/kwsch/PKHeX/blob/641c26ba100225f3605e5a44b61b3b16e15a3774/PKHeX.Core/Items/Bags/PlayerBag6AO.cs). These offsets are inside the MyItem block, not whole-file offsets.

Gen VII uses packed InventoryPouch7 slots with sort/new bits; preserve them when changing quantities. LGPE uses its own PlayerBag7b. PLA has regular/key/stored/recipe blocks; SV and ZA use item-indexed formats and per-item exceptions. For each title, derive `maxCount(title, revision, pocket, item)` from PlayerBag + ItemStorage + item encoding. Until that complete extraction is tested, expose only validated existing lane operations. Do not carry Gen VI's 999 cap into all Switch games.

## Recommended integration data interface: INFERRED

Use immutable `CatalogKey(exactGameId, saveRevision, dataRevision)` → species/form presence, learnability query, ability slots, growth/base stats, held item compatibility, pouch membership/cap. Return Supported/Unsupported/Unknown, preserving existing source moves when chronology is incomplete. UI search and staged validators share the same catalog instance; final export independently rechecks native limits.

## License boundary

Root NX LICENSE is AGPL-3.0; PKHeX is GPL-3.0 and the existing legality adapter declares GPL-3.0-or-later. PKSM-Core/vendor notices must be preserved; do not equate “already a submodule” with permission to copy into every component. AGPLv3/GPLv3 combination may be possible, but exact `only`/`or-later`, source-file exceptions, distribution and attribution need owner license review before importing implementations/data. This research branch adds independently written tools and factual numeric metadata/source hashes, not copied upstream algorithms. No ROMs, event archives, personal saves or proprietary art were imported. Original game decompilation code for VI–IX was not found/verified in this campaign; do not cite older pret games as proof of modern layout.

## Delivered quantity extraction and important exceptions

`metadata/pkhex-index.json::bag_pouches` extracts offsets/slot counts/default caps from PlayerBag6XY/6AO/7SM/7USUM/7b/8 and default caps from ItemStorage8BDSP/9SV/9ZA. Gen VI slot counts are null where supplied indirectly by ItemStorage, rather than invented. These defaults are **not** final quantity policy.

Observed exceptions requiring implementation: SM/USUM Z-Ring item797 may be2 (existing NX defect reproduced); SWSH TMs cap1 while TRs use the pouch cap; LGPE mixed item/key and battle/Mega pockets have per-item caps; SV picnic accessories cap1. Current ZA PlayerBag9a uses item2618 Mega Shard max9999 only when block `0x0ABC6547` is present, preserves item2619 Colorful Screw current quantity, and permits zero of quest-only item2612 Cherished Ring. These constants and conditions are PKHEX source facts from `Items/Bags/PlayerBag9a.cs::GetMaxCount`, not certified retail/patch acceptance. Never automatically run its cleanup behavior on an immutable source.
