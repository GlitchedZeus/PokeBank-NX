# L5 Stage 4 — Major independent adversarial testing of L1–L4 helpers

Date: 2026-10-12. Repository: GlitchedZeus/PokeBank-NX. Existing PR #294, branch audit/legality-l5-research-qa-20261012. No writes to production, siblings, MAIN, shared CI or original Pokémon saves.

## Verified executable engineering

Four independent L5-owned C++ fixtures compile against **actual header-only production helper sources** from the four sibling PRs. Reconstructed local header files were checked with `git hash-object` to match exact GitHub blobs; committed L5 test source blobs were likewise checked byte-for-byte.

| Lane | Actual tested source helper | GCC O2 assertions | GCC ASan+UBSan | Clang O2 |
|---|---|---:|---|---|
| L1 | HGSS original Headbutt group/empty-tree provenance | 635,893 | PASS | PASS |
| L2 | Diamond/Pearl original egg IV inheritance RNG and witness | 327,683 | PASS | PASS |
| L3 | Versioned GBA Kanto starter and Nincada/Ninjask/Shedinja original move-source tables | 885,145 | PASS | PASS |
| L4 | Gen IV original event clock/egg/met receipt decision | 828,165 | PASS | PASS |
| **Total per compiler mode** | **Four actual helper C++ source units** | **2,676,886** | **PASS** | **PASS** |

**12 actual compiled and executed programs** under GCC 14.2 `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror`; GCC 14.2 `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer` with ASan/UBSan; and Clang 17 `-O2 -Werror`. No original save or upstream PKHeX executable was run. This is significant helper-level regression evidence, **not** proof the entire integrated legality analyzer works or that retail event histories are authenticated.

## New implementation

- `tests/fixtures/legality_l5_stage4_l1_tree_adversarial.cpp`: HGSS exact-game, full-trainer-ID-derived A/B/empty trees; 39,729 local source-compatible decisions, 39,729 local-only contradictions, 347,760 missing original-history Unknown, 9,951 empty tree candidates; special-tree identity/source negative controls.
- `tests/fixtures/legality_l5_stage4_l2_egg_iv_adversarial.cpp`: 8,192 independent original RNG seeds in both Diamond and Pearl, six LCRNG draw/rewind and parent donor paths, malformed IVs, unsupported Platinum/HGSS, 81,920 missing attestation Unknown, 32,600 source-local contradictory cases, 7,624 repeated inherited indices.
- `tests/fixtures/legality_l5_stage4_l3_move_source_adversarial.cpp`: exhaustive row/level/move domain against actual L3 version-specific source tables, historical per-game level changes, and source-free/partial-history Unknown rather than global Invalid. 149 exact source offer matches, four genuine local version-row divergences, 382,674 absent-level Unknown; checks lv20 triple Ninjask offers and separate Shedinja path.
- `tests/fixtures/legality_l5_stage4_l4_clock_adversarial.cpp`: complete Gen IV 2000..2255 byte-representable year/month/day grid, missing date authenticity, egg-vs-met field selection, release-window boundaries, forged leap days, incomplete egg/hatching transformations, clock changes and 128 evidence-bit masks. 93,259 candidate-local out-of-window, 24,770 native-calendar Unknown, zero authenticated/global verdicts.
- `tools/audit_legality_l5_stage4_crosslane.py`: fail-closed runner using **four independently supplied lane checkout roots** (`--l1-root` through `--l4-root`), byte-verifies the five original header SHA blobs and four L5 fixture SHA blobs, builds isolated temp headers, compiles GCC normal and ASan+UBSan for all four, optional Clang. Reports structured JSON. Runner `--self-test` PASS including negative attestation controls; an independent single-byte header tamper test returned nonzero with expected SHA mismatch.
- `docs/legality/research/l5/Q20_STAGE4_CROSSLANE_ADVERSARIAL_EVIDENCE.json`: machine-readable exact heads, blobs, metrics, boundaries, issue taxonomy, coordinator blockers.

## Notable QA finding — L5-S4-COLLISION-168 (test assumption fixed)

The initial test **incorrectly assumed** that a different final RNG state in Diamond/Pearl necessarily produces different six-IV child values. The observed bounded 16,384 DP seed/game controls contained **168 distinct-seed alternatives that yielded identical final six-IV results**. The final test explicitly replays the alternative and treats this result as **another candidate-compatible local history**, not a false impossible verdict. The corrected code passes all three compilers, including ASan+UBSan. This is an independent QA/test-model correction and non-injectivity evidence; it is **not yet a production L2 bug** and is not an original retail egg.

## Exact source pins

- L1 Gen4HgssOriginalTreeStage.h: `9c767ede360d503baa5831db3ce9c6eff04c9441` (PR #293 head at inspection `7a972afec0f2382c79cec6c13cf4044215e1d710`).
- L2 Gen4DiamondPearlEggIVStage.h: `6b1d2a7bc2299fe7780deba849caf033fa2ff0f5` (PR #295 head `4bd51f0b1cdd879bf6f1a8217d246ebc91bb4a15`).
- L3 Gen14CandidateMoveGbaStarterTables.h: `c60d5594d92bae65b8e4d1f46e2dbede27227a40`; Gen14CandidateMoveGbaNinjaTables.h: `ed5510aedd2440bfd7d6bd6e29650d5d2ebbe4dd` (PR #296 head `76de17be98914329d0b55017dd8c3a5d1d9d85e0`).
- L4 Gen4EventClockReceiptEvidence.h: `1d9bef10b28ca58d54f0539eb74712be37d4ab71` (PR #292 head `85a9856fe6d326b6dddad6152ecfd6751539c25e`).

## Coordinator replay and remaining blockers

From L5 checkout, with independent full sibling checkout dirs, run:

```sh
python3 tools/audit_legality_l5_stage4_crosslane.py --self-test
python3 tools/audit_legality_l5_stage4_crosslane.py --l1-root /checkout/L1 --l2-root /checkout/L2 --l3-root /checkout/L3 --l4-root /checkout/L4 --clang --json-out /tmp/l5-stage4.json
```

1. **P0** Run these exact checks from a full Git checkout for independent reproducibility; source SHA drift must fail and be reviewed, not bypassed. This completed run reconstructed precisely byte-verified helper source files in an isolated local build, but not a single reconciled full checkout.
2. **P0** Verify the integrated production `Legality::analyze` L1–L4 wiring and all normal/sanitizer CI gates on coordinator PR #298. Its last inspected integration head was `2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc`; earlier workflow `38296998403` failed L2 despite other lane jobs passing.
3. **P0/P1** L3 actual same-individual full four-move joint acquisition/transfer chronology is NOT executed by this source-table helper suite; L2 original physical parent pair/PID alternatives and L1 full encounter family exhaustiveness remain unproven.
4. **P1** L4 original archive authenticity, immutable receipt and event date/time evidence are not provided; mismatch never proves global Invalid. No original retail files or exclusive PKHeX-generated whole-mon verdicts were used.
5. **P1** Stage 5 matrix and release-gate automation must not raise any of the 336 family/game cells to PKHeX parity solely because helper-level synthetic regressions passed.

Original saves remain immutable, no producer/editor writes, no cross-lane changes, no force push/reset/rebase, no merge. Next major work: **Stage 5 — executable 336-cell evidence and release gates**, with fail-closed provenance and whole-analyzer controls.
