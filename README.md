<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline-first Pokémon bank, save editor, collection manager, and preservation project for **CFW Nintendo Switch**.

The goal is one native `.nro` that can discover supported Pokémon saves, browse and safely edit them, preserve Pokémon in a PokeBank-owned Master Vault, organize collections into named Banks, track provenance, and eventually perform carefully approved transfer/write workflows without requiring a PC for normal use.

> **Product direction:** HOME-style browsing and organization, PKSE-style Switch-native interaction, and progressively stronger PKHeX-class validation/creation capabilities — unified into one PokeBank NX experience.

**Current status:** 🚧 active alpha development  
**Platform:** 🎮 Nintendo Switch with Atmosphère/CFW  
**Storage model:** 💾 local-first / offline-first  
**Live writes:** 🔒 hard disabled  
**License:** 📜 AGPL-3.0

---

## ⚠️ Safety first

PokeBank NX treats original saves as evidence, not scratch space.

- original source saves remain **immutable**;
- installed Switch saves remain **read-only**;
- RetroArch and other emulator-source files remain **read-only**;
- supported editing happens in a **staged PokeBank workspace**;
- staged changes can be reviewed before export;
- live installed-game writing is **HARD DISABLED**;
- live RetroArch writing is **HARD DISABLED**;
- live other-emulator writing is **HARD DISABLED**;
- unknown or unsupported save variants fail closed.

An editable screen does **not** mean PokeBank NX will overwrite the original save.

---

# 📍 Current project state

Last updated: **2026-09-22**

## Production / accepted baseline

```text
Production branch: feature/pokebank-playable
Current production head: 3aeecb8061a3cf8cf19670c042c5ce4cd06330fb

Latest hardware-accepted Gen I/II source:
c24859ce17d33040685ea19b9aff068ba378d8ae

Accepted NRO SHA-256:
0eeef3c4752fc52240332567bf3919099271be4985fda346da26d66018fff82f
```

The Gen I/II editor and storage interaction foundation is now physically accepted. PR #74 is merged, and the pre-Gen-III architecture freeze from issue #71 / PR #75 is complete.

## Current active milestone

```text
Branch: feature/gen3-shared-pokemon-editor-20260919
PR #77 — Generation III: extend the shared Pokemon editor
State: OPEN / DRAFT / NOT MERGED
Current CI candidate: 2e4780412377abab3ffbe4fc2e4757339214a90f
Candidate tree: cdbf0b8faae901c8d765d06b95cac3140bd69e75
Automated CI: VERIFIED for the exact candidate
Hardware state: DEVICE RETEST IN PROGRESS
Device acceptance: NOT DEVICE ACCEPTED
```

Generation III is now being brought onto the **same shared editor architecture** used by the accepted Gen I/II work. The current PR includes exact-format Gen III capabilities/providers, a generation-native staged Pokémon editor, shared View/Edit/Create surfaces, safety-focused field policies, and Gen III-specific sparse box movement behavior.

The current exact PR #77 candidate is CI-verified across the required host/regression/source-safety/sanitizer/native/package gates. The owner is physically retesting the exact GitHub Actions-built NRO now. It is **not DEVICE ACCEPTED** until that exact artifact is accepted on hardware.

Exact candidate being tested:

```text
Application SHA: 2e4780412377abab3ffbe4fc2e4757339214a90f
Tree SHA:        cdbf0b8faae901c8d765d06b95cac3140bd69e75
NRO:             PokeBank-NX-Gen1-UX4-Retest-2e478041.nro
NRO SHA-256:     32b08c1cf589252022e68bf50fe0847fea7cbf1b86835178e7d2a2c268e3b43c
Actions run:     35684844741
Status:          CI VERIFIED / DEVICE RETEST REQUIRED / NOT DEVICE ACCEPTED
```

### Why some editor work was redone

PokeBank NX started from inherited PKSE-era editor behavior, but physical Switch testing exposed places where generation-specific overlays and legacy modal paths had drifted apart. The project therefore froze a **one shared editor** contract in issue #71 / PR #75 and has been moving Gen I, II, and III onto that same capability-driven shell instead of maintaining separate editor products.

That rewrite/reconciliation work is intentional: exact-game storage rules still live in generation-native adapters, while navigation, presentation, staged edit lifecycle, and safety semantics are shared. Hardware findings are treated as regressions to fix on the same line rather than permission to fork another editor.

Current testing is still finding edge cases in shared Gen I/II/III interaction. A green CI build remains only a candidate until the exact Actions artifact is physically accepted.

---

# ✅ What is already device accepted

| Area | Status |
|---|---|
| Native Switch `.nro` runtime | ✅ Working on real hardware |
| Red / Blue / Yellow read support | ✅ Device accepted |
| Gold / Silver / Crystal read support | ✅ Device accepted |
| Ruby / Sapphire / Emerald / FireRed / LeafGreen read support | ✅ Device accepted |
| Classic staged Inventory editor | ✅ Device accepted |
| Gen I boxed staged Pokémon editor | ✅ Device accepted |
| Gen I/II fullscreen View / Edit / Create | ✅ Device accepted |
| Gen I/II exact game identity + generation-correct fields | ✅ Device accepted |
| Gen I/II packed single-Pokémon movement | ✅ Device accepted |
| Gen I/II rectangular multi-select / group movement | ✅ Device accepted |
| Gen I/II Release with confirmation | ✅ Device accepted |
| Gen I/II native 20-slot source boxes | ✅ Device accepted |
| PokeBank Legacy Storage 30-slot boxes | ✅ Device accepted |
| Issue #71 universal editor/reuse architecture freeze | ✅ Complete / merged |
| Gen III shared staged editor | 🚧 PR #77 — CI green, hardware pending |
| Master Vault | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Universal emulator discovery | ⬜ Planned |
| Nintendo DS / 3DS support | ⬜ Planned |
| Modern Switch per-game validation | 🟨 Foundation exists; production validation pending |
| Live save writing | 🔒 HARD DISABLED |

---

# 🎮 Accepted Gen I / II experience

PokeBank NX now has a hardware-accepted shared editor and storage interaction model for:

- **Red / Blue / Yellow**
- **Gold / Silver / Crystal**

Accepted behavior includes:

- dedicated fullscreen **View Pokémon**, **Edit Pokémon**, and **Create Pokémon** pages;
- exact game identity in the UI;
- generation-correct fields rather than fabricated modern metadata;
- staged Create/Edit with source immutability;
- filtered move selection and compatibility presentation;
- generation-correct DVs / Stat Exp;
- derived HP DV behavior;
- split Special presentation where required;
- battle-stat radar presentation;
- A-button Pokémon Actions;
- empty-slot Add Pokémon;
- **Y tap** packed single-Pokémon movement;
- **Y hold** rectangular multi-select;
- deterministic same-box and cross-box packed movement;
- atomic rejection when the destination lacks capacity;
- **B** exact staged restore/cancel;
- Release only through Actions with explicit confirmation.

### Packed vs sparse storage

Gen I/II native boxes are treated as **packed lists**. Moving a Pokémon reorders/inserts/removes entries without inventing permanent holes.

PokeBank-owned Legacy Storage remains separate and uses its own 30-slot box model.

Generation III uses its own exact-format rules; its native box storage is not forced into Gen I/II packed semantics.

---

# 🚧 Current milestone — Generation III shared editor

PR **#77** extends the existing shared editor instead of creating a parallel Gen III-only UI.

Current implementation includes:

- exact identities for Ruby, Sapphire, Emerald, FireRed GBA, and LeafGreen GBA;
- exact-format editor capability/provider plumbing;
- generation-native staged PK3 editing;
- shared editor shell / species-picker reuse;
- shared hardware View/Edit/Create presentation;
- Gen III-specific sparse box movement behavior;
- permanent host/regression tests for the Gen III surface and staged editor;
- safety guards preserving Gen I/II accepted behavior;
- source immutability and all live-write locks.

### Gen III safety boundary

PID-correlated fields such as Nature, Gender, Shiny, Ability/PID relationships remain deliberately constrained until an atomic policy can prove they can be changed without silently breaking related values.

PR #77 is **OPEN / DRAFT / NOT MERGED** and remains hardware-pending.

### Safety work immediately after Gen I–III acceptance

Feature expansion does **not** jump straight from editor acceptance to live writeback. After the first three generations have exact hardware-accepted editor artifacts, the next milestone is a full storage/transfer/conversion audit and hardening pass, followed by Master Vault persistence/recovery validation.

Current audit tracking already includes atomic bank persistence, crash recovery, transfer custody/rollback, malformed-save boundaries, conversion correctness, account/profile namespacing, provenance, and exact-artifact reproducibility. See `docs/FULL_PROJECT_AUDIT_2026-09-22.md` and issue #69.

External technical references and their reuse/license status are indexed in `docs/REFERENCE_INDEX.md`.

---

# 🧱 Universal editor architecture

Issue **#71** is complete and PR **#75** is merged.

The frozen direction for all future generations is:

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
HIDDEN
DERIVED
READ_ONLY
EDITABLE
```

The exact game/save format decides what appears. PokeBank NX does not fabricate fields simply because later generations have them.

Examples:

- **Gen I:** DVs, Stat Exp, five native battle stats, no Held Item/Nature/Ability/Ribbons.
- **Gen II:** Held Item, Friendship, Pokérus, DVs/Stat Exp, split SpA/SpD display, no Nature/Ability/Ribbons.
- **Gen III+:** fields appear only when supported by that exact game/save format and the staged adapter has earned the required safety coverage.

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

`COPY`, `MOVE`, `CLONE`, staged edit, and source writeback remain distinct operations.

A future true Move must never destroy the source until the destination has been durably created and validated.

---

# 🗃️ Master Vault and named Banks

The long-term storage system is PokeBank-owned.

Planned Master Vault properties include:

- immutable original Pokémon payloads;
- SHA-256 / stable Vault IDs;
- source provenance;
- parent / clone / derived relationships;
- journal / recovery behavior;
- profile-aware ownership.

Named Banks will organize Vault IDs into collections such as Living Dex, Shiny Dex, Events, Trade/Extras, and user-defined sets without duplicating the authoritative original payload.

---

# 🔎 Universal save discovery

The planned Save Source Adapter architecture is intended to support more than RetroArch.

Target sources include:

- RetroArch
- mGBA
- melonDS
- DraStic
- Azahar
- custom folders
- validated unknown-but-recognized save files

Discovery and write authorization remain separate concerns. Finding a save never implies permission to modify it.

---

# 🎮 Planned game expansion

**Nintendo DS:** Diamond, Pearl, Platinum, HeartGold, SoulSilver, Black, White, Black 2, White 2.

**Nintendo 3DS:** X, Y, Omega Ruby, Alpha Sapphire, Sun, Moon, Ultra Sun, Ultra Moon.

**Nintendo Switch validation targets:** Let's Go Pikachu/Eevee, Sword/Shield, Brilliant Diamond/Shining Pearl, Legends: Arceus, Scarlet/Violet, Legends: Z-A, FireRed Switch, and LeafGreen Switch.

FireRed/LeafGreen **GBA** and FireRed/LeafGreen **Switch** are always treated as separate release/platform identities.

Modern games require explicit save-revision/update/DLC validation. One writable adapter must never globally authorize unrelated games.

---

# 🛣️ High-level v1 path

```text
Gen I / II / III legacy reads                 DEVICE ACCEPTED
Classic inventory editor                      DEVICE ACCEPTED
Gen I/II shared editor + packed movement       DEVICE ACCEPTED
Universal editor architecture freeze           COMPLETE
        ↓
Gen III shared staged editor                   CURRENT / PR #77 / HARDWARE PENDING
        ↓
Master Vault + named Banks
        ↓
Universal SaveSource adapters
        ↓
DS + 3DS identities/read adapters
        ↓
Modern Switch validation
        ↓
Summary / provenance / Dex / search
        ↓
Conversion / legality / transfer workspace
        ↓
Staged transaction framework
        ↓
Individually approved writes / true Move
        ↓
Full touch-only completion
        ↓
Release hardening / RC hardware torture pass
        ↓
v1.0
```

Canonical roadmap tracking remains issue **#29**.

---

# 🧪 Validation philosophy

Before a milestone becomes device accepted, PokeBank NX expects:

- focused host tests;
- permanent regression coverage;
- source-immutability checks;
- ASan / UBSan;
- generated-data verification where applicable;
- device asset preflight;
- clean devkitA64 compile/link;
- embedded source/RomFS verification;
- exact artifact hashes;
- real physical Switch testing.

`DEVICE ACCEPTED` is reserved for the exact artifact/hash physically tested by the owner.

---

# Repository / developer handoff

```text
Repository: GlitchedZeus/PokeBank-NX
Default branch / landing page: main
Production branch: feature/pokebank-playable
Production head: 3aeecb8061a3cf8cf19670c042c5ce4cd06330fb
Current focused branch: feature/gen3-shared-pokemon-editor-20260919
Current draft PR: #77
Current PR head: dcbd1ff92f1b4f1127c071d3ccb958495436e539
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

Canonical runtime root:

```text
sdmc:/switch/PokeBank-NX/
```

PKSE, PKHeX, PKSM and other projects may be used as references/oracles where licensing and project boundaries permit. Custom PokeBank NX work belongs on `origin`; it must not be pushed to upstream reference repositories.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
