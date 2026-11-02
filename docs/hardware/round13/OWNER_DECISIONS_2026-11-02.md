# Round 13 owner decisions and tracking (November 2, 2026)

**Status: NOT DEVICE TESTED.** Round 13 candidate `9c301c48` is frozen for the owner's hardware test. Every
repair stays *Fixed in Code — Awaiting Hardware* until the owner tests it. No candidate is release-ready on
automated tests alone.

## 1. Product decisions (owner, Nov 2)

| # | Decision | Where it is implemented | State |
| --- | --- | --- | --- |
| D1 | Keep native Gen III Japanese full-width Latin characters when required; don't alter untouched original text. | Already in `9c301c48` (R12-008: re-store only on a language change; untouched text kept) | Fixed in Code — Awaiting Hardware |
| D2 | Do not duplicate HMs between PC and Bag. | Already in `9c301c48` (R12-007: HMs unique across Bag + PC) | Fixed in Code — Awaiting Hardware |
| D3 | Successful Move/Clone cancellation is silent; errors and refusals still show messages. | Already in `9c301c48` (R12-002) | Fixed in Code — Awaiting Hardware |
| D4 | Gen V touchscreen drag multi-select with controller parity and regression tests. | Next candidate (`fix/round13-gen15-repairs`) | In progress |
| D5 | Proper egg-group and breeding compatibility data; no generic egg histories presented as verified. | Next candidate | In progress |
| D6 | Gen IV Day-Care / trade location markers where native semantics and history are verified; unverified choices distinguished. | Next candidate | In progress |
| D7 | Gift/static encounters need source-backed data before v1.0 acceptance; markers alone are provisional. | Next candidate | In progress; **v1.0 blocker until data is in and accepted** |
| D8 | Crystal time rows may be grouped visually without losing encounter-time distinctions; ancestry labels accurate. | Next candidate | In progress |
| D9 | Existing Pokémon keep their current level on species change; EXP reconciled correctly. | Next candidate | In progress |

## 2. Evidence records (preserved honestly)

- **Round 12 custom checklist.** 24 tests: 20 PASS, 4 FAIL (H03, H04, H08, H14). Raw results:
  `docs/hardware/round12/OWNER_RESULTS_066e1db9.json` (byte-identical to the upload). Cross-reference:
  `docs/hardware/round12/ROUND12_OWNER_HARDWARE_INTAKE.md` ("Raw recorded test statuses"). The preserved
  record gives each custom ID's result and notes but **not the custom items' titles**. The custom numbering is
  not the repository's official H01–H18, so the 20 PASS results are recorded against their custom IDs and
  notes. They are never mapped to repository items by H-number alone.
- **Round 11 H46** (Export Copy reload and compare, optional). Owner-reported PASS on `3634d1e6`. No game and
  no comparison notes were recorded. Automated guards cover export-copy verification only, not the owner's
  emulator reload. Recorded as: owner PASS, evidence detail not recorded.
- **Round 6.** Results are preserved in `docs/hardware/ROUND6_RESULTS_26186464.md` (commit `6ec8dc53`; 49
  PASS / 9 FAIL / 1 NOT TESTED), built from the owner's Round 7 directive, which quotes every result. The
  owner's original `PokeBank-NX-Round6-Results-26186464.md` and its screenshots were never added to the
  repository or any PR or issue. The preserved record is used as the source, and the files are not requested
  again.

## 3. HOME card artwork repair (tracked)

- Current implementation: commit `c21d3747` (Round 11). The region scenery is drawn as one rounded vector path
  (`UI/RoundedArtwork.h`, `PKSEFramebuffer::drawImageRoundedCover`, `CardScenery::drawTopBand`). It was merged
  into `fix/round11-gen15-repairs` and is in candidates `066e1db9` and `9c301c48`.
- Owner result on `066e1db9`: custom H01 PASS. The custom titles are not preserved, so this is not counted as
  HOME-card acceptance. It is re-checked in Round 13 (`RETEST_9c301c48.md` H13).
- Sonnet's patch: **not present** in any branch, commit, PR, issue or upload in this repository as of Nov 2.
  When it is supplied, it will be reviewed against `c21d3747` and applied only where it improves the current
  implementation, never blindly. State: open, waiting for the patch.

## 4. v1.0 blockers (never dropped)

| Requirement | Source | State |
| --- | --- | --- |
| Move to Vault in every Gen I–V Actions list | R12-011; R11 H41–H43, Extra 7 | Missing Feature |
| Persistent Vault deposit and withdrawal | R12-011; R8 H04; R11 H41/H42 | Missing Feature |
| Vault search with highlighted matches | R12-012; R11 Extra 8 | Missing Feature |
| Cross-generation transfer (e.g. Gen I → later) | R11 H38 | Missing Feature |
| In-save box renaming | R11 H40 | Open |
| Gift/static encounter data, source-backed and accepted | D7 | In progress |

These blockers are listed in `docs/qa/V1_RELEASE_FEATURE_GAPS.md` and in every owner checklist's "Not in this
build" section until each is built and accepted on the owner's Switch.

## 5. Regression coverage before the next candidate

- Round 11 owner PASS items get a guard table in `tests/test_hw_round1_regression_gate.py`.
- The 94 earlier Hardware Accepted items whose code changed stay marked for re-verification. Each gets a
  targeted regression based on the actual code change (`docs/qa/RECHECK_REGRESSIONS.md`).
- No new candidate is built until both are in.
