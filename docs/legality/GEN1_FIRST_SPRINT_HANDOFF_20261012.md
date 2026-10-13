# Gen I-first legality sprint — five specialist handoffs

**Decision:** finish Generation I (Red/Blue/Yellow) applicable rule coverage
before expanding the next generation. The five lanes continue **in parallel
by specialty**, not in parallel by generation. Work on all prior Gen II–IX
branches, PRs and source pins is preserved; no force push/rebase/backward
reset, no discarding later commits, no early merge into accepted or MAIN.
The app/editor Gen V development remains an independent product lane.

**Read this before acting:** GitHub branch HEADs change continuously; fetch
your own PR and the accepted/head coordinator first. This sprint creates
**assignments**, not proof that five other chats automatically started.

## Current reference PRs, roles and code entrypoints

| Lane | Draft PR | Owns | Existing native runner |
|---|---|---|---|
| L1 encounters/RNG | [#293](https://github.com/GlitchedZeus/PokeBank-NX/pull/293) | `include/Legality/Gen1EncounterEvidence.h`, `Gen1OriginalWildFishingStage.h`, `Gen1IndexedOriginalSourceHistory.h` and owned tests | `bash tests/test_legality_l1_run.sh` |
| L2 Gen I mechanics | [#295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295) | Gen I DV/Stat Experience source-evidence helpers and new L2-owned tests; Gen II breeding stays parked | `bash tests/test_legality_l2_run_all.sh` |
| L3 moves/chronology | [#296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296) | `Gen12CandidateMoveJointChronology.h`, original move/machine tables, Time Capsule candidate snapshots and L3 tests | `bash tests/test_legality_l3_run_all.sh` |
| L4 events/metadata | [#292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292) | Gen I original distribution, NPC trade / gift, raw name/OT/catch-rate evidence and L4 tests | `bash tests/test_legality_l4_runner.sh` plus `L4_SANITIZE=1` |
| L5 independent QA | [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294) | Source-pinned differential, PK1 native negative/Unknown and false-Invalid audit; never overwrite production `Legality.cpp` | `make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline` |

Coordination is **[#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298)**,
original accepted isolated legality is **[#103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103)**.
**[#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301)** preserves
PK5/PK6/PK7 and PKHeX host-oracle groundwork; no completion credit is assigned
to Gen I from those format tests. Independent Codex integration audit: [#299](https://github.com/GlitchedZeus/PokeBank-NX/pull/299).

## Copy-ready prompt: L1 — Gen I encounters and original RNG source

> Continue substantial code engineering on your existing L1 PR #293. FIRST refetch its GitHub head, the accepted isolated legality branch and coordinator #298; preserve every later commit, do not merge, reset/rebase or force push. STOP Gen II–IX feature expansion and concentrate exclusively on **Red/Blue/Yellow**. Reuse and audit existing `Gen1EncounterEvidence.h`, `Gen1IndexedOriginalSourceHistory.h`, `Gen1OriginalWildFishingStage.h`, `Gen1CatchRateEvidence.h`, their pinned data and owned L1 tests. Pinned original R/B source `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`, Yellow `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`; original PKHeX source map also available. Expand the existing **partial** RBY route/method coverage toward all applicable wild grass/surf, Old/Good/Super Rod, version-specific static/gifts/in-game trades. Preserve original ordered slot multiplicity, exact source version, map/level/time/method and evolution/Time Capsule unknown-history controls. Do not claim Yellow rod source evidence from R/B tables or infer original catch-rate custody from detached PK1. Deliver a substantial tranche of actual code + positive, candidate-negative, alternative-origin and Unknown native fixtures. Native normal + ASan/UBSan, `bash tests/test_legality_l1_run.sh`, exact Git blob/SHA and remaining route counts. Do not touch L2–L5 or production `src/Legality/Legality.cpp`. Unknown/unsupported routes never become global Invalid. Commit and push exclusively to the current L1 branch; report exact 40-char SHA and actual tests, marking unrun gates blocked.

## Copy-ready prompt: L2 — Gen I DVs and Stat Experience (NOT breeding)

> Continue substantial engineering on your existing L2 PR #295; re-fetch actual remote head before changes, never rewrite commits, never merge into #298/main. Redirect to **Gen I native legality semantics**: there is **no Gen I breeding, ability, gender, nature, held-item, native shiny flag or ordinary egg**. Do not invent Gen I daycare/egg validation. Source-pin and implement independent correct four DV nibble constraints with HP DV derived from four low bits, Stat Experience 0..65535 and level/stat calculation under RBY rules, and flags distinguishing native values from absent/synthesized properties. Reuse existing PK1 reader and Gen I battle-stat code after reading them; do not duplicate a parser. Include tests for valid/min/max DV/StatExp, malformed bytes/length, derived HP DV, mixed DV bit patterns, mixed levels and missing/mismatched exact original source. Cross-generation stat/gender/shiny interpretations of transferred records are NOT proof the native PK1 stored them. Keep all evidence read-only and candidate scoped; unknown outside retained context. New helpers and tests belong to L2 only; production `Legality.cpp` changes are coordinator-owned. Execute real normal/ASan+UBSan and `bash tests/test_legality_l2_run_all.sh` from a full checkout if available; otherwise state precise blocker. Commit and push substantial tested work exclusively to existing L2 PR; report 40-character SHA, tested filenames/source references and remaining deficits.

## Copy-ready prompt: L3 — all Gen I four-slot moves and Time Capsule gates

> Refetch existing L3 PR #296, accepted engine and coordinator before any write. Work exclusively on **Gen I Red/Blue/Yellow** full *version-specific* move acquisition. Build on existing `Gen12CandidateMoveJointChronology.h`, `Gen12TimeCapsuleEvidence.h` and existing native snapshots; source-pin complete original level-up chronology, 50 RBY TMs, 5 HMs, evolution and relevant NPC/Tutor/trade origins per applicable source. Do NOT treat a union of independently learnable moves as proof of a feasible simultaneous four-slot moveset. Respect each level offer, overwrite/skip decisions, last-four slot state, explicit version, origin ancestry and source-received acquisition; unrecorded trainer decisions must stay Unknown. Test RBY vs Yellow move differences, TM numbering, four-slot conflicts, evolved stages, source tradeback Gen II-only moves rejected only at a documented Time Capsule stage, and competing candidate alternatives. No unconstrained global Invalid from a single move-route contradiction. Add major code and native normal plus ASan/UBSan suites via `bash tests/test_legality_l3_run_all.sh`, show exact-head results and source coverage, and push only L3 draft branch forward preserving all work. No shared analyzer/MAIN commits.

## Copy-ready prompt: L4 — Gen I events, gifts, trainer and original metadata

> Re-fetch existing L4 PR #292 and all heads; preserve ancestry and lane boundaries. Focus exclusively on **Red/Blue/Yellow** source-verified original static gifts, NPC trades and event/distribution metadata, original species/level/OT/nickname and catch-rate custody; avoid reimplementing L1 encounter slot enumeration. Reuse current `Gen12GiftHistoryEvidence.h`, `Gen12OriginalDistributionJoinEvidence.h`, `Gen1CatchRateEvidence.h` and accepted RBY source tables. Audit Gen I representation: no native ribbons, Ball field, nature, ability, gender or standard breeding data; don't demand them. A historical Mew/event distribution must not be certified from a named modern internet table: require independent original distribution payload/license/context or keep Unknown. Handle Japanese Blue/Stadium, tradeback item interpretation and text/trainer encoding as separate source-qualified, bounded paths. Add code and native positive/selected-candidate-negative/Unknown tests, event trust/redistribution controls and `bash tests/test_legality_l4_runner.sh` plus `L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh`. Push only L4 branch, report actual tests/40-char SHA, original sources and incomplete historical categories; do not touch product/source saves, shared `Legality.cpp` or other lanes.

## Copy-ready prompt: L5 — independent Gen I PKHeX parity and false-Invalid QA

> Re-fetch existing L5 PR #294, coordinator #298 and accepted isolated legality #103 before acting. Switch audit scope to **Gen I Red/Blue/Yellow** read-only native PK1 and game-original method/evidence parity, not Gen II–IX expansion. Prioritize existing documented P0 source-free false Invalid findings **L5-F3** (native PK1 ability=0 rejected) and **L5-F4** (native PK1 neutral gender=2 rejected), with real production-linked `Legality::analyze` positive/negative/Unknown source variants, exact source-label mismatch controls and old-source/current-source gap analysis. L5 must write independent QA fixtures, source manifests, host original PKHeX oracle and fail-closed per-method scoreboard; it must **not** edit production analyzer. Verify all source-original Gen I current methods and register tested vs untested separately. An original C# PKHeX invocation is required before claiming differential parity; a source lookup, Python imitation or C++ source-reproduction alone is not original host oracle execution. Run `make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline` and applicable sanitizer variant (or explain unavailable runner), plus at least one complete native PK1-analyzer fixture cohort, all with explicit exact SHA. Flag any still-unproven original event/save histories and all current direct Invalid sites that could give false verdicts. Push exclusively to L5 branch and report exact commit SHA/tests/no fabricated green gates.

## Coordinator integration and acceptance

When specialists submit their code, the coordinator re-fetches each latest PR,
compares commits and Git trees against accepted original HEAD and all earlier
integrated work, performs **forward-only semantic reconciliation** (never
overwrite one side of a conflict), and links native tests to the production
analyzer only after scoped review.

- **Development-order gate:** Gen I applicable category implementations,
  original-source pins, positive / selected-candidate-contradiction / Unknown
  coverage and format-specific false-Invalid fixes have all been accounted for.
  Exact-head full host, focused RSE, native and ASan/UBSan must pass.
- **Reference acceptance:** original PKHeX.Core source-pinned oracle must
  execute against the relevant Gen I fixtures; differences need adjudication
  and second independent L5 review. Original-distribution/parent/trainer
  custody still unobservable from detached files remains explicitly Unknown.
  A Gen II-dependent Time Capsule back-transfer gate may be deferred with
  documented unresolved dependencies rather than faking Gen I provenance.
- **Never use an arbitrary completion percentage as a release gate.**
  A passing structural parser or 90 closed task packages is not full PKHeX
  legality parity. Neither PR #298 nor #301 is authorized to merge into the
  accepted engine or product default branch while gated.
- **Preserve the product README hero/badges/sections and owner-tested 2026-10-08
  Gen I–IV NRO boundary** independently of this legality sprint.

### Standard terminal commands from each *actual* branch checkout

```bash
git fetch origin --prune
git status --short
git rev-parse HEAD
git diff --check
# Then execute ONLY the applicable existing lane-native runner(s):
bash tests/test_legality_l1_run.sh
bash tests/test_legality_l2_run_all.sh
bash tests/test_legality_l3_run_all.sh
bash tests/test_legality_l4_runner.sh
L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
# L5 includes its own Makefile additive native probe target:
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline-sanitize
```

**These are commands to run, not tests claimed to have been executed in this
handoff.** Each specialist runs only its owned runner plus integration gates
requested by the coordinator. Preserve existing GitHub Actions, no bypasses.
