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
- preservation of the existing exact-game Gen III move context.

Current encounter-table coverage is **partial** for Crystal and the five Gen III games. Gen I and Gen IV encounter provenance remain explicitly uncovered until their encounter datasets are imported and independently verified.

## Planned layers

1. **Format / structure** — ids, checksums, field ranges and format invariants.
2. **Internal consistency** — level/EXP, names, gender, ability, forms and stats.
3. **Exact-game move legality** — compatibility tables are wired now; event/tutor chronology and full tradeback provenance remain to be completed.
4. **Encounter provenance** — species, location, level, method, time and version restrictions.
5. **PID/RNG correlation** — Gen III/IV method constraints, nature, ability, gender and shiny correlation.
6. **Egg / breeding legality** — hatch level/location, inherited moves and generation-specific breeding rules.
7. **Event / gift legality** — fixed trainer data, fateful flags, ribbons, dates and distribution records.
8. **Transfer legality** — legitimate movement between generations.
9. **Bulk/provenance checks** — duplicate identities, clone lineage and Vault history once the Vault backend exists.

PKHeX is used as a reference implementation for architecture and cross-checking. PokeBank NX does not claim PKHeX parity until equivalent evidence and regression coverage exist.
