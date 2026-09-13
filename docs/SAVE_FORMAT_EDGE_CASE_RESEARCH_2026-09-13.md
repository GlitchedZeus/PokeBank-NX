# PokeBank NX — Save Format, Discovery & Write-Safety Edge-Case Research

Date: 2026-09-13
Status: research/reference; implementation still requires exact source pinning + tests

This document preserves a second 2026-09-13 research pass that materially reduces uncertainty around Switch FireRed/LeafGreen, DraStic/Tico discovery, DS/3DS save architecture, Gen III serialization, native Switch save writes, save provenance, historical fixtures and conversion structure.

Important confidence rule: some findings below come from strong public implementation reports/release notes and are useful enough to guide architecture, but the implementation phase must still pin exact upstream revisions/URLs and add PokeBank NX tests before treating them as production truth.

Related tracking:

- #11 — modern Switch save adapters/revisions
- #17 — reproducible golden corpus
- #31 — Nintendo DS read adapters
- #32 — Nintendo 3DS read adapters
- #56 — universal emulator/source discovery
- #10 — conversion/transfer engine
- #20 — future true Move / safe write pipeline
- #49 — Bank → HOME preservation research

---

## 1. Switch FireRed / LeafGreen: likely ordinary 128 KiB Gen III save payload inside Switch save FS

This is the strongest architecture-changing result of this pass.

Reported evidence converges on the Switch releases using ordinary FRLG Gen III save bytes rather than a Nintendo-specific payload wrapper:

- hardware reports describe Checkpoint-exported files named like `FireRed_e.sav` / `LeafGreen_e.sav` for English;
- an old ordinary GBA FR/LG save reportedly loads after being renamed to the Switch filename and restored;
- PKHeX's 2026-02-27 release notes reportedly state that Switch Gen III FR/LG saves require no additional handling/conversion;
- pkHouse independently describes the payload as standard `0x20000` / 128 KiB GBA-sector save data.

Architectural implication:

```text
Switch save filesystem
    ↓
FireRed_e.sav / LeafGreen_e.sav (locale-specific filename as applicable)
    ↓
standard 0x20000-byte Gen III save
    ↓
ordinary FRLG sector parser/serializer
```

Do **not** invent a proprietary wrapper layer unless new evidence proves one exists.

PokeBank should reuse the same Gen III payload parser/serializer for:

```text
firered_gba
firered_switch
leafgreen_gba
leafgreen_switch
```

while keeping release/platform/source provenance identities separate.

Implementation acceptance still requires:

- exact filenames/locale variants;
- title-save filesystem discovery;
- byte-level comparison against known GBA FRLG save fixtures;
- parser round trip;
- independent PKHeX/pkHouse comparison;
- physical Switch validation.

---

## 2. DraStic Switch: `.dsv` is a filename convention until proven otherwise

Do not assume every `.dsv` is a DeSmuME-style wrapped save.

The new research indicates DraStic commonly accepts ordinary raw cartridge save data renamed to `.dsv`. Current Switch-port documentation places cartridge saves under:

```text
/switch/drastic/user/backup/
```

Recommended resolver behavior:

```text
DraStic candidate
    ↓
identify game / expected cartridge save size
    ↓
try Pokémon DS parser directly
    ↓
if invalid, detect known emulator footer/wrapper
    ↓
normalize only when proven necessary
```

Do not strip a footer merely because the extension is `.dsv`.

A particularly useful edge case from DrasticDS_nx history: a release reportedly fixed Pokémon Black/White 2 save corruption for German, Italian and Spanish ROMs because save-chip metadata was wrong. This reinforces:

```text
ROM/game identity + region/language
    → expected cartridge-save metadata/size
```

Extensions and filenames are hints, never identity authority.

---

## 3. Tico discovery: default path evidence improved, but remain version/profile aware

Historical Tico documentation reportedly uses the general hierarchy:

```text
sdmc:/tico/
    roms/<system>/
    saves/<system>/
    states/<system>/
    system/<system>/
```

Older releases used the former `tiicu` root. Newer core integrations also use `tico/system/...`, `tico/saves/...`, and per-core config files.

This makes:

```text
sdmc:/tico/saves/gba/
```

a **strong default probe** for modern Tico, not an unconditional source-of-truth path.

Resolver should remain version-aware:

```text
legacy tiicu
    sdmc:/tiicu/saves/...

modern tico
    sdmc:/tico/saves/...

possible profile-scoped behavior
    resolve through current/profile-specific source metadata
```

A 2026 Tico feature-request report suggests saves may already be associated with the Switch account/user. Treat this as an implementation question to verify, but design the resolver API so profile-aware discovery is possible.

Conceptually:

```text
discoverTicoSaves(EmulatorInstall install, optional<SwitchUid> profile)
```

not only:

```text
discoverTicoSaves("/tico/saves")
```

---

## 4. Tico/Azahar 3DS must use a virtual-3DS-filesystem resolver

Tico's Azahar integration reportedly uses:

```text
sdmc:/tico/system/3ds/
```

as its 3DS environment root/fallback, including Title-ID-oriented data such as cheats.

Therefore do not search for a flat file like:

```text
tico/saves/3ds/Game.sav
```

Treat the Tico/Azahar source as a virtual 3DS environment:

```text
tico/system/3ds/
    ↓
virtual NAND/SD/title layout
    ↓
installed title identity
    ↓
SaveData archive
    ↓
decrypted Pokémon save file such as `main`
```

This should share the same higher-level Azahar/3DS source model even if the frontend root differs.

---

## 5. 3DS save support: do not parse DISA for normal Azahar access

3DS storage has distinct concepts including normal save data, extdata, system save data, SD application data, gamecard save data and secure values.

At the physical/container level, 3DS save storage uses DISA with integrity metadata. But Azahar/Citra normally exposes a decrypted virtual filesystem to the game.

Therefore PokeBank NX should not build normal Azahar support around direct DISA parsing.

Preferred discovery path:

```text
find Azahar virtual NAND/SD roots
    ↓
resolve Pokémon Title ID
    ↓
resolve emulated SaveData archive
    ↓
read decrypted game save (`main` where applicable)
```

DISA knowledge remains useful as a low-level reference and for understanding integrity/rollback behavior, not as the default emulator adapter interface.

---

## 6. 3DS secure values are a real future write/restoration constraint

The research confirms Nintendo's save rollback-prevention / secure-value mechanism matters for Pokémon-era software. Reported documented users include Pokémon X/Y, OR/AS and Pokémon R/B/Y Virtual Console among others.

Commands in the platform model include secure-value get/set operations.

Implication:

- **Azahar/Citra:** the emulator may expose/neutralize this through its virtual FS; adapter behavior must be tested.
- **Real 3DS support later:** restoring only a copied `main` file is not automatically equivalent to a valid full restore; secure-value behavior must be handled or delegated to a proven manager/API.

Do not enable real-3DS write/restore support until secure-value behavior is part of the source-specific safety proof.

---

## 7. Generation IV DS save architecture: general/storage replicas must be resolved independently

The new research strongly warns against a naive `use first half / use second half` model.

For Diamond/Pearl-style Gen IV saves, two complete block-pair regions exist, with a second replica around `+0x40000`. Each pair contains a general block, storage block and Hall of Fame data.

The current general and current storage block must be selected independently from their save counters/linkage metadata.

Useful conceptual model:

```text
Gen4SaveReplica {
    GeneralBlock general;
    StorageBlock storage;
};

resolveCurrentGeneral();
resolveCurrentStorage();
```

not:

```text
bool useSecondHalf;
```

because the newest general and storage blocks may legitimately come from different replicas.

Reported footer shape:

```text
+00 u32 block-link value
+04 u32 save counter
+08 u32 block size
+0C u32 constant/runtime value
+10 u16 type/value
+12 u16 CRC16
```

with CRC-16-CCITT over the block excluding the footer.

Reported Pokémon sizes:

```text
boxed: 136 bytes
party: 236 bytes
```

Implementation must verify the exact game-specific offsets/sizes for D/P/Pt/HGSS instead of applying one universal Gen IV map.

---

## 8. Generation V DS architecture should be a separate block-table strategy

Do not create one generic `NDS save writer` for Gen IV and Gen V.

The research indicates:

```text
Gen IV
    replica/block-pair resolver

Gen V
    block table / many separately checksummed regions
    main + backup areas
```

B/W and especially B2/W2 use many independently checksummed regions. Project Pokémon mappings reportedly show separate records/checksums for box names, each PC box and other structures; community research also reports CRC-16-CCITT for B2/W2 blocks.

Recommended architecture:

```text
Gen4SaveAdapter
    replica resolver
    general/storage block checksums

Gen5SaveAdapter
    explicit region table
    per-region checksum metadata
    main/backup selection
```

Keep game-specific block maps data-driven and test every touched region.

---

## 9. Generation III staged writer: preserve the game's recovery semantics

The Gen III serializer contract is well understood enough to guide the future full staged Pokémon editor.

A valid Gen III save copy contains 14 logical sector IDs. Each 4 KiB physical section ends with metadata conceptually:

```text
0x0FF4  u16 section ID
0x0FF6  u16 checksum
0x0FF8  u32 signature = 0x08012025
0x0FFC  u32 save index
```

Sector ordering rotates between saves and checksum span depends on the logical section ID.

Two viable staged-export strategies:

### Minimal existing-slot mutation

```text
parse active slot
→ preserve physical sector mapping
→ modify affected logical data
→ recompute affected checksum(s)
→ preserve valid IDs/signature/index
→ reparse/validate
```

### Emulate a new in-game save

```text
start from current valid state
→ write the inactive replica
→ increment save index
→ use expected rotation
→ recompute all required checksums
→ verify all 14 logical sectors
→ leave the previous valid replica intact for recovery
```

For live-write design, prefer preserving a previous known-good save replica rather than blindly editing both copies identically unless physical tests prove another approach is safer.

---

## 10. Native Switch live-save writes: filesystem commit is mandatory

A future native Switch write adapter must explicitly commit the save filesystem after writes.

Research from Switchbrew/libnx states that unmounting does **not** implicitly commit save-data writes. The future pipeline therefore needs the equivalent of:

```text
backup
→ mount writable save
→ revalidate original fingerprint
→ bounded write(s)
→ close written files
→ fsFsCommit / fsdevCommitDevice
→ reopen/read back
→ parser validation
→ semantic verification
→ unmount
```

A path that simply writes/closes/unmounts is invalid.

This does **not** authorize live writes now. It adds a mandatory condition to the future source-specific safety gate.

---

## 11. Native Switch save enumeration/provenance should preserve richer SaveData metadata

Reported Switch `SaveDataInfo`/extra metadata exposes more than just title ID/user:

```text
SaveDataId
SaveDataSpaceId
SaveDataType
SaveDataUserId
SystemSaveDataId
ApplicationId
SaveDataSize
Index
Rank
State

OwnerId
TimeStamp
Flags
AvailableSize
JournalSize
CommitId
```

For provenance and change detection, preserve relevant source metadata such as:

```text
source ApplicationId
source UID
save-data ID/type
CommitId at read
source timestamp
```

These are acquisition/source diagnostics, **not Pokémon identity fields**.

`CommitId` may also provide an additional source-change/concurrency signal alongside size/time/hash fingerprints.

JKSV's support for account saves, device saves, BCAT, cache saves and system saves further supports a save-first model:

```text
enumerate records
→ classify save type / owner
→ attempt mount
→ preserve failed/unavailable records explicitly
```

Do not silently hide a record merely because mounting failed.

---

## 12. Modern historical fixtures remain a gap: explicitly allow synthetic revision fixtures

The research pass did not find one redistributable public corpus covering all desired historical paths such as:

```text
SWSH 1.0 → 1.1 → 1.3
SWSH 1.0 → 1.3
SV early fresh vs post-feature layouts
PLA base vs Daybreak
BDSP 1.0 / 1.1 / 1.2 / 1.3
Z-A base / Mega Dimension
```

Therefore issue #17 should not wait for a perfect public corpus.

Create synthetic/versioned fixtures where the format can be generated faithfully, and record provenance explicitly:

```json
{
  "game": "swsh",
  "revision": "1.3",
  "upgrade_path": ["1.0", "1.1", "1.3"],
  "size": "...",
  "sha256": "...",
  "synthetic": true,
  "expected": {
    "detected_revision": "..."
  }
}
```

The fixture manifest must distinguish:

- synthetic reconstruction;
- public redistributable real-layout sample;
- local-only developer fixture;
- malformed derivative.

---

## 13. Transfer/conversion engine: explicit generation transitions, not one giant converter

The new research reinforces that conversion rules accumulate many tiny transition-specific exceptions.

Do not build:

```text
Pokemon convert(Pokemon src, Generation dest)
```

as one opaque implementation.

Prefer explicit independently testable transitions:

```text
PK3 → PK4
PK4 → PK5
PK5 → PK6
PK6 → PK7
PK7 → HOME-compatible representation
...
```

Each transition can own rules for:

```text
PID preservation/regeneration
nature
gender
ability slot
ribbons
ball
met location
language/text encoding
moves / PP quirks
trash bytes
fateful encounter
origin game
handling trainer
HOME-era remapping/scalar fields where applicable
```

Preserve the immutable source Vault entity and create derived representations with explicit parent/provenance links.

---

## 14. Bank → HOME preservation: valuable regardless of exact shutdown date

This research pass did not independently recover a clean primary-source Nintendo/Pokémon announcement that should be used to certify an exact shutdown timestamp.

Keep #49 high priority, but distinguish:

- **preservation need:** unquestionably valuable/time-sensitive;
- **exact shutdown timestamp:** must be backed by a preserved primary-source announcement before being treated as canonical project fact.

High-value official-path capture metadata includes source/result hashes and all fields likely to transform across Bank → HOME, including origin, IDs, PID/EC, moves, ribbons, met data, ball, language, gender, nature, ability, HOME tracker and scalar/size fields.

Do not fabricate server provenance or HOME trackers.

---

## Engineering priority from this research

This pass materially changes/clarifies several future milestones:

1. **Switch FR/LG source validation becomes simpler**: treat payload as standard Gen III unless contradictory evidence appears.
2. **#56 gets stronger DraStic/Tico defaults**, but still validates content and remains config/profile aware.
3. **#31 should use separate Gen IV and Gen V adapter architectures**, not one generic NDS writer.
4. **#32 must model Azahar virtual SaveData and secure-value concerns.**
5. **Future Switch writes must explicitly call save-FS commit and read back.**
6. **Native provenance should retain save-record/CommitId metadata.**
7. **#17 should explicitly generate synthetic historical revision fixtures.**
8. **#10 should be transition-oriented.**

Current active development remains the Gen I staged boxed-Pokémon editor. These findings should reduce later implementation time, not derail the active milestone.

Current safety state remains:

```text
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE RETROARCH WRITES: HARD DISABLED
LIVE EMULATOR-SOURCE WRITES: HARD DISABLED
```
