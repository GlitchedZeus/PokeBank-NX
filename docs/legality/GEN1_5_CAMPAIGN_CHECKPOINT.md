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

Columns: PKHeX-valid = PKHeX-valid records in the mutation corpus over the seeds named below;
false Invalid = PKHeX valid, PokeBank NX Invalid; caught = PKHeX-invalid records PokeBank NX also
reports Invalid (the rest are Incomplete); certifies = share of PKHeX-valid records that read
"No problems found" (base corpus, then the grown corpus).

| # | Game | Analyzer path | PKHeX-valid | False Invalid / False Valid | Caught | Certifies | Status |
|---|---|---|---|---|---|---|---|
| 1 | Red | `GB::checkGen1` (app) | Gen I corpus 3 × 50,000 | 0 / 0 (differences listed in GB_LEGALITY_VERDICTS.md) | matches PKHeX | yes | Done |
| 2 | Blue | same | same | 0 / 0 | matches PKHeX | yes | Done |
| 3 | Yellow | same | same | 0 / 0 | matches PKHeX | yes | Done |
| 4 | Gold | `GB::checkGen2` (app, since round 2) | Gen II corpus 3 × 50,000; app path parity 18,291/18,291 (seed 1) | 0 / 0 (differences in GB_LEGALITY_VERDICTS.md) | matches PKHeX | yes | Done |
| 5 | Silver | same | same | 0 / 0 | matches PKHeX | yes | Done |
| 6 | Crystal | same | same | 0 / 0 | matches PKHeX | yes | Done |
| 7 | Ruby | `Gen3Pk3` analyzer via `Legality::analyze` (app) | 8,413 | 0 / 0 | 8,796 / 8,799 | 95.3%, 97.9% | Done |
| 8 | Sapphire | same | 8,071 | 0 / 0 | 7,812 / 7,813 | 95.6%, 97.6% | Done |
| 9 | Emerald | same | 8,351 | 0 / 0 | 8,196 / 8,197 | 96.8%, 98.2% | Done |
| 10 | FireRed | same | 9,303 | 0 / 0 | 9,142 / 9,145 | 96.2%, 98.4% | Done |
| 11 | LeafGreen | same | 9,250 | 0 / 0 | 9,164 / 9,166 | 95.7%, 98.7% | Done |
| 12 | Diamond | `Gen4` analyzer via `Legality::analyze` (app) | 12,514 | 0 / 0 | 14,614 / 14,614 | 99.7%, 99.3% | Done |
| 13 | Pearl | same | 12,167 | 0 / 0 | 13,961 / 13,961 | 99.7%, 99.2% | Done |
| 14 | Platinum | same | 11,831 | 0 / 0 | 13,744 / 13,744 | 99.7%, 99.2% | Done |
| 15 | HeartGold | same | 16,513 | 0 / 0 | 20,720 / 20,720 | 100%, 99.2% | Done |
| 16 | SoulSilver | same | 16,385 | 0 / 0 | 20,560 / 20,560 | 100%, 99.2% | Done |
| 17 | Black | `Legality::analyzePk5` (engine only: no PK5 path in the app yet) | 227,135 across B/W/B2/W2 (seeds 1, 2, 7, 11) | 0 / 0 | 231,941 / 233,840 (99.2%) | engine: native, trade, event and Gen III/IV transfers (98.1%); Colosseum/XD transfers stay Incomplete. App: not wired | Engine done; app wiring with #307 |
| 18 | White | same | same | same | same | same | same |
| 19 | Black 2 | same | same | same | same | same | same |
| 20 | White 2 | same | same | same | same | same | same |

Gen III and IV: `tools/pkhex_gen34_diff/run.sh PKHeX <seed> 3` for seeds 1-3 (Gen III and IV
mutation corpus), `... <seed> 3 4 grown` (grown Gen IV) and `GROW=1 ... <seed> 3` (grown Gen III),
the CI set, on `d05d64f9`. The 10 uncaught Gen III records all hold an Enigma Berry (a deliberate
Unknown). The Gen IV records not certified in the base corpus (112) are exactly the ones the
deliberately stricter met-location rule rejects (review item 14). Gen V from
`tools/pkhex_gen5_diff/run.sh PKHeX <seed> 4` for seeds 1, 2, 7 and 11 (460,475 records).
Gen V "No problems found" by origin: native 117,358/117,444, in-game trade 8,587/8,587,
event 16,060/16,073, Gen III/IV transfer 80,992/82,536 (the rest are PKHeX "Fishy" R/S egg
TIDs and Gen III RNG frames, kept as Warnings, and one name outside the modelled text set);
Colosseum/XD transfer 0/1,995 (not analyzed).

## Open Unknown / Incomplete cases

- **Gen III/IV:** records stay Incomplete when PKHeX itself only calls them "Fishy" (bad RNG
  frame, PID 0, SID 0, all IVs equal, a Ruby/Sapphire TID without a seed), for a Colosseum/XD
  origin (Orre tables not ported), a checksum or sanity mismatch, or an Enigma Berry. That is
  about 4% of PKHeX-valid Gen III base records (2% grown) and 0.8% of grown Gen IV records.
- **Gen V:** Colosseum/XD-origin PK5 records (Orre tables not ported) and Gen III/IV-origin
  ones with a PKHeX "Fishy" origin result stay Incomplete; fateful records that match no PGF
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
- Stricter than PKHeX, each on decompiled game code: the stored level against the EXP (Gen
  I/II, item 1; Gen IV, item 10) and the two Gen IV met-location fields (item 14). The Gen V
  name-buffer limit (item 12) is stricter only in PKHeX's exemptions, on the record format.
- Gen V: a hidden-ability-only template with the hidden-ability flag but a slot 1/2
  ability was Invalid here while PKHeX accepts it. No original-game evidence rules it
  out, so it is now a Warning (Incomplete), as is a non-event Celebi or beast at its
  capsule transfer location (item 13). See
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
- (3, transfers) Done in merge `e7572bf5`: a Gen III/IV-origin PK5 is checked through the Gen IV
  analyzer on a PK4 view of the record (encounter, PID/IV, moves through Gen V, IV and III,
  ribbons, ball, ability), the way PKHeX analyzes a format-5 record. Seeds 1, 2, 7, 11: 0 false
  Invalid, 0 false Valid; 80,992 of 82,536 PKHeX-valid transfers certify (none did before).
  Colosseum/XD origins keep a Warning.
- (4) Done: [PKHEX_DISAGREEMENT_REVIEW.md](PKHEX_DISAGREEMENT_REVIEW.md) reviews all 14
  deliberate differences. Only the Gen V hidden-ability case lacked original-game evidence and
  is now a Warning; the Gen V name-length limit (item 12) matches PKHeX except its fixed-trainer
  and traded-egg exemptions and is kept. Item 13 (found by CI on 2877d2a5): a non-event
  Celebi/beast at its Gen IV capsule transfer location was Invalid and PKHeX accepts it; with
  no original-game source either way it is now a Warning. Item 14 (found by the title matrix):
  Pt/HG/SS write both met-location fields through one setter, so a native record whose fields
  disagree is now Invalid, stricter than PKHeX, with decomp evidence and a byte-level predicate
  in the Gen III/IV differential; My Pokemon Ranch records (Wii-written) only get a Warning.
- (2) Done. Gen III: `Gen3Pk3` analyzer (merges `517108e2`, `2877d2a5`); Gen IV: PK4 analyzer
  incl. Pal Park (merge `b5b44daf`). Both run in the app path and certify "No problems found".
  0 false Invalid and 0 false Valid on seeds 1-3, base and grown corpora. PKHeX-invalid caught:
  Gen III 13-14% -> 99.97%, Gen IV 31-32% -> 100%. See
  [GEN3_PKHEX_RULE_INVENTORY.md](GEN3_PKHEX_RULE_INVENTORY.md) and
  [GEN4_PKHEX_RULE_INVENTORY.md](GEN4_PKHEX_RULE_INVENTORY.md).
- (5) [LEGALITY_GATE_MATRIX.md](LEGALITY_GATE_MATRIX.md): 41 gates, every applicable one runs on
  this branch's head (the L4 event matrix and the lane workflows now fire here; L5 Stage 3 moved
  to ubuntu-24.04). CI on `afa3d6af` ran all 46 checks green; the Host tests job now builds
  its binaries in parallel (it had reached its 40-minute limit).
- (6) [TITLE_TEST_MATRIX.md](TITLE_TEST_MATRIX.md): 175 cases over all 20 titles (positive,
  negative, malformed, Unknown, original-game, title-specific), all PASS, no BUG rows. Its Gen IV
  rows found the met-location gap (item 14), now fixed.
- (7) [GEN1_5_RULE_COVERAGE.md](GEN1_5_RULE_COVERAGE.md) reports production-wired rules, corpus
  detection rates and test counts separately. Gen I-IV are production-wired; Gen V is engine-only
  until integration with #307.
- Integration: [GEN1_5_APP_INTEGRATION_PREP.md](GEN1_5_APP_INTEGRATION_PREP.md) (trial merge with
  #307, five conflicts, combined test plan). Nothing merged.

## Next steps

1. Every gate on the exact final commit; final report to the owner.
2. Reviewed integration with #307 and a combined Gen I-V app and legality test campaign on
   Switch hardware (owner decides; nothing is merged before that).
