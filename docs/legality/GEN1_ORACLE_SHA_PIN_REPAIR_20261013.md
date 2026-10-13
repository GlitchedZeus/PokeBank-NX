# Gen I original PKHeX oracle — source SHA pin gate repair

The coordinator source-verification workflow accidentally kept **two
contradictory local Git blob assertions** for the same
`tests/fixtures/legality_l5_gen1_all_151_sourcefree_native.cpp`.
Its earlier expected blob was `02711fccd0b5f0342c5dbe259bafaed5c003046e`;
the current exact source is
`4c4fc7e3b17c9097eb36d11c660adc821b902af2`.
Both assertions could not pass at the same exact HEAD.

Removed only the obsolete assertion. Preserved all original accepted
legality, original PKHeX.Core pin checks, 151-species QA, ASan/UBSan,
and their executable evidence gates.

Added a low-cost deterministic checker:
`python3 tools/legality/verify_gen1_oracle_source_pins.py --self-test`
and `python3 tools/legality/verify_gen1_oracle_source_pins.py`.

The checker verifies local source Git blobs against every local
`git hash-object` workflow pin; it also fails closed on duplicates,
missing files, unexpected checkout namespaces, path traversal and
truncated pin lists. It does *not* replace the pre-existing original
PKHeX/accepted source checks, which still run in the workflow.

**Execution evidence:** checker self-tests executed locally with
Python 3 and `git hash-object` and all five positive/negative cases
passed. A fresh exact-head GitHub Actions oracle job is still needed
to claim original PKHeX differential execution; a fixed pin is not
a claim that the actual legality engine passes.
