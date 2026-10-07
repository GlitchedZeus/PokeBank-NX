# PokeBank NX — Master Vault v1 Specification

Status: DESIGN SPECIFICATION  
Last updated: 2026-09-01

The Master Vault is the permanent, game-independent storage layer for PokeBank NX. It is not just another PC box. Its job is to preserve Pokémon data, identity, source context, lineage, and transformation history even after a Pokémon is copied, converted, edited, or transferred.

This document defines the v1 invariants before implementation begins.

---

## 1. Core invariants

1. **Original imported Pokémon bytes are immutable.**
2. **Every stored entity has a stable internal Vault ID.**
3. **Every raw payload is hashed with SHA-256.**
4. **Origin and current location are separate concepts.**
5. **Derived Pokémon never overwrite their parent.**
6. **A clone, edit, shiny transform, conversion, or generation creates traceable provenance.**
7. **Banks organize Vault entities; they are not the authoritative copy of the Pokémon.**
8. **Display metadata is a cache. Raw Pokémon bytes + provenance are authoritative.**
9. **UNKNOWN legality is never silently promoted to LEGAL.**
10. **All persistent mutations are transactional and recoverable.**
11. **Live game-save writing is outside the Vault transaction and remains disabled until the separate save-write safety contract is met.**

---

## 2. Terminology

### Vault Entity

One immutable raw Pokémon payload plus metadata/provenance describing that specific representation.

Examples:

- original FireRed GBA `PK3`
- derived Sword-compatible `PK8`
- derived shiny copy
- generated encounter-driven Pokémon

### Root Entity

The first Vault entity created from an external source such as a save, imported `.pk*` file, or supported event file.

### Derived Entity

A new entity created from a parent entity through conversion or transformation.

### Bank

A logical collection of references to Vault IDs. A Bank does not own the raw Pokémon payload.

### Current Location

Where the Vault believes the specific entity is currently represented or organized. This is not the same as the Pokémon's original game of origin.

### Provenance Event

An append-only record describing an import, clone, edit, conversion, transfer, generation, legality check, or other meaningful transition.

---

## 3. Proposed on-disk layout

The exact serialization can evolve, but v1 should preserve this separation:

```text
PokeBankNX/
  vault/
    manifest.json
    entities/
      <vault-id>.bin
    metadata/
      <vault-id>.json
    provenance/
      <vault-id>.json
    indexes/
      hash-index.json
      species-index.json
      source-index.json
    transactions/
      pending/
      journal/
  banks/
    <bank-id>.json
  backups/
    vault/
  config/
```

A later migration to SQLite or another indexed store is allowed only if the same invariants remain true and a safe migration path exists.

---

## 4. Vault ID

Vault IDs must be independent of species, game slot, filename, and hash.

Recommended representation:

```text
UUID v4 or equivalent 128-bit random identifier
```

Example:

```text
93ab52d0-9b72-44f4-aab3-37eea8cf1570
```

The raw hash is **not** the Vault ID because two independently imported records may contain identical raw bytes while having different source/provenance contexts.

---

## 5. Raw entity payload

`entities/<vault-id>.bin` stores the exact canonical raw Pokémon payload for that entity.

Required metadata:

```json
{
  "vault_id": "93ab52d0-9b72-44f4-aab3-37eea8cf1570",
  "schema_version": 1,
  "raw_format": "pk3",
  "raw_size": 80,
  "sha256": "...",
  "created_at": "..."
}
```

For a root import, the bytes written to the entity file must match the extracted/imported Pokémon representation exactly unless the source adapter explicitly documents a required canonicalization step. Any canonicalization must be recorded as provenance rather than hidden.

---

## 6. Metadata cache

Metadata exists to make browsing fast. It can include:

```text
species
form
gender
shiny
level
nickname
nature
ability
item
moves
OT
TID
SID
origin game
origin generation
language
Ball
legality summary
```

This metadata is **not authoritative**. If it disagrees with the raw entity, the raw bytes must be reparsed and the cache repaired.

Metadata should include:

```json
{
  "metadata_schema": 1,
  "parser_version": "...",
  "parsed_from_sha256": "..."
}
```

That allows PokeBank NX to know when cached fields were produced by an older parser.

---

## 7. Source provenance

A root import should capture as much source context as is safely available.

Example:

```json
{
  "source_kind": "game_save",
  "source_game_id": "firered_gba",
  "source_platform": "Game Boy Advance",
  "source_save_fingerprint": "sha256:...",
  "source_container": "RetroArch",
  "source_path_hint": "...",
  "source_box": 3,
  "source_slot": 12,
  "source_party_slot": null,
  "imported_at": "..."
}
```

Do not store secrets, credentials, console-unique authentication material, or unnecessary personal data in provenance.

---

## 8. Parent/child lineage

Every derived entity records one parent Vault ID.

Example:

```text
Root PK3
  93ab...
     |
     +-- convert-to-pk8 --> a120...
     |
     +-- make-shiny -----> 44fe...
```

The parent remains immutable.

A derived entity records:

```json
{
  "parent_vault_id": "93ab...",
  "operation": "convert",
  "operation_version": 1,
  "from_format": "pk3",
  "to_format": "pk8",
  "created_at": "..."
}
```

A future family-tree view can traverse these relationships without changing the storage model.

---

## 9. Clone semantics

`Clone` must create a new Vault ID even if the raw bytes are byte-identical.

The new entity should initially have:

- identical raw bytes
- identical SHA-256
- new Vault ID
- parent link to the source entity
- provenance event `clone`

This is intentional: identity/provenance differs even when bytes do not.

---

## 10. Edit semantics

Editing must not mutate the source Vault entity.

Preferred flow:

```text
source entity
    ↓
create working copy
    ↓
apply edit
    ↓
serialize
    ↓
validate
    ↓
create derived Vault entity
```

The provenance event should record the type of edit without needing to expose every internal implementation detail.

For meaningful fields, a structured change summary is desirable:

```json
{
  "operation": "edit",
  "changes": [
    {"field": "nickname", "before": "CHARMANDER", "after": "Blaze"}
  ]
}
```

---

## 11. Make Shiny semantics

`Make Shiny` creates a derived entity; it never overwrites the parent.

Before creation:

1. identify generation/origin constraints
2. check whether the claimed encounter can be shiny
3. apply generation-appropriate shiny/PID/ID rules
4. serialize
5. run legality validation

Results:

- known shiny lock → refuse with explanation
- supported and valid → derived shiny entity
- legality cannot be established → preserve `UNKNOWN`; do not present as legal

---

## 12. Conversion semantics

A conversion creates a derived entity with a new raw format.

Example:

```text
PK3 root
  ↓ convert
PK4 child
  ↓ convert
PK5 child
  ↓ convert
PK6 child
```

The project may optimize by generating the final compatible representation directly when the conversion engine supports it, but provenance must still record the actual conversion path/rules used.

The original representation remains permanently available.

---

## 13. Generated Pokémon

Generated Pokémon are allowed as Vault roots, but provenance must say they were generated.

Required provenance concepts:

```text
source_kind = generated
requested species/form/game
selected encounter template
engine/oracle version
generation constraints
legality result
```

Never label generated data as officially obtained.

---

## 14. Duplicate detection

Duplicate detection has multiple levels and should not collapse them into one concept.

### Byte duplicate

Same raw format + identical SHA-256.

### Likely same Pokémon

Different serialized representations that appear to represent the same underlying Pokémon after conversion. This requires generation-aware identity logic and should initially be advisory, not destructive.

### Intentional clone

Same bytes/identity but explicit clone provenance.

Default import behavior should **not silently discard data** merely because a hash already exists. Present/record the duplicate relationship and let policy determine whether another root provenance record is needed.

---

## 15. Banks

Banks are logical references.

Example:

```json
{
  "bank_id": "...",
  "name": "Living Dex",
  "schema_version": 1,
  "boxes": [
    {
      "name": "001-030",
      "slots": [
        "93ab...",
        null,
        "a120..."
      ]
    }
  ]
}
```

A Vault entity may appear in multiple banks without duplicating raw storage.

Examples:

```text
same Vault ID
  ├── Living Dex
  ├── Favorites
  └── Gen III
```

Deleting a bank does not delete Vault entities.

Permanent Vault deletion, if implemented at all, must be a separate explicit operation with recovery/backup semantics.

---

## 16. Pokédex indexing

The Master Vault is authoritative for collection ownership.

Pokédex indexes should be derived from Vault entities and metadata:

```text
Normal owned
Shiny owned
Form owned
Gender difference owned
Alpha owned
Shiny Alpha owned
Event owned
```

Rebuilding the Pokédex index from the Vault must always be possible.

---

## 17. Legality state

Allowed top-level states:

```text
LEGAL
INVALID
UNKNOWN
```

A legality record should include:

```json
{
  "state": "UNKNOWN",
  "engine": "pokebank-native",
  "engine_version": "...",
  "checked_at": "...",
  "warnings": [],
  "unsupported_checks": []
}
```

A later PKHeX Oracle comparison may be recorded as another check; it should not erase historical results.

---

## 18. Transaction model

A Vault insertion/modification transaction should follow:

```text
BEGIN
  ↓
create transaction journal
  ↓
write payload to temporary path
  ↓
fsync/close where supported
  ↓
calculate SHA-256
  ↓
parse/validate payload
  ↓
write metadata/provenance temporary files
  ↓
atomically publish entity files
  ↓
update indexes/bank references
  ↓
verify published files
  ↓
COMMIT journal
```

If any stage fails:

```text
ABORT
  ↓
remove unpublished temporary files
  ↓
restore previous index/bank state
  ↓
keep diagnostic journal if useful
```

No partially published Vault entity should appear in normal browsing.

---

## 19. Crash recovery

On startup:

1. inspect pending transaction journals
2. determine whether each transaction committed, partially published, or never published
3. verify hashes for any published entity
4. finish only provably safe metadata/index publication steps
5. otherwise roll back references and quarantine incomplete data

Never guess that a partially written payload is valid.

---

## 20. Schema migration

Every persistent JSON/object format must carry a schema version.

Migration rules:

- migrations are explicit and versioned
- make a Vault metadata backup before migration
- raw Pokémon payload files are not rewritten merely to migrate metadata
- migrations are idempotent or record completion
- unknown newer schema versions open read-only rather than being overwritten

---

## 21. Performance targets

The Vault should support a very large collection without loading all raw Pokémon into RAM.

Use:

- lazy entity loading
- compact indexes
- bounded caches
- metadata-first browsing
- background/index rebuild stages where safe
- no full-Vault raw scan on every app launch unless recovery/integrity verification requires it

---

## 22. Minimum Vault v1 milestone

Vault v1 is complete only when all of the following are true:

- [ ] create/open Vault storage
- [ ] import a Pokémon from a read-only source
- [ ] preserve exact raw payload
- [ ] compute/store SHA-256
- [ ] assign stable Vault ID
- [ ] store source provenance
- [ ] re-open after app restart
- [ ] browse entity from Vault
- [ ] create named Bank referencing Vault IDs
- [ ] put same entity in multiple Banks without duplicating raw payload
- [ ] clone into a new Vault ID with parent provenance
- [ ] recover cleanly from interrupted simulated transaction
- [ ] host regression tests pass
- [ ] sanitizer tests pass
- [ ] native `.nro` builds
- [ ] physical Switch read-only Vault test passes before any live-save-write work proceeds

---

## 23. Explicit non-goals for Vault v1

Do not block Vault v1 on:

- full family-tree UI
- every generation conversion
- direct Pokémon HOME connectivity
- live save injection
- complete event archive
- DS/3DS source discovery
- advanced cloud synchronization

Get immutable storage, provenance, banks, and recovery correct first.
