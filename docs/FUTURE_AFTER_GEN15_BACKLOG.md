# PokeBank NX — Deferred ideas, retained for later

**October 15, 2026.** This is an **index of existing issue discussions**, not a new implementation promise. To keep the active GitHub Issues board focused on getting Gen I–V working and hardware accepted, the original idea issues below have been **closed as deferred / not planned for the current Gen I–V gate**. Closing them does NOT mean implemented, abandoned forever, or permission to delete associated branches/source code.

**Current work:** [Gen I–V integration #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) + [complete hardware acceptance issue #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313), with [106 initial action checks across all 20 releases](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md). Do not make Gen VI/new emulator integration the active release milestone until the owner signs off that checklist.

## Master Vault, collections and storage

- [#3](https://github.com/GlitchedZeus/PokeBank-NX/issues/3) — Persistent Master Vault and named banks
- [#7](https://github.com/GlitchedZeus/PokeBank-NX/issues/7) — Living Dex / vault-driven Pokédex
- [#27](https://github.com/GlitchedZeus/PokeBank-NX/issues/27) — App-owned Storage versus future Vault boundaries

## Transfers and historical route tooling

- [#10](https://github.com/GlitchedZeus/PokeBank-NX/issues/10) — Conversion/transfer without live source writes
- [#20](https://github.com/GlitchedZeus/PokeBank-NX/issues/20) — True Move after individually verified write adapters
- [#40](https://github.com/GlitchedZeus/PokeBank-NX/issues/40) — Historical transfer / Prepare for HOME
- [#47](https://github.com/GlitchedZeus/PokeBank-NX/issues/47) — GB/GBC/GBA original hardware link transfers

## Later-generation and console support

- [#32](https://github.com/GlitchedZeus/PokeBank-NX/issues/32) — Gen VI/VII 3DS source integration — hold until Gen I–V accepted
- [#33](https://github.com/GlitchedZeus/PokeBank-NX/issues/33) — Pokémon Colosseum / XD read-only saves
- [#34](https://github.com/GlitchedZeus/PokeBank-NX/issues/34) — Pokémon Stadium 1/2 read-only saves
- [#11](https://github.com/GlitchedZeus/PokeBank-NX/issues/11) — Modern Switch save adapter review

## Presentation and optional experiences

- [#25](https://github.com/GlitchedZeus/PokeBank-NX/issues/25) — 3D models / render support on Summary
- [#35](https://github.com/GlitchedZeus/PokeBank-NX/issues/35) — Pokémon cry playback
- [#46](https://github.com/GlitchedZeus/PokeBank-NX/issues/46) — Gift/Event/Mystery Gift library
- [#78](https://github.com/GlitchedZeus/PokeBank-NX/issues/78) — Vault Catalog request storefront

## Companion and social apps

- [#39](https://github.com/GlitchedZeus/PokeBank-NX/issues/39) — Android / USB-C Link app
- [#41](https://github.com/GlitchedZeus/PokeBank-NX/issues/41) — Companion Mode — guides/Move Lab/telemetry
- [#60](https://github.com/GlitchedZeus/PokeBank-NX/issues/60) — Friends / NX Plaza social hub

## Reopen rules

Reopen the original issue when (a) the owner prioritizes the particular feature after the Gen I–V acceptance gate; (b) the relevant generation/emulator has an independently testable native provider; and (c) work has a dedicated safety/test scope. Do not create duplicate replacement issues. Historical PRs/issues remain readable, and no application source has been removed by this housekeeping.

**Not deferred:** Gen I–V UI/editor/sources/hardware bugs, explicit no-source-writes safety, real QA/sanitizer failures, legality evidence work, original asset licensing/preservation research and Gen I–V test artifacts. Those remain open where necessary.
