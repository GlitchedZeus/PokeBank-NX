# Gen I original PKHeX oracle — SHA pin update for the Gen I/II legality engine

`.github/workflows/legality-gen1-exact-pkhex-oracle.yml` pins the exact Git blob of
the coordinator analyzer and of the fixtures it runs. This branch changes three of
them on purpose, so those three pins move. Nothing else in the workflow changes.

| Pinned file | Old blob | New blob | Why it changed |
|---|---|---|---|
| `_l5_gen1_coordinator/src/Legality/Legality.cpp` | `aeede141…` | `2e80b428…` | Native PK1 records now go through the Gen I/II origin search (`GB::checkGen1`), see [GB_LEGALITY_VERDICTS.md](GB_LEGALITY_VERDICTS.md). |
| `tests/fixtures/legality_l5_gen1_all_151_sourcefree_native.cpp` | `4c4fc7e3…` | `eecec04d…` | One assertion replaced, below. |
| `tests/fixtures/legality_l5_gen1_rby_real_save_pipeline.cpp` | `d7f571ec…` | `3cd51b86…` | Same assertion replaced, below. |

## The replaced assertion

Both fixtures asserted:

```cpp
report.coverage.transfer == CoverageLevel::Partial
report.verdict() != Verdict::NoProblemsFound
```

That encoded the earlier policy: every native PK1 stays incomplete, because Time
Capsule histories could not be checked. The analyzer now searches those histories,
and the owner's decision for this branch is PKHeX-style verdicts. "No problems found"
is reported only when a legitimate origin exists, and Invalid only for definitive
rules. The transfer coverage is therefore Complete, and the first line no longer
holds.

The replacement is stronger, not weaker:

```cpp
report.verdict() == Verdict::Invalid
has(report, Severity::Invalid, "Stat experience from battles raises every stat")
```

Every record in both fixtures has stat experience from battles with one stat still at zero
(`{65535,0,1,32768,65534}` and its rotations). A battle raises all five stats, so no
origin, cartridge label or Time Capsule trip can produce such a record. PKHeX
(kwsch/PKHeX@6501f0ab, `PK1` + `LegalityAnalysis`) was run on the exact bytes of all
5,738 distinct records the two fixtures build. It reports every one as invalid with
`EVs:EffortShouldBeZero`, the same rule. The old assertion only required "not No
problems found"; the new one requires a definite Invalid for the right reason. Every
other assertion in both fixtures is unchanged, and the expected output lines in the
workflow are unchanged.

## What is not changed

- The accepted analyzer pin (`_l5_gen1_accepted`, `b1d02d66…`) and the accepted
  baseline commit `d29eada5…`.
- The PKHeX pin `6501f0ab…` and its `PK1.cs` / `LegalityAnalysis.cs` blobs.
- `Gen1ReadOnlySave.cpp`, the oracle program, the oracle cases, the live-diff and
  protocol-negative tools, and the vector-integrity and raw-offset fixtures.
- Every step of the workflow, its expected output lines and its sanitizer runs.

`python3 tools/legality/verify_gen1_oracle_source_pins.py` passes with the new pins:
12 local sources match, and the 3 external pins are still enforced by the workflow.

## Second update: level, EXP and duplicate moves

`_l5_gen1_coordinator/src/Legality/Legality.cpp` moves again, from `2e80b428…` to
`5963b60d…`, for three structural fixes an independent audit found:

- A level-100 PK1 with EXP above the level-100 total read "No problems found": the
  level-from-EXP comparison stops at 100. The rule now lives in `GB::formatProblems`
  (shared with PK2 and with the PKHeX differential harness), and the generic analyzer path
  that native PK2 records take rejects EXP above the total as well. PKHeX reports the same
  records as `LevelEXPTooHigh`.
- Native PK1's level-against-EXP check moved from its own `Legality.cpp` site into the same
  GB rule, so the Invalid-site gate counts 43 sites instead of 44 (`host-tests.yml`).

No other pin, expected output line or step changes. The oracle workflow's 16 steps were
rerun locally at the new head with this pin.
