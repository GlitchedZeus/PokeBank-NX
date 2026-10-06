# Gen I-IV PKHeX parity ledger

This ledger tracks **accepted production legality behavior** on `feature/legality-engine-gen1-4-20260929` against the pinned PKHeX reference commit:

- PKHeX: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`
- PokeBank NX accepted baseline when this ledger was created: `90816ca57ffa918359b01e7f8cc994c8e57af6a1`

It is intentionally conservative. A similarly named source file is not enough to claim parity: `Implemented` requires production-path behavior and regression evidence. `Partial` means useful source-backed checking exists but material PKHeX behavior remains unreconstructed. `Missing` means no accepted production implementation was verified. `Needs source verification` means the current comparison is not yet strong enough to classify safely.

The engine's semantic safety rule remains unchanged: **unknown, unsupported, or unreconstructable history is unresolved / Incomplete, not Invalid.** `Invalid` requires source-backed evidence that the represented Pokémon is impossible.

## Status ledger

| Area | Status | Accepted PokeBank evidence | Verified parity gap / next work |
| --- | --- | --- | --- |
| 1. Structural Pokémon validity | Partial | Production `Legality.cpp` performs generation/profile-aware field checks and structured findings. Existing engine has exact-game profiles and generation ceilings. | PKHeX has a wider verifier surface for structural/format-specific invariants. Continue verifier-by-verifier comparison before claiming full structure parity. |
| 2. Species / forms | Partial | Generation species ceilings plus Gen IV `Gen4FormEvidence` and source-aware encounter matching are in production. | Form availability and source/history interactions are not yet proven exhaustive across Gen I-IV. |
| 3. Moves | Partial | Exact game/context move legality, Gen III/IV egg-move evidence, and generation-correct move ceilings are accepted. | Historical move acquisition/retention through evolution, trade and transfer is not yet exhaustive. |
| 4. Abilities | Partial | Generation-aware ability checks exist and Gen IV lead mechanics model ability-driven encounter effects such as Pressure, Hustle, Vital Spirit, Static and Magnet Pull. | Source/evolution/history ability legality is not yet exhaustive; Gen I-II are not applicable to native ability legality. |
| 5. Natures | Partial | Gen III/IV PID/RNG reconstruction checks nature-bearing PID histories in multiple source families. | Remaining Method J/K rejection/retry histories and special source mechanics prevent parity. Gen I-II native nature legality is not applicable. |
| 6. PID / IV correlation | Partial | Accepted Gen III handheld Method 1/2/4, roamers, GameCube/source families, and substantial Gen IV Method J/K, event and special RNG evidence. | Known unsupported lead/rejection/retry histories remain, so unmatched reconstructable classes must not be over-classified Invalid. |
| 7. Wild encounters | Partial | Gen I catch-rate/encounter data, Gen II wild data, significant Gen III provenance, and Gen IV wild datasets plus RNG/source matching are accepted. | Obscure encounter categories and additional source-specific exceptions remain. |
| 8. Static / gift encounters | Partial | Gen II static/trade evidence, Gen III fixed/GameCube/event sources, and Gen IV static/trade/gift/event evidence are accepted. | Hatched/evolved gift histories and remaining special gifts/statics are not exhaustive. |
| 9. Eggs | Partial | `Gen34EggState`, `Gen34EggMoveEvidence`, Gen III Wondercard/event egg evidence, and Gen IV hatch-location/origin evidence are present in accepted production paths. | Full egg-origin, breeding-parent, inherited-move and post-hatch provenance reconstruction is incomplete. |
| 10. Event Pokémon | Partial | Multiple pinned Gen III event templates and generated Gen IV WC4/PCD template evidence are accepted; production reporting explicitly marks surviving invariants vs unresolved history. | OT text, ribbons, dates, trash bytes, evolved event-source histories and remaining event edge cases are incomplete. |
| 11. Evolution history | Partial | Wurmple evolution evidence, Gen IV static evolution evidence, and some evolution-line event matching are accepted. | Deep chains, pre-evolution provenance, egg/evolution interactions and source-game move history are incomplete. |
| 12. Transfer / history | Partial | Gen I/II Time Capsule evidence and Gen IV transfer evidence (including Pal Park stored-field classification) exist in production. | Full tradeback, Pal Park, cross-game transformations, move retention/loss and historical field transitions are not exhaustive. |
| 13. Trainer / OT metadata | Partial | Source-specific trainer rules exist (for example Colosseum/XD OT gender and fixed/event trainer identities). | Comprehensive OT/TID/SID relationships, trainer gender/identity restrictions and trade-history effects remain. |
| 14. Language / text / trash bytes | Partial | Production currently rejects language ID 6 and broad out-of-range values; event/source matchers consume language and some fixed-source text evidence. | **Verified gap:** accepted production permits Gen III language IDs 8-10 and Gen IV 9-10, while pinned PKHeX caps Gen III at Spanish (7), Gen IV at Korean (8), rejects unused ID 6, and treats 0 as unobtainable except a Gen V-only exception. Text encoding/trash-byte history remains much broader than language ID validation. |
| 15. Ribbons | Partial | Specific Gen III egg/event ribbon evidence exists and event templates preserve some ribbon-related provenance. | No verified comprehensive acquisition-order / impossible-combination / game-specific ribbon provenance checker yet. |
| 16. Met data | Partial | Exact encounter matchers, Gen IV origin/release/hatch evidence, static/event templates and transfer classifiers validate substantial met-location/level combinations. | Full met-data combination and historical transformation parity remains incomplete. |
| 17. Balls | Partial | Production has a basic ball range check and source/template-specific ball matching for multiple Gen III/IV event/static/shadow sources. | PKHeX has a dedicated ball verifier / use-legality layer; no equivalent general Gen I-IV capture-ball legality surface is yet verified in PokeBank. |
| 18. Held-item / history constraints | Partial | Production resolves held-item IDs in the Pokémon's source item namespace and warns on unknown IDs. | Legal held-item sets, source-game restrictions and historical item transitions are not yet reconstructed to PKHeX parity. |
| 19. Lead / RNG mechanics | Partial | Extensive Gen IV Method J/K lead evidence is accepted: BCC rerolls, Pressure/Hustle/Vital Spirit, Static/Magnet Pull, Synchronize success/failure subsets, Safari/Suction Cups and central reporting. | Mixed failed+successful Synchronize retry chains, failed-Synchronize nature-lock transitions, PID nature-rejection loops and other unsupported histories remain. |
| 20. Game-specific exceptions | Partial | Exact-game profiles plus many Gen I-IV special-source modules (Time Capsule, e-Reader, GameCube, event, Pokéwalker, Ranger Manaphy, Safari/BCC, etc.) are accepted. | PKHeX contains additional game/version-specific exceptions that still require source-by-source comparison and permanent vectors. |
| 21. Verdict / reporting behavior | Implemented | Accepted engine has structured findings, `Invalid`, `No Problems Found`, `Incomplete`, explicit coverage tracking, a production report bridge and lead-history descriptions. | This status covers the reporting contract only, **not** parity of every verifier feeding it. New unsupported history must continue to resolve conservatively as Incomplete. |

## Highest-impact verified next gap

### Gen III-IV language-domain legality

This is the first bounded parity tranche selected from the ledger because it is already source-verified on both sides and currently allows impossible metadata to escape the generic validator.

Pinned PKHeX `LanguageVerifier` rejects:

- language ID `6` (`UNUSED_6`),
- language IDs above the generation/context maximum,
- language ID `0` unless the encounter is the specific Gen V BW trade exception.

Pinned PKHeX `Legal.GetMaxLanguageID` uses:

- Gen III: Spanish (`7`) maximum,
- Gen IV: Korean (`8`) maximum.

Accepted PokeBank production currently rejects only non-zero language IDs above `10` or equal to `6`. Therefore a native Gen III record with language `8`, `9` or `10`, and a native Gen IV record with `9` or `10`, can pass this generic domain check.

The first implementation tranche should be deliberately narrow:

1. apply the pinned maximum to **wired Gen III and Gen IV language fields**;
2. keep language ID `6` invalid;
3. treat zero conservatively where PokeBank format wiring is uncertain rather than expanding the rule into Gen I/II inference paths;
4. add permanent positive/negative regression vectors for the generation boundary;
5. do not mutate Pokémon data and do not add any auto-fix behavior.

## Priority queue after language-domain parity

1. General Gen I-IV ball legality versus PKHeX's dedicated ball verifier/use-legality model.
2. Trainer/OT/language/text/trash-byte metadata, broken into source-backed bounded tranches.
3. Ribbon acquisition and impossible-combination legality.
4. Egg/evolution/transfer history interactions, especially pre-evolution provenance and move-history transformations.
5. Remaining Method J/K lead/rejection/retry histories.
6. Remaining obscure encounter and game-specific exception families.

## Promotion discipline

Every code candidate remains forward-only from the live accepted legality head. Before promotion: re-fetch the accepted branch; prove no backward history; require exact-head CI, clean host build, full host suite, focused RSE regression, ASan and UBSan. Only then may the legality branch fast-forward. Never force-push. PR #103 remains draft and must not be merged into MAIN without explicit owner permission.
