# Save formats, structural revisions, profiles, containers and localization — 2026-09-09

> Research only. This file does not activate implementation scope. Verify exact source revisions before copying code/data.

This note consolidates the September 9 research sweep around save identification, update/DLC revisions, profile/save identity, emulator wrappers, RTC state and historical text encodings.

---

# 1. Structural save revision must be independent from installed game version

The strongest evidence came from current PKHeX save identification. Modern Switch Pokémon saves cannot safely be modeled as one fixed layout per title.

## Sword / Shield

Known structural sizes include upgrade-path-dependent variants such as:

```text
1.0                          0x1716B3
1.0 -> 1.1                   0x17195E
1.0 -> 1.1 -> 1.2            0x180B19
1.0 -> 1.2                   0x180AD0
1.0 -> 1.1 -> 1.2 -> 1.3     0x1876B1
1.0 -> 1.1 -> 1.3            0x187693
1.0 -> 1.2 -> 1.3            0x187668
1.0 -> 1.3                   0x18764A
```

Important implication: two users on the same final software version may still have structurally different saves because the save evolved through different patch histories.

## Scarlet / Violet

Research identified distinctions including:

```text
1.0.0 fresh
1.0.0 after multiplayer initialization
1.0.1 fresh
1.0.1 after multiplayer
1.1.0 fresh
1.0.1 -> 1.1
1.0.0 -> 1.0.1
1.0.0 -> 1.0.1 -> multiplayer
1.0.0 -> 1.0.1 -> 1.1
1.0.0 -> 1.1
... later configurations / DLC-era optional blocks
```

Later SV identification may accept size ranges because optional blocks can exist or not exist, then use SCBlock/SwishCrypto validity and block presence to finish classification.

Research also identified a Pokédex/block-model change around the 2.0.1 era, reinforcing that layout assumptions must be revision-specific.

## Brilliant Diamond / Shining Pearl

Known layout sizes:

```text
1.0.x     0xE9828
1.1.x     0xEDC20
1.2.x     0xEED8C
1.3.x     0xEF0A4
```

BDSP also stores revision information internally, and layout/model behavior changes by revision. Its save checksum work includes an MD5 region that must be handled deliberately when recomputing.

## Legends: Arceus

At least these structural sizes are known:

```text
0x136DDE
0x13AD06
```

Do not assume every PLA save is one universal layout.

## Legends Z-A

Current known structural sizes from the research pass:

```text
1.0.0    0x2F3284
1.0.2    0x2F3289
2.0.0    0x309FA6
2.0.1    0x309FB3
```

Revision-era changes have also touched representation details such as size/scale handling, so a parser that only recognizes the title but not the structural revision can appear partly correct while misreading some Pokémon data.

## Required architecture rule

Do not authorize parsing/writing based only on installed game version.

Preferred probe model:

```text
struct SaveProbeResult {
    GameId game;
    StructuralRevision revision;
    size_t canonicalLength;
    HashStatus hashStatus;
    BlockPresenceFingerprint blocks;
    MigrationFingerprint migration;
    bool writable;
    std::string reason;
};
```

Detection inputs may include:

```text
game/title identity
exact or accepted size range
known block/key presence
internal revision marker
required/optional block set
crypto/checksum validity
migration/layout fingerprint
```

Permanent rule:

```text
SUPPORTED_READ_WRITE
SUPPORTED_READ_ONLY
UNKNOWN_REVISION
CORRUPT
NOT_A_SAVE
```

Unknown structural revision => **read-only/unsupported**, never "probably latest".

Primary research baseline:

```text
kwsch/PKHeX — SaveUtil / title-specific save classes
```

---

# 2. Switch save-container identity is not the same thing as profile display identity

libnx exposes richer savedata attributes than a visible profile name or `AccountUid` alone.

Useful identity/container fields include:

```text
application ID
AccountUid
save-data type
save-data ID / index/rank
data size
journal size
owner ID
timestamp / flags
commit ID
```

Save categories include account, device, BCAT, temporary, cache, system and SystemBCAT-style saves.

Preferred conceptual split:

```text
struct SaveContainerRef {
    u64 applicationId;
    AccountUid uid;
    SaveDataType type;
    u8 rank;
    u16 index;
    u64 ownerId;
    u64 dataSize;
    u64 journalSize;
    u64 commitId;
};

struct ProfileRef {
    AccountUid uid;
    std::optional<std::string> currentDisplayName;
    bool accountResolvable;
};
```

## Deleted/orphaned profile case

A save can remain present even if its original UID no longer resolves through the current account service.

Do not hide or silently reassign it.

Desired UX direction:

```text
Orphaned profile save
Application: Pokémon Scarlet
UID: <stable identity>
Profile currently unavailable

Allowed:
    backup/export

Not automatic:
    attach to another profile
```

Backup owner identity and restore target identity should remain distinct concepts.

## Save-container metadata matters during restore

Research around mature Switch save managers found cases where restored payload bytes were valid but subsequent game writes failed because the target savedata container was configured with a different data size. This means safety/recovery work may need both:

```text
payload bytes
+
container metadata
```

References to inspect when this work becomes active:

```text
switchbrew/libnx
J-D-K/JKSV
FsSaveDataInfo / FsSaveDataAttribute / save extra-data APIs
```

---

# 3. Gen I/II SRAM and RTC must be modeled separately

Crystal and emulator ecosystems make it unsafe to assume one `.sav`/`.srm` file equals one canonical byte blob.

Research identified several RTC storage patterns:

```text
raw cartridge SRAM
mGBA-style appended RTC/trailer
SameBoy RTC representation
separate `.rtc` sidecar
RetroArch/Gambatte sidecar behavior
MiSTer/openFPGA representations
unknown trailing data
```

Permanent preservation rule:

> Understand enough to parse it; preserve everything you do not understand.

Preferred wrapper model:

```text
struct LegacySaveContainer {
    ByteVector canonicalSram;
    std::optional<RtcState> rtc;
    WrapperKind wrapper;
    ByteVector opaquePrefix;
    ByteVector opaqueSuffix;
};
```

or equivalently:

```text
RTCArtifact {
    producer format
    raw bytes
    optional normalized value
}
```

Important rules:

- do not blindly trim everything beyond a presumed SRAM length;
- if `pokemon_crystal.sav` and `pokemon_crystal.rtc` are paired, keep them associated;
- if RTC is appended, normalize logically but retain enough wrapper provenance/raw data to reconstruct the original form;
- unknown wrapper data survives untouched roundtrip;
- Gen II Japanese/international structures require explicit region/layout handling;
- Virtual Console provenance should be stored as import/source provenance, not inferred forever from filename alone.

References:

```text
pret/pokecrystal
SameBoy / BESS research
mGBA / libretro save+RTC behavior
PKHeX generation/region detection
```

---

# 4. Emulator/container normalization should happen before the Pokémon parser

Do not build a separate Pokémon save parser for every emulator wrapper.

Preferred pipeline:

```text
SaveDiscovery
    -> ContainerNormalizer
    -> GameSaveDetector
    -> StructuralRevisionDetector
    -> Generation/Title Parser
```

The normalizer can eventually understand:

```text
raw `.sav`
RetroArch `.srm`
DeSmuME `.dsv`
No$GBA compressed save
GameCube `.gci`
raw GameCube memory-card image
N64 endian/container variants
RTC sidecars/trailers
```

## DeSmuME `.dsv`

Research indicates `.dsv` is essentially:

```text
[raw NDS backup/save payload]
[DeSmuME metadata footer]
```

The footer contains save metadata and a recognizable cookie:

```text
|-DESMUME SAVE-|
```

Implementation direction:

```text
normalizeDeSmuME(file)
    -> raw cartridge save
    -> Sav4/Sav5 parser
```

So:

```text
Diamond.dsv
Diamond.sav
Diamond.srm
```

can converge on the same Gen IV parser.

## No$GBA compressed saves

Recognizable header:

```text
NocashGbaBackupMediaSavDataFile
```

The payload uses a comparatively simple compression scheme with literal/repeated runs. Normalize to a raw NDS backup image, then use the normal Gen IV/V parser.

## DraStic Switch provider

Current native Switch DraStic research:

```text
sdmc:/switch/drastic/user/backup/      normal cartridge saves
sdmc:/switch/drastic/user/savestates/ save states
```

Policy remains: support real cartridge/battery saves, not emulator savestates as a substitute.

## Azahar NX provider

Switch userdata root found in the research pass:

```text
sdmc:/switch/azahar/userdata/
```

Under it, follow the normal emulated 3DS SD tree toward:

```text
Nintendo 3DS/<ID0>/<ID1>/title/00040000/<TITLE-ID-LOW>/data/00000001/main
```

Known Pokémon title-low IDs recorded in the sweep:

```text
X           00055d00
Y           00055e00
Omega Ruby  0011c400
Alpha Sapphire 0011c500
Sun         00164800
Moon        00175e00
Ultra Sun   001b5000
Ultra Moon  001b5100
```

Never trust a file merely because it is named `main`; validate length + structure + checksums/revision.

---

# 5. Historical Pokémon strings are not just Unicode text

International string handling must preserve both semantic display text and historical bytes.

Relevant edge cases include:

```text
Gen I Japanese
Gen II Japanese
Gen II Korean
Gen III Japanese
Gen III Western accented characters
Gen IV Korean/Japanese
Gen V compatibility glyphs
ligatures
full-width characters
Nidoran gender symbol
embedded terminators
unterminated buffers
invalid bytes
maximum-length names
nickname unset/set behavior
OT names
trash bytes after shortened names
transfer-generated residual bytes
```

Preferred representation:

```text
struct EncodedPokemonString {
    std::u32string decoded;
    std::vector<uint8_t> originalBytes;
    EncodingId encoding;
    TerminatorPolicy terminator;
    std::vector<uint8_t> trailingBytes;
};
```

View-only invariant:

```text
parse -> no edit -> serialize == byte-identical original
```

Do **not** normalize every untouched nickname/OT into UTF-8/Unicode and then regenerate the whole field on save. That can erase legitimate historical/trash bytes used by real games and legality/transfer analysis.

Primary references to triangulate:

```text
pret/pokered
pret/pokecrystal
pret/pokeemerald
PKHeX StringConverter / generation-specific string logic
Project Pokémon trash-byte research
Pal Park transfer research
```

---

# 6. Test implications

When these adapters become active, fixture coverage should include:

```text
[REVISION]
all known SwSh migration paths
SV fresh/multiplayer/migration paths
BDSP 1.0/1.1/1.2/1.3
PLA known revisions
Z-A known revisions
unknown future revision

[PROFILE]
multiple users same title
orphan/deleted UID
account save
device save
container size mismatch

[WRAPPERS]
raw SAV
RetroArch SRM
DeSmuME DSV
No$GBA compressed
RTC sidecar
RTC trailer
unknown wrapper bytes

[TEXT]
JP/KR/Western encodings
terminator edge cases
trash preservation
exact no-edit roundtrip
```

The objective is not merely "parses current English save." The objective is safe structural identification and exact preservation across revisions, languages and source containers.
