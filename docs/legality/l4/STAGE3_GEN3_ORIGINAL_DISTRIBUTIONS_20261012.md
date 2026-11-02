# L4 Stage 3 — Gen III original distributions: engineering handoff

Date: 2026-10-12. Repository: GlitchedZeus/PokeBank-NX. L4-owned PR #292,
`feature/legality-l4-events-meta-20261012`. Scope is source information,
original trainer/game-font bytes, transferred gifts and original moves.
Nothing on MAIN, accepted legality, L1–L3/L5/L6, production Legality.cpp,
shared workflows or coordinator PR #298 has been changed.

## Implemented in production L4 evidence

1. `Gen3DistributionWitnessEvidence.h` rejects **unverified original release**
   evidence before any source-specific PK3 field comparison, rather than
   considering an alleged archived trainer, language or move mismatch a
   candidate exclusion. Individual original receipt remains a separate
   independently attested fact, never inferred from matching fields.
2. A possible source-to-native language conversion also quarantines **raw
   nickname and OT game-font byte comparisons**, as well as language.
   A Unicode label is not a reconstruction of Gen III native bytes.
3. New `TransferHistory` + `evaluateOriginalHistory` permits a
   **candidate-local Gen III native/source join** only after independent
   original/native record linkage, the GBA receiving version and, for
   GameCube sources, specific original GC source and transfer history.
   It separately guards original-font encoding and event move histories.
   A native GC origin marker need NOT equal the destination GBA edition.
4. Original source media are distinctly modeled as **Colosseum/XD,
   Colosseum Bonus Disc and Pokémon Channel**. Wishmaker and Channel gift
   claims cannot borrow an unrelated Colosseum/XD source identity.
   Impossible family/source-media combinations remain UnknownRoute.
5. Explicit `originalSourceRecognized` enumerates nine supported
   original-source tags, including the two distinct extra GC media, and
   quarantines out-of-domain serialized tags before source-field
   comparisons. Even all supplied candidates contradicted does not
   authenticate or globally invalidate an individual Pokémon.

## Extended original-source and native PK3 regressions

- `tests/test_legality_l4_gen3_distribution_matrix.cpp` now covers
  no-release mismatches, language-dependent raw OT/nickname evidence,
  independently attested GC/recipient link flags (32 combinations),
  256 source-family tags and 256 original-GC-source tags, explicit
  Bonus Disc / Channel / Colosseum/XD family-source pairings, 256
  serialized original-source tags, and event move-history transitions.
  Existing source-matrix / 16-field baseline remains registered.
- `tests/test_legality_l4_gen3_distribution_native_pk3.cpp` extends
  the existing **real production encrypted PK3 parser regression**:
  24 PK3 shuffle positions x stored/party layouts, native raw OT/name
  evidence, independently simulated GC-to-GBA source join, missing
  delivery witness, ambiguous game-font encoding, and an independently
  verified final move-set witness. It retains ciphertext mutation and
  checksum rejection tests and read-only source assertions.
- Synthetic fixtures prove interface and evidence classification only:
  all independently true inputs in the fixture are hypothetical.
  There are **no authenticated original receipts or retail archive payloads**
  in this test set, and no redistribution rights asserted.

## Execution evidence — distinguish actual tests from queued gates

**Actual local executions:** Using a separately developed, uncommitted
self-contained Gen III source-chain prototype under
`/mnt/data/l4-stage3`, four GCC/Clang normal+ASan/UBSan executions of
`legality_l4_stage3_chain.cpp` passed; four corresponding executions of
`legality_l4_stage3_labeled_source.cpp` passed. The former exercised
65,536 source-family/channel pairs, 256 trust-mask combinations and
eight converted-archive proof masks; the latter exercised original
GameCube/recipient source tags and Unknown receipt/transfer gates.

Command format actually executed for those **scratch-only** checks:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 \
  tests/fixtures/legality_l4_stage3_chain.cpp -o /tmp/l4stage3
/tmp/l4stage3
# Same with clang++; sanitizer runs used:
# -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
# ASAN_OPTIONS=detect_leaks=0:halt_on_error=1
# UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1
```

**These scratch tests are NOT in this PR, did not compile the committed
Gen3DistributionWitnessEvidence.h changes, and DO NOT satisfy the
registered native PK3 or full-checkout acceptance gate.** Never report
the production L4 matrix as green on the strength of these prototypes.

**Unexecuted production gates:** Full checkout is unavailable in this
runtime: `git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD`
fails `Could not resolve host: github.com`. Therefore full 28 registered
C++ suites, complete native encrypted PK3 compilation, GCC/Clang normal
and ASan/UBSan for committed Stage 3 source, source archive rights and
coordinator reconciliation are **BLOCKED / NOT VERIFIED**.
GitHub Actions exact-head L4 matrix and host-test workflow are queued,
not passed. Re-fetch and inspect those exact-head job results before
any integration approval.

## Handoff to coordinator / next L4 stage

Coordinator PR #298 may review and reconcile this informational source
candidate after it completes the exact-head CI matrix. Never infer global
validity from one template or global invalidity from a single exclusion.
Original source digests, individual receipts, GC transfer histories and
rights remain independent evidence obligations. Do not self-merge.

L4 Stage 3 source and tests are committed **but acceptance is blocked**.
Next major engineering stage: Stage 4, Gen IV PCD/PGT, Wondercard
projection, recipient-save history, Ranger and malformed source identities,
while retaining all earlier blockers.
