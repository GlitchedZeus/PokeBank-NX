# PokeBank NX — Universal Pokémon Randomizer ZX Reference Audit

Date: 2026-09-03  
Scope: long-term research/reference only; no PokeBank NX application source changed.

Reference:

- `Ajarmar/universal-pokemon-randomizer-zx`
- default branch inspected: `master`
- license: GNU GPL v3 or later

## What UPR-ZX is

Universal Pokémon Randomizer ZX is primarily a multi-generation **ROM/game-data randomizer and editor**, not a Pokémon save parser, Bank, Vault, or live-save transfer engine.

Its README describes a long-running fork of the original Universal Pokémon Randomizer with expanded features and 3DS support.

The inspected tree contains dedicated generation handlers for:

```text
Gen1RomHandler
Gen2RomHandler
Gen3RomHandler
Gen4RomHandler
Gen5RomHandler
Gen6RomHandler
Gen7RomHandler
```

plus common platform abstractions:

```text
AbstractGBRomHandler
AbstractGBCRomHandler
AbstractDSRomHandler
Abstract3DSRomHandler
RomHandler
AbstractRomHandler
```

This makes UPR-ZX valuable as an **independent game-data architecture/oracle** for generations I–VII.

## Highest-value architecture lesson

`RomHandler.java` defines one broad common interface implemented by game/generation-specific handlers.

The interface separates common concepts such as:

```text
load/save game container
ROM validity/diagnostics
Pokémon + forms
starters
base stats
abilities
wild encounters
trainer Pokémon
items
moves
trade data
other generation-specific capabilities
```

PokeBank NX should not copy this interface literally: our product operates primarily on saves/Pokémon/Vault objects rather than randomized ROMs.

However, the pattern strongly supports our planned adapter model:

```text
common capability interface
        ↓
per-generation / per-title implementation
        ↓
capability queries instead of giant title-specific conditionals
```

Long-term PokeBank NX equivalent should remain closer to:

```text
GameAdapter
  identity
  source kinds
  Pokémon format
  READ_PARTY
  READ_BOXES
  EXPORT_PKM
  VAULT_IMPORT
  CONVERT_IN / CONVERT_OUT
  STAGED_WRITE
  LIVE_WRITE
  validation
  diagnostics
```

with unsupported capabilities remaining explicitly unavailable.

## Data/reference value

The repository contains large generation-specific knowledge bases including:

```text
Gen1Constants ... Gen7Constants
Species / Moves / Items / Abilities
Evolution + EvolutionType
Encounter / EncounterSet
StaticEncounter
Trainer / TrainerPokemon
IngameTrade
FormeInfo
MegaEvolution
Type data
```

and configuration/offset files for:

```text
gen1_offsets.ini
gen2_offsets.ini
gen3_offsets.ini
gen4_offsets.ini
gen5_offsets.ini
gen6_offsets.ini
gen7_offsets.ini
```

This is useful as an independent cross-check for future:

- Pokédex availability/data;
- encounter/location context;
- form/evolution behavior;
- game/version capability differences;
- DS/3DS game-data interpretation;
- compatibility/legality supporting metadata.

PKHeX remains the primary correctness oracle for Pokémon/save semantics. UPR-ZX should not become the authority for save legality or `.pk*` conversion.

## DS / 3DS technical value

UPR-ZX also contains container/data helpers such as:

```text
NDSRom
NDSFile
NARCArchive
CRC16
GARCArchive
NCCH
RomfsFile
N3DS text handlers
```

These are ROM/data-container tools, not save-file adapters, but they may help understand Gen IV–VII static game data and provide independent evidence when implementing #31/#32.

## Important limits

Do **not** infer from UPR-ZX that PokeBank NX now has:

```text
save parsing
Pokémon Bank storage
Vault support
live save writes
PKM legality
cross-generation transfer
Switch Gen VIII/IX support
```

The inspected source tree has generation handlers through Gen VII. A comment in the generic interface mentions game updates for 3DS/Switch games, but no Gen VIII/IX handler was observed in the inspected tree; do not claim modern Switch support from that comment.

## Licensing / reuse

The repository is GPLv3-or-later. PokeBank NX is AGPLv3, so any direct reuse still needs deliberate compatibility/notice review and should not be assumed merely because both are copyleft.

Because UPR-ZX is Java while PokeBank NX is native C++, the likely role is:

```text
REFERENCE / INDEPENDENT DATA ORACLE — HIGH VALUE
ARCHITECTURE PATTERN REFERENCE — HIGH VALUE
SELECTIVE TRANSLATION — POSSIBLE AFTER LICENSE/TEST REVIEW
DIRECT RUNTIME DEPENDENCY — NO
```

## Best future use

When implementing a generation adapter, compare:

```text
PKSM-Core / PKSM
PKHeX
pret game-source work where available
UPR-ZX game-data handler/constants
PokeBank NX golden fixtures
```

UPR-ZX is especially useful when the question is about **what exists in a game/version and how game data differs across generations**, while PKSM-Core/PKHeX remain stronger for save/Pokémon entity semantics.

## Roadmap placement

This reference does not change Session 2.6 or the current v1 critical path.

Useful future issue relationships:

```text
#7   Vault-driven Pokédex
#9   Summary / encounter-location context
#10  compatibility/conversion supporting metadata
#31  Nintendo DS Gen IV/V adapters
#32  Nintendo 3DS Gen VI/VII adapters
```
