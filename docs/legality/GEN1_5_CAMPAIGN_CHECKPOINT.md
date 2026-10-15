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
| 4 | Gold | app: older Gen II checks in `Legality.cpp`; `GB::checkGen2` tested, not wired | Gen II corpus 3 × 50,000 (checkGen2) | 0 (checkGen2) | matches PKHeX (checkGen2) | no | Gap: wire `GB::checkGen2` into the app |
| 5 | Silver | same | same | same | same | no | same gap |
| 6 | Crystal | same | same | same | same | no | same gap |
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
| 17 | Black | `Legality::analyzePk5` | ~19,000 across B/W/B2/W2 | 0 (the one deliberate case is now a Warning, below) | 99.9% | no (OT/nickname text pending) | Strong, text gap |
| 18 | White | same | same | same | same | same | same |
| 19 | Black 2 | same | same | same | same | same | same |
| 20 | White 2 | same | same | same | same | same | same |

Gen III/IV counts are from `tools/pkhex_gen34_diff/run.sh PKHeX 1 3`; Gen V from
`tools/pkhex_gen5_diff/run.sh PKHeX 1 4` (seeds 7 and 11 also checked).

## Open Unknown / Incomplete cases

- **Gen III/IV never certify.** Encounter, PID/RNG, moves and egg coverage stay Partial,
  so every legitimate PK3/PK4 is Incomplete. The largest PKHeX-only rejections:
  PID/IV correlation (`PIDTypeMismatch`), encounter location/level (`EncInvalid`),
  Gen IV encounter type, PP/PP-ups, fateful flags and event-card matching, untrained
  EVs, unobtainable moves, Pokérus, Johto balls, nickname language.
- **Gen II in the app** still uses the older per-rule checks, not the PKHeX-matched origin
  search.
- **Gen V:** OT/nickname text of trades and event cards; fateful records that match no
  PGF card (warned, not Invalid: the card list is not proven complete); Gen III/IV-origin
  PK5 records only get Gen V-side checks.
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

## Next steps (not started)

1. Wire `GB::checkGen2` into `Legality::analyze` for PK2 (Gold/Silver/Crystal).
2. Raise Gen III/IV coverage using the differential's top PKHeX-only categories, keeping
   false Invalid at 0.
3. Gen V OT/nickname text for trades and event cards.
4. Combined Gen I–V app and legality test campaign on Switch hardware (owner).
