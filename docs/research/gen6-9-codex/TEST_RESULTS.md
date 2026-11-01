# Executed verification and limitations

2026-10-18. Compilers actually used: `gcc (GCC) 16.2.1 20260810`, `clang version 23.1.1`. No native devkitA64 build or physical device test was performed. Evidence categories follow SOURCE_LEDGER.

## Unchanged existing native lanes: NX-TEST

Checkout `2f065ee2b0768d01bf621f969aef01120c6231e3` (#312), recursive PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`, memecrypto `bd3b16ee2c12bd9ee353cf3098cb998e7880638c`. Git diff confirms its Gen6/Gen7 include/src/test/Makefile files are unchanged from #309/#311 respectively. This is a stacked audit checkout, not the current app branch.

Executed from that checkout:

```sh
make -f Makefile.host gen6-host-test gen7-host-test gen8-host-test CXX=g++ HOST_BUILD=build-research/gcc
make -f Makefile.host gen6-host-test gen7-host-test gen8-host-test CXX=clang++ HOST_BUILD=build-research/clang
make -f Makefile.host gen6-host-sanitize gen7-host-sanitize gen8-host-sanitize CXX=g++ HOST_BUILD=build-research/gcc-san ASAN_OPTIONS=detect_leaks=1
make -f Makefile.host gen6-host-sanitize gen7-host-sanitize gen8-host-sanitize CXX=clang++ HOST_BUILD=build-research/clang-san ASAN_OPTIONS=detect_leaks=1
```

All four commands exited0: **13 binaries, 66,078 assertions per command**. Gen6 counts: 591/567/166/232/31270/87/31; Gen7: 350/574/30932/17/102; Gen8:1159. Full build/output logs and machine summary are under `evidence/`. No existing assertion/gate was changed. Existing vendor shift-base exemption and scoped RSA leak suppressions remain; sanitizer success does not certify those defects fixed.

Two setup failures were corrected transparently: absolute HOST_BUILD paths broke the make recipe's `./` prefix; relative separate build dirs solved it. Initial sandbox LSan runs failed because ptrace was unavailable; identical commands passed outside the sandbox **with detect_leaks=1 retained**. No permission review blocked execution.

`tools/run_existing_tests.py <isolated-lane> <logs-dir>` reproduces four gates and refuses a different source SHA. It creates build outputs only. Running all current Gen I–V application/legality suites locally was not performed; fetched CI evidence is reported separately below.

## New research tests: INDEPENDENT

From the research checkout:

```sh
python3 -m unittest discover -s docs/research/gen6-9-codex/tests -v
python3 docs/research/gen6-9-codex/tools/generate_metadata.py /tmp/gen69-PKHeX /tmp/gen69-regenerated.json
cmp docs/research/gen6-9-codex/metadata/pkhex-index.json /tmp/gen69-regenerated.json
python3 docs/research/gen6-9-codex/tools/run_signature_probe.py vendor/PKSM-Core/memecrypto
python3 docs/research/gen6-9-codex/tools/run_signature_probe.py vendor/PKSM-Core/memecrypto --cc clang
```

Four unittest methods pass: all four BDSP size/revision/hash structural envelopes and tamper/truncation/wrong-revision/source immutability, randomized record geometry, pinned table/overlap bounds, transfer-policy guard checks. Metadata regeneration is byte-identical. These are synthetic **structural** fixtures, not playable/retail positives. Signature probe passes both compilers: two sentinels unchanged after reverseCrypt(zero); both initialized payload constructions verify, prefix tamper fails.

```sh
python3 docs/research/gen6-9-codex/tools/run_signature_probe.py vendor/PKSM-Core/memecrypto --cc clang --msan
```

**Expected defect, exit1:** MemorySanitizer use-of-uninitialized-value in unchanged vendor `memecrypto.c:218:9::reverseCrypt`. Full diagnostic in `evidence/signature-msan.log`. This is not recorded as a passing clean sanitizer run. No ASan/UBSan suppressions were removed or expanded. MSan is additional evidence for an existing latent defect.

```sh
g++ -std=c++20 -Wall -Wextra -Werror -I/tmp/gen69-lane/include docs/research/gen6-9-codex/tests/reproduce_gen7_zring.cpp -o /tmp/gen69-zring-gcc
/tmp/gen69-zring-gcc
clang++ -std=c++20 -Wall -Wextra -Werror -I/tmp/gen69-lane/include docs/research/gen6-9-codex/tests/reproduce_gen7_zring.cpp -o /tmp/gen69-zring-clang
/tmp/gen69-zring-clang
```

Both exit0 **reproducing an unfixed defect** (x1 accepted, PKHeX-supported x2 rejected for both families). Repeated compilation/execution with `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` and `ASAN_OPTIONS=detect_leaks=1` also exits0 under each compiler. This validates reproducibility and memory safety of the harness execution, not correctness of the backend's quantity policy.

## #301 standalone checks

At `809a31841fc2ad43dda02d4412cb9891f29a482f`:

- `python3 tools/audit_legality_gen59_wiring.py --source-root .`: **FAIL**, mandatory production guard missing `Kind::LetsGo`.
- `python3 tools/legality_pkhex_source_pin.py --repo-root . --product-only`: exit0, two product reader blob identities verified; reports PKHeX archive/upstream verification **false**. Do not claim full source attestation from this mode.
- `g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_gen59_native_foundation.cpp -o /tmp/gen69-foundation-test` then binary: PASS15,813 domain assertions.
- Same flags, `tests/test_legality_gen6_native_pk6.cpp`: PASS64 records +114,688 ciphertext tamper mutations.
- Same flags, `tests/test_legality_gen7_native_pk7.cpp`: PASS64 records +114,688 ciphertext tamper mutations.

No full #301 sanitizer or executable .NET oracle run is claimed. .NET is unavailable locally.

## Fetched exact-head GitHub evidence: ORIGINAL CI records

`evidence/github-snapshot.json` lists queried heads and returned runs. Wrapper returns PR-triggered runs, first page only; not a guarantee of every repository workflow.

- #309/#311/#312: returned full host/Clang/native/candidate and focused jobs successful at their observed heads. Native base app jobs do **not** prove Gen6/7/8 source dirs are linked; lane docs say registration is pending.
- #301: exact-head foundation run38333848100 and host run38333847961 fail. Foundation compile/contract and PK5 differential steps fail.
- #307: host GCC/Clang, native, integrated candidate and several QA jobs succeed; NRO audit39074620706 fails at **Candidate identity versus head under test**. No root cause beyond inspected step metadata is claimed.
- #314: many legality jobs and host pass; Gen V differential39093998123 fails at **PK5 verdicts against PKHeX**. Gen V GCC/Clang normal/sanitizer and ten-minute libFuzzer steps in that same job succeeded. This is still an acceptance failure.

Failed job/step snapshots are in `evidence/failed-workflow-jobs.json`. Hardware acceptance is not implied by workflow names.

## Host benchmark and remaining coverage

`evidence/host-benchmark.json` measured SWSH's whole1159-check suite at about0.77s and the whole Gen6 stats suite at about0.017s. Cumulative child RSS high-water was18,428KiB. These are host suite measurements, not individual parser or Switch memory/time claims. No `.nro` is produced here.

Missing: authenticated retail vectors, independent PKHeX executable signing comparison, full later-game staging/export tests, original game acceptance, exact modern revision/catalog coverage and on-device performance. See RISKS_AND_BLOCKERS.
