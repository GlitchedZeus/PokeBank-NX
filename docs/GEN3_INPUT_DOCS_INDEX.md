# Issue #71 — Pre-Gen-III input/document archaeology index

Audit base: `feature/pokebank-playable@48753c2b0093d30213aa32f016100df0906c3cfb`
Audit branch: `audit/pre-gen3-shared-editor-71`
Accepted Gen I/II tree: `11c6d899f0d842ac97a3813a48c8d33beba675bb`

This file records the corpus-discovery pass required by issue #71. The complete `docs/` tree at the accepted baseline was enumerated recursively, including `docs/history/` and `docs/research/`. The canonical research routers were read first, then current implementation and potentially relevant historical material were cross-checked.

## Triage key

- **A — DIRECTLY RELEVANT**: editor/reuse/capabilities/format/source/safety/storage/conversion research used by #71.
- **B — HISTORICAL/POTENTIALLY RELEVANT**: status, prompt, device, product or roadmap evidence checked for hidden references/decisions; not authority over current code.
- **C — OUTSIDE THIS AUDIT'S DECISION SURFACE**: build/device/asset/companion material inventoried but not used to decide the pre-Gen-III shared-editor contract.

## Top-level docs — exhaustive triage

### A — directly relevant

`ARCHITECTURE.md`, `BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`, `CLASSIC_SAVE_EDITOR_ARCHITECTURE.md`, `CREATE_POKEMON_VISION.md`, `FEATURE_FEASIBILITY_MATRIX.md`, `FUTURE_PRODUCT_VISION.md`, `GAME_SUPPORT_MATRIX.md`, `GEN1_EDITOR_DEVICE_ACCEPTANCE_2026-09-14.md`, `GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md`, `GEN1_MOVE_COMPATIBILITY_REFERENCE.md`, `GEN1_SHINY_SPRITE_REFERENCE.md`, `GEN1_SUMMARY_RADAR_RETEST.md`, `GEN1_UX_CLEANUP3_HARDWARE_FOLLOWUP_2026-09-13.md`, `GEN1_UX_CLEANUP3_MODEL_CHECKPOINT.md`, `GEN2_AUDIT_RESEARCH.md`, `HOME_BRIDGE_HISTORICAL_TRANSFER_RESEARCH_2026-09-03.md`, `MASTER_VAULT_SPEC.md`, `MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`, `OPENHOME_SWITCH_PLATFORM_REFERENCE_AUDIT_2026-09-03.md`, `PKHEX_ORACLE.md`, `PKHOUSE_REFERENCE.md`, `PKSM_CORE_INTEGRATION.md`, `POKEDEX_SPEC.md`, `POKEMON_EDITOR_UI_CONTRACT.md`, `PRODUCT_DECISIONS.md`, `PRODUCT_UI_PHILOSOPHY.md`, `PROJECT_MAP.md`, `PROJECT_RESOURCE_INDEX.md`, `RESEARCH_CURRENT_INDEX.md`, `RESEARCH_INTAKE_2026-09-09.md`, `RESEARCH_REFERENCE_INDEX.md`, `RESEARCH_REFERENCE_MATRIX.md`, `RESEARCH_STATUS_2026-09-13.md`, `RETROARCH_SOURCE_NAMING.md`, `SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md`, `SAVE_FORMAT_EDGE_CASE_RESEARCH_2026-09-13.md`, `SAVE_SAFETY.md`, `SAVE_STORAGE_DISCOVERY_RESEARCH_2026-09-13.md`, `SESSION2_6_SAFETY_IMPLEMENTATION.md`, `SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md`, `TRANSFER_MODEL.md`, `UI_FLOW.md`, `UI_OWNERSHIP_STATUS.md`, `UI_STYLE_GUIDE.md`, `UPSTREAM_AUDIT.md`, `V1_ROADMAP.md`.

### B — historical / potentially relevant evidence

`BUILD_RECORD.md`, `CODEX_SESSION.md`, `CONTEXTUAL_CONTROL_FOOTER.md`, `CONTROLS.md`, `DEVICE_TEST_CHECKLIST.md`, `DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md`, `DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`, `DEVICE_TEST_FOLLOWUP_2026-09-03.md`, `DEVICE_TEST_REPORT_2026-09-01.md`, `DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md`, `FINAL_HANDOFF_2026-09-03.md`, `GAMECUBE_REFERENCE_AUDIT_2026-09-09.md`, `MODDED_SWITCH_FEATURE_BACKLOG.md`, `NEXT_CODEX_PROMPT.md`, `NEXT_SESSION_PLAN.md`, `NRO_QUALITY_ROADMAP.md`, `PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md`, `PROMPT_MONDAY_RECOVERY_SESSION2_6.md`, `PROMPT_RECOVERY_PUBLISH_NRO_2026-09-09.md`, `PROMPT_SESSION2_5_FINISH.md`, `PROMPT_SESSION2_5_VISUAL_SHELL.md`, `PROMPT_SESSION2_6_BUILD_ASSET_ADDENDUM.md`, `PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`, `PROMPT_SESSION2_RECOVERY.md`, `PROMPT_SESSION3_PKSM_CORE.md`, `RECOVERY_CONTRACT.md`, `RELEASE_CHECKLIST.md`, `SESSION_LOG_2026-09-01.md`, `SESSION_LOG_2026-09-02.md`, `SESSION_RUNBOOK.md`, `STANDALONE_RUNTIME.md`, `TRAINER_PLAZA_VISION.md`, `UPR_ZX_REFERENCE_AUDIT_2026-09-03.md`, `V2_PLATFORM_VISION.md`, `V2_ROADMAP.md`.

### C — inventoried, not used to decide #71 editor/reuse freeze

`BUILD_BLOCKER_92bde34d_2026-09-09.md`, `COMPANION_MOVE_LAB_MAP_GUIDE_RESEARCH_2026-09-03.md`, `DEVICE_ARTIFACT_PACKAGING.md`, `DEVICE_BUILD_ASSET_GATE.md`, `POKEMON_CRY_AUDIO_AUDIT_2026-09-02.md`, `POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md`.

These remain useful in their own milestones; classification C means only that they do not determine the Gen III shared-editor delta.

## `docs/history/` — exhaustive triage

All four files were explicitly inventoried and treated as B/history evidence:

- `history/CURRENT_STATUS_PRE_LEAFGREEN_2026-09-09.md`
- `history/NEXT_CODEX_PROMPT_PRE_LEAFGREEN_2026-09-09.md`
- `history/PROJECT_RESOURCE_INDEX_PRE_CHUNKS_2026-09-09.md`
- `history/README.md`

Useful references that survived the historical pass are already represented by the current research routers/reuse matrix. No historical prompt/status statement overrides the accepted Gen I/II tree or current safety contract.

## `docs/research/` — exhaustive triage

### 2026-09-09

All were inventoried. The following are A for source/format/provenance/safety/reuse decisions:

- `research/2026-09-09/SAVE_FORMATS_PROFILES_CONTAINERS.md`
- `research/2026-09-09/SOURCE_MANIFEST.md`
- `research/2026-09-09/TRANSFER_FIXTURES_PROVENANCE.md`
- `research/2026-09-09/VAULT_DATA_EVENTS_PERFORMANCE.md`
- `research/2026-09-09/POWERLOSS_DURABILITY_TEST_PLAN.md`

The following are B because they are future subsystem/reference evidence rather than prerequisites to the shared Gen III editor provider contract:

- `research/2026-09-09/BANK_HOME_PRESERVATION_CAPTURE_PLAN.md`
- `research/2026-09-09/BANK_HOME_PUBLIC_CORPUS_AUDIT.md`
- `research/2026-09-09/LEGACY_RECOVERY_GAMECUBE_STADIUM.md`
- `research/2026-09-09/OFFICIAL_PATH_TRANSFER_CORPUS_AUDIT.md`
- `research/2026-09-09/RIBBON_BANK_EVENT_PRESERVATION_REFERENCES.md`
- `research/2026-09-09/VAULT_PERFORMANCE_EVENT_RIGHTS_AUDIT.md`

### 2026-09-11

- `research/2026-09-11/GSC_FORMAT_ORACLE.md` — A, because it cross-checks the accepted Gen II semantics that must not leak into Gen III.

## External/reference ecosystem conclusions checked

The current routers and reuse audits were cross-checked before freeze:

- **PKSE** — already inherited native Switch foundation; reuse existing PokeBank-owned routes, never push custom code upstream.
- **PKSM-Core** — native C++ historical-format engine candidate; already wrapped by the Gen III read-only adapter. Do not leak PKSM types into UI.
- **PKSM / Pokémon Chest / PHBank** — integration, bank and UX references; do not import their complete UI/ownership models.
- **PKHeX** — host correctness/oracle/reference, not Switch runtime UI dependency.
- **Auto Legality/CoreConsole** — host/reference for future legality/creator work, not a runtime shortcut.
- **pkHouse/pkBakery** — modern Switch behavior references; reference/reimplement behind PokeBank interfaces.
- **Project Pokémon / pret / emulator-save projects** — format/test/reference evidence, not permission to bypass PokeBank provenance/safety.
- **Poke_Transporter_GB/PCCS** — future conversion-policy/reference evidence, not a Gen III editor implementation dependency.

## Contradiction/staleness findings

1. Root/current status documents still described PR #68 as open/device-pending. That is stale after exact device acceptance and merge; this audit updates current status files.
2. Older architecture documents may describe future write capabilities. Those are design targets only; `SourceMutationPolicy` and the accepted write-policy tests remain current runtime authority.
3. `PokemonViewActions.h` contains a Gen2-namespaced slot-action helper despite living under shared UI. Do not use that namespace as the Gen III action contract; the actual shared action model is `SharedPokemonEditorContract`.
4. `TrainerViewScreenGSCOverlay.inc` still contains legacy Gen II prompt/editor code, but the accepted parity layer statically makes legacy normal-editor paths unreachable. It is historical compatibility implementation, not the template for Gen III.
5. `EditableSaveCapabilities` and `SharedEditor::Capabilities` are intentionally different layers (save-operation versus field/UI capability). Do not collapse them into one bitset.

## Completion note

The recursive docs tree was exhaustively inventoried for this audit. Relevant conclusions were promoted into `GEN3_UI_REUSE_MATRIX.md`, `GEN3_REUSE_FREEZE.md`, `GEN3_ITEM_LANE_DECISION_TREE.md`, and `GEN3_MINIMAL_SAFE_CHANGE_LIST.md`. No new external research was required to settle the P0 editor/provider boundary; unresolved later-generation semantics remain explicitly deferred rather than guessed.
