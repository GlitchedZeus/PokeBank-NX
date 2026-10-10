# Gen I-IV PKHeX parity ledger

This ledger tracks **accepted production legality behavior** on `feature/legality-engine-gen1-4-20260929` against the pinned PKHeX reference commit:

- PKHeX: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`
- PokeBank NX accepted baseline when this ledger was created: `90816ca57ffa918359b01e7f8cc994c8e57af6a1`
- Last historical reconciliation: `4405098059f0f6a5a649732913b844ac77d4da35` on 2026-10-08.
- Rechecked against accepted legality HEAD `1f5f2027c5ee47f3adbb0efee659db3e3c205918` on 2026-10-09. Updated acceptance checkpoint: `34f420b88f0b2f06c3a43ca55dbe3000f4e974c3` (exact-head CI #3141/#3147/#3149/#3150/#3151 all green; stacked #173-#176 accepted by non-force FF). Emerald Safari PRs #164–#171 are now accepted **as evidence-only** and are **not** counted as production verifier parity.

- Historical verified accepted legality HEAD **`03ea51536691497bd4309df271cb6756ceaf4087`** on 2026-10-09. Exact-head clean/full host, focused RSE and ASan/UBSan CI green and safe non-force fast-forward accepted PRs **#177–#186**. #178–#180 add bounded positive-only HGSS mixed Synchronize 2/3-retry reconstruction and reporting; #182 pins native Method K one-/two-PID nature-rejection history tests; #183–#186 accept real-source and native PK4 Gen IV Sport/Safari/Great Marsh and Gen III direct/evolved Safari Ball **informational** provenance. Stacked sibling PRs were administratively closed after proven accepted ancestry, not merged into MAIN.
- **At the older 2026-10-09 checkpoint (now accepted; see current checkpoint above):** draft [#187](https://github.com/GlitchedZeus/PokeBank-NX/pull/187) tests native encrypted PK4 multi-retry analyzer reporting; draft [#188](https://github.com/GlitchedZeus/PokeBank-NX/pull/188) tests real HGSS Nincada -> Shedinja Bug Contest Sport/Poké Ball exception. Neither is accepted or counted as parity while its own exact-head CI is unfinished.

It is intentionally conservative. A similarly named source file is not enough to claim parity: `Implemented` requires production-path behavior and regression evidence. `Partial` means useful source-backed checking exists but material PKHeX behavior remains unreconstructed. `Missing` means no accepted production implementation was verified. `Needs source verification` means the current comparison is not yet strong enough to classify safely.

The engine's semantic safety rule remains unchanged: **unknown, unsupported, or unreconstructable history is unresolved / Incomplete, not Invalid.** `Invalid` requires source-backed evidence that the represented Pokémon is impossible.

## Current accepted checkpoint (2026-10-10)

**Authoritative isolated Gen I–IV legality HEAD:** `bf9d2e418e516b6eff5054803f037c921f027b2c` (#203). **Do not merge umbrella #103 into MAIN.**

- Accepted by exact-head CI (identity, clean/full host, focused RSE, ASan/UBSan), non-force expected-head fast-forwards, verified zero-behind ancestry: **#187–#203**. #187's native encrypted PK4 Method-K retry-report fixture and #188's evolved HGSS Shedinja Sport/Poké Ball exception are accepted; the previous "pending #187/#188" language further below is historical.
- #189 reconciled the earlier RNG/ball parity position. #190–#195 contribute source-backed positive Gen III/IV egg/Apricorn/Safari/Shedinja ball evidence, native evolved Great Marsh verification, Wurmple PID branch and a hatched-egg wild-origin guard. #196–#198 expand exact Crystal wild-pre-evolution caught-data support with strict PK2/egg and six evolution-family regressions.
- #199 validates the strict 32-KiB international Crystal raw-save to immutable party PK2 to full production legality report path; the corrected retail fixture uses 20 slots per box × 14 boxes (7 per SRAM bank). #200 adds evolved Eevee gift evidence; #201 evolved Johto starter gifts; #202 evolved Dragon's Den Dratini gift; #203 native shiny-DV vs non-shiny Lake of Rage Red Gyarados source reporting. **These are positive compatibility findings, not unique history proof or global hard-Invalid evidence.**
- **Unaccepted, running exact-head CI at this checkpoint:** [#204](https://github.com/GlitchedZeus/PokeBank-NX/pull/204), Tyrogue's three evolved Hitmon gift histories; [#205](https://github.com/GlitchedZeus/PokeBank-NX/pull/205), Crystal boxed PK2 evidence through both SRAM banks. These are not counted as accepted production parity.
- **Parity measurement:** of the 21 verifier/reporting areas below, **1 is classified Implemented (reporting contract only) and 20 remain Partial**. Touching every area is not equivalent to passing every PKHeX verifier. No measured percentage of PKHeX parity should be inferred from this count; historical coverage gaps below remain authoritative unless superseded explicitly.

Earlier checkpoint paragraphs and "pending" statements below describe **their historical dates**, not today's PR or acceptance status. Historical acceptance records are preserved to avoid losing provenance; new decisions must use this current checkpoint, the live GitHub heads and completed exact-head CI.

## Status ledger

| Area | Status | Accepted PokeBank evidence | Verified parity gap / next work |
| --- | --- | --- | --- |
| 1. Structural Pokémon validity | Partial | Production `Legality.cpp` performs generation/profile-aware field checks and structured findings. Existing engine has exact-game profiles and generation ceilings. | PKHeX has a wider verifier surface for structural/format-specific invariants. Continue verifier-by-verifier comparison before claiming full structure parity. |
| 2. Species / forms | Partial | Generation species ceilings plus Gen IV `Gen4FormEvidence` and source-aware encounter matching are in production. | Form availability and source/history interactions are not yet proven exhaustive across Gen I-IV. |
| 3. Moves | Partial | Exact game/context move legality, Gen III/IV egg-move evidence, and generation-correct move ceilings are accepted. | Historical move acquisition/retention through evolution, trade and transfer is not yet exhaustive. |
| 4. Abilities | Partial | Generation-aware ability checks exist and Gen IV lead mechanics model ability-driven encounter effects such as Pressure, Hustle, Vital Spirit, Static and Magnet Pull. | Source/evolution/history ability legality is not yet exhaustive; Gen I-II are not applicable to native ability legality. |
| 5. Natures | Partial | Gen III/IV PID/RNG reconstruction checks nature-bearing PID histories in multiple source families. | Remaining Method J/K rejection/retry histories and special source mechanics prevent parity. Gen I-II native nature legality is not applicable. |
| 6. PID / IV correlation | Partial | Accepted Gen III handheld Method 1/2/3/4, roamers, GameCube/source families, and substantial Gen IV Method J/K, event and special RNG evidence. | Known unsupported lead/rejection/retry histories remain, so unmatched reconstructable classes must not be over-classified Invalid. |
| 7. Wild encounters | Partial | Gen I catch-rate/encounter data, Gen II Crystal direct and evolved wild caught-data (#196–#198), significant Gen III provenance, and Gen IV wild datasets plus RNG/source matching are accepted. | Obscure encounter categories and additional source-specific exceptions remain. |
| 8. Static / gift encounters | Partial | Gen II exact-source static/trade evidence plus Crystal evolved Eevee, Johto starter and Dratini gift compatibility (#200–#202) and fixed shiny Red Gyarados native DV reporting (#203); Gen III fixed/GameCube/event and Gen IV static/trade/gift/event evidence are accepted. | Hatched/evolved gift histories and remaining special gifts/statics are not exhaustive. |
| 9. Eggs | Partial | `Gen34EggState`, `Gen34EggMoveEvidence`, Gen III Wondercard/event egg evidence, and Gen IV hatch-location/origin evidence are present in accepted production paths. | Full egg-origin, breeding-parent, inherited-move and post-hatch provenance reconstruction is incomplete. |
| 10. Event Pokémon | Partial | Multiple pinned Gen III event templates and generated Gen IV WC4/PCD template evidence are accepted; production reporting explicitly marks surviving invariants vs unresolved history. | OT text, ribbons, dates, trash bytes, evolved event-source histories and remaining event edge cases are incomplete. |
| 11. Evolution history | Partial | Wurmple PID branch evidence (#193), Gen IV static evolution, Gen II Crystal evolved wild and several pinned evolved gift ancestors (#196–#202), and some event evolution-line matching are accepted. | Deep chains, pre-evolution provenance, egg/evolution interactions and source-game move history are incomplete. |
| 12. Transfer / history | Partial | Gen I/II Time Capsule evidence and Gen IV transfer evidence (including Pal Park stored-field classification) exist in production. | Full tradeback, Pal Park, cross-game transformations, move retention/loss and historical field transitions are not exhaustive. |
| 13. Trainer / OT metadata | Partial | Source-specific trainer rules exist (for example Colosseum/XD OT gender and fixed/event trainer identities). | Comprehensive OT/TID/SID relationships, trainer gender/identity restrictions and trade-history effects remain. |
| 14. Language / text / trash bytes | Partial | Production now wires `Gen34LanguageEvidence.h` for exact Gen III/IV source identities: rejects unused ID 6 and Gen III IDs above 7 / Gen IV IDs above 8; treats zero as unresolved where format wiring is uncertain. Event/source matchers consume language and some fixed-source text evidence. | **The earlier Gen III/IV language-domain gap is closed** for wired exact sources. Full PKHeX parity still requires OT/nickname text encoding, trash bytes, source-specific language constraints and historical transformations. Do not treat unresolved ID 0 as proof of validity. |
| 15. Ribbons | Partial | Specific Gen III egg/event ribbon evidence exists and event templates preserve some ribbon-related provenance. | No verified comprehensive acquisition-order / impossible-combination / game-specific ribbon provenance checker yet. |
| 16. Met data | Partial | Exact encounter matchers, Gen IV origin/release/hatch evidence, static/event templates and transfer classifiers validate substantial met-location/level combinations. | Full met-data combination and historical transformation parity remains incomplete. |
| 17. Balls | Partial | Exact Gen III/IV ball ID-domain checks (Gen III max 12, Gen IV max 24; zero unresolved) and **positive INFO-only** HGSS Sport/Shedinja exception (#188), Apricorn and Safari (#191–#192), D/P/Pt Great Marsh (#183–#186, #195), Gen III Safari direct/evolved (#183–#186), plus Gen III/IV egg-ball evidence (#190), Wurmple branch gate (#193), and hatched-egg wild-origin guard (#194). | Ball legality is **not globally complete**: no exhaustive egg/gift/trade/transfer and evolved alternative-history closure, no hard Invalid from absence of compatible fixed-ball evidence. The bounded #188 Shedinja exception is accepted, but is not exhaustive global ball parity. |
| 18. Held-item / history constraints | Partial | Production resolves held-item IDs in the Pokémon's source item namespace and warns on unknown IDs. | Legal held-item sets, source-game restrictions and historical item transitions are not yet reconstructed to PKHeX parity. |
| 19. Lead / RNG mechanics | Partial | Accepted Method J/K evidence includes native no-lead/source/slot, BCC/Safari minimum-31 rerolls, Pressure/Hustle/Vital Spirit, Static/Magnet Pull, one-reroll mixed Synchronize (#173–#176), **two/three-reroll mixed Synchronize positive proof and central report** (#178–#180), bounded HGSS one-/two-PID nature rejection (#182), and native encrypted PK4 full-report multi-retry evidence (#187). | Full PID-nature rejection loops, competing RNG lead modes, odd special encounters, all historical origins, and comprehensive PKHeX parity remain incomplete. Native encrypted PK4 full-report proof from #187 is accepted; full historical RNG parity is still incomplete. |
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

## Gen IV Method K bounded retry checkpoint (2026-10-09)

Accepted: first-Synchronize-success/retained-failure and first-failure/retained-success HGSS Bug Catching Contest one-reroll histories (#173-#176). The proofs are positive-only against the real pinned HeartGold Kakuna slot 3, location 207, levels 9-18 and encounter rate 25. They are integrated into source-indexed reporting without any new hard-Invalid classification.

Accepted now: #177 proves the real-source central Method K report bridge; #178–#180 reconstruct and report mixed Synchronize chains with two or three rejected minimum-31 IV attempts, including a fourth retained attempt without a 31 IV after the earlier three valid rejections. #182 pins one-/two-PID nature rejection vectors for an existing no-lead path. Their exact heads have green clean/full host/RSE/sanitizer CI and are ancestors of the accepted legality branch. **Accepted #187** now confirms those bounded deeper histories through a native encrypted PK4 -> production legality-report fixture.

Not reconstructed: full PID-nature rejection loops, ambiguous lead histories, all alternative origins and source-specific transfer/evolution interactions. These remain Incomplete / Unresolved, never Invalid based on absence of a bounded RNG proof.

## Highest-impact remaining work (source-by-source)

1. **General Gen I-IV encounter-specific ball-use legality**, beyond the now-wired ID-domain checks. Account for wild vs static, gifts/events, evolution, eggs, transfers and exceptional histories.
2. **Trainer/OT/text/trash-byte metadata** by exact game and source. Keep unwired language 0 unresolved.
3. **Ribbons**: acquisition order, impossible combinations, historical transfer availability.
4. **Egg/evolution/transfer provenance**: inherited moves, post-hatch changes, pre-evolution source and cross-game transformations.
5. **Remaining Method-H / Method-J / Method-K lead and RNG rejection/retry histories**. Distinguish tested evidence-only paths from production verdict rules.
6. Remaining obscure encounters and per-version exceptions with pinned source data and permanent vectors.

## Promotion discipline

Every code candidate remains forward-only from the live accepted legality head. Before promotion: re-fetch the accepted branch; prove no backward history; require exact-head CI, clean host build, full host suite, focused RSE regression, ASan and UBSan. Only then may the legality branch fast-forward. Never force-push. PR #103 remains draft and must not be merged into MAIN without explicit owner permission.
