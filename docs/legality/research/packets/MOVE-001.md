# MOVE-001 — Replace membership pools with candidate-scoped acquisition witnesses

Research ID: **MOVE-001**  
Affected generations/games: I–IV: RB/Y, GS/C, RS/E/FR/LG, DP/Pt/HGSS  
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`  
Estimated complexity: **High**  
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`src/Legality/Legality.cpp:1507–1592` invokes exact-game compatibility pools. Gen III passes the current four moves as `sourceMoves`, so an otherwise unknown in-range current move classifies Preserved; Gen IV passes `existingSourceMove=true`, with the same fallback. Both issue warnings, not a proven impossibility verdict. `Gen34EggMoveEvidence::classify` walks one ancestor chain and reports individual egg membership at analyzer lines 1133–1148. Neither API receives level, encounter candidate, ordered evolution timing, or parent history. This is **Implemented but incomplete**, not an absent move feature. Existing `tests/test_gen34_egg_moves.cpp` proves table/ancestor queries, not four-slot historical satisfiability.

## Live development reconciliation

Accepted source freeze advanced to `4d3026e0`; four-slot game-family coverage and guarded retained Gen III paths are now implemented. Pending #247 adds full Emerald level-up method evidence and #249 adds Gen IV egg-family evidence. These are not complete acquisition histories. See [live reconciliation](../LIVE_DEVELOPMENT_RECONCILIATION.md) for exact hashes, boundaries and the independent Emerald comparison. The baseline description above is historical.

## Authoritative sources

- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.
- **NX-MOVE3**: [include/Integration/Gen3/Gen3LearnsetTable.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Integration/Gen3/Gen3LearnsetTable.h); `Availability classify`; grade B.
- **NX-MOVE4**: [include/Integration/Gen4/Gen4MoveCompatibility.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Integration/Gen4/Gen4MoveCompatibility.h); `Availability classify`; grade B.
- **NX-EGGMOVE**: [include/Legality/Gen34EggMoveEvidence.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen34EggMoveEvidence.h); `classify`; grade B.
- **PK-HISTORY**: [PKHeX.Core/Legality/LearnSource/Verify/LearnVerifierHistory.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Verify/LearnVerifierHistory.cs); `MarkAndIterate`; grade B.
- **PK-GROUP1**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup1.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup1.cs); `public bool Check`; grade B.
- **PK-GROUP2**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup2.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup2.cs); `public bool Check`; grade B.
- **PK-GROUP3**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs); `public bool Check`; grade B.
- **PK-GROUP4**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs); `public bool Check`; grade B.
- **PK-LEARNREADER**: [PKHeX.Core/Legality/Learnset/LearnsetReader.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Learnset/LearnsetReader.cs); `ReadLearnset16`; grade B.
- **PK-BIN**: [PKHeX.Core/Legality/Assets/BinLinkerAccessor16.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Assets/BinLinkerAccessor16.cs); `GetEntry`; grade B.

## Compact implementation brief

1. **Missing:** a common encounter/evolution/transfer witness for the entire moveset.
2. **Impact:** union pools erase acquisition levels, methods, encounter identity and mutually exclusive origins; warnings can also be produced for legitimate Gen III retained moves in PK4.
3. **Data:** per-version ordered level-up pairs, personal TM/HM/tutor bits, egg lists, encounter initial moves, evolution criteria, and transfer edges.
4. **Integration:** add a read-only move-history service between encounter-candidate enumeration and the current move findings. Keep editor selection pools as UI conveniences.
5. **Positive tests:** an evolved Pokémon retaining a pre-evolution move; a move learned via another trade-compatible version; an event gift retaining its initial special move.
6. **Negative tests:** one selected encounter candidate cannot supply both event-exclusive and ordinary egg-only acquisition; a move only available above the maximum reachable stage level fails that candidate.
7. **Unknown tests:** event-source registry incomplete, unknown origin, uncertain permitted console history, or bounded search exhaustion.
8. **False-Invalid risk:** rejecting the Pokémon after one candidate fails, using enclosing save as birth game, or treating every missing table row as exhaustive evidence.
9. **Acceptance:** every accepted slot has a witness, all witnesses belong to one feasible history, and Unsupported survives candidate aggregation.
10. **Order:** typed resources → history/candidate model → per-slot witnesses → shared-history solver → native parser/report regressions.

## Retail mechanism and reference behavior

`LearnVerifierHistory.MarkAndIterate` marks special encounter moves and iterates generation-specific groups. `LearnGroup1/2` uses stage level ranges; the Gen I group additionally checks incompatible level-up/evolution ordering. `LearnGroup3/4` checks reachable ancestor stages and level ceilings. These groups have special handling, including Shedinja, Rotom and cross-version sources. `GetAllMoves` is a suggestion union and is not interchangeable with `Check`.

Implement a set of possible witnesses per slot, not a single greedy best match. Suggested record:

`{move, generation, game, species, form, method, acquisition_level_min/max, encounter_candidate_id, prerequisites, irreversible_boundary, evidence_id}`.

A history node records species/form, level interval, game, egg state, and four-slot state when order matters. Trade is a reversible same-generation edge subject to language/game restrictions; evolution and Pal Park are directional. A witness survives only if reachable without violating a shared boundary. Explore alternative acquisition methods before declaring a contradiction. Do not implement a global full state-space search over all 467^4 movesets: restrict to the observed moves plus required overwritten/intermediate moves and candidate boundary conditions.

## Resource extraction contract

`evidence/levelup-resource-manifest.json` hashes all **11** pinned I–IV level-up resources. A BinLinker16 file has a two-byte identifier, little-endian entry count, and consecutive little-endian 16-bit offsets. Each level-up entry has `N` little-endian move IDs followed by `N` one-byte levels, so its byte length is `3N`. Preserve order and duplicate move acquisitions. Personal resources are version-specific; map machine/tutor indices using the same version's constants, never a modern TM number.

`MOVE_HISTORY_EVIDENCE_MATRIX.csv` enumerates every acquisition family and boundary at the roadmap level. It is **not a complete species-by-move pathway database**. Selected Nincada/Pichu facts are extracted for focused tests; full machine/tutor and parent-combination materialization is still required.

## Inputs and information loss

PK1/PK2 preserve current moves and current species/level, but not complete traded-game/evolution timelines. PK2 Crystal caught data has limited provenance. PK3/PK4 preserve PID, IVs, origin game and current moves; native PK4 keeps egg-location evidence, but no parent move records or chronological move history. Pal Park replaces original met fields. Neither an absent parent nor an absent original save is a contradiction.

## Positive evidence, Invalid, and unresolved

A found witness proves compatibility, not authenticity. Hard Invalid requires every admissible candidate to be disproved with complete relevant data. Until then return Incomplete and explain which slots/history families remain unresolved. Tests in the vector catalog are proposals unless their verification column explicitly identifies an executed check.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
