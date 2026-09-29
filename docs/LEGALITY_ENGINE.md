# PokeBank NX Legality Engine

Status: **active foundation work**

PokeBank NX already had an informational structural/internal checker. The legality engine extends that work into an exact-game, evidence-driven analyzer for the generations the application currently supports.

## Design rules

- Legality analysis is **read-only**. It never mutates or auto-fixes a Pokémon.
- A clean report means **no problem was found by the checks that ran**. It is not automatically proof that the Pokémon is fully legal.
- Every analysis area tracks coverage. Missing evidence is reported as incomplete coverage rather than silently treated as legal.
- Exact save identity matters. Ruby, Emerald and FireRed share the PK3 entity format but do not share every legal move or encounter.
- Historical mechanics stay historical. Gen I/II DV + Stat Experience rules are not replaced with modern IV/EV assumptions.
- Encounter, move, PID/RNG, event, egg, trade and transfer legality are separate evidence layers.
- Legality never changes save-write authorization.

## First Gen I-IV milestone

The first legality-engine tranche adds:

- exact source-game profiles for R/B/Y, G/S/C, R/S/E/FR/LG and D/P/Pt/HG/SS;
- generation-correct species and move ceilings;
- exact-game Gen I, II, III and IV move-pool checks using the editor's audited compatibility tables;
- structured check identifiers;
- coverage-aware reports and an explicit Incomplete verdict;
- conservative exact-game encounter evidence where audited data already exists;
- preservation of the existing exact-game Gen III move context;
- Gen III handheld PID/IV correlation for Methods 1, 2, 3 and 4, including Unown's reversed-half variants.

Current encounter coverage is **partial** for R/B/Y, Crystal, the five Gen III games, and all five Gen IV games. Gen I now checks exact-game PK1 catch-rate evidence and Time Capsule held-item-byte compatibility without pretending PK1 stores met location/level. Gen IV now has pinned wild-slot, static/gift, and fixed in-game trade evidence for D/P/Pt/HG/SS. External event and PokeWalker templates are still incomplete, so an encounter non-match is not treated as illegal. Gen I static/gift/trade and full pre-evolution catch-rate provenance are still incomplete, so unmatched catch-rate evidence remains unknown rather than illegal.

## Planned layers

1. **Format / structure** — ids, checksums, field ranges and format invariants.
2. **Internal consistency** — level/EXP, names, gender, ability, forms and stats.
3. **Exact-game move legality** — compatibility tables are wired now; event/tutor chronology and full tradeback provenance remain to be completed.
4. **Encounter provenance** — Gen IV wild, static/gift, and fixed in-game trade evidence is imported; external events, PokeWalker, evolution provenance and remaining per-method restrictions are still incomplete.
5. **PID/RNG correlation** — Gen III handheld Methods 1/2/3/4, normal Gen IV Method-1 PID/IV, and the deterministic PokéWalker PID formula are recognized. PokéWalker IV/course provenance, Gen IV Method J/K lead frames, Cute Charm, Chain Shiny, Mystery Gift anti-shiny, plus Gen III Channel/Colosseum/XD/roamer/BACD classes remain incomplete.
6. **Egg / breeding legality** — hatch level/location, inherited moves and generation-specific breeding rules.
7. **Event / gift legality** — fixed trainer data, fateful flags, ribbons, dates and distribution records.
8. **Transfer legality** — legitimate movement between generations.
9. **Bulk/provenance checks** — duplicate identities, clone lineage and Vault history once the Vault backend exists.

PKHeX is used as a reference implementation for architecture and cross-checking. PokeBank NX does not claim PKHeX parity until equivalent evidence and regression coverage exist.
