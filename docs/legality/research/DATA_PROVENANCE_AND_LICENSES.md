# Data provenance and reuse boundaries

This report records observed license/provenance facts and integration gates; it does not decide legal compatibility.

| Material | Observed provenance | Disposition |
|---|---|---|
| PokeBank NX | Root LICENSE is GNU Affero GPL version 3; audit baseline fecfbca9 | Research files follow existing project policy; no license change |
| PKHeX | Root LICENSE is GNU GPL version 3; pinned 6501f0ab46e8f8ca048539dbaf8cae8cb104e722 | Source-reference only here; copied/translated code and redistributed datasets require license review |
| Nine pret projects | Exact revisions in repositories.json; retail disassembly/decompilation evidence | LICENSE REVIEW REQUIRED for code/data reuse; public GitHub availability is not permission to redistribute game assets |
| EventsGallery | Pinned tree/README inspected; includes Released, Unreleased, Extras, PKHeX Legality | Reference only; no Pokémon/event binaries or distribution ROMs imported |
| Existing Gen IV compatibility pool | Header attributes PokeAPI commit 168b1e89467054cda2e7df43ccebbb69b459497a | Existing project input, not freshly imported; upstream license/complete provenance not independently cleared here |
| Audit prose and research scripts | Independently authored; scripts read external sources and emit counts/hashes/limited facts | Research-only; no production integration or third-party module copies |
| Reproduction vectors | Synthetic stage inputs generated/replayed; not private saves or preserved event Pokémon | Use as bounded algorithm fixtures with their scope labels |

The event archive README explicitly says some missing Gen IV wondercards were reconstructed from PGTs with placeholder data. Its PKHeX Legality README says those cards have been altered and do not match original preserved data. Keep archive status and field-specific confidence in every proposed event record. Do not derive hard Invalid from placeholder dates/card headers or from an unreleased record's existence.

All nine requested pret repositories were verified and fetched. Depth of inspection differs: Ruby/FireRed/Emerald inheritance functions were executed in a host harness; Crystal/HGSS/Platinum breeding and associated move/metadata sources were inspected; Red/Yellow learnset structures and Gold breeding symbols were checked, not exhaustively audited. EventsGallery was an index/README audit, not per-distribution authentication.

Reproduction scripts extract retail functions only into temporary directories, compile them locally and delete the temporary files. Those bodies are not committed. The scripts do not build or acquire commercial game ROMs. No source saves, staged Pokémon, personal event records or synced project references were modified. Resource manifests store SHA-256 and counts; selected factual learnset values are reference-only pending review. The full PKHeX binary resource sets and complete retail learnsets are deliberately not vendored in this PR.

Source registry entries include repository, exact revision, path, symbol, matching line locations, whole-file line range, SHA-256, retrieval date, grade and uncertainty. Whole-file line ranges are navigation aids, not claims that every line was audited. Referenced source revisions may be newer than an old local cache; the actual pinned revision governs the comparison.
