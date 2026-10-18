# Gen I–V hardware round 2 — status (QA)

Baseline: `PokeBank-NX-Gen1-5-Candidate-7fea1797` (owner sheet: 42 PASS / 6 FAIL / 6 NOT VERIFIED of 54;
[ROUND1_RESULTS_7fea1797.md](../hardware/ROUND1_RESULTS_7fea1797.md)). Lanes and ownership:
`docs/handoffs/ROUND2_LANE_PLAN.md` on the coordinator branch.

Rules for this table: an item moves only on evidence. A NOT VERIFIED item is never written as failed,
and a FAIL is never written as fixed until its lane's commit is on #307 with a host test, and only the
owner's retest of the next candidate turns it PASS. "Host-verified" means an automated host check, never
device acceptance.

| FAIL confirmed (owner) | Lane | Host evidence so far |
| --- | --- | --- |
| B01 Gen I Add Item workflow (owner: numpad before the item is in the bag) | I (phase 1, `SharedBagAddFlow.h`) | lane push `52017cd1` (not yet on #307): `tests/test_bag_add_flow_gen15.cpp`; QA CG12 XPASS on that tree (XFAIL here until merged) |
| B02 Gen II Add Item workflow (owner: same as B01) | I | as B01 (`52017cd1`) |
| B03 Gen III item editing (owner: same as B01/B02) | I | as B01 (`52017cd1`) |
| B06 Stackable item quantity editing (owner: part of B01–B03) | I | as B01 (`52017cd1`): quantity numpad only from an editable inventory row (CG12, `decideRow`) |
| S01 Gen V matches the shared Gen I–IV editor (owner note: PP / PP Ups text too close) | S (phase 9) | lane push `b1d34f9` (PP/PP Ups spacing, 10-row chooser; not yet on #307) |
| S03 Gen V Create, Clone, Move and Release (owner note: left-panel species text cut off, parity) | S (phase 12) | lane pushes `b57ffe0` + `4f030aa` (one Gen III–V DETAILS column 96/148 px, locked rows gold, no "(read-only)" labels; `tests/test_gen5_create_edit_parity.cpp` reproduces the old clip and shows it fixed); not yet on #307 |

| NOT VERIFIED (owner) — not a failure | Lane | Host evidence so far |
| --- | --- | --- |
| B04 Gen IV/V Add Item | I (phase 13) | lane push `52017cd1`: Gen IV/V through the same flow (`test_gen4_readonly_foundation.cpp` B04 round trip, `test_gen5_bag_screen.cpp`) |
| E03 Origin & Legality wording | QA (CG03, enforced) | CG03 PASS on 7fea1797 (run 38947904299) |
| E11 shiny vs non-shiny rerolls | R (phase 10/13) | lane push `951a8993`: `tests/test_reroll_mechanics_gen15.cpp` (100 rerolls per case, every generation), `docs/REROLL_MECHANICS.md`; Gen III `rerollPreservingShiny` and Gen IV `rerollPid` defects fixed there |
| L06 party/summary taps | 2R (phase 13) | none yet |
| S04 Gen V Review/Discard/Export | S (phase 12) | lane push `0f132eb`: `tests/test_gen5_review_discard_export_trace.cpp`, 360 checks on W/B/W2/B2, no production defect found. Retest notes: a Move appears in Review as two rows ("Clone to Box" + "Remove from Box"); Export Copy always also writes `EXPORT_MANIFEST.txt` |
| S06 exported copy round-trip in an emulator | QA / owner (device only) | host export contracts C1–C5 PASS (`tests/test_qa_export_contracts.cpp`) |

| PASS (owner, 42) | Lane | Guard |
| --- | --- | --- |
| C01–C09, M01–M08, B05, B07, B08, E01, E02, E04–E10, E12, E13, L01–L05, N01–N04, S02, S05 | all lanes: frozen | `tests/test_hw_round1_regression_gate.py` (per-ID guards) |

S01/S03: the sheet titles and the owner's notes differ; both are kept verbatim in
[ROUND1_RESULTS_7fea1797.md](../hardware/ROUND1_RESULTS_7fea1797.md). Whether Gen V
Create/Clone/Move/Release themselves fail is not stated by the owner and is not recorded as broken.

## E11 — what the owner asked (Lane R's finding, cited in `docs/REROLL_MECHANICS.md`)

The owner wrote that a shiny and a non-shiny Pokémon cannot have the same stats. Lane R's finding:
this is **true for Gen I/II only**. There shininess *is* a DV pattern (Defense, Speed and Special DV 10;
Attack DV one of 2, 3, 6, 7, 10, 11, 14, 15), so a shiny and a non-shiny of one species can never share
a DV spread, and Random DVs on a shiny may only move the Attack DV among those 8 values. In Gen III–V
shininess comes from the PID with the Trainer and Secret IDs (PID⊕TID⊕SID < 8); IVs and EVs are separate
fields, so identical stats on a shiny and a non-shiny are valid and there is no minimum "points" rule.
Documented limit: a Gen V shiny toggle may not preserve the Wurmple-line evolution value
(legality only, #314). E11 stays NOT VERIFIED until the owner checks the next candidate.
