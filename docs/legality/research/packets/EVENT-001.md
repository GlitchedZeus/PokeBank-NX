# EVENT-001 — Event template projection loses moves, text and ribbons

Research ID: **EVENT-001**  
Affected generations/games: III–IV events, with exact field audit of IV WC4/PCD  
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`  
Estimated complexity: **Medium for projection; High for historical closure**  
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`Gen4EventTemplate::EventTemplate` stores species, IDs, PID, met data, ball/form/gender/language/version/OT gender/fateful/card ID, but **no moves, OT/nickname bytes or ribbons**. `tools/gen_legality_gen4_events.py` extracts 247 Pokémon PCD templates and intentionally drops those other fields. Analyzer matching is positive-only. PR #223 adds detached-source wiring of the same matcher; it does not fill these data gaps. PR #222 is now accepted; neither pending nor accepted wiring should be counted as complete event legality.

## Live development reconciliation

Accepted source freeze advanced to `4d3026e0`; four-slot game-family coverage and guarded retained Gen III paths are now implemented. Pending #247 adds full Emerald level-up method evidence and #249 adds Gen IV egg-family evidence. These are not complete acquisition histories. See [live reconciliation](../LIVE_DEVELOPMENT_RECONCILIATION.md) for exact hashes, boundaries and the independent Emerald comparison. The baseline description above is historical.

## Authoritative sources

- **NX-EVENT4**: [include/Legality/Gen4EventTemplate.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen4EventTemplate.h); `struct EventTemplate`; grade B.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.
- **PK-HISTORY**: [PKHeX.Core/Legality/LearnSource/Verify/LearnVerifierHistory.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Verify/LearnVerifierHistory.cs); `MarkAndIterate`; grade B.
- **PK-TRASH**: [PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/TrashByteVerifier.cs); `VerifyTrashBytesPalPark`; grade B.
- **PK-RIBBON**: [PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierUnique4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Verifiers/Ribbons/RibbonVerifierUnique4.cs); `AddMissingContest4`; grade B.

## Implementation brief

Preserve an event candidate's full initial moves and identity constraints before using it as a move-history source. The current template can establish structural compatibility only. Required new research schema: source archive revision/path/hash, template payload hash, original/reconstructed/unreleased status, card identity, initial moves, fixed/dynamic fields, trainer text bytes, ribbons, release-language evidence, dates with confidence, and transformation exceptions.

Do not union special moves from different events by species. For an event candidate, first enforce its fixed identity and generation method; then permit deletions and subsequent learnable moves. An ordinary egg candidate is a distinct history. Absence of an exact archive match cannot become global Invalid while the archive or identity transformations are incomplete.

## Archive audit and normalized record policy

Pinned primary archive notes: [Gallery README](https://github.com/projectpokemon/EventsGallery/blob/154d81be88453f6f78ec1d6d86e85fe0f2f5c240/README.md) and [altered legality cards README](https://github.com/projectpokemon/EventsGallery/blob/154d81be88453f6f78ec1d6d86e85fe0f2f5c240/PKHeX%20Legality/README.md). The Gallery explicitly confirms PGT-derived placeholder wondercards; the legality directory explicitly distinguishes altered cards from original preserved data.

The EventsGallery tree was fetched at the revision in `evidence/repositories.json`. Its top-level separation includes **Released**, **Unreleased**, **Extras**, and **PKHeX Legality**. Folder placement alone does not prove original distribution authenticity. This tranche inspects the index and README, not the individual distribution files; it makes no claim that every record in Released is original or complete. Do not download commercial distribution ROMs. No archive Pokémon, personal saves, or commercial binaries are imported by this research.

`evidence/event-projection.json` records input resource hash/count and projected vs omitted field families. This is a provenance-gap record, not a new event dataset. Retain reconstructed, placeholder or disputed records as separate confidence/status categories. A trusted original PCD plus source-backed conversion is stronger than a filename/date label.

## Inputs / lost evidence / positive and negative tests

PK4 can preserve trainer bytes, moves, ribbons, PID/IVs and origin fields, but a deposited Pokémon does not retain the entire PCD or proof of gift receipt. Moves may have been deleted; nickname may change or be rewritten on evolution. Do not require the original full four moves on an evolved/trained gift.

- Positive: exact matching template with a retained event move; later replace an ordinary move through a legal source.
- Negative candidate: keep a fixed template PID/OT identity but demand a special move supplied only by another incompatible template, with all later sources excluded.
- Unknown: reconstructed record, missing release proof, missing trash-byte capability, or ambiguous event identity.

No named event-exclusive incompatible moveset is proven in this tranche; enumerating the complete original event set and all alternative paths is required first. This avoids inventing a superficially plausible negative.

## Smallest correct integration / acceptance

1. Extend a research-only projection to moves and hashes with license review.
2. Add template candidate IDs to MOVE-001 witnesses.
3. Expose field capabilities, not default-false ribbon values.
4. Validate original/evolved/deleted-move controls through native encrypted PK4.
5. Maintain positive-only reporting until release/provenance coverage closes.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
