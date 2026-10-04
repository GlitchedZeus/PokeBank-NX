# Generation III WC3 legality coverage

PokeBank NX treats PKHeX's pinned `EncountersWC3.cs` catalog as source evidence, not as a blanket declaration that every matching-looking PK3 is legal. Direct event provenance is only reported when the implemented RNG class and the persistent/template fields that are actually covered by the matcher agree.

## BACD_R_A restricted anti-shiny distributions

The current generated `Gen3BacdRaEventTemplateData.inc` contains all **114** pinned `PIDType.BACD_R_A` rows from PKHeX commit `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

Each generated row retains:

- species
- TID/SID
- distribution level
- language restriction when fixed by the source
- source OT name
- source OT-gender rule reconstructed from the BA-CD origin seed
- fateful flag
- National Ribbon state (including the required FESTA/ROCKS Metang ribbon)
- absence of Country / Champion Battle / Champion Regional / Champion National ribbons for the pinned `BACD_R_A` gifts
- the exact four-move distribution-time payload

Direct encounter matching validates the persistent identity fields plus Ruby origin / event location / Poké Ball / non-egg / non-shiny invariants and restricted 16-bit BA-CD RNG compatibility.

The four source moves are deliberately **not** treated as immutable current fields. PKHeX's pinned `EncounterGift3.IsMatchExact()` does not compare current moves for non-egg gifts: a player can legally replace moves after receiving the event. PokeBank NX therefore keeps the initial moves in the generated row and exposes them through the positive match result as provenance metadata without rejecting an otherwise matching event whose current moves changed legally.

This is still **positive-only evidence**. A non-match does not by itself prove that the Pokémon is illegal because some historical distribution/recipient state is not reconstructable from a surviving PK3. Those gaps remain **Incomplete** until their individual source contracts are implemented.

## WC3 parity direction

The pinned WC3 method inventory is already represented by dedicated engine paths: MYSTRY Mew (`BACD_M`), WISHMKR (`BACD_R`), Channel, Berry Fix (`BACD_RBCD`), Negai Boshi (`BACD_TA` / `BACD_U_AX`), `BACD_R_A`, Pokémon Box (`BACD_U`), PCJP 5th Anniversary (`BACD_TA` / `BACD_TS`), Wondercard event eggs (`Method_2` with source-supported Method 1/4 outcomes), PokéPark DS Download (`BACD_R`), plus PCJP/PCNY machine gifts.

The remaining WC3 parity work is increasingly about template depth and historical constraints rather than simply recognizing another PID method: extending fixed event-ribbon parity beyond the now-covered `BACD_R_A` family, mutable-vs-immutable move provenance, held-item derivation where applicable, evolved-event reconstruction, recipient/trade history, and other fields that can still be proven from a surviving Pokémon. Earth Ribbon remains separate because a Gen III Pokémon can acquire it later through GameCube cross-transfer history.
