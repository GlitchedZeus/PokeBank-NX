# Generation III WC3 legality coverage

PokeBank NX treats PKHeX's pinned `EncountersWC3.cs` catalog as source evidence, not as a blanket declaration that every matching-looking PK3 is legal. Direct event provenance is only reported when the implemented RNG class and the persistent/template fields that are actually covered by the matcher agree.

Pinned reference: PKHeX commit `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

## BACD_R_A restricted anti-shiny distributions

The generated `Gen3BacdRaEventTemplateData.inc` contains all **114** pinned `PIDType.BACD_R_A` rows from the pinned PKHeX reference.

Each generated row retains:

- distribution species
- TID/SID
- distribution level
- language restriction when fixed by the source
- source OT name
- source OT-gender rule reconstructed from the BA-CD origin seed
- fateful flag
- National Ribbon state, including the required FESTA/ROCKS Metang ribbon
- absence of Country / Champion Battle / Champion Regional / Champion National ribbons for the pinned `BACD_R_A` gifts
- the exact four-move distribution-time payload

Direct encounter matching validates the persistent identity fields plus Ruby origin / event location / Poké Ball / non-egg / non-shiny invariants and restricted 16-bit BA-CD RNG compatibility.

The four source moves are deliberately **not** treated as immutable current fields. PKHeX's pinned `EncounterGift3.IsMatchExact()` does not compare current moves for non-egg gifts: a player can legally replace moves after receiving the event. PokeBank NX therefore keeps the initial moves in the generated row and exposes them through positive provenance without rejecting an otherwise matching event whose current moves changed legally.

Source-species provenance now survives the Gen III evolutions that can follow receipt for the evolvable species present in this pinned `BACD_R_A` catalog. The mapping is directional: a distributed ancestor can become its valid descendant, but a surviving descendant cannot reconstruct backward into a pre-evolution. This is positive source-species provenance only; exact evolution timing/history remains incomplete.

## PCNY machine gifts

The generated PCNY table contains all **50** pinned rows from `encounter_pcny.pkl`, covering all 12 source distributions. Matching retains the pinned Ruby/Sapphire origin, distribution level, event location, Poké Ball, non-egg/non-shiny state, SID/TID range, allowed PCNY trainer name and forced anti-shiny BA-CD class.

Pinned `EncounterGift3NY.IsMatchExact()` is evaluated with an `EvoCriteria`, so the surviving PK3 is not required to remain the distributed species. PokeBank NX now preserves source-species provenance through every descendant reachable in Generation III from an evolvable species actually present in the 50-row catalog, including branched lines such as Gloom -> Vileplume/Bellossom. Reverse histories remain rejected.

Exact evolution timing, level-range history and unreconstructable distribution-machine selection remain **Incomplete**. A descendant match is therefore partial event provenance, not a claim that the full historical chain has been proven.

## PCJP machine gifts

All **58** pinned PCJP machine-gift source species across the six Japanese distribution groups retain their fixed distribution TID, Japanese city-OT policy, Ruby origin, Japanese language, level 10, event location, Poké Ball, SID 0, non-egg/non-shiny state and BA-CD-derived OT-gender rule.

Pinned `EncounterGift3JPN.IsMatchExact()` also receives an `EvoCriteria`. The matcher therefore now preserves source-species provenance through descendants that are actually reachable in Generation III, while retaining the exact original TID/distribution table. Later-generation-only evolutions such as Electivire, Magmortar, Honchkrow, Mismagius, Weavile, Gliscor, Ambipom and Roserade are deliberately excluded because they cannot exist in a PK3.

Focused host and sanitizer regressions cover all Gen III-reachable PCJP descendant families, one complete evolved positive vector from every distribution, cross-distribution negatives, the sixth-distribution Sapporo exclusion and the existing event RNG-class boundary.

As with PCNY, this proves compatible source-species provenance; exact evolution timing/history remains **Incomplete** unless separately reconstructed.

## Event3 ribbons

PK3 persisted Event3 readers expose Champion Battle, Champion Regional, Champion National, Country, National and Earth Ribbon independently from the fateful/obedience bit. Those fixed fields are used where a pinned WC3 template makes them invariant.

Earth Ribbon remains deliberately separate from fixed non-egg event-template matching because a legitimate Gen III Pokémon can earn Earth later through Pokémon Colosseum/XD history. Pinned PKHeX source also proves a narrower rule for **unhatched Gen III event eggs**: an unhatched egg may not already carry any of the Event3 ribbons, including Earth. That egg-only invariant is now integrated through the exact Generation III source profile using `Gen3EggEventRibbonEvidence.h`, with Invalid egg diagnostics for each present ribbon. Focused normal/sanitizer regressions cover all six readers, all ribbon combinations, exact-source exclusions and unchanged input bytes. It must not be implemented as a blanket Earth-Ribbon prohibition on normal hatched/event Pokémon.

## WC3 parity direction

The pinned WC3 method inventory is represented by dedicated engine paths: MYSTRY Mew (`BACD_M`), WISHMKR (`BACD_R`), Channel, Berry Fix (`BACD_RBCD`), Negai Boshi (`BACD_TA` / `BACD_U_AX`), `BACD_R_A`, Pokémon Box (`BACD_U`), PCJP 5th Anniversary (`BACD_TA` / `BACD_TS`), Wondercard event eggs (Method 2 with source-supported Method 1/4 outcomes), PokéPark DS Download (`BACD_R`), plus PCJP/PCNY machine gifts.

Remaining WC3 parity work is increasingly about historical/template depth rather than recognizing another PID method: additional fixed event-ribbon families, held-item derivation where source-backed, recipient/trade history, exact evolution timing/history, trash-byte/text restrictions and other persistent evidence that can still be proven from a surviving Pokémon.

All of these paths remain **positive-only evidence** unless an invariant is independently impossible. A provenance non-match does not automatically make a Pokémon Invalid; history that cannot be proven remains **Incomplete**.

## Accepted checkpoint

Last fully green legality-code checkpoint: `14ae3382cf39e07ddca58af985b6c0ca958b54c1`.

PokeBank NX Host Tests **#2478** passed exact pull-request identity, the complete host suite, focused RSE regression and ASan/UBSan on that exact code head.

## Secondary reference audit

See [ALM 23.09.25 Gen I–IV audit](audit/ALM_23_09_25_GEN1_4_AUDIT.md) for the pinned-source comparison, corpus changes, remaining source-backed gaps and read-only provenance-trace design. This is not an auto-legalization integration or parity claim.
