# PokeBank NX documentation

Start with **[STATUS.md](STATUS.md)** for where the project stands today. This page indexes everything else
by audience. Paths under `docs/hardware/` and `docs/qa/` are fixed: active repair branches and tests write
there, so they are never reorganised.

| I want to… | Read |
|---|---|
| Know what works and what is still missing | [STATUS.md](STATUS.md) |
| Install and try a build | [Installation in the README](../README.md#install-and-use) |
| See every candidate NRO and its hardware result | [RELEASES.md](RELEASES.md), [CHANGELOG](../CHANGELOG.md) |
| Understand how my saves are protected | [user/SAVE_SAFETY.md](user/SAVE_SAFETY.md), [qa/SAVE_SAFETY_CHAIN.md](qa/SAVE_SAFETY_CHAIN.md) |
| Report a bug or contribute | [CONTRIBUTING](../CONTRIBUTING.md), [SECURITY](../SECURITY.md) |
| Find my way around the code | [ARCHITECTURE.md](ARCHITECTURE.md) |

## User documentation

- [Game support matrix](user/GAME_SUPPORT_MATRIX.md): which games and save providers are recognised.
- [Controller map](user/CONTROLS.md) (target input contract) and the [contextual control footer](development/CONTEXTUAL_CONTROL_FOOTER.md).
- [Save-safety contract](user/SAVE_SAFETY.md): why original saves are never written, and what a future write pipeline must prove.
- [RetroArch source naming](RETROARCH_SOURCE_NAMING.md): how RetroArch saves are grouped and identified.
- [Standalone runtime](STANDALONE_RUNTIME.md): the `sdmc:/switch/PokeBank-NX/` folders the app owns.

## Development

- [Architecture overview](ARCHITECTURE.md) (the code as it is) and the original [target architecture](development/TARGET_ARCHITECTURE.md) (the design it grows toward).
- [Engineering protocol](ENGINEERING_PROTOCOL.md) and [engineering authority](ENGINEERING_AUTHORITY.md): how work is reviewed, which document wins.
- Editor and UI contracts: [Pokémon editor UI](development/POKEMON_EDITOR_UI_CONTRACT.md), [identity, custody, move and clone](development/POKEMON_IDENTITY_CUSTODY_CONTRACT.md), [UI flow](UI_FLOW.md), [UI style guide](UI_STYLE_GUIDE.md), [UI ownership](UI_OWNERSHIP_STATUS.md), [product and UI philosophy](PRODUCT_UI_PHILOSOPHY.md), [product decisions](development/PRODUCT_DECISIONS.md), [classic save editor architecture](development/CLASSIC_SAVE_EDITOR_ARCHITECTURE.md), [universal editor reuse freeze (#71)](development/ISSUE71_UNIVERSAL_EDITOR_REUSE_FREEZE.md).
- Generation notes: [Gen II native domains](GEN2_AUDIT_RESEARCH.md), [Gen III via PKSM-Core](PKSM_CORE_INTEGRATION.md), [Gen V development](development/GEN5_DEVELOPMENT.md), [Gen V native Create](GEN5_NATIVE_CREATE.md), [re-roll mechanics](REROLL_MECHANICS.md).
- Build and data: [generated data](development/DATA_REGENERATION.md), [device asset gate](development/DEVICE_BUILD_ASSET_GATE.md), [device artifact packaging](development/DEVICE_ARTIFACT_PACKAGING.md), [recovery contract](RECOVERY_CONTRACT.md), [build / NRO record](BUILD_RECORD.md), [release checklist](development/RELEASE_CHECKLIST.md), [PKHeX oracle](development/PKHEX_ORACLE.md), [upstream reuse audit](UPSTREAM_AUDIT.md).
- Project management: [project map](PROJECT_MAP.md), [project board](development/PROJECT_BOARD.md) ([definition](project-board/)), [main-dev instructions](main-dev/).

## Hardware QA and acceptance

Nothing is "accepted" until the owner has tested that exact NRO on a Switch. A green CI run never counts.

- [`hardware/`](hardware/): per-round retest checklists (`RETEST_<sha8>.md`), owner results (`ROUND<n>_RESULTS_<sha8>.md`) and the [candidate NRO archive](hardware/NRO_ARCHIVE.md).
- [`qa/`](qa/): independent QA ([overview](qa/README.md)), the [save-safety chain](qa/SAVE_SAFETY_CHAIN.md), the [Gen I–V matrix](qa/GEN1_5_MATRIX.md) and [findings](qa/FINDINGS.md).
- Current Round 13 work lives on `fix/round12-gen15-repairs` until it merges: [Round 13 checklist](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/RETEST_9c301c48.md), [master hardware acceptance ledger](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/qa/MASTER_HARDWARE_ACCEPTANCE_LEDGER.md), [v1.0 feature gaps](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/qa/V1_RELEASE_FEATURE_GAPS.md), [historical Round 1–12 audit](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/qa/HISTORICAL_ROUND_1_12_AUDIT.md).
- [Full Gen I–V acceptance gate](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) (all 20 releases) and the [physical test checklist](DEVICE_TEST_CHECKLIST.md) used by the bug-report template.

## Legality research

The legality engine is developed separately and is **not** part of the app (paused at PR #314, with lanes #327/#328).

- [`legality/`](legality/): Gen I first-sprint handoff.
- [Future legal Pokémon generator](roadmap/FUTURE_LEGAL_POKEMON_GENERATOR.md) (planning only).

## Vault and roadmap

- [v1.0 roadmap](V1_ROADMAP.md) and [v1.0 polish audit](V1_POLISH_AUDIT_2026-09-28.md).
- [Master Vault specification](roadmap/MASTER_VAULT_SPEC.md), [transfer model](roadmap/TRANSFER_MODEL.md), [Pokédex specification](roadmap/POKEDEX_SPEC.md).
- Later: [v2 roadmap](roadmap/V2_ROADMAP.md), [v2 platform vision](roadmap/V2_PLATFORM_VISION.md), [future product vision](roadmap/FUTURE_PRODUCT_VISION.md), [Create Pokémon vision](roadmap/CREATE_POKEMON_VISION.md), [Trainer Plaza](roadmap/TRAINER_PLAZA_VISION.md), [feature feasibility](roadmap/FEATURE_FEASIBILITY_MATRIX.md), [NRO quality](roadmap/NRO_QUALITY_ROADMAP.md), [modded-Switch backlog](roadmap/MODDED_SWITCH_FEATURE_BACKLOG.md), [ideas after Gen I–V](roadmap/FUTURE_AFTER_GEN15_BACKLOG.md), [Gen VI–IX PKSE backlog](future/GEN6_9_PKSE_BACKLOG.md).

## Research

[`research/`](research/) holds reference audits (save engines, file systems, GameCube, OpenHome, PKHeX-adjacent projects) and indexes: [research router](RESEARCH_CURRENT_INDEX.md), [reference index](research/REFERENCE_INDEX.md), [research reference index](research/RESEARCH_REFERENCE_INDEX.md), [reference matrix](research/RESEARCH_REFERENCE_MATRIX.md), [external references](research/REFERENCES.md). Generated-data source manifests are in [`data/`](data/).

## Maintenance

- [Branch cleanup record, 2026-11-02](maintenance/BRANCH_CLEANUP_STATE_2026-11-02.md) (331 → 88 branches; authoritative). The earlier session's audit and plan are archived and disabled in [maintenance/archive/superseded-2026-11-02-phase2-plan/](maintenance/archive/superseded-2026-11-02-phase2-plan/).
- [Main consolidation ledger, 2026-10-31](MAIN_CONSOLIDATION_20261031.md) and [three-branch consolidation plan](maintenance/THREE_BRANCH_CONSOLIDATION_PLAN.md).

## Agent session files

Kept at their paths because tests and agent instructions reference them: [CODEX_SESSION.md](CODEX_SESSION.md), [NEXT_CODEX_PROMPT.md](NEXT_CODEX_PROMPT.md), [NEXT_SESSION_PLAN.md](NEXT_SESSION_PLAN.md), [SESSION_RUNBOOK.md](SESSION_RUNBOOK.md), [PROJECT_RESOURCE_INDEX.md](PROJECT_RESOURCE_INDEX.md), [handoffs/](handoffs/). Their contents are historical working notes; [STATUS.md](STATUS.md) supersedes any status they state.

## Archive

[`archive/`](archive/) keeps historical material unchanged: session prompts ([`archive/prompts/`](archive/prompts/)), session logs, handoffs and snapshots ([`archive/sessions/`](archive/sessions/)), dated device-test reports from September–October 2026 ([`archive/device-tests/`](archive/device-tests/)) and dated audits ([`archive/audits/`](archive/audits/)). Older snapshots are in [`history/`](history/), [`audit/`](audit/) and [`audits/`](audits/). [AUDIT_STATUS_2026-09-23.md](AUDIT_STATUS_2026-09-23.md) stays at the top level because a workflow references it.

## Moved documents

These files moved on 2026-11-02 (history preserved with `git mv`; links inside the repository were updated):

| Old path | New path |
|---|---|
| `docs/ARCHITECTURE.md` (target design) | [`docs/development/TARGET_ARCHITECTURE.md`](development/TARGET_ARCHITECTURE.md); `docs/ARCHITECTURE.md` is now an overview of the current code |
| `docs/SAVE_SAFETY.md` | [`docs/user/SAVE_SAFETY.md`](user/SAVE_SAFETY.md) |
| `docs/GAME_SUPPORT_MATRIX.md` | [`docs/user/GAME_SUPPORT_MATRIX.md`](user/GAME_SUPPORT_MATRIX.md) |
| `docs/CONTROLS.md` | [`docs/user/CONTROLS.md`](user/CONTROLS.md) |
| `docs/MASTER_VAULT_SPEC.md` | [`docs/roadmap/MASTER_VAULT_SPEC.md`](roadmap/MASTER_VAULT_SPEC.md) |
| `docs/TRANSFER_MODEL.md` | [`docs/roadmap/TRANSFER_MODEL.md`](roadmap/TRANSFER_MODEL.md) |
| `docs/FUTURE_AFTER_GEN15_BACKLOG.md` | [`docs/roadmap/FUTURE_AFTER_GEN15_BACKLOG.md`](roadmap/FUTURE_AFTER_GEN15_BACKLOG.md) |
| `docs/THREE_BRANCH_CONSOLIDATION_PLAN.md` | [`docs/maintenance/THREE_BRANCH_CONSOLIDATION_PLAN.md`](maintenance/THREE_BRANCH_CONSOLIDATION_PLAN.md) |
| `docs/RELEASE_CHECKLIST.md`, `docs/DATA_REGENERATION.md`, editor/UI contracts | [`docs/development/`](development/) |
| `docs/PROMPT_*.md` | [`docs/archive/prompts/`](archive/prompts/) |
| `docs/SESSION_LOG_*.md`, `docs/FINAL_HANDOFF_2026-09-03.md`, `docs/ENGINEERING_SNAPSHOT_20261014.md`, `docs/GITHUB_CLEANUP_20261015.md` | [`docs/archive/sessions/`](archive/sessions/) |
| `docs/DEVICE_TEST_*_2026-09-*.md`, `docs/GEN1_*` hardware follow-ups | [`docs/archive/device-tests/`](archive/device-tests/) |
| `docs/*_REFERENCE_AUDIT_*.md`, `docs/RESEARCH_*.md` (except `RESEARCH_CURRENT_INDEX.md`), `docs/REFERENCES.md` | [`docs/research/`](research/) |

The full old → new list is in the commit that made the move (`git log --follow` on any moved file shows it).
