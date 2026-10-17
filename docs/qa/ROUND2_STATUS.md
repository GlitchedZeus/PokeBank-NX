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
| B01 Gen I Add Item workflow (owner: numpad before the item is in the bag) | I (phase 1, `SharedBagAddFlow.h`) | none yet; QA CG12 pending the lane SHA |
| B02 Gen II Add Item workflow (owner: same as B01) | I | none yet |
| B03 Gen III item editing (owner: same as B01/B02) | I | none yet |
| B06 Stackable item quantity editing (owner: part of B01–B03) | I | none yet |
| S01 Gen V matches the shared Gen I–IV editor (owner note: PP / PP Ups text too close) | S (phase 9) | none yet |
| S03 Gen V Create, Clone, Move and Release (owner note: left-panel species text cut off, parity) | S (phase 12) | none yet |

| NOT VERIFIED (owner) — not a failure | Lane | Host evidence so far |
| --- | --- | --- |
| B04 Gen IV/V Add Item | I (phase 13) | none yet |
| E03 Origin & Legality wording | QA (CG03, enforced) | CG03 PASS on 7fea1797 (run 38947904299) |
| E11 shiny vs non-shiny rerolls | R (phase 10/13) | none yet |
| L06 party/summary taps | 2R (phase 13) | none yet |
| S04 Gen V Review/Discard/Export | S (phase 12) | none yet |
| S06 exported copy round-trip in an emulator | QA / owner (device only) | host export contracts C1–C5 PASS (`tests/test_qa_export_contracts.cpp`) |

| PASS (owner, 42) | Lane | Guard |
| --- | --- | --- |
| C01–C09, M01–M08, B05, B07, B08, E01, E02, E04–E10, E12, E13, L01–L05, N01–N04, S02, S05 | all lanes: frozen | `tests/test_hw_round1_regression_gate.py` (per-ID guards) |

S01/S03: the sheet titles and the owner's notes differ; both are kept verbatim in
[ROUND1_RESULTS_7fea1797.md](../hardware/ROUND1_RESULTS_7fea1797.md). Whether Gen V
Create/Clone/Move/Release themselves fail is not stated by the owner and is not recorded as broken.
