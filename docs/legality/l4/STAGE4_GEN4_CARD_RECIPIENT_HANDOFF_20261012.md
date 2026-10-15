# L4 Stage 4 — Original Gen IV Mystery Gift card and recipient evidence

Date: 2026-10-12. PR #292 on `feature/legality-l4-events-meta-20261012`.
Status: **Stage 4 implementation and focused local verification delivered;
full repository acceptance BLOCKED**. Stages 1–3 complete-checkout gates
likewise remain blocked while Github Actions remains queued.

## Actual production/source implementation

`include/Legality/Gen4OriginalCardRecipientEvidence.h` provides a new
read-only, bounded, candidate-local **original PCD -> independently
identified archive -> independently identified recipient save ->
individual native PK4 -> receipt clock** join.

It deliberately requires separately attested:
- exact original card archive SHA-256 and original C::Attestation;
- exact card ID, type, raw compatibility bytes, independently proved
  PCD gift-body-to-card link (never inferred from PGT byte equality);
- original recipient save identity and nonzero independently verified
  source digest, original recipient trainer and receipt slot;
- individual receipt record-to-original-card binding and original
  source save-to-current native PK4 history;
- separately verified release-window/receipt field, clock accuracy,
  field preservation and no later rewrite;
- for Manaphy/Ranger, independently checked original recipient context
  **and** native Ranger PID/IV and anti-shiny history.

A PGT gift body alone has no PCD card ID, original card type provenance,
card compatibility history or individual recipient record. Matching
PCD/PGT bytes, including differences in the PGT slot byte, are **never**
evidence of identical original cards. Card-only metadata can differ
despite a matching 260-byte gift body. Original gift type must be pinned
from the independently attested original PCD source; it cannot be
promoted from a different currently parsed type.

Unsupported/unknown gift types, malformed PCD/PGT pairings, unverified
card identities, absent recipient history, mutable DS-clock dates and
Ranger gaps return Unknown or candidate-specific exclusions. No status
authenticates a release/receipt, declares global Invalid or supplies
copyrighted retail distribution bytes. Original-compatible raw
metadata is not decoded to a guessed region/version.

The L4-owned runner and L4 PR workflow now compile and execute the
new fixture **in addition to the unchanged set of 28 registered C++
suites**. The existing registered PCD/PGT suite independently exercises
the new card/receipt gate and withholding of Ranger evidence.

## Exact source identity / executed verification

Locally reconstructed source files were matched with the original
GitHub blob IDs using `git hash-object`. Original unchanged source
blobs:
- `Gen4MysteryGiftContainerEvidence.h`:
  `8ca2ddd5c83f3659645d5296075b7caa32bfee3d`;
- `Gen4EventClockReceiptEvidence.h`:
  `69506bbc1b75355991a591415c64312311949e00`;
- `test_legality_l4_clock_receipt_history.cpp`: same existing GitHub
  registered test;
- updated card/receipt header:
  `1fa0b067252d1a8077a05e8c50a16c067599a5e5`;
- exhaustive new fixture:
  `e29bd6a0ba1d1f338674805be71edc908c756ab2`;
- updated existing registered PCD/PGT suite:
  `cb272eeee3e7648cd8d5ea7f7c020ec0fc286c02`.

**Actually run:** GCC and Clang, each with normal `-O2`
and instrumented `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer`. Each compiler/mode compiled and
executed **three** exact-source C++ suites:
`tests/fixtures/legality_l4_stage4_card_recipient.cpp`,
`tests/test_legality_l4_gen4_pcd_pgt_container.cpp`,
`tests/test_legality_l4_clock_receipt_history.cpp`.
**12/12 executions PASS; no ASan or UBSan finding**.

Representative normal and sanitizer commands (same for `clang++`
and the other two exact suite paths):

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 \
  tests/fixtures/legality_l4_stage4_card_recipient.cpp -o /tmp/l4g4
/tmp/l4g4
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude \
  -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/fixtures/legality_l4_stage4_card_recipient.cpp -o /tmp/l4g4-san
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4g4-san
```

The new fixture checks 65,536 same-PGT-body/different-card identity
combinations (exactly one independently pinned card match), 256 source
proof masks, 512 recipient proof masks, 64 clock history combinations,
2,072 one-bit paired-body tamper cases, 256 gift type labels, and
Ranger/recipient mismatches. The existing PCD/PGT suite checks
62,160 gift payload mutation negatives, 448 archive configurations
and 32 converted-original proof combinations. The existing
clock suite checks 118,272 date permutations and 64 witness masks.

The synthetic positive is **not** an archived real-world gift, real
recipient record or actual Nintendo DS clock receipt. No source save
writes, emulator writes or legal classification of actual Pokémon.

## Explicit blockers, acceptance and next stage

The full repository cannot be checked out here:
`git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD`
reported `Could not resolve host: github.com`.
Thus no full 28-suite exact-head run or native encrypted PK3/PK4
and Ranger production-link verification has been executed here.
The L4 four-way workflow and full host checks are queued; verify actual
per-job results before integration. No complete-checkout success or
coordinator acceptance is asserted. New L4 evidence logic is
candidate-only and has not been merged into the global legality analyzer.

Coordinator PR #298, accepted legality, MAIN, other lanes and newer
commit ancestry were preserved; commits are forward-only. The
coordinator must review the independent archive/recipient/rights
gates, verify source identity and run the full acceptance matrix before
reconciliation. Stage 4 implementation is delivered but acceptance
**remains BLOCKED**. Next single major session is L4 Stage 5:
original-to-current metadata chains across Gen I–IV transfers.
