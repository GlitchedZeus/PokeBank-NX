# L4 Stage 2 — Gen I/II original gift and distribution evidence

Date: 2026-10-12. Target: PR #292 branch
`feature/legality-l4-events-meta-20261012` only. Stage 2 source/test
implementation complete; acceptance **BLOCKED** pending complete-checkout
CI for the latest L4 head and independent coordinator review. Earlier
Stage 1 28-suite acceptance is likewise not green.

## Production implementation and behavioral gates

- `Gen12GiftHistoryEvidence.h` now refuses to treat same-generation
  Red/Blue/Yellow or Gold/Silver/Crystal edition changes as witnessed
  automatically. Without `sameGenerationEditionTransferIndependentlyVerified`,
  the candidate route is Unknown and any candidate-only contradictions
  from that unverified route are suppressed. Cross-generation Gen I/II
  requires its separate independently witnessed Time Capsule party snapshot.
- New `Gen12OriginalDistributionJoinEvidence.h` combines the existing
  per-original gift history and original archive provenance gate. It
  supports regular gifts, static and NPC exchanges (offered-mon level),
  Stadium gifts in Gen I/II, and PCNY only in Gen II.
- Stadium/PCNY require independent source edition identification,
  matching original distribution family/platform, independently
  verified row-to-archive identity, and matching nonzero SHA-256 of the
  original archived payload. Valid structure or same species/TID never
  substitutes for a proven source and receipt.
- A source labeled `OriginalConvertedAuthenticated` is withheld as
  UnknownArchiveProof unless a separately verified conversion chain,
  digest of the converted file, and original archive digest are present.
  The original and converted evidence objects remain distinct.
- Original OT/nickname game-font bytes are comparable only where an
  independent charset link exists. Missing links yield
  UnknownFieldEncoding, even if reconstructed names look identical.
- The `evaluateAll` aggregator handles up to 128 independently pinned
  candidate histories and counts conditional, Unknown and excluded
  candidates. None is ever an event authenticity claim or globally
  Invalid; even all provided candidates excluded leaves whole-Pokémon
  legality unanswered. Redistribution permission is never granted here.
- No copyrighted original distribution record, Wondercard, ROM assets,
  third-party code, or fabricated archival receipt was added. All test
  archive digests, names, OT IDs and gift data are synthetic.

## Executed native GCC/Clang and sanitizer verification

The GitHub connector supplied exact source blobs; `git hash-object`
independently matched the locally compiled source files and the committed
GitHub blobs. Original upstream-sourced headers matched:

- `Gen12GiftHistoryEvidence.h` prior revision
  `f6072f31d755c65311e546346bc6c15d7ac45bea`.
- `Gen14EventDistributionSourceEvidence.h` retained unchanged
  `f6b044f8b8487156e9b427d0e1c06081a95184fe`.

Exactly tested final files:

- `Gen12GiftHistoryEvidence.h`:
  `2f16dbfa2f63da12957e97b336c21815a7a3109c`.
- `Gen12OriginalDistributionJoinEvidence.h`:
  `adb2eb6cc6c42831754c2255ce70b4a1a3146b8c`.
- `test_legality_l4_gen12_gift_history.cpp`:
  `30f1d1d5042b22b7814ef4983fb01091bcf37130`.
- `tests/fixtures/legality_l4_stage2_gen12_join.cpp`:
  `31a37a1baa9083f9a54f915a05927e98cc6df304`.

Actual host: g++ 14.2.0 and clang++ 17.0.0.
Both source-backed C++ test suites compiled and ran in each of four
configurations: GCC normal PASS, GCC ASan+UBSan PASS, Clang normal PASS,
Clang ASan+UBSan PASS: **8/8 final-source runs passed**.

Representative commands (repeated with both compilers, and each of
the two test filenames below):

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter -Iinclude -O2 \
    tests/fixtures/legality_l4_stage2_gen12_join.cpp -o /tmp/l4-stage2
/tmp/l4-stage2

g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
    -Wno-unused-parameter -Iinclude -O1 -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/fixtures/legality_l4_stage2_gen12_join.cpp -o /tmp/l4-stage2-asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4-stage2-asan
```

The new join test exhausts **786,432 source/edition/grade/attestation
combinations**, plus 16 converted-source proof masks, source-family mismatch,
wrong original digest, untrusted receipt, NPC offered level,
cross-version trades, a Time Capsule Gen II-only move, raw-byte
uncertainty, and two competing source candidates. The modified
original registered gift-history suite exhausts **64,009** Time Capsule
species/move combinations and all **36** Gen I/II edition pairs; its
expected 38,943 candidate-only Time Capsule contradictions remain local.

## Not run / blocking acceptance gates

A complete repository checkout is unavailable locally:
`git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD`
failed: `Could not resolve host: github.com`.
The two exact-source standalone suites are not a substitution for a
complete checkout or actual encrypted PK3/PK4 native and Ranger tests.
The L4-owned dedicated four-way workflow has been updated to execute
the Stage 2 suite alongside its existing **28** registered C++ programs,
using exact source head and GCC/Clang normal+ASan/UBSan.
Until every job completes green on the final source and the coordinator
reconciles the L4 candidate, Stage 2 and overall release acceptance
remain **BLOCKED**.

Source archiving, individual historical receipt, source licensing,
redistribution rights, and original-to-current provenance remain
Unknown without independent reviewed evidence. Do not merge PR #292,
do not promote findings to globally Invalid or globally Valid, and do
not modify PR #298, accepted legality, MAIN or other lanes. Next
engineering stage is Gen III original distributions (L4 Stage 3).
