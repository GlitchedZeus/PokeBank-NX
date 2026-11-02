# Gen I–V rule inventory against official PKHeX26.10.18

Reference: [release26.10.18](https://github.com/kwsch/PKHeX/releases/tag/26.10.18), source `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`. Frozen older baseline: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`. SOURCE_PINS.json binds the principal inspected files; the evidence archive retains both source comparisons. This is a rule-family gap inventory, **not a completed branch-by-branch proof of parity**. An unreviewed condition is a gap even when an implementation exists. No rule-completeness percentage is calculated.

Status meanings: **Equivalent** is limited to the named condition and represented record formats; **Evidence** is supported by the cited original-game construction; **Represented** means code exists but all current-release branches are not independently certified; **Partial/Missing** identifies a concrete unresolved obligation. A family's positive differential results do not upgrade its status. `NoProblemsFound` means the checks for that record completed without a problem; it is not historical authentication.

Twenty-title applicability: G1 = Red/Blue/Yellow; G2 = Gold/Silver/Crystal; G3 = Ruby/Sapphire/Emerald/FireRed/LeafGreen; G4 = Diamond/Pearl/Platinum/HeartGold/SoulSilver; G5 = Black/White/Black2/White2. Japanese Red/Green/Blue/Yellow and Japanese/Korean/international language variants are additional regional dimensions, not substitutes for title coverage. Colosseum/XD, Stadium, Ranch, Ranger, Dream Radar and event origins are ancestry sources; they do not expand the twenty-title denominator. A G3/G4-origin PK5 inherits the applicable origin obligations.

The older detailed inventories remain symbol/specification references: `GB_PKHEX_RULE_INVENTORY.md`, `GEN3_PKHEX_RULE_INVENTORY.md`, `GEN4_PKHEX_RULE_INVENTORY.md`, `GEN5_PKHEX_RULE_INVENTORY.md`. Their historical “Implemented”, “complete” and app-wiring statements do **not** certify this release or current app PR307.

| ID | Titles | Rule / current PKHeX source family | Engine condition and remaining gap |
|---|---|---|---|
| F01 | G1–5 | Native size, decode, encryption, checksum, sanity (`LegalityAnalysis`, PK1–PK5) | Represented. GB has no checksum. Malformed or context-ambiguous records cannot become clean. Native adapters and quarantine gates cover this; all original save encodings are not independently authenticated. |
| F02 | G1–5 | Empty slot versus species zero | Equivalent for tested native blank PK5. Nonzero species-zero payload is Incomplete. All GB/PK3/PK4 slot representations still require per-save trace. |
| F03 | G1–5 | Exact source game / personal table | Represented by `Legality.cpp`, native parsers and `Gen5AppAdapter`. Native III/IV preserve independently known host-title coverage through uncertain histories. Unknown Gen IV titles stay Partial even with zero findings; a default personal table cannot certify the caller context. Generated record origin is not proof of source save title. |
| F04 | G1–5 | Party/box provenance and original/staged bytes | G5 boundary equivalent for tested136/220-byte records; source enum, identity, current trainer and slot geometry checked. Bank ownership and every active UI caller still require integration. |
| E01 | G1–2 | Wild level/location/version candidate search (`EncounterGenerator12`, GBRestrictions) | Represented in GBLegality. Both Time Capsule directions searched. New raw differences remain; do not infer exhaustive native encounter evidence from the catalog. |
| E02 | G1 | Japanese Blue, Yellow-specific slots and catch rates | Represented, native RBY table gates. Regional-name and alternative-history adjudication remains per record. |
| E03 | G2 | Time of day, Headbutt TID/tree, fishing, swarm, static/trade | Represented. Regional/native table and current source-site crosswalk not fully certified. |
| E04 | G3 | Wild slot methods, Safari, fishing, swarm, Rock Smash | Represented in Gen3Legality; cap exhaustion is uncertain. All lead/slot RNG histories need independent source review. |
| E05 | G3 | Static, starter, fossil, gift, in-game trade templates | Represented. Catalog completeness and event egg exceptions remain independently unproven. |
| E06 | G4 | DP/Pt/HGSS wild slot and encounter-type search | Partial. Retained newer-reference82 encounter-type,6 ball and4 frame clean disagreements from the prior corpus remain open; the terrain-byte repair does not resolve these families. HG/SS rod and contest-only grass placeholders are now corrected with native map/flag evidence; remaining resource drift and all other frame/encounter alternatives still need a typed comparison. |
| E07 | G4 | Great Marsh, Safari Zone, Trophy Garden, swarm, Radar, radio | Represented; no exhaustive original-source proof across all combinations. |
| E08 | G4 | Pokewalker | Represented by templates and reversal. New source changes inspected are arithmetic/comment refactors; all route/encounter provenance is not authenticated. |
| E09 | G4 | Ranch fixed trainer, language, level, ribbons, ball, moves | Represented. Original Wii encounter/distribution evidence absent; do not authenticate a Ranch gift from template agreement. |
| E10 | G4 | Ranch MRNG IV first/third outputs, independent PID | Model-equivalent exhaustive reversal in Gen4RanchIV.h; no-state becomes Incomplete. NativePK4 and transferredPK5 controls retained. Original Wii generation evidence needed before a definitive Invalid. Native PK4 Ranch ability/PID-slot contradictions also stay Incomplete; ordinary PID enforcement is retained after encounter selection. |
| E11 | G5 | Wild doubles, shaking grass, dust/water, fishing/surfing | Represented. Fresh differential discrepancies and every native seasonal/version table branch still need adjudication. |
| E12 | G5 | Static/roamer/gift, N's Pokémon, Hidden Grotto, trades | Represented. Evolved fixed-slot trade handling repaired: current ability must satisfy PID after evolution; the original fixed slot still constrains unevolved trades. Public Snorlax and both negative directions retained. |
| E13 | G5 | Dream Radar (`Encounters5DR`) | Represented. Language/version pairing is exported in Global Link audit where applicable; native3DS distribution custody absent. |
| E14 | G5 | Global Link recipient language, version, gender, ability, special move | Partial, substantially repaired.774 current typed rows; all762 old Entree rows paired by represented identity;3 added represented variants. Unsupported recipient/move alternatives are Incomplete. No filename/species-only whitelist. |
| E15 | G5 | Global Link date/account/game/device history | Missing original distribution receipt evidence. PK5 metadata alone cannot authenticate receipt. |
| E17 | G3 | Original cartridge versus Switch-exclusive event-ticket/cross-trade settings | Partial profile equivalence. New ParseSettings separates these contexts. Ordinary original-game reports must not assume Switch-exclusive item/ticket constraints or treat optional ticket availability as authenticated receipt. |
| E16 | G3–5 | Candidate priority / alternative encounter history | Represented. Supported complete alternatives win; uncertain move history cannot be discarded to invent Invalid. Full all-family priority equivalence still unproved. |
| B01 | G2 | Native four-slot donor, sex, Ditto, maternal groups, evolution | Evidence-backed finite solver integrated in GBLegality, no depth cutoff; independent pret witness checks in completion-session2. Some gift-dependent witnesses remain Incomplete. |
| B02 | G2 | Recursive previous-era TM/level/tutor inheritance | Represented and source-backed for reviewed native routes, including Stadium2 reminder and three tutor/TM combinations. Twelve old chain cases all retained; no claim that every historical gift is known. |
| B03 | G3 | Native inherited slot/order, move source, event-egg distinctions | Partial production integration: ordinary exact eggs and hatched inherited survivors require simultaneous parent certificates. Bounded recursive native routes now include the five-title Chikorita Flail chain. Missing/event/cross-era lower witnesses remain Incomplete; no failed search proves impossibility. Full ancestry and original regional birth writers remain uncertified. |
| B04 | G4 | Native inherited move slots, HGSS/Pt sources, Nidoran/Volbeat PID | Partial production integration: exact eggs, hatched records and Gen III/IV-origin PK5 histories require joint ancestry, beyond per-slot source unions. Five-title Tangela Flail controls have recursive certificates. Complex/shared/event/cross-era chronology and all original regional writers remain incomplete. |
| B05 | G5 | BW/B2W2 inheritance, hidden ability, incense, male/genderless | Partial production integration for ordinary exact and hatched PK5, with bounded recursive four-slot positive certificates. Existing Munchlax incense history is preserved before alternate search spends the budget. All matching ordinary hatched acquisitions are considered. Seven original shared-level uncertainties, native BW paired-species role dispatch and broader regional/form/event histories remain Incomplete. Published English B2W2 child-order proof: `../campaign-20261028/BW2_BREEDING_REPAIR.md`. |
| B06 | G2–5 | Separate event individuals as donor seeds | Gen III–V finite upper models preserve distinct gift seeds; constructive ordinary lower witnesses are integrated in native III/IV and exact/hatched V. General historical event ancestry is still uncertified and cannot silently become clean. G2 work remains owned separately. |
| B07 | G3 | Unhatched ordinary/event eggs, native met level | Represented; new valid-reference egg mutations are rejected by the older structural guard. Original-game exception adjudication pending; no automatic waiver. |
| B08 | G4 | Unhatched/hatched/traded egg met and egg-location state | Partial. Basic state represented. Current hatch-map flag and traded hatch-game ball reconstruction repaired. Native HGSS/Pt routines rebuild on hatch and preserve origin version; public Pt→HG/HG→Pt positives and contradictory-ball negatives retained. DP native hatch routine and all regional cases still need independent review. |
| B09 | G5 | Egg met location, level1, nickname, hatch counter, PP and stats | Represented. Gift recipient/hatch-language exception remains separate. Recursive donor completeness not implied. |
| M01 | G1–5 | Move range, duplicates, empty first slot | Represented in all native analyzers. Existing structural and title gates exercise these invariants. |
| M02 | G1–5 | Level-up chronological acquisition / evolution restrictions | Represented. Cross-era maximum/minimum history and all four-slot combinations remain independently unaudited. |
| M03 | G1–5 | TM/HM and tutor source/version availability | Represented with native RBY parity. HGSS table drift and current GS eggmove resource require typed comparison. |
| M04 | G2–5 | Sketch / Smeargle, event-only moves, same-source constraints | Represented local learning; general G3–5 joint ancestry missing. Historical gift incompleteness cannot prove impossibility. |
| M05 | G3–5 | HM removal and transfer move chronology | Represented in origin analyses. All III→IV→V evolved/traded alternatives and native limits need further fixtures. |
| M06 | G1–5 | PP limits, PP Ups, egg base PP, empty-slot PP | Represented. Box-only healed-state rules require trustworthy slot ownership. GenIV boxed PP remains Warning where Wii/native storage behavior is unproved. |
| R01 | G1–2 | DV ranges, derived HP, gender, shiny, Unown | Represented, nativeDV/stat source gates. Event-specific DV profiles remain partial. |
| R02 | G3 | PID/IV Method1/2/4/H, seed/lead/slot feasibility | Represented; exhaustion stays Incomplete. No unbounded historical source proof for every represented event method. |
| R03 | G3–5 origins | BACD, table gifts, MYSTRY, WISHMKR, Channel | Represented. Source changes to Channel helper arithmetic do not establish new event custody. |
| R09 | G3 | Unhatched inherited-level-up source versus conditional species/PID bit | Repaired source propagation from the existing ordered birth validator. Both-parent level-up sources reach the existing PID check; lower-source/Ditto alternatives are preserved. Four permanent source/reference controls; native Emerald mechanism independently pinned. Full recursive donor ancestry still missing. |
| R04 | G3–5 origins | Berry Fix RTC seed range and dual-OT gender | Equivalent to released model0..213 after repair; old3 minimum removed.72 JP/English constructions acrossPK3/4/5 retain mismatched gender and214 negatives. Original distribution executable not authenticated; equivalence is not original-game proof. |
| R05 | G3–5 origins | Colosseum/XD shadow locks, shiny locks, e-Reader, starter TID/SID | Represented native GameCube judge and transfer bridge. Undecided/capped judge stays Incomplete. Need acquired regional GameCube saves and encounter receipts. |
| R06 | G4 | MethodJ/K, Cute Charm, Radar chain shiny | Represented. Four retained newer frame cases unresolved. New CuteCharm/MethodK/Pokewalker diffs are mostly refactor/comment; full behavior still needs independent equivalence proof. |
| R08 | G3–5 origins | Optimized LCRNG/gapped/XDRNG/Channel seed reversal | Latest source uses new lattice bounds. Existing native reversal solves the same model; complete seed-set equivalence has not been independently proven. Do not change algorithms for test counts or call this a new native acquisition rule. |
| R07 | G5 | Wild PID correlation, fixed PID/nature/gender, shiny locks | Represented. It does not implement a universal GenV PID/IV correlation that the games do not impose. |
| A01 | G3 | Ability bit, duplicate abilities, GameCube mismatch | Represented with native transfer ability interpretation. Full fixed-template evolution cases need current reference review. |
| A02 | G4–5 origins | GenIII one-ability retention and later evolution | Partial. Preserves uncertainty when GenIII→IV/V history is not identified. All transfer/evolution orders not certified. |
| A03 | G4 | Stored ability versus PID, fixed encounter slot | Represented. Original and transferred checks run; all fixed-template evolved branches need audit. |
| A04 | G5 | Hidden flag versus actual hidden ability | Unsupported Scizor/Basculin/event ability histories preserve explicit Partial independently of warning text. Gen III/CXD origins reject both the hidden flag and a hidden-only ability value before origin delegation; the literal failed-CI evolved Lairon value-only record and both ordinary ability controls are retained. Full native Gen V ability-writer/event chronology remains incomplete. |
| A05 | G5 | Fixed first/second slot after evolution | Released VerifyFixedAbility evolved branch repaired for normal slots: evolved PID-slot match, unevolved fixed-slot and hidden permission all retained. Public evolved Munchlax→Snorlax is a permanent regression; original GenV executable proof remains absent. |
| A06 | G5 | Blue-Striped Basculin origin/egg/trade exceptions | Represented; ambiguous hidden ability histories deliberately Incomplete without native proof. |
| V01 | G1–2 | Stat EXP vitamin/battle invariants | Represented native rule families. Fresh stricter and lenient disagreements remain individually reviewable. |
| V02 | G3–5 | EV total510, native per-stat byte, eggEV0 | Represented. No unsupported GenVI252 cap imposed on native formats. |
| V03 | G3–4 | Zero gained EXP versus battle-only EV | Joint production solver preserves the zero-EXP contradiction under known zero initial EVs, including vitamins, berries, held items, Pokerus, EXP Shares and native caps. Unknown initial gift/Orre histories cannot use this premise to establish either impossibility or a clean history. |
| V04 | G3–4 | New minimum required EXP for aggregate EVs | Partial production integration: joint six-stat action witnesses and necessary global 12/14 EV-per-action bounds replace independent per-stat minima. Native HGSS HP+1/ATK+4 for 1 EXP remains legal. Positive catalog/search exhaustion is Incomplete. General action catalogs, Orre deferred shadow EXP, native gift initial states and chronological move preservation remain uncertified. |
| V05 | G3–4 | Power item and multi-recipient EV/EXP accounting | **Evidence-backed improvement** for the explicit native one-battle counterexample in ev_exp.txt. Actual HGSS EXP split and EV body compiled unchanged with pinned encounter/item evidence. Not a proof of full solver correctness or hardware execution. |
| V06 | G1–5 | Level bracket, EXP cap, encounter minimum, DayCare stale level | Represented. Native stored-level checks can be stricter than reference. Public PK1 files with zero party level need storage/custody adjudication, not silent normalization. |
| N01 | G1–5 | Name buffers, terminators, empty names, language limits | Represented, including rawGB and PK5 buffers. Native/regional exceptions and original event trash evidence remain incomplete. |
| N02 | G1–2 | Japanese punctuation, glyphless bytes, Korean text/link rules | Reviewed native routes in session2; historical Korean link/PCNY custody unresolved. Fresh differences remain raw, not globally approved exceptions. |
| N03 | G3–5 | Word filters / foreign species names / trade names | Partial. Regex subset is not full .NET equivalence; fresh raw clean differences include filter complaints. Original-game filter applicability must be adjudicated before copying modern filters. |
| N04 | G3–5 | Trainer ID, gender, fixed OT, RS seed reachability | Represented. Fishy heuristics are not proof of impossibility. GenV omits several non-definitive heuristics; Fishy counts must therefore be separate. |
| N05 | G4 | Korean GTS active-trainer restrictions | Missing save-language/session context at low-level PK4 interface. Source game ID alone is insufficient. |
| I01 | G1–5 | Catch-rate held-item ambiguity; legal held items | Represented. GenIII Enigma evidence must come from the same original/staged save; five retained missing-save specimens remain Incomplete. |
| I02 | G3–5 | Ball per encounter, Safari/Sport/Cherish/Park, egg Poke Ball | Represented except named newerG4 traded-hatch/ball gaps and current data drift. FRLG Switch-exclusive inventory profile is not silently applied to originalGBA saves. |
| I03 | G4 | DPPt/HGSS split ball and egg hatch game | Newer hatch-game branch repaired with native hatch rebuild evidence. Origin version alone cannot select the ball writer. Ranch and Wii origins still need original writing evidence. |
| I04 | G3–5 | Forms, item-linked forms, battle-only/stored forms | Represented, source-game personal table chosen. Public DP-origin Rotom needs actual host save identity before calling a form mismatch an engine defect. |
| I05 | G3–5 | Contest stats/sheen/rank correlations | Represented. Current contest diff is parentheses only. Native joint feeding/evolution histories not comprehensively proven. |
| I06 | G3–5 | Common, unique, event and shadow ribbons | Represented. New GenIII World interface remainsfalse for represented gifts. BattleTowerTeamSwap is a separately enabled glitch profile, not presumed ordinary legality. |
| I07 | G2–5 | Pokerus, fateful, date validity, markings | Represented ordinary invariants. RTC dates do not authenticate distribution availability. No blanket release-date Invalid from an arbitrary clock. |
| I08 | G4–5 origins | Ground tile native range and template mask | **Evidence-backed stricter** byte range;25..255 cannot alias a valid bit. Original native HGSS terrain evidence and positive/negative controls retained. This does not solve encounter-type semantics. |
| I09 | G4 | Paired D/P and Pt/HGSS met fields | **Evidence-backed stricter** coupled setters for reviewed ordinary native records. Eggs/Ranch/PCD exceptions kept distinct.45 old plus17 fresh source-reviewed contradictions remain raw stricter results. |
| T01 | G1–2 | Time Capsule moves/types/items and alternative origin | Represented. No generic rejection when a native alternative exists. Historical language transfer restrictions remain partial. |
| T02 | G3→4 | Pal Park location, level, ball, HM, ability, ribbon | Represented through production origin analysis; exact native storage/receipt provenance remains partial. |
| T03 | G4→5 | Poke Transfer location/HMs/egg rejection/ability/names | Represented with Gen4 origin analyzer and GameCube judge. All combinatorial histories and ordinary transfer combinations still need further native trace. HasOriginalMetLevel equals the old predicate for valid native1–5 versions; its new difference is modernVC3. |
| T04 | G3→4→5 | Cross-generation evolution and moves | Partial. Fresh base corpora include transfers, but are not authenticated complete play histories; add independent saved-play fixtures. |
| H01 | G1–2 | Mew/Celebi/Stadium/Mobile/PCNY events | Partial. No complete authenticated original distribution catalog. Native GSBall availability evidence is not an authenticated individual receipt. |
| H02 | G3 | Regional events, egg gifts, unique RNG/OT | Partial. Source catalog and synthetic witnesses exist; original distributor binaries and real custody largely absent. |
| H03 | G4 | PCD/PGT, Ranger Manaphy egg, region/language/ribbons | Represented catalog rules. Native receipt evidence and exact event scope incomplete. |
| H04 | G5 | PGF payload identity and paired language/game metadata | Model-equivalent pairing for709 frozen payloads;3515 old unsupported claims deliberately became Incomplete. Actual receipt scope is not authenticated by metadata. |
| H05 | G5 | WC5FULL game flags and metadata conflicts | Evidence reader and81 original released/debug files retained; unsupported0x83/0x23 World11 conflict unresolved. No new authenticated distributor receipt. |
| H06 | G3–5 | Unmatched fateful events | Partial.298 old unmatched fateful records plus9 other histories remain Incomplete; missing catalog coverage is not proof of impossibility. |
| P01 | G1–5 | Invalid / clean / Incomplete aggregation | Exercised nine-area lattice keeps zero-warning Partial as Incomplete and Invalid above uncertainty. Native III/IV merges retain authoritative incomplete coverage over stale Complete reports while preserving independently known title identity; same-text Warnings cannot hide Invalid. Origin5 clean requires an explicit breeding certificate. Gen III RNG-cap aggregation now preserves audited native PP maxima and EV510 contradictions; the retained actual capped XD pair remains unknown/Invalid respectively. Other findings still require a dependency-by-dependency audit; no universal aggregation-completeness claim. |
| P02 | G1–5 | Actual current classic app report wiring | **Missing integration** on PR307 d6cc878a. GenI/II/III active panels areNotChecked, GenIV/V Incomplete placeholders. Old branch call graphs cannot establish current application wiring. |
| P03 | G5 | Current staged trainer, slot, title and checked identity | Engine/app bridge tested on allfour exact titles and staged edits; native renderer call, complete issue paging and deployment remain missing. |
| X01 | nativeG1–5 | Memories, relearn, EC, hypertraining, marks, HOME/FRX/LGX modern conversion | Not applicable to native format1–5 outputs where fields/contexts do not exist. OriginalFR/LG Switch-exclusive profile is a distinct requested-context question, not a license to add GenVI–IX work. Shared diffs still require applicability review. |
| X02 | G1–5 | Bulk duplicates/trainer handlers/settings | Bulk-save heuristics, nondefault glitch settings and modern handlers are separate from ordinary individual native reports. Their applicability is not silently counted as implemented. |

Remaining audit work: independently trace **every applicable result-producing source branch** to these rows, its native implementation, positive/negative fixtures and title/region/context applicability. The existing source-site inventory intentionally leaves uncertified fields unknown; it is not a semantic denominator. In particular, unchanged upstream files are not automatically proven equivalent, generated helpers are not production wiring, and green L1–L6 gates do not close the named missing rules.


## October31 production milestone (PR314, ca03ebcf)

Native PK3/PK4 ordinary egg checks now require a simultaneous parent certificate;
hatched PK5 tries all matching encounter alternatives and certifies ordinary
parent births through the actual version-specific four-slot writer. Exact
unhatched and inherited-survivor hatched queries share the GenIII–V API.
Recursive positive traversal has cycle/depth/visit guards; unsupported complex,
event and cross-era lower witnesses remain Incomplete. This supersedes the
“not integrated” wording in B03/B04/B05 for these scoped adapters, and does not
certify complete recursive ancestry. Seven retained original uncertain eggs remain.

V04 joint EV/EXP now has production native III/IV action certificates, six-stat
vitamin/berry/item/Pokerus/clamp modeling, a global12/14 EV/action necessary
bound, explicit exhaustion and incomplete-history outcomes. Source-based battle
catalogs are sufficient positive subsets; their absence cannot prove Invalid.
The initial milestone used exact event/Orre zero-EV models. The current audit
withdraws native Orre clean certification for non-vitamin EV histories until
initial EV/deferred shadow EXP evidence is established. Transferred/later-era
training remains a separate gap.
Original HGSS HP+1/ATK+4 for1EXP remains accepted. Full encounter-action and
chronological move preservation certification is still incomplete.

All38 retained GenV name disputes now produce Incomplete, without declaring the
new Switch lookback expressions native-game prohibitions. All205 newly added
source expressions are represented as supplemental uncertainty;38 benign rename
controls stay clean. All647 reference exceptions reproduce in a diagnostic-only
copy as MoveBreed5 array-index failures and are excluded from successful-reference
agreement. Original raw measurements and records remain preserved.

## November 1 continuation after the verified CI repair

The build-only duplicate translation-unit repair and separate Ranch/Gen III-origin
hidden-ability repairs pass all 24 workflows / 49 jobs at `3f620c79`. This evidence
certifies that exact head, not subsequent changes. The follow-up production work
is recorded in `../major-20261031/COVERAGE_INTEGRITY_EVIDENCE.md`; the rows above
distinguish newly wired scoped certificates from missing family coverage.

The published BW2 EV-item helper independently demonstrates the no-EXP wing/berry
operations for its stated domain. HP increases on Shedinja, the reconstruction's
48/51 dispatcher-parameter discrepancy and lack of an independent ROM rebuild
remain explicit limitations. It is not a universal Gen V EV-history certificate.
No new authenticated real-acquisition specimens or deployed application reports
were established by this milestone. PR307/main and the Gen I/II and modern
legality branches remain separate.
