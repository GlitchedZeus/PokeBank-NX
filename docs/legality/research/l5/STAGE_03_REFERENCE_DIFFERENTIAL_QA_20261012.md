# L5 Stage 3 — MAJOR independently executable PKHeX reference differential QA

2026-10-12. Existing PR #294 only. Source reference: kwsch/PKHeX at 6501f0ab46e8f8ca048539dbaf8cae8cb104e722. L5 does not copy PKHeX code or original saves.

## Implementation and exact test evidence

1. tests/fixtures/legality_l5_reference_differential.cpp (exact Git blob ba484cdcc4f454b7b54a99255b125b9a12ed663a): 655,450 method-scoped comparisons, 655,466 independent C++ assertions; GCC 14.2 O2 C++20 Werror PASS; GCC 14.2 O1 ASan+UBSan PASS; Clang 17 O2 Werror PASS. Committed source was byte-verified using local git hash-object.
2. The parameter matrix covers each two-stat 256x256 EV byte domain for Gen III and IV with four egg/encounter/EXP configurations, each full 16-bit stat-experience domain for Gen I and II, and direct boundary cases. Counts: agreement 131,860, local reference-rule gaps in narrow NX *source model* 392,746, unresolved histories 130,844. Not distinct production defects, not original encounters, not whole-Pokémon parity.
3. Seven additional classification controls distinguish differing format parser outcomes, source data, unknown history, reference-only Invalid, NX overstatement, severity differences, and same-Invalid diagnostic reason/order. Exact source missingness dominates hypothetical encounter candidates.
4. tools/audit_legality_l5_stage3_reference.py compiles and executes the independent fixture under GCC normal and GCC ASan+UBSan, optional Clang, checks machine-readable exact counts and supports independent byte-exact PKHeX/NX source pin verification. The Python full-checkout invocation is NOT RUN here; the exact committed C++ source was run directly as above.
5. tests/fixtures/legality_l5_pk2_egg_stat_exp_analyzer_probe.cpp and tools/audit_legality_l5_stage3_pk2_probe.mk add opt-in 200-case actual full Legality::analyze probe (80 unhatched Gen II nonzero stat-experience contexts) under normal or sanitizer. These full production analyzer modes remain NOT EXECUTED. No shared CI or production source changed.
6. docs/legality/research/l5/Q19_STAGE3_PINNED_DIFFERENTIAL_EVIDENCE.json lists eight upstream source references, Git blob pins, exact synthetic counts and the F5 reproduction contract.

## Pinned reference evidence (inspected upstream, not redistributed)

- PKHeX EffortValueVerifier.cs blob 86c9bc6ea0361821df86e1473788c228ce7506a0: egg nonzero EVTotal Invalid; format-dependent 510 total; Gen III/IV gain conditions depend on actual encounter history; Gen I/II stat-experience heuristic.
- PKHeX EffortValues.cs blob 5904496c4e60df3eb421bdf4c387f9df08f130ad: per-stat 255 for Gen III–V, 252 for Gen VI+, Gen I/II vitamin increments.
- PKHeX PKM.cs blob b6ac182fa32d60804a87752df0a80d554c30f574: EVTotal aggregates getters.
- PKHeX PK1.cs a4b692ae7045afb9d11ea3a3b6e25b94c0c4de8b, PK2.cs fe6fea5c5a2ff82f2ace7ba9af289348c16a0df0, GBPKM.cs 4e09e9f209c58c53de4437be1a876be57faf9ab0: native Gen I/II 16-bit stat experience.
- PKHeX G3PKM.cs 1042c3edef50b4f2d16b164c56ef2a3461befc9f and G4PKM.cs c42aad87ec573ee59377db28a110a6403163de0c: per-stat maximum 255.

## L5-F5 new P0 source-backed candidate — native Gen II egg stat experience

Input (synthetic only): native PK2 Bulbasaur #1, egg slot marker 0xFD, level 5, stat experience (2560,0,0,0,0), source label missing/gold_gbc/crystal_gbc/firered_gba/unknown. Expected pinned PKHeX method behavior: unhatched egg with nonzero native stat-experience EVTotal -> Invalid. Existing NX source representation: Gen2ReadOnlySave.cpp:176/185/202 retains egg and stat-experience; Pokemon2ReadOnly.h:51 evHP getter is neutral zero while :92 and :120 expose egg and actual stat experience; coordinator Legality.cpp:1597 reads modern EV getters and its native egg check at :1647–1653 is Gen III/IV-only. No Gen II stat-exp check was observed in reviewed source blob 2ee1624de1520630c54d69599834c8e29d589ffb.

Status: production-source candidate + independently executed model differential; NOT YET a confirmed actual full-analyzer false conclusion. The 200-context production probe is committed for exact verification. The coordinator alone edits Legality.cpp and shared CI after actual reproduction and mechanics review.

## Prioritized coordinator handoff

P0. Run full production baseline and sanitizer on reconciled integration checkout:
    make -f Makefile.host -f tools/audit_legality_l5_stage3_pk2_probe.mk l5-pk2-egg-baseline
    make -f Makefile.host -f tools/audit_legality_l5_stage3_pk2_probe.mk l5-pk2-egg-baseline-sanitize
Then, only if verified and approved, implement the Gen II egg fix in coordinator-owned production, rerun l5-pk2-egg-fixed and l5-pk2-egg-fixed-sanitize, and retain zero-stat-exp egg/non-egg/source-mismatch controls.

P0. Run pinned source differential runner on full repo with normal and sanitizer:
    python3 tools/audit_legality_l5_stage3_reference.py --clang
    python3 tools/audit_legality_l5_stage3_reference.py --pkhex-root /pinned/PKHeX --nx-root /pinned/integration --require-both-pins
The source pin intentionally fails closed if production or upstream reference changed. Do not bypass drift.

P0. Coordinator PR #298 remained draft at 2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc; Actions run 38296998403 completed FAILURE on L2, despite passing L1/L3/L4/L5 jobs at that older exact SHA. This Stage 3 work is not integrated; fix L2 before any promotion. Current L5 exact-head PR run requires fresh verification.

P1. Obtain lawful original paired source saves + attested encounter-level/EXP evidence to turn hypothetical Gen III/IV EV-gain negatives into actual original route decisions. Missing candidate histories remain Unknown/Incomplete. P1/P2: 336 game×family original-retail parity certifications remain 0; archival Gen IV event attestations and redistribution rights remain blockers.

Stage boundaries: No production Legality.cpp, shared CI, original saves, source Pokémon, PR #298, accepted legality, MAIN, or sibling work edited. No rebase/reset/force push, no merge. Stage 4 next: cross-lane adversarial evaluation of L1/L2/L3/L4 history candidates.
