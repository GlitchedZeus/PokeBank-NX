# L5 — major tranche 4: independent false-Invalid differential and release audit

**Lane:** draft [PR #294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294); source-only, read-only research. No accepted production C++, original/staged save, sibling work, shared CI workflow, shared Makefiles, UI, Gen V, Master Vault or umbrella legality PR #103 changed.

## Research checkpoint and pinned reference

- Current accepted Gen I–IV legality branch read: `feature/legality-engine-gen1-4-20260929@d29eada5c087e201e00c749703f1a2de3ddfadeb`. Verify the live head before any integration.
- NX [analyzer](https://github.com/GlitchedZeus/PokeBank-NX/blob/d29eada5c087e201e00c749703f1a2de3ddfadeb/src/Legality/Legality.cpp#L1625-L1633) Git blob `b1d02d6620e8fffca2cf71874b786d44108e193f`. The current EV loop unconditionally marks per-stat values over 252 Invalid; checks total over 510 separately.
- Pinned [PKHeX EffortValueVerifier](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/EffortValueVerifier.cs), Git blob `86c9bc6ea0361821df86e1473788c228ce7506a0`: **only format >=6** uses the 252 hard-limit; native formats 3/4 use source/history-dependent gained-EV tests and total <=510, not 252 hard-limit.
- Pinned [PKHeX Gen III G3PKM](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/Shared/G3PKM.cs#L24) blob `1042c3edef50b4f2d16b164c56ef2a3461befc9f`, [Gen IV G4PKM](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/Shared/G4PKM.cs#L25) blob `c42aad87ec573ee59377db28a110a6403163de0c`: native `MaxEV = EffortValues.Max255`. [Pinned constants](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/Util/EffortValues.cs#L13-L17): per-stat byte max 255, total 510, modern hard cap 252.

## Finding F1 — source-confirmed false-Invalid rule risk for Gen III and IV

`NXINVALID-1629`: current NX emits Invalid for **EV 253, 254 and 255** for native PK3/PK4. Pinned PKHeX does not reject these numbers on the basis of the per-stat hard cap. Values 253–255 yield no more stat points than 252, but remain historically legal *native values* in those generations. Rule discrepancy proven by exact source; **a full native actual retail saved Pokémon incorrectly rejected by the running analyzer has NOT yet been captured**.

The independent bounded arithmetic domain enumerates both Gen III and IV with `(EV_HP, EV_ATK)` each 0..255 and four other current EVs 0. **65,536 source-contract combinations per generation; 1,527 hard-cap disagreements per generation = 3,054** across 131,072 combinations. This number is **not 3,054 global legality failures**: encounter, original training/experience stages and other independent verifiers may still reject individual whole Pokémon.

`Q12_EV_HARDCAP_FALSE_INVALID_DIFFERENTIAL.json` pins seven direct source conditions plus 24 detailed boundary vectors. `tests/fixtures/legality_l5_ev_cap_reference_diff.cpp` compiled/executed both normal `-O2` and `-fsanitize=address,undefined` on the standalone copied source-contract model, with the 131,072 domain comparison and static boundary assertions PASS. This proves an isolated rule difference; not actual `Legality::analyze` invocation.

**MAIN/coordinator fix request (NOT an L5 code change):** use the verified *native storage format* for EV per-stat cap (255 for PK3/PK4; modern hard limit 252 for later applicable formats), not the source save provider, before setting Severity::Invalid. Keep global total 510. Do not normalize/purge 253–255 from original PK3/PK4, and do not assume a Gen III-origin native PK4 uses modern rules. Native source-verified control vectors for each Gen III and IV: 252,253,254,255 per stat; [255,255,0,0,0,0] valid *cap*, [255,255,1,0,0,0] invalid *total*; 510 vs 511 boundary. Maintain standalone original/native distinction.

## Finding F2 — inspected unhatched egg EV rule gap

At pinned `EffortValueVerifier.cs` lines 16–21, a nonzero EV total on an **unhatched egg** is Invalid and terminates that verifier. NX's inspected `Legality.cpp` EV section lines 1596–1632 contains no egg-specific EV guard; its `Gen34EggState::analyze(generation,isEgg,eggLocation,metLevel)` accepts **no EV information**. This is a **missing direct verifier path**, not a proven full-system NoProblemsFound response. NX may surface other issues, and a lack of whole-native vector means this does not prove actual runtime parity failure yet.

`Q12_EGG_NONZERO_EV_MISSING_RULE.json` records seven source-scoped positive/candidate-negative/Unknown boundary controls (PK3/PK4 eggs with EV 0, 1, 100, 255; hatched comparison). MAIN must add a format-aware native egg EV rule and run real PK3+encrypted PK4 exact-head controls; preserve the distinction from missing original breeding parents.

## All Invalid issue-site review, Q18

`Q18_FALSE_INVALID_SITE_RISK_36.json` enumerates **36 exact Severity::Invalid emission sites** from the same accepted analyzer: one confirmed semantic disagreement and 35 sites requiring independent game-field/history exhaustiveness work. Other sites (Gen IV form by save source, event release status, version-specific ability/PID, move-name sentinel and historical transfer) are **risk targets, not confirmed false-Invalid bugs**.

`Q18_RELEASE_GATES_TRANCHE04.json` tracks **14 release gates**: sources, F1/F2, all 36 Invalid sites, original encounter multiset, move paths, egg binary differential, 247 authenticated original WC4 templates, original paired source saves, 200 analyzer calls/305 PKHeX declarations, 336 cells of genuine game×method parity, latest-head host/RSE/sanitizer/native CI, source license, and synthetic contract scope.

`tools/audit_legality_l5_release_gate.py --verify-manifests` checks that the many research files are internally consistent; it should exit success for **research-manifest integrity** even while full release is blocked. Without `--verify-manifests`, it exits nonzero until all release criteria are actually achieved. **Never represent a manifest-validation pass as full Gen I–IV legality completion.**

## Acceptance and handoff

- L5 draft remains isolated in [PR #294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294); no self-merge, force-push, reset/rebase backward, accepted/main movement.
- Coordinator must preserve every newer accepted commit, reconcile L5 branch forward-only to zero-behind, and run latest exact-SHA identity + host clean/full tests + focused RSE + ASan/UBSan + independent L5 source tests.
- Original-source provenance/license and original paired retail history are **not resolved** by this tranche. Unknown remains Incomplete; a failed *candidate* is not proof of global Invalid.
- Deliver F1 and F2 to master legality issue #285 and L5 #290; production fixes and integration belong to coordinator.
