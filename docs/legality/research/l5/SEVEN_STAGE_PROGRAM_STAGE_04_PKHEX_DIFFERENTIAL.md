# L5 Seven-Stage Engineering Program — Stage 4 Pinned PKHeX Differential

**Date:** 2026-10-12. **Lane:** L5 independent QA only; existing draft PR #294.
This is independently reviewable executable engineering, not an assertion that
a full PKHeX legality engine or original retail specimen was executed.

## Original source revisions used for the independent models

PKHeX revision **6501f0ab46e8f8ca048539dbaf8cae8cb104e722**.
The following exact Git blob IDs were fetched through the GitHub connector
and their pinned method semantics inspected:

- `PKHeX.Core/Legality/Verifiers/EffortValueVerifier.cs`:
  `86c9bc6ea0361821df86e1473788c228ce7506a0`
- `PKHeX.Core/PKM/Util/EffortValues.cs`:
  `5904496c4e60df3eb421bdf4c387f9df08f130ad`
- `PKHeX.Core/Legality/Restrictions/GBRestrictions.cs`:
  `4e741e87568012384c50d245ba5c5953b95f2605`

Historical accepted L5 analyzer source:
`src/Legality/Legality.cpp`, Git blob
`b1d02d6620e8fffca2cf71874b786d44108e193f`.
The real, compiled original NX Time Capsule helper, Git blob
`4be5eda0dac2f43e886024d2febc07d9639abc18`, was reconstructed
byte-identically for independent host testing.

**Current coordinator distinction:** coordinator PR #298 as re-fetched at
`ad9c1c859f94c0321a5d17489c72b0e7dbb9f440`
has `Legality.cpp` Git blob `4ac8a0dc0badb8211eb105811eb3fa9f928e36e4`.
Its native-format Gen III/IV EV code at about lines 1653–1671 now permits
byte-native values 253..255 unless other stored constraints fail.
Therefore **3,054 NX-only-invalid results below are old accepted-source
model deltas**, not 3,054 new/current coordinator defects. The current
coordinator F5 also reads all five PK2 stat-experience fields for
unhatched egg checks. Actual current coordinator analyzer execution is pending.

## Major executable implementation

1. `tests/fixtures/legality_l5_stage4_pinned_oracle.cpp`, exact original
   fixture blob `1e840be5a733b12457aef029c0ceea3978679ea4`.
   Independently implements bounded, *method-specific* pinned PKHeX EV
   checks and models historical accepted PokeBank EV checks; links and
   **executes** the actual original PokeBank NX Time Capsule helper. It
   covers all 65,536 possible 16-bit Gen I/II stat-experience values
   across five distinct positions, native Gen II egg/hatched alternatives,
   all 256² two-stat EV value pairs across PK3 and PK4, incomplete versus
   hypothetically independently grounded encounters, level-100 history,
   unchanged EXP and vitamin-only values. Native species 1..251 are
   distinguished from PKHeX modern evolutions outside native PK2;
   current back-trade ability is distinct from historical Gen I ancestry.
   Unknown Korean trade-policy history remains unresolved. Empty slot
   species 0 is not treated as an illegal retail Pokémon.

2. `tools/audit_legality_l5_stage4_oracle.py`, exact Git blob
   `143b8319f2b258323bfdf03b5cd96be2a2746598`.
   Requires exact production header + fixture hashes, optional local
   PKHeX and accepted-source hash verification, GCC and Clang optimized
   plus ASan/UBSan, strict result accounting and 13 parser/receipt
   controls (11 malformed rejected, two valid baselines accepted).
   Provides an opt-in **direct live runtime adapter** protocol that
   executes BOTH external engines and binds exact input SHA256,
   engine ID, case ID, executable SHA256 and verdict. With no adapters,
   the runtime gate fails closed. Source-modeled results can never claim
   the runtime adapter executed.

3. `docs/legality/research/l5/L5_STAGE4_RUNTIME_VECTOR_PROTOCOL.json`:
   22 deliberately synthetic and unauthenticated input descriptions
   for eventual two-executable differential integration; zero original
   Pokémon histories are attributed to these vectors. No engines or
   original saves are invented.

4. `.github/workflows/l5-independent-pkhex-stage4.yml`: isolated L5-only
   exact-head workflow using a sparse checkout of the pinned PKHeX commit,
   then verifying those **specific C# Git blobs**, verifying the exact
   NX analyzer source blob, and running all four C++ modes. It asserts
   runtime verifier flags and full-parity certificate counts remain zero.
   No shared coordinator CI or source files are edited.

## Tests actually executed with local byte-identical source

The four original-codec/helper fixture runs were compiled with
`-std=c++20 -Wall -Wextra -Wpedantic -Werror` and:

| Compiler/mode | Actual result | Assertions |
|---|---|---:|
| GCC 14.2, O2 optimized | PASS | 4,390,400 |
| GCC 14.2, O1 ASan+UBSan | PASS | 4,390,400 |
| Clang 17, O2 optimized | PASS | 4,390,400 |
| Clang 17, O1 ASan+UBSan | PASS | 4,390,400 |

Total in the final verification matrix: **17,561,600 executed assertions**,
with **1,638,410** method-scoped EV comparisons and **720,885**
historical/format-scoped controls **per binary**. Test output counts
are identical across all compilers and sanitizer modes.

The method-scoped modeled classifications per binary were:
- reference-only Invalid: **1,238,477** (mostly unimplemented historical
  stat-experience methods in the old L5 accepted-source model);
- both Invalid: **137,178**;
- both without an observed rule contradiction: **128,871**;
- historical accepted-source NX-only Invalid: **3,054**;
- incomplete historical-method outcomes: **127,938** including 162
  otherwise possible gen1-ancestry hypotheses with unmodeled Korean policy;
- suspected historical overstatement with missing original routes: **3,054**.

These categories are NOT 336 full-Pokémon game/family cells, are NOT
actual PokeBank analyzer runtime outcomes, and do NOT establish original
distribution legitimacy. Method-level `NoContradiction` is not `Legal`.

Additional tests:
- Python attestation and runtime-receipt controls: **13 PASS**
  (11 corruption tests correctly rejected; two valid baselines accepted).
- GCC `-DNDEBUG` compile: **expected failure confirmed**; assertions
  cannot be disabled to produce a false-green suite.
- `--require-runtime` without both verified runtime adapters: **expected
  failure confirmed**, explicit BLOCKED (not a live verifier PASS).
- One-adapter-only argument: **expected failure confirmed**.
- Earlier test-harness failures (GB five-stat reasoning, scope over 251,
  count accounting) were corrected before final successful four-mode runs.
  None constituted a confirmed production bug.

## Missing gates and required coordinator action

**No full repository checkout** was obtainable locally: GitHub clone
fails DNS resolution. This prevents local linkage/execution of the full
production PokeBank NX `Legality::analyze`. No `dotnet` runtime is
installed locally, and no PKHeX verifier executable has been built or run.
The independent L5 Stage 4 workflow can verify pinned C# source bytes,
but checking source bytes **does not execute PKHeX**. The adapter protocol
cannot be marked PASS until a real PKHeX program is built and invoked.

**Release acceptance remains BLOCKED**. Original game-save history,
paired distribution records, licensing, 336-cell game/family parity
and full historical-route exhaustion are not attested.

Coordinator recommendations in order:
1. Execute the exact-head Stage 4 independent workflow on a full checkout,
   inspect any failed compilation/sanitizer/PKHeX pin gate and retain logs.
2. Preserve the source-generation-format EV correction and F5 PK2 egg
   regression on PR #298; do not regress to historical accepted errors.
3. When actual executable adapters exist, run paired PKHeX and NX
   reference observations on **synthetic** vectors, storing executable
   identities and input digests; still no full historical certification.
4. Never turn a source-modeled discrepancy or incomplete encounter
   into a global `Severity::Invalid` without exhaustive original
   alternative routes.

**Next distinct user continuation:** L5 Stage 5 — cross-lane adversarial
correctness. Do not advance Stage 5 during this Stage 4 session.
