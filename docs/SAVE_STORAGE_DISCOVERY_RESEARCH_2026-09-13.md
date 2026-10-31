# PokeBank NX — Storage, Save Revision, Discovery & Fixture Research

Date: 2026-09-13

Status: **engineering reference / implementation planning**

This document preserves the 2026-09-13 research pass covering PKSE Storage migration, modern Switch save revisions, emulator save discovery, native Switch save enumeration, golden-corpus strategy, and Vault recovery design.

It is intentionally a research/reference document. Upstream projects may change; when implementing a subsystem, pin the exact upstream revision used for the implementation audit and add permanent PokeBank NX tests for the behavior we depend on.

Related tracking:

- #27 — legacy PKSE Storage / `bank.dat` migration
- #11 — modern Switch save validation/revision support
- #56 — universal emulator save discovery
- #17 — golden test corpus
- #3 / Master Vault work — durable bank/vault destination and recovery model

---

## 1. PKSE `bank.dat` / `PKSEBANK` — importer format is implementation-ready

The current PKSE `Bank.cpp` / `Bank.h` implementation is concrete enough to freeze a PokeBank NX **read-only decoder contract** now.

### Unified format

The current unified bank starts with a 16-byte header:

```text
0x00..0x07  ASCII "PKSEBANK"
0x08..0x0B  little-endian u32 format version
0x0C..0x0F  little-endian u32 box count
```

After the header come fixed-size slot records.

Current PKSE uses:

```text
100 boxes
30 slots per box
3,000 possible occupied slots
```

The file-stored box count is important and is what allows older/smaller banks to remain forward-compatible. PokeBank NX must honor the stored count rather than assuming exactly 100 boxes.

### Frozen persistent group tags

The persisted group tag is a **file-format value**. It must not be tied directly to a changing `GameVersion` enum or renumbered later.

```text
0 = Empty
1 = LGPE / GG
2 = Sword / Shield
3 = Brilliant Diamond / Shining Pearl
4 = Legends: Arceus
5 = Scarlet / Violet
6 = Legends: Z-A
7 = FireRed / LeafGreen
```

### Slot size

The largest current native Pokémon payload is 376 bytes (`0x178`), used by BDSP/PLA party-format Pokémon. Sword/Shield is smaller (`0x158`).

Freeze PKSE unified v1 as:

```text
u32 little-endian persisted group tag
byte payload[376]
-------------------------------------
380 bytes per slot
```

Current fixed Pokémon region:

```text
16-byte header
+ 100 boxes × 30 slots × 380 bytes
= 1,140,016 bytes
```

**Important:** the PokeBank NX v1 decoder should hard-freeze `376` / `380`. Do not calculate the old v1 slot width from whatever the future application considers its largest Pokémon payload. If PKSE later adds a 400-byte payload format, that must not change how old v1 offsets are interpreted.

Do not require EOF immediately after the Pokémon slot region. Current PKSE can persist optional box display-name data after the collection.

### Legacy pre-`PKSEBANK` storage

PKSE contains explicit migration support for older headerless per-game banks.

Known migration filenames include:

```text
gg_bank.dat
swsh_bank.dat
za_bank.dat
sv_bank.dat
pla_bank.dat
bdsp_bank.dat
```

The legacy representation is effectively a flat sequence of native Pokémon records with no unified header. A record is considered occupied when its first four bytes — the Encryption Constant — are non-zero. PKSE imports occupied records into unified Storage first-fit.

Current migration code does not list an old `frlg_bank.dat`, likely because FRLG support arrived after the unified format.

### Legacy type-inference warning

If a legacy file has been renamed, file size alone may not uniquely identify its Pokémon type. For example, BDSP and PLA can share the same 376-byte record width and multiple modern formats share the 344-byte family.

PokeBank NX should not guess solely from file length. A safer strategy is:

```text
collect several occupied records
-> attempt candidate decrypt/parse/structural validation
-> score candidate native formats
-> accept inferred group only if one candidate wins unambiguously
-> otherwise ask for explicit type / fail closed
```

### PokeBank NX implementation split

The parser is now sufficiently specified to implement and fuzz independently of Master Vault:

1. build a read-only `PkseBankImporter` / decoder;
2. support unified `PKSEBANK` v1, older stored box counts, and allowed trailing metadata;
3. support known legacy flat-bank inputs;
4. expose legacy box/slot, persisted group, raw payload, parse status, and stable payload hash;
5. never modify/delete legacy data during scan/import;
6. keep destination creation separate until Master Vault is available.

The long-term migration remains:

```text
PKSE Storage
-> PokeBank NX read-only importer
-> immutable Master Vault object
-> provenance: Legacy PKSE Storage Import
-> optional user Banks / collections
```

Do **not** create a second long-term authoritative PokeBank bank merely to land this parser early.

---

## 2. Modern Switch save revision handling — do not use one filesize as revision authority

The most important conclusion from current PKHeX save-detection code is:

> A game's final installed update does not uniquely determine its physical save layout or filesize.

Upgrade path, optional blocks, and gameplay-created data can change valid save sizes. PokeBank NX therefore needs a game-specific revision detector abstraction rather than `game + one known size`.

Recommended shape:

```text
SaveRevisionDetector
  -> validate container / crypto / hash
  -> identify exact game family
  -> run game-specific revision logic
  -> expose capability flags
```

File size remains useful as an identification clue and rejection bound, but not the sole revision source.

Reference: PKHeX `SaveUtil.cs`

https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/Saves/Util/SaveUtil.cs

### Sword / Shield

PKHeX recognizes multiple historical layouts for different upgrade histories, including:

```text
1.0                         0x1716B3
1.0 -> 1.1                  0x17195E
1.0 -> 1.1 -> 1.2           0x180B19
1.0 -> 1.2                  0x180AD0
1.0 -> 1.1 -> 1.2 -> 1.3    0x1876B1
1.0 -> 1.1 -> 1.3           0x187693
1.0 -> 1.2 -> 1.3           0x187668
1.0 -> 1.3                  0x18764A
```

Accepted layout/size should be paired with Swish validation rather than blindly trusted.

DLC revision is better determined by actual Pokédex/DLC block presence:

```text
no Rigel1 data      -> Base
Rigel1 only         -> Isle of Armor
Rigel1 + Rigel2     -> Crown Tundra
```

Reference: PKHeX Zukan8

https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/Saves/Substructures/PokeDex/Zukan8.cs

### Scarlet / Violet

SV is even less suited to one-size detection. PKHeX has distinct valid early layouts for fresh saves, multiplayer-used saves, and different update paths. Example historical sizes include:

```text
0x31626F
0x31627C
0x319DB3
0x319DC0
0x319DC3
0x319DD0
0x31A2C0
0x31A2CD
0x31A2DD
0x31A2D0
```

DLC-era saves also contain many optional blocks, so valid ranges + hash/block validation are safer than one exact filesize.

Logical revision should be game-specific block presence:

```text
KBlueberryPoints present -> Indigo Disk / revision 2
else Kitakami raid data  -> Teal Mask / revision 1
else                     -> Base / revision 0
```

A useful regression lesson exists in PKSE: an earlier SV path attempted to read SCBlock key `0x0926555A` as a generic save revision because that key was copied from Z-A. That key is not the corresponding generic revision field for SV.

**PokeBank NX rule:** no generic “Gen 9 revision block.” Revision descriptors are per game.

### Legends Z-A

Z-A has an explicit revision block:

```text
KSaveRevision = 0x0926555A  // u64
0 = Base
1 = Mega Dimension
2 = later/EOL revision
```

Known container sizes include:

```text
1.0.0  0x2F3284
1.0.2  0x2F3289
2.0.0  0x309FA6
2.0.1  0x309FB3
```

Recommended sequence:

```text
container validity
-> SCBlock decrypt/hash
-> read 0x0926555A
-> enable revision-specific features
```

### Legends: Arceus

Known accepted encrypted layouts include approximately:

```text
0x136DDE
0x13AD06
```

The more useful revision signal is block presence:

```text
HasBlock(0x8184EFB4) ? Daybreak : Base
```

### BDSP

BDSP has a comparatively straightforward revision/size matrix:

```text
V1_0  0xE9828
V1_1  0xEDC20
V1_2  0xEED8C
V1_3  0xEF0A4
```

Validate the revision value stored by the save as well as the layout size. Capability gates can then use the explicit revision enum rather than heuristics.

### LGPE / Switch FRLG

LGPE currently uses a `0x100000` container with structural markers rather than a Sword/Shield-style revision matrix.

Switch FireRed/LeafGreen remains Generation III Pokémon/save semantics. The Switch-specific challenge is source/container discovery and validation, not inventing a modern SCBlock parser.

### Safety consequence

No modern installed-game write adapter should be approved until its exact revision detector and regression fixtures exist. Unknown/ambiguous revisions remain read-only / fail-closed.

---

## 3. Emulator discovery — resolver plugins, config first

The correct architecture is not a recursive list of guessed folders. Use one resolver per emulator/frontend family:

```text
RetroArchResolver
MGBAResolver
MelonDSResolver
DrasticResolver
AzaharResolver
TicoResolver
CustomFolderResolver
```

Each resolver should follow:

```text
probe installation/config
-> parse configured save directory
-> fallback to emulator default
-> fallback to ROM-adjacent behavior where supported
-> unwrap/normalize emulator-specific wrapper if necessary
-> feed candidate bytes to Pokémon save detector
```

### RetroArch on Switch

Source-level Switch defaults include:

```text
/retroarch
/retroarch/cores
/retroarch/cores/savefiles
/retroarch/cores/savestates
/retroarch/config
/retroarch/retroarch.cfg
```

Reference:

https://github.com/libretro/RetroArch/blob/master/frontend/drivers/platform_switch.c

Configuration schema/reference:

https://github.com/libretro/RetroArch/blob/master/retroarch.cfg

`savefile_directory` is configurable, and per-core/per-game overrides can supersede global configuration. Parse configuration first; use `/retroarch/cores/savefiles` only as a fallback.

Modern melonDS-libretro uses the RetroArch frontend save directory. Treat `melonDS core` and standalone `melonDS` as different resolvers.

### Standalone mGBA

mGBA exposes `savegamePath` / `savestatePath`; its Switch config is based on the project directory and loads `config.ini`.

Reference:

https://github.com/mgba-emu/mgba/blob/master/src/core/config.c

Resolver behavior:

```text
configured savegamePath if set
else ROM/content directory
-> <ROM basename>.sav
```

Do not assume `/switch/mgba/saves`.

### Standalone melonDS Switch

The Switch fork uses `melonDS.ini` / instance-specific config. Its ROM manager reads `SaveFilePath`; when empty, save files fall back beside the ROM. Cartridge saves use `.sav`.

Reference:

https://github.com/melonDS-emu/melonDS-switch

### DraStic Switch

A current native Switch wrapper documents:

```text
/switch/DrasticDS.nro
/switch/drastic/
    user/backup/
    user/savestates/
    games/
    slot2/
    launcher.ini
    drastic.ini
```

Reference:

https://github.com/NaGaa95/DrasticDS_nx

Use `user/backup` as an initial probe and content-detect the actual save. Do not hard-code `.dsv` solely because desktop/Android DraStic commonly uses it.

### Tico / Azahar

Tico has separate mGBA/Azahar core projects:

https://github.com/ticohq/tico-mgba

https://github.com/ticohq/tico-azahar

There is currently **not enough source proof** for a universal Tico GBA save root such as `/tico/saves/gba`. Do not hard-code an unverified path. Future work should audit Tico frontend↔core environment callbacks/configuration.

An experimental native Azahar Switch fork relocates its emulated filesystem beneath:

```text
sdmc:/switch/azahar/
    userdata/sysdata
    userdata/nand
    userdata/sdmc
```

Reference:

https://github.com/devgsantos/azahar-nx

This is useful architecture evidence but must not be assumed to equal Tico's layout. For 3DS, resolve the virtual SD/NAND root first, then derive title/save paths from the emulated filesystem model rather than recursively accepting random `.sav` files.

---

## 4. Native Switch save discovery — enumerate saves first, users second

JKSV remains a strong real-world reference for native save enumeration:

https://github.com/J-D-K/JKSV

libnx reference:

https://github.com/switchbrew/libnx

The important architecture lesson is to enumerate actual save-data records first, classify them, and then correlate owner UIDs to currently known users.

Do not begin from “enumerate the current profiles, then search inside each one.” That can hide or mishandle a save whose owner UID no longer maps to an active profile.

Recommended model:

```text
enumerate FsSaveDataInfo records
-> classify application/device/BCAT/cache/system/etc.
-> identify title/application
-> retain AccountUid if present
-> correlate UID to current profile when possible
-> represent unmatched owner as orphaned save rather than dropping it
-> only then attempt source-specific mount/open
```

JKSV's pseudo-user categories for device/BCAT/cache/system saves are useful UX/reference precedent.

This work must remain read-only until source-specific write safety is separately approved.

---

## 5. Golden corpus strategy — generate our own deterministic corpus

A ready-made public PKHeX test-data corpus covering all PokeBank NX needs was not found in the audited checkout. The stronger approach is to build our own deterministic corpus.

Recommended layers:

```text
generated blank saves
+ synthetic Pokémon-filled saves
+ legally redistributable historical-revision samples when available
+ programmatically corrupted variants
```

This extends the existing generated FRLG fixture approach already tracked in #17.

### PKSE bank fixtures

Generate at minimum:

```text
valid PKSEBANK v1
older/smaller box count
trailing optional metadata
bad magic
unsupported version
unknown persisted group
truncated header
truncated slot region
zero/partial payload
valid legacy flat bank
renamed/ambiguous legacy bank
```

### Modern save-revision fixtures

Generate/preserve cases for:

```text
old Sword/Shield upgrade-path layouts
SWSH DLC block-presence permutations
SV early-update/multiplayer/optional-block permutations
bad Swish hash
missing required block
duplicate SCBlock key
wrong block length
BDSP revision/size mismatch
PLA base vs Daybreak block presence
Z-A revision key variants
```

### Fixture metadata

Every generated fixture should have a deterministic manifest containing:

- generator input/seed;
- exact size;
- SHA-256;
- expected game identity;
- expected revision;
- expected semantic values;
- expected failure class for malformed cases.

Where coverage overlaps, compare the same fixture against independent engines/oracles (PokeBank NX, PKHeX/PKSM-Core where applicable, and another independent parser). A disagreement is an investigation, not a majority vote.

---

## 6. Vault crash/recovery design — keep the stronger immutable-generation model

Useful references include libnx filesystem commit behavior and real Switch bank projects such as pkHouse:

https://github.com/Insektaure/pkHouse

For Nintendo save-data mounts, libnx exposes `fsFsCommit`; writes to mounted save-data filesystems need an explicit commit. This is separate from the design of app-owned Vault files on SD.

For Master Vault on FAT32/exFAT, do not assume one rename equals a database-grade transaction. Preserve the stronger PokeBank design:

```text
immutable object payloads
-> write transaction into staging
-> hash/verify everything
-> write a new manifest generation
-> promote completed files
-> switch a tiny CURRENT/generation pointer last
-> keep previous valid generation

startup:
scan generations
-> choose newest completely valid generation
-> ignore incomplete transaction
-> rebuild derived indexes when necessary
```

No canonical Pokémon object should require in-place payload mutation.

pkHouse remains useful real-Switch precedent for validated bank headers, backups, trash/soft-delete behavior and synchronizing bank/save operations, but PokeBank NX Master Vault has stronger provenance and transactional requirements and should not simply copy its storage architecture wholesale.

---

## 7. Concrete work unlocked by this research

Priority order from this research pass:

1. **PKSE importer core** — format research is sufficiently complete to implement a read-only parser/fuzz harness now; Master Vault remains the eventual destination.
2. **`SaveRevisionDetector` abstraction** — required before trusting modern revision-specific behavior, especially any future writes.
3. **Resolver-based emulator discovery** — RetroArch, standalone mGBA, standalone melonDS, DraStic, Tico, Azahar, CustomFolder; configuration outranks defaults.
4. **Save-first native Switch discovery** — enumerate `FsSaveDataInfo`, then associate users/UIDs rather than assuming every save belongs to a currently active profile.
5. **Golden-corpus generator** — synthetic/blank/historical/malformed fixtures under deterministic CI.

### Research status change

```text
PKSE bank format:             IMPLEMENTATION-READY / TESTS NEEDED
Modern save revision model:   ARCHITECTURE + GAME-SPECIFIC DETECTORS DEFINED
Emulator discovery:           RESOLVER ARCHITECTURE DEFINED; Tico exact save root still needs audit
Native save enumeration:      SAVE-FIRST DESIGN IDENTIFIED
Golden corpus:                GENERATOR STRATEGY IDENTIFIED
Vault recovery:               EXISTING IMMUTABLE-GENERATION DESIGN REAFFIRMED
```

These findings are accelerators, not permission to weaken PokeBank NX's safety model. Original saves remain sacred; unknown layouts fail closed; live installed-game, RetroArch, and emulator-source writes stay disabled until separately proven and physically accepted.