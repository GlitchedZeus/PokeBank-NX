# Thread 4 — Gen I–V completion campaign checkpoint

Owner-authorised continuous QA for Gen I–V (no Gen VI–IX). Updated after every verified unit.
Resume rule: re-fetch, forward-merge the current #307 head (`claude/gen5-integration-20261014`)
with a merge commit, re-run `make -f Makefile.host qa` (GCC and Clang), then continue below.

## Current state

| Item | Value |
| --- | --- |
| Lane branch | `audit/app-qa-20261014` |
| Last #307 head merged | `85421cfa1f847169ae78a801bfb06bd6bf536125` (merge `8fa5061`) |
| Draft PR | #310 (CI evidence only, never merge; Thread 2R will merge this lane into #307) |
| Continuation | `send_later` self check-in every ~60 min: "Thread 4 campaign: re-fetch, verify checkpoint docs/qa/CAMPAIGN_CHECKPOINT.md, continue" |

## Phases

| Phase | Status | Notes |
| --- | --- | --- |
| Merge #307 `44a976e` | done (`47a4794`) | one trivial `Makefile.host` include-line conflict, both lines kept; `make qa` (GCC) PASS on the merged tree |
| Phase 5 — Gen I–V acceptance matrix (20 versions) | not started | `tests/test_qa_matrix_*`, `docs/qa/GEN1_5_MATRIX.md` |
| Phase 6 — independent audit of #307 | in progress | `fuzz_gen5_edits` (edit sequences + export) added: found QA-F4; Home snapshot cross-gen review found QA-F5 | shared editor surface, Gen IV/V adapters, PID-coordinated edits, Home snapshot cache; fuzz every new parser/adapter |

## Open findings (owner)

| ID | Severity | Owner | Status |
| --- | --- | --- | --- |
| QA-F2 invalid one-time workflow YAML | Medium | owner approval | open (repro in `qa-known-failures`) |
| QA-F3b Gen II export publish via `rename()` | Low | C (Gen I–IV integration) | open (by inspection) |
| QA-F4 Gen V PID replay guard conjoins gender+ability links | Low | 2R | open (repro `tests/test_qa_known_failures.cpp`) |
| QA-F5 Gen V Home party strip drops form (Gen IV keeps it) | Low | 2R | open (repro `tests/test_qa_known_failures.cpp`, real Home path) |

## Log

- 2026-10-15 07:12 UTC — campaign resumed; merged #307 `44a976e`.
- 2026-10-15 — `fuzz_gen5_edits` (Gen V edit-sequence fuzzer) added; libFuzzer found QA-F4; 5-min soak clean on a retail-consistent seed.
- 2026-10-15 — QA-F5: Gen V Home party strip drops form; repro on the real assignment -> HomeSnapshotCache -> homeCardSnapshot path (GCC and Clang, ASan/UBSan). Cross-gen shiny/level/form notes recorded in FINDINGS.md.
- 2026-10-15 — merged #307 `85421cf` (Gen V Clone/Move keypads, Release dialog; UI-only, reviewed, no defect).
