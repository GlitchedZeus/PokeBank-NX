# Gen I–IV completion campaign (Thread C) — checkpoint

Branch: `claude/gen14-campaign-20261015`, created from `c952565c` (PR #307 head), forward-merged
with #307 `44a976e1`. Integrated into #307 by Thread 2R (merge only).

Every NRO built from this branch is **NOT DEVICE TESTED**.

## Gates (per push)

GCC and Clang `host-test` + `host-sanitize` (touched targets each unit, full suites per
milestone), every `tests/*.py` contract, `git diff --check`.

## Units

| # | Item | Status | Commit | Tests |
|---|------|--------|--------|-------|
| 1 | QA-F3b: Gen II export publishes with exclusive mkdir (fail-closed), not rename(2) | done | see log | `test_gsc_export_transaction` (GCC/Clang, ASan/UBSan); fails against the old rename publish |
| 2 | PR #280 Gen III/IV empty-slot touch/A parity tests (5d47d6d9, e02995b2, dcd855a7) | done; behaviour already correct on #307, no production change needed | see log | `test_gen3/gen4_hardware_surface_contract` (GCC/Clang, ASan/UBSan); an occupancy-gated tap fails the scoped route check |
| 3 | Shared-editor contract parity, Gen I–III | pending | | |
| 4 | Phase 4 safety per generation | pending | | |
| 5 | Other Gen I–IV defects | pending | | |

## Next

Item 3: run the shared-editor 14-point contract for Gen I–III and fix divergences.
