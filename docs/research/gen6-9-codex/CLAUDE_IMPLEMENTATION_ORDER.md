# Ready-to-execute implementation stages

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Preconditions common to every stage

Owner authorizes transition only after exact-current Gen I–V app/legality/native/asset/device acceptance. Fetch all heads again; preserve newer work and source files. Implement on the assigned future integration lane, never this research branch or old stacked snapshots. Each stage requires GCC+Clang normal+ASan/UBSan tests, narrow documented vendor exceptions only, pinned data provenance, native devkitA64 build **including its source dirs**, and physical Switch load/save/reopen acceptance on copies. Complexity below is qualitative, not a deadline.

### Stage A — Gen VI (high; most backend already exists)

Reuse #309 `Gen6PK6`, `Gen6SaveLayout/Save`, `Gen6StagedSession`, `Gen6Stats/PersonalData`, `Gen6Bag/Catalog`, `Gen6Readouts`, `Gen6ExactFormatEditorProvider`, `Gen6SaveSources`; Common VerifiedStagedExport. Extend native session for Create, Trainer and full party placement only after independent byte masks/invariants. Add proposed Gen6 shared bridge and exact-game catalog. Native registration touches Makefile, identity, Save Instances and shared provider routing described in SHARED_EDITOR_INTEGRATION.

Data: XY versus ORAS species/forms/moves/abilities, trainer string layout, item caps. Tests: all seven existing binaries plus Create occupancy/origin/name/PP/stats, party reference/last-member cases, Trainer mask, exact diff/CRC, stale source rejection, XY/ORAS mismatch. Hardware: Dekopon and copied Checkpoint discovery, all four titles, controller/touch, export game re-open. Risk: current #309 merge conflicts and newer shared picker/quantity interfaces. Done when native source is linked, operations are registered in the same editor and every exposed operation meets copy-only export gates. Read-only Amie/dex/ribbons may remain explicitly scoped.

### Stage B — SM/USUM (high; signing gate)

Reuse #311 `Gen7Save`, PK7 codec alias, `Gen7StagedSession`, stats/Bag/readouts/provider/source files. Read GEN7_SIGNATURE_INVESTIGATION before any signer. Resolve vendor helper failure and payload/game-acceptance evidence on a separate reviewed vendor lane; do not equate self-verification with acceptance. Add trainer/Create/party features only after the signature/export path is reviewed.

Data: family-native personal/learnset/item tables and SM/USUM differences. Tests: five existing binaries, independent PKHeX-vendor signature vectors, original diagnostics, field/CRC/signature whitelist, corruption/tail/digest distinctions, vendor MSan/LSan. Hardware: each family matching patch accepts edited copy and game resave. Native build adds C vendor objects without broad sanitizer suppression. Conflict: shared field gating must remain read-only while ExportSupported=false. Done only after cryptographic and game acceptance plus full export verifier; otherwise stage is blocked and read-only support can remain useful.

### Stage C — LGPE (high; distinct storage model)

Reuse `Pokemon7LGPE`, `Encryption7LGPE`, Trainer/Inventory readers and `LGPEReadValidation`; extend in proposed `Integration/Gen7b` with full 21-block validation. Data: PersonalTable.GG, PB7 AV/friendship/CP formula, PlayerBag7b, PokeListHeader and partner forms. No SM signing reuse.

Tests: 1000×260 storage bounds, six party pointers/starter pointer/sentinel, compression/index updates, partner clone/move refusal, AV/CP stats oracle, every block CRC, source/unknown-byte preservation. Hardware: both games' partner + ordinary storage operations, game reload, touch/controller. Native build includes new adapter sources. Conflict: shared grid presentation must support 25-slot UI pages and pointer-backed party. Done when allowed edits preserve pointers/starter, exact-game catalog, all CRCs and shared operation UX; leave unproven partner operations disabled.

### Stage D — SWSH (high; codec/parser done)

Reuse #312 Gen8SwishCrypto/PK8/Save/Layout, existing Trainer8SWSH read access, common export. Add proposed Gen8StagedSession/provider/bridge; never call legacy saveTrainerInfoSwSh. Data: SWSH personal presence/learnsets/TR/Dynamax/Gigantamax, patch/DLC schemas, item pockets. Tests: existing 1159-check suite plus touched-block record masks, complete key order/type/length preservation, battle-team lock refusal, scalar geometry, party cached stats, bag, unknown block carry-through and companion hashes.

Hardware: Sword and Shield, supported DLC variants, backup/poke_trade carried unchanged, new main copy load/save/reopen. Native build must compile Gen8 dir; base-app green jobs alone do not prove that. Conflict: legacy SC/PK8 duplication and existing Save Instances route. Done when independent decrypt/diff verifier and immutable companion bundle pass with all exposed shared editor actions.

### Stage E — BDSP (high; evidence-bound multi-file gate)

Reuse Pokemon8BDSP/Encryption8BDSP/Trainer8BDSP/BDSPReadValidation and structural probe. Implement proposed `Integration/Gen8b` flat parser/session and manifest bundle, keeping existing saveTrainerInfoBDSP refusal until paired-file protocol is established. Data: four Gem8Version sizes, full-file MD5, BDSP personal/learnsets/items and pair-selection evidence.

Tests: every revision pair/unknown revision, hash at fixed middle offset, full 40×30 party-sized boxes, malformed party/count, cross-title refusal, two-file snapshot mismatch and interrupted bundle publication. Hardware: both titles, paired manual/autosave/rollback behavior and edited-copy re-open. Native build adds adapter only after protocol review. Conflict: common exporter needs additive multi-file hooks; never weaken existing single-file safety. Done when pair semantics and transactional publication are both proven; integrity-only diagnostic is not enough.

### Stage F — PLA (high; mechanics-specific)

Reuse generic SC crypto plus existing Pokemon8LA/Encryption8LA/Trainer8LA/PLAReadValidation; add proposed `Integration/Gen8a` strict parser/session/provider. Data: PA8 personal/Ganbaru/mastery/move shop, native pastures and research sources. Tests: stored0x168 versus party0x178, key mismatch, effort-level/alpha-size/stat oracle, purchased/mastered flags and unknown research data preservation. Hardware: base/update layouts, pasture/party native stats and mastery save/reopen. Native build links adapter and relevant catalogs. Conflict: shared stat schema needs effort levels without relabeling EVs; generic current stat grid assumptions require provider extension. Done when native mechanics and all unknown blocks remain intact and exposed operations are verified.

### Stage G — SV (very high; broad revision/legality data)

Reuse legacy Pokemon9SV/Encryption9SV/Trainer9SV and tested SC crypto, with proposed strict `Integration/Gen9`. Data: SV revision predicate/DLC blocks, personal/learnsets/items, tera/raid/encounter tables. Tests: all supported size classes, tera sentinel, original/minted nature separation, genetic IV/Hyper Training separation, raid/marks/memory preservation, wrong-family same-size records, unchanged companions and touched-block verifier. Hardware: Scarlet/Violet supported DLC, game reload/resave. Native build includes exact data revision. Conflict: avoid existing generic Modern editor and domain-ceiling-only legality. Done when supported revisions and native fields are explicit and source immutability/exact diff gates pass; event/raid editing stays read-only unless separately proven.

### Stage H — ZA (very high; separate PA9/revision metadata)

Reuse Pokemon9LZA/Encryption9LZA/Trainer9LZA as audited readers and SC crypto; add proposed `Integration/Gen9a`, not an SV flag. Data: PA9/PersonalTable.ZA, Plus flags, alpha/form/scale rules, revision/presence/footer structures, Z-A transfer barrier. Tests: 0x198 box stride, 0x1E0 party stride, DLC presence byte and footer updates, UInt64 revision, missing blocks, unknown revisions, Plus flags at PK9-teratype region, no SV tera editor. Hardware: supported ZA revisions, native box/party metadata and game save/reopen. Native build and source schema must be pinned. Conflict: inherited PKM accessors can expose non-native mechanics; provider must hide them. Done when independent strict parser/diff verifier and native acceptance cover every advertised revision; Switch 2 equivalence remains separate research.

### Stage I — legality completion (very high; incremental across A–H)

Extend existing #314 analyzer interfaces and selectively reuse #301 detached readers/differentials after fixing failing wiring. Work per family and rule coverage, not a new giant analyzer. Required data: encounter/breed/evolution/move chronology/events/Bank-HOME historical graph, pinned license-reviewed tables. Tests: native structures, historical positive/negative/unknown cases, cross-game format confusion, missing-event-history never globally Invalid, joint move witness tests and audited PKHeX divergence logs.

Hardware tests validate UI/coverage display and immutable analysis, not comprehensive historical legality. Native build must retain verdict/coverage serialization and performance. Conflict: unresolved source label versus native storage and duplicate readers. Done per coverage contract only; full parity is not claimed until every advertised rule/era has evidence. Stages can add applicable legality incrementally; enabling exports must not depend on pretending incomplete history is valid.
