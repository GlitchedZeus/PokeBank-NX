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
