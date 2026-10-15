# Thread 4 — Gen I–V completion campaign checkpoint

Owner-authorised continuous QA for Gen I–V (no Gen VI–IX). Updated after every verified unit.
Resume rule: re-fetch, forward-merge the current #307 head (`claude/gen5-integration-20261014`)
with a merge commit, re-run `make -f Makefile.host qa` (GCC and Clang), then continue below.

## Current state

| Item | Value |
| --- | --- |
| Lane branch | `audit/app-qa-20261014` |
| Last #307 head merged | `2d8195d` (merge `f7837e9`; #307 now contains QA 4ae526f and L 95255c97) |
| Draft PR | #310 (CI evidence only, never merge; Thread 2R will merge this lane into #307) |
| Continuation | `send_later` self check-in every ~60 min: "Thread 4 campaign: re-fetch, verify checkpoint docs/qa/CAMPAIGN_CHECKPOINT.md, continue" |

## Phases

| Phase | Status | Notes |
| --- | --- | --- |
| Merge #307 `44a976e` | done (`47a4794`) | one trivial `Makefile.host` include-line conflict, both lines kept; `make qa` (GCC) PASS on the merged tree |
| Phase 5 — Gen I–V acceptance matrix (20 versions) | done (gate `qa-matrix`) | 420 cells: 415 PASS, 5 N/A, 0 FAIL; GCC = Clang; `docs/qa/GEN1_5_MATRIX.md` (hardware-only residue + cross-gen classification); found QA-F6 |
| Phase 6 — independent audit of #307 | in progress | `fuzz_gen5_edits` (edit sequences + export) added: found QA-F4; Home snapshot cross-gen review found QA-F5 | shared editor surface, Gen IV/V adapters, PID-coordinated edits, Home snapshot cache; fuzz every new parser/adapter |

## Open findings (owner)

| ID | Severity | Owner | Status |
| --- | --- | --- | --- |
| QA-F2 invalid one-time workflow YAML | Medium | 2R applied | FIXED (#307 96a44685); regression in qa-tools-test |
| QA-F3b Gen II export publish via `rename()` | Low | C | FIXED (cb88043); regression in qa-contracts |
| QA-F4 Gen V PID replay guard conjoins gender+ability links | Low | 2R | FIXED (bc83815); regression in test_qa_regressions |
| QA-F5 Gen V Home party strip drops form (Gen IV keeps it) | Low | 2R | FIXED on #307; regression in test_qa_regressions |
| QA-F6 Gen III release hint from a substring of the whole path | Low | C | open (repro `tests/test_qa_known_failures.cpp`) |
| QA-F7 Gen IV Unown letter cannot be edited (form count 1 vs PKSM-Core 28) | Low | C | open (repro `tests/test_qa_known_failures.cpp`) |
| QA-F8 Gen IV no-op clone+release still reviews as "(empty) (edited)" | Low | C | open (repro `tests/test_qa_known_failures.cpp`) |

## Log

- 2026-10-15 07:12 UTC — campaign resumed; merged #307 `44a976e`.
- 2026-10-15 — `fuzz_gen5_edits` (Gen V edit-sequence fuzzer) added; libFuzzer found QA-F4; 5-min soak clean on a retail-consistent seed.
- 2026-10-15 — QA-F5: Gen V Home party strip drops form; repro on the real assignment -> HomeSnapshotCache -> homeCardSnapshot path (GCC and Clang, ASan/UBSan). Cross-gen shiny/level/form notes recorded in FINDINGS.md.
- 2026-10-15 — merged #307 `85421cf` (Gen V Clone/Move keypads, Release dialog; UI-only, reviewed, no defect).
- 2026-10-15 — `fuzz_gen5_sav` extended to the Gen V save diagnostics (inspectSave5 / unopenableCopyDiagnostic5 / formatSaveReport5) and the Home snapshot from a ready probe; deterministic 60 s (GCC ASan/UBSan) and libFuzzer 180 s (Clang) clean.
- 2026-10-15 — merged #307 `6c366bd` (QA-F4 fix bc83815, Thread C merge cb88043). QA-F4/F5 moved to test_qa_regressions; QA-F3b regression and the production Gen I export transaction added to qa-contracts (18 paths); GCC and Clang green.
- 2026-10-15 — QA-F2 repair prepared (YAML re-indent only + workflow_dispatch gating, verified with bash -n / Python compile) but the push of the three shared one-time-* workflows was blocked by the session's permission policy; held as a scratch patch pending the user's decision. `tools/qa/check_workflows.py` committed (reproduces QA-F2 on the current tree).
- 2026-10-15 — Phase 5 matrix landed: 20 versions x 21 cells on real production paths, `qa-matrix` in `make qa` and App QA Contracts CI; libqacore object rule now tracks header dependencies (-MMD), after a stale object caused an ASan new-delete-type-mismatch; QA-F6 filed (C).
- 2026-10-15 08:25 UTC — merged #307 `181423a` (QA-F2 repaired by 2R in 96a44685): QA-F2 moved to qa-tools-test regressions; `app-qa-workflow-hygiene.yml` added; QA-F6 owner corrected to L (RetroArch discovery).
- 2026-10-15 — verified matrix observations: (b) filed QA-F7 (Unown, PKSM-Core PersonalDPPtHGSS::formCount = 28), (c) filed QA-F8 (no-op release review line; Gen IV matrix cell no longer pins the defect). make qa EXIT 0 on the 181423a merge.
- 2026-10-15 08:40 UTC — coordinator routed QA-F6 to C (owner corrected back from L).
- 2026-10-15 — Phase 6 (launch/return, Thread L 86c3490 via #307): `fuzz_launch_plan` added. Covers buildGameLaunchChainPlan vs the libnx argvSetup tokenizer and loader caps, hosted argv accepted unchanged by rh_parse_args, rh_* buffer bounds, the return host off unless explicitly enabled, and the return-selection parse/serialize round trip. Deterministic 140k execs (GCC ASan/UBSan) and libFuzzer 1.75M runs/180 s (Clang): no defect. Return host stays off by default: `GameLaunchReturnOptions{}` is false, and it is only enabled behind `experimental_return_host.enable` (GameLauncher.cpp; the file check runs on the device only).
- 2026-10-15 — Phase 6 save-safety chain: export contract C6 (Exit: Discard All restores, later export refused) on all 18 paths — all refuse after discard, no files left; `docs/qa/SAVE_SAFETY_CHAIN.md` maps Open→Validate→Working copy→Edit→Review→Export→Verify→Exit to the gates. make qa EXIT 0; Clang contracts green.
- 2026-10-15 — Phase 6 Home cache: `fuzz_home_cache` (refresh/invalidate/clear sequences against the documented contract) — 280k deterministic + 597k libFuzzer runs, no defect.
- 2026-10-15 — integrated #307 `2d8195d` (QA + L merged by 2R) forward-merged as `f7837e9`: make qa EXIT 0 under GCC AND Clang (fuzz smoke incl. launch-plan/home-cache, 18 paths C1–C6, matrix 420 cells 0 problems, regressions F1–F5); open repros F6/F7/F8 still reproduce (owner C).
