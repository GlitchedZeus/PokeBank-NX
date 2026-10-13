# L6 Stage 2 Native Matrix Receipts

`tools/audit_l6_stage2_matrix.py` is the authoritative fail-closed release
matrix for a freshly combined checkout. It pins the latest fetched lane
receipts, validates source and runner registration before execution, captures
exact HEAD/tree identity, records compiler and elapsed command duration, and
runs GCC or Clang with normal plus ASan/UBSan lane modes after preflight.

The receipt matrix contains 114 declared programs after registering all
existing L6 Stage 2/4/5 fixtures and the two Stage 6 fixtures:

| lane | receipt | programs |
| --- | --- | ---: |
| L1 | `36f1f147dbea4a7607ec7440e83ce608147eaa19` | 31 |
| L2 | `6198ac5dfd867bf756de2458eeee3ae0645fa797` | 17 |
| L3 | `2de53eb330f09c13c112d5270d36110c26cd33ec` | 19 |
| L4 | `3c8c28d597cfc3bc982782ed9ec8ec708a3284ae` | 28 |
| L5 | `6f3e12eff17980d6e0b5857f8b470c2ed50a5033` | 8 |
| Codex audit | local branch | 11 |

The current coordinator integration head has only 13/11/7/15 L1–L4 programs.
Preflight consequently reports the 49 missing source/registration entries and
two L5 source-pin mismatches (`Pokemon3FRLG.h` and `Pokemon4ReadOnly.cpp`),
then returns nonzero rather than running a partial matrix or calling it green.

The same source drift was confirmed by the real L5 integration runner: its
first four Clang commands passed, including the ASan/UBSan native contract,
before its authoritative EV-vector gate stopped on those two stale wrapper
pins. The L5 receipt branch contains the expected blobs; no L5-owned source
or fixture is changed here.

Confirmed coordinator-owned Clang blocker: compiling the production analyzer
under Clang with `-Werror` diagnoses a dangling `std::u16string_view` at
`src/Legality/Legality.cpp:447`. `Gen3NegaiBoshiEvent::Candidate` outlives the
temporary `pk.otName()` used to initialize its view. The separately reviewable
recommendation is [l6_stage2_legality_ot_name_lifetime.md](../../patches/l6_stage2_legality_ot_name_lifetime.md).
It materializes `otName` once before constructing event candidates. The patch
is intentionally not applied by this Codex branch.
