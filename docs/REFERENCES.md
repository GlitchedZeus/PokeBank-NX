# External References

This file records external projects and data sites used for research, comparison, and manual verification during PokeBank NX development. A reference being listed here does **not** mean its code or data is bundled into PokeBank NX.

## PKSE

- Project: https://github.com/kiasta/PKSE
- License: AGPL-3.0
- Use in PokeBank NX: architecture, UX, save-editor behavior, and implementation comparison only unless license-compatible reuse is explicitly reviewed.

### PKSE PR #126 — on-screen keyboard

- Pull request: https://github.com/kiasta/PKSE/pull/126
- Status: merged
- Reference area: input/editor UX
- Useful behaviors: themed NanoVG keyboard, controller navigation and repeat, touch input, text and numeric layouts, caret movement, Shift, live list filtering while typing, and fallback to the Switch system keyboard for Japanese/Chinese/Korean input.
- Integration policy: reference the design and behavior; do not copy AGPL-3.0 implementation code into PokeBank NX without an explicit license-compatibility review.

## PokemonStatsExporter

- Project: https://github.com/kiasta/PokemonStatsExporter
- License: MIT
- Purpose: generates C++ base-stat arrays from Pokemon Database Pokédex data.
- Use in PokeBank NX: research/example implementation for build-time data generation and table-validation workflows. It is not a runtime dependency.

## Pokémon Database

- Site: https://pokemondb.net/
- Use in PokeBank NX: manual secondary cross-check for human-readable Pokémon information such as base stats, move power/accuracy/PP, evolution information, and location summaries.
- Integration policy: do not make PokeBank NX depend on live scraping of Pokémon Database and do not treat it as the primary legality authority.

## Evidence hierarchy

For save-format correctness and legality decisions, prefer primary or format-authoritative evidence (game data, documented save structures, PKHeX/PKSM-Core models, and reproducible test saves) over secondary web references. External websites are useful corroboration, not sole proof for byte-level or legality rules.
