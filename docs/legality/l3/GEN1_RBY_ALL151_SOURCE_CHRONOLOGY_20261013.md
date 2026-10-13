# L3 Gen I-first: all-151 Red/Blue/Yellow original level-up and evolution chronology

## Delivery scope and original source

This is a **candidate-local, read-only move-history evaluator**, not a Gen I legality verdict and not an accepted production analyzer connection. Build on earlier `Gen12CandidateMoveJointChronology.h`, `Gen12TimeCapsuleEvidence.h`, `Gen1RbyOriginalJointHistory.h` and the native PK1 snapshot guard tests. **Do not treat an edited/detached PK1 as proof of a historical level-up choice, gift/trade, TM consumption or physical game visit.** Every positive path needs all independent evidence flags, still hypothetical in the tests.

Source pins:

- `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`, `data/pokemon/evos_moves.asm` blob `6a6704f98b8b075f2fd81c91d41a1dda0f2a047d` (Red/Blue); `constants/move_constants.asm`, `constants/pokedex_constants.asm` for the official original indexes
- `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382`, `data/pokemon/evos_moves.asm` blob `8bee626ae17d052a17562be9959a13b6102146f7` (Yellow)
- Existing narrower retail TM/HM witnesses in `Gen1RbyOriginalJointHistory.h`: the original `constants/item_constants.asm` and verified Pikachu, Raichu and Bulbasaur base-stats machine lists. Other species' machine acquisitions stay Unknown even if a broad modern compatibility union contains the move.
- `Gen12TimeCapsuleEvidence.h`: Gen II→I original native species ≤151, non-egg and move ≤165 restrictions. The new candidate evaluator only models an **attested terminal Gen I→II service boundary**; it does not create or guess a Gen II move history.

The compact constexpr catalog holds **728 Red/Blue level-up offers and 755 Yellow offers**, alongside **72 Gen I evolution records across 151 species**. `EVOLVE_LEVEL`, `EVOLVE_ITEM`, `EVOLVE_TRADE` have distinct replay rules, species/target/level checks, stone identity and independent receipt/visit evidence. Original row order is preserved for evolution selection; **move offers are stably sorted by numerical acquisition level**, retaining same-level order. In `pokeyellow`, Primeape's source has `db 46, THRASH` *before* `db 45, SCREECH`; naive raw file order would give an impossible chronological level 46→45 false rejection.

## Behavioral contracts

- Four contiguous, nonduplicate move slots; exact learning level and original game; independent deliberate learn, decline or already-known choices; full-slot replacements; no skipped known level offers. A missing choice/path remains Unknown, not Invalid.
- Original evolution between level-up offers follows the *actual source species*: e.g. Bulbasaur's level-20 PoisonPowder is **not** the same event as evolved Ivysaur's level-22 PoisonPowder. Stone evolution uses its own original item identity. Kadabra→Alakazam requires a separately authenticated inter-cartridge trade evolution.
- Original Red/Blue vs Yellow level-up and first-party special partner exception: Yellow's starter Pikachu cannot be casually asserted to Thunder Stone evolve. A source-attested non-partner Pikachu may evolve, with original stone and individual identity evidence.
- Original machine compatibility requires a per-species original source identity. An invented TM receipt (including a move that happens to be in the overall species learnset) cannot justify a positive path. Receipts for TM, stone, trade, terminal Capsule are nonzero and may not be reused.
- Preflight all step witnesses and supported later machine methods before considering an earlier local contradiction. A contradicted **candidate path** is never a global Pokémon Invalid verdict. A complete set of all original historical alternative acquisition routes has **not** been authenticated; caller must preserve Unknown for the Pokémon.
- Generation I has no original breeding, abilities, gender, nature, ribbons, native held item or event payload field. Nothing here invents those methods or overwrites original saves.

## Testing and verification

New C++ suites: `test_legality_l3_gen1_retail_catalog.cpp` scans the 151 species for both source versions and 72 source evolution records; `test_legality_l3_gen1_all_species_joint.cpp` performs positive/negative candidate chronology, trade/stone/level evolution, Red/Blue/Yellow changes, unsorted Yellow Primeape, Time Capsule original bound, Unknown source/evidence, and cross-path alternative controls. Existing L3 native PK1/PK2 snapshot suites remain in the full runner.

Source-audit script: `python3 tools/legality/verify_l3_gen1_retail_catalog.py --pokered /actual/pokered --pokeyellow /actual/pokeyellow` reads the **pinned Git commits and blob IDs**, deterministically reconstructs both catalogs and verifies every character of the committed packed tables. It requires these original Git checkouts. Direct `git` network cloning from this tool's local container was DNS-blocked; this original checkout parity command cannot be claimed executed without real source checkouts. A connected GitHub source comparison can verify source data independently.

### Still unimplemented; do not claim retail parity

- Full **151-species per-version TM/HM compatibility**; the existing original machine helper intentionally supports only bounded species/machine pairs. HM usability and Badges are trainer/custody context, not native move ownership evidence.
- Actual original gift, event, NPC trade, encounter-at-level, Stadium/Surfing Pikachu, Pay Day, trainer-owned original move, Catch/Transfer and field-service acquisition histories.
- Unknown pre-snapshot overwrites, unobserved in-game trainer decisions, original event distributions and historical RTC/Time Capsule return legs. Trade-evolution cancellation, where not source-proven, remains Unknown.
- Exhaustive alternative route enumeration across source/trade/evolution/gift histories; a single contradicted proposed path must never prove global Invalid.
- Actual PKHeX.Core C# host-oracle differential and native retail specimen history correlation. Unit fixtures declare synthetic proof flags, not authenticated source objects.
- Coordinator-only `Legality.cpp` integration, full L3+other-lane CI, sanitizer acceptance on integrated HEAD, Switch build and NRO field testing. No branch merge or save write requested.
