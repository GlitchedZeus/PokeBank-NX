# ENCOUNTER-001 — Audit encounter conditions without treating table absence as impossibility

Research ID: **ENCOUNTER-001**
Affected generations/games: I–IV coverage inventory, detailed normalized IV categories
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`
Estimated complexity: **Medium**
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

The accepted IV packed table contains **20,147 rows across 46 game/method categories**. `Gen4WildEncounter.h` already contains source rate/slot, lead metadata, Radar eligibility and Munchlax tree-ID logic. Gen I/II static/wild/trade and extensive III event/GameCube modules exist. Pending PR #225 is detached PK3 wild-source reporting; it is not a missing encounter dataset. Broad claims that Honey Trees, Radar, Safari or NPC trades are wholly missing would be incorrect.

## Authoritative sources

- **NX-WILD4**: [include/Legality/Gen4WildEncounter.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen4WildEncounter.h); `munchlaxTreeIndices`; grade B.
- **NX-EVENT4**: [include/Legality/Gen4EventTemplate.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen4EventTemplate.h); `struct EventTemplate`; grade B.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.

## Normalized inventory and limitations

`evidence/encounter-category-inventory.json` records game, method ID, row count and a representative species/location/min/max/slot/rate row per category. It decodes the accepted packed table read-only and deliberately labels conditions not reconstructed. It is an inventory, not 20,147 newly discovered encounters. The pinned generator and source tables must be compared by multiset, retaining duplicate persistent identities whose lead histories differ.

Missing evidence is best expressed as fields rather than guessed missing species: time/weather/swarm/slot substitutions, tree eligibility, block/area conditions, exact template source and any conditions irrecoverable from PK4. A current held item or current save state does not prove the capture-time condition. Candidate compatibility may existentially permit a prior condition that is no longer stored.

## Event/source exclusions

Sinnoh Underground in original DP/Pt supplies fossils/items that lead to revival gifts. Do not import BDSP Grand Underground wild hideaway tables into Gen IV. The original Platinum underground source inspected contains treasure/mining handling; a complete fossil gift audit remains to be done. Event archive presence is not release proof (EVENT-001).

## Positive / negative / unknown tests

- Positive: decode every table value and assert species/game/level/rate domains and parallel metadata lengths; then match one representative **native** specimen per supported category in development.
- Negative: wrong exact location/level for a known candidate rejects that candidate only.
- Unknown: no row, unsupported conditional encounter, missing capture-time state, ambiguous evolved lineage.
- False-Invalid risk: uniqueness inferred after deduplication, whole-dataset completeness inferred from row count, or absent table match treated as a global contradiction.

## Integration / acceptance / next source work

Attach confidence and source-condition provenance to candidate enumeration. Compare source resource checksums and generated row counts before extending production tables. Complete per-game category reconciliation for I–III, condition-preserving IV extraction, and native-record coverage mapping. This packet's normalized inventory is ready for coverage planning, **not** a complete missing-encounter dataset. Difficulty Medium for inventory, High for exhaustive conditional histories.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
