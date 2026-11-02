# Actual test ledger

Production source reviewed/tested: `49240a36dd5dad2608b2571145652a39c0aeefd5`.
QA changes were present during later runs; `src/` and `include/` stayed identical
to that commit. Reference PKHeX source was exactly
`6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

Environment: GCC 16.2.1 (20260810), Clang 23.1.1, .NET SDK 10.0.402.
All sanitizer results below are ASan/UBSan; managed host tests disable leak
inspection because the process sandbox prevents it. The L2 runner deliberately
requires LeakSanitizer, so it was separately rerun without that sandbox.

## Production and independent QA executions

| Command / executable | Actual outcome | Scope |
|---|---|---|
| `make -f Makefile.host -j4 host-test` | FAIL at `test_gen4_readonly_foundation:274`, `view.ball() == 17` | All prerequisites built. The loop stops at the first failure. |
| Remaining normal host binaries, individually after that stop | Final accounting **190 PASS / 1 FAIL / 191** | `host-ledger.json`; earlier binaries passed because the original set-e loop reached the failing one. This is not a passing make target. |
| `make -f Makefile.host -j4 host-sanitize` | Same assertion FAIL | All prerequisites built; no memory sanitizer diagnostic caused this failure. |
| Remaining sanitized binaries, individually after that stop | Final accounting **190 PASS / 1 FAIL / 191** | `sanitize-ledger.json`, ASan/UBSan with leak detection disabled as in the host Makefile. |
| `build-host/test_gb_legality_gen1` and `_sanitize` | **198 checks PASS each**, including 6 intentional Incomplete checks | Existing real PK1 wrapper/application analyzer fixture. |
| `build-host/test_gb_legality_gen2` and `_sanitize` | **244 checks PASS each** | Existing standalone PK2 origin fixture. |
| Clang `test_gb_legality_gen2` and `_sanitize` | **244 checks PASS each** | Built separately using `CXX=clang++ HOST_BUILD=/tmp/gen12-clang`. |
| Strict Clang `test_gb_legality_gen1` and `_sanitize` | **BUILD FAIL; executables not run** | Named Negai dangling view; also unused `unownFormValue` and GCC-only warning pragmas in Gen III dependencies. No warnings were suppressed to report green. |
| `python3 -m unittest discover -s tools/pkhex_gb_diff/audit -p test_protocol.py -v` | **4 test methods PASS** | Includes 24 malformed/accounting rejection scenarios plus valid/exception controls. |
| `audit/test_native_protocol.py NX_EXECUTABLE` | Control + **12 malformed inputs PASS** | Repaired actual C++ executable; additionally run with Clang ASan/UBSan. |
| `audit/test_native_protocol.py dotnet PKHeXGBOracle.dll` | Control + **12 malformed inputs PASS** | Repaired actual C# executable linked to pinned PKHeX.Core. |
| `audit/adversarial_corpus.py --manifest ...` followed by both real runtimes | **28 deterministic native records executed** | Two legal controls; 26 independent contradictory records. |
| `audit/check_structural.py MANIFEST PKHEX NX --json-out ...` | **FAIL: NX 24 disagreements; PKHeX 8** | No differential exemptions. Expected audit findings remain acceptance failures. |
| Strict `compare.py` on those 28 records | **FAIL: 17 unexplained differences** | 9 matching legal (7 independently contradictory), 1 matching invalid, 1 existing deliberate level-100 difference. |
| `audit/run.sh PKHEX_CHECKOUT OUTPUT_DIR` | **exit 1**, after passing protocol tests and running both engines; same 24/8 structural failures | End-to-end new CI acceptance runner. |
| `make -f Makefile.host -f tests/audit/Makefile.gen12 ...audit_gen1_analyzer_guards...` then both normal/sanitized executables | **30 checks, 27 PASS / 3 FAIL each** | Two above-cap wrapper contexts plus the above-cap real parsed synthetic Red save fail expected Invalid. Coherent cap control passes; source bytes stay immutable. |
| `audit_gen12_verdict_integrity` and `_san` | **2,048 checks PASS each** | Every byte value across species, level, leading move, catch rate/held-item boundary axes; status/problem consistency and input immutability. No ASan/UBSan diagnostic. |
| Clang ASan `tests/audit/gen3_negai_lifetime_repro.cpp` | **exit 1, stack-use-after-scope** | Actual production candidate/matcher with the same temporary lifetime as the application. Intentionally preserved failing reproducer. |
| Original pre-repair GB harness, `1 truncated 0 01 50 50` | **exit 1, heap-buffer-overflow** | Malformed corpus parser defect, fixed on audit branch; native save parser is a separate path. |
| `git diff --check` | **PASS** | Repeated after final edits. |

## Differential and original-source verification

The new GB differential ran generators for generations 1 and 2, seeds 1, 2 and 3,
50,000 records each. The C++ harness and Release .NET oracle were built and
actually executed against every record. The SDK needed a writable
`XDG_RUNTIME_DIR`; the initial first-use environment failure was repaired before
any differential result was counted. The complete raw outputs were retained
locally and hashed in `differential-summary.json`.

Commands used, with the oracle/compiled harness reused across the six runs:

```sh
python3 -I tools/pkhex_gb_diff/gen_corpus.py --pkhex PKHEX --pokered POKERED --count 50000 --seed SEED
python3 -I tools/pkhex_gb_diff/gen_corpus2.py --pkhex PKHEX --pokered POKERED --count 50000 --seed SEED
dotnet PKHeXGBOracle.dll < corpus.txt > pkhex.txt
gb_diff < corpus.txt > nx.txt
python3 tools/pkhex_gb_diff/compare.py corpus.txt pkhex.txt nx.txt --show 2
```

All six runs exit successfully with the exact reported 300,000-case totals;
strict comparison preserves every count. This is not complete parity or proof
of historical legitimacy.

The original, separate 72-vector PKHeX differential was also actually executed:
original `tools/audit_legality_l5_gen1_pkhex_oracle/Program.cs` (unchanged source,
relocated project reference only), real pinned PKHeX.Core, original accepted NX
`d29eada5c087e201e00c749703f1a2de3ddfadeb`, and audited NX.
`audit_legality_l5_gen1_rby_live_diff.py` exits 0; corrected native ability/gender
observations have zero disagreements, versus 36/18 false Invalid observations
in the accepted baseline. Its 72 vectors verify those native fields/source
contexts, **not whole-Pokémon global legality**. Results are in
`original-differential.json`.

Other source/oracle checks actually run:

- `verify_gb_legality_data.py --pkhex PKHEX --pokered POKERED --pokeyellow POKEYELLOW`:
  **2,673 Gen I comparisons, 2 explained overrides, 0 mismatches**. Initial sparse
  references lacked wild files; complete pinned temporary worktrees were fetched
  before counting the passing verification.
- `gen_gb_legality_data.py --pkhex PKHEX --out-cpp ABSOLUTE_WORKSPACE_OUTPUT`:
  regeneration succeeds; byte comparison with production CPP succeeds. Reports
  8,976 wild rows, 35+58 statics, 26+11 trades and 11+157 event gifts. Relative and
  outside-workspace output attempts first exposed a logging-path bug after writing
  the CPP; those failed invocations were not counted as passes.
- `verify_gen1_oracle_source_pins.py --self-test` and normal run: **PASS**, 12 local
  source pins match; the tool separately reports 3 externally enforced pins.
- `verify_gen1_suite_registry.py`: **PASS**, focused=22, L3=29, 45 distinct sources.
- Original differential integrity self-test: **PASS**, explicitly not a PKHeX
  runtime execution; original protocol negative suite: **10 tests PASS**.

## L1–L5 execution and inherited failures

- `bash tests/test_legality_l1_run.sh`: **45 suites x 2 profiles = 90 PASS**.
- `bash tests/test_legality_l2_run_all.sh`: initial managed-sandbox run **22/48
  PASS** because its mandatory LeakSanitizer cannot inspect processes, plus real
  assertion failures. Unrestricted rerun, retaining `detect_leaks=1`: **44/48
  PASS**, four failures at two assertions, in both normal and sanitized modes:
  `test_legality_l2_gen3_pid_iv_moves_alternatives.cpp:140` and
  `test_legality_l2_gen4_native_iv_history.cpp:108`. Both normal assertion failures
  reproduce on the exact parent; no test expectation was changed to make them pass.
- `bash tests/test_legality_l3_run_all.sh`: first stops at a pre-existing incompatible
  multi-variable auto declaration in `test_legality_l3_platinum_native_sources.cpp:27`.
  The same compile failure reproduces on the parent. Splitting the declaration
  preserves both expressions and repairs that QA compilation. The rerun then
  stops at the pre-existing static assertion in `test_legality_l3_move_chronology.cpp:250`.
  That assertion also fails on the parent. Remaining suites were separately compiled
  and executed with the original flags and assertions active: **56 PASS / 2 compile
  FAIL / 58 profiles**; all 29 suite names accounted for in `l3-ledger.json`.
- `bash tests/test_legality_l4_runner.sh` and `L4_SANITIZE=1 ...`:
  **28 C++ programs PASS per profile**, plus **8 Python tests PASS per runner**.
- L5 source/model/codec checks were executed through the original tools:
  production wiring, stage1 integrity/manifests, invalid-site scanner, stage2
  native source pins, stage4 cross-lane checks, stage2 Time Capsule self-tests and
  native runner, stage3 original-codec fuzz self-tests/native runner, native EV
  vectors and native codec normal/sanitized. All command exits are recorded in
  `l5.txt`. The wiring contract initially fails on stale 38-versus-44 text; that
  exact failure reproduces on the parent. The QA-only correction passes, while
  the invalid-site scanner still requires 44 and the native guards.
  The stage1 integrity command returns success while explicitly reporting
  **release BLOCKED / 0 authenticated historical pairs**; this is not acceptance.
- Additional full-production L5 make targets actually run normal and sanitized:
  `l5-stage3-native-entities` (**8,921 checks each**), `l5-ev-fixed` (**52 each**),
  `l5-pk12-fixed` (**200 synthetic contexts each**), `l5-pk2-egg-fixed`
  (**1,000 contexts / 400 reference egg-invalid candidates each**) and
  `l5-platinum-native` (**19 contexts each**). All these focused targets pass.

Exact parent reproduction used `c2544ac6d278468f6d30826b9a0115b098e5913b` with
PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`:

```sh
make -f Makefile.host build-host/test_gen4_readonly_foundation
build-host/test_gen4_readonly_foundation # fails the same ball assertion
L2_TEST_FILTER=gen1_native_exp_growth bash tests/test_legality_l2_run_all.sh # same sandbox leak limitation
L2_TEST_FILTER=gen3_pid_iv_moves_alternatives L2_MODES=normal bash tests/test_legality_l2_run_all.sh
L2_TEST_FILTER=gen4_native_iv_history L2_MODES=normal bash tests/test_legality_l2_run_all.sh
g++ ... -fsyntax-only tests/test_legality_l3_move_chronology.cpp
g++ ... tests/test_legality_l3_platinum_native_sources.cpp -o OUTPUT
clang++ ... -fsyntax-only src/Legality/Legality.cpp
python3 tools/audit_legality_prod_wiring_contract.py --self-test --source-root .
```

The ball failure, both L2 assertions, both L3 compile failures, Negai strict-Clang
failure and stale wiring failure were therefore demonstrated on the exact parent;
they were not merely labeled inherited from memory. Gen IV ball behavior remains
outside this audit's production changes and belongs to PR #305.

## Environment limits and publication

No Switch native build was executed: `/opt/devkitpro` and the cross compilers
are absent. No `.nro` was produced and no physical Switch or cartridge test was
performed. No installed game or emulator data was written. Synthetic buffers and
temporary test files are distinct from original source saves.

CI on the audit branch is intentionally expected to fail until F2/F3/F7 are
repaired: the new application and structural acceptance gates expose production
failures. No claim of green remote CI, complete original-game parity, authenticated
retail history, or integration approval is made.
