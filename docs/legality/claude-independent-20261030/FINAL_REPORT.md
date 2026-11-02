# Lane A final report: independent Gen I–V legality (Claude), 2026-11-01

PR #327, branch `work/claude-gen15-legality-independent-20261030`.
- Head when this report was written: `0c157f091db8a2805c2accab60667c66ae306344` (plus this report).
- It contains #314 up to `3f620c791634736d293ae69bf2686b0790f344f1`.
- References:
  - PKHeX 26.10.18 `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`
  - pret/pokered `af519899`, pret/pokeyellow `e89ead15`, pret/pokecrystal `3bc8daa4`

This is a record of work done and gates run. It is not a completeness or release claim.

## What changed in the engine

Every rule below is definitive only where the disassemblies show that no routine can produce the
state. Each one fires only when the raw record bytes agree with the parsed fields.

| Milestone | Commit | Rule | PKHeX 26.10.18 |
|---|---|---|---|
| 1 | `828bdcda` | `Context::storage`: party or box slots make a stale level or over-cap EXP definitive. Day Care slots expect them, and Unknown slots give a Warning. | Same direction, no slot context |
| 2 | `6cc05036901bd7d1f2c43fb0a8857ec04651dc53` | Party max HP equals the stat formula for some HP stat exp ≤ the stored value | Not checked |
| 3 | `af06513e20baca6b3e2cb3f63932d5422fee31ab` | All party stats, including the Time Capsule Special recomputation. `GBBaseStats.inc` is generated from pret. | Not checked |
| 4 | `7b1b1efab36315fa3117c57210a8398fe9f6a49f` | Current HP ≤ max HP (party), and ≤ the formula max at the stored level (Gen I box) | Not checked |
| 5 | `4d78fdf0546d920e7481ebad4336bf9e85d3b487` | Status byte is 0, a sleep counter of 1–7, or exactly one of PSN/BRN/FRZ/PAR. A PK2 egg has status 0. The Gen I party box-level byte is ≤ the level. | Not checked |
| 6 | `7c7b160960557c91a95a73fc627f6247baf5038d`, `c330589dad9f978a44e0c24566e5d1e0c8c92d0b` | An unhatched Day Care, Togepi or Odd Egg has no Crystal met data. Kenya and the Mania Shuckle have a male OT bit. | Accepts both |

One rule was considered and refused: "a fainted Pokémon has no status". Recoil and Destiny Bond
double KOs save HP 0 with a status. The README has the citations for each rule.

## Gates at the head

| Gate | Result |
|---|---|
| `make -f Makefile.host host-test` (g++ 13, -Werror) | Pass on `0c157f09` |
| GB tests under ASan/UBSan | Pass |
| GB tests under clang++ -Werror | Pass |
| `test_gb_legality_structural` | 3,964,650 checks pass |
| CI `legality-gb-pkhex-diff` (3 × 50,000 per format) | Pass on `0c157f09` |
| GB differential, seed 20261031, 12k per format | PK1: 0 differing, and verdicts are byte-identical to Milestone 5. PK2: the same 2 known records (#9716, hand-verified ancestry; #11060, an unauthenticated gift kept Incomplete). |
| Appended records | Controls are legal in both engines. Defects are in the deliberate classes: `party-stats`, `status-byte`, `box-level`, `egg-met-data`, `gift-ot-gender`. |
| PK2 production parity | Pass |
| CI seeds 1–3 (3 × 50,000 PK2), appended records | None in a differing class |
| `compare.py --self-test` | 41/41 |

**Skipped or limited gates, stated plainly:**
- Locally, `run.sh` for PK2 exits 1. Plain `compare.py` leaves the native-certified ancestry
  records as "differing", while CI classifies them. I checked only the appended records'
  classes by hand.
- `tools/pkhex_gen5_diff/run.sh` stops at the word-filter patterns step. The Switch list has
  4,533 entries and PKHeX has 4,733. I ran the remaining steps by hand. This is Codex's area.
- CI on #327 is red at the time of writing because of #314's head `44016106`, not #327:
  - That head fails its own `test_gen3_legality_context.cpp:342`.
  - Its new `legality-completion.yml` builds a target that #327 gets only once that head is
    merged.
  - The PR comment has the details. The merge is waiting for a passing #314 head.

## Cross-generation differentials (A3 and the Phase 3/5 Gen I–V parts)

| Generation | Corpus | False Invalid | False clean | Owner of the gaps |
|---|---|---:|---:|---|
| III | base and grown corpora | 0 | 0 | — |
| IV | grown, seed 2026103102, 80,820 records | 0 | 2 (`EVs:EffortUntrainedMoreEXP_0`, Pal Park PK4) | Codex (EV/EXP) |
| V | seed 2026103104, 80,847 records | 0 | 20: 18 from the word-filter list, 2 from a B2 Petilil Bide + Helping Hand egg | Codex (naming, ancestry) |

Repro hex for the Gen V records: `/mnt/project-files/legality/lane-a-gen5-codex-findings-20261101.md`.
I did not change anything in Codex's areas.

## Scope items not advanced here

- **A2, historical events** (Mew, GS Ball Celebi, PCNY, Korean): the lane relied on #314's
  existing classes. It added no new event authentication. The GS Ball Celebi and Korean
  records stay in their documented classes, and unverified histories stay Incomplete.
- **The Gen I/II Incomplete column.** It is mostly Day Care level/EXP and stat-exp Warnings.
  Milestone 1 settles it only when the caller supplies the slot. The app still has to pass
  `Context::storage`, which is out of this lane's scope.

## Consolidation into `development/legality-engine`

1. Merge #314.
2. Merge #327 with a merge commit, not a rebase.
3. Refresh the Legality.cpp blob pin in `legality-gen1-exact-pkhex-oracle.yml`.
4. Rerun `legality-gb-pkhex-diff`.

#327 shares no file with #328 (Gen VI–IX). Possible conflicts are limited to the GB sections of
`Legality.cpp` and that pin.
