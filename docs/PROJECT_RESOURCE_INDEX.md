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
| Vault / Banks / Dex | **Vault chunk** below |
| Cross-generation transfer | **Transfer chunk** below |
| Create Pokémon / editor / legality | **Create + Legality chunk** below |
| Events / Mystery Gifts | **Events chunk** below |
| UI / controls / artwork / audio | **UI + Assets chunk** below |
| Build / package / physical test | **Build + Test chunk** below |
| Save/write safety / recovery | **Safety chunk** below |
| Friends / Mailbox / connected features | **Connected chunk** below |
| External project research | `docs/RESEARCH_REFERENCE_MATRIX.md` first |

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
docs/GAME_SUPPORT_MATRIX.md
```

The research matrix already indexes useful DS-era references such as IR-GTS, battle-video tools, egg-move/NARC archaeology and older conversion work. Search the matrix before new GitHub research.

---

# 6. 3DS chunk — Gen VI / VII

Use for XY, ORAS, SM and USUM.

```text
docs/PKHEX_ORACLE.md
docs/SAVE_ENGINE_REFERENCE_AUDIT_2026-09-03.md
docs/RESEARCH_REFERENCE_MATRIX.md
docs/GAME_SUPPORT_MATRIX.md
```

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
```

The research matrix already indexes Switch gift/BCAT research, PokePia/LAN protocol work, BDSP Unity data extraction, poke-client and related tools.

Keep these identities separate from legacy releases, e.g.:

```text
firered_gba != firered_switch
leafgreen_gba != leafgreen_switch
```

---

# 8. Vault / Banks / Pokédex chunk

```text
docs/ARCHITECTURE.md
docs/MASTER_VAULT_SPEC.md
docs/POKEDEX_SPEC.md
docs/PRODUCT_DECISIONS.md
```

Core ownership model:

```text
Master Vault = authoritative immutable Pokémon entities + hashes + provenance
Banks        = logical organization / references
Pokédex      = rebuildable collection index derived from Vault
Game saves   = external sources/destinations behind adapters
```

Do not make Banks or the Pokédex a second independent ownership database.

---

# 9. Transfer / conversion chunk

```text
docs/TRANSFER_MODEL.md
docs/ARCHITECTURE.md
docs/PKHEX_ORACLE.md
docs/HOME_BRIDGE_HISTORICAL_TRANSFER_RESEARCH.md
docs/RESEARCH_REFERENCE_MATRIX.md
```

External references already researched include `Poke_Transporter_GB`, `pk2pk`, PKHeX and HOME-format preservation work.

Permanent distinctions:

```text
COPY != MOVE != CLONE != TRADE
```

Destination compatibility and lossy/irreversible changes must be explicit.

---

# 10. Create Pokémon / editor / legality chunk

This is a permanent product pillar; do not let it disappear from the roadmap.

```text
docs/CREATE_POKEMON_VISION.md
docs/PKHEX_ORACLE.md
docs/PRODUCT_DECISIONS.md
docs/RESEARCH_REFERENCE_MATRIX.md
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

PKHeX / Auto Legality / CoreConsole are host-side research/oracle references unless a later explicit architecture decision changes that.

---

# 11. Events / Mystery Gifts chunk

```text
docs/PRODUCT_DECISIONS.md
docs/FUTURE_PRODUCT_VISION.md
docs/RESEARCH_REFERENCE_MATRIX.md
```

Already researched/indexed references include Project Pokémon EventsGallery, PKHeX event handling, Switch-Gift-Data-Manager, G6 Pokémon Link tooling and IR-GTS/Wonder Card behavior.

Permanent provenance rule:

```text
legality != witnessed historical redemption
```

Never fabricate official server receipt, Nintendo-account history, HOME trackers or witnessed redemption provenance.

---

# 12. UI / controls / assets / audio chunk

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

# 13. Build / package / device-test chunk

```text
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
docs/RELEASE_CHECKLIST.md
docs/DEVICE_TEST_CHECKLIST.md
docs/BUILD_RECORD.md
```

Use `BUILD_RECORD.md` for artifact history, **not** as current-state authority.

Evidence terminology must remain exact:

```text
BUILDS != DEVICE TESTED
DEVICE TESTED != DEVICE ACCEPTED
```

Physical acceptance applies only to the exact tested artifact/source identity.

---

# 14. Save safety / writes / recovery chunk

```text
docs/SAVE_SAFETY.md
docs/ARCHITECTURE.md
docs/TRANSFER_MODEL.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/SESSION2_6_SAFETY_IMPLEMENTATION.md
```

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

# 15. Connected / Trainer Plaza / social chunk

Future-only unless explicitly promoted by `NEXT_CODEX_PROMPT.md`.

```text
docs/TRAINER_PLAZA_VISION.md
docs/V2_PLATFORM_VISION.md
docs/V2_ROADMAP.md
docs/PRODUCT_DECISIONS.md
```

The local Vault remains primary. Online services must not become a mandatory cloud Vault.

---

# 16. External research master index

Before searching GitHub or implementing a Pokémon format/protocol/helper already likely solved elsewhere, open:

```text
docs/RESEARCH_REFERENCE_MATRIX.md
```

It classifies projects as:

```text
DIRECT-CANDIDATE
PINNED-HOST-TOOL
REFERENCE
HISTORICAL
IGNORE / OUT-OF-SCOPE
```

Public visibility is not permission to copy. Check license and asset/data rights separately.

---

# 17. Product roadmap / future ideas

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

# 18. Historical / recovery evidence

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

# 19. AI efficiency rules

Do:

```text
1. read the three authority files
2. identify the active subsystem
3. open only that resource chunk
4. check RESEARCH_REFERENCE_MATRIX before new external research
5. reuse verified source/assets when unchanged
6. push coherent source checkpoints early
7. keep application SHA, docs SHA and artifact SHA distinct
8. stop at NEXT_CODEX_PROMPT's STOP condition
```

Avoid:

```text
reading the entire docs folder
reading every roadmap before a narrow bug fix
redoing external research already indexed
rerunning sanitizers only because a session restarted
redownloading complete asset sets without proving they are missing
using README or an old prompt as active engineering scope
moving/deleting old files merely for cosmetic organization
```

The repository is intentionally organized by **navigation and authority**, not by physically moving every historical file. This keeps old links/recovery evidence stable while allowing AI tools to jump directly to the relevant game family or subsystem.
