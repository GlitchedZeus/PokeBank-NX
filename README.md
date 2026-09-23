<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline-first Pokémon bank, save editor, collection manager, and preservation project for **CFW Nintendo Switch**.

The goal is one native `.nro` that can discover supported Pokémon saves, browse and safely stage edits, preserve Pokémon in a PokeBank-owned Master Vault, organize named Banks, track provenance, and eventually perform carefully approved transfer/write workflows without requiring a PC for normal use.

> **Product direction:** HOME-style browsing and organization, Switch-native staged editing, and progressively stronger PKHeX-class validation/conversion capabilities inside one coherent PokeBank NX experience.

**Current status:** 🚧 active alpha development  
**Platform:** 🎮 Nintendo Switch with Atmosphère/CFW  
**Storage model:** 💾 local-first / offline-first  
**Live source writes:** 🔒 hard disabled  
**License:** 📜 AGPL-3.0

---

## ⚠️ Permanent safety rule

PokeBank NX treats original saves as evidence, not scratch space.

- original source saves remain **immutable**;
- installed Switch saves remain **read-only**;
- RetroArch and other emulator-source files remain **read-only**;
- supported editing happens in a **staged PokeBank workspace**;
- staged changes are validated/exported separately;
- live installed-game writing is **HARD DISABLED**;
- live RetroArch writing is **HARD DISABLED**;
- live other-emulator writing is **HARD DISABLED**;
- unknown/unsupported save variants fail closed.

An editable screen does **not** mean PokeBank NX will overwrite the original save.

---

# 📍 Current project state

Last updated: **2026-09-23**

## Gen I–III editor milestone — DEVICE ACCEPTED

The first three generations are now physically accepted on the shared editor architecture.

Exact accepted Generation III checkpoint:

```text
PR:               #77 — Generation III: extend the shared Pokemon editor
Branch:           feature/gen3-shared-pokemon-editor-20260919
Application SHA:  996e6aa40c96e4408282f3d55476dae8e64968b2
Tree SHA:         8826147ff5dc1b498b4b8505c9212243ed2f9498
NRO:              PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro
NRO SHA-256:      ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c
Actions run:      35825830004
Artifact ID:      10735208869
Artifact digest:  sha256:9e052b90ab6d7ab27cfef55f4bb0016bf784174b35fa203bb9dfcef2627b47e6

Status:
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
```

PR #77 remains **OPEN / DRAFT / NOT MERGED** until an explicit merge decision is made. Device acceptance belongs to the exact application SHA/NRO above.

### Accepted editor coverage

| Area | Status |
|---|---|
| Red / Blue / Yellow read + boxed staged editor | ✅ DEVICE ACCEPTED |
| Gold / Silver / Crystal read + shared staged editor | ✅ DEVICE ACCEPTED |
| Ruby / Sapphire / Emerald / FireRed / LeafGreen read + shared staged editor | ✅ DEVICE ACCEPTED |
| Classic staged Inventory editor | ✅ DEVICE ACCEPTED |
| Shared View / Create / Edit architecture | ✅ DEVICE ACCEPTED for Gen I–III |
| D-pad / Left Stick navigation parity | ✅ DEVICE ACCEPTED |
| Held-stick repeat / long-list navigation | ✅ DEVICE ACCEPTED |
| Packed Gen I/II movement + multi-select | ✅ DEVICE ACCEPTED |
| Gen III species shiny preview/commit/cancel | ✅ DEVICE ACCEPTED |
| Gen III EXP numeric editing | ✅ DEVICE ACCEPTED |
| Gen III exact-game clean move picker | ✅ DEVICE ACCEPTED |
| Gen III read-only PID presentation | ✅ DEVICE ACCEPTED |
| 20-theme/readability system, Poké Classic preserved | ✅ DEVICE ACCEPTED in accepted Gen III build |
| Master Vault | ⬜ Not trusted/complete yet |
| Universal SaveSource adapters | ⬜ Planned |
| Nintendo DS / 3DS support | ⬜ Planned |
| Modern Switch per-game validation | 🟨 Foundation exists; production validation pending |
| Live source writing | 🔒 HARD DISABLED |

---

# 🔍 CURRENT NEXT PHASE — full audit, organization, durability hardening

**Do not jump straight to Gen IV, DS/3DS, or live writes.**

With Gen I–III accepted, the immediate project milestone is issue **#69 — full project audit backlog**.

The purpose is to make the storage/transfer foundation trustworthy **before** the Master Vault is allowed to hold somebody's only copy of a Pokémon.

Current confirmed audit priorities include:

1. make Bank persistence atomic/durable instead of overwriting `bank.dat` in place;
2. guarantee held Pokémon cannot be lost during failed rollback;
3. keep original custody payload separate from destination conversion candidates;
4. design one durable transaction for Bank + destination-save operations;
5. preserve every corrupt/unreadable recovery generation instead of replacing one fixed casualty file;
6. harden malformed/truncated save parser boundaries, including BDSP;
7. prevent unsupported larger/newer Bank files from being silently truncated;
8. namespace mutable PokeBank backup workspaces by Switch profile/account;
9. replace in-place backup-save writes with the same durable replacement primitive;
10. re-audit conversion correctness against the exact current source;
11. audit repository/documentation/reference/license organization;
12. physically test crash/recovery behavior on real Switch SD storage.

The repository already contains a detailed map:

- [Full Project Audit](docs/FULL_PROJECT_AUDIT_2026-09-22.md)
- [Technical Reference Index](docs/REFERENCE_INDEX.md)
- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Next Session Plan](docs/NEXT_SESSION_PLAN.md)
- issue **#69** — audit/hardening backlog
- issue **#29** — v1 master roadmap

### Audit sequencing

```text
Gen I / II / III editor milestone        DEVICE ACCEPTED
        ↓
repository + storage + transfer audit    CURRENT
        ↓
reproduction/regression tests for A01–A09
        ↓
durable persistence / recovery primitive
        ↓
conversion + malformed-input re-audit
        ↓
physical crash/recovery testing
        ↓
Master Vault hardening + migration
        ↓
universal SaveSource adapters
        ↓
DS / 3DS
        ↓
modern Switch validation
        ↓
legality / provenance / transfer expansion
        ↓
individually approved writes only after proof
        ↓
v1.0 hardening
```

---

# 🧱 One shared Pokémon editor

Issue #71 / PR #75 froze the permanent editor direction:

```text
shared PokeBank editor UI / lifecycle
        ↓
exact-game capability + provider layer
        ↓
generation-native staged adapter
        ↓
strict serialization / validation boundaries
```

Field state is explicit:

```text
HIDDEN / UNSUPPORTED
DERIVED
READ_ONLY
EDITABLE
```

The exact game/save format decides what the editor exposes. Later-generation fields are never fabricated in older formats.

Examples:

- **Gen I:** DVs, Stat Exp, five native battle stats; no Held Item/Nature/Ability.
- **Gen II:** Held Item, Friendship, Pokérus, DVs/Stat Exp, split SpA/SpD display.
- **Gen III:** IVs/EVs, Nature, Ability, Held Item, Ball, richer met/origin data and PID-linked rules where safely supported.

---

# 🧩 Save/edit architecture

```text
source save
   ↓ read only
immutable original bytes
   ↓
PokeBank staged workspace
   ↓
Review Changes
   ↓
strict serialize / finalize / reparse / validate
   ↓
exported edited copy
   ↓
future individually approved source-specific write adapter
```

`COPY`, `MOVE`, `CLONE`, staged editing, and source writeback remain distinct operations.

A future true Move must never retire a source Pokémon until the destination is durably written and verified.

---

# 🗃️ Master Vault and named Banks

Master Vault is the next major product subsystem **after** the audit/durability layer is trustworthy.

Planned properties include:

- immutable original Pokémon payloads;
- SHA-256 + stable Vault IDs;
- origin/source/platform/save provenance;
- parent/clone/derived relationships;
- active location separate from historical provenance;
- journal/recovery;
- profile-aware ownership;
- named Banks as logical references instead of duplicate authoritative payloads.

Legacy app Storage is **not** automatically the Master Vault. Its migration/retirement path is tracked separately.

---

# 🔎 Universal save discovery

The planned SaveSource architecture expands beyond RetroArch while keeping discovery separate from write permission.

Target sources include:

- RetroArch
- mGBA
- melonDS
- DraStic
- Azahar
- custom folders
- validated unknown-but-recognized save files

Finding a save never grants permission to modify it.

---

# 🎮 Planned expansion

**Nintendo DS:** Diamond, Pearl, Platinum, HeartGold, SoulSilver, Black, White, Black 2, White 2.

**Nintendo 3DS:** X, Y, Omega Ruby, Alpha Sapphire, Sun, Moon, Ultra Sun, Ultra Moon.

**Nintendo Switch validation targets:** Let's Go Pikachu/Eevee, Sword/Shield, Brilliant Diamond/Shining Pearl, Legends: Arceus, Scarlet/Violet, Legends: Z-A, FireRed Switch, LeafGreen Switch.

FireRed/LeafGreen **GBA** and FireRed/LeafGreen **Switch** remain separate release/platform identities.

---

# 🧪 Milestone verification standard

Before a milestone becomes `DEVICE ACCEPTED`, PokeBank NX expects the relevant combination of:

- focused host tests;
- permanent regression suite;
- source-immutability/write-lock checks;
- ASan / UBSan;
- generated-data verification where applicable;
- device asset preflight;
- clean devkitA64 compile/link;
- AArch64/native identity verification;
- embedded source SHA + RomFS verification;
- exact artifact/NRO SHA-256 evidence;
- physical Switch testing of that exact artifact.

`DEVICE ACCEPTED` never transfers automatically to another SHA.

---

# Repository / developer handoff

```text
Repository: GlitchedZeus/PokeBank-NX
Default branch / GitHub landing page: main
Production branch: feature/pokebank-playable
Active Gen III PR: #77
Gen III branch: feature/gen3-shared-pokemon-editor-20260919
Accepted Gen III application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Next engineering milestone:
issue #69 — full audit / organization / durability hardening
```

Canonical runtime root:

```text
sdmc:/switch/PokeBank-NX/
```

PKSE, PKHeX, PKSM, pkmn-chest, OpenHomeNX and other projects are references/oracles or selective integration sources only where licensing, provenance, and PokeBank NX safety boundaries permit.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
