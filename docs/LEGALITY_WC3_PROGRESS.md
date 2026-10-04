# Generation III WC3 legality coverage

PokeBank NX treats PKHeX's pinned `EncountersWC3.cs` catalog as source evidence, not as a blanket declaration that every matching-looking PK3 is legal. Direct event provenance is only reported when the implemented RNG class and the persistent/template fields that are actually covered by the matcher agree.

## BACD_R_A restricted anti-shiny distributions

The current generated `Gen3BacdRaEventTemplateData.inc` contains all **114** pinned `PIDType.BACD_R_A` rows from PKHeX commit `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

Each generated row now retains and validates:

- species
- TID/SID
- distribution level
- language restriction when fixed by the source
- source OT name
- source OT-gender rule reconstructed from the BA-CD origin seed
- fateful flag
- Ruby origin / event location / Poké Ball / non-egg / non-shiny invariants
- restricted 16-bit BA-CD RNG compatibility
- the exact four-move distribution payload

A candidate with the correct PID/IV seed, trainer identity and event metadata but a different move in the four stored move slots no longer receives positive direct `BACD_R_A` event provenance.

This is still **positive-only evidence**. A non-match does not by itself prove that the Pokémon is illegal because `EncountersWC3.cs` contains additional Gen III distribution families and some historical distribution/recipient state is not reconstructable from a surviving PK3. Those gaps remain **Incomplete** until their individual source contracts are implemented.

## Next WC3 parity work

Continue inventorying the remaining pinned `EncountersWC3.cs` method families against the already implemented MYSTRY Mew, WISHMKR, Channel, Berry Fix, Negai Boshi, event-egg, PCJP/PCNY machine-gift and `BACD_R_A` paths. Add each uncovered family with source-derived template fields and regression vectors rather than broad permissive event matching.
