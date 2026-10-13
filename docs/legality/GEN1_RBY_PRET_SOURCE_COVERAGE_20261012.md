# Gen I Red/Blue/Yellow — pret-pinned original encounter expansion
Updated: 2026-10-12. **L1-owned, read-only, draft, no production runtime merge.**

## Original source revisions
- `pret/pokered@af519899719f0754965776faac0e836a3b906e6d` — International Red and Blue build conditionals (`IF DEF(_RED)`, `IF DEF(_BLUE)`), **58 original grass/water map files**, both physical Route 19/20 entries from the real `SeaRoutes.asm` common label, original Red/Blue Super Rod source remains available in prior 149-slot archive.
- `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382` — **59 Yellow grass/water map files** (including separately authored Route 19/20), its own **124 Super Rod physical slots across 31 location rows**.
- Both source trees' `constants/pokedex_constants.asm` are the National Dex mapping used to interpret original ASM species names. Do not mistake Gen I's internal species index for the National Dex ID.
- Per-file exact Git blobs accompany every generated `Gen1PretWildRB*.inc` and `Gen1PretWildYellow*.inc`. Source text is not copied from a mutable `master` target; the immutable commit is stated.
- Red/Blue/Yellow `data/events/trades.asm` retains **10 native ROM NPC offers per codebase** including **four ROM entries explicitly unused**. The original request, offered species, scene index and source game are separate identities, not a final species lookup.
- Original `data/wild/good_rod.asm` two choices, both original `engine/items/item_effects.asm` Old Rod Magikarp level 5, and Yellow `data/wild/super_rod.asm` are independently pinned.
- Overworld original source scenes derive from game scripts and object lists: Routes 12/16 Snorlax, Power Plant Zapdos, Cerulean Cave Mewtwo, Seafoam Articuno, Victory Road Moltres, Silph Lapras, Fighting Dojo choices, Celadon Eevee, Oak's R/B starters and Yellow Pikachu, Yellow Bulbasaur/Charmander/Squirtle gifts, and three fossil forms with a distinct original fossil-receipt gate.

## Physical source coverage

| Native source category | New source rows | Notes |
|---|---:|---|
| Red/Blue grass and surf | 755 | Original slot ordinals, exact map, method and encounter rate; conditional Red versus Blue sources remain separate |
| Yellow grass and surf | 630 | Independent Yellow physical map/slot identity |
| Yellow Super Rod | 124 | 31 source map rows × 4 exact slots |
| R/B/Y Old Rod and Good Rod | 3 | Game-global, source map fishability **Unknown** |
| **Total new physical source rows** | **1,512** | **1,923 unique game-specific source-slot identities** after expanding shared Red/Blue masks |
| Previously verified Gen I slots | 149 | **Unmodified**, in separate index namespace |
| Source-pinned static/gift scenes | 20 | Includes game-specific choices and fossil receipt requirement; not all original static events |
| Source-pinned NPC trade offers | 20 | 4 ROM-unused entries produce Unknown, not eligible positives |

## API and safety contract

- `Gen1EncounterEvidence.h` retains its existing PKHeX-derived `kStaticEntries` 31/`kTradeEntries` 17; adds `kPretTradeOffers` and `kPretStaticSources` with original ROM table indices and scene identity.
- `Gen1OriginalWildFishingStage.h` leaves `kVerifiedSlots` and `evaluate()` unchanged; `pretSlot(index)` and `evaluatePret()` use a **separate** 1,512-row index namespace. No missing or foreign row is a global Invalid determination.
- `Gen1IndexedOriginalSourceHistory.h` adds `evaluatePretStatic()` and `evaluatePretNpcTrade()`, honoring source game, original scene, original map, species, level, NPC request, OT and original transaction, and separate revived fossil receipt. Conflicting source rows are **candidate-local negatives** only. Unknown historical identity is **Unknown**.
- In Gen I, international Red and Blue are the only `pokered` build variants represented; Japanese Blue is deliberately excluded. Time Capsule tradeback, mutation of the Gen I catch-rate byte, and evolution in later generations are not ruled out.
- Grass/water encounter rate is retained; exact RNG/pseudo-random encounter selection and catch-rate legality **are not** inferred from slot compatibility.

## Test and acceptance gates

- `test_legality_l1_gen1_pret_slots.cpp`: every new physical row across R/B/Y titles, retained legacy 149-row ABI, map/method/slot, candidate negatives, missing witnesses, 256-value method corruption, Yellow rod and Red/Blue Viridian Forest distinctions, Route 19/20 aliases.
- `test_legality_l1_gen1_pret_statics_trades.cpp`: all pinned static/gift scenes and ROM offers, game exclusivity, original NPC requested species, original OT and trade receipt, unreachable ROM entries, independent fossil-revival receipt, and malformed row indices.
- `tests/test_legality_l1_run.sh` lists **50 unique native C++ programs** and the L1-owned exact SHA/tree workflow requires host + ASan/UBSan (100 individual program executions).
- **Source inspection executed:** all generated table rows total 1,512; 1,923 unique game/slot keys; no duplicate physical keys; all National Dex IDs 1–151 and levels 1–100; nine source includes and twenty native NPC offers plus twenty scene rows checked against commit content. No full checkout is locally available.
- **Local native smoke executed:** GCC 14 and Clang 17 host and ASan/UBSan, each 1,312,856 scenario assertions PASS. The smoke is an **independent reproduction of representative source rows/status semantics**, *not byte-for-byte the newly committed 50-suite native regression*.
- The full local runner was attempted but failed **before compilation** because `/mnt/data/tests/test_legality_l1_run.sh` is absent; Git remote checkout has failed DNS resolution. Github Actions exact-head 50-suite host/sanitize + host CI are queued at handoff. **Acceptance BLOCKED.**

## Remaining Gen I work before claiming complete retail Gen I encounter legality

1. Source-pin and enumerate remaining original gifts, Game Corner prizes, in-world statics and event-conditioned scenes beyond the twenty explicit examples. Exact one-time event flags and original scene/receipt history are not currently exhaustive.
2. Validate physical catch RNG/encounter slot weighting and source-game version, original caught catch-rate/DVs with authentic PK1 samples; source-compatible slot does not certify actual catch.
3. Verify Good/Old Rod location usability and fishing animation conditions from exact map tilesets; game-global source table alone cannot certify specific location.
4. Complete complete original NPC trade OT/IV/nickname/level inheritance, original source transaction proofs and legitimate time-capsule tradeback. ROM-unused trade entries remain Unknown.
5. Run the **actual full checkout** 50-suite matrix under normal + ASan/UBSan, host/RSE regression and representative genuine Red/Blue/Yellow original saves, fix any exact-head failures. Acceptance may only move when gates genuinely pass.

**Priority control:** no work in Gen II+, Gen V/VI, MAIN, coordinator or sibling legality lane was performed in this tranche. Do not merge PR #293 until an explicit separate coordinator acceptance decision.


## Next L1 original-acquisition hardening — Game Corner / source levels / ancestry

Source-pinned **Game Corner redemption** has 18 independent physical vendor/slot records (six original Red, six Blue, six Yellow), preserving species, **exact original prize level**, **exact original coin cost**, original game, vendor and per-vendor slot. `Gen1PretGameCorner.inc` is derived from pret Red/Blue and Yellow `data/events/prizes.asm`, `data/events/prize_mon_levels.asm`, and actual `engine/events/prize_menu.asm`. `Gen1PretAcquisitionStage.h` requires separately witnessed original purchase and payment evidence, never infers purchase from a matching specimen. Without it the result is Unknown; conflicting price/species/level is **candidate-local only**.

**Original NPC trade level:** Original `engine/events/in_game_trades.asm` copies the player-donated specimen's level into `wCurEnemyLevel` and creates the received NPC specimen at that same level. The ROM offer table provides **no single static trade level**. `Gen1IndexedOriginalSourceHistory.h` now checks original submitted and received **acquisition-stage levels** when independently attested. Absent original receipt/levels are Unknown; mismatching witnessed levels are candidate-local contradiction only. The 20 source trades, four unreachable offers and existing earlier index namespaces are preserved.

**Fossil receipt identity:** Source-pinned Cinnabar fossil outcomes are **Helix → Omanyte #138**, **Dome → Kabuto #140**, **Old Amber → Aerodactyl #142**. The evaluator requires the separately attested physical fossil item, not merely a true/false history flag; a different witnessed original fossil is a candidate-local mismatch, and no fossil witness is Unknown.

**Rod action history:** Exact original Red/Blue and Yellow `engine/items/item_effects.asm` performs `FishingInit` before Old/Good/Super Rod results. `evaluatePretFishingInit` adds an optional original action witness: absence of original water/shore action evidence is Unknown, and independently witnessed `FishingInit` failure contradicts **that specific rod candidate only**. This does not invent fishable maps, RNG encounters or global Invalid.

**Yellow Oak starter special byte:** Original Yellow `scripts/OaksLab.asm` writes `LIGHT_BALL_GSC` into the Pikachu starter's PK1 catch-rate byte, and original `constants/item_constants.asm` defines it as **`$A3`**. That is a historical acquisition-specific byte, **not a native Gen I held-item feature**. `evaluateYellowStarterByte` requires independently witnessed original Yellow Oak/level-5 Pikachu and original catch byte. It does not treat a later Time Capsule mutation as necessarily Invalid.

**Original evolutionary ancestry graph:** Red/Blue and Yellow original `data/pokemon/evos_moves.asm` both yield **72 exact transitions**: 52 level, 16 item, 4 trade. Species are mapped via the pinned `data/pokemon/dex_order.asm` and `constants/pokedex_constants.asm` rather than raw internal Gen I index. These transitions are shared by R/B/Y. `Gen1PretEvolutionStage.h` uses exact edges for one or two hops, including Eevee branches; `Gen1EncounterEvidence::canDescendFrom` and `Gen1IndexedOriginalSourceHistory::possibleOriginalEvolution` now reuse this graph. This removes false possible ancestors caused by contiguous National Dex number intervals; it is **not** a claim of completed item/trade/evolution history and never turns a failed candidate into whole-Pokémon Invalid. A later-generation evolution remains Unknown.

### New actual independent test execution

From exact GitHub-blob-verified six files, the **actual committed** C++20 source and tests were compiled and executed:
- `tests/test_legality_l1_gen1_pret_prizes.cpp`: GCC 14.2 and Clang 17, normal `-O2` and ASan/UBSan `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`, **4/4 PASS**, **82 source-compatible, 309 local candidate-negative, 66,700 Unknown/bounds assertions per build**, including all 256 historical Yellow starter bytes.
- `tests/test_legality_l1_gen1_pret_evolution.cpp`: same four profiles, **4/4 PASS**, **2799 compatible, 203,058 local candidate-negative, 68,207 Unknown/bounds assertions per build**, covering all 151×151 source/current species pairs per R/B/Y.
- Each of the six C++ sources/included tables/tests independently matched the GitHub Git blob SHA-1 before compilation; this is not a synthetic stand-in.
- `tests/test_legality_l1_run.sh` is configured for **53 distinct suites** × host/sanitize = **106 required repository executions**. Attempted locally from a partial mirror: fails before tests because the actual full checkout and runner file are unavailable. `git ls-remote` still fails `Could not resolve host: github.com`. No claims that unexecuted 106 tests passed. GH exact-head CI and full host runs must be checked and accepted separately.

**Open gaps:** full Gen I original script event flag coverage and source acquisition records, genuine original save evidence, all historical fishing action context, Game Corner and NPC transaction receipts, Original-PKHeX.Core host differential encounter behavior and source-specific encounter RNG remain incomplete. Source graph membership, exact Game Corner offers, and byte compatibility are **narrow conditional checks** only.


## Major follow-on: scripted gift gates and native random physical-slot selection (2026-10-13)

**Pinned retail source:** same immutable `pret/pokered@af519899719f0754965776faac0e836a3b906e6d` and `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`; no unverified modern event database promoted to original-game proof.

### Exact one-time scripted gifts

`include/Legality/Gen1PretGiftEventStage.h` supplies five acquisition scenes in a separate L1 original scene namespace. These supplement rather than replace the existing twenty static/gift records:

| Original acquisition scene | Game source | Native record | Distinguishing original acquisition condition |
|---|---|---|---|
| Mt. Moon Pokémon Center salesman | R/B/Y | Magikarp #129, level 5 | BCD `500` purchase, `EVENT_BOUGHT_MAGIKARP`, successful `GivePokemon` |
| Celadon Mansion rooftop | R/B/Y | Eevee #133, level 25 | Original one-time Poké Ball object hidden after a successful gift |
| Cerulean Melanie | Yellow | Bulbasaur #1, level 10 | Pikachu friendship `wPikachuHappiness >= 147`, `EVENT_GOT_BULBASAUR_IN_CERULEAN` |
| Route 24 Damian | Yellow | Charmander #4, level 10 | One-time gift `EVENT_54F` |
| Vermilion Officer Jenny | Yellow | Squirtle #7, level 10 | Thunder Badge bit at acquisition, `EVENT_GOT_SQUIRTLE_FROM_OFFICER_JENNY` |

The original (R/B) Magikarp source is `scripts/MtMoonPokecenter.asm` Git blob `da3d3bc5fbd07130f6a6c4da686b3e0eaa024b93`. The original Yellow script is `scripts/MtMoonPokecenter_2.asm` Git blob `ba5650a235b40c8c9bd41afba72448b3a7b465a3`. Yellow's Bulbasaur, Charmander and Squirtle scripts are respectively `8b8095e012a6f6e38e2313907228a966ecd11c31`, `8e2931ba5cde850e53a3e7a7e265ff15462fc724` and `c6cd9478558dc212e88bb4a6a5339eb4ff154237`.

Original flag states, 500 Pokédollar payment, Pikachu happiness and Thunder Badge are **acquisition-time** witnesses. A modern save containing them is NOT proof they existed at original acquisition. Every mismatch applies to the singled-out route, and missing historical prerequisites/receipt return Unknown rather than global Invalid.

### Exact ten-slot Gen I wild probability and version-specific original half-block semantics

`include/Legality/Gen1PretWildRollStage.h` is a read-only independent witness for original hRandomAdd (rate gate), original hRandomSub (slot selector), independently observed original tile, movement and repel context. Both version families' original `data/wild/probabilities.asm` are byte-identical Git blob `6896d93704230993297625f23837631b7da4adf0`; the physical-slot **out-of-256** distribution is:

| Original physical slot | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Weight / 256 | 51 | 51 | 39 | 25 | 25 | 25 | 13 | 13 | 11 | 3 |

Original Red/Blue `engine/battle/wild_encounters.asm` blob `b5cf9f518e9b03ce6122c1c8eea2193d6f500be0` compares bottom-right half-block tile (9,9) to grass/water for **encounter triggering**. Original Yellow corresponding blob `9fe0c96c16ee897608709eeb6ce006058af4e6f8` uses **bottom-left (8,9)**. Both select the physical grass versus water Pokémon table by **bottom-left**, explaining original R/B east-shore behavior that Yellow avoids. Indoor non-FOREST fallback, encounter-rate byte, original repel and party-lead-level constraints are also modeled at the candidate stage.

`Gen1OriginalWildFishingStage.h` now exports `pretWildSlotWeight(index)` and the `evaluatePretWildRoll` join. It checks the prior 1,512-row physical source identity before comparing the original roll and tile witnesses, preserving all legacy 149 source indices. The encounter odds **do not apply to rods**; three global rods and 124 Yellow Super Rod rows are kept separate. No 8-bit original RNG state is reconstructed from present-day Pokémon fields.

**Independent source-data audit executed** on all nine current generated GitHub files: 1,275 Grass rows, 110 Surf rows, and 127 rod rows. Exactly **179 physical version/map/method grass-water tables** (58 Red, 58 Blue, 63 Yellow) have ten unique slots each, with an exact probability sum of **256/256 per table**, zero collisions. `test_legality_l1_gen1_pret_slot_weights.cpp` formalizes this as a new C++ regression for exact-head CI; this particular new test is NOT locally compiled, since its 1,512-row original checkout is unavailable.

### Actually executed tests

The current exact GitHub Git blobs for both NEW independently compiled C++ suites were mounted byte-for-byte in a local source mirror and confirmed with `git hash-object`, then run with GCC 14.2 and Clang 17, host `-O2 -Wall -Wextra -Wpedantic -Werror` and `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` (ASan/UBSan):
- `test_legality_l1_gen1_pret_gift_event.cpp`: **4/4 PASS**, each run prints **136 conditional, 202 candidate-local and 65,604 missing-history/selector scenarios**. This includes all 256 original Pikachu happiness values and 65,536 scene-selection adversarial combinations.
- `test_legality_l1_gen1_pret_wild_roll.cpp`: **4/4 PASS**, each run prints **4,048 positive, 2,117 candidate-local and 68,609 Unknown/selector scenarios**. It covers all 256 original slot rolls in R/B/Y, left-shore versus Yellow, indoor non-FOREST, witnessed Repel, and 65,536 malformed game/RNG combinations.
- Also **reran** 8 earlier unchanged exact-blob native Game Corner and 72-edge evolution binaries under normal and ASan/UBSan (GCC/Clang) with all PASS. This counts as eight new compiled/regression runs + eight previously compiled regression reruns; do not represent the latter as new compilation.
- Full native matrix is now **56 distinct L1 suites × host + ASan/UBSan = 112 exact-head executions required**, plus shared host/RSE gates. `bash tests/test_legality_l1_run.sh` attempted; no local full checkout so runner absent, and external `git ls-remote` still reports GitHub DNS failure. Full acceptance **BLOCKED** pending exact-head CI or real full repository checkout.

**Remaining scope:** real acquisition-time original saves, complete one-time events/gift conditions, retail RNG seed/frame reconstruction, historical fishing eligible-map proofs and item/event originals, authentic PKHeX.Core differential oracle. Original stage matches are intentionally conditional; no broad Legal or Invalid claims.
