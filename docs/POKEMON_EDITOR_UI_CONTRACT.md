# PokeBank NX — Universal Pokémon Editor UI Contract

This document freezes the permanent UI/field-availability rules for the reusable Pokémon editor. Future generation adapters must follow this contract without re-deciding the editor philosophy generation by generation.

## Core rule

The editor is PKSE-style and capability-driven.

**If a Pokémon field genuinely exists in the exact current game/save format, the reusable editor must expose it in the shared Create/Edit/View layout. If the field does not exist in that format, it must be hidden rather than fabricated.**

Field availability is determined by the Pokémon's **current game/save format and revision**, not by the generation in which the species originally debuted.

Example: Charmander in Pokémon Red has no Held Item, Nature, Ability, generic Friendship, Egg/met metadata, Pokérus, Ribbons, or Marks because those concepts are not native editable PK1 fields. The same species in a later save gains the fields that genuinely exist in that later format.

## Exact-format capability resolution

`capabilitiesForGeneration(...)` is a reusable baseline, not permission to invent data. Exact game/save adapters may refine the baseline when a mechanic exists only in certain titles or revisions.

The exact adapter wins whenever generation-wide assumptions are too broad. Examples include title-specific caught/met structures, special-case friendship mechanics, DLC/revision differences, or later-game metadata changes.

No later-generation value may leak backward into an older save format.

## Shared PKSE-style layout

Create, Edit, and View use the same reusable editor shell.

- **View** shows the same supported fields but is read-only.
- **Create** exposes every field the destination/current save format can genuinely store and validates choices against that format.
- **Edit** exposes the same native fields and stages changes without mutating the original source.
- Unsupported fields are hidden, not shown as fake defaults or editable placeholders.
- Compact fields stay in the three-panel editor where practical.
- Large collections or complex sub-editors open dedicated screens.

The established three-column foundation remains:

1. **Details / Identity** — species, nickname, level/EXP, OT/TID and other format-native identity/encounter fields.
2. **Values** — generation-appropriate DV/IV, Stat Exp/EV, calculated stats, shiny/gender/etc. only where meaningful.
3. **Moves + supplemental data** — moves and compatibility at the top; lower section split between generation-aware supplemental fields and the battle-stat radar.

## Supplemental field rules

### Held Item

- Hidden when the current format has no held-item field.
- Interactive in Create/Edit when supported.
- `A` opens a generation/game-valid held-item picker.
- The picker must contain only items representable/valid for the exact current game/save format.
- Species debut generation does not restrict the picker; current save format does.

### Ribbons

- Hidden when the current format has no ribbon data.
- When supported, the compact panel shows a summary/count.
- `A` opens a dedicated larger Ribbon Collection screen.
- The collection must expose only ribbons supported by the exact current game/save format.

### Nature / Ability / Friendship / Egg / Met data / Ball / Language / Pokérus / Marks and similar fields

Apply the same rule automatically:

- If the exact current format stores the field, show it using the established PKSE-style editor conventions and make it editable where safe/appropriate.
- If the exact current format does not store it, hide it.
- If the value is known only from PokeBank NX provenance rather than stored Pokémon bytes, show it as read-only provenance instead of fabricating an editable native field.
- Complex collections (for example Ribbons/Marks) get dedicated collection screens rather than being crammed into the compact panel.

## Provenance

Origin/source history is not ordinary editable Pokémon data.

PokeBank NX must preserve immutable provenance such as original game/platform/save identity and transfer history. Current-format metadata may change through a legitimate conversion/transfer, but provenance must continue to record where the Pokémon actually came from.

Language/origin information that is not independently stored by an old format may still be displayed from trusted source/provenance metadata, but must not be presented as a fabricated editable field in that old save.

## Generation examples

### Generation I

Show genuine PK1 concepts only: species, nickname, level/EXP, OT/TID, moves/PP, DVs, derived HP DV, Stat Exp, calculated battle stats, and the DV-derived shiny compatibility control used by PokeBank NX. Do not fabricate modern fields.

### Generation II

Add native Gen II concepts such as Held Item, generic Friendship, Pokérus and other exact-format fields supported by the individual GSC adapter. Do not add Nature/Ability/Ribbons because those are not Gen II native fields.

### Generation III and later

Expose the native fields provided by the exact adapter, including concepts such as Held Item, Nature, Ability, IV/EV systems, Ball/origin/met information, Pokérus and Ribbons when genuinely stored by that format. Later-only concepts such as Marks remain hidden until supported by the current format.

These are examples, not a substitute for exact adapter capability checks.

## Safety invariants

This UI contract does not authorize source writeback.

- Original source bytes remain immutable.
- Live RetroArch writes remain HARD DISABLED.
- Live installed-game writes remain HARD DISABLED.
- Live other-emulator writes remain HARD DISABLED.
- Create/Edit operate on staged copies/drafts.
- Party mutation remains deferred until separately proven safe.
- Preview/navigation must never mutate the source.

## Permanent implementation rule

Future Pokémon game adapters should plug their exact-format capability/data provider into this shared editor instead of building a new generation-specific UI from scratch. Do not ask the project owner to redesign the field layout for each generation unless a genuinely new mechanic cannot fit the established model.