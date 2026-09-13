<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline-first Pokémon bank, save editor, collection manager, and preservation project for **CFW Nintendo Switch**.

The long-term goal is simple: one native `.nro` that can discover supported Pokémon saves, browse and safely edit them, store Pokémon in a PokeBank-owned Vault, organize collections into Banks, track provenance, and eventually handle transfers and carefully approved write-back workflows without needing a PC-side tool for normal use.

> **Product direction:** HOME-style browsing and organization, PKSE-style Switch-native interaction, PKHeX-class creation/validation power over time — all brought together as one PokeBank NX experience.

**Current status:** 🚧 active alpha development  
**Platform:** 🎮 Nintendo Switch with Atmosphere/CFW  
**Storage model:** 💾 local-first / offline-first  
**Live writes:** 🔒 hard disabled unless a game/source adapter earns explicit approval  
**License:** 📜 AGPL-3.0

---

## ⚠️ Alpha safety notice

PokeBank NX is already useful, but it is still under active development.

The current safety rule is intentional:

- installed Switch saves are treated as **read-only unless explicitly approved later**;
- emulator/source files are treated as **read-only unless explicitly approved later**;
- original source bytes are preserved;
- edits happen in a **staged workspace**;
- supported staged edits can be reviewed and exported as a separate edited copy;
- live installed-game writes remain **HARD DISABLED**;
- live emulator-source writes remain **HARD DISABLED**.

A screen being editable does **not** mean PokeBank NX will overwrite the original save.

---

# 📍 Where the project is now

Last updated: **2026-09-13**

Active production/development line:

```text
feature/pokebank-playable
```

The project has moved well past the original proof-of-concept stage. The native Switch app boots and runs on real hardware, the standalone runtime no longer requires PKSE folders/helpers, and the first three Pokémon generations now have physically tested save-reading foundations.

The current milestone is the **safe inventory/editor foundation**.

### Current snapshot

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Standalone PokeBank NX runtime | ✅ Working — PokeBank-owned paths |
| Gen I Red / Blue / Yellow read support | ✅ Device accepted |
| Gen II Gold / Silver / Crystal read support | ✅ Device accepted |
| Gen III Ruby / Sapphire / Emerald / FRLG read support | ✅ Device accepted |
| Classic staged inventory editor | ✅ Functional on hardware, final UI polish retest pending |
| Exact-game item catalogs | ✅ Implemented |
| Add items to empty valid categories | ✅ Required / supported by editor model |
| TM/HM/TR move-name display | ✅ Implemented and physically checked |
| Modern Switch backup/workspace inventory editing | ✅ Working foundation; hardware tested |
| Unified Inventory controls | ✅ `A Edit / X Add / Y Remove` |
| PokeBank staged-change highlighting | ✅ Implemented |
| Gen II staged boxed Pokémon editor | ✅ Implemented / validated; broader hardware acceptance still ongoing |
| Host regression suite | ✅ Active |
| ASan / UBSan validation | ✅ Active |
| Native devkitA64 build gates | ✅ Active |
| Master Vault | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Universal emulator discovery | ⬜ Planned for v1 |
| Nintendo DS support | ⬜ Planned |
| Nintendo 3DS support | ⬜ Planned |
| Full touch-only UI | ⬜ Planned for v1 |
| Live save writing | 🔒 HARD DISABLED |

### What is being worked on right now

The newest physical Switch tests are functionally good. The remaining inventory milestone work is mostly polish:

- show the **full staged-edit footer** on classic saves when the staged editor is available;
- keep `X Add Item` available even when a valid category is completely empty;
- hide `A Edit Amount` / `Y Remove Item` only when there is no row to edit/remove;
- improve Add Item picker readability on the physical Switch;
- keep the new slim, centered picker layout;
- run another hardware retest before marking the inventory milestone device accepted.

In other words: the editor works; the current pass is making the UI communicate those capabilities properly.

---

# ✅ What already works

## Standalone Switch app

PokeBank NX now owns its runtime instead of depending on another homebrew app's folder structure.

Canonical app root:

```text
sdmc:/switch/PokeBank-NX/
```

The final product goal is:

```text
Install Atmosphere/CFW
Install PokeBank NX
Open PokeBank NX
Use PokeBank NX
```

No runtime dependency on PKSE, PKSM, pkDex, pkHouse, JKSV, Checkpoint, or PC-side PKHeX is intended for normal PokeBank functionality.

---

## Generation I — Red / Blue / Yellow

Physically tested read support includes:

- save discovery from supported RetroArch battery saves;
- strict save recognition and bounds checking;
- Trainer data;
- Party;
- PC Boxes;
- PK1 details;
- Bag / PC Items;
- generation-appropriate item names and fields;
- source immutability;
- safe malformed-input rejection.

The staged inventory layer can present useful UI views such as Medicines, Poké Balls, Key Items, TM/HM, and PC Items while still respecting the real Gen I Bag/PC storage underneath.

An **empty valid category remains addable**. For example, a new Pokémon Blue save with no Poké Balls yet can still open the Poké Balls view and use `X Add Item` to add game-valid items.

---

## Generation II — Gold / Silver / Crystal

Physically tested read support includes:

- Trainer;
- Party;
- Boxes;
- inventory;
- PK2 details;
- generation-correct Trainer fields;
- source immutability.

Generation-specific behavior is respected:

```text
Gold / Silver trainer gender: Male (game-fixed)
Crystal trainer gender: save-derived
Modern SID field: not fabricated for Gen II
```

The staged editor now also includes:

- Trainer Name / Money editing;
- real Gen II inventory pockets;
- safe staged quantity/add/remove behavior;
- exact game-aware item lists;
- Key Item warnings;
- boxed Pokémon editing / creation foundation;
- Add / Clone / shiny handling with Gen II rules;
- semantic round-trip validation.

Party mutation remains intentionally deferred until the same level of safety is proven.

---

## Generation III — RSE / FireRed / LeafGreen

Physically tested read support includes:

- rotating-sector save parsing;
- signatures, counters, and checksum validation;
- newest-valid-slot selection;
- Trainer;
- Party;
- all PC Boxes;
- PK3 extraction/details;
- FRLG/RSE inventory;
- Emerald quantity handling;
- source immutability.

The staged inventory editor now covers Ruby, Sapphire, Emerald, FireRed, and LeafGreen with generation/game-aware storage rules and checksum repair on exported staged copies.

FireRed/LeafGreen **GBA** and FireRed/LeafGreen **Switch** are treated as separate game identities.

---

# 🎒 Inventory editor

The inventory editor is currently the most mature shared editing workflow in the project.

## Controller layout

For an editable staged inventory:

```text
D-pad / Left Stick   Navigate
A                    Edit Amount
X                    Add Item
Y                    Remove Item
L / R                Change Category
+                    Options
-                    Help
B                    Back / Cancel
```

### Empty categories

A valid category does **not** stop being editable just because it contains zero items.

For an empty category:

```text
(None)

X       Add Item
L / R   Change Category
+       Options
-       Help
B       Back
```

`A Edit Amount` and `Y Remove Item` are hidden only because there is no existing row to act on.

This matters for fresh saves: PokeBank NX should be able to populate a valid empty pocket/category from its exact-game item catalog.

### Read-only source vs staged editing

These are intentionally separate ideas:

```text
SOURCE:  READ ONLY / LIVE LOCKED
EDITOR:  staged editing may still be available
```

PokeBank can safely let you stage a change and export an edited copy without ever writing back to the original source.

---

## Exact-game catalogs

PokeBank NX does not use one giant modern item list for every game.

The rule is:

```text
Game identity
+
current inventory category/pocket
+
that game's real item availability
=
valid Add Item list
```

So Pokémon Yellow does not suddenly offer Dusk Balls, Ability Capsules, later-generation berries, or other impossible items.

The same principle applies to machine labels:

```text
TM01 — Focus Punch
TM02 — Dragon Claw
...
```

Machine numbers are resolved against the correct game/generation instead of a global TM table.

---

## Staged-change colors

PokeBank NX keeps two concepts separate:

- the **game's own `isNew` metadata**;
- whether **PokeBank NX currently has a staged modification** relative to the loaded source.

The game's `isNew` flag is preserved, but it is not used as PokeBank's dirty-state indicator.

Presentation rule:

```text
unchanged source item           normal / white
source isNew only               normal / white
PokeBank staged modification    accent / red
exact revert back to source     normal / white
newly staged item               accent / red
removed item                    appears in Pending Changes
```

That way the UI color means something useful: **this row currently differs from the loaded source**.

---

# 🎮 Modern Switch game foundation

PokeBank NX already has source/editor foundations for modern Switch Pokémon families including:

- Let's Go Pikachu / Eevee;
- Sword / Shield;
- Brilliant Diamond / Shining Pearl;
- Legends: Arceus;
- Scarlet / Violet;
- Legends: Z-A;
- FireRed Switch / LeafGreen Switch identities.

Existing backup/workspace editing can already browse and manipulate several of these on real hardware, and the Inventory control contract has been unified with the classic editor.

However, **live installed-game writing remains locked**. Each game family must independently earn write approval through backup, validation, write, readback, and rollback testing. One game's success will never globally unlock every other game.

---

# 🔒 Save safety architecture

PokeBank NX treats save safety as a product feature.

Current model:

```text
original source save
        ↓
read + fingerprint
        ↓
immutable baseline
        ↓
staged in-memory/workspace edits
        ↓
Review Pending Changes
        ↓
serialize + repair checksum/container
        ↓
strict reparse / validation
        ↓
export edited copy
```

Future source-specific write adapters must add:

```text
backup
→ source fingerprint check
→ validated write
→ readback
→ exact validation
→ rollback/recovery path
```

If a source has changed since it was loaded, the eventual writer should fail safely instead of overwriting newer progress.

---

# 🚧 What comes next

The current near-term order is:

1. **Finish inventory UI hardware acceptance**
   - classic staged footer;
   - empty-category Add Item visibility;
   - picker readability;
   - physical retest.

2. **Generation I full staged Pokémon editor**
   - boxed Pokémon editing;
   - species / nickname / level / EXP;
   - moves / PP;
   - DVs / Stat Experience;
   - OT / TID;
   - Add / Clone;
   - no fake modern fields.

3. **Generation III full staged Pokémon editor**
   - boxed editing / creation;
   - IVs / EVs / nature / ability / shiny / gender;
   - moves / held items / friendship / OT / TID / SID / met data;
   - exact RSE vs FRLG rules.

4. **Shared legacy editor polish**

5. **Master Vault + named Banks**

6. **Universal Save Source adapters**

7. **Nintendo DS + Nintendo 3DS save support**

8. **Modern Switch per-game validation**

9. **Professional Summary / Pokédex / provenance / legality**

10. **Full touch-only handheld coverage**

11. **Individually approved live-write adapters + true Move**

12. **Release hardening → v1.0**

The canonical detailed roadmap is tracked in **issue #29**.

---

# 🏦 Master Vault & Banks — planned

The long-term storage layer will be owned by PokeBank NX rather than simply reusing a game save as a "bank."

Planned Master Vault features include:

- immutable original Pokémon bytes;
- stable Vault IDs;
- hashes;
- source game / platform / save provenance;
- parent / clone / derived relationships;
- current location vs historical provenance;
- backups and recovery;
- profile-aware ownership.

Named Banks will provide flexible organization without duplicating the authoritative Pokémon payload:

- Living Dex;
- Shiny Dex;
- Events;
- Trade / Extras;
- custom personal Banks.

---

# 🔎 Universal save discovery — planned for v1

PokeBank NX is intended to discover saves through reusable adapters instead of being permanently tied to one emulator folder layout.

Planned sources include:

- RetroArch;
- Tico;
- mGBA;
- melonDS;
- DraStic;
- Azahar;
- custom folders;
- unknown-but-valid Pokémon save files.

The source layer should handle emulator configuration, wrapper formats, deduplication, provenance, multiple saves for one game, and source-change detection.

Tracked by **issue #56**.

---

# 👆 Touch support — planned for v1

The final app must be usable with:

```text
controller only   ✅ target
screen only       ✅ target
mixed input       ✅ target
```

Touch-only coverage is planned for Home, game/source selection, Trainer, Inventory, Party, Boxes, Vault/Banks, Pokémon details, dialogs, keyboards, scrolling, and Back/Cancel navigation.

Tracked by **issue #55**.

---

# 🌐 Later / post-v1 ideas

There are also longer-term ideas that should not derail the core offline v1 work, including:

- Friends / PokeBank friend codes;
- gifting Pokémon between PokeBank users;
- transactional trades;
- Mailbox notifications;
- a low-cost privacy-conscious network backend;
- Android companion tooling;
- historical transfer/preservation workflows.

The app remains **offline-first**. Network features should be optional and should never be required for Games, Vault, Banks, local editing, backups, or local transfers.

---

# 🧪 Testing & validation

PokeBank NX uses multiple layers of regression protection:

- focused parser/editor tests;
- full host regression suite;
- ASan;
- UBSan;
- source-immutability checks;
- exact game/item catalog tests;
- build/source policy checks;
- devkitA64 native compile and final link;
- embedded application identity / RomFS checks;
- physical Switch testing for milestone acceptance.

A green host build does not automatically mean "device accepted." Physical hardware results are tracked separately.

---

# 🛠️ Building

PokeBank NX is built with the normal Nintendo Switch homebrew toolchain.

Requirements include:

- devkitPro;
- devkitA64;
- libnx;
- Git submodules where used by the project.

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/GlitchedZeus/PokeBank-NX.git
cd PokeBank-NX
```

Build the native application:

```bash
make
```

The main Makefile expects `DEVKITPRO` to be configured and builds the native **PokeBank NX** `.nro`.

For full validation, use the repository's host-test Makefiles and GitHub Actions workflows rather than treating a successful native compile alone as sufficient proof.

---

# 📲 Installing a development build

Copy the built `.nro` into the PokeBank NX application directory on the Switch SD card, for example:

```text
sdmc:/switch/PokeBank-NX/
```

Then launch it through the Homebrew Menu under Atmosphere/CFW.

Development builds may change data structures and behavior. Keep normal backups of anything important and do not treat an alpha build as a replacement for your original save files.

---

# 🗂️ Repository map

A few useful areas when reading the source:

```text
src/Integration/Gen1     Generation I adapters/editors
src/Integration/Gen2     Generation II adapters/editors
src/Integration/Gen3     Generation III adapters/editors
src/Inventory            shared classic inventory/catalog logic
src/Legacy               legacy save/trainer bridges
src/UI                    Switch UI and interaction code
src/Legality              legality-related foundation
src/Conversion            conversion-related foundation
tests                     host regression tests
romfs                     runtime packaged resources
CURRENT_STATUS.md         detailed engineering handoff/status
PROJECT_STATUS.md         project milestone status
```

---

# 🤝 Project philosophy

A few rules guide development:

- original save data is more important than convenience;
- generation-specific data should stay generation-accurate;
- old games should not be padded with fake modern fields;
- `Copy`, `Move`, `Trade`, and `Clone` are different operations;
- provenance should survive conversions and transfers;
- one safe write adapter must not silently unlock every other game;
- shared/generic fixes that would still be useful without PokeBank NX should be considered for upstream contribution;
- PokeBank-specific Vault, provenance, UI, and transaction architecture stays PokeBank NX's own product work.

---

# 🙏 Credits

PokeBank NX builds on years of work from the Pokémon save-editing and Switch homebrew communities.

Special thanks to:

- **PKSE contributors** — important Switch-native editor foundations and inherited architecture;
- **PKSM / PKSM-Core contributors** — Pokémon data/tooling research and reusable reference work;
- **devkitPro / libnx contributors** — the Switch homebrew toolchain;
- the Pokémon preservation, emulation, and save-research communities whose public work makes projects like this possible.

PokeBank NX is being developed as its own standalone application and product direction while respecting upstream licensing and attribution.

---

# 📜 License

This repository is licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0)**. See [`LICENSE`](LICENSE).

---

# ⚖️ Disclaimer

PokeBank NX is an independent homebrew project and is not affiliated with, endorsed by, or sponsored by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc.

Pokémon and related names are trademarks of their respective owners.

Use PokeBank NX with saves and game data you are legally entitled to use.

---

## Useful project links

- **Current engineering state:** [`CURRENT_STATUS.md`](CURRENT_STATUS.md)
- **Project status:** [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
- **Main roadmap:** issue **#29**
- **Universal inventory editor:** issue **#59**
- **Touch-only v1 requirement:** issue **#55**
- **Universal save discovery:** issue **#56**

If you are following development, the most useful places to look are the production branch, the current status documents, and the open roadmap issues.