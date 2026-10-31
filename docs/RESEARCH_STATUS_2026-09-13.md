# PokeBank NX — Research Status

Date: 2026-09-13

This is the short engineering status for the latest research passes. Detailed specifications live in:

```text
docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md
docs/SAVE_FORMAT_EDGE_CASE_RESEARCH_2026-09-13.md
```

The canonical map of research documents is `docs/RESEARCH_REFERENCE_INDEX.md`.

## What changed

The 2026-09-13 research materially reduced uncertainty across several roadmap areas:

| Area | Previous state | New research state | Tracking |
|---|---|---|---|
| PKSE Storage migration | format details still needed | **implementation-ready read-only parser contract; fuzz/legacy tests needed** | #27 |
| Modern Switch revisions | known major risk | **game-specific detector strategy identified** | #11 |
| Switch FireRed/LeafGreen | Switch wrapper/container uncertain | **strong evidence payload is standard 128 KiB Gen III save; source/provenance stays Switch-specific** | #11 |
| Universal emulator discovery | broad planned adapters | **resolver/config-first architecture; stronger DraStic/Tico/Azahar guidance** | #56 |
| Nintendo DS saves | broad Gen IV/V plan | **Gen IV replica resolver and Gen V block-table strategies now clearly separate** | #31 |
| Nintendo 3DS saves | title-path discovery still fuzzy | **virtual SaveData model + secure-value restore constraint identified** | #32 |
| Native Switch writes | generic future transaction plan | **save-FS commit is mandatory; richer SaveData metadata available for provenance/change detection** | #11/#20 |
| Golden test corpus | partial generated fixtures | **deterministic generator strategy expanded to synthetic historical revisions + malformed variants** | #17 |
| Conversion engine | generic cross-generation concept | **explicit transition-module architecture recommended** | #10 |

## PKSE importer

Current research freezes the unified v1 decoder around:

```text
16-byte header
magic: PKSEBANK
stored u32 version
stored u32 box count
30 slots per box
380-byte slot record
  4-byte persisted group tag
  376-byte payload
```

Persisted group IDs `0..7` are file-format values and must remain stable. Older headerless per-game bank migration behavior is also documented. The safe engineering split is to implement/test a read-only decoder before Master Vault, while deferring authoritative import destination creation until Vault exists.

## Modern Switch revisions

Use a shared detector interface with **game-specific** logic:

- SWSH: historical layout acceptance + Rigel DLC block presence;
- SV: optional/block-presence logic, not a generic Gen 9 revision field;
- PLA: Daybreak block `0x8184EFB4`;
- BDSP: explicit revision + layout matrix;
- Z-A: explicit `KSaveRevision = 0x0926555A`;
- LGPE: structural/container validation.

Unknown or ambiguous revisions remain read-only/fail-closed. This work is required before future live modern write adapters can be considered safe.

## Switch FireRed / LeafGreen

The newer research materially simplifies the expected payload model. Strong public evidence indicates Switch FR/LG exposes a normal `0x20000` / 128 KiB Gen III save file inside the Switch title's save filesystem, with no additional Pokémon-specific wrapper/conversion layer.

Recommended model:

```text
Switch save FS
→ locale-specific FR/LG .sav file
→ standard Gen III sector parser
```

Reuse the Gen III payload parser/serializer while keeping these identities distinct:

```text
firered_gba
firered_switch
leafgreen_gba
leafgreen_switch
```

Exact filenames/locale behavior and physical-device round-trip evidence still require implementation-time verification.

## Emulator discovery

Target resolver set:

```text
RetroArchResolver
MGBAResolver
MelonDSResolver
DrasticResolver
AzaharResolver
TicoResolver
CustomFolderResolver
```

Config outranks defaults. Standalone emulators and libretro cores are distinct when their save behavior differs.

New details:

- DraStic `.dsv` should first be treated as a candidate raw cartridge save and content-validated; do not strip a DeSmuME-style footer just because of the extension.
- Tico now has stronger evidence for `tico/saves/<system>/` defaults and historical `tiicu` roots, but exact behavior must remain version-aware.
- possible Tico profile/account scoping should be supported by resolver design rather than assuming one global user.
- Tico/Azahar 3DS should resolve a virtual 3DS environment rather than flat `.sav` files.

## Nintendo DS architecture

Do not build one generic NDS save writer.

Current direction:

```text
Gen IV
    independently resolve current general block
    independently resolve current storage block
    CRC/checksum + replica semantics

Gen V
    explicit region/block table
    per-block checksums
    main/backup regions
```

This distinction should be carried into #31 before any mutation support is designed.

## Nintendo 3DS architecture

For Azahar-style sources, resolve the virtual NAND/SD/title SaveData archive and read decrypted game files such as `main` where applicable. Do not make normal emulator support depend on direct DISA parsing.

For future real-3DS restoration/writing, secure-value rollback-prevention behavior is part of the safety proof. Writing only the game save file is not automatically a valid full restore.

## Native Switch save enumeration / writes

Future discovery should enumerate actual save records first, classify save-data type/title/UID, then correlate with current account users. Preserve relevant source metadata such as application ID, save-data ID/type, UID, timestamp and CommitId as acquisition provenance/change-detection inputs.

For any future native live write, save filesystem commit is mandatory:

```text
backup
→ mount writable save
→ revalidate source fingerprint
→ write bounded changes
→ close written files
→ fsFsCommit / fsdevCommitDevice
→ reopen/read back
→ parser + semantic verification
→ unmount
```

Unmount alone is not a commit.

## Golden corpus

Build deterministic generators instead of waiting for one giant public corpus:

```text
generated blank saves
+ synthetic populated saves
+ synthetic historical revision/layout fixtures
+ legally redistributable real-layout fixtures
+ programmatically malformed variants
```

Every fixture should preserve deterministic generator inputs, provenance class, size, SHA-256, expected identity/revision/semantics and expected failure class.

## Conversion engine

Prefer explicit independently testable transitions rather than one monolithic cross-generation converter:

```text
PK3 → PK4
PK4 → PK5
PK5 → PK6
PK6 → PK7
PK7 → HOME-compatible representation
...
```

Each transition owns its own PID/nature/gender/ability/ribbon/ball/met/language/move/provenance rules.

## Vault recovery direction

The research did not justify weakening the planned Master Vault design. Keep immutable Pokémon objects and generation-based transactional manifests:

```text
stage -> hash/verify -> new manifest generation -> promote -> switch CURRENT pointer last
```

Startup selects the newest completely valid generation, ignores incomplete transactions, and rebuilds derived indexes when necessary.

## Priority / scheduling

Do **not** derail the currently active Gen I staged boxed-Pokémon editor just because later work is now better researched.

Recommended use of the findings:

1. continue the active Gen I editor work;
2. implement/test PKSE read-only bank decoding when there is a clean core milestone;
3. land `SaveRevisionDetector` before any modern write work;
4. implement #56 resolver adapters with the new DraStic/Tico/Azahar rules;
5. implement #31 with separate Gen IV and Gen V architectures;
6. implement #32 around virtual SaveData first, secure-value-aware writes much later;
7. continuously expand #17 fixtures alongside every parser/revision;
8. use explicit generation-transition modules when #10 begins;
9. connect PKSE migration to Master Vault when Vault v1 lands.

## Safety remains unchanged

```text
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE RETROARCH WRITES: HARD DISABLED
LIVE EMULATOR-SOURCE WRITES: HARD DISABLED
```

Research evidence is not permission to write to user saves. Every write adapter still requires source-specific implementation, host/fuzz/sanitizer coverage, backup/commit/readback/rollback behavior, native validation and physical-device acceptance.