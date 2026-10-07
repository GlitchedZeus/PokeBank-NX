# PokeBank NX — Transfer Model

Status: DESIGN SPECIFICATION  
Last updated: 2026-09-02

PokeBank NX separates storage, conversion, transfer, active location, archival provenance, and live save writing.

The product goal is to support **real Pokémon movement** between games and the Master Vault/Banks once the relevant save adapters are proven safe. A Move must feel like a move to the user: after success, the Pokémon is active in the destination and no longer active in the source.

At the same time, no source should disappear before the destination, backup, validation, and readback have succeeded.

---

## Current alpha rule

**Installed game saves are read-only.**

Therefore current early milestones may implement/import with safe COPY semantics only:

```text
Game/source Pokemon
      | COPY / IMPORT
      v
Master Vault

Source remains unchanged
```

This is temporary safety behavior while parsers, the Master Vault, conversion engines, staged-save validation, and per-game write adapters are built.

Do **not** enable true Game -> Vault or Vault -> Game Move merely because the UI has a Move command.

Tracking issue for later true Move: **#20**.

---

## User-facing operation types

### Copy

Create another playable representation while leaving the source active.

```text
COPY = source remains + destination is created
```

Copy must be explicit. It should not silently happen when the user chose Move.

### Move

Relocate the active Pokémon from one location to another.

```text
MOVE = destination becomes active + source stops being active
```

A Move may preserve immutable archival/provenance records behind the scenes for recovery and history, but those records are **not another active playable copy**.

### Clone

Create an intentional duplicate with a new Vault identity and clone provenance.

```text
CLONE = deliberate duplicate
```

Clone is distinct from Copy and Move because lineage/provenance explicitly records that the new entity was created as a clone.

### Add to Master Vault

Current-alpha behavior: safely import/copy the selected Pokémon into the Vault while leaving a live game source untouched.

Future behavior may offer both:

```text
Copy to Master Vault
Move to Master Vault
```

when the source game's removal/write adapter has passed its safety gate.

### Add to Bank

Banks are logical organization over Vault identities. Adding an existing Vault Pokémon to another Bank does not create another raw payload.

A Bank reference is not automatically another active Pokémon copy.

### Copy to Game

Create/convert a destination-compatible representation and, once live destination writes are supported for that game, write it while leaving the source active.

### Move to Game

Create/convert and verify the destination first, then retire/remove the source active placement only after destination write + readback verification succeed.

### Convert

Create a derived Vault entity in another Pokémon format/generation. Conversion alone does not write a game save and does not automatically change the active location.

### Export

Write a Pokémon/save representation to a user-controlled file/backup location without modifying an installed live game.

---

## Active location versus archival history

PokeBank NX needs two separate concepts:

```text
historical/provenance record
```

and

```text
where the Pokemon is currently active from the user's point of view
```

Suggested active-location states:

```text
GAME
VAULT
BANK/Vault placement
STAGED
ARCHIVED_HISTORY
UNKNOWN/RECOVERY
```

An immutable root entity may remain in archival history after a successful true Move. That archival record exists for recovery, provenance, hashes, and lineage; it should not appear as a second active Pikachu that the user can independently move again without an explicit Copy/Clone operation.

The Summary/Provenance UI should eventually show both:

```text
Origin: FireRed — GBA
Active location: Master Vault / Living Dex
History: moved from FireRed on <date>
```

---

## Provenance principles

Every import/transfer/conversion should preserve, where available:

```text
root/original Vault identity
original game/platform
source save fingerprint/context
source box/slot/party slot
parent entity
operation type
copy/move/clone semantics
conversion engine/version
source raw format
result raw format
destination game identity
validation/legality result
active-location transition
timestamp
```

Origin is historical truth. Active/current location is present state.

---

## Generation conversion

Cross-generation conversion uses explicit adapters/rules rather than pretending all `.pk*` formats are interchangeable.

Target design:

```text
source entity
   v parse
canonical semantic model / engine object
   v destination conversion rules
serialize destination format
   v
validate
   v
create derived Vault representation
```

Where PKSM-Core provides mature native conversion behavior, audit/integrate it rather than reimplementing blindly. Validate with the PKHeX Oracle where available.

---

## Loss/change reporting

Some transfers cannot preserve every source-generation property.

Before a user chooses a destination, report material changes such as:

```text
move replacement/removal
ability/nature assignment
met/origin field transformation
ribbon/mark behavior
form restrictions
item incompatibility
species unavailable in destination
move incompatibility/replacement
```

Early versions should be conservative: reject unsupported conversions rather than guess.

---

## Destination compatibility

A Pokémon is not transferable merely because PokeBank NX can serialize the destination format.

Compatibility must consider:

```text
species present in destination game
form present/supported
required transfer transformations
moves/items/abilities supported or transformed appropriately
encounter/origin preservation rules
special restrictions
```

The compatibility matrix should be data-driven and versionable.

Transfer result states:

```text
SUPPORTED
SUPPORTED_WITH_CHANGES
UNSUPPORTED
UNKNOWN
```

`UNKNOWN` must not silently become `SUPPORTED`.

---

## True Move: Game -> Vault / Bank

Once a specific source-game adapter has independently passed live-write safety gates, a true Move from a game should follow:

```text
1. read source Pokemon
2. create destination Vault entity
3. hash + parse + validate destination entity
4. persist transaction/provenance journal
5. create and verify a backup of the source game
6. stage a clone of the source save with Pokemon removed
7. repair generation/game-specific checksums/containers
8. reparse staged source save
9. verify intended removal and no unrelated mutation
10. write source only through the approved adapter
11. read source game back
12. verify Pokemon is absent exactly where intended
13. mark Vault/Bank as the active user-facing location
14. commit transfer journal
15. retain rollback/provenance history
```

If any verification fails, abort/rollback and leave the Pokémon active in the source game.

Never delete from the source first.

---

## True Move: Vault / Bank -> Game

Once a destination-game adapter is approved for writes:

```text
1. select active Vault entity
2. choose destination game/slot
3. preflight compatibility
4. create destination-compatible representation if needed
5. validate derived representation
6. create and verify destination-game backup
7. stage destination insertion
8. repair checksums/containers
9. reparse/validate staged destination save
10. write through the approved adapter
11. read destination back
12. verify Pokemon exists exactly as intended
13. only now retire/remove the active Vault/Bank placement for Move
14. preserve archival/provenance lineage
15. commit transfer journal
```

For **Copy to Game**, step 13 does not retire the source active placement.

---

## Game -> Game true Move

A game-to-game Move is conceptually a coordinated destination-first operation:

```text
Game A
  | read
  v
Vault/staged transfer representation
  | validate/convert
  v
Game B destination write + readback verification
  |
  v
only then remove from Game A through its approved adapter
```

If Game B succeeds but Game A removal cannot be safely completed, report a **partial transfer requiring recovery** rather than pretending the Move completed. Preserve enough journal state to resolve the duplicate safely.

Direct Game A -> Game B choreography may be optimized later, but the safety ordering must remain destination-first.

---

## Banks and Move semantics

Banks are organizational references over the Master Vault, not independent raw-storage silos.

A Pokémon can be visible in organizational categories such as:

```text
Living Dex
Favorites
Gen III
```

without creating three playable copies.

A true Move into or out of the Vault should update active-location/reference state transactionally.

Deleting a Bank reference is not equivalent to deleting the Pokémon from the Vault or a game.

---

## FireRed GBA -> Switch example

GBA and Switch titles are distinct game identities. Do not assume identical outer save containers.

Planned high-level path:

```text
FireRed GBA save
    v read PK3
Master Vault / transfer representation
    v destination adapter
Switch-compatible representation/container
    v staged save
Switch destination
```

Current stages remain read-only/staged until each platform adapter is validated.

---

## HOME Bridge

PokeBank NX does not need to impersonate Pokémon HOME or directly connect to private Nintendo/Pokémon protocols.

Supported concept:

```text
PokeBank NX Vault
     v
compatible supported Switch game
     v
official Pokémon HOME
```

Inbound:

```text
official Pokémon HOME
     v
compatible Switch game
     v
PokeBank NX reads game
     v
Master Vault
```

Preserve genuine HOME-specific identity/tracker data when present. Do not generate/fake HOME trackers or history. Use wording such as `No known compatibility issues`, never guaranteed acceptance/ban safety.

---

## Legality versus legitimacy

```text
LEGAL
= data matches a configuration that could be valid under supported rules

LEGITIMATE
= actually obtained through the normal gameplay/distribution process claimed
```

PokeBank NX can often evaluate legality but generally cannot prove real-world legitimacy from bytes alone.

Generated Pokémon remain provenance-labeled as generated even if legal.

---

## Transfer journal

Each operation should have an auditable journal entry.

Example fields:

```text
transfer_id
source_vault_id / source game identity
source active location
derived_vault_id
destination game/location
operation: copy | move | clone | convert
state
conversion engine/version
warnings
backup identity
created_at / completed_at
```

Possible states:

```text
PLANNED
CONVERTED
VALIDATED
STAGED
DESTINATION_WRITTEN
DESTINATION_READBACK_VERIFIED
SOURCE_REMOVAL_STAGED
SOURCE_REMOVED
SOURCE_READBACK_VERIFIED
COMPLETED
FAILED
PARTIAL_RECOVERY_REQUIRED
ROLLED_BACK
```

---

## Action-sheet integration

Pressing A on a Pokémon opens a deliberate menu; it never immediately transfers/removes.

Future transfer UI should explicitly separate:

```text
Copy to Game...
Move to Game...
Copy to Master Vault
Move to Master Vault
Clone
```

Only show/enable Move when the source and destination capabilities required for that move are actually verified.

When live writing is disabled, offer only safe Vault/export/staged results and explain why Move is unavailable.

---

## Minimum pre-live-write transfer milestone

Before any true live Move is enabled, PokeBank NX can still complete:

- [ ] choose Vault entity
- [ ] select target game identity
- [ ] compatibility result
- [ ] convert to target Pokémon format
- [ ] store derived Vault representation
- [ ] record parent/conversion provenance
- [ ] validate against native engine
- [ ] compare against PKHeX Oracle where available
- [ ] export target `.pk*` / staged result
- [ ] no live game save modified

---

## True Move enablement gate

A particular source/destination adapter may participate in true Move only after its relevant read/write safety gates are independently satisfied.

One proven adapter does not authorize every game.

Dependencies include, as applicable:

```text
Master Vault v1
read-only parser validation
untouched round-trip behavior
conversion compatibility
staged save mutation tests
backup + rollback
reparse validation
write/readback verification
malformed/unsupported-version rejection
physical Switch testing
```

Until then, the project stays safe and uses COPY/import/staged semantics.
