# Gen I–IV secondary-reference audit: Auto Legality Mod 23.09.25

## Identity and limits

- Application audit base: `25cdc17661208b2a88bdd04b38332bce016a1e53`, PR #103, fetched live before creating this isolated branch. This preserves the newer WC3 checkpoint commit above the handoff's `14ae3382…`.
- Primary source: [kwsch/PKHeX at 6501f0ab46e8f8ca048539dbaf8cae8cb104e722](https://github.com/kwsch/PKHeX/tree/6501f0ab46e8f8ca048539dbaf8cae8cb104e722). All PKHeX paths/functions below refer to **this commit**, not current HEAD.
- Secondary source: [release 23.09.25](https://github.com/architdate/PKHeX-Plugins/releases/tag/23.09.25), published 2023-09-25; tag resolves to `b385ecb4395a27f2f36af6d8ce89ec7c5f2e1b49`.
- Comparison: [23.08.23…23.09.25](https://github.com/architdate/PKHeX-Plugins/compare/23.08.23...23.09.25); older tag resolves to `1026554b9124d1e6343072d2e3c6fd323fcc99d5`.
- Source audit only: the 2023 .NET plugin was not executed. Its generated output and corpus descriptions are not validated PK3/PK4 specimen verdicts. No claim of PKHeX parity.
- Only implementation in this tranche: persisted Earth reader and exact-native-Gen-III unhatched Event3 ribbon contradictions. All other findings below are recommendations, not new rejection rules.

## Implemented primary-source rule

[PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierEvent3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierEvent3.cs), `ParseEgg`, adds a ribbon error independently for Earth, National, Country, Champion Battle, Champion Regional and Champion National. No encounter reconstruction or bounded RNG search is needed to establish this particular contradiction.

[PKHeX.Core/PKM/PK3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/PK3.cs), `RIB0` and the six ribbon properties, establish canonical word offset `0x4C`, bits 20–25, Earth mask `0x02000000`.

`RibbonVerifierEvent3.Parse` and `PKHeX.Core/Legality/Verifiers/Ribbons/RibbonRules.cs::IsEarthRibbonAllowed` distinguish encounter-required Earth from later GameCube acquisition. Therefore the new helper returns NotApplicable for **every non-egg**, including Earth-bearing event Pokémon. It does not add Earth to fixed-negative WC3 template checks or claim every non-egg Earth history is proven.

Application integration uses `sourceGameProfile(exactSourceGameId)->generation == 3`, not stored origin generation. PK4 with Gen III origin must not receive a native PK3 egg-ribbon diagnostic. Each actual ribbon contradiction is `Severity::Invalid`, `CheckIdentifier::Egg`; partial coverage and existing verdict semantics remain unchanged.

Tests cover 128 helper combinations; all five exact GBA source profiles with zero, each individual, and all six ribbons in both egg states; unknown/absent/Gen I/II/IV context exclusions; all 32 persisted single-bit positions; fateful-bit independence; report equality before/after adding Earth to a non-egg; and input-byte equality across analysis. Cross-context tests deliberately reuse a ribbon-capable PK3 as a routing probe, not as a claim to have built a structurally valid PK4. Existing BACD_R_A, PCNY and PCJP suites retain ordinary event-template coverage.

## What actually changed in the secondary release

The release notes emphasize version matching and generation traceback alongside modern features outside this task. The diff also contains extensive formatting churn: `WC3Seeds.cs` retains exactly the same nine nature/seed entries and zero fallback; `LegalityTests.cs` keeps the same read-original / legalize / reanalyze behavior. Neither is a new 2023 ribbon rule.

Substantive relevant changes include `APILegality.GetPokemonFromEncounter` rerolling Wurmple-origin candidates until their evolution branch matches the requested species, `GetAllEncounters` retrying alternate forms, `FilteredGameList` / `TryGetSingleVersion` using version filters, and the new `ALMTraceback` record/categories. These are generation/search mechanisms; they must not be copied into a read-only verifier.

Corpus changes replace many broad `~=Generation=3/4` constraints with exact or grouped `~=Version=…` filters. Native-only PokéWalker examples switch from persisted-field `=Met_Location=233` to encounter-filter `~=Location=233`. This distinction matters: a search instruction is not an immutable recorded field or proof that a particular encounter occurred.

Counts below are nonempty blank-line-delimited set blocks, not passing test counts (the runner further filters/deduplicates):

| Corpus under AutoModTests/ShowdownSets | 23.08.23 | 23.09.25 |
|---|---:|---:|
| Anubis Tests/Anubis - pk3.txt | 104 | 104 |
| Anubis Tests/Anubis - pk4.txt | 54 | 54 |
| RoCs-PC Tests/RoC - pk3.txt | 333 | 332 |
| RoCs-PC Tests/RoC - pk4.txt | 400 | 399 |
| RoCs-PC Tests/RoC notransfer - pk3.txt | absent | 1 |
| RoCs-PC Tests/RoC notransfer - pk4.txt | 33 | 35 |
| Underleveled Tests/Underlevel - pk3.txt | 36 | 36 |
| Underleveled Tests/Underlevel - pk4.txt | 178 | 173 |
| Underleveled Tests/Underlevel notransfer - pk4.txt | 7 | 12 |

## Audit matrix

`API` below is `PKHeX.Core.AutoMod/AutoMod/APILegality.cs`. Application paths are relative to this repository; PKHeX paths are relative to pinned `PKHeX.Core/`. High confidence means directly inspected source; it does **not** mean an entire species/history is solved.

| Plugin file/test | Behavior / edge case | Gen | Matching pinned PKHeX source/function | PokeBank NX coverage | Real gap? | Recommended regression / implementation | Confidence |
|---|---|---|---|---|---|---|---|
| API `FindPIDIV`; `AutoMod/Seeds/BACD_R/WC3Seeds.cs::GetShinyWishmakerSeed` | Nine shiny WISHMKR seed shortcuts; unknown nature returns zero and generator continues | III | `Legality/RNG/ClassicEra/Gen3/CommonEvent3Checker.cs::IsRestrictedSimple`; `Legality/Encounters/Data/Gen3/EncountersWC3.cs::WISHMKR row`; `EncounterGift3.IsCompatibleReviseReset` | `Gen3BacdPidIvCorrelation.h`, `Gen3WishmkrEventTemplate.h`, existing BACD tests | Nine-seed regression coverage opportunity, not evidence of a missing BACD rule | Recreate all nine seed-derived PID/IV vectors using pinned equations; check nature/shiny/TID 20043/SID 0 and nearby negative identities. Do not treat dictionary absence as Invalid | High |
| API `HandleEggEncounters` | Force-hatches eggs and replaces trainer/language | III/IV | `Legality/Encounters/Templates/Gen3/Gifts/EncounterGift3.cs::IsMatchExact`, `GetSafeLanguage`, `SetForceHatchDetails` | Unhatched Box/PCJP/Wondercard/PokéPark helpers; explicit post-hatch incomplete messages | Yes: post-hatch distribution/recipient reconstruction remains partial | Paired unhatched and hatched specimens; retain uncertain recipient history as Incomplete. Never call a hatch/setter path during analysis | High |
| API egg handling versus native ribbon state | Generator normalizes away egg state; cannot justify allowing ribbons on an unhatched egg | III | `Legality/Verifiers/Ribbons/RibbonVerifierEvent3.cs::ParseEgg`; `PKM/PK3.cs` | **Implemented in this tranche** | Closed for the six specified ribbons in exact Gen III context | `test_gen3_egg_event_ribbons.cpp`, `test_gen3_event_ribbon_bits.cpp` | High |
| API `GetPokemonFromEncounter`; RoC pk3/pk4 Wurmple family | Wurmple-origin PID determines evolution branch; direct wild cocoon encounters differ | III/IV | `Editing/WurmpleUtil.cs::GetWurmpleEvoVal`, `IsWurmpleEvoValid`; `Legality/Verifiers/PIDVerifier.cs::VerifyECPIDWurmple`; `EncounterSlot3/4.IsDeferredWurmple` | Generic ancestor lookup exists; no Wurmple branch check found in legality helpers/central report | Yes, encounter-conditioned branch evidence is missing | Use PID high word modulo 10; test both branches and boundary 4/5/9/10. Reject a candidate Wurmple-origin history, not all directly caught Silcoon/Cascoon. If alternative encounters remain, keep overall provenance Incomplete | High |
| API `GetAllEncounters`; evolved corpus entries | Enumerates encounters and alternate forms rather than insisting current species equals source species | III/IV | `Legality/Encounters/Generator/ByGeneration/EncounterGenerator3/4.cs::GetEncounters`; `EncounterGift3.IsMatchExact(PKM,EvoCriteria)`; `Legality/Encounters/Verifiers/EvolutionVerifier.cs::VerifyEvolution` | PCNY/PCJP/BACD_R_A descendant matching; Gen IV event/wild/PokéWalker ancestor matching | Exact level/timing/form-history constraints remain partial, already acknowledged | Preserve source species + evolution path as positive evidence; add timing constraints only when pinned origin-chain semantics are ported | High |
| API `FilteredGameList`, `TryGetSingleVersion`; widespread corpus version-filter edits | Requested generation target, encounter version and stored origin are distinct | III/IV | `EncounterGenerator4.GetEncounters` uses `pk.Version`; `EncounterGift3.IsMatchExact` checks Version containment | `Gen4OriginEvidence.h`; exact source profiles; native Gen IV matching routes by stored retail origin | Already covered in core routing; regression expansion useful | Same stored Diamond origin in a Platinum container; separate Ruby-origin Pal Park specimen. Never substitute destination game for encounter origin | High |
| API `IsRequestedLevelValid`; Underlevel pk3 Metapod/Kakuna L4, Silcoon/Cascoon L5 | Naturally caught evolutions can be below normal evolution level | III | `Legality/Encounters/Templates/Gen3/EncounterSlot3.cs::IsMatchExact`; `Legality/Verifiers/LevelVerifier.cs::Verify`; `EvolutionVerifier.VerifyEvolution` | Encounter guardrails and met/current-level checks; Method H slot utilities; full history incomplete | No new global minimum-level rule justified; broader reconstruction remains partial | Recreate direct-capture low-level cocoons with pinned slot data plus evolved-from-Wurmple alternatives; do not reject on species evolution level alone | High |
| Underlevel notransfer pk4 Gloom L14, Poliwhirl/Poliwrath L15 at encounter 233 | PokéWalker fixed encounter or descendants can be below normal wild/evolution expectations | IV | `Legality/Encounters/Templates/Gen4/EncounterStatic4Pokewalker.cs::IsMatchExact`; `EvolutionVerifier.VerifyEvolution` | `Gen4PokewalkerEncounter.h::matchEvolutionLine`, `Gen4PokewalkerPid.h` | IV/course-unlock provenance and exact evolution history remain incomplete | Recreate direct and evolved pairs retaining original met level/course; keep unresolved IV history distinct from deterministic PID evidence | High |
| RoC notransfer pk3 new Gengar L21; new native-only pk4 Gengar entries, including Gaspar/Mindy | Trade evolution must retain encounter/trainer provenance | III/IV | `EvolutionVerifier.VerifyEvolution`; `Legality/Encounters/Templates/Gen4/EncounterTrade4PID.cs::IsMatchExact` | Generic ancestry and `Gen4TradeEvidence.h`; no complete chronological trade-history reconstruction | Partial history remains a real limitation | Recreate original encounter then later trade evolution, with exact fixed trainer/IV identity; native-only is a corpus policy, not proof of no link trades | High |
| API `GetTrainer`; Anubis Pichu Follow Me Japanese, `FatefulEncounter=False` | Identity varies by distribution; same special move can have multiple event origins | III | `EncountersWC3.cs` Wondercard and DS-download Pichu entries; `EncounterGift3.IsMatchExact` | Separate `Gen3WondercardEggEventTemplate.h` and `Gen3PokeParkEggEventTemplate.h` already distinguish these unhatched paths | Post-hatch evidence is still incomplete; no new universal fateful rule | Paired distributions with language, fateful, origin and met-level changes; never infer template solely from Follow Me | High |
| API trainer/language mutation; Underlevel pk3 Spanish Meowth / ISA | Generator may choose identity; verifier must check surviving fixed/traded identity | I–IV | `Legality/Verifiers/LanguageVerifier.cs::Verify`, `IsValidLanguageID`; `TrainerNameVerifier.cs::Verify`; `EncounterGift3.IsMatchExact` | Fixed Gen III event OT/TID/SID/language and Gen II/IV trade tables; broad language range check | Generation-specific text/trash-byte and recipient semantics remain partial | Pinned per-format language bounds and fixed-OT trade specimens, without normalizing strings or changing language | High |
| API `FindLikelyPIDType`, Manaphy adjustment; Anubis pk4 shiny Manaphy L1 | Generator writes Link Trade egg location to search for a legal shiny path | IV | `MysteryGifts/PGT.cs::IsRangerManaphy`, `IsG4ManaphyPIDValid`, `SetPINGAManaphy` | `Gen4RangerManaphy.h` recognizes native/traded structures and unresolved original recipient | Core distinction covered; original recipient can remain unknowable | Test shiny for current versus original recipient with unchanged bytes; no automatic link-trade marker insertion | High |
| API `nativeOnly` filtering; RoC native-only Spiky Pichu | Native-only constraints must not become blanket origin rejection | I–IV | `Legality/Verifiers/TransferVerifier.cs`; `Gen4 FormInfo` rules; `Legality/Restrictions/GBRestrictions.cs::CanVisitGen1` | `Gen12TimeCapsuleEvidence.h`, `Gen4OriginEvidence.h`, `Gen4TransferEvidence.h`, `Gen4FormEvidence.h` | Transfer chronology remains incomplete where overwritten evidence is lost | Keep Time Capsule compatibility separate from historical proof; Gen III egg in PK4 is a transfer contradiction, not this new native ribbon check | High |
| API `FindPIDIV` / timeout wrapper | Search may time out; after many attempts generation may compromise requested properties | III/IV | `EncounterGenerator3/4.GetEncounters` and event correlation methods define candidate semantics, not an ALM timeout verdict | Bounded GameCube histories already explicitly emit Incomplete | Existing policy correct; no timeout-to-Invalid rule warranted | Preserve known safety-limit regression vectors; expose exhausted budget as evidence status, never overwrite Pokémon | High |
| `AutoModTests/LegalityTests.cs::VerifyAll`; `TeamTests.cs::RunVerification` | First validates saved specimens, then legalizes; Showdown corpus tests generate and reanalyze | I–IV | `LegalityAnalysis`; pinned encounter/verifier classes | Read-only host fixtures and explicit verdict assertions | Testing architecture opportunity, not a legality rule | Copy the idea of positive/negative paired fixtures only. Recreate and pin bytes/source; do not execute ALM's repair stage or equate requested sets to legal source records | High |
| `AutoMod/ALMTraceback.cs`; API trace additions | Categories describe mutations made to a generated candidate | I–IV subset | `Legality/Structures/CheckResult.cs`, `CheckIdentifier.cs`, verifier/encounter evidence | `Issue`, `CheckIdentifier`, `CoverageSummary` exist, but no structured positive provenance trace | Yes, structured trace is a design gap, not required implementation here | Read-only trace design below; retain separate aggregate coverage | High |

## Concrete follow-up vectors

1. **WISHMKR**: Bashful `0x353D`, Careful `0xF500`, Docile `0xECDD`, Hasty `0x9359`, Jolly `0xCF37`, Lonely `0x7236`, Naughty `0xA030`, Timid `0x7360`, Serious `0x3D60`. These are secondary seed leads, not a replacement for pinned BACD_R verification. Independently derive PID/IVs and verify the source constraints before adding golden fixtures.
2. **Wurmple**: valid/invalid Wurmple-origin branch pairs and direct-caught low-level Silcoon/Cascoon counterexamples. The pinned verifier gates the check on **encounter species Wurmple**. A species-only implementation would overreject.
3. **Pichu Follow Me**: Wondercard Method 2/VBlank variants versus DS-download BACD_R; then post-hatch trainer/language changes. The 2023 requested set lacks enough bytes to prove distribution identity.
4. **Native low-level encounters**: FRLG Metapod L4, Emerald Silcoon/Cascoon L5, HGSS PokéWalker Poliwhirl L15 followed by stone/trade evolution. Establish pinned source rows before expecting a complete verdict.
5. **Gen IV Manaphy**: unchanged original egg, traded unhatched egg, hatched shiny recipient, and unknown original-recipient anti-shiny state. No rewriting egg location to satisfy the validator.

## Read-only provenance trace design (not implemented)

Retain `Issue` for user-facing contradictions/warnings and `CoverageSummary` for aggregate completeness. Add a separate immutable result vector in a future scoped change; entries could carry `checkIdentifier`, category/subcategory, outcome, source reference (repository/commit/path/function/template ID), observed fields, source species/game, optional RNG seed/method, and reason for unresolved evidence. Reference observed values or owned snapshots, never mutable Pokémon pointers or setter callbacks.

| ALM category | Current read-only mapping / treatment |
|---|---|
| Encounter | Encounter; source species, template/slot, level range, origin game |
| Trainer | Trainer / EventGift; fixed OT, TID/SID, language, recipient uncertainty |
| PID_IV | PidRng; method, recovered seed and bounded-search state |
| EC | Fold into PID provenance for native Gen III/IV (no independent modern EC field); not applicable to native Gen I/II |
| Species, Level | Species / Stats / Encounter; keep encounter level distinct from current level and evolution timing |
| Shiny, Gender, Nature | PidRng or Stats with explicit subcategory; Gen I/II require their own DV/gender semantics, not a fabricated PID |
| Form | Species / Origin with form subcategory; generation-specific availability/history |
| Ability | Ability for Gen III/IV; not applicable to Gen I/II |
| Item, Moves, EVs | Items, Moves, Stats; preserve DV/stat-experience versus IV/EV distinctions |
| Friendship | Misc or Egg/Trainer subcategory when applicable; not proof of a chronological history |
| AVs, Size, HyperTrain | Not applicable to native Gen I–IV modern awakening values/scalar size/hyper-training mechanics |
| Misc | Explicit named subcategory only; do not bury unsupported checks as successes |

Outcomes should distinguish:

- **Proven / consistent**: state exactly what was established, e.g. the six event ribbons are absent on this egg; this does not certify its parentage or whole legality.
- **Contradiction**: cite an invariant and the observed contradictory values. Only these source-proven contradictions justify Invalid.
- **Not proven / incomplete**: unknown provenance, partial tables, unresolved alternatives, missing original trainer or exhausted search; retain a reason and optional budget information.
- **Not applicable / not run**: separate from a successful check; no false positive coverage credit.

An exact candidate match is one piece of evidence. A failed candidate is not automatically a failed entity when other histories remain possible. A trace must never be a sequence of 'set PID', 'force hatch', 'replace trainer', or 'fix met location' commands. Generation and repair APIs are excluded. Coverage can only become Complete when required checks actually cover the relevant history; unchanged `Report::verdict()` semantics remain authoritative.

## Disagreements and nonportable assumptions

No inspected plugin behavior overrides the primary ribbon rule. More importantly, ALM **generation behavior is not a validation contract**: forced hatching, trainer/language replacement, Manaphy location rewriting, seed shortcuts and search compromise cannot become rules that modify a PokeBank input or condemn unreconstructed history. The old plugin's `WC3` type has also been reorganized into pinned `EncounterGift3` families; names alone do not establish semantic parity. The corpus changes prove search/test intent, not that every described set is independently source-proven at the newer pin.

No Gen V+, HOME, Master Vault, UI, save provider, write-permission or auto-legalization implementation is included.
