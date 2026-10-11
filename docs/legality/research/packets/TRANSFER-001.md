# TRANSFER-001 — Pal Park is a lossy, one-way acquisition boundary

Research ID: **TRANSFER-001**  
Affected generations/games: III → IV; native PK3/PK4  
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`  
Estimated complexity: **Medium for evidence; High for global closure**  
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`Gen4TransferEvidence` already classifies persistent Pal Park fields, including source-free native paths accepted before this audit. The Gen IV move pool includes only Gen IV direct/transfer membership, so it cannot itself prove retained Gen III tutor/TM/event histories. Do not redevelop the accepted Pal Park marker classifier.

## Live development reconciliation

Accepted source freeze advanced to `4d3026e0`; four-slot game-family coverage and guarded retained Gen III paths are now implemented. Pending #247 adds full Emerald level-up method evidence and #249 adds Gen IV egg-family evidence. These are not complete acquisition histories. See [live reconciliation](../LIVE_DEVELOPMENT_RECONCILIATION.md) for exact hashes, boundaries and the independent Emerald comparison. The baseline description above is historical.

## Authoritative sources

- **NX-TRANSFER**: [include/Legality/Gen4TransferEvidence.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen4TransferEvidence.h); `namespace`; grade B.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.
- **NX-MOVE4**: [include/Integration/Gen4/Gen4MoveCompatibility.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Integration/Gen4/Gen4MoveCompatibility.h); `Availability classify`; grade B.
- **PK-GROUP3**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs); `public bool Check`; grade B.
- **PK-GROUP4**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs); `public bool Check`; grade B.
- **PK-SOURCE3E**: [PKHeX.Core/Legality/LearnSource/Sources/LearnSource3E.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Sources/LearnSource3E.cs); `GetCanLearn`; grade B.
- **PK-TRASH**: [PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs); `VerifyTrashBytesPalPark`; grade B.

## Rule and algorithm

A PK4 candidate with Gen III origin may need Gen III acquisition witnesses. Pinned `LearnGroup4.GetPrevious` reaches Gen III for a non-Gen-IV encounter. `LearnSource3E` only allows the Gen III HM acquisition path while format remains 3, reflecting the transfer deletion boundary; the equivalent restriction appears in RS/FR/LG sources. Non-HM Gen III tutors/TMs/events can remain relevant after transfer.

Do **not** ban every Gen III HM move ID on a current PK4. A legal Gen IV source may reacquire the same move. Apply deletion to the edge, then search for later acquisition. For a Gen III-only source plus no post-transfer source, that witness fails. To turn witness failure into Invalid, every alternative encounter/source must be excluded.

## Inputs and lost fields

Use format, stored origin game, current moves/species/form/level, persistent met location/level and candidate ancestry. Original Gen III met location is overwritten by migration; do not compare a PK4 Pal Park location directly with Gen III wild tables. Current met level describes migration level and constrains possible earlier levels; it is not original capture level. Parent/history data and exact source-save identity are not recoverable.

## Implementation brief / tests

- Missing: source-aware cross-generation move witnesses and irreversible edge rules.
- Data: per-version Gen III methods and Gen IV relearn/level/TM/tutor paths.
- Positive: a retained Gen III non-HM tutor move; a deleted Gen III HM subsequently relearned through Gen IV.
- Negative candidate: only proposed acquisition requires carrying an HM across migration and there is no post-transfer reacquisition.
- Unknown: event history or original encounter cannot be reconstructed.
- Risk: current-save rules and HM-ID blacklists generate false Invalid; history is required.
- Acceptance: existing Pal Park tests remain unchanged; add native PK4 retained-move positives and edge-specific negatives without implying capture-history recovery.

The HM migration gate is supported by pinned reference logic here. Direct retail migration UI/selection path was not fully traced; corroboration remains a named follow-up. No unverified concrete species+HM combination is labeled proven impossible.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
