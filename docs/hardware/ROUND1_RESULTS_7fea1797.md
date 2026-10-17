# Hardware round 1 results — PokeBank-NX-Gen1-5-Candidate-7fea1797

Recorded by QA from the owner's report, as relayed by the coordinator on 2026-10-17. Nothing here is
reclassified. Only the owner records acceptance.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-7fea1797.nro` |
| Code SHA | `7fea17971906cae557a7207bfb7b0bed3548cea2` |
| NRO SHA-256 | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` (175685297 bytes) |
| Build run / artifact | run `38946941097`, artifact `12066747065` |
| QA-F9 audit | app-qa-nro-audit run `38947904299`, job `117131594707`: PASS |
| Checklist | [RETEST_7fea1797.md](RETEST_7fea1797.md) |

**Owner's totals: 42 of 54 PASS, 6 confirmed FAIL, 6 NOT VERIFIED.**

## Confirmed FAIL (6)

| ID | As reported |
| --- | --- |
| B01 | Gen I Add Item asks for the quantity before placing the item |
| B02 | Gen II Add Item: same |
| B03 | Gen III Add Item: same |
| B06 | Stackable-item quantity workflow (same root cause as B01–B03) |
| S01 | Gen V PP / PP Ups captions too close together |
| S03 | Gen V Create/Edit left-panel text clipped; parity with the other generations |

## NOT VERIFIED (6) — not failures

| ID | As reported |
| --- | --- |
| B04 | Gen IV/V Add Item |
| E03 | Origin & Legality wording |
| E11 | Shiny vs non-shiny stat rerolls |
| L06 | Party / summary-panel taps |
| S04 | Gen V Review / Discard / Export |
| S06 | Exported copy round-trip in an emulator |

## PASS (42) — frozen behaviour for round 2

The owner's "Part 1" list, verbatim and grouped as relayed. The relayed list has 31 statements, and
some of them cover more than one of the owner's 42 checks. The per-ID breakdown of the 42 has been
requested from the coordinator and is not invented here. P-labels are QA's own, used only to map each
statement to the automated checks in
[`tests/test_hw_round1_regression_gate.py`](../../tests/test_hw_round1_regression_gate.py).

| P | Area | Owner's statement |
| --- | --- | --- |
| P01 | Controls | A selects/advances |
| P02 | Controls | B doesn't quit without warning |
| P03 | Controls | Launch requires confirmation |
| P04 | Controls | L/R correct |
| P05 | Controls | Gen II Create A/B |
| P06 | Controls | Popups don't activate background |
| P07 | Controls | Backup deletion confirm (native-save path) |
| P08 | Controls | Release/Remove confirm |
| P09 | Controls | Second controller |
| P10 | Moves | Gen I–V shared Moves panel alignment |
| P11 | Moves | Shared Move chooser |
| P12 | Moves | PP and PP Ups editing |
| P13 | Moves | Gen I hidden-looking row |
| P14 | Items | Unique-item quantity limited to 1 |
| P15 | Items | Removal confirm |
| P16 | Items | Bag edits in Review/export |
| P17 | Editor | Make Shiny removed |
| P18 | Editor | Consistent action order |
| P19 | Editor | Row counters removed |
| P20 | Editor | Consistent picker footers |
| P21 | Editor | Shiny Yes/No |
| P22 | Editor | Inline Pokérus/Gender/Gen III Language |
| P23 | Editor | Immediate Y rerolls |
| P24 | Editor | Gen I Pikachu L5 default |
| P25 | Editor | Gen III PID/Unown editing |
| P26 | Layout/scroll/saves | Gen II Data panel clipping |
| P27 | Layout/scroll/saves | Crystal + Gen III–V location picker readability |
| P28 | Layout/scroll/saves | Cursor/row overlap |
| P29 | Layout/scroll/saves | D-pad/analog/touch scrolling smooth |
| P30 | Layout/scroll/saves | Gen V party/box/trainer display |
| P31 | Layout/scroll/saves | Original emulator saves unchanged |

A passed item is frozen for round 2: any change that breaks its guard fails `make -f Makefile.host qa`
and the app-qa-contracts workflow.
