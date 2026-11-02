# Hardware round 2 results — PokeBank-NX-Gen1-5-Candidate-e10f307f

Recorded by QA from the owner's round 2 result as relayed by the coordinator on 2026-10-19
(`docs/handoffs/ROUND3_LANE_PLAN.md` on `claude/optimistic-euler-ph2z4f`, 6e4ba8cb). Nothing is
reclassified. Only the owner records acceptance.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-e10f307f.nro` |
| Code SHA | `e10f307faf7254a47429babc49093e407ed65f1a` |
| NRO SHA-256 | `1dd518b756274cfe70d7a87b06a2e30f027389aeaddd9640f03ef3c5a2eb7ac0` (175791793 bytes) |
| QA-F9 audit | app-qa-nro-audit run `39122863086`, job `117661839347`: PASS |
| Checklist | [RETEST_e10f307f.md](RETEST_e10f307f.md) |

## Round 1 FAIL items retested — owner verdict

| ID | Item (round 1 sheet) | Result |
| --- | --- | --- |
| B01 | Gen I Add Item workflow improved | PASS |
| B02 | Gen II Add Item workflow improved | PASS |
| B03 | Gen III item editing still works correctly | PASS |
| B06 | Stackable item quantity editing works | PASS |
| S01 | Gen V matches the shared Gen I–IV editor (PP / PP Ups captions) | PASS |
| S03 | Gen V Create, Clone, Move and Release work (left-panel clipping, parity) | PASS |

These six are now **hardware-accepted** and guarded with the round 1 PASS items by
[`tests/test_hw_round1_regression_gate.py`](../../tests/test_hw_round1_regression_gate.py).

## Still NOT VERIFIED — not failures

| ID | Item | Result |
| --- | --- | --- |
| B04 | Gen IV and V Add Item workflows match | NOT VERIFIED |
| E03 | Origin & Legality heading matches in Gen I–V | NOT VERIFIED |
| E11 | Rerolls preserve requested Shiny / Gender | NOT VERIFIED |
| L06 | Party / summary taps open the right Actions screen | NOT VERIFIED |
| S04 | Review / Discard / Export Copy work | NOT VERIFIED |
| S06 | Exported edited save loads in emulator | NOT VERIFIED |

The round 1 PASS items (42) stay regression requirements; not all of them were retested on round 2
hardware. New round 2 reports (R2-BUG-001…013, R2-CHECK-009…011) are tracked in the round 3 plan.
