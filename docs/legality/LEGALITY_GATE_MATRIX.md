# Legality gate matrix (Gen I-V integration branch, PR #314)

Audited commit: `c71730cbf648f8b483a35d8df8005958d586d943` (head of
`claude/gen1-5-legality-campaign-w9mkom`). Toolchain for local runs: g++ 13.3,
clang++ 18.1, Python 3.13, .NET SDK 10.0.112. The L5 workflows that run on
`ubuntu-22.04` were also replayed with g++ 11.5 and clang++ 14.0 (the 22.04
compilers); all passed. Pinned inputs: PKHeX `6501f0ab`, pret/pokered `af519899`,
pret/pokeyellow `e89ead15`.

Class: **a** = already ran on PR #314 for this commit; **b** = applicable but did
not run there; **c** = not applicable to this branch (reason given).
"Now" = after the commits that add this file.

Workflows without a `pull_request` trigger run on push to `claude/gen1-5-legality-**`.
They check out `github.sha` and assert `HEAD == GITHUB_SHA`, so they test the exact
pushed head. Their checks appear on the PR because they run on the same head SHA.

## L1 encounters and RNG

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 1 | legality-l1-native.yml / l1-native | The runner registers exactly 58 suites (line 40) | b | Now: yes (push trigger) | PASS (58) |
| 2 | legality-l1-native.yml / l1-native (host, sanitize) | 58 L1 suites with g++, -O2 and ASan/UBSan | a | host-tests.yml `Legality lane L1` (line 145; the runner runs both profiles by default, runner line 92). Now also its own two jobs | PASS, exit 0, "all 58 native suites PASS for profiles host sanitize" |

## L2 breeding

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 3 | legality-l2-final-acceptance.yml | `test_legality_l2_run_all.sh` (24 programs) with g++, normal and ASan/UBSan | a | host-tests.yml `Legality lane L2` (line 148) | PASS |
| 4 | legality-l2-final-acceptance.yml | The same 24 programs with clang++ | b | Now: yes | PASS |
| 5 | legality-l2-final-acceptance.yml | 19 extra L2 fixtures and tests, g++/clang++ x normal/ASan (76 runs) | b | Now: yes | PASS, exit 0, "96 native + 76 extra = 172 ... executions" |
| 6 | legality-l2-native.yml (gcc, clang) | Fixture output counts (roles 12150, Gen III transcript 312000, DP witness 131072), snapshot guard, personal join. Its last step repeats rows 3-4 | b | Now: yes | PASS (20 runs) |
| 7 | legality-l2-gen1-native-dv-stats.yml (GCC, Clang) | 7 Gen I DV/Stat Exp tests and `test_gen1_battle_stats` with the Gen I link set, normal and ASan/UBSan | b | Now: yes | PASS (32 runs) |
| 8 | legality-l2-stage4-source.yml | Pt/HGSS daycare source and source-bound test, g++/clang, normal/ASan | b | Now: yes, inside row 5 (stage 7 script lines 39 and 46, same flags). Not enabled on its own, so it does not run twice | PASS (in row 5) |
| 9 | legality-l2-stage5-joint.yml | 5 same-route programs | b | Now: yes, inside row 5 (lines 40-41 and 47-49) | PASS (in row 5) |
| 10 | legality-l2-stage6-lineage.yml | 5 lineage programs | b | Now: yes, inside row 5 (lines 42-43 and 50-52) | PASS (in row 5) |
| 11 | legality-l2-gen56-pkhex-phase-a.yml | Gen V/VI egg fixtures (g++/clang, normal/ASan, 9590-row export) and a PKHeX `66ef5983` .NET differential | c (frozen scope) | Not enabled: it also asserts Gen VI behaviour, which is frozen until the owner authorizes Gen VI, and uses a second PKHeX pin. Its Gen V egg fixtures pass locally | PASS, exit 0, "matched 9590 rows" |

## L3 moves

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 12 | legality-l3-native-validation.yml | 29 L3 suites, g++ and clang++, normal and ASan/UBSan | a | Yes (pull_request) | CI success on c71730cb |
| 13 | host-tests.yml / Legality lane L3 | The same suites with the Emerald blob pin | a | Yes | CI success |

## L4 events

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 14 | legality-l4-evidence-matrix.yml (4 jobs) | 28 L4 suites, L4 fixtures and Python audits; g++/clang x normal/ASan | a | Yes | CI success. Local g++ normal and clang++ ASan PASS |
| 15 | L4 runner with `L4_L5_Q07_JSON` | Q07 247-row event provenance join | a | host-tests L4 lane (line 140, g++). Now also all four matrix jobs | PASS (247/247) |
| 16 | L4 runner with `L4_L5_Q08_JSON` | Q08 source-rights gate | b (skipped everywhere) | Now: all four matrix jobs | FAIL on c71730cb (exit 1, "pokediamond: legal basis/usage changed"). Stale expectation in the tool, now fixed. PASS after the fix |

## L5 independent QA

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 17 | host-tests.yml / Legality lane L5 | The long list at lines 159-216, including the Invalid-site gate (48) | a | Yes | CI success |
| 18 | l5-independent-native-stage2.yml | `audit_legality_l5_stage2.py` self-test and helper run | a | host-tests lines 185-186 | PASS |
| 19 | l5-independent-native-stage2.yml | `--native-analyzer` (l5-stage2-baseline) and the `l5-pk12-baseline` greps | c | Not enabled. These are pre-fix baselines that must fail once F3/F4/F5 are fixed. The after-fix targets run in host-tests lines 197-198 and 206-207 | Expected failures: exit 1 and exit 2 (baseline asserts). The after-fix targets PASS |
| 20 | l5-independent-native-stage3.yml | Entity fixture blob pin `a0135799` | b | Now: yes | PASS |
| 21 | l5-independent-native-stage3.yml | Crypto fuzz self-test and the GCC/Clang fuzz run | a | host-tests lines 190-191. Now also here | PASS |
| 22 | l5-independent-native-stage3.yml | PK3/PK4 entity quarantine (17310 cases), g++ | a | host-tests lines 193-194 (same fixture and sources, without the output grep). Now also here | PASS |
| 23 | l5-independent-native-stage3.yml | The same with clang++ | b | Now: yes | FAIL as written (exit 2): Clang -Werror rejects the GCC-only pragmas in Encryption3FRLG.cpp and the unused `unownFormValue` in Pokemon3FRLG.cpp. It never passed on the lane. PASS with the two Clang-only flags the other Clang gates already use |
| 24 | l5-independent-pkhex-stage4.yml | Oracle self-test, GCC/Clang normal and ASan differential, PKHeX and model pins | b | Now: yes | FAIL as written (exit 1, Legality.cpp 493f0a5a != pinned b1d02d66). Stale pin: the model is of the accepted analyzer d29eada5. PASS with that commit as the model root |
| 25 | l5-independent-crosslane-stage5.yml | Self-test, 900024-assertion ledger, L1-L4 headers from four pinned lane commits. New step: the same check with this checkout as all four roots | b | Now: yes | PASS (498829 assertions per build, both runs) |
| 26 | l5-independent-provenance-stage6.yml | Three blob pins, 247-row provenance report, receipt-chain fuzz g++/clang x 2, NDEBUG build must fail | b | Now: yes | PASS |
| 27 | l5-independent-final-stage7.yml | Pins, self-test, 336-cell dashboard of frozen coordinator ad9c1c85 (38 sites), verdict fuzz g++/clang x 2, NDEBUG build must fail | b | Now: yes. The dashboard is a frozen audit by design; the current head's Invalid sites are row 17 | PASS |
| 28 | l5-independent-gen1-rby-real-pkhex.yml | Gen I three-engine PKHeX differential | c | Not enabled. Superseded by row 30, which runs the same steps on the exact head. This copy pins the coordinator at ad9c1c85 (line 37) and two fixture blobs that this tree has since updated (lines 61 and 72) | Not run (its pins fail by design) |

## Gen I coordinator and PKHeX differentials

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 29 | legality-gen1-coordinator-exact-head.yml | Suite registry, Gen I smoke normal/ASan, g++/clang | a | Yes | CI success |
| 30 | legality-gen1-exact-pkhex-oracle.yml | Gen I PKHeX vs accepted vs exact-head differential, 151-species and saved-byte pipelines | a | Yes | CI success. Pins the current Legality.cpp blob 493f0a5a (line 78) |
| 31 | legality-gen1-original-machine-sources.yml | RBY TM/HM tables vs pret pokered/pokeyellow | b (its paths are not in the PR diff) | Now: yes (push trigger) | PASS, "Red/Blue 3037, Yellow 3043, 8 version differences" |
| 32 | legality-gb-pkhex-diff.yml | Gen I/II tables, pret check, 6 x 50,000 PKHeX verdicts | a | Yes, but on the PR merge ref. Now pinned to the exact head | CI success |
| 33 | legality-gen34-pkhex-diff.yml | Gen III/IV encounters vs PKHeX, 3 seeds | a | Yes | CI success |
| 34 | legality-gen5-pkhex-diff.yml | Gen V tables, tests, libFuzzer, PK5 vs PKHeX | a | Yes | CI success |

## L6 integration hardening

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 35 | legality-l6-stage5.yml | Stage 5 joint-history gate, g++/clang | a | Yes | CI success |
| 36 | legality-l6-audit.yml / stage4-native (4 jobs) | Stage 4 gate ledger, g++/clang x normal/ASan | b | Now: yes | PASS (6709 checks each) |
| 37 | legality-l6-audit.yml / stage4-python-failures | Failure-receipt and audit-runner regressions | b | Now: yes | PASS |
| 38 | legality-l6-native.yml (2 jobs) | Native PK1-PK4 format properties, 2297 checks x 2 runs | b | Now: yes | PASS |
| 39 | l6-codex-pk4-intake.yml (4 jobs) | Bounded PK4 intake, g++/clang x normal/ASan | b | Now: yes (paths filter dropped) | PASS (1028 cases each) |

## Other

| # | Workflow / job | What it checks | Class | Runs on PR #314 | Local result |
|---|---|---|---|---|---|
| 40 | host-tests.yml / Host tests (GCC) | Full host suite, RSE regression, ASan/UBSan | a | Yes | CI success (31 min) |
| 41 | legality-data-generation.yml | Regenerates six tables and commits them (contents: write) | c | Not enabled. It is a maintenance generator with no pass/fail comparison, and its push trigger commits to its own branch | Replayed in a scratch copy: all six tables match the committed ones byte for byte. Two one-shot migrations exit 1 (anchors gone), and one would add a second `ribbonNational()` to Pokemon.h |

Summary: 41 rows. 17 are **a**, 20 are **b** and 4 are **c** (one of them, row 11, only
because Gen VI is frozen). Every **b** row now runs on the exact head of this branch, either in its own
workflow or inside L2 final acceptance (rows 8-10).

## Pins that will move with the Gen III/IV analyzer work

- host-tests.yml line 179: `--expected-invalid 48` on `src/Legality/Legality.cpp`.
- legality-gen1-exact-pkhex-oracle.yml line 78: `Legality.cpp` blob `493f0a5a`.
- Row 24 no longer depends on the current `Legality.cpp`. It pins the d29eada5 copy.
