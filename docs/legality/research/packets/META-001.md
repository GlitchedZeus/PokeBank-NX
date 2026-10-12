# META-001 — Native ribbon chains and raw-text capability boundaries

Research ID: **META-001**
Affected generations/games: III–IV with immediate bounded IV ribbon work
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`
Estimated complexity: **Medium**
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`Pokemon::Pokemon` exposes a few Gen III event ribbons with default false values; `Legality.cpp` has bounded Gen III egg/event checks, not a general contest-rank verifier. The common interface has decoded OT/nickname strings but no general raw trash-byte evidence API. “False”/empty can mean unwired. This blocks safe direct translation of PKHeX metadata checks.

## Authoritative sources

- **NX-FIELDS**: [include/Pokemon/Pokemon.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Pokemon/Pokemon.h); `ribbonNational`; grade B.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.
- **PK-RIBBON**: [PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierUnique4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierUnique4.cs); `AddMissingContest4`; grade B.
- **PK-RIBRULE**: [PKHeX.Core/Legality/Verifiers/Ribbons/RibbonRules.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/Ribbons/RibbonRules.cs); `IsAllowedContest4`; grade B.
- **PK-TRASH**: [PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs); `VerifyTrashBytesPalPark`; grade B.

## Immediate implementation brief

The smallest useful bounded verifier is Gen IV contest-ribbon prefix consistency. `RibbonVerifierUnique4.AddMissingContest3/4` requires earlier ranks whenever a higher rank is present. For each category use a four-bit rank vector; valid prefix patterns are `0000,0001,0011,0111,1111` (lowest rank at bit 0). A `1000` Master-only value contradicts the rank chain when the native fields are known and the normal acquisition model applies.

Required data is the actual PK4 ribbon bit mapping and a capability flag proving all four bits are decoded. Add `Known(value)` / `Unavailable`, not false as a sentinel. Native PK3 contest ribbons use rank counts, so do not apply a four-bit layout to PK3. Tests must originate in encrypted native records, not only a mock interface.

## Important exceptions and risks

`RibbonRules` permits Gen III contests across reachable Gen III history; Gen IV contests exclude Ditto, Unown and spiky-eared Pichu. Do not reject an HGSS-hosted Pokémon's Sinnoh contest ribbons merely because HGSS has no Super Contest. It could have traded. Gen III contest ribbons can survive Pal Park. Event ribbons and Battle Frontier eligibility require different rules. There is no general preserved ribbon-acquisition timestamp, so chronology beyond logical rank prerequisites is not reconstructable.

PKHeX `TrashByteVerifier` dispatches PK4 PCD and Pal Park separately. PCD OT bytes are compared with the template; nickname comparison is skipped after species evolution. Pal Park OT trash rules distinguish Japanese vs non-Japanese. Do not apply these checks to decoded strings or to missing raw buffers, and do not infer that all unused bytes should be zero.

## Inputs / lost evidence

Need raw native ribbon bytes and text buffers, language, species/form, origin and candidate kind. Current decoded text loses terminator/trailing data. Trade/evolution history is incomplete; renaming may rewrite nickname bytes. Preserve read-only buffers and mark capability unavailable rather than reconstructing bytes from Unicode.

## Tests and expected reporting

- Prefix 1111: no rank-chain contradiction; not overall legal.
- Prefix 1000 or 0101: candidate contradiction after complete raw decoding.
- Same malformed bits behind an unavailable capability: Incomplete, never Invalid.
- PCD direct gift with known matching OT bytes: compatible; evolved nickname rewrite must not be rejected by direct-template nickname comparison.
- Pal Park Japanese/non-Japanese raw-byte cases require independent transfer fixtures before production hard Invalid.

## Acceptance criteria / order

First field-capability API and native bit mapping, then contest-rank positives/negatives, then source eligibility, then event and trash-byte rules. Add parser→view→analyzer regression plus unknown-capability controls. The source-backed prefix rule is ready; raw-text hard negatives remain further research. No new false-Invalid bug is claimed in the current production path from this packet.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
