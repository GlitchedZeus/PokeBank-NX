# L2 post-freeze breeding audit — PR #298 compatibility and conservative alternatives

**2026-10-12 · L2 owner · Existing draft [PR #295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295)**

## Exact reference state and validation boundary

- Accepted Gen I–IV legality head re-fetched: `d29eada5c087e201e00c749703f1a2de3ddfadeb`; **untouched**.
- Original L2 integration freeze: `cdc128a4a6fa914f54844c372019fd995b567619`; **included** in coordinator [draft PR #298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), exact combined head `c91b53e84ef75669049d159616894ec68cd4b0dd`.
- **Real, full-checkout GitHub CI PASS for that frozen integration head**: [Actions #38287166355](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38287166355), completed success. `Legality lane L2 (native + sanitizers)` successfully checked out exact integration SHA and executed `tests/test_legality_l2_run_all.sh` (**all 11 L2 C++ programs, normal + ASan/UBSan, 22 executions**). The other L1/L3/L4/L5 jobs and combined Host Tests also passed. This **does not validate the later post-freeze L2 changes** below.
- HGSS independent PID sibling [#284](https://github.com/GlitchedZeus/PokeBank-NX/pull/284) head `dd395c3cc5881d96dd84b7af44717cb767bfdf6f` is included in #298. Do not copy its code into L2 or move the sibling ref.
- L2 post-freeze changes are additive on **only this lane's branch**. They must be integrated and **retested at a NEW combined exact SHA** before production acceptance. Branch Host Tests does not include the coordinator's special L2 matrix job. No false statement that current lane head's eleven updated tests all passed.

## Retail mechanic audit (conditional original-stage coverage only)

| Source family | Pinned source | Source-backed behavior checked and limits |
|---|---|---|
| Ruby/Sapphire | `pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f/src/daycare.c` | Pending 16-bit `(Random()%0xfffe)+1`, fresh high16 LCRNG at collection; six IV inheritance RNG draws; RS stat **value** removal (not Emerald indexed loop bug); source physical daycare parent order and four-slot move father/mother roles must be independently attested |
| Emerald | `pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/daycare.c` lines 414–487, 527–597, 762–819 | Second `Random2` RNG seeded by original VBlank counter, female/Ditto-precedence creation-time Everstone, high16 roll **strictly below 0x7FFF**, up to **2,401** PID/nature attempts retaining final fallback; different from RS/FRLG; six IV draws and original **loop-position removal**. Ditto move-father with female, or Ditto mother with male/genderless, follows original parent-slot assignment, not automatic physical-slot order |
| FireRed/LeafGreen | `pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14/src/daycare.c` | Pending low16 / collection high16 as RS; purported Everstone code **commented out** in original retail source; six IV draws and original stat-value removal. Original light-ball Volt Tackle function also commented out; do not accidentally borrow Emerald behavior |
| Diamond/Pearl | `pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290/arm9/overlays/05/asm/ov05_021EC458.s` Git blob `e084c052388870e0d3e984379b1bb23e1a911790` | `ov05_021EC9D4`: six LCRNG IV draws and indexed-position removal; `ov05_021EC8A8`/`ov05_021EC944`: original female/Ditto Everstone and 2,401 consecutive MT bounds; collection-time up to 4 `PRandom` transforms only for differing original parent languages and non-shiny PID; no DP Power-item stage |
| Platinum | Accepted `Gen4PlatinumEggPidStage.h` (separate #283) and source freeze / independent RNG-003 | No HGSS Power-item mechanism: six-draw Platinum IV replay with indexed-removal behavior. Historical Everstone and MT/PID stage is **separate accepted source**, bounded audit horizon **64** returning `SearchBoundExceededUnknown`, not impossibility. Platinum parent-item history must not be inferred from later PK4 |
| HGSS | `pret/pokeheartgold@9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/get_egg.c` lines 223–275, 294–329, 596–610, 999–1028 | One/two Everstones, two-holder LCRNG choice, strict 0x7FFF threshold; consecutive MT PID until nature match or 2,401st forced fallback; international up to 4 later `PRandom` steps. Power items: physical slot 0/1 and both holders (even identical items) determine 4/5/6-draw IV stage. Parent original held item may differ from today's PK4, so source attestation is mandatory. HGSS PID lane #284 is **separate, integrated in #298**, not duplicated here |

The source audit is based on the **pinned decompilation source**, not independently executed original retail ROMs. Independent region/binary correspondence, distribution licensing, authentic original save-backed parents and egg snapshots, original RNG transcripts/items/languages and complete other-origin coverage remain L5 release blockers. No retrospective history reconstruction from an isolated child PK3/PK4 is permitted.

## Correctness fix: a malformed sibling must not erase a legitimate history

Four previously implemented bounded helper families rejected the **entire set immediately** if *any* route identity was zero or duplicated, even when another candidate independently explained the original child. That could hide a valid breeding history (a false-negative **candidate discovery**, not necessarily a global Invalid).

**Post-freeze correction (only L2-owned source):**
- `Gen3EggFullHistoryAlternatives.h` — PID+IV+four-slot moves, up to 8 routes.
- `Gen3EggIVAttestedAlternatives.h` — original five-game IVs, up to 16 routes.
- `Gen4DiamondPearlEggAlternativeHistories.h` — original D/P PID+IV, up to 8 routes.
- `Gen4EggIVHistoricalCandidateSet.h` — source-specific Platinum/HGSS IVs, up to 16 routes.

Each independent nonzero, first-seen route is evaluated as before. Duplicated/zero identities are counted **unverified/Unknown**, never as independent evidence of exhaustive contradiction. **One independently attested consistent route takes precedence over bad sibling routes.** Without any consistent route, duplicate/zero identity remains an explicit Unknown identity status and cannot cause `All...Contradicted`. Any `All...Contradicted` label still refers **only** to that purported, independently exhaustive *local* daycare method set, never whole-Pokémon `Invalid`.

**Regression changes:** four existing L2 alternative-history C++ suites now exercise a matching original route with an additional duplicate/zero route; separately test that an all-unresolved/contradictory duplicated set cannot invent exhaustive impossibility. No broadening of permissible source-game inheritance mechanics.

## Encrypted native PK4 strengthened without production changes

- `test_legality_l2_dp_native_egg_history.cpp`: uses the exact real `Pokemon4ReadOnly` + `Encryption4` production API and synthetic **encrypted 136-byte stored and 236-byte party egg** records, with unchanged source encrypted bytes. Independently proposed original Everstone-present versus absent parent-item histories for the same egg demonstrate that one locally contradictory candidate does not erase a different coherent candidate. Original actual creation-time held item evidence is **not** inferred from retained parent held items.
- `test_legality_l2_gen4_native_iv_history.cpp`: uses original encrypted HGSS parent+egg, one Power-item hypothesis locally contradicted, a second independently attested history matching the original IVs and a third **duplicate archive route**. Conditional match must survive the contradictory/duplicate siblings; removing original held-item evidence from the valid route leaves a missing/duplicate identity **Unknown**, not whole-record Invalid. Source PK4 bytes checked unchanged.

**Native Gen III PK3:** L2's Gen III historical witnesses remain explicit-source PID/IV/move helpers, not a PK3 record provenance extractor. The current coordinator integration has independent encrypted PK3/PK4 codec and production analyzer tests in L5 (including 336 codec roundtrip inputs and 52 synthetic analyzer boundary cases), but these **do not prove real original daycare histories** or L2 end-to-end PK3 native provenance binding. This remains explicitly Incomplete rather than fabricate a validated Gen III native adapter.

## Checks completed / still required

- Live GitHub branch/PR/accepted/integration heads retrieved before modifying. **No accepted, integration #298, other lane, MAIN, shared CI, or production `Legality.cpp` changes**.
- Post-freeze diff restricted to **4 L2 header files + 6 existing L2 C++ tests**. All ten current GitHub blobs retrieved; bracket/source-structure checks passed; GitHub compare reported no trailing-whitespace/conflict-marker defects.
- **Baseline** integrated exact SHA `c91b53e84ef75669049d159616894ec68cd4b0dd` all 11 new L2 tests **PASS** under coordinator CI, normal + ASan/UBSan, along with combined host tests. **Post-freeze changes not present in that integration SHA**, and their full C++ compilation/ASan/UBSan on an integrated exact head is **NOT YET ATTESTED**. The L2 PR branch's own Host Tests workflow does not register them; do not mistake default Host Tests success for this new regression gate.
- Coordinator must *forward*-integrate only this lane's new commits after frozen head, re-run all 11 L2 programs using actual complete original `Gen34EggMoveData.inc` and production PK3/PK4 parsers, **normal + ASan/UBSan**, and rerun full combined host/RSE/ASan/UBSan/device gates. No force push, reset, backward rebase, self-merge, or replacement of other lanes.
- **Cross-lane audit note:** Accepted `Gen4PlatinumEggPidStage::evaluateUniverse` still pre-checks duplicate IDs for the entire history set. It returns `UnknownCandidateIdentity` (not global Invalid), but could similarly suppress an independently matched alternative. **Do not edit accepted code in L2**; coordinator/Platinum owner should review an equivalent conservative positive-witness rescue with original native source identity retained.
- L5 primary-source gaps: 0 fully original-retail binary/region certified game-family parity cells per Q17; no immutable original paired daycare parents, egg, PID/IV RNG transcript, held-item chronology or complete acquisition-path enumerations; licensing and provenance reviews not signed off. **Keep unsupported/ambiguous histories `Incomplete` / `Unknown`, never whole-Pokémon `Invalid`.**

**Disposition:** L2 post-freeze improvement is **source-review ready; new combined-head CI still required**. No further feature expansion until coordinator integrates this batch.


## Follow-up: order-independent duplicate-ID rescue (second validation pass)

After the initial patch, the archived-route audit identified a **second** candidate false-exclusion: when an attested contradictory original-stage proposal appeared **before** a separately attested matching proposal with the **same nonzero route ID**, the first-seen-only deduplication logic skipped the later match. This makes the candidate result order-dependent, even though the later independently supported original parent/egg history could explain the observed child.

All four L2-owned helpers now:

1. Reject a **zero ID** as Unknown, not an independently provable original egg history.
2. Treat a later *duplicate* ID as unverified for historical route completeness, and **never count its negative stage mismatch** toward exhaustive rejection.
3. If another route with the same ID **already conditionally matched**, skip the duplicate so a repeated positive is not double-counted.
4. Otherwise independently assess the later proposed stage's positive condition against the original source and observed egg; a later attested match **rescues the candidate set** despite the earlier contradiction.
5. If no matching route exists, preserve `MissingOrDuplicate...` (Unknown/provenance) and **never promote candidate exhaustion to whole-Pokémon Invalid**.

A further four existing L2 test programs now assert **both history orders** and no fabricated exhaustive rejection. This is a *bounded candidate-discovery correction*, not evidence that any test fixture came from an authentic retail egg.

**Execution caveat remains:** the last fully green all-11 L2 **normal and ASan/UBSan** run is the *frozen* integrated SHA `c91b53e84ef75669049d159616894ec68cd4b0dd`. The latest L2-only branch includes these post-freeze changes and **does not have an L2-specific exact-head CI run yet**. The branch's ordinary Host Tests job does not invoke `test_legality_l2_run_all.sh`. Coordinator must carry this **entire post-freeze L2 diff** into a *new* combined candidate and rerun the 11-program matrix plus full host/RSE/sanitizers. Do not cite the prior green SHA as validation of the changed test sources.

The accepted Platinum-only `Gen4PlatinumEggPidStage::evaluateUniverse` duplicate-ID early rejection remains **coordinator/accepted owner territory**. No L2 edit or inference of a global Invalid from that possible masking issue.
