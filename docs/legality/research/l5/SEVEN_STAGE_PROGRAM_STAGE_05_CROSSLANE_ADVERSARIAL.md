# L5 Seven-Stage Engineering Program — Stage 5 Cross-Lane Adversarial QA

Date: 2026-10-12. PR #294 is the existing L5 research/QA draft.
No production Legality.cpp, coordinator CI, accepted legality, MAIN or
sibling lane branches were modified. No original source save was written.

## Re-fetched independently maintained lane snapshots

L1 encounter HGSS original tree:
f7138ad95ed1e0cccab283d51e5ba87502bb3f79
Original header Git blob: 9c767ede360d503baa5831db3ce9c6eff04c9441

L2 Gen IV original egg inheritance and historical candidate set:
110372b2a877c41881eb5466392684d378b4a90d
Exact Git blobs: 47d5a3d5f04c5bb9d2246ec6e2ec694c0fb409cc,
11e72b655d6963b5f31f07017122e28a5a7a4996

L3 Gen III same-original-game move acquisition:
2bf58df8033e045e0d4483a3900617ff61b350ea
Exact Git blob: c3368b800d9ae34125ebc65278d829c26219bf66

L4 Gen IV mutable event clock receipt:
28fa55a9e56b9de7a40137b73199dd9051c53552
Exact Git blob: 69506bbc1b75355991a591415c64312311949e00

Coordinator #298 at inspection:
ad9c1c859f94c0321a5d17489c72b0e7dbb9f440
Accepted legality:
d29eada5c087e201e00c749703f1a2de3ddfadeb

## Substantial implemented code

tools/audit_legality_l5_stage5_candidate_ledger.h
Git blob 580a3690554bf55761ccca5d4548da75c9980b34

A bounded 16-packet audit-only candidate-set composer. It refuses
to splice a Gen IV encounter, breeding, move, or event candidate
from different Pokémon or historical routes. Conflicting original
source packets quarantine the individual route, while unattested
alternatives, missing stages, wrong source ID, duplicate conflicts,
unclosed candidate enumeration and search capacity prevent even a
candidate-set contradiction. It NEVER emits global Invalid.

tests/fixtures/legality_l5_stage5_joint_ledger.cpp
Git blob e12aca2d911e86036a62a1f54e9e00ba48448c20

100,000 deterministic packet metamorphic cases plus directed negative
controls: permutation invariance, duplicates, incomplete evidence,
source mismatch, unverified receipts and additional alternative
routes. The suite demands 900,024 actual assertions per executable.

tests/fixtures/legality_l5_stage5_live_four_lane.cpp
Git blob c10fcee44dbe78e6d6f81fd054c3b2344c459eec

Independent integration fixture including the REAL four candidate
headers. Dense HGSS tree/TID/group matrix, 20,000 Gen IV egg
final-state challenges, 20,000 Gen III move-sequence challenges,
Gen IV clock/date controls and a joint witness consistency probe.

tools/audit_legality_l5_stage5_crosslane.py

Pins exact Git blobs for each of the four original lane sources,
copies into a temporary immutable overlay, compiles and executes
GCC and Clang in optimized and ASan/UBSan configurations, and
rejects falsified result attestations. Source-only ledger success
cannot claim the actual four-lane helper integration passed.

.github/workflows/l5-independent-crosslane-stage5.yml

L5-owned exact-head Actions workflow. Checks out L1-L4 only at
the fixed SHAs above, requires actual production-header execution
in four modes, and rejects mutations of any tracked sibling files.
Shared coordinator workflows and source remain unchanged.

## Tests ACTUALLY executed (scope: L5 audit-owned ledger)

| Compiler / execution | Assertions | Result |
|---|---:|---|
| GCC 14.2 C++20 optimized O2, Werror | 900,024 | PASS |
| GCC 14.2 O1 AddressSanitizer + UndefinedBehaviorSanitizer | 900,024 | PASS |
| Clang 17 C++20 optimized O2, Werror | 900,024 | PASS |
| Clang 17 O1 AddressSanitizer + UndefinedBehaviorSanitizer | 900,024 | PASS |
| GCC NDEBUG build negative control | n/a | Correctly rejected |

Total 3,600,096 actual ledger assertions. The audited source and
fixture Git hashes match the pushed files. Sanitizer settings:
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1,
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1.

## UNEXECUTED and blocked acceptance gates

The original L1-L4-header integration fixture was COMMITTED but NOT
executed in the local environment. The original full GitHub checkout
and local GitHub hostname resolution are unavailable. The exact
head Stage 5 GitHub Actions job was QUEUED at inspection, not PASS.

No complete production Legality::analyze, integrated host/RSE suite,
original retail save, original distribution event receipt, live PKHeX
verifier or exact 336-cell game/family parity was validated here.
The release acceptance gate remains BLOCKED.

## Prioritized coordinator checks

1. HGSS Headbutt tree contradiction is only local to one original,
   attested tree; unidentified TID32/tree or alternative game is Unknown.
2. Gen IV parent IV/held items and seed cannot be deduced from a child.
   Conflicting duplicate original-route packets contaminate enumeration.
3. A mismatched Gen III action-history candidate cannot exclude a
   second compatible original TM, HM, tutor or relearner history.
4. An event date is a mutable DS-clock field, not authenticated
   original distribution timing or egg/hatch provenance.
5. Never merge separately verified L1/L2/L3/L4 routes into a
   synthetic all-origins contradiction. A bounded candidate space
   alone cannot establish global Invalid.

Coordinator should execute Stage 5 original-header tests normal
and ASan/UBSan at their exact pinned heads, investigate any
compiler/test failure, and keep parity/original-history flags
uncertified until authentic paired evidence is available.

Next L5 user continuation: Stage 6, original history, provenance,
chain-of-custody and release-evidence pipeline. Do not start it here.
