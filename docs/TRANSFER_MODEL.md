# PokeBank NX — Transfer Model

Status: DESIGN SPECIFICATION  
Last updated: 2026-09-01

PokeBank NX separates storage, conversion, transfer, and live save writing. A Pokémon moving between contexts should remain traceable, and a source should not disappear before the destination has been safely created and verified.

---

## Operation types

### Add to Master Vault

Store the selected Pokémon as a Vault entity while leaving the source untouched.

```text
Game/source Pokémon
      ↓ COPY
Master Vault

Source remains unchanged
```

This is the safest and most important first operation.

### Add to Bank

Add a reference to an existing/new Vault entity into a named logical Bank.

If the Pokémon is not already in the Vault, the app should normally perform an `Add to Master Vault` transaction first, then reference the resulting Vault ID.

### Clone

Create a new Vault entity with a new Vault ID and parent provenance. Initially the bytes may be identical.

### Copy to Game

Create/convert a destination-compatible representation while leaving the source entity intact.

Future live writing is a separate final stage governed by `SAVE_SAFETY.md`.

### Move to Game

A move is **copy + verified destination + explicit source removal/retirement policy**.

Never delete/remove the source before destination write and readback verification succeed.

### Convert

Create a derived Vault entity in a different Pokémon format/generation. Conversion alone does not write any game save.

### Export

Write a Pokémon/save representation to a user-controlled file/backup location without modifying an installed live game.

---

## Provenance principles

Every transfer/conversion should preserve:

```text
root/original Vault identity
original game/platform
source save fingerprint/context when available
parent entity
operation type
conversion engine/version
source raw format
result raw format
destination game identity if targeted
validation/legality result
timestamp
```

Origin is historical truth; current location is present organization/storage state.

Example:

```text
Charmander root
Origin: FireRed — Game Boy Advance
Format: PK3
Vault ID: A

A --convert--> B
B format: PK8
Target compatibility: Sword

B --copy-to-game--> Sword save

A still exists as the immutable root
```

---

## Generation conversion

Cross-generation conversion should use explicit adapters/rules rather than pretending all `.pk*` formats are interchangeable.

Target design:

```text
source entity
   ↓ parse
canonical semantic model / engine object
   ↓ destination conversion rules
serialize destination format
   ↓
validate
   ↓
create derived Vault entity
```

Where PKSM-Core provides mature native generation conversion, audit/integrate it rather than reimplementing blindly. Validate with the PKHeX Oracle.

---

## Loss/change reporting

Some transfers cannot preserve every source-generation property.

Before a user chooses a destination, PokeBank NX should eventually report material changes such as:

```text
move replacement/removal
ability/nature assignment due to later-generation representation
met/origin field transformation
ribbon/mark behavior
form restrictions
item incompatibility
species unavailable in destination
```

Early versions can be conservative: reject unsupported conversions rather than guess.

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

---

## Transfer result states

Use explicit states:

```text
SUPPORTED
SUPPORTED_WITH_CHANGES
UNSUPPORTED
UNKNOWN
```

`UNKNOWN` should not silently become `SUPPORTED`.

---

## Safe move semantics

A true move must follow:

```text
1. source exists
2. create destination-compatible derived entity
3. validate derived entity
4. stage destination save
5. validate staged save
6. create/verify destination backup
7. write destination if capability enabled
8. read destination back
9. verify Pokémon exists exactly as intended
10. record transfer success
11. only now apply explicit source-removal policy
```

For the Master Vault, the recommended default is to **retain immutable historical source entities** even when the user semantically "moves" a playable representation to a game.

The UI can distinguish archived history from currently active/organized specimens without destroying provenance.

---

## FireRed GBA ↔ Switch FireRed example

These are distinct game identities:

```text
firered_gba
firered_switch
```

The planned path is:

```text
FireRed GBA save
    ↓ read PK3
Master Vault root PK3
    ↓ destination adapter
Switch FireRed-compatible representation/container
    ↓ staged save
Switch FireRed
```

Do not assume the Switch title's outer save container is identical to a raw GBA save merely because it stores Gen III Pokémon data. Validate the platform/container separately.

pkHouse is a reference for regional Switch FR/LG title IDs/save filenames and observed `pk3` slot behavior; PokeBank NX should independently implement and test the adapter.

---

## HOME Bridge

PokeBank NX does not need to implement Pokémon HOME's private server protocol.

Supported architecture:

```text
PokeBank NX Vault
     ↓
compatible supported Switch game
     ↓
official Pokémon HOME application
```

Inbound:

```text
official Pokémon HOME
     ↓
compatible Switch game
     ↓
PokeBank NX reads game
     ↓
Master Vault
```

The preflight should check known game/species/form compatibility and PokeBank legality state, then use wording such as:

```text
HOME Bridge: No known compatibility issues
```

Never promise guaranteed HOME acceptance or ban immunity.

Preserve genuine HOME-specific identity/tracker data when present. Do not generate/fake HOME tracker/history fields.

---

## Legality vs legitimacy

PokeBank NX should keep the distinction clear:

```text
LEGAL
= data matches a configuration that could be valid under supported rules

LEGITIMATE
= actually obtained through the normal gameplay/distribution process claimed
```

The app can often test legality. It generally cannot prove real-world legitimacy from bytes alone.

Generated Pokémon should remain provenance-labeled as generated even if they are legal.

---

## Transfer journal

Each planned/finished transfer can record:

```json
{
  "transfer_id": "...",
  "source_vault_id": "...",
  "derived_vault_id": "...",
  "source_game": "firered_gba",
  "destination_game": "sword_switch",
  "operation": "copy_to_game",
  "state": "validated_not_written",
  "conversion_engine": "...",
  "warnings": [],
  "created_at": "..."
}
```

Possible states:

```text
PLANNED
CONVERTED
VALIDATED
STAGED
WRITTEN
READBACK_VERIFIED
COMPLETED
FAILED
ROLLED_BACK
```

---

## Action-sheet integration

Pressing A on a Pokémon opens a menu; it does not immediately transfer.

`Transfer to Game…` should lead to:

```text
choose destination
   ↓
compatibility/preflight
   ↓
show changes/warnings
   ↓
create/validate derived representation
   ↓
if live writing disabled:
  allow Vault/export/staged result only

if a future adapter has verified live-write capability:
  explicit confirmation → safe-write pipeline
```

---

## Minimum transfer milestone

Before any live writes, PokeBank NX can still complete a useful transfer-engine milestone:

- [ ] choose Vault entity
- [ ] select target game identity
- [ ] compatibility result
- [ ] convert to target Pokémon format
- [ ] store derived Vault entity
- [ ] record parent/conversion provenance
- [ ] validate against native engine
- [ ] compare against PKHeX Oracle
- [ ] export target `.pk*` representation
- [ ] no live game save modified

This should be completed before enabling direct save injection.
