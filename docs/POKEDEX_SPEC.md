# PokeBank NX — Pokédex / Living Collection Specification

Status: DESIGN SPECIFICATION  
Last updated: 2026-09-01

The PokeBank NX Pokédex is a collection view over the Master Vault, not a separate manual tracker that can drift out of sync.

pkDex is used as a UX/organization reference. PKHeX/game-derived data should remain the primary technical reference for species/forms/availability/legality-sensitive facts.

---

## Core rule

**The Master Vault is authoritative for what the user owns.**

Pokédex completion must be rebuildable from Vault entities at any time.

Manual notes may exist later, but they must not override the underlying ownership index.

---

## Views

PokeBank NX should eventually expose:

```text
National Dex
Regional Dexes
Living Dex
Shiny Living Dex
Forms
Gender Differences
Alpha / Shiny Alpha
Events
Missing
Completion Statistics
```

Regional/game views should be data-driven rather than hard-coded UI branches.

---

## Ownership states

For a species/form, derive at least these independent states where applicable:

```text
owned_normal
owned_shiny
owned_alpha
owned_shiny_alpha
owned_event
```

Future states can include marks/ribbons or special collection categories, but v1 should not become blocked on them.

`Seen` and `caught_in_game` are different concepts from Vault ownership. If PokeBank NX later reads game Pokédex flags, display them separately.

---

## Species identity

A Dex key must distinguish:

```text
species
form
regional form
meaningful gender difference
other permanent appearance/state required by the collection definition
```

Do not create separate completion entries for purely cosmetic/transient states unless explicitly chosen by the collection rules.

The collection rules must be versioned so a future data update cannot silently redefine a completed Living Dex.

---

## Shiny rules

A Shiny Living Dex entry is satisfiable only when the relevant species/form can legally be shiny under supported rules.

Possible states:

```text
MISSING
OWNED
SHINY-LOCKED / NOT APPLICABLE
UNKNOWN SUPPORT
```

Never require an impossible shiny for 100% completion.

Never mark a shiny-locked generated Pokémon as valid completion merely because bytes exist in the Vault.

---

## Alpha rules

Alpha/Shiny Alpha categories apply only to formats/games where the concept exists and can be meaningfully represented.

These should be additional collection lenses, not requirements for the basic National Living Dex unless the user selects such a goal.

---

## Event collection

Events should be indexed separately from ordinary species ownership.

An Event Vault/catalog can eventually track:

```text
event identity
distribution title/source
generation/game
language/region constraints
distribution dates
Wondercard/event file identity
owned normal/shiny variants where applicable
legality/support state
```

Possessing an ordinary specimen of the same species does not satisfy an event-specific entry.

---

## Regional Dex data model

A regional Dex definition should contain machine-readable entries such as:

```json
{
  "dex_id": "paldea",
  "display_name": "Paldea Pokédex",
  "game_family": "scarlet_violet",
  "entries": [
    {
      "regional_number": 1,
      "species": 906,
      "form": 0
    }
  ]
}
```

DLC Dexes should be separate definitions when that matches the game presentation.

Useful pkDex organization references include Kanto, FireRed/LeafGreen, Galar and DLC, Sinnoh/BDSP, Legends Arceus, Paldea and DLC, and Legends Z-A datasets.

---

## Data authority hierarchy

For technical fields used by legality/conversion/generation:

```text
1. PKHeX / extracted game-derived data
2. PKSM-Core or other independently tested implementation
3. PokeBank NX regression/device evidence
4. pkDex and community reference datasets as secondary cross-check/display inspiration
```

Do not promote community-readable location/evolution strings into legality rules without a technical source.

---

## Display information

The Pokédex detail view can show:

```text
National number
Regional number
Species/form name
Types
Normal/shiny art
Owned status
Evolution chain
Game/version availability
Locations
Shiny-lock state
Vault specimens
Origin breakdown
```

Data absent or unsupported must be shown as unknown/unavailable rather than invented.

---

## Vault linkage

Selecting an owned Dex entry should allow the user to open the matching Vault specimens.

Example:

```text
#006 Charizard
Normal: 3 owned
Shiny: 1 owned

Origins:
- FireRed GBA x1
- Sword x1
- Scarlet x2
```

A Dex entry should reference Vault IDs/index results instead of duplicating Pokémon payloads.

---

## Living Dex selection

When several Vault entities satisfy the same Dex slot, PokeBank NX may choose a preferred specimen for the visual Living Dex layout.

Initial deterministic preference could consider:

```text
explicit favorite/pinned specimen
then legitimate/imported provenance over generated where known
then oldest root provenance
then stable Vault-ID order
```

Do not silently delete or hide the other specimens; this only chooses which one represents the slot in the Living Dex view.

---

## Missing filter

The most useful action is often answering "what am I missing?"

Filters should eventually include:

```text
missing normal
missing shiny
missing forms
missing regional forms
missing meaningful gender variants
missing Alpha/Shiny Alpha
missing events
```

Completion counts must exclude impossible/not-applicable entries from the denominator.

---

## Fill Master Vault integration

The future Fill Master Vault feature should use the same collection-definition engine as the Pokédex.

Pipeline:

```text
collection definition
      ↓
calculate missing required entries
      ↓
for each supported missing entry
      ↓
select valid encounter template
      ↓
generate normal
      ↓
if legally shiny-capable and shiny collection requested
      ↓
generate shiny
      ↓
validate
      ↓
store as generated Vault entity with provenance
      ↓
rebuild/update Dex index
```

The operation must be idempotent: re-running it should not flood the Vault with duplicates.

---

## pkDex reference ideas to adapt

Useful concepts from pkDex include:

- region/DLC organization
- clean controller-oriented lists
- detailed Pokémon pages
- normal/shiny imagery
- shiny-lock visibility
- evolution and location display
- version exclusivity display
- bulk operations
- multi-select
- Alpha/Shiny Alpha categories
- localization structure

PokeBank NX should reimplement these ideas within its own architecture rather than copy GPLv2 source.

---

## Performance

Do not parse every raw Vault entity each time the Pokédex opens.

Use an index derived from Vault metadata/hash/provenance. The index must be disposable/rebuildable.

Expected operations:

```text
Vault transaction commits
      ↓
update affected species/form index

or

index missing/corrupt/version changed
      ↓
rebuild from Vault metadata/raw entities
```

---

## Minimum Pokédex v1 milestone

- [ ] National Dex list
- [ ] species name/number/type display
- [ ] Vault-driven normal ownership
- [ ] Vault-driven shiny ownership
- [ ] missing filter
- [ ] owned/missing counts
- [ ] select owned entry → matching Vault specimen(s)
- [ ] one regional Dex definition loaded from data
- [ ] index can be deleted and rebuilt without data loss
- [ ] host tests for ownership/index behavior
- [ ] native `.nro` builds

Later milestones add forms, gender differences, Alpha, events, detailed locations/evolutions, and collection generation.
