# Gen VI–IX legality extension inventory

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Existing analyzer boundary

#314 `1ceea5634d1bfc251087727aa9444ec0a1109305` is the active legality campaign; #301 `809a31841fc2ad43dda02d4412cb9891f29a482f` is a separate later-format foundation. Do not merge either into the other here. #301's native format evidence and #309–312 decoders are candidates for immutable adapter inputs. A checksum proves structural integrity only. Native context and original history require separate evidence, and missing event/transfer history stays **Unknown/Incomplete**, never globally Invalid.

## Rule inventory: PKHEX source navigation, implementation pending

All below refer to current pinned PKHeX, with exact hashes/URLs in the generated source index. A source location is not a completed NX implementation or licensed table import.

| Rule group | Primary source family / symbol | Required evidence and boundary |
|---|---|---|
| Wild/static | `Legality/Encounters/Generator/EncounterGenerator6/7/7b/8/8b/8a/9/9a` and matching Data/Templates | exact game, revision, location, level, form, encounter RNG; no global invalid from unmatched incomplete database |
| Gifts/trades/events | encounter trade/static/gift templates, `MysteryGift`, event verifiers | distribution dates/languages/OT/IDs/ball/moves; absence of event archive is incomplete coverage |
| Transfers | `Legality/Verifiers/TransferVerifier`, HOME game-data conversion and native PKM converter | route + native origin + visited games + HOME tracker; label alone is not history |
| Breeding/eggs | `Legality/Breeding`, `EggStateLegality`, learn sources | parent/egg moves, hatch location, game-native availability; LGPE/PLA/ZA require distinct support decisions |
| Move chronology | `Legality/LearnSource`, move verifiers and move result/evolution chains | four-slot joint chronology, relearn/TM/TR/tutor/purchased/mastered/Plus flags; union of individual legal moves insufficient |
| Abilities | `AbilityVerifier`, PersonalTable family/form | slot/hidden ability, capsule/patch direction and historical eligibility |
| PID/IV/RNG | encounter RNG generators and PID/IV verifiers | modern gifts/raids/overworld correlation; no old universal PID=nature assumption |
| Hyper Training | `HyperTrainingVerifier` and native IHyperTrain | effective stats separate from IVs; eligible title/level/history |
| Mints/nature | nature/stat alignment verifiers, PK8/9/PA8/PA9 | original genetic nature versus effective nature; family-native limitations |
| Shiny | encounter shiny policy and RNG checks | locks, shiny type and origin; toggling flag/PID is not legalization |
| Balls | BallVerifier + encounter/egg ball rules | origin and historical availability; no all-balls list |
| Forms/evolution | FormVerifier, `Legality/Evolutions`, personal forms | battle-only forms, fusion/partner/noble/alpha/mega state, destination form compatibility |
| Ribbons/marks | Ribbon/Mark verifiers + native interfaces | encounter/visit/distribution conditions; count ranges alone insufficient |
| Trainer/OT | TrainerName/TrainerID/Language/Memory verifiers | encoding, gender, OT, handler memories, country/region fields by family |
| Gen VII | Z-move/Hyper Training/encounter and transfer checks | SM versus USUM; signatures are save integrity, not Pokémon legality |
| LGPE | GG encounters, PB7, Awakening values/CP | partner immovability, no EV reinterpretation, storage references |
| SWSH | encounter raid/overworld/egg checks, Dynamax/Gigantamax/TR | DLC presence, correlated raid parameters, permitted Gmax forms |
| BDSP | BDSP encounters/breeding, contest/seal state | remake-native pools, special HOME transfer restrictions |
| PLA | LA encounters, Ganbaru/mastery and alpha/noble checks | move-shop flags, effort levels, alpha size, research-driven availability |
| SV | Gen9 encounter/tera/raid verifiers | Tera original/override, scale/marks, raid RNG, DLC-native moves |
| ZA | Gen9a encounters, `MiscVerifierPA9`, Plus flags/forms | no inherited SV tera mechanics; native shiny/evolution restrictions |

## Official routes versus historical assumptions

Current official HOME documentation confirms conditional compatibility with LGPE/SWSH/BDSP/PLA/SV/ZA, one-way Bank→HOME, and LGPE return restrictions after visiting another game: [HOME movement rules](https://home.pokemon.com/en-ca/move/). Z-A visiting prevents any older-game return: [official Z-A notice](https://legends.pokemon.com/en-us/news/pokemon-home). A Pokémon must exist in its destination game. These source-backed restrictions are represented in `metadata/transfer-policy.json`; this graph is intentionally not a transfer engine.

Historical Gen V→Transporter→Bank, Gen VI↔Bank and Gen VII↔Bank must be added as separately attested routes with one-way Gen VII return constraints and species/form/item restrictions. No direct Gen VI→Switch or SV→older-generation edge is asserted. Current service availability and installed Bank/Transporter prerequisites were not exercised; mark them Unknown. Official documentation establishes allowed service behavior, not proof an individual record took a route.

## Interfaces to extend after app acceptance: INFERRED

Feed analyzer `NativeRecordEvidence{context, exactGame, saveRevision, checksumVerified, fields}`, `SourceIdentityEvidence`, `HistoryEvidence`, `DataRevision`, and per-rule `Coverage{complete, missingReason, sourcePins}`. Keep structure, native-domain, acquisition, move chronology, transfer and event coverage separable. Preserve existing verdict aggregation: only definite contradictions within verified coverage can cause Invalid; unexplained or unsupported history cannot be promoted to Valid.

No giant new analyzer was built. Required next assets are small generated availability tables, encounter-origin indexes and conditional transfer predicates, reviewed one family at a time. Differential PKHeX comparisons need deliberate divergence records and authenticated originals rather than treating every oracle verdict as truth. Current #314 Gen V differential is failing even though its unit/fuzzer steps pass; see TEST_RESULTS.
