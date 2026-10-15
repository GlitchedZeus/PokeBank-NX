# Thread 4 — Gen I–V completion campaign checkpoint

Owner-authorised continuous QA for Gen I–V (no Gen VI–IX). Updated after every verified unit.
Resume rule: re-fetch, forward-merge the current #307 head (`claude/gen5-integration-20261014`)
with a merge commit, re-run `make -f Makefile.host qa` (GCC and Clang), then continue below.

## Current state

| Item | Value |
| --- | --- |
| Lane branch | `audit/app-qa-20261014` |
| Last #307 head merged | `0c2fffb` (merge `dd5a2bc`; C up to afd96e3b, QA 2fa40e3) |
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
| QA-F6 Gen III release hint from a substring of the whole path | Low | C | FIXED (82032a9); regression in test_qa_regressions |
| QA-F7 Gen IV Unown letter cannot be edited | Low | C | FIXED (f17926f); regression in test_qa_regressions |
| QA-F8a Gen IV no-op clone+release review line | Low | C | FIXED (c7b0f97); regression in test_qa_regressions |
| QA-F8b Gen IV no-op clone+release still changes the slot bytes (export) | Low | C | FIXED (f883f4d0); regression in test_qa_regressions |
| QA-F9 NRO audit defaulted to the stale fa9cfcea artifact on pull_request (run 38732921392 INVALID) | High | 4 | FIXED; regression tests/test_qa_nro_audit_workflow.py |

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
- 2026-10-15 09:30 UTC — merged #307 `d57e5bc`: QA-F6/F7/F8a moved to regressions; QA-F8 split, export-bytes half QA-F8b stays known-failing (126 bytes). Matrix home.party.* facts now DERIVED from HomePartyPreview::fromGen1/2/3 (tests/test_qa_matrix_home.h) and the Gen IV/V snapshots, plus a new home.party.gender fact; all four are consistent across Gen I–V. The runner no longer counts "n/a" as an inconsistency. Gen I–III have no STALE Home state: a decided, known limitation.
- 2026-10-15 10:15 UTC — Gen IV review cell adopts C 2b2be00 (Bag-only edits in Review): accepts exactly the 4 slot lines or those + exactly "Bag Medicines: item #17 5 -> 9"; verified in a scratch worktree with 2b2be00's Gen4ExportTransaction applied (strict 5-line form passes, 0 problems). Tighten to strict once #307 carries 2b2be00.
- 2026-10-15 11:35 UTC — merged #307 `0c2fffb`: QA-F8b fixed (f883f4d0) and moved to test_qa_regressions; qa-known-failures has no open C++ defect. Gen IV review cell strict (C 7dec0c78). ALL QA findings F1–F8 closed.
- 2026-10-15 — QA-F9 (owner-reported, High): app-qa-nro-audit.yml rewritten with a single identity source (dispatch inputs or docs/qa/CANDIDATE_UNDER_AUDIT.env = a0633b6d), no defaults, GitHub API run/artifact binding, staleness and tree guards, BUILD_IDENTITY/SHA256SUMS cross-check, no paths filter. Run 38732921392 is INVALID evidence. Forward-merged #307 a0633b6 first (tree now identical to a0633b6 apart from QA files).
