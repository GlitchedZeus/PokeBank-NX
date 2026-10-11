# Gen I–IV legality research audit — October 11, 2026

**Follow-up refresh:** the newer accepted engine is `fe3e3d05`. Read [Tranche 2](TRANCHE_2_ENGINE_REFRESH.md) before acting on the original backlog below. It credits the implemented work and supplies a confirmed parent-slot integration defect plus independently executed eligibility evidence.


The highest-value remaining work is **feasible move history**, built on the engine's accepted four-slot game-family coverage. Current pool coverage answers which families can collectively explain move membership. It does not prove that one encounter, evolution, breeding and transfer history can produce all four moves together. Keep those claims separate in both the implementation and its reports.

This research-only tranche delivers **10 evidence packets, 28 verifier-family rows, 30 move-acquisition matrix rows, 67 pinned source records and 70 cataloged test cases/scenarios**. Counts include explicitly unexecuted proposals; they are not 70 verified legitimate Pokémon. No new production rule, ROM, preserved Pokémon binary or save modification is included.

## GitHub reconciliation

Original accepted baseline: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`. Delivery source freeze: **`4d3026e0bbe90f2eb1505c107b1c74498c051549`**. The active development lane advanced substantially during research. [Live reconciliation](LIVE_DEVELOPMENT_RECONCILIATION.md) credits accepted joint move pools, guarded Pal Park ancestry/stage reporting and pending #244–#249 work. PR #103 remained open/draft at capture. This research never moved main or the accepted branch.

The pending #247 Emerald level-up table was compared in full with pinned PKHeX: **386 species, 3,947 ordered pairs, zero mismatches**. This is cross-source consistency, not independent retail execution or proof of historical feasibility. Reuse that work; do not recreate its data extraction.

## Principal findings

1. **Move membership is incomplete historical evidence.** Candidate identity, acquisition method, level, evolution stage, retained moves, and transfer edges must remain attached to each witness. A union of incompatible encounter candidates can falsely overstate compatibility. [MOVE-001](packets/MOVE-001.md) gives the proposed representation and acceptance boundary.
2. **Time Capsule direction matters.** Current eligibility is not proof of a historical round trip. Gen II-only moves, evolutions, Crystal and Stadium services, and Korean restrictions require separate history policies. Pending #244/#245 already add bounded reporting. [MOVE-002](packets/MOVE-002.md).
3. **Shedinja requires evolution-time reasoning.** Emerald constructs Shedinja from the resulting Ninjask move state. Agility at 38 and Baton Pass at 45 cannot both arise from the same ordinary evolution event. Alternate acquisition must be checked before rejecting the specimen. Pinned PKHeX's one otherwise-unresolved Ninjask-move allowance is not a blanket limit of one Ninjask-derived move. [MOVE-003](packets/MOVE-003.md).
4. **Egg move order and overflow are meaningful for ordinary unhatched eggs.** Emerald applies base, inherited egg, inherited TM/HM, shared level-up and final Volt Tackle steps in order. Parents and provenance are usually absent; an unknown parent is not a contradiction. Gen II upstream order fixtures are provided but not executed here. Pending #249 only supplies family evidence. [BREED-001](packets/BREED-001.md).
5. **Retail IV inheritance is not uniformly “choose three distinct stats.”** Emerald and Ruby/FireRed removal behavior can repeat a stat. Twenty-four fixed-seed executions of unchanged extracted retail C agreed with an independently written Python model. Platinum has the related indexed-removal behavior; HGSS uses distinct selection and Power-item exceptions. Only the three Gen III implementations were executed in this harness. [RNG-001](packets/RNG-001.md).
6. **The 128-pair nature reversal is a coverage bound.** Seed `0x12`, nature 20, rejects 129 PID candidates before `0xEA0705DB`; original HGSS function and independent model agree. The audited NX helper returns no result at its bound. This is a synthetic generation-stage witness, not a complete matched native encounter and not a proven final false-Invalid. [RNG-002](packets/RNG-002.md).
7. **Event projections lose important evidence.** The audited 247-template Gen IV projection omits initial moves, raw text and ribbons. Archive placeholders and altered legality cards cannot establish original distribution metadata. Candidate identity and per-field provenance should precede stricter rules. [EVENT-001](packets/EVENT-001.md).
8. **Metadata needs explicit field availability.** Decoded text cannot substitute for raw trash bytes; a default false ribbon property is not evidence of an absent ribbon. PK4 contest-rank prerequisites support bounded checks after native access is proven. [META-001](packets/META-001.md).
9. **Pal Park deletes and loses evidence.** Apply HM deletion to the transfer edge and permit later reacquisition. Current PK4 HM membership alone is not a transfer contradiction. Accepted source guards and ancestry evidence already exist. [TRANSFER-001](packets/TRANSFER-001.md).
10. **Encounter coverage needs condition-level comparison.** The audited Gen IV table contains 20,147 packed rows in 46 game/method groups, including existing Radar/Munchlax-related evidence. Those categories are not missing just because reporting is partial. A complete reference multiset comparison remains open. [ENCOUNTER-001](packets/ENCOUNTER-001.md).

**No newly confirmed current production false-Invalid bug was established. No wholly unwired orphan helper was proven.** These are deliberate limits of the evidence, not assertions that the engine is complete.

## READY FOR MAIN LEGALITY DEVELOPMENT

The [prioritized backlog](IMPLEMENTATION_BACKLOG.md) gives the top ten tasks, sequence, complexity and packet links. Begin with candidate-scoped witnesses and version/stage data extensions; then ordinary egg construction, Shedinja and event move candidates; then transfer chronology. Ribbon availability and search-exhaustion reporting are narrower improvements. Ordinary egg RNG reconstruction needs careful parent-known/parent-unknown semantics.

Every packet contains a compact implementation brief, source locations, necessary inputs, information-loss limits, candidate positives/negatives/unknowns, integration points and acceptance criteria. “Ready” applies to the bounded task described, not to a complete hard-Invalid solver or license-cleared import of a whole reference dataset.

## Validation and scope

- 24 retail-C/Python inheritance comparisons passed, with 24 fixed-input contradictory-output controls.
- One long nature-rejection witness passed original HGSS/Python/NX helper comparison; its deliberately incorrect fixed-output control is cataloged separately.
- Five existing focused helper tests and two existing native-format analyzer tests passed at the original accepted baseline.
- Full pending Emerald table cross-check passed.
- Eighteen remaining catalog rows are scenario proposals or inspected upstream expectations; the two existing native test rows describe suites, not two newly captured Pokémon.
- Source hashes, CSV structure, local links and research-only changed paths are checked by [validation tooling](VALIDATION.md).

No full host suite, sanitizer suite, Switch build, PKHeX .NET run, whole-game execution or preserved retail save validation is claimed. The matrix is an acquisition-family roadmap, **not a complete species-by-species move-history database**. Source/game references were verified, but audit depth differs by family.

## NEEDS FURTHER RESEARCH

Complete parent-chain and mutually exclusive event-move solving; full egg PID/IV reversal; a native source-matched >128-rejection fixture; remaining Method H/J/K lead combinations; per-distribution event authenticity; direct retail Pal Park HM conversion; exhaustive raw encoding/trash-byte fixtures; full I–IV encounter category/condition reconciliation; and complete trainer/ribbon/language exceptions remain open. These are enumerated in the backlog, not silently treated as implemented or impossible.

Licensing/provenance remains a production integration gate. See [DATA_PROVENANCE_AND_LICENSES.md](DATA_PROVENANCE_AND_LICENSES.md). Research scripts use external pinned source locally; no wholesale PKHeX modules or retail function bodies are committed.
