# L5 seven-stage program: Stage 3 native PK3/PK4 crypto QA

Date: 2026-10-12. L5 PR #294 remains draft and unmerged.

## Implemented code

- tests/fixtures/legality_l5_stage3_native_crypto_fuzz.cpp: 14,336 PK3/PK4
  synthetic stored/party specimens covering every PK3 24-order permutation
  and PK4 32-order shuffle; 16,896 single-data-bit ciphertext mutations;
  36,864 checksum-refreshed metadata alterations (species, original game,
  held item, moves, IV32/Egg, flags, Bad Egg, event/ribbons, form/gender,
  PP/EV); and 301 bounded transport lengths.
- tools/audit_legality_l5_stage3_native_fuzz.py: exact source/fixture Git blob
  pins, independent checksum/result attestation, GCC/Clang optimized and
  ASan+UBSan modes, JSON scope and original-header versus isolated shim
  distinction; never claims whole analyzer success.
- tests/fixtures/legality_l5_stage3_native_entity_boundary.cpp: 8,921
  additional planned checks against the actual linked PK3/PK4 entity
  classes (synthetic native records, Bad Egg/header state, immutable source
  ciphertext and corrupt-PK4 semantic quarantine).
- tools/audit_legality_l5_stage3_entities.mk: separate normal/sanitized
  entity linkage against original full production host source graph.
- .github/workflows/l5-independent-native-stage3.yml: L5-only exact-head
  workflow requiring both original codec tests and real entity linking.

## Actual executed normal/sanitizer tests

The local directory contains the EXACT original PokeBank NX production
codec C++ files, verified by Git hash-object against live GitHub refs:
PK3 eec27b73a15d6652371ed2e066690ef0fa9fb7ef;
PK4 406935761713db95ae738398b65ba0a37d4cf97b.
Fixture exact Git blob: 15a0e2f6d7b37f93a7359ea9687c15180ceedf97.

Compilers and commands, all actually compiled and executed:
- g++ 14.2 -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror
  -Wno-unknown-warning-option -Iinclude fixture.cpp original3.cpp
  original4.cpp -o test; execute: PASS 255566 assertions.
- g++ 14.2 -O1 -g -fsanitize=address,undefined
  -fno-omit-frame-pointer (remaining flags identical); execute:
  PASS 255566 assertions.
- clang++ 17 optimized with the same other flags: PASS 255566 assertions.
- clang++ 17 sanitized with the same other flags: PASS 255566 assertions.

Aggregate verified: **1,022,264 executed assertions** on byte-identical
production crypto implementations. The isolated local directory used
a minimal reproduction of the original 128-entry Gen IV shuffle table
as a dependency-only header. The exact full source-header SHA is
9eab685b76511d5ea08e0fffdff63529a5824fe6; only the full checkout
gate can verify it. Local source reconstruction is not a full checkout.

An early QA generator evaluated paired random draws in unspecified
argument order, resulting in compiler-dependent specimens. This QA-only
defect was corrected and all 4 builds rerun with deterministic ordered
draws and identical final counts. The original PK3 cpp includes GCC-only
diagnostic pragmas at lines 15-16. Clang strict -Werror rejected those
pragma options until the audit runner added
-Wno-unknown-warning-option. Coordinator owns any production fix.

## Explicit failed/blocked gates

No actual Pokemon3FRLG/Pokemon4ReadOnly class-linked 8,921-case fixture
ran locally; full Makefile.host and production graph are unavailable,
and GitHub DNS prevents cloning. Do NOT claim it passed.
No current whole Legality::analyze, full host/RSE, Switch NRO, original
retail authenticated saves, distributed-card receipts, live PKHeX runtime
differential or 336-cell original-source parity was executed/certified
here. Pending exact-head CI is an acceptance blocker.

Coordinator: run the L5 Stage 3 exact-head workflow on a real full
checkout, inspect logs, optionally fix compiler diagnostic-pragma
portability in production; leave synthetic encryption successes separate
from historical legality/authenticity. Do not merge accepted/MAIN or
advance Stage 4 until a new user continuation.
