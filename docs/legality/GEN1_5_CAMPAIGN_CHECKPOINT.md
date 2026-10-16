# Gen I–V legality campaign checkpoint

Branch `claude/gen1-5-legality-campaign-w9mkom`, PR #314 (draft, not merged).
Scope: the legality engine for the 20 mainline games Red through White 2. Gen VI–IX
legality is frozen until the owner authorizes it. Nothing here writes saves or changes
a Pokémon; every analyzer reads its input and returns a report.

Reference: PKHeX.Core `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.
Result classes: **Invalid** (a definitive rule fails), **No problems found** (every check
complete, nothing wrong), **Incomplete** (Unknown: something could not be checked).
Unknown or unchecked is never turned into Invalid.

## How each generation is measured against PKHeX

Each differential builds every encounter PKHeX can generate for the games, adds seeded
single-field mutations, and compares PKHeX's verdict with PokeBank NX's on the same bytes.
A **false Invalid** (PKHeX valid, PokeBank NX Invalid) fails the run.

| Generation | Tool | CI workflow |
|---|---|---|
| I, II | `tools/pkhex_gb_diff/run.sh` (PK1, PK2 corpora) | `legality-gb-pkhex-diff.yml`, `legality-gen1-exact-pkhex-oracle.yml` |
| III, IV | `tools/pkhex_gen34_diff/run.sh` (PK3, PK4 through the production entities and `Legality::analyze`) | `legality-gen34-pkhex-diff.yml` |
| V | `tools/pkhex_gen5_diff/run.sh` (PK5 through `Legality::analyzePk5`) | `legality-gen5-pkhex-diff.yml` |

## 20-game checklist

Columns: records = PKHeX-valid records in one run (seed 1); false Invalid = PKHeX valid,
PokeBank NX Invalid; caught = share of PKHeX-invalid records PokeBank NX also reports
Invalid (the rest are Incomplete); certifies = whether the app can say "No problems found".

| # | Game | Analyzer path | PKHeX-valid records | False Invalid | Caught | Certifies | Status |
|---|---|---|---|---|---|---|---|
| 1 | Red | `GB::checkGen1` (app) | Gen I corpus 3 × 50,000 | 0 (by design: differences are listed in GB_LEGALITY_VERDICTS.md) | matches PKHeX | yes | Done |
| 2 | Blue | same | same | 0 | matches PKHeX | yes | Done |
| 3 | Yellow | same | same | 0 | matches PKHeX | yes | Done |
| 4 | Gold | `GB::checkGen2` (app, since round 2) | Gen II corpus 3 × 50,000; app path parity 18,291/18,291 (seed 1) | 0 (differences in GB_LEGALITY_VERDICTS.md) | matches PKHeX | yes | Done |
| 5 | Silver | same | same | 0 | matches PKHeX | yes | Done |
| 6 | Crystal | same | same | 0 | matches PKHeX | yes | Done |
| 7 | Ruby | `Legality::analyze` (PK3) | 2,846 | 0 | 13% | no | Safe, partial |
| 8 | Sapphire | same | 2,679 | 0 | 14% | no | Safe, partial |
| 9 | Emerald | same | 2,805 | 0 | 13% | no | Safe, partial |
| 10 | FireRed | same | 3,105 | 0 | 13% | no | Safe, partial |
| 11 | LeafGreen | same | 3,066 | 0 | 13% | no | Safe, partial |
| 12 | Diamond | `Legality::analyze` (PK4) | 4,176 | 0 | 31% | no | Safe, partial |
| 13 | Pearl | same | 4,112 | 0 | 31% | no | Safe, partial |
| 14 | Platinum | same | 3,951 | 0 | 31% | no | Safe, partial |
| 15 | HeartGold | same | 5,512 | 0 | 32% | no | Safe, partial |
| 16 | SoulSilver | same | 5,416 | 0 | 32% | no | Safe, partial |
| 17 | Black | `Legality::analyzePk5` | 227,135 across B/W/B2/W2 (seeds 1, 2, 7, 11) | 0 (false Valid also 0) | 93.4% | yes for native, trade and event records (99.9%); Gen III/IV transfers stay Incomplete | Strong, transfer gap |
| 18 | White | same | same | same | same | same | same |
| 19 | Black 2 | same | same | same | same | same | same |
| 20 | White 2 | same | same | same | same | same | same |

Gen III/IV counts are from `tools/pkhex_gen34_diff/run.sh PKHeX 1 3`; Gen V from
`tools/pkhex_gen5_diff/run.sh PKHeX <seed> 4` for seeds 1, 2, 7 and 11 (461,340 records).
Gen V "No problems found" by origin: native 117,432/117,521, in-game trade 8,579/8,579,
event 16,061/16,074, Gen III/IV transfer 0/84,961 (their original encounter is not yet
checked from the PK5 side).

## Open Unknown / Incomplete cases

- **Gen III/IV never certify.** Encounter, PID/RNG, moves and egg coverage stay Partial,
  so every legitimate PK3/PK4 is Incomplete. The largest PKHeX-only rejections:
  PID/IV correlation (`PIDTypeMismatch`), encounter location/level (`EncInvalid`),
  Gen IV encounter type, PP/PP-ups, fateful flags and event-card matching, untrained
  EVs, unobtainable moves, Pokérus, Johto balls, nickname language.
- **Gen V:** Gen III/IV-origin PK5 records get the Gen V-side transfer checks but not their
  original Gen III/IV encounter, so they stay Incomplete; fateful records that match no PGF
  card (warned, not Invalid: the card list is not proven complete); a BACD PID coincidence
  on a shiny card PID; JP/KR OT names of exactly six characters; a nickname that differs
  from the card's; gender when several templates match; names outside the modelled text
  set; Gen III fixed-OT gifts.
- **Gen I/II stat experience:** battle stat experience next to a zero stat stays Unknown
  (gains are divided among recipients, pret `DivideExpDataByNumMonsGainingExp`).

## Deliberate differences from PKHeX

Each one is reviewed against original-game evidence in
[PKHEX_DISAGREEMENT_REVIEW.md](PKHEX_DISAGREEMENT_REVIEW.md).

- Gen I/II: listed in [GB_LEGALITY_VERDICTS.md](GB_LEGALITY_VERDICTS.md).
- Gen V: a hidden-ability-only template with the hidden-ability flag but a slot 1/2
  ability was Invalid here while PKHeX accepts it. No original-game evidence rules it
  out, so it is now a Warning (Incomplete). See
  [GEN5_PKHEX_RULE_INVENTORY.md](GEN5_PKHEX_RULE_INVENTORY.md).

## False Invalids fixed in this campaign (found by the differentials)

- Gen IV Unown: every letter but A was Invalid (native personal tables list one form).
- Gen III Pokémon Park event eggs (met location 255) and Gen IV My Pokémon Ranch
  Phione/Wynaut: Invalid "egg-origin met level".
- Gen IV PCD event gifts whose card ability differs from the PID slot.
- Gen V: Basculin, Deoxys/Rotom forms, N's natures, PGF and fixed-slot abilities, Entree
  move sets, unused ribbon bits, B2W2 trade lists (all before the first push).

## Verification commands

```
make -f Makefile.host host-test host-sanitize            # GCC; CXX=clang++ for Clang
bash tests/test_legality_l1_run.sh                       # lanes L1-L4
bash tests/test_legality_l2_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 bash tests/test_legality_l3_run_all.sh
bash tests/test_legality_l4_runner.sh; L4_SANITIZE=1 bash tests/test_legality_l4_runner.sh
python3 tools/audit_legality_l6_stage5.py --compiler g++ --output /tmp/l6.json
tools/pkhex_gen34_diff/run.sh /path/to/PKHeX 1 3
tools/pkhex_gen5_diff/run.sh /path/to/PKHeX 1 4
python3 tools/gen_legality_gen5_data.py --pkhex /path/to/PKHeX --check
```

## Round 2 (owner review 2026-10-15): progress

Owner priorities, in order: (1) wire `GB::checkGen2` into the app; (2) Gen III/IV production
rules, the largest priority; (3) Gen V trade/event text, PGF provenance, Gen III/IV-origin
transfer; (4) review every deliberate PKHeX disagreement, requiring original-game evidence for
any stricter Invalid; (5) every L1-L5 gate on the exact final commit, including skipped event
checks; (6) broader tests across all 20 titles; (7) report production-wired rule coverage
separately from corpus detection rates and test counts. No merge without the owner.

- (1) Done in `7356b3835d2d97eed270cae2d86300ece9b55f26`: native PK2 goes through
  `analyzeNativeGen2` (format checks, then `GB::checkGen2`); the older PK2 provenance notes stay
  as Info. `tools/pkhex_gb_diff/run.sh` now also runs the corpus through the app path
  (`production_parity.cpp`) and fails when the app's verdict differs from the origin search:
  PK2 seed 1: 18,291 records, 0 differences (1,709 Korean records skipped: the app reads no
  Korean saves); PK1 seed 1: 20,000 records, 0 differences.
- (5) The L4 event-metadata matrix (g++/clang++, sanitizer on/off) only ran on PR #292; it now
  also runs on this branch.
- (3) Done in merge `2565e0c9`: Gen V in-game trade and event OT/nickname/language/OT gender
  checks, PGF card provenance, Gen III/IV-origin transfer checks (Gen V side), a decoder for
  Gen V names (`Gen5Text`). Seeds 1, 2, 7, 11: 0 false Invalid, 0 false Valid; PKHeX-invalid
  records caught 82.3% -> 93.4%; native/trade/event records now certify "No problems found".
  Three Gen V calls for the owner to confirm: the hidden-ability flag on a fateful Gen IV
  origin is a Warning (PKHeX `VerifyAbilityPCD`); the Wurmple branch is Invalid only when no
  other template matches; Mothim forms 1-2 are accepted.
- (4) Done: [PKHEX_DISAGREEMENT_REVIEW.md](PKHEX_DISAGREEMENT_REVIEW.md) reviews all 13
  deliberate differences. Only the Gen V hidden-ability case lacked original-game evidence and
  is now a Warning; the Gen V name-length limit (item 12) matches PKHeX except its fixed-trainer
  and traded-egg exemptions and is kept. Item 13 (found by CI on 2877d2a5): a non-event
  Celebi/beast at its Gen IV capsule transfer location was Invalid and PKHeX accepts it; with
  no original-game source either way it is now a Warning.
- (2) In progress: separate Gen III (PK3) and Gen IV (PK4, incl. Pal Park) analyzers measured by
  `tools/pkhex_gen34_diff` with a false-Valid gate and a grown-record corpus.

## Next steps

1. Raise Gen III/IV coverage using the differential's top PKHeX-only categories, keeping
   false Invalid at 0.
2. Gen V OT/nickname text for trades and event cards.
3. Combined Gen I–V app and legality test campaign on Switch hardware (owner).
