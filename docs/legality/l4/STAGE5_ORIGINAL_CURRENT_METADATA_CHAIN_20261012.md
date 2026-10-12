# L4 Stage 5 — Original-to-current Gen I–IV metadata chain handoff

**Date:** 2026-10-12. **PR:** #292 only.
**Branch:** `feature/legality-l4-events-meta-20261012`.
**Status:** substantial L4 implementation + focused exact-source verification
delivered; **full acceptance remains BLOCKED**.

## Implementation

New `include/Legality/Gen14OriginalCurrentChainEvidence.h` is a
read-only, header-only, original-to-current **candidate-scoped**
metadata-history gate. Unlike a lone final PK4, the evaluator requires:

- Independently identified original distribution/source and original
  native record, both native record digest attestations, source-to-final
  individual record continuity and independently validated final native.
- Each intermediate hop is bounded (maximum 16), directionally linked
  with digest of actual source/target records, independently attested
  identity of the same individual, and separately extracted before/after
  canonical snapshots. An unreconciled digest never guesses a bridge.
- Only documented generation/action transitions (not arbitrary
  serialized action or generation tags). Time Capsule is I↔II;
  Pal Park III→IV; same-generation actions stay in their generation.
- **Native field availability by generation:** PK1/PK2 do not supply a
  standalone original Ball or language byte; Gen I has no native egg
  or held-item field; Gen III lacks DS met-calendar bytes. Incompatible
  source-native field claims remain UnknownProjection.
- Track each field's evidence through EACH hop: raw nickname/OT encoding,
  evolution, transfer and calendar changes cannot preserve their
  original comparison value without an independent endpoint witness.
  Missing intermediate observations downgrade ONLY those fields to
  Unknown; independent observation of other fields remains useful.
- Attested `verifiedAfter` snapshots cannot override normally
  immutable trainer IDs, balls or other fields outside an action's
  documented mutation/re-encoding domain. This closes a false
  source-evidence promotion in malicious transfer manifests.

A conditional candidate is NEVER an authentic original distribution,
a proven historical player receipt, or a globally legal Pokémon.
Candidate mismatch is scoped to exactly one supplied original record and
its independently linked transfer history. No source save writes,
original archive binaries, copyrighted Wondercards, game ROMs or
original retail gift records were added.

## Exact code / tests / CI registration

New source: `include/Legality/Gen14OriginalCurrentChainEvidence.h`

Git blob SHA-1:
`0c9bb43d63e3f5a31c11c364d4c2bbd737a3f09f`

New regression:
`tests/fixtures/legality_l4_stage5_original_current_chain.cpp`

Git blob SHA-1:
`e3ef4561112ba95c6fac6a01862e1227822ebf2e`

Previously accepted existing dependency
`include/Legality/Gen14HistoricalEventPathEvidence.h` was byte-for-byte
verified against `3c9f399282d67b5ba872298b288bcf77635f455c`.

L4-specific `tests/test_legality_l4_runner.sh` now compiles and runs the
new fixture **in addition to** its 28 registered C++ suites; the L4-owned
workflow path filters also include both the new production header and
the auxiliary fixture. The 28-suite root registry was NOT changed.

## Tests actually executed (12/12 PASS)

Available compiler versions: GCC 14.2.0, Clang 17.0.0. All of the
following were independently compiled and executed, both normal
`-O2` and instrumented `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer`, under each compiler:

1. `tests/fixtures/legality_l4_stage5_original_current_chain.cpp`.
   **32,768** field comparison masks, **256** source/record-link proof
   masks, **256** raw action labels, Time Capsule I→II→I,
   Pal Park III→IV with subsequent evolution, snapshot/digest drift,
   masked OT/raw-name/DS-clock histories, invented PK1/PK2 language,
   immutable trainer-ID override, 17-hop bound.
2. `tests/test_legality_l4_historical_event_path.cpp`.
   **32,767** field-mask negatives, **120** action/field combinations,
   independently verified transformation and invalid-tag boundaries.
3. `tests/test_legality_l4_historical_adversarial.cpp`.
   Multi-step mutation and corrupted-middle-hop history regression.

Both original registered tests and the historical production header
were copied without substitution from exact GitHub blobs before
running. Locally tested new source/fixture `git hash-object` SHA-1s
match the final committed GitHub blobs above.

Representative commands (each run used the exact source files):

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -Wno-unused-parameter -Iinclude -O2 \
  tests/fixtures/legality_l4_stage5_original_current_chain.cpp \
  -o /tmp/l4-stage5
/tmp/l4-stage5
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -Wno-unused-parameter -Iinclude -O1 -g \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/fixtures/legality_l4_stage5_original_current_chain.cpp \
  -o /tmp/l4-stage5-san
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4-stage5-san
```

Same executions with `clang++` and both existing registered tests
were completed and **all 12 PASS**, no ASan/UBSan finding.

## Blockers and ownership

Local full clone was attempted; `git ls-remote
https://github.com/GlitchedZeus/PokeBank-NX.git HEAD` fails:
`Could not resolve host: github.com`.
The complete **28 L4 C++ suites** and all normal plus sanitizer CI
jobs at this exact new source HEAD have NOT run locally. Host CI and
native encrypted PK3/PK4/Ranger code integration are not proven green.
The GitHub Actions exact-head matrix has remained queued; inspect
the latest exact-head run and its individual jobs before gate
acceptance. We **do not** merge or self-promote to accepted legality
or into the coordinator.

The L4 source-evidence lane owns this implementation, not the
production global legality verdict. Coordinator PR #298, accepted
`feature/legality-engine-gen1-4-20260929`, MAIN and all other
development lanes remain unchanged by this session.

**Stage 5 engineering: delivered. Stage 5 acceptance: BLOCKED.**
**Next stage (one major session):** Stage 6 authenticated source,
license/rights and L5 Q07/Q08 cross-check gates, with independent
source content and no copyrighted payload embedding.
