# L6 Codex — Stage 2 native PK4 field-width and intake boundary

Date: 2026-10-12. Existing isolated audit branch:
`audit/legality-integration-hardening-20261012`, draft PR #299.
No merge, rebase, force update, source save writes, Gen V, or MAIN/accepted
legality/coordinator/L1–L5 edits.

## Production defect and narrow correction

Pre-patch `src/Pokemon/Pokemon4ReadOnly.cpp` used
`if (offset + 1 >= decrypted_.size())` and
`if (offset + 3 >= decrypted_.size())` for private native u16/u32 reads.
The additions can wrap for `offset = SIZE_MAX` or nearby values. On a valid
nonzero-PID PK4, `u16At(SIZE_MAX)` could combine the upper byte at offset
zero into a fictitious field value; similarly u32 reads near SIZE_MAX.
The new `Pokemon::NativeRecordBounds::contains(extent,offset,width)`
does not perform offset-plus-width arithmetic, and returns false unless
`offset <= extent && width <= extent-offset`.

- Production patch `src/Pokemon/Pokemon4ReadOnly.cpp`: use the shared
  constant-expression predicate for byte/u16/u32 reads; no public API,
  byte-layout, serialization, or save-writing change.
- `include/Pokemon/NativeRecordBounds.h`: small dependency-free guard.
  Exact Git blob `5f8f9f809d0fd75a98f6b0360f76125902fb2bf0`.
- Existing independently committed PK3 `fieldFits` fix is preserved, not
  overwritten; existing PK4 `Gen4NativeRecordBoundary::copyExact` remains
  unchanged. Full-width boundary tests prevent either regression.
- White-box native integration test
  `tests/fixtures/legality_l6_stage2_pk4_offset_native.cpp` constructs
  128 synthetic encrypted stored and party PK4 records each (256 records),
  covers near-SIZE_MAX u16/u32 reads and 2 MiB malformed storage, checks
  caller input immutability and demands **6,948 native assertions**.
  It compiles with production C++ sources; no original retail specimens.

## Actual independently executed Linux C++ verification

This environment cannot Git-clone the full repository: `git ls-remote`
fails because GitHub DNS resolution is unavailable. Exact byte copies of
the four relevant committed header/test files were independently confirmed
with `git hash-object` against GitHub's live blob SHA:

- `Encryption4.h`: `e823d3fb2fdd2008047d79e15faea1c2a12f48a0`
- `Gen4NativeRecordBoundary.h`: `69ec600e9cc04588d7fbe79c68a232bdd8bc6eb7`
- `NativeRecordBounds.h`: `5f8f9f809d0fd75a98f6b0360f76125902fb2bf0`
- `legality_l6_stage2_bounds_contract.cpp`: `76fddd370648f0a371e73ad8fd89c301e9801826`
- `legality_l6_stage2_joint_headers.cpp`: `6b1d9e6355dd99b87c0b954b59db9e69e36d71e7`

Both native suites were built and **executed** with:

```sh
g++     -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude TEST.cpp -o /tmp/test
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude TEST.cpp -o /tmp/test
g++     -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude TEST.cpp -o /tmp/test
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude TEST.cpp -o /tmp/test
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/test
```

`TEST.cpp` denotes each of the two exact-fixture sources under
`tests/fixtures/`; the local compiler include root contained the
byte-identical production headers listed above.

| Actually executed native suite | GCC O2 | GCC ASan+UBSan | Clang O2 | Clang ASan+UBSan |
|---|---|---|---|---|
| Shared overflow-safe field width, **644,426 checks/run** | PASS | PASS | PASS | PASS |
| Joined real PK4 intake and width headers, **11,800 checks/run** | PASS | PASS | PASS | PASS |

Eight independent compiled binaries, **2,624,904 executed bounded checks**
in aggregate. The local Python executor's independent attestation controls
and scoped `--header-only` test runs passed. These are exact header/fixture
builds, **not** a full production class link or `Legality::analyze` test.

## Full checkout and coordinator release blockers

`tools/audit_l6_stage2_native_bounds.py` is registered with the
audit-owned `tools/run_legality_integration_audit.py`. The default runner
requires 12 builds: two header suites and the real production-linked PK4
suite, each in GCC/Clang normal and ASan+UBSan modes. It must be executed on
the current exact PR #299 checkout with:

```sh
python3 tools/audit_l6_stage2_native_bounds.py \
  --expect-head "$(git rev-parse HEAD)"
python3 -m unittest tests/test_legality_integration_audit_runner.py
python3 tools/run_legality_integration_audit.py --suite all \
  --expect-head "$(git rev-parse HEAD)" \
  --expect-tree "$(git rev-parse 'HEAD^{tree}')" --keep-going
```

**Not executed here:** full linked 6,948-assertion PK4 suite; production
`Legality::analyze`; entire L1–L5 full-snapshot matrix; clean host/RSE;
complete ASan/UBSan exact-head CI. Their states remain PENDING/BLOCKED,
not green. Stage 1's stale Diamond/Pearl L2 duplicate-route assertion
remains a coordinator/lane handoff, not an authorized L6 edit.
No original save or historic authenticated reference receipt was created.

This Stage 2 stops at a narrow production-read boundary and independently
verifiable regression coverage. Do not infer full PKHeX parity from it.
