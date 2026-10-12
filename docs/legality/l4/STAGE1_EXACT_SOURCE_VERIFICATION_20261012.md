# L4 Stage 1 — exact-source event validation, 2026-10-12

## Scope and trust boundary

L4 PR #292 only. **Do not accept this stage or promote the branch while the exact-head
28/28 normal and sanitizer matrix is queued or red.** These checks are informational
and candidate-local. No event template, synthetically asserted witness, or matching
card/container establishes retail authenticity, individual receipt, or global Valid.
Candidate mismatches never imply whole-Pokémon Invalid. Source saves untouched.

The L4 source and test runner register **28 distinct C++ programs, no duplicate/missing
registrations** at the audited head. The dedicated four-way workflow covers:
GCC optimized; GCC ASan/UBSan; Clang optimized; Clang ASan/UBSan.
It uses an exact pull-request application SHA and source checkout.

## Native tests actually executed locally

A full clone was **not** available (git remote DNS: Could not resolve host
github.com). Two self-contained C++ test suites and their production headers were
retrieved through the GitHub connector and independently matched to Git blob SHA-1
using `git hash-object`. There were **no altered source stubs** for these two suites.
They were compiled/run with both available compilers (GCC and Clang) under:
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2` and
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`.
Execution used `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

1. `test_legality_l4_clock_receipt_history.cpp` and
   `Gen4EventClockReceiptEvidence.h`: **4/4 normal/sanitizer combinations PASS**.
   118,272 calendar byte combinations, 93,502 valid dates, 64 history/clock
   witness masks, and the new 256-value original receipt field domain.
   Committed source blob `69506bbc1b75355991a591415c64312311949e00`;
   test blob `be9316a7d5860167738e8ea32e8bf5bbafd6afd6`.
2. `test_legality_l4_gen4_pcd_pgt_container.cpp` and
   `Gen4MysteryGiftContainerEvidence.h`: **4/4 normal/sanitizer combinations PASS**.
   15 distribution/gift types, 62,160 negative payload bit-flip checks in both
   directions, four card-only metadata offsets per type, 448 source claim masks,
   and all 256 archive-grade tags. Committed source blob
   `0721a12dcf124a05b352b1b788cf85b21d87dd89`;
   test blob `e6ce638faedc7af2af77b1a07c37dd1a37e620c5`.

These **eight actual executions** prove only two of the 28 complete-checkout C++
suites. They are **not** a complete checkout, nor a green integration matrix.
They use synthetic data and establish no authentic event provenance.

## Reproduced and corrected defects

- An out-of-domain receipt field tag, e.g. `Field(255)`, was silently interpreted
  as `Met`; a normally admissible source could thereby yield an unwarranted
  `CandidateOutsideWindow`. The exact-original-source assertion terminated with
  exit 134. Now only explicit `Egg` and `Met` tags are eligible; the rest remain
  `UnknownReceiptField`.
- An out-of-domain archive grade, e.g. `ArchiveGrade(255)`, could pass as
  `ConditionalOriginalSource` when other independent flags were injected true.
  The original exact-header repro terminated with exit 134. Now only OriginalPCD,
  OriginalPGT or ConvertedOriginal enter the conditional source gate; unsupported
  tags return UnknownSource, while Reconstructed/Altered/Unreleased stay UntrustedSource.
- Expanded symmetrical PCD/PGT bit mutation, card-ID reuse and compatibility-field
  tests ensure a matching gift body does **not** identify its original archived
  card, release, or receipt.

## Hard blocking acceptance requirements

1. The latest PR #292 head must complete **28/28** real C++ test suites using an
   exact complete repository checkout under **all four** GCC/Clang normal and
   ASan/UBSan configurations. Check
   `L4 Event Metadata Exact-Head Verification` on PR #292.
2. Full native encrypted PK3/PK4, Ranger, PCD/PGT, historical Gen I/II gift,
   clock, trainer, Pal Park and candidate-join tests must each execute.
3. Full Python projection, L4/L5 Q07 join, original-source/rights audits and
   ordinary host/RSE/sanitizer gates require independently recorded outcomes.
4. The coordinator must re-sync the final reviewed L4 head into PR #298 and
   independently rerun exact-head acceptance. No lane-owned merge or promotion.
5. Source authenticity, individual receipts and original archive rights remain
   Unknown until independently attested and approved; passing tests cannot
   remove this gate.

Status: **STAGE 1 IN PROGRESS / NOT GREEN.** Next L4 session must inspect the
exact-head four-job matrix, address real failures forward-only and finish the
remaining 26 full-checkout C++ suite executions before advancing to Stage 2.
