# L1 validation-and-completion handoff — 2026-10-12

**PR:** [#293](https://github.com/GlitchedZeus/PokeBank-NX/pull/293), draft. **Owned issue:** #286; **coordinator:** #285; independent research/QA: #290. **Lane ref:** `feature/legality-l1-encounters-rng-20261012`. **This file's addition advances the head past audit snapshot `c2f46474e24588ea8537d02e9343b3499bb982b3`; re-fetch the exact CURRENT PR head before checkout, test or reconcile. Do not treat old snapshot SHAs as latest.** No new encounter feature expansion until the coordinator accepts this tranche.

## Validation inventory — evidence, not optimistic test claims

| Gate | Verified result / condition |
| --- | --- |
| Live branch/PR and target | PR #293 draft, still open; author ref matched PR HEAD at audit checkpoint; accepted legality `d29eada5c087e201e00c749703f1a2de3ddfadeb` |
| Forward-only ancestry | `compare_commits`: **L1 ahead 29, behind 2**, common ancestor `0dccf2e10289164ad0d065985dcb29f14fd2fb35` (before this documentation-only commit). New accepted files: `Gen4PlatinumCompleteEggHistory.h`, `Gen4PlatinumEggPidStage.h`, `GEN4_PLATINUM_DAYCARE_PID_EVIDENCE.md`, and modified `tests/test_gen34_ball_domain_analyzer.cpp`. No L1 file collision; **not yet reconciled**. Coordinator owns forward-preserving merge/validation |
| L1 standalone tests | **13 C++ source files exist**, all 13 suite names registered in `tests/test_legality_l1_run.sh`; **14/14 direct project headers** found on exact author head; **260 `assert(...)` sites** statically counted across 13 C++ files (some loop over many fixtures). This is source inventory, **not C++ compilation or execution** |
| Exact runner provenance | Exact `tests/test_legality_l1_run.sh` copied into restricted compiler container; local `git hash-object` `41036003f50f8e4beaaada1d20d2ede27f15f262` **matches** GitHub blob SHA for audit head `c2f46474...`. Ran `bash -n`: **PASS**. In disposable local git fixture, supplied false `L1_EXPECT_SHA` and `L1_EXPECT_TREE`, each correctly refused execution with exit **2** |
| Independent fixture source check | Bayleef Colosseum physical row 1 has `TeamSet::First` (empty CPU-lock variant). Corrected old unverified `0xCC0DE0A1` to accepted ordinary CXD `PID 0x0985A297 / IV {6,1,0,7,17,7}`. Independently reversed accepted XDRNG sequence and replayed seed `0x00000000` to expected PID and both IV words; **stage verified in independent JS**, **not** full C++ suite |
| Previous session's independent native RNG QA | Eight synthetic original RSE/FRLG roamer forward LCRNG vectors and 24 negative controls reportedly ran under optimized GCC and independent ASan/UBSan harness in earlier L1 work. **Not re-executed this validation session**, and separate harness is NOT any of the 13 repo-native test suites |
| Full 13 suite host / ASan+UBSan execution | **BLOCKED / NOT EXECUTED** at exact author head. Source checkout is not mountable here; direct GitHub DNS and outbound HTTPS failed in container, while GitHub connector exposes text/file APIs but no attached checkout or executable workflow dispatch. Do not mark any of the 26 planned host/sanitizer suite executions PASS |
| Shared host CI | Latest pre-documentation author HEAD run [38220458165](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38220458165) was **QUEUED** at audit. Even if successful, present `host-tests.yml` runs shared `Makefile.host host-test`, focused RSE, and `host-sanitize` but does **not call L1 standalone runner**; cannot cover these new tests until coordinator registration |
| Production legality reporting | `src/Legality/Legality.cpp` currently does **not include/call** `Gen14EncounterCandidateUniverse.h`, `Gen3OriginalRoamerStage.h`, `Gen3GameCubeOriginalFixedHistory.h`, `Gen4SinnohSpecialOriginalStage.h`, `Gen3GameCubeIndexedShadowHistory.h`, `Gen2IndexedOriginalSourceHistory.h`, or `Gen4StaticIndexedOriginalHistory.h`. New contracts therefore cannot be claimed production-integrated |
| Source-grade gaps | E01 RBY original wild multiset; original GSC Headbutt/clock/tree-ID/swarm and tradeback provenance; GBA Method H all lead conditions, original GameCube CPU/team/disc and XD PokeSpot variants; original DPPt/HGSS Radar/ChainShiny/CuteCharm/Pokewalker; Method J/K unlimited rejects; native original save/snapshot witnesses. L5 Q04/Q06/Q09 evidence remains incomplete. No global Invalid from isolated local negatives |

## Known defects corrected in this validation pass

1. **Bad Colosseum Bayleef positive fixture** in `tests/test_legality_l1_gamecube_indexed_shadow.cpp`: predecessor used a PID assembled from XDRNG without its mandatory intervening ability call, so the supposedly positive test was unsupported. Replaced with the independently accepted standard CXD `0x0985A297` / `{6,1,0,7,17,7}` (first/no-lock source).
2. **Malformed original GameCube IV handling** in `Gen3GameCubeOriginalFixedHistory.h`: source snapshot with IV outside 0..31 now returns `UnknownMalformedOriginalNativeIvs` and maps to `UnknownOriginalHistory` in E17 instead of `CandidateOriginalPidIvContradicted`; updated negative fixtures and test mapping.
3. **Wrong-commit testing risk:** `tests/test_legality_l1_run.sh` prints real application/tree SHA and optionally refuses mismatched `L1_EXPECT_SHA`/`L1_EXPECT_TREE` before compiling, with two exact-blob-tested negative guard cases. Sanitizer `-fno-sanitize-recover=all` and strict ASan/UBSan halt handling preserved.

## Coordinator-only exact-head run plan

Run on a **real full git checkout with recursive submodules** and checkout the PR's latest exact SHA, not GitHub synthetic merge ref. The commands below are required gates, NOT tests reported complete:

```bash
git fetch origin --prune
# Resolve the LIVE PR head from GitHub (do not reuse the audited old SHA).
git switch --detach <LIVE_L1_PR_293_HEAD_SHA>
git submodule update --init --recursive
git diff --check
git rev-parse HEAD
git rev-parse 'HEAD^{tree}'

# Run ALL thirteen source suites, each O2/-Werror then ASan/UBSan:
L1_EXPECT_SHA="$(git rev-parse HEAD)" \
L1_EXPECT_TREE="$(git rev-parse 'HEAD^{tree}')" \
bash tests/test_legality_l1_run.sh

# Exact existing shared host gates (from .github/workflows/host-tests.yml):
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize

# Also run the workflow's "Focused RSE save-open bridge regression"
# both -O2 and ASan/UBSan as defined in host-tests.yml.
```

**Missing shared registration:** `Makefile.host` includes a fixed set of `Makefile.host.*` fragments and none for L1. `.github/workflows/host-tests.yml` never runs `bash tests/test_legality_l1_run.sh`. The coordinator should register the L1 suite as a dedicated CI step (guard exact PR head/merge head with environment-derived SHA, and include both profiles), and, after accepting production wiring, integrate its permanent host target into coordinator-owned shared Makefiles/workflows. Do not assume a successful legacy host job proved L1. No L1 edits to shared files.

**Production integration:** coordinator must wire source-attested original encounter observations with real original save/history provenance, preserve Unknown/incomplete alternatives, avoid global Invalid when one or more candidate families locally fail, and validate with L1 + L2–L5 tests on the final merged exact SHA. Wiring without archival metadata should leave histories Incomplete; do not fabricate original time, trainer, lead, transfer, game/version or capture-source witnesses from a detached Pokémon.

**Merge block:** latest author branch is **diverged** from accepted (accepted contains two newer commits) despite GitHub reporting the PR mergeable. Coordinator should forward-preservingly integrate accepted changes, independently run all L1 native suites (fix any compiler/link/runtime/sanitizer failures), all existing host/RSE/sanitizers, then accept the exact **combined** head. No force push, backward reset/rebase, self-merge, MAIN or umbrella #103 merge. All E01–E18 remain partial/needs original proof; no full PKHeX parity claim.

## Held after this package

This is a **validation/handoff freeze** of L1 #293. No new L1 encounter feature expansions are planned before native-suite and coordinator acceptance. Outstanding L5 original-source packets are listed in earlier L1 docs and #290; do not close research-blocked catalog rows on test-source compatibility alone.
