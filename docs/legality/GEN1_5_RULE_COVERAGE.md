# Gen I-V legality: rule coverage, detection rates and test counts

Three separate measures, kept apart on purpose (owner priority 7):

1. **Production-wired rule coverage**: which rules the app actually runs when it shows a
   Pokemon's legality report. A rule that exists only as engine API or in a test does not count.
2. **Corpus detection rates**: how the engine's verdicts compare with PKHeX on generated
   corpora. They measure agreement on those corpora, not completeness of the rules.
3. **Test counts**: how many test cases and fuzz inputs run in the host suites.

Head measured: `b5b44daf84dbfccb30f559f8c440c6f4cfda2e0e` (re-check on the final head).
Reference: PKHeX.Core `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

## 1. Production wiring

Every app screen calls one function, `Legality::analyze(pokemon, gameGroup, exactSourceGameId)`
(`src/Legality/Legality.cpp`). What it runs depends on the record's format.

| Gen | Titles | App call sites | What `analyze` runs | App can show "No problems found" |
|---|---|---|---|---|
| I | Red, Blue, Yellow | `src/UI/Modals/Gen1PokemonDetailsModal.cpp:72` | format checks, then `GB::checkGen1` (`Legality.cpp:229`) | yes |
| II | Gold, Silver, Crystal | `src/UI/Modals/Gen2PokemonDetailsModal.cpp:235` | `analyzeNativeGen2` -> `GB::checkGen2` (`Legality.cpp:962`, `:1031`) | yes |
| III | Ruby, Sapphire, Emerald, FireRed, LeafGreen | `src/UI/Gen3SharedPokemonSurface.inc:1587`, `src/UI/Modals/PokemonDetailsModal.cpp:65`, `src/UI/TrainerViewScreenBase.inc:3241` | older checks, then `Gen3Pk3::verify` + `applyGen3Analyzer` (`Legality.cpp:1124`, `:3255`) | yes |
| IV | Diamond, Pearl, Platinum, HeartGold, SoulSilver | `src/UI/Gen4SharedPokemonSurface.inc:1863` | older checks, then `applyGen4Analyzer` (`Legality.cpp:3256`) | yes |
| V | Black, White, Black 2, White 2 | **none** | `Legality::analyzePk5` exists as engine API only | **no**: this branch has no PK5 reader, and the Gen V panel on #307 does not call legality (see `GEN1_5_APP_INTEGRATION_PREP.md`) |

So four of five generations (15 of 20 titles) are production-wired today. Gen V's rules are
complete in the engine but reach the app only after the reviewed integration with #307.

### Rule areas per generation

Rows follow PKHeX's `LegalityAnalysis.UpdateChecks` order. **P** = runs in the app path;
**E** = engine only (not reachable from the app on this branch); **n/a** = the format has no
such field, or derives it from another field that is checked; **none** = no rule.

Severity follows PKHeX: Invalid where PKHeX reports Invalid, and a Warning where PKHeX reports
"Fishy", so the record stays Incomplete. The exceptions are the deliberate differences listed in
each generation's inventory and reviewed in `PKHEX_DISAGREEMENT_REVIEW.md`. Most are more
lenient. The two stricter ones (Gen I/II stored level against EXP, the Gen V name-buffer length) rest
on decomp evidence.

| PKHeX area | Gen I (PK1) | Gen II (PK2) | Gen III (PK3) | Gen IV (PK4) | Gen V (PK5) |
|---|---|---|---|---|---|
| Encounter match (wild, static, gift, trade, egg, event) | P | P | P | P (incl. Pokewalker, Ranch, PCD/PGT) | E |
| PID / IV correlation (RNG method) | n/a (DVs) | n/a (DVs) | P (Methods 1-4, H, BACD, Channel, Cute Charm) | P (Method 1/J/K, BACD, Cute Charm, Chain Shiny, Pokewalker) | E |
| Language | P (Japanese vs international tables) | P (incl. Korean rules) | P | P | E |
| Nickname | P (length) | P (length, egg names) | P (incl. word filter) | P (incl. word filter) | E (incl. trades and events) |
| Trainer name, gender | P (name) | P (name; gender in Crystal) | P | P | E |
| Trainer ID | none (PKHeX: TID 0 is only Fishy) | P (event and trade IDs, Headbutt trees) | P | P | E |
| IVs / DVs | P (DV/stat consistency) | P | P | P | E |
| EVs / stat experience | P | P | P | P | E |
| Level / EXP | P | P | P | P | E |
| Ribbons | n/a | n/a | P | P | E |
| Ability | n/a | n/a | P | P | E |
| Ball | n/a | n/a | P | P | E |
| Form | n/a | n/a (Unown from DVs) | n/a (Unown from PID) | P | E |
| Misc: egg, fateful, Pokerus, dates | n/a | P (egg, Pokerus) | P | P | E |
| Move PP / PP Ups | P | P | P | P | E |
| Gender | n/a | n/a (from DVs) | n/a (from PID) | P | E |
| Held item / catch rate | P (catch rate) | P | P | P | E |
| Contest stats | n/a | n/a | P | P | E |
| Markings, ground tile | n/a | n/a | P (markings) | P | E |
| Moves (level-up, TM/HM, tutor, egg, event, evolution order) | P (incl. Stadium 2 reminder) | P | P | P | E |
| Evolution | P | P | P | P | E |
| Transfer (Time Capsule, Pal Park, Poke Transfer) | P (Time Capsule) | P (Time Capsule, Crystal met data) | n/a | P (Pal Park, Gen III origin verified) | E (Gen V side only; original encounter in progress) |

Detail per rule, with PKHeX file references: `GB_LEGALITY_VERDICTS.md` (I, II),
`GEN3_PKHEX_RULE_INVENTORY.md`, `GEN4_PKHEX_RULE_INVENTORY.md`, `GEN5_PKHEX_RULE_INVENTORY.md`.

### What is not covered (stays Incomplete, never Invalid)

- Gen I/II: unrecorded GB-era distributions (Mew, Celebi), glitch encounters, Korean edge
  cases, Day Care slots (`GB_LEGALITY_VERDICTS.md`, "Not covered yet").
- Gen III/IV: Colosseum/XD origins (Orre tables not ported); PKHeX "Fishy" findings
  (bad RNG frame, PID 0, SID 0, all IVs equal, R/S TID without a seed); a checksum or
  sanity mismatch; Enigma Berry holders.
- Gen V: Gen III/IV-origin transfers (original encounter not yet checked from the PK5 side);
  fateful records matching no supported PGF card; the text cases in the Gen V inventory's
  "Still Incomplete".

## 2. Corpus detection rates (PKHeX differential)

Each tool generates every encounter PKHeX can build for the games, adds seeded single-field
mutations (and, for Gen III/IV, a "grown" corpus of PKHeX-valid records played forward),
and compares verdicts. **False Invalid** = PKHeX valid, PokeBank NX Invalid. **False Valid** =
PKHeX invalid, PokeBank NX "No problems found". Every tool fails on either.

| Gen | Corpus | False Invalid | False Valid | PKHeX-invalid caught as Invalid | PKHeX-valid certified "No problems found" |
|---|---|---|---|---|---|
| I | `pkhex_gb_diff`, 3 x 50,000; app-path parity PK1 20,000 / 20,000 | 0 | 0 | matches PKHeX except the listed differences | yes, except the listed Incomplete cases |
| II | same, PK2; app-path parity 18,291 / 18,291 (1,709 Korean skipped) | 0 | 0 | same | same |
| III | `pkhex_gen34_diff` seed 1, 3 mutations (28,836 records) | 0 | 0 | 14,321 / 14,325 (99.97%; the 4 are Enigma Berry) | base 13,935 / 14,511 (96.0%); grown 18,900 / 19,241 (98.2%) |
| IV | `pkhex_gen34_diff` seeds 1-3, 3 mutations | 0 | 0 | 83,524 / 83,524 (100%) | base 100%; grown 114,408 / 115,268 (99.25%) |
| V | `pkhex_gen5_diff` seeds 1, 2, 7, 11, 4 mutations (461,340 records) | 0 | 0 | 93.4% | native 117,432 / 117,521; trade 8,579 / 8,579; event 16,061 / 16,074; Gen III/IV transfer 0 / 84,961 |

Before this round, Gen III caught 13-14% and Gen IV 31-32% of PKHeX-invalid records, and
neither could certify any record.

## 3. Test counts (host suites, this head)

| Suite | Cases |
|---|---|
| `test_gb_legality_gen1` | 198 checks vs PKHeX verdicts (15 deliberately Incomplete) |
| `test_gb_legality_gen2` | 244 checks |
| `test_gb_legality_structural` | 1,511 checks |
| `test_legality_gen3` | 268 cases + 12,000 fuzz records |
| `test_legality_gen4` | 198 cases + 40,000 fuzz records (400,000 in the gen34 CI job) |
| `test_legality_gen5` | 121 cases + 40,000 fuzz records |
| `test_legality_title_matrix` | 118 cases (Gen I, II, III and V titles; Gen IV rows in progress) |

Each also builds with ASan/UBSan, and GCC and Clang where the lane gates require it. The
lane gates (L1-L6) and their workflows are listed in `LEGALITY_GATE_MATRIX.md`.
