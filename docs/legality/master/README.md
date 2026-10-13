# PokeBank NX — Gen I–IV legality parallel execution plan

**Freeze date:** 2026-10-12 · **Root coordination issue:** [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285) · **Tasks:** [90 tracked work packages](WORK_CATALOG.csv).

We replace repeated ad hoc research→micro-fix cycles with **one indexed source inventory and five simultaneously executable engineering lanes**. Read the [copy-ready five-chat handoffs](PARALLEL_CHAT_HANDOFFS.md). Do not count 90 tasks as 90 PRs; each session should complete a substantial coherent bundle in one branch/PR, with actual code, native tests and source documentation.

## Where this audit starts

- Isolated **accepted legality branch** at plan creation: `feature/legality-engine-gen1-4-20260929` commit `0dccf2e10289164ad0d065985dcb29f14fd2fb35` (fully green, PR #282 accepted). **GitHub is authoritative; re-fetch before every session.**
- Stacked PR [#283 Platinum egg PID](https://github.com/GlitchedZeus/PokeBank-NX/pull/283) at `d29eada5c087e201e00c749703f1a2de3ddfadeb` was still in exact-head CI when this inventory started. Treat B14 as **draft**, not Missing. Coordinator accepts on fully green CI only.
- Umbrella legality PR [#103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) remains open/draft and cannot enter MAIN without owner authorization. MAIN, UI, touch, Gen V, Master Vault, original source save writes and staged Pokémon mutation remain out of scope.
- Research-only [#252](https://github.com/GlitchedZeus/PokeBank-NX/pull/252) is a **pinned reference** with 13 source packets, 79 registry entries and 145 test/catalog records; proposed test scenarios are not proof of passed native tests. [Frozen references](REFERENCE_FREEZE.md).
- Existing [21-family parity ledger](../GEN1_4_PKHEX_PARITY_LEDGER.md) found **20 Partial and only the reporting contract Implemented** at its last audit; this is **not a numeric PKHeX completion percentage**. Some findings are superseded by newer accepted commits. Do not blindly recreate old research recommendations.

## Five owners

| Lane | Owns | Issue | Assigned packages | Start with |
|---|---|---|---:|---|
| L1 Encounters and wild RNG | source-linked wild/static encounter helper headers (Gen1Encounter, Gen2Wild/Static, Gen3Safari/MethodH/Colo/XD encounters, Gen4Wild/Static/Lead) | [#286](https://github.com/GlitchedZeus/PokeBank-NX/issues/286) | 18 | E07 E11 E13 E15 |
| L2 Breeding and egg PID/IV | Gen*Egg*, Gen*Breeding*, Gen4NativeBreeding*, Gen4Platinum* | [#287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287) | 18 | B12 B13 B15 B16 |
| L3 Moves and historical transfers | Gen14CandidateMove*, Gen*JointMove*, EmeraldLevelUp*/ShedinjaEvolutionMove*, Gen*PalPark*/PostPalPark* | [#288](https://github.com/GlitchedZeus/PokeBank-NX/issues/288) | 18 | M06 M07 M11 M12 |
| L4 Events and metadata | Gen*Event*, Gen4UnifiedEvent*, Gen4RawText*, Gen4ContestRibbon*, event source attestation | [#289](https://github.com/GlitchedZeus/PokeBank-NX/issues/289) | 18 | V07 V08 V10 V11 |
| L5 Research and independent QA | docs/legality/research/** in independent branch, new tools/audit_legality_l5_* scripts, source/fixture manifests; no production C++ files | [#290](https://github.com/GlitchedZeus/PokeBank-NX/issues/290) | 18 | Q01 Q02 Q03 Q06 Q09 |

**Central coordination:** this main chat alone updates accepted legality, `src/Legality/Legality.cpp`, shared host Makefiles/workflows, global parity ledger and umbrella #103. Each lane owns its own independent branch and named tests; never push another lane's ref. Source-research L5 is deliberately documentation/fixtures-only, not a sixth conflicting production editor.

## Frozen research once → execute batches

1. **L5 creates source cells and pins data** for all **16 exact games × applicable verifier families** using the 21-family ledger, pinned PKHeX legality source, pret original routines and archive confidence grades. Distinguish `implemented`, `accepted but partial`, `draft`, `research-ready`, `unknown/source blocked` and `not applicable`. No blind claims that the finite list captures every obscure retail exception. L5 reports discoveries back into catalog IDs.
2. **L1–L4 implement in parallel**, taking 4–8 closely related ready tasks per major session, not one or two. Every task must have original source/game identity, an explicit candidate-history model, exact source stage constraints, and positive/negative/Unknown native assertions.
3. **If one source is blocked**, document exact file/symbol/game and supporting evidence in that lane issue, assign its research to L5, and proceed with the next ready task *within the same major session*. The central inventory is updated once; don't restart the full research hunt.
4. **Coordinator accepts/reconciles** only after complete exact-head clean build, host tests, focused RSE, ASan/UBSan and approved read-only changes, with no commits lost and no force push. Shared analyzer wiring happens only after lane code is accepted.
5. **L5 continuously audits and measures** verifiers × source games × method categories; update [release gates](RELEASE_GATES.md) and remaining knowledge gaps. No manual "40%→100%" progress calculation based on raw number of files/PRs.

## Snapshot truth and branch safety
A detached PK1/PK2/PK3/PK4 cannot reconstruct erased parents, trade histories, original event archives, or unknown MT/lead RNG snapshots. Source-attested conditional compatibility means **only that particular candidate history is possible**, not a proven authentic originating Pokémon. A contradiction to one candidate does not establish global Invalid; unsupported or unobservable histories stay Incomplete/Unknown.

Each lane must first inspect current accepted SHA and exact own PR HEAD; create from live branch, compare with sibling heads for file collisions, preserve all newer commits, no backward reset/rebase or force push. When independent lanes diverge because accepted branch advanced, main coordinator builds a forward-only reconciled **combined** tree with both parent histories, retests combined head and only then integrates. Do not self-merge.

## Completion definition
A usable parity claim requires each **applicable source×game×method** implemented and production-wired with pinned evidence, reproducible native positive/negative/Unknown coverage, full independent cross-differential validation, legal data reuse and a second false-Invalid audit. Genuine historical unobservability stays represented as Unknown. See [release gates](RELEASE_GATES.md); **100% Gen I–IV** is not the same as parity with every modern generation in PKHeX.
