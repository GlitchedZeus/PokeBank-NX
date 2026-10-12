# L6 Stage 2 Native Matrix Receipts

`tools/audit_l6_stage2_matrix.py` is the authoritative fail-closed release
matrix for a freshly combined checkout. It pins the latest fetched lane
receipts, validates source and runner registration before execution, captures
exact HEAD/tree identity, records compiler and elapsed command duration, and
runs GCC or Clang with normal plus ASan/UBSan lane modes after preflight.

The receipt matrix contains 103 declared programs:

| lane | receipt | programs |
| --- | --- | ---: |
| L1 | `075a597146ab2924fb86d3efaa0311eb065fba3c` | 30 |
| L2 | `7e3c0a2a558d489b429795fc22a35cd2dd95fafb` | 17 |
| L3 | `e9b603c07dc20b75451f84a268bce8fab59e56c5` | 17 |
| L4 | `5f7e5b44bedc555c67171e400fc86b347fc6810a` | 28 |
| L5 | `8cb0bec1eb0dbd62ffdb0f40e4f582a2aeed7042` | 8 |
| Codex audit | local branch | 3 |

The current coordinator integration head has only 13/11/7/15 L1–L4 programs.
Preflight consequently reports the 46 missing source/registration entries and
returns nonzero rather than running a partial matrix or calling it green.

Confirmed coordinator-owned Clang blocker: compiling the production analyzer
under Clang with `-Werror` diagnoses a dangling `std::u16string_view` at
`src/Legality/Legality.cpp:447`. `Gen3NegaiBoshiEvent::Candidate` outlives the
temporary `pk.otName()` used to initialize its view. The separately reviewable
recommendation is [l6_stage2_legality_ot_name_lifetime.md](../../patches/l6_stage2_legality_ot_name_lifetime.md).
It materializes `otName` once before constructing event candidates. The patch
is intentionally not applied by this Codex branch.
