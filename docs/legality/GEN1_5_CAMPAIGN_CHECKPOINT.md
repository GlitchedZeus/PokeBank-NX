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
| 7 | Ruby | `Gen3Pk3` analyzer via `Legality::analyze` (app) | 9,457 | 0 / 0 | 10,187 / 10,191 | 96.0%, 98.2% | Done |
| 8 | Sapphire | same | 9,126 | 0 / 0 | 9,128 / 9,134 | 95.6%, 98.2% | Done |
| 9 | Emerald | same | 9,404 | 0 / 0 | 9,458 / 9,460 | 96.5%, 98.1% | Done |
| 10 | FireRed | same | 10,292 | 0 / 0 | 10,466 / 10,468 | 96.3%, 98.5% | Done |
| 11 | LeafGreen | same | 10,357 | 0 / 0 | 10,378 / 10,379 | 96.4%, 98.7% | Done |
| 12 | Diamond | `Gen4` analyzer via `Legality::analyze` (app) | 12,546 | 0 / 0 | 14,574 / 14,574 | 97.0% | Done |
| 13 | Pearl | same | 12,224 | 0 / 0 | 13,904 / 13,904 | 96.6% | Done |
| 14 | Platinum | same | 11,874 | 0 / 0 | 13,705 / 13,705 | 97.0% | Done |
| 15 | HeartGold | same | 16,443 | 0 / 0 | 20,790 / 20,790 | 97.1% | Done |
| 16 | SoulSilver | same | 16,410 | 0 / 0 | 20,535 / 20,535 | 97.1% | Done |
| 17 | Black | `Legality::analyzePk5` (engine) and `Gen5App::analyze` (adapter; no app caller until #307) | 38,425 | 0 / 0 | 46,271 / 46,623 | 99.97% | Engine done; app wiring with #307 |
| 18 | White | same | 33,245 | 0 / 0 | 40,944 / 41,200 | 99.95% | same |
| 19 | Black 2 | same | 47,564 | 0 / 0 | 54,475 / 54,743 | 99.95% | same |
| 20 | White 2 | same | 44,184 | 0 / 0 | 50,219 / 50,494 | 99.95% | same |

Gen V per-game rows count native, trade and event records of that game. Gen III/IV-origin
transfers into Gen V (all four games): 83,895 / 85,696 PKHeX-valid certify, 95,786 / 95,894
PKHeX-invalid caught; Colosseum/XD-origin transfers 1,842 / 2,021 certify (none did before
this campaign) and 3,442 / 3,470 caught. Colosseum and XD as origins of a PK3 are judged by the
Gen III analyzer (Lane A) and appear in the Gen III rows' corpus.

Gen III and IV: `tools/pkhex_gen34_diff/run.sh PKHeX <seed> 3` for seeds 1-3 (the Gen III/IV
mutation corpus, Colosseum/XD included) and `GROW=1 ... 1 3 34 grown` (grown Gen III, 21,963
records), on the engine head `69624064`. The 15 uncaught Gen III records all hold an Enigma
Berry, whose legality depends on the berry stored in the original save (a deliberate Unknown). 129 PKHeX-valid Gen IV records are
Invalid by the deliberately stricter met-location rule (review item 14); each is checked against
its byte-level predicate. Gen V: `tools/pkhex_gen5_diff/run.sh PKHeX <seed> 4` for seeds 1, 2, 7
and 11 (539,297 records). Gen I/II: `tools/pkhex_gb_diff/run.sh` seeds 1-3, 50,000 records each
for PK1 and PK2 (300,000). Total: 1,112,533 records, 0 false Invalid, 0 false Valid.

## Open Unknown / Incomplete cases

- **Gen III/IV:** records stay Incomplete when PKHeX itself only calls them "Fishy" (bad RNG
  frame, PID 0, SID 0, all IVs equal, a Ruby/Sapphire TID without a seed), for an English XD
  TID/SID with no seed, a checksum or sanity mismatch, or an Enigma Berry. That is about 4% of
  PKHeX-valid Gen III base records (2% grown) and 3% of Gen IV records.
- **Gen V:** Gen III/IV-origin records (Colosseum/XD included) with a PKHeX "Fishy" origin
  result, and a Gen III species that evolved after its transfer with an ability the PID does not
  set (review item 20), stay Incomplete; fateful records that match no PGF
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

## Final campaign (owner mandate 2026-10-17): result

- Lanes merged into this branch: C (Gen V audit, `Pk5Input`, `Gen5App::analyze`) `f61de953`;
  A (Colosseum/XD in the Gen III analyzer) `5e4e318f`; B (Gen IV audit, Colosseum/XD judge in Pal
  Park) `4a8b5faa`; D (Gen I/II audit) `0bbb0a6f`. Then the stored Gen I/II name bytes reach the
  engine (`5028a0a3`), the Colosseum/XD judge runs for Poke Transfer records (`93279d10`,
  `d113b8a9`, `c33cb2c9`, `69624064`), the Gen I oracle pins moved (`b19f6732`) and the Gen III
  context test follows the single GameCube path (`c36679b0`).
- Differentials on the engine head `69624064`: 1,112,533 records (Gen I/II 300,000; Gen III/IV
  273,236; Gen V 539,297), 0 false Invalid, 0 false Valid. 129 Gen IV records are Invalid by the
  deliberately stricter item 14. Colosseum/XD-origin Gen V transfers certify 1,842 / 2,021 (0 before).
- Host suites (`host-test`, GCC) pass; title matrix 183 / 183. Sanitizer, Clang and CI results
  for the final head are in the final report.
- Every deliberate difference: `PKHEX_DISAGREEMENT_REVIEW.md` items 1-20; only four entries are
  Verified improvements (`PKHEX_VERIFIED_IMPROVEMENTS.md`: D-1, D-2, D-3, IV-1).
- #307 rechecked at `348129fda54dc4c5a6ca04838f8479ab9af19aad`: ten conflicting files, all with a
  proposed resolution (`GEN1_5_APP_INTEGRATION_PREP.md`); `handoff/overlay-verdict-fix.patch`
  still applies. Nothing merged.
- Final report: `/mnt/project-files/legality/final-campaign-report.md` (sections A-G).

## Next steps

1. Owner decisions listed at the top of the final report.
2. Reviewed integration with #307 and a combined Gen I-V app and legality test campaign on
   Switch hardware (owner decides; nothing is merged before that).
