# Gen I-IV PKHeX Parity Ledger

Date: 2026-10-06

PokeBank NX accepted legality baseline: `90816ca57ffa918359b01e7f8cc994c8e57af6a1`

Pinned PKHeX architecture/data reference: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`

Scope: Gen I-IV legality analysis only. This ledger is not a claim of full PKHeX parity.

## Rules used by this ledger

A similarly named file, table, or helper does **not** count as parity. A row is marked `Implemented` only when accepted production code covers the relevant Gen I-IV behavior with sufficiently equivalent semantics and there is permanent regression evidence for the important boundary cases. `Partial` means meaningful accepted coverage exists but one or more source-backed histories, edge cases, metadata relations, or regression classes remain. `Missing` means no material production implementation was found for the compared behavior. `Needs source verification` means the accepted engine has related behavior but the pinned PKHeX semantics have not yet been traced deeply enough to make a safe equivalence claim. `Not applicable` is reserved for behavior outside the Gen I-IV scope.

The engine's conservative verdict contract remains authoritative: **unknown, unsupported, or unreconstructable history is `Incomplete`/unresolved, not `Invalid`.** `Invalid` is reserved for a source-backed impossibility. This ledger must not be used to convert unimplemented PKHeX behavior into speculative invalid findings.

## Summary

| # | Area | Status | Accepted PokeBank NX evidence | Pinned PKHeX comparator | Verified gap / next evidence needed |
|---|---|---|---|---|---|
| 1 | Structural Pokémon validity | Partial | `src/Legality/Legality.cpp` checks format/generation ceilings, checksum-backed format readers, core field ranges, move/PP bounds, egg/fateful/source constraints; host legality targets exercise structural failures. | Core legality analysis plus format/context-specific verifiers and encounter matching. | Still need a field-by-field Gen I-IV structural matrix against the pinned verifier stack, including format-specific unused bits/flags and mutually dependent fields. |
| 2 | Species / forms | Partial | Production enforces source-generation species ceilings and source-aware form support; unsupported source form data can remain unresolved instead of becoming invalid. | Species/form availability and form-specific legality through encounter/evolution/context verifiers. | Remaining form exceptions and game-specific form transitions need explicit source-backed tables and permanent vectors. |
| 3 | Moves | Partial | Exact-game move legality, generation ceilings, move-slot/PP/PP-Up checks, and substantial source-context move data are in production. Gen III exact-game regression coverage exists. | MoveVerifier plus encounter/evolution/egg/transfer move-source history. | Current move existence is stronger than historical move provenance. Evolution, breeding, tutors, transfers, and source-game retention/loss still need deeper reconstruction. |
| 4 | Abilities | Partial | Production checks ability presence, slot validity, and source-generation/source-species availability where data is known. | AbilityVerifier and encounter/evolution-specific ability constraints. | Need complete source-specific slot transitions, hidden/special cases where applicable to Gen III-IV, and evolution/history interactions. |
| 5 | Natures | Partial | Nature range and substantial PID/RNG-context correlation are checked, especially Gen III and Method J/K Gen IV paths. | Nature validation integrated with PID/encounter/RNG methods. | Nature alone is structurally covered, but failed-Synchronize transitions and remaining nature-rejection histories keep parity incomplete. |
| 6 | PID / IV correlation | Partial | Accepted Gen III handheld/GameCube correlation work plus substantial Gen IV Method J/K RNG reconstruction and permanent vectors. | PIDVerifier, encounter generators, RNG method identification and source-specific correlation logic. | Known tail remains: mixed Synchronize retry chains, failed-Synchronize nature-lock transitions, PID nature-rejection loops, and other unreconstructed method families. |
| 7 | Wild encounters | Partial | Conservative encounter matching, generated Gen IV wild datasets, Gen III provenance work, Method J/K slots, HGSS BCC/Safari and several lead families are integrated. | Encounter generators/slots/areas plus context-specific matching and RNG verification. | Obscure Gen I-II categories, remaining Gen III edges, Safari lead families, and unreconstructed Gen IV encounter histories remain. |
| 8 | Static / gift encounters | Partial | Gen IV static/gift production evidence and focused host targets exist; significant Gen III/GameCube source-specific gift/static logic is present. | Static encounter templates, gifts/trades, encounter matching, PID/OT/event verifiers. | Hatched/static crossovers, obscure gifts/trades, special fixed-property exceptions, and complete source identity relationships remain. |
| 9 | Eggs | Partial | Production rejects pre-Gen II eggs, checks egg/source met-level/context rules, and has dedicated egg legality regression coverage. | EggVerifier plus breeding encounter generation, hatch metadata, inherited moves, species/evolution and location rules. | Egg-origin histories, breeding-source reconstruction, inherited move provenance, hatch-location/game exceptions, and egg-to-evolution history remain incomplete. |
| 10 | Event Pokémon | Partial | Gen III event data and generated Gen IV event/WC4 legality data are present with event-focused regressions. | Mystery Gift/event encounter templates, fateful/ribbon/OT/language/nickname/PID and distribution restrictions. | Exact distribution-language/region/date-like constraints where encoded, event ribbons, nickname/OT edge cases, and remaining special distributions require comparison. |
| 11 | Evolution history | Partial | Accepted engine has several evolution/provenance tranches including source-aware reconstruction and Wurmple evolution evidence. | Evolution chains/criteria integrated into encounter matching, moves, forms, levels and source context. | Deep pre-evolution chains, branching conditions, traded evolution state, egg origins, and move-history interaction are not yet comprehensively reconstructed. |
| 12 | Transfer / history | Partial | Some tradeback/transfer/source-history constraints and explicit incomplete coverage tracking exist. | Transfer verifiers/converters, encounter context transitions, Pal Park and historical metadata transformations. | Pal Park provenance, cross-game metadata transformations, move retention/loss, tradeback edge cases, and impossible post-transfer field combinations need a dedicated matrix. |
| 13 | Trainer / OT metadata | Partial | TID/SID/OT fields participate in PID/shiny/event/GameCube checks; accepted regression proves at least the Colosseum/XD female-OT impossibility centrally. | TrainerID/OT/gender/event identity and encounter-specific trainer restrictions. | General OT/TID/SID relationships, trainer gender restrictions by source, event OT identity, and trade/history interactions are not yet complete. |
| 14 | Language / text / trash bytes | Partial | Gen I/II representability logic exists and language ID `6` is rejected; text fields are read by generation-aware formats. | `LanguageVerifier`, nickname/OT string verifiers, encoding and trash-byte/history logic. | **Verified immediate gap:** accepted production currently uses a modern `<=10` language ceiling and skips zero. Pinned PKHeX caps Gen III at Spanish (`7`), Gen IV at Korean (`8`), rejects unused `6`, and rejects `0`. Therefore PK3 `0/8/9/10` and PK4 `0/9/10` can evade the language finding. Trash-byte/string-history parity also remains. |
| 15 | Ribbons | Partial | Production has structural/source ribbon checks and dedicated ribbon regression coverage. | RibbonVerifier and encounter/event/source-specific ribbon acquisition restrictions. | Full acquisition provenance, event ribbons, mutually impossible combinations, contest/history requirements, and game-specific ribbon routes remain. |
| 16 | Met data | Partial | Production checks met level, location/encounter evidence, source-game context, and numerous source-specific encounter records. | Encounter matching and met-location/level/game metadata verification. | Special met-location encodings, egg/hatch transitions, transfer rewrites, event exceptions, and full combination legality remain. |
| 17 | Balls | Partial | Source-generation and encounter-aware ball constraints exist in production; source-specific fixed-ball logic is used in several Gen III/IV paths. | BallVerifier plus encounter-template fixed/allowed ball rules. | Need a complete encounter-family ball matrix, inherited/egg rules where relevant, and all event/fixed-ball exceptions. |
| 18 | Held item / history constraints | Partial | Production checks whether the held-item value can exist in the source generation and reports unresolved coverage when source item knowledge is incomplete. | Item legality plus encounter/event/history-specific held-item constraints. | Historical item acquisition/transfer relationships, fixed event/gift items, source removals, and game-specific exceptions are not comprehensively reconstructed. |
| 19 | Lead / RNG mechanics | Partial | Strong accepted Method J/K evidence: Pressure/Hustle/Vital Spirit, Static/Magnet Pull, BCC reroll geometry, Synchronize direct/all-failed subsets, Safari/fishing/Suction Cups and deterministic lead-history reporting. | Gen IV encounter RNG/lead generators and PID verification in the pinned encounter architecture. | Mixed failed+successful Synchronize chains, failed-Synchronize nature-lock transitions, nature rejection loops, unsupported Safari lead families and other Method J/K tails remain. |
| 20 | Game-specific exceptions | Partial | Exact-game profiles and multiple generated/source-specific tables avoid treating a generation as one uniform ruleset; Gen III exact-source and Gen IV source-specific regressions exist. | Version/context-aware encounter, verifier and data tables throughout PKHeX. | Remaining obscure Gen I-II exceptions, Gen III provenance exceptions, Gen IV special encounters/events, and metadata edge cases need explicit comparison. |
| 21 | Verdict / reporting behavior | Partial | Structured findings, `Invalid`, `No Problems Found`, `Incomplete`, explicit coverage tracking, central report bridge and deterministic lead-history reporting are in accepted production. | PKHeX legality analysis exposes verifier results and encounter matching, but does not share PokeBank NX's exact tri-state product contract. | PokeBank's conservative `Incomplete` policy is intentional. Exact report taxonomy/text parity is not required, but every definitive invalid must stay source-backed and every unsupported history must remain unresolved. More permanent tests are needed around coverage-to-verdict boundaries. |

## Evidence notes

### Accepted PokeBank NX production surface

The accepted line is not a simple stat checker. Its legality production surface includes exact-game profiles, generation ceilings, source-aware move/species/ability/item rules, conservative encounter matching, structured findings and coverage, Gen I catch-rate evidence, substantial Gen III handheld/GameCube PID/IV and provenance reconstruction, generated Gen III event data, Gen IV wild/static/gift/event datasets, Gen IV Method J/K RNG reconstruction, evolution/provenance work, egg/transfer/ribbon slices, and central reporting integration.

Permanent host coverage is spread across focused legality targets rather than a single monolithic test. Existing targets cover Gen III exact context/provenance and multiple Gen IV origin, encounter, RNG, egg, transfer, ribbon, event and reporting slices. A category is still `Partial` when those tests prove only a subset of PKHeX's pinned semantics.

### Pinned PKHeX comparison surface

The pinned PKHeX commit organizes legality across dedicated verifiers and encounter/evolution/history machinery rather than one flat rule table. Relevant comparison surfaces include language, moves, abilities, balls, eggs, evolution, PID/RNG, ribbons, trainer/nickname/string metadata, encounter generators/templates and version/context-specific source data. This ledger compares semantics, not class names.

## Highest-impact verified gap selected from this ledger

The first implementation tranche after this ledger should be **Gen III/IV language ID legality**.

Why this gap is selected before another RNG tranche:

1. The mismatch is directly source-backed by the pinned `LanguageVerifier` and `Legal.GetMaxLanguageID` behavior.
2. The accepted PokeBank code path is already identified and narrow.
3. The current behavior creates false negatives: impossible language metadata can pass without a language finding.
4. The fix does not require speculative historical reconstruction.
5. Boundary tests are deterministic and cheap: Gen III `0`, `6`, `7`, `8`; Gen IV `0`, `8`, `9` (with optional `10` aliases) can be checked by `CheckIdentifier::Language` rather than overall verdict.
6. The change remains read-only and does not alter staged Pokémon, save writes, permissions, auto-fix behavior, Gen V, Master Vault, or Product UI.

Expected production rule for this tranche:

- Gen III: language `0`, unused `6`, and values greater than `7` are invalid.
- Gen IV: language `0`, unused `6`, and values greater than `8` are invalid.
- Preserve existing Gen I/II text-representability behavior; do not globally reinterpret their default/format language representation.
- Do not infer broader language/text/trash-byte parity from this fix. Category 14 remains `Partial` until string-history and trash-byte semantics are source-verified and covered.

## Priority queue after the language tranche

The next work should continue from verified evidence rather than percentage chasing. Current high-value candidates are:

1. remaining Gen IV Method J/K Synchronize/nature-rejection histories;
2. egg-origin and evolution/pre-evolution provenance reconstruction;
3. transfer/Pal Park metadata transformations and move-history constraints;
4. event/trainer/language/nickname/ribbon provenance edge cases;
5. Gen I/II and Gen III obscure encounter/data exceptions;
6. trash-byte and string-history rules after a pinned-source audit.

Each item must first be decomposed into source-backed, independently testable behavior. Unsupported histories stay `Incomplete` until that evidence exists.
