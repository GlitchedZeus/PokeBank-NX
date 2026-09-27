<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline-first Pokémon bank, save editor, collection manager, and preservation project for **CFW Nintendo Switch**.

The long-term goal is one native `.nro` that can discover supported Pokémon saves, browse and safely edit them, preserve Pokémon in a PokeBank-owned Master Vault, organize collections into named Banks, track provenance, and eventually perform carefully approved transfer/write workflows without requiring a PC-side tool for normal use.

> **Product direction:** HOME-style browsing and organization, PKSE-style Switch-native interaction, and progressively stronger PKHeX-class validation/creation capabilities — unified into one PokeBank NX experience.

**Current status:** 🚧 active alpha development
**Platform:** 🎮 Nintendo Switch with Atmosphere/CFW
**Storage model:** 💾 local-first / offline-first
**Live writes:** 🔒 hard disabled unless a game/source adapter later earns explicit approval
**License:** 📜 AGPL-3.0

---

## ⚠️ Alpha safety rule

PokeBank NX treats original saves as evidence, not scratch space.

- installed Switch saves remain **read-only unless separately approved later**;
- emulator/source files remain **read-only unless separately approved later**;
- original source bytes are preserved;
- supported editing happens in an **in-memory staged workspace**;
- staged changes can be reviewed and exported as a separate edited copy;
- live installed-game writing is **HARD DISABLED**;
- live RetroArch/emulator-source writing is **HARD DISABLED**;
- party mutation remains deferred where the format needs additional safety proof.

An editable screen does **not** mean PokeBank NX will overwrite the original save.

---

# 📍 Where the project is now

Last updated: **2026-09-27**

Current engineering line:

```text
audit/full-project-hardening-20260923
PR #79 — OPEN / DRAFT / NOT MERGED
Generation IV read-only preview checkpoint: CI verified; hardware/UI smoke test pending
```

PokeBank NX now has hardware-accepted Gen I–III workflows plus a CI-verified **Generation IV read-only preview**. Diamond, Pearl, Platinum, HeartGold and SoulSilver use the same game-cover-first source-assignment model as the earlier generations: once a compatible save is deliberately assigned, the game card remembers it and opens that exact source read-only. Gen IV editing, conversion, live writes and True Move remain disabled.

### Current snapshot

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Standalone PokeBank NX runtime | ✅ Working — PokeBank-owned paths |
| Gen I Red / Blue / Yellow read support | ✅ **Device accepted** |
| Gen II Gold / Silver / Crystal read support | ✅ **Device accepted** |
| Gen III Ruby / Sapphire / Emerald / FRLG read support | ✅ **Device accepted** |
| Classic staged inventory editor | ✅ **Hardware accepted** |
| Exact-game item catalogs / machine names | ✅ Implemented |
| Gen I boxed staged Pokémon editor | ✅ **Device accepted** |
| Gen I passive View / Summary presentation | ✅ **Device accepted** |
| Shared capability-driven Pokémon editor foundation | ✅ Established |
| Gen II shared View / Create / Edit | ✅ Hardware accepted |
| Gen III shared View / Create / Edit | ✅ Hardware accepted |
| Gen IV Diamond / Pearl / Platinum / HG / SS read-only backend | ✅ CI verified |
| Gen IV persistent game cards + assigned-save opening | ✅ CI verified preview; hardware smoke test pending |
| Gen IV editing / conversion / True Move | 🔒 Disabled |
| Master Vault | ⬜ Planned for v1 |
| Named Banks | ⬜ Planned for v1 |
| Emulator/source provider discovery | 🚧 RetroArch / DraStic / melonDS + manual Gen IV assignment implemented; provider architecture expanding |
| Nintendo 3DS support | ⬜ Planned |
| Modern Switch per-game validation | 🟨 Foundation exists; production validation pending |
| Full touch-only UI | ⬜ v1 requirement |
| Live save writing | 🔒 **HARD DISABLED** |

---

# ✅ Physically accepted legacy read foundation

| Game | Platform | Read status |
|---|---|---|
| Red | GB | ✅ Device accepted |
| Blue | GB | ✅ Device accepted |
| Yellow | GB | ✅ Device accepted |
| Gold | GBC | ✅ Device accepted |
| Silver | GBC | ✅ Device accepted |
| Crystal | GBC | ✅ Device accepted |
| FireRed | GBA | ✅ Device accepted |
| LeafGreen | GBA | ✅ Device accepted |
| Ruby | GBA | ✅ Device accepted |
| Sapphire | GBA | ✅ Device accepted |
| Emerald | GBA | ✅ Device accepted |

These accepted paths include Trainer, Party, PC Boxes, generation-appropriate Pokémon details, inventory where supported, strict format validation, source immutability, and bounded legacy save discovery.

Gold/Silver Trainer gender is fixed `Male`; Crystal keeps save-derived Trainer gender. Gen II does **not** fabricate a modern SID.

FireRed/LeafGreen **GBA** and FireRed/LeafGreen **Switch** remain separate release/platform identities.

---

# ✅ Hardware-accepted editor milestones

## Classic Inventory

The classic staged Inventory workflow is hardware accepted with the shared control contract:

```text
A             Edit Amount
X             Add Item
Y             Remove Item
L / R         Previous / Next valid category
+             Options
-             Help
B             Back / Cancel
```

Exact-game/exact-pocket catalogs, empty valid categories, machine move labels, Key Item warnings, staged Pending Changes, and source immutability are part of the accepted direction.

## Generation I boxed Pokémon editor

The accepted Gen I editor is a capability-driven staged editor using the permanent three-panel visual language:

```text
DETAILS | VALUES | MOVES
```

Accepted hardware candidate:

```text
Application source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
Issue #62: CLOSED / DEVICE ACCEPTED
```

The accepted behavior includes staged Create/Edit, transactional Keep/Discard/Continue editing, Y DV randomization, L/R panel navigation, DV-derived shiny behavior, generation-correct fields, semantic move compatibility colors, real five-stat battle presentation, and source immutability.

A later accepted UI follow-up unified Party/Storage/action-sheet **View Pokémon** into the same passive visual language with no editable cursor. PR #67 was merged after hardware acceptance.

---

# 🚧 Current milestone — Generation IV read-only preview

Generation IV is being introduced read-only-first rather than by immediately exposing another writable editor.

Current CI-verified preview includes:

- separate Diamond, Pearl, Platinum, HeartGold and SoulSilver game cards;
- persistent per-profile source assignment;
- bounded discovery for known RetroArch, DraStic and melonDS locations plus manual assignment;
- strict validation before a candidate can be assigned;
- raw Diamond/Pearl ambiguity preserved honestly while the chosen game card retains the user's exact external identity;
- exact HeartGold/SoulSilver ROMCode validation;
- trainer, party, PC boxes and Pokémon details routed through the shared PokeBank presentation;
- explicit Missing, Invalid, Assignment mismatch and Recovered older copy states;
- source files opened read-only with no automatic repair, normalization or reassignment.

### Important status boundary

This Generation IV preview is **CI verified, not device accepted**.

A CI-built NRO is suitable for the owner's first Gen IV hardware/UI smoke test, but the feature will not be called device accepted until that exact artifact is tested on a real Switch.

Generation IV editing, Create/Delete, inventory editing, conversion and True Move remain disabled.

Azahar and Dekopon are planned as future **Nintendo 3DS** source providers using the same persistent game-card/source-assignment architecture; they are not searched for Nintendo DS saves.

---

# 🧱 Permanent Pokémon editor rule

The exact game/save format decides what the editor shows.

> If the field genuinely exists in the exact current save format, expose it. If it does not exist, hide it. Never fabricate modern fields just because the Pokémon exists in a newer generation.

Examples:

- Gen I: DVs, Stat Exp, five battle stats, no Held Item/Nature/Ability/Ribbons.
- Gen II: Held Item, Friendship, Pokérus, DVs/Stat Exp, split SpA/SpD battle display, no Nature/Ability/Ribbons.
- Gen III+: later native fields appear only when the exact game/save really stores them.

Large collections such as Ribbons/Marks should use dedicated collection screens when those generations are implemented.

---

# 🧩 Save/edit architecture

```text
source save
   ↓ read only
immutable original bytes
   ↓
in-memory staged changes
   ↓
Review Changes
   ↓
strict serialize / finalize / reparse / validate
   ↓
export edited copy
   ↓
future individually approved source-specific write adapter
```

`COPY`, `MOVE`, and `CLONE` remain distinct operations. A real Move may only retire/remove the source after a destination has been durably created and validated.

---

# 🗃️ Master Vault and named Banks

The long-term storage model is PokeBank-owned rather than inherited writable PKSE Storage.

Planned Master Vault properties include:

- immutable original Pokémon payloads;
- SHA-256 hashes and stable Vault IDs;
- source game/platform/save provenance;
- parent/clone/derived relationships;
- active-location vs archival-history separation;
- journal/recovery behavior;
- profile-aware ownership.

Named Banks will organize Vault IDs into collections such as Living Dex, Shiny Dex, Events, Trade/Extras, or personal named sets without duplicating the authoritative raw payload.

---

# 🔎 Universal save discovery

PokeBank NX v1 is planned to use a reusable Save Source Adapter architecture rather than staying RetroArch-only.

Target providers include RetroArch, Tico, mGBA, melonDS, DraStic, DeSmuME-compatible sources where their wrappers are understood, Azahar, Dekopon, custom folders, and validated manual files.

Discovery is provider-aware, bounded, provenance-preserving, deduplicated, and separated from write authorization. Azahar and Dekopon are 3DS providers and therefore are not scanned for Diamond/Pearl/Platinum/HeartGold/SoulSilver saves.

---

# 🎮 Planned game expansion

## Nintendo DS

Diamond, Pearl, Platinum, HeartGold, SoulSilver, Black, White, Black 2, White 2.

## Nintendo 3DS

X, Y, Omega Ruby, Alpha Sapphire, Sun, Moon, Ultra Sun, Ultra Moon.

## Nintendo Switch validation targets

Let's Go Pikachu/Eevee, Sword/Shield, Brilliant Diamond/Shining Pearl, Legends: Arceus, Scarlet/Violet, Legends: Z-A, FireRed Switch, and LeafGreen Switch.

Modern families require explicit save-revision/update/DLC validation. One writable game must never globally authorize all games.

---

# 🛣️ High-level v1 path

```text
Gen I / II / III legacy workflows      DEVICE ACCEPTED
shared Gen I / II / III editors        DEVICE ACCEPTED
Gen IV strict read-only backend        CI VERIFIED
Gen IV game cards/source assignment    CURRENT PREVIEW / HARDWARE PENDING
        ↓
Gen IV read-only hardware acceptance
        ↓
Gen IV staged editing only after safety gates
        ↓
Master Vault + named Banks
        ↓
broader SaveSource provider adapters
        ↓
DS Gen V + 3DS identities/read adapters
        ↓
modern Switch validation
        ↓
Summary / provenance / Dex / search
        ↓
conversion / legality / transfer workspace
        ↓
staged transaction framework
        ↓
individually approved writes / true Move
        ↓
full touch-only completion
        ↓
release hardening / RC hardware torture pass
        ↓
v1.0
```

Canonical roadmap tracking remains issue **#29**.

---

# 🧪 Validation philosophy

Before any new device-accepted milestone, PokeBank NX expects the relevant combination of:

- focused host tests;
- permanent regression suite;
- source-immutability checks;
- ASan / UBSan;
- deterministic generated-data verification where applicable;
- device asset preflight;
- clean devkitA64 compile and final link;
- embedded application identity verification;
- complete RomFS verification;
- exact artifact/NRO SHA-256 preservation;
- real physical Switch testing by the owner.

`DEVICE ACCEPTED` is reserved for the exact physically tested artifact.

---

# Repository / developer handoff

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Current focused branch: audit/full-project-hardening-20260923
Current draft PR: #79
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

Useful state files:

```text
CURRENT_STATUS.md
PROJECT_STATUS.md
docs/GAME_SUPPORT_MATRIX.md
docs/V1_ROADMAP.md
docs/NEXT_SESSION_PLAN.md
docs/NEXT_CODEX_PROMPT.md
docs/POKEMON_EDITOR_UI_CONTRACT.md
```

PKSE/PKHeX/PKSM and other projects may be used as references or optional migration/oracle tooling. Custom PokeBank NX work belongs on `origin`; it must not be pushed to upstream reference repositories.

Canonical runtime root:

```text
sdmc:/switch/PokeBank-NX/
```

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
