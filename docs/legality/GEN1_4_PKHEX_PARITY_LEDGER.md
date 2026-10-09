# Gen I-IV PKHeX parity ledger

This ledger tracks **accepted production legality behavior** on `feature/legality-engine-gen1-4-20260929` against the pinned PKHeX reference commit:

- PKHeX: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`
- PokeBank NX accepted baseline when this ledger was created: `90816ca57ffa918359b01e7f8cc994c8e57af6a1`
- Last historical reconciliation: `4405098059f0f6a5a649732913b844ac77d4da35` on 2026-10-08.
- Rechecked against accepted legality HEAD `1f5f2027c5ee47f3adbb0efee659db3e3c205918` on 2026-10-09. Emerald Safari PRs #164–#171 are now accepted **as evidence-only** and are **not** counted as production verifier parity.

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
| 6. PID / IV correlation | Partial | Accepted Gen III handheld Method 1/2/3/4, roamers, GameCube/source families, and substantial Gen IV Method J/K, event and special RNG evidence. | Known unsupported lead/rejection/retry histories remain, so unmatched reconstructable classes must not be over-classified Invalid. |
| 7. Wild encounters | Partial | Gen I catch-rate/encounter data, Gen II wild data, significant Gen III provenance, and Gen IV wild datasets plus RNG/source matching are accepted. | Obscure encounter categories and additional source-specific exceptions remain. |
| 8. Static / gift encounters | Partial | Gen II static/trade evidence, Gen III fixed/GameCube/event sources, and Gen IV static/trade/gift/event evidence are accepted. | Hatched/evolved gift histories and remaining special gifts/statics are not exhaustive. |
| 9. Eggs | Partial | `Gen34EggState`, `Gen34EggMoveEvidence`, Gen III Wondercard/event egg evidence, and Gen IV hatch-location/origin evidence are present in accepted production paths. | Full egg-origin, breeding-parent, inherited-move and post-hatch provenance reconstruction is incomplete. |
| 10. Event Pokémon | Partial | Multiple pinned Gen III event templates and generated Gen IV WC4/PCD template evidence are accepted; production reporting explicitly marks surviving invariants vs unresolved history. | OT text, ribbons, dates, trash bytes, evolved event-source histories and remaining event edge cases are incomplete. |
| 11. Evolution history | Partial | Wurmple evolution evidence, Gen IV static evolution evidence, and some evolution-line event matching are accepted. | Deep chains, pre-evolution provenance, egg/evolution interactions and source-game move history are incomplete. |
| 12. Transfer / history | Partial | Gen I/II Time Capsule evidence and Gen IV transfer evidence (including Pal Park stored-field classification) exist in production. | Full tradeback, Pal Park, cross-game transformations, move retention/loss and historical field transitions are not exhaustive. |
| 13. Trainer / OT metadata | Partial | Source-specific trainer rules exist (for example Colosseum/XD OT gender and fixed/event trainer identities). | Comprehensive OT/TID/SID relationships, trainer gender/identity restrictions and trade-history effects remain. |
| 14. Language / text / trash bytes | Partial | Production now wires `Gen34LanguageEvidence.h` for exact Gen III/IV source identities: rejects unused ID 6 and Gen III IDs above 7 / Gen IV IDs above 8; treats zero as unresolved where format wiring is uncertain. Event/source matchers consume language and some fixed-source text evidence. | **The earlier Gen III/IV language-domain gap is closed** for wired exact sources. Full PKHeX parity still requires OT/nickname text encoding, trash bytes, source-specific language constraints and historical transformations. Do not treat unresolved ID 0 as proof of validity. |
| 15. Ribbons | Partial | Specific Gen III egg/event ribbon evidence exists and event templates preserve some ribbon-related provenance. | No verified comprehensive acquisition-order / impossible-combination / game-specific ribbon provenance checker yet. |
| 16. Met data | Partial | Exact encounter matchers, Gen IV origin/release/hatch evidence, static/event templates and transfer classifiers validate substantial met-location/level combinations. | Full met-data combination and historical transformation parity remains incomplete. |
| 17. Balls | Partial | Production wires `Gen34BallDomainEvidence.h` for exact Gen III/IV sources (nonzero IDs above 12 / 24 respectively are Invalid); zero is unresolved. Source/template-specific ball matching exists for several static/event/shadow sources. | **Only ID-domain checking is established generally**, not whether a particular ball was obtainable for a particular species, encounter, location or transfer history. General Gen I-IV capture-ball use legality and complete competing provenance remain incomplete. |
| 18. Held-item / history constraints | Partial | Production resolves held-item IDs in the Pokémon's source item namespace and warns on unknown IDs. | Legal held-item sets, source-game restrictions and historical item transitions are not yet reconstructed to PKHeX parity. |
| 19. Lead / RNG mechanics | Partial | Extensive Gen IV Method J/K lead evidence is accepted: BCC rerolls, Pressure/Hustle/Vital Spirit, Static/Magnet Pull, Synchronize success/failure subsets, Safari/Suction Cups and central reporting. | Mixed failed+successful Synchronize retry chains, failed-Synchronize nature-lock transitions, PID nature-rejection loops and other unsupported histories remain. |
| 20. Game-specific exceptions | Partial | Exact-game profiles plus many Gen I-IV special-source modules (Time Capsule, e-Reader, GameCube, event, Pokéwalker, Ranger Manaphy, Safari/BCC, etc.) are accepted. | PKHeX contains additional game/version-specific exceptions that still require source-by-source comparison and permanent vectors. |
| 21. Verdict / reporting behavior | Implemented | Accepted engine has structured findings, `Invalid`, `No Problems Found`, `Incomplete`, explicit coverage tracking, a production report bridge and lead-history descriptions. | This status covers the reporting contract only, **not** parity of every verifier feeding it. New unsupported history must continue to resolve conservatively as Incomplete. |

## Native PK4 language regression and pinned reference

Pinned PKHeX `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (`PKHeX.Core/Legality/Verifiers/LanguageVerifier.cs` and `Legal.cs`) confirms unused ID 6 is invalid, native Gen III language max is Spanish (7), and native Gen IV max is Korean (8). The previous Gen IV PokeBank source-context analyzer test used a PK3 fixture and did not prove native encrypted PK4 decoding. A new fixture exercises encrypted PK4 -> immutable PK4 reader -> shared view -> exact source legality analyzer with IDs 6 (invalid), 8 (not rejected by language-domain check), 9 (invalid), and 0 (the codebase's intentionally unresolved sentinel). This adds a boundary regression, **not** wider OT/text/trash-byte parity or a new production hard-Invalid rule.

## Reconciled Gen III-IV domain checks (closed bounded gaps)

The accepted production path `src/Legality/Legality.cpp` already uses `Gen34LanguageEvidence.h` and `Gen34BallDomainEvidence.h` whenever an exact Gen III/IV source profile is wired. The earlier priority text incorrectly treated these checks as missing.

| Domain | Accepted source-backed classification | Still unresolved |
| --- | --- | --- |
| Native Gen III language ID | ID 6 or IDs above 7 => Invalid; IDs 1-5 and 7 => within generation domain; 0 => Unresolved | Full event/source text, OT and trash-byte history, and safe interpretation of unwired fields |
| Native Gen IV language ID | ID 6 or IDs above 8 => Invalid; IDs 1-5, 7-8 => within generation domain; 0 => Unresolved | Language acquisition/history and source-specific constraints |
| Native Gen III Ball ID | Nonzero ID above 12 => Invalid; IDs 1-12 => within domain; 0 => Unresolved | Encounter-specific ball use (Safari and event/fixed sources), evolution, egg and transfer alternatives |
| Native Gen IV Ball ID | Nonzero ID above 24 => Invalid; IDs 1-24 => within domain; 0 => Unresolved | Sport/Safari/Cherish restrictions, static/gift/event and special-history alternatives |

**Domain-valid does not imply encounter-legal.** Broad format fallbacks for unwired/unknown source identity are not evidence that the source generation has been reconstructed.

## Evidence-only versus production distinction

The accepted Gen III Safari tables, evolution-aware fixed-ball origin evidence and Method-H lead paths progressively prove **positive compatible encounter histories**. The candidate Emerald lead tranches likewise remain evidence-only, with no analyzer hard `Invalid` path. No source-specific Safari conclusion should be upgraded to global Invalid until all compatible competing histories are accounted for and the production analyzer has direct regression coverage. Unknown, unsupported, or unreconstructable stays **Incomplete/unresolved**.

## Highest-impact remaining work (source-by-source)

1. **General Gen I-IV encounter-specific ball-use legality**, beyond the now-wired ID-domain checks. Account for wild vs static, gifts/events, evolution, eggs, transfers and exceptional histories.
2. **Trainer/OT/text/trash-byte metadata** by exact game and source. Keep unwired language 0 unresolved.
3. **Ribbons**: acquisition order, impossible combinations, historical transfer availability.
4. **Egg/evolution/transfer provenance**: inherited moves, post-hatch changes, pre-evolution source and cross-game transformations.
5. **Remaining Method-H / Method-J / Method-K lead and RNG rejection/retry histories**. Distinguish tested evidence-only paths from production verdict rules.
6. Remaining obscure encounters and per-version exceptions with pinned source data and permanent vectors.

## Promotion discipline

Every code candidate remains forward-only from the live accepted legality head. Before promotion: re-fetch the accepted branch; prove no backward history; require exact-head CI, clean host build, full host suite, focused RSE regression, ASan and UBSan. Only then may the legality branch fast-forward. Never force-push. PR #103 remains draft and must not be merged into MAIN without explicit owner permission.
