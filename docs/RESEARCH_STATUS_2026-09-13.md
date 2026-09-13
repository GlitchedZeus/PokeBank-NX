# PokeBank NX — Research Status

Date: 2026-09-13

This is the short engineering status for the latest research pass. Detailed specifications live in `docs/SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`; the canonical map of research documents is `docs/RESEARCH_REFERENCE_INDEX.md`.

## What changed

The latest research materially reduced uncertainty in four roadmap areas:

| Area | Previous state | New research state | Tracking |
|---|---|---|---|
| PKSE Storage migration | format details still needed | **implementation-ready read-only parser contract; fuzz/legacy tests needed** | #27 |
| Modern Switch revisions | known major risk | **game-specific detector strategy identified** | #11 |
| Universal emulator discovery | broad planned adapters | **resolver/config-first architecture with several source-proven defaults** | #56 |
| Golden test corpus | partial generated fixtures | **deterministic generator strategy expanded to PKSE + modern revisions + malformed variants** | #17 |

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

## Modern save revisions

Use a shared detector interface with **game-specific** logic:

- SWSH: historical layout acceptance + Rigel DLC block presence;
- SV: optional/block-presence logic, not a generic Gen 9 revision field;
- PLA: Daybreak block `0x8184EFB4`;
- BDSP: explicit revision + layout matrix;
- Z-A: explicit `KSaveRevision = 0x0926555A`;
- LGPE: structural/container validation;
- Switch FR/LG: Gen III semantics + Switch source/container discovery.

Unknown or ambiguous revisions remain read-only/fail-closed. This work is required before future live modern write adapters can be considered safe.

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

Config outranks defaults. Standalone emulators and libretro cores are distinct when their save behavior differs. Tico exact save-root behavior remains one of the unresolved discovery details and must not be guessed.

## Native Switch save enumeration

Future discovery should enumerate actual `FsSaveDataInfo` records first, classify save-data type/title/UID, then correlate to current account users. This allows orphaned/deleted-profile ownership and device/BCAT/cache/system categories to be represented instead of silently disappearing.

## Golden corpus

Build deterministic generators instead of waiting for one giant public corpus:

```text
generated blank saves
+ synthetic populated saves
+ legally redistributable historical layouts
+ programmatically malformed variants
```

Every fixture should preserve deterministic generator inputs, size, SHA-256, expected identity/revision/semantics and expected failure class.

## Vault recovery direction

The research did not justify weakening the planned Master Vault design. Keep immutable Pokémon objects and generation-based transactional manifests:

```text
stage -> hash/verify -> new manifest generation -> promote -> switch CURRENT pointer last
```

Startup selects the newest completely valid generation, ignores incomplete transactions, and rebuilds derived indexes when necessary.

## Priority / scheduling

Do **not** derail the currently active editor milestone just because later work is now research-ready.

Recommended use of the findings:

1. continue the active classic editor work;
2. implement the PKSE read-only bank decoder when there is a clean parallel/core milestone;
3. land `SaveRevisionDetector` before any modern write work;
4. implement #56 resolver adapters as universal discovery becomes active;
5. continuously expand #17 fixtures alongside every new parser/revision;
6. connect PKSE migration to Master Vault when Vault v1 lands.

## Safety remains unchanged

```text
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE RETROARCH WRITES: HARD DISABLED
LIVE EMULATOR-SOURCE WRITES: HARD DISABLED
```

Research evidence is not permission to write to user saves. Every write adapter still requires source-specific implementation, host/fuzz/sanitizer coverage, backup/readback/rollback behavior, native validation, and physical-device acceptance.