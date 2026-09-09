# PokeBank NX — AI / Codex Resource Map

Last updated: 2026-09-09

> **Navigation only. This file does not define active scope.**
>
> Current engineering authority is always:
>
> 1. `CURRENT_STATUS.md`
> 2. `docs/CODEX_SESSION.md`
> 3. `docs/NEXT_CODEX_PROMPT.md`
>
> Read this file only to find the small set of supporting documents needed for the active task.

The root `README.md` is the user's human-facing project dashboard. Do not use it as implementation authority.

---

# 1. AI fast path

A normal coding session should usually read only:

```text
CURRENT_STATUS.md
        ↓
docs/CODEX_SESSION.md
        ↓
docs/NEXT_CODEX_PROMPT.md
        ↓
ONE relevant resource chunk below
```

Do **not** read every roadmap, research file, historical device report or old prompt before starting a narrow task.

Before destructive Git/worktree/generated-asset/recovery actions:

```text
inspect worktrees
inspect git status / diff / untracked files
preserve useful local work
preserve recovery refs/assets
then act
```

Writable development target:

```text
origin/feature/pokebank-playable
```

Never push custom PokeBank NX code upstream to PKSE.

---

# 2. Pick the chunk that matches the task

| Task / subsystem | Open these first |
|---|---|
| Current bug / coding milestone | `CURRENT_STATUS.md`, `CODEX_SESSION.md`, `NEXT_CODEX_PROMPT.md` |
| GBA / Gen III saves | **GBA chunk** below |
| GB / GBC saves | **GB/GBC chunk** below |
| DS saves | **DS chunk** below |
| 3DS saves | **3DS chunk** below |
| Modern Switch saves | **Switch chunk** below |
| GameCube Colosseum / XD | **GameCube chunk** below |
| Vault / Banks / Dex | **Vault chunk** below |
| Cross-generation transfer | **Transfer chunk** below |
| Create Pokémon / editor / legality | **Create + Legality chunk** below |
| Events / Mystery Gifts | **Events chunk** below |
| UI / controls / artwork / audio | **UI + Assets chunk** below |
| Build / package / physical test | **Build + Test chunk** below |
| Save/write safety / recovery | **Safety chunk** below |
| Friends / Mailbox / connected features | **Connected chunk** below |
| External project research | `docs/RESEARCH_REFERENCE_MATRIX.md` + relevant research intake/audit |

If a task crosses two chunks, open two. Do not automatically open all of them.

---

# 3. GBA / Gen III chunk

Use for Ruby/Sapphire/Emerald and FireRed/LeafGreen GBA work.

```text
docs/PKSM_CORE_INTEGRATION.md
    Gen III engine integration / adapter decisions

docs/PKHEX_ORACLE.md
    host-side correctness/oracle behavior

docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
    secondary save-engine references

docs/UPSTREAM_AUDIT.md
    inherited PKSE behavior worth reusing

docs/GAME_SUPPORT_MATRIX.md
    exact release/platform identity rules

docs/RESEARCH_REFERENCE_MATRIX.md
    external conversion/save references already researched

docs/RESEARCH_INTAKE_2026-09-09.md
    latest high-value research additions by subsystem
```

For FRLG physical/build work also use:

```text
docs/DEVICE_TEST_CHECKLIST.md
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
GitHub issue #6
```

Current GBA rule: `.sav` / `.srm` battery/in-game saves are the normal RetroArch save sources. Do not invent `.state` support unless a future explicit task requires it.

---

# 4. GB / GBC chunk

Use for Red/Blue/Yellow and Gold/Silver/Crystal.

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
    PHBankGBC and older bank/save behavior research

docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
    pksav and portable save-engine references

docs/PKSM_CORE_INTEGRATION.md
    PK1/PK2 / Sav1/Sav2 engine direction

docs/PKHEX_ORACLE.md
    host-side correctness oracle

docs/GAME_SUPPORT_MATRIX.md
```

Before implementing cross-generation GB/GBC transfer behavior, also open the **Transfer chunk**.

---

# 5. DS chunk — Gen IV / V

Use for Diamond/Pearl/Platinum, HGSS, BW and B2W2.

```text
docs/PKHEX_ORACLE.md
docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
docs/GAME_SUPPORT_MATRIX.md
```

High-value starting references now include:

```text
FlagBrew/PKSM-Core
NaGaa95/DrasticDS_nx
```

Current native DraStic Switch discovery research distinguishes normal battery/cart saves from savestates:

```text
sdmc:/switch/drastic/user/backup/      normal cartridge saves
sdmc:/switch/drastic/user/savestates/ save states
```

For RetroArch melonDS, use the configured frontend Save directory and normal `.srm` saves rather than inventing one universal absolute path.

The research matrix also indexes useful DS-era references such as IR-GTS, battle-video tools, egg-move/NARC archaeology and older conversion work. Search the matrix before new GitHub research.

---

# 6. 3DS chunk — Gen VI / VII

Use for XY, ORAS, SM and USUM.

```text
docs/PKHEX_ORACLE.md
docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
docs/GAME_SUPPORT_MATRIX.md
```

Primary direction is PKSM-Core + PKHeX with older XYSAV/KeySAV-family implementations as independent historical checks.

For Citra/Azahar-style emulator sources, do not accept a file merely because it is named `main`; validate expected size, format and integrity. Emulator-created wrong-size/corrupt files are a known class of edge case worth fixture coverage.

High-value references already indexed include Gen VI save/checksum research, Pokémon Link/event research and historical KeySAV-style tooling. Do not rediscover them from scratch.

---

# 7. Modern Switch chunk

Use for LGPE, SwSh, BDSP, PLA, SV, Z-A and official Switch FR/LG work.

```text
docs/PKHOUSE_REFERENCE.md
docs/OPENHOME_SWITCH_PLATFORM_REFERENCE_AUDIT_2026-09-03.md
docs/PKHEX_ORACLE.md
docs/GAME_SUPPORT_MATRIX.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

Preferred research order before rebuilding modern save behavior from scratch:

```text
PKHeX oracle
    ↓
current upstream kiasta/PKSE
    ↓
Insektaure/pkHouse
    ↓
Insektaure/pkBakery / title-specific tools
    ↓
PokeBank-owned independently tested adapter
```

The current upstream PKSE README now documents hardware-validated support across the main Switch Pokémon families plus native bank/creator/legality/data-table work. Diff/reference upstream before reimplementing a solved modern-game issue, but never push PokeBank custom code upstream.

The research matrix also indexes Switch gift/BCAT research, PokePia/LAN protocol work, BDSP Unity data extraction, poke-client and related tools.

Keep these identities separate from legacy releases, e.g.:

```text
firered_gba != firered_switch
leafgreen_gba != leafgreen_switch
```

---

# 8. GameCube chunk — Colosseum / XD

Future-only until explicitly promoted by `NEXT_CODEX_PROMPT.md`.

```text
docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
docs/RESEARCH_INTAKE_2026-09-09.md
docs/PKHEX_ORACLE.md
docs/GAME_SUPPORT_MATRIX.md
```

Primary reference stack:

```text
PKHeX
logdog2325/PokéBridge
TuxSH/PkmGCTools / LibPkmGC
Dolphin memory-card code
```

PokéBridge is the modern GameCube-homebrew reference. PkmGCTools is an older independent C++ implementation useful as a historical/host oracle, not a default Switch dependency because it is Boost-dependent, exception-using and old.

Initial PokeBank support should be read-only. Do not let future GameCube writeback bypass normal staging/reparse/readback safety.

---

# 9. Vault / Banks / Pokédex chunk

```text
docs/ARCHITECTURE.md
docs/MASTER_VAULT_SPEC.md
docs/POKEDEX_SPEC.md
docs/PRODUCT_DECISIONS.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

Core ownership model:

```text
Master Vault = authoritative immutable Pokémon entities + hashes + provenance
Banks        = logical organization / references
Pokédex      = rebuildable collection index derived from Vault
Game saves   = external sources/destinations behind adapters
```

Important crash-safety reference from the September research intake: `prodeveloper0/uNSS` uses pending/committed revisions so an incomplete upload cannot hide the previous known-good revision. The idea maps well to immutable Vault objects + generation manifests.

Do not make Banks or the Pokédex a second independent ownership database.

---

# 10. Transfer / conversion chunk

```text
docs/TRANSFER_MODEL.md
docs/ARCHITECTURE.md
docs/PKHEX_ORACLE.md
docs/HOME_BRIDGE_HISTORICAL_TRANSFER_RESEARCH.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

External references already researched include `Poke_Transporter_GB`, PCCS, `pk2pk`, current PKSE, PKHeX and HOME-format preservation work.

Prefer explicit generation-boundary conversions with independent test vectors rather than one opaque arbitrary-source-to-arbitrary-destination converter.

Permanent distinctions:

```text
COPY != MOVE != CLONE != TRADE
```

Destination compatibility and lossy/irreversible changes must be explicit.

---

# 11. Create Pokémon / editor / legality chunk

This is a permanent product pillar; do not let it disappear from the roadmap.

```text
docs/CREATE_POKEMON_VISION.md
docs/PKHEX_ORACLE.md
docs/PRODUCT_DECISIONS.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

Product direction:

```text
Quick Legal
Guided Create
Advanced / Expert editor
human-readable legality explanations
destination-aware constraints
Event Library integration
provenance/history
Vault + staging/preview/rollback
```

High-value architecture reference: `projectpokemon/PoGoEncTool` maintains legality source data as JSON and generates a compact PKHeX Legality Binary. PokeBank should consider the same host-generator -> versioned compact runtime-pack pattern rather than shipping giant editable rule databases directly in the NRO.

PKHeX / Auto Legality / CoreConsole are host-side research/oracle references unless a later explicit architecture decision changes that.

---

# 12. Events / Mystery Gifts chunk

```text
docs/PRODUCT_DECISIONS.md
docs/FUTURE_PRODUCT_VISION.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

Already researched/indexed references include Project Pokémon EventsGallery, PKHeX event handling, Switch-Gift-Data-Manager, PKSM, `wc-beacon`, G6 Pokémon Link tooling and IR-GTS/Wonder Card behavior.

Technical availability and redistribution rights are separate questions. EventsGallery can remain an oracle/reference while PokeBank maintains its own generated metadata/index layer.

Permanent provenance rule:

```text
legality != witnessed historical redemption
```

Never fabricate official server receipt, Nintendo-account history, HOME trackers or witnessed redemption provenance.

---

# 13. UI / controls / assets / audio chunk

```text
docs/UI_STYLE_GUIDE.md
docs/UI_FLOW.md
docs/CONTROLS.md
docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md
docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
docs/POKEMON_CRY_AUDIO_AUDIT_2026-09-02.md
```

Project visual material:

```text
assets/screenshots/
assets/game_cards/
assets/banner.png
assets/icon.jpg
```

Generated runtime assets under `romfs/` are not equivalent to Git-tracked `assets/`. Do not regenerate thousands of files until the build/asset gate proves they are missing or corrupt.

Technical availability of Pokémon art/audio does not automatically grant redistribution rights.

---

# 14. Build / package / device-test chunk

```text
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
docs/RELEASE_CHECKLIST.md
docs/DEVICE_TEST_CHECKLIST.md
docs/BUILD_RECORD.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

Use `BUILD_RECORD.md` for artifact history, **not** as current-state authority.

The September research intake also proposes a deterministic multi-generation fixture corpus built from known blank/test saves plus mechanically derived corruption/edge cases. That belongs in host/test tooling when the corresponding adapters become active.

Evidence terminology must remain exact:

```text
BUILDS != DEVICE TESTED
DEVICE TESTED != DEVICE ACCEPTED
```

Physical acceptance applies only to the exact tested artifact/source identity.

---

# 15. Save safety / writes / recovery chunk

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
    immediate libnx/fsdev SD replacement research and LeafGreen relevance

docs/SAVE_SAFETY.md
docs/ARCHITECTURE.md
docs/TRANSFER_MODEL.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/SESSION2_6_SAFETY_IMPLEMENTATION.md
docs/RESEARCH_INTAKE_2026-09-09.md
```

For ordinary SD-card/config/Vault files, keep a `SafeSdFileReplace` abstraction separate from mounted retail-title `SwitchSaveTransaction` semantics.

Default pipeline for any future write-capable adapter:

```text
backup / snapshot
        ↓
stage
        ↓
validate
        ↓
write
        ↓
readback
        ↓
verify
        ↓
commit history OR rollback
```

No global unsafe live-write switch.

---

# 16. Connected / Trainer Plaza / social chunk

Future-only unless explicitly promoted by `NEXT_CODEX_PROMPT.md`.

```text
docs/TRAINER_PLAZA_VISION.md
docs/V2_PLATFORM_VISION.md
docs/V2_ROADMAP.md
docs/PRODUCT_DECISIONS.md
```

The local Vault remains primary. Online services must not become a mandatory cloud Vault.

---

# 17. External research master index

Before searching GitHub or implementing a Pokémon format/protocol/helper already likely solved elsewhere, open:

```text
docs/RESEARCH_REFERENCE_MATRIX.md
```

For the newest September 9 additions, then open only the relevant section of:

```text
docs/RESEARCH_INTAKE_2026-09-09.md
```

Dedicated deep dives currently include:

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
```

Research is classified conceptually as:

```text
DIRECT-CANDIDATE
PINNED-HOST-TOOL
REFERENCE
HISTORICAL
IGNORE / OUT-OF-SCOPE
```

Public visibility is not permission to copy. Check license and asset/data rights separately.

---

# 18. Product roadmap / future ideas

These describe destination, not today's coding scope:

```text
docs/V1_ROADMAP.md
docs/V2_ROADMAP.md
docs/FUTURE_PRODUCT_VISION.md
docs/FEATURE_FEASIBILITY_MATRIX.md
docs/MODDED_SWITCH_FEATURE_BACKLOG.md
docs/PRODUCT_DECISIONS.md
```

`NEXT_CODEX_PROMPT.md` alone decides what gets implemented in the current session.

---

# 19. Historical / recovery evidence

Historical material remains useful, but Codex should not read it by default.

```text
docs/history/
docs/SESSION_LOG_*.md
docs/PROMPT_SESSION*.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
older sections of docs/BUILD_RECORD.md
older device-test reports
```

Historical snapshots can contain stale phrases such as “current” or “next.” They are evidence, not active authority.

We preserve them instead of deleting them so recovery information and previous artifact identities remain available.

---

# 20. AI efficiency rules

Do:

```text
1. read the three authority files
2. identify the active subsystem
3. open only that resource chunk
4. check RESEARCH_REFERENCE_MATRIX / relevant intake section before new external research
5. reuse verified source/assets when unchanged
6. push coherent source checkpoints early
7. keep application SHA, docs SHA and artifact SHA distinct
8. stop at NEXT_CODEX_PROMPT's STOP condition
```

Avoid:

```text
reading the entire docs folder
reading every roadmap before a narrow bug fix
reading the entire research intake when only one section is relevant
redoing external research already indexed
rerunning sanitizers only because a session restarted
redownloading complete asset sets without proving they are missing
using README or an old prompt as active engineering scope
moving/deleting old files merely for cosmetic organization
```

The repository is intentionally organized by **navigation and authority**, not by physically moving every historical file. This keeps old links/recovery evidence stable while allowing AI tools to jump directly to the relevant game family or subsystem.
