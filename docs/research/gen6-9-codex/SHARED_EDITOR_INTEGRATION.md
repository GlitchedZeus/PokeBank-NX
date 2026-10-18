# Minimal shared-editor integration map

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Current architecture: ORIGINAL #307 at d104310da30d8095931ca16e097a2f6e2652ddfe

The authoritative shared surface is [src/UI/SharedPokemonEditorSurface.inc](https://github.com/GlitchedZeus/PokeBank-NX/blob/d104310da30d8095931ca16e097a2f6e2652ddfe/src/UI/SharedPokemonEditorSurface.inc) with `SharedPokemonEditorContract`, `SharedPokemonEditorWorkspace`, `SharedEditorPresentation`, `SharedEditorChrome`, `SharedMovesPanel`, `SharedMoveFacts`, `SharedMenuGeometry`, `SharedFieldState`, and `SharedDataPaneLayout`. Gen III still has separate rendering that consumes the common components; do not describe consolidation as complete. Current Gen IV/V routing demonstrates the native session bridge pattern (`Gen5SharedEditorBridge.h`).

Read `docs/handoffs/THREAD_S_HANDOFF.md` from that exact head before coding. The generic Gen VI+ editor still has outdated labels/routing and is excluded by some Gen I–V QA contracts. Simply enabling the existing Modern presenter would bypass the user's shared-editor requirement.

## File-by-file future changes (proposed; not applied)

| Current file/group | Minimal change / responsibility | Gate |
|---|---|---|
| `include/Games/GameIdentity.h`, `src/Games/GameIdentity.cpp` | Add Nintendo3DS and exact XY/ORAS/SM/USUM identities; preserve existing Switch IDs | content-derived identity mismatch refuses assignment |
| `include/UI/ExactSaveCapabilities.h` | Map exact native format/family/stat model and operations; LGPE AV, PLA effort levels, ZA distinct PA9 | unsupported fields hidden/locked; no blanket Gen8/9 policy |
| `include/UI/SharedPokemonEditorContract.h` | Add provider-driven per-field states for Gen6+; no new editor | old Gen I–V field-state results unchanged |
| `include/UI/ExactFormatEditorProvider.h` | Reuse FieldState, MoveCompatibilityQuery/Provider and SourceCapabilityBridge | native capability + staged workspace + field Editable required |
| `include/Integration/Gen6/Gen6ExactFormatEditorProvider.h` | Register existing descriptor; separate exact-game availability from numeric limits | XY cannot select ORAS-only form/move |
| `include/Integration/Gen7/Gen7ExactFormatEditorProvider.h` | Register read-only capability until signing accepted | export disabled even with staged candidate |
| `include/Integration/Gen5/Gen5SharedEditorBridge.h` pattern | Add generation-native bridge for stage/refresh/review/discard/export | roll back stage if presentation refresh fails |
| `include/Source/SaveInstance.h`, `SaveProviderIdentity.h` | Extend provider descriptor/assignment without losing canonical path and fingerprint | independent copied backups remain selectable instances |
| `src/UI/SaveSelectScreen.cpp`, existing GameCard/Save Instance route | Dispatch exact validated adapter and preserve selected source/header | wrong-title source visible refusal; no filename identity |
| `src/UI/TrainerViewScreenBase.inc`, `TrainerViewScreenCompositeOverlay.cpp`, `SharedPokemonEditorSurface.inc` | Route Gen6+ via shared surface and provider, stop reaching generic mutation dialogs | both controller and touch reach same native operation |
| `include/UI/SharedMenuGeometry.h`, `SharedFieldState.h` and `SharedEditorChrome.h` | Consume current geometry/state tokens; ordinarily no geometry change needed | same menu TYPE keeps same sizing/footer/touch layer |
| Shared Species/Move/Bag pickers + `SharedMoveFacts.h` | Supply exact-game/revision catalogs and native metadata | title-specific search, PP/PP Ups, common scrolling/indexing |
| `include/Integration/Common/VerifiedStagedExport.h`, `.cpp` | Add optional immutable companion bundle manifest under review; preserve refusal of existing target | no source rewrite; atomic new bundle publication |
| `Makefile` | Add reviewed native source dirs and C vendor objects only when integrating | devkitA64 link proves new code is in NRO |
| Legality boundary (`Legality.cpp`, generation record analysis) | Supply immutable record+source+coverage evidence | no global Invalid from absent history |

Paths not introduced yet are explicitly **proposed** (`Gen6SharedEditorBridge` and Gen7b/8/9 staged adapters). Current `Gen6StagedSession`/`Gen7StagedSession` methods already supply most backend operations; see headers at #312, not stale PR summaries.

## Adapter contract and operation semantics: INFERRED

Adapter input: immutable source snapshot, exactGameId, saveRevision, source fingerprint, validated format and data revision. Output: read-only Trainer/Party/Box/Bag views plus a private staged session. Provide `fieldState`, native limits, derived values, species/form availability and move compatibility. Return errors/status with Unknown distinct from Unsupported; no fake editable row.

Create builds a native blank record and validates required trainer/origin/met fields, names, species/form, encryption constant, move PP and stats before placing it into a legal empty slot. Clone copies exact native bytes into an empty destination while respecting teams/partner/fused references. Move is an atomic clone+remove with rollback, including any pointer/index updates. Remove never overwrites quarantined malformed slots; party compaction and minimum-party constraints need native proofs. No cross-game True Move is enabled by this plan.

View never stages. Edit stages a candidate and refreshes UI atomically. Review includes Pokémon, Bag and Trainer changes. Discard restores the session and provider views from immutable source. Export builds a new candidate; an independent verifier checks same game/revision, changed-field allowlists, native stats, checksums/signatures/container hashes, untouched blocks/companions and source fingerprint before `VerifiedStagedExport` publishes a new copy.

## Product requirements preserved

Use existing Home/product shell, assets, Nunito fonts and regional backgrounds. Use shared action order and `Origin & Legality`; hidden native fields never get substitute controls. Destructive confirmations commit on explicit Y per current shared convention, with B cancel and touch equivalent. Use `beginModal` and footer chrome so covered controls cannot receive touch. ReadOnly fields retain current gold/locked appearance and focus rules. Same field menu TYPE gets shared layout; different mechanics use family-native rows in that same editor.

Acceptance: existing Gen I–V contract, host and native gates unchanged; additive Gen VI–IX provider tests; per-title catalog filters; Create/View/Edit/Clone/Move/Remove/Review/Discard/Export exercised with controller and touch; unchanged source hash and explicit export identity. Do not change #307/#314 in this research branch.
