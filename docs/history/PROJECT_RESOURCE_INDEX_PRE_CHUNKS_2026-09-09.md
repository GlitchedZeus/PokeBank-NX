# PokeBank NX — Codex Project / Resource Index

Last updated: 2026-09-09

> **Navigation only. This file does not define active scope.**
>
> The active engineering authority remains, in order:
>
> 1. `CURRENT_STATUS.md`
> 2. `docs/CODEX_SESSION.md`
> 3. `docs/NEXT_CODEX_PROMPT.md`
>
> If any roadmap, old session log, old build record, old project map, issue text, README text, or research file conflicts with the three files above, the three files above win for the current coding session.

This index exists so Codex can find the right information quickly instead of reading the entire repository or rediscovering work that has already been researched.

The root `README.md` is the user's human-facing project front page. **Do not rewrite, reorganize, replace, or use it to expand implementation scope unless the user explicitly asks for README work.**

---

# 1. Codex fast path

For a normal coding/recovery session, do **not** read every document.

Start with:

```text
CURRENT_STATUS.md
        ↓
docs/CODEX_SESSION.md
        ↓
docs/NEXT_CODEX_PROMPT.md
```

Then consult only the category below that is directly relevant to the active task.

Before any destructive Git/worktree/generated-asset/recovery action:

```text
inspect worktrees
inspect git status
preserve useful local/uncommitted/recovery/build/generated state
preserve parked recovery refs
then act
```

Writable branch/remote:

```text
origin/feature/pokebank-playable
```

Never push custom PokeBank NX code upstream to PKSE.

---

# 2. Current recovery / build / artifact work

Use these when the active task is building, packaging, hashing, preserving, publishing, or physically testing an `.nro`:

```text
CURRENT_STATUS.md

docs/NEXT_CODEX_PROMPT.md
    exact active mission and STOP point

docs/DEVICE_BUILD_ASSET_GATE.md
    required RomFS / sprite / font / type-icon / game-card checks

docs/DEVICE_ARTIFACT_PACKAGING.md
    exact-source packaging workflow and package_device_build.py behavior

docs/BUILD_RECORD.md
    historical and current artifact identities / hashes / verification records

docs/RELEASE_CHECKLIST.md
    build/device/prerelease/release evidence rules

docs/DEVICE_TEST_CHECKLIST.md
    physical Switch test coverage
```

Current FRLG recovery facts must always come from `CURRENT_STATUS.md` / `NEXT_CODEX_PROMPT.md`, not from older sections of `BUILD_RECORD.md`, `PROJECT_MAP.md`, or `NEXT_SESSION_PLAN.md`.

As of this index update, the canonical application source named by the authoritative files is:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

Canonical application tree:

```text
ed5912093886384894c44538d569fe4955fd2e47
```

Do not treat this index as a replacement for checking `CURRENT_STATUS.md` in a later session; those authoritative files may advance.

---

# 3. Architecture / boundaries

Read only when the task changes app structure, adapters, services, storage, legality, transfer, or write safety:

```text
docs/ARCHITECTURE.md
    layer boundaries and dependency direction

docs/SAVE_SAFETY.md
    staged-write / backup / readback / rollback contract

docs/TRANSFER_MODEL.md
    COPY / MOVE / CLONE / conversion semantics

docs/MASTER_VAULT_SPEC.md
    immutable Vault entities, hashes, provenance, transactions

docs/GAME_SUPPORT_MATRIX.md
    game/release/platform identity and planned support
```

Core rule:

```text
external engine/reference
        ↓
PokeBank adapter/service boundary
        ↓
PokeBank safety/provenance/transactions
        ↓
PokeBank controller-first UI
```

Do not let PKSM, PKHeX, PKSE, pkHouse, or another project become the UI/business architecture directly.

---

# 4. UI / controls / visual design

Use these for UI work instead of re-deriving design from old screenshots/chat history:

```text
docs/UI_STYLE_GUIDE.md
    visual contract, themes, cards, Summary, Dex, Vault, dialogs

docs/UI_FLOW.md
    controller-first navigation and action semantics

docs/CONTROLS.md
    controller behavior / normalization details

docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md
    external visual/artwork sources, rights notes, resolver recommendations

docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
    inherited offline sprite generation/runtime pipeline

docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md
    physical feedback for Pokémon render motion/presentation
```

Actual project visual material includes:

```text
assets/screenshots/
assets/game_cards/
assets/banner.png
assets/icon.jpg
```

`assets/game_cards/README.md` records the pinned source revision and hashes for the packaged FRLG GBA cards.

The generated runtime asset tree under `romfs/` is intentionally not equivalent to the Git-tracked `assets/` directory. Follow the asset-gate and sprite-pipeline docs before assuming a clean checkout contains every runtime image.

---

# 5. Pokémon visual / artwork source strategy

Before searching for or rebuilding Pokémon artwork infrastructure, consult:

```text
docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md
docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
```

Already researched sources include:

```text
PokeAPI/sprites
PokéSprite
Project Pokémon sprite/model indexes
Pokemon-3D-api organization
Pokémon Database sprite pages
libretro game-cover thumbnails
```

Important distinction:

```text
technical availability != redistribution permission
```

Repository code licensing and Pokémon artwork/model rights may be different. Do not assume an MIT/BSD repository license automatically licenses Nintendo/TPC/Game Freak imagery for redistribution.

---

# 6. Audio / cries

For Pokémon cry/audio work:

```text
docs/POKEMON_CRY_AUDIO_AUDIT_2026-09-02.md
```

Do not begin new audio-source research without checking that audit first.

---

# 7. Save engines / format research

Before writing a Pokémon/save parser or conversion engine from scratch, consult:

```text
docs/PKSM_CORE_INTEGRATION.md
docs/PKHEX_ORACLE.md
docs/PKHOUSE_REFERENCE.md
docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
docs/UPSTREAM_AUDIT.md
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
docs/OPENHOME_SWITCH_PLATFORM_REFERENCE_AUDIT_2026-09-03.md
docs/UPR_ZX_REFERENCE_AUDIT_2026-09-03.md
docs/RESEARCH_REFERENCE_MATRIX.md
```

Primary roles:

```text
PKSE          inherited native Switch foundation
PKSM-Core     native historical Pokémon/save engine candidate
PKSM          mature full-app/bank/event/save-management reference
PKHeX         primary host-side correctness / legality / encounter / conversion oracle
Auto Legality host-side Quick Legal / generation research
pkHouse       modern Switch save-behavior reference
```

Use triangulation where practical instead of trusting one implementation blindly.

---

# 8. External GitHub research inventory

The master external-project triage is:

```text
docs/RESEARCH_REFERENCE_MATRIX.md
```

Check it before searching GitHub or implementing a solved subproblem.

It classifies projects as:

```text
DIRECT-CANDIDATE
PINNED-HOST-TOOL
REFERENCE
HISTORICAL
IGNORE / OUT-OF-SCOPE
```

High-value references currently include, among others:

```text
kwsch/PKHeX
PKSM / PKSM-Core
Striaton-Lab-Team/Poke_Transporter_GB
Manu098vm/Switch-Gift-Data-Manager
kwsch/PokePiaSWSH
JamieJQuinn/IR-GTS
Reisyukaku/poke-client
foohyfooh/PKHeXPluginPile
kwsch/CoreConsole
kwsch/HomeRip
kwsch/pk2pk
kwsch/XYSAV
kwsch/UnityDPtools
carimatics/switch-poke-pilot
ItsMeJoji/PokeTrainerTools
PoshoDev/DexTool
DigitalFlow/Pokemon-Team-Builder
suloku/G6_pl_tool
```

Do not copy code merely because a repository is public. Check exact license and file/data rights first.

---

# 9. Create Pokémon / editor / legality

This feature is permanent and must not be accidentally dropped when other roadmap ideas are discussed.

Read:

```text
docs/CREATE_POKEMON_VISION.md
docs/PKHEX_ORACLE.md
docs/PRODUCT_DECISIONS.md
```

Product shorthand:

```text
PKHeX-class capability baseline
+ Quick Legal
+ Guided Create
+ Advanced / Expert Editor
+ controller-first workflow
+ human-readable legality explanations
+ destination compatibility
+ Event Library
+ provenance/history
+ Vault integration
+ staging / preview / rollback
= PokeBank NX Create Pokémon
```

PKHeX is a host-side oracle/reference under the current architecture; the Switch runtime remains PokeBank-native unless a deliberate later architecture/licensing decision changes that.

---

# 10. Vault / Banks / Dex / collection intelligence

Use:

```text
docs/MASTER_VAULT_SPEC.md
docs/POKEDEX_SPEC.md
docs/TRANSFER_MODEL.md
docs/MODDED_SWITCH_FEATURE_BACKLOG.md
docs/FUTURE_PRODUCT_VISION.md
```

Concept boundary:

```text
Master Vault = authoritative immutable entities + provenance
Banks        = logical organization/references
Pokédex      = rebuildable collection index derived from Vault
Game saves   = external sources/destinations behind adapters
```

Do not make Banks own independent Pokémon payloads and do not make the Pokédex a second manual ownership database.

---

# 11. Event / Mystery Gift preservation

Use:

```text
docs/FUTURE_PRODUCT_VISION.md
docs/PRODUCT_DECISIONS.md
docs/RESEARCH_REFERENCE_MATRIX.md
```

Important external references already indexed include:

```text
Project Pokémon EventsGallery
PKHeX event/Mystery Gift handling
Switch-Gift-Data-Manager
G6_pl_tool
IR-GTS Wonder Card behavior
```

Permanent provenance rule:

```text
legality != witnessed historical redemption
```

A locally restored/generated event must be labeled truthfully and must not fabricate HOME trackers, Nintendo-account history, server receipts, or official live-redemption provenance.

---

# 12. Social / Trainer Plaza / trading

Future-only unless explicitly promoted into the active prompt:

```text
docs/TRAINER_PLAZA_VISION.md
docs/V2_PLATFORM_VISION.md
docs/V2_ROADMAP.md
docs/PRODUCT_DECISIONS.md
```

Current design direction:

```text
Trainer Plaza
Friends
Mailbox
Poké Radar
TrainerDex
Trainer Card / Passport
Partner Pokémon
Trade Center
Gift
LAN / Internet PokeBank exchange
Android companion later
```

The local Vault remains primary. Online infrastructure should not become a mandatory hosted Vault.

---

# 13. Long-term mega-app backlog

These are accepted product directions but **never automatically become the next coding task**:

```text
docs/MODDED_SWITCH_FEATURE_BACKLOG.md
docs/FUTURE_PRODUCT_VISION.md
docs/FEATURE_FEASIBILITY_MATRIX.md
docs/PRODUCT_DECISIONS.md
docs/V1_ROADMAP.md
docs/V2_ROADMAP.md
```

The backlog includes Save Scanner, Rescue Mode, Save Time Machine, Pokémon Journey/provenance, Transfer Lab, compatibility map, offline legality, Living Dex intelligence, evolution planning, duplicate detection, EventDex, Ribbon Master, shiny-hunt tools, team building, bulk tools, cross-save search, Verified Breeding, Trainer Plaza, LAN/Internet exchange, companion apps, backup/disaster recovery, provider/plugin systems, and offline-first/no-lock-in guarantees.

Roadmap documents describe destination. `NEXT_CODEX_PROMPT.md` decides what Codex is allowed to implement now.

---

# 14. Testing / evidence / safety history

Use only the exact report relevant to the artifact/source under investigation:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md
docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/SESSION2_6_SAFETY_IMPLEMENTATION.md
```

Never upgrade evidence terminology merely because a build succeeds:

```text
BUILDS != DEVICE TESTED
DEVICE TESTED != DEVICE ACCEPTED
```

Physical acceptance requires the user's test of the exact filename/hash.

---

# 15. Historical session documents

These are retained deliberately and must not be deleted merely because newer state exists:

```text
docs/SESSION_LOG_*.md
docs/PROMPT_SESSION*.md
docs/PROMPT_MONDAY_RECOVERY_SESSION2_6.md
docs/FINAL_HANDOFF_2026-09-03.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
```

They can contain useful archaeology, previous artifact identities, failed approaches, or recovery details.

However, some contain older "current" states. Treat them as **historical evidence/navigation unless CURRENT_STATUS.md or NEXT_CODEX_PROMPT.md explicitly points to them**.

Do not delete or rewrite historical evidence simply to make the docs directory look cleaner.

---

# 16. Codex efficiency rules

When beginning work:

```text
1. read authority files
2. identify exact active subsystem
3. use this index to open only relevant detailed docs
4. check RESEARCH_REFERENCE_MATRIX before new external research
5. reuse verified work when source is unchanged
6. do not rerun expensive verification without cause
7. do not regenerate large asset trees without proving they are missing/corrupt
8. keep source SHA, documentation SHA and artifact SHA distinct
9. preserve recovery refs/worktrees before maintenance
10. stop at the active prompt's STOP condition
```

Avoid:

```text
reading every roadmap before a narrow recovery build
reimplementing a solved external format blindly
turning a research document into active scope
using old PROJECT_MAP/NEXT_SESSION_PLAN state as current truth
changing README during engineering cleanup
rewriting mature adapters merely to make architecture look uniform
rerunning full sanitizers because a session restarted with unchanged source
redownloading thousands of sprites when a verified cache already survives
committing large generated `.nro` binaries to normal Git history when a GitHub Release/Actions artifact is the intended persistent distribution path
```

---

# 17. Repository cleanup policy

This repository intentionally keeps engineering history because recovery evidence matters.

"Cleanup" therefore means:

```text
add indexes
add clear authority labels
add cross-links
classify historical vs current
reduce duplicate searching
preserve old evidence
```

It does **not** mean:

```text
delete old prompts
remove recovery refs
throw away build records
move files in ways that break old links
rewrite the user's README
squash away artifact history
```

This index is the preferred map for future Codex sessions.