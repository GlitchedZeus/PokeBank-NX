# Future Legal Pokémon Generator / Legalizer

Status: **planning only — not implemented in the Crystal hardware-fix pass**

PokeBank NX should eventually provide generation-aware **Generate Legal Pokémon** and
**Legalize Pokémon** workflows. The goal is to construct or repair a Pokémon from a real,
documented origin rather than making arbitrary fields merely look plausible.

## Core behavior

A request is a set of desired constraints such as species, destination game, current level,
shiny status, gender, nature, ability, ball, moves, or "lowest legal level". The engine searches
real origin templates and returns an exact legal origin when one exists.

Origin families must eventually include:

- wild encounters
- static encounters
- gifts
- eggs and breeding
- in-game trades
- Mystery Gift and event distributions
- special encounter mechanics
- valid transfers from earlier or later compatible generations
- Pokémon GO / Pokémon HOME origins where those are relevant to modern formats

The generator must preserve generation-specific correlations and restrictions. Encounter data,
met level, location, time, OT data, PID/IV relationships, shiny locks, ability rules, ball rules,
moves and transfer transformations must agree with the selected origin.

If the exact request cannot exist legally, the UI should say why and offer nearby legitimate
alternatives rather than fabricating history. A species/event with a real Level 1 origin may be
generated at Level 1, but a normal Crystal wild Graveler must not be given a fake Level 1 encounter
just because the raw format can store the number.

## Architectural direction

- Keep immutable original/source records separate from generated or legalized records.
- Search origin templates before mutating correlated fields.
- Prefer data-driven encounter/event tables over species-specific UI exceptions.
- Return provenance explaining the chosen origin and every important normalization.
- Allow "closest legal" results only as explicit user choices.
- Build Gen I-III accuracy first, then extend the same engine through later generations.

This milestone is intentionally deferred. The current Crystal pass only adds local encounter
guardrails needed for truthful Gen II editing.
