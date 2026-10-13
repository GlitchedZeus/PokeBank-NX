# L4 Stage 3 — Gen IV PCD/PGT gift history, source-local only

Scope: D/P/Pt/HGSS source candidate evidence. This tranche adds no retail event archive, no presumed distributions, and no global severity verdict.

## Mechanic anchors reviewed on 2026-10-12

- Original source format definitions, read-only reference: [PKHeX `PGT.cs`](https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/MysteryGifts/PGT.cs), [PKHeX `PCD.cs`](https://github.com/kwsch/PKHeX/blob/master/PKHeX.Core/MysteryGifts/PCD.cs). Reference only; no source copied. PGT=0x104, PCD=0x358, PCD embeds PGT at offset 0, card ID at 0x150, card-compatibility raw ushort at 0x14c. Standalone PGT has no card ID; pairing ignores PGT linked-slot byte 0x02 only. Lock Capsule is Item type with item ID 533 and PCD cannot be losslessly represented as PGT. PGT has gift type values 0..15. In the original implementation only ordinary Pokémon and PokémonEgg gift data's PGT encryption is positively covered by the generic PGT writing path; leave Manaphy and movie encryption state unasserted.
- The upstream format's `GiftType4` distinguishes Pokémon/egg/movie gift, Ranger Manaphy egg, item/goods, unlocking item types and special services. A PCD card need not represent a distributed Pokémon. PCD metadata, PGT matching and card identity are structural witnesses, not retail-distribution proof.
- Previously accepted L4 original witnesses: `Gen4EventArchiveAttestation`, `Gen4EventRangerGiftHistory`, `Gen4EventNativeFieldEvidence`, and 247-entry `Gen4EventTemplateAudit`. Do not infer 247 authentic archives from the generated row list, and do not substitute a reused card ID for a unique source ID.

## Engineering contracts

1. `Gen4MysteryGiftContainerEvidence.h`: strict bounded 260/856-byte original PGT/PCD structural projection; 15 nonempty gift types; no reading beyond passed `span`; PCD-only card ID; PCD↔PGT equality except slot byte; raw 0x14c compatibility retained **uninterpreted**; item service and Ranger distinction; empty/unknown/overlong rejection. Independently submitted archive grade, digest, original payload extraction, release and individual receipt remain separate gates. `ConditionalOriginalSource` is not historical authentication.
2. `Gen4MysteryGiftHistoryEvidence.h`: supplied historical source and independently verified field-change histories compared against final valid PK4 fields. Species, OT TID/SID, language, version, egg state, Ball, four moves, persistent ribbon masks, raw OT/nickname, fateful uncertainty. Distinguishes confirmed single-candidate contradictions from missing, evolved, traded, renamed or hatched history; no global Invalid. Ranger does not become conditional without its separately verified Ranger history.
3. `Gen4MysteryGiftNativeEvidence.h`: read-only native adapter wraps original `Pokemon4ReadOnly` and `Gen4EventNativeFieldEvidence`; rejects corrupted/empty native before any comparison; refuses caller-supplied final evidence; Ranger specifically delegates to `Gen4EventRangerGiftHistory::evaluateNative` to prevent false proof supplied by a caller.

## Executable regressions

- `test_legality_l4_gen4_pcd_pgt_container.cpp`: all 15 nonempty delivery types, all 259 independently flipped inner-gift byte positions × 8 bits = 31,080 pair negatives, 448 archive grade/proof combinations, zero-slot/card ID alias/Lock Capsule/size/unknown type/Manaphy unlock cases.
- `test_legality_l4_gen4_mystery_gift_histories.cpp`: 65,536 original-game/history combinations, 11 candidate-negative domains and fateful Unknown, evolution/moves/trainer/egg mutation history regressions, missing archive and source/recipient guards.
- `test_legality_l4_gen4_mystery_gift_native.cpp`: synthetic native **encrypted** PK4 tested via original host read-only PK4 parser and encryption sources; PCD card, PGT pair, independent field gates, wrong OT, bad checksum, copied source immutability and adversarial Ranger assertion. Not an original distribution fixture.

## Limits / coordinator instructions

No independently verified historical original PCD/PGT payload digest, release receipt, per-language/per-country distribution period, anti-shiny RNG seed, original recipient and right to redistribute was supplied. Synthetic `Attestation` flags are conditional proof-interface test inputs only, not source authentication. The original PCD `CardCompatibility` bitfield is deliberately not mapped to D/P/Pt/HGSS until a version-verified specification is sourced; do not invent exclusions. Original RNG, egg/hatch, tradeback, rename and mutable metadata may remain unresolved despite template match. For Stage 5 native validation the coordinator must run all new tests with exact original PK4 implementations, cross-check L5 original-source documentation and preserve Unknown/Incomplete. No shared CI, production `Legality.cpp`, accepted branch, L1/L2/L3/L5, PR #298, or MAIN edits in this stage.
