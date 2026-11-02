# Changelog

PokeBank NX is in **alpha** (`0.1.0-alpha`). There is no stable release. Each entry below is a development or
hardware-test candidate, identified by its commit and NRO SHA-256, with the result the owner recorded on a real
Switch. "Partial" means the owner tested it and recorded both passes and failures. The full classification is in
[docs/RELEASES.md](docs/RELEASES.md); the detailed results are in [docs/hardware/](docs/hardware/).

Dates are the candidate commit dates.

## Unreleased

- Round 13 fixes on `fix/round12-gen15-repairs` (draft PR #333), not yet on `main`.
- Repository documentation reorganised: `docs/README.md` index, `docs/STATUS.md`, this changelog,
  contributor guide and security policy.

## Round 13 — `9c301c48` (2026-11-02) — alpha candidate, owner-tested: 15 PASS / 12 FAIL, not accepted

- NRO SHA-256 `0ecf8655bcbef6490e73e2367801d972a8a9339396e73e16a4afe8f6ca76367d`, build run 41392463888.
- Addresses the Round 12 owner findings (R12-001 … R12-010): Gen V Boxes controls, routine Move/Clone
  pop-ups, the second cursor in Gen I/II packed-box Clone, Met Location lists and met-level checks, the Top Up
  item limit, TM/HM choices, Gen III language changes and PID, Japanese/Korean name display, and a stale export
  confirmation. Also re-checks earlier requirements that had dropped off checklists. (R12-011/012, the Vault
  features, are not in this build.)
- Status: **awaiting owner hardware acceptance.**

## Round 12 — `066e1db9` (2026-11-01) — alpha candidate, partial

- NRO SHA-256 `ba6aea09f9032fcab52b397cdd9c9d04fdf5b9b9cb4babf2bfa2f0fa7bd63c03`, build run 41273304655.
- Repairs for the Round 11 worksheet and the October 31 safety clarifications. Gen I–V only.
- Owner result: 20 PASS / 4 FAIL on the owner's 24-item checklist.

## Round 11 — owner worksheet on the Round 10 build

- No new build. The owner tested `3634d1e6`: 34 PASS / 13 FAIL / 1 NOT TESTED, plus 8 extra findings
  (including the missing Vault/Bank deposit and withdraw, and in-save box renaming).

## Round 10 — `3634d1e6` (2026-10-28) — alpha candidate, partial

- NRO SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`.
- Gen IV Bag: Remove packs the pocket; Berries and TMs are added in game order.
- Result: see Round 11.

## Round 9 — `d6a20a2a` (2026-10-28) — alpha candidate, not tested

- NRO SHA-256 `57b2c1186942473ee53f78df6f44bc5ab430acb0b86f26ecddc8326c87c0494a`.
- Combined candidate; no owner results recorded (superseded by Round 10).

## Round 8 — `6ec5ccb9` (2026-10-27) — alpha candidate, not tested

- NRO SHA-256 `49d95c555636e3ed3f461c080c168d06c963415801d8ad02b0f7ae5f87eee186`.
- Safety candidate; the checklist moved to ordinary saves, with edge cases covered by an automated matrix.
  No owner results recorded.

## Round 7 — `e88957f2` (2026-10-27) — alpha candidate, not tested

- NRO SHA-256 `9e1b6a314fd7643901b6bb157dc69bfa82de61b9b4c45dc97214021f3db813b5`.
- No owner results recorded.

Rounds 7–10 also added standalone `.pk1`–`.pk5` / `.ek3`–`.ek5` import and export, Bag Top Up / Add Missing,
folder favourites, a Recovery files view, and read-only opening of Japanese Gen I/II saves.

## Round 6 — `26186464` (2026-10-26) — alpha candidate, partial

- NRO SHA-256 `5b1a85c878e2999f0f8212a583f037643c3b42147a21fef631ea11a832d2a3bf`.
- Owner result: 49 PASS / 9 FAIL / 1 NOT TESTED ([results](docs/hardware/ROUND6_RESULTS_26186464.md)).

## Round 5 — `1ca2faec` (2026-10-25) — alpha candidate, partial

- NRO SHA-256 `b8966cdbd108530ae1c056a07e318d2f05d0cdee30d910b361ab29f381e41511`.
- Owner result: 31 PASS / 8 FAIL / 3 NOT TESTED ([results](docs/hardware/ROUND5_RESULTS_1ca2faec.md)).

## Round 4 — `c60d4da0` (2026-10-22) — repairs, partial

- No separate candidate NRO; the repairs shipped in Round 5. Retests on this commit are recorded in
  [ROUND4_REPAIRS_AND_RETEST.md](docs/hardware/ROUND4_REPAIRS_AND_RETEST.md) and
  [OCT24_REPAIRS_AND_RETEST.md](docs/hardware/OCT24_REPAIRS_AND_RETEST.md).

## Round 3 — `62a2f8c3` (2026-10-19) — alpha candidate, partial

- NRO SHA-256 `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078`.
- Owner result: 70 PASS / 37 FAIL / 3 NOT TESTED ([results](docs/hardware/ROUND3_RESULTS_62a2f8c3.md)).

## Round 2 — `e10f307f` (2026-10-18) — alpha candidate, partial

- NRO SHA-256 `1dd518b756274cfe70d7a87b06a2e30f027389aeaddd9640f03ef3c5a2eb7ac0`.
- Owner result: 6 PASS / 6 NOT VERIFIED ([results](docs/hardware/ROUND2_RESULTS_e10f307f.md)).

## Round 1 — `7fea1797` (2026-10-16) — alpha candidate, partial

- NRO SHA-256 `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5`.
- First combined Gen I–V shared-editor candidate. Owner result: 42 PASS / 6 FAIL / 6 NOT VERIFIED
  ([results](docs/hardware/ROUND1_RESULTS_7fea1797.md)).

## Before the numbered rounds (September – mid-October 2026)

- `fa9cfcea` (2026-10-15): Gen I–V integration run — **failed** on hardware (Gen V routing, blank Create,
  new-game trainer).
- `0760334f` (2026-10-07): integrated Gen I–IV / Product Home — **accepted on hardware for that checkpoint**,
  NRO SHA-256 `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91`.
- `50dac31f` (2026-09-11): read-only Red/Blue/Yellow with Items — **accepted on hardware for that milestone**,
  NRO SHA-256 `b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b`.
- `a2df4c1a` (2026-09-10): read-only Ruby/Sapphire/Emerald — **accepted on hardware for that milestone**,
  NRO SHA-256 `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.
- `d9077e2d` (2026-09-10): first Red/Blue/Yellow device build — **failed** (Items category defect),
  NRO SHA-256 `b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664`.
- 2026-09-01: project imported from PKSE 1.1.3 and restricted to read-only sources.

Those accepted builds covered narrow, read-only milestones. They are superseded and are not releases.

## Gen VI–IX (paused, `development/gen6-plus`)

- `43c296dc` (2026-11-02): experimental, not device tested, NRO SHA-256
  `fd67dd1fdb21f835b6b6e392dc25bdf73a041b07aca77cc711de0716b124e251`.
- `5895e6b9` (2026-11-01): experimental — **failed** owner hardware testing, NRO SHA-256
  `dc6427244aff9cee3016ddd930bf65075059dc152e126878b039e3faa9204b41` (as reported by the build lane).
