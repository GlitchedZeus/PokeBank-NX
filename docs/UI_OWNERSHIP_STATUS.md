# PokeBank NX UI Ownership Status

Last updated: **2026-10-14**

This file tracks the visible product layer separately from proven save/editor backends. The October 8 Gen I–IV owner-tested UI is the previous acceptance checkpoint; newer Claude [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) and Gen V [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) UIs are **development candidates**, not merged releases.

| Surface | Current ownership | Backend retained | Remaining visible work |
|---|---|---|---|
| Product Home | **POKEBANK-OWNED** | profile/title/source state, launch model, regional/game art and Trainer/Party preview | validate in-NRO 10-region / 20-trainer artwork, Gen V cards and touch on real Switch |
| Classic Game Sources | **POKEBANK-OWNED / RESTORED WORKFLOW** | provider-neutral Save Instances, profile bindings, Gen V exact primary/backup source choice | verify fair Gen IV roots, Gen V profile/restart/backup-copy UI and touch |
| Settings | **POKEBANK-OWNED** | persisted settings model | hardware validation, touch |
| Trainer | MIXED → PokeBank presentation | proven trainer/save models | touch and remaining legacy chrome |
| Party | MIXED → PokeBank presentation | party model + Pokémon entities | touch parity and remaining shared-screen polish |
| Boxes | MIXED | proven box models / staged move safety | PokeBank-owned storage/box presentation over time |
| Inventory / Items | MIXED | proven per-game pouch/item models | Backpack quick-entry hardware validation + touch |
| Pokémon Summary | MIXED | proven parsing / move / compatibility data | finish unified product presentation |
| Pokémon Editor | POKEBANK SHARED SHELL + GENERATION-NATIVE BACKENDS | Gen I–IV native staged editors; experimental Gen V PK5 staged Edit/Create/Review [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) | current Gen I–IV finalization hardware; Gen V exact save/editor round |
| Backups | MIXED | proven backup-copy plumbing | polished backup/recovery browser |
| Banks | POKEBANK-OWNED PREVIEW | none persistent yet | real backend later |
| Master Vault | POKEBANK-OWNED PREVIEW | none persistent yet | real persistence/journal later |
| Pokédex | POKEBANK-OWNED PRESENTATION | per-save Gen I–IV progress exposed | future global Dex/collections backend |
| Search | POKEBANK-OWNED PREVIEW | no global Vault index yet | real index/backend later |
| More / future modules | POKEBANK-OWNED PREVIEW | no fake backend | implement features only when real |

## Current UI direction

The retired equal-card developer dashboard is not the product target.

The product hierarchy is:

```text
Product Home
  ├─ selected game / trainer / source / Dex / Party
  ├─ grounded trainer portrait
  ├─ Open
  ├─ Launch
  ├─ Master Vault preview
  ├─ Pokédex preview
  └─ compact navigation / quick actions

Games
  └─ Classic Game Sources
       └─ proven existing game/editor flows

Settings
  └─ category list + option pane
```

Normal focus uses the PokeBank teal/cyan product accent. Red is reserved for semantic danger/destructive states or restrained brand use.

## Near-term UI boundary

The active October 14 lanes are **Gen I–IV finalization [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306)** (hardware candidate/Gen IV/native/touch checks passed, full host still running, device pending) and **Gen V [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300)** (six CI checks green; hardware pending). These both extend the one shared PokeBank Pokémon editor; neither authorizes source writeback.

Before the next accepted hardware candidate:

- keep real party sprites;
- keep real Gen I–IV Dex progress;
- keep Gen IV trainer names;
- keep grounded trainer portraits;
- validate Settings and cursor-memory behavior;
- validate Backpack quick entry;
- verify emulator launch paths;
- produce one exact integrated NRO.

After that passes hardware, full touch parity becomes the next UI milestone.

Internal class names such as `PKSEFramebuffer` and namespaces such as `PokeVault::` remain implementation details, not the public product identity. Required upstream attribution and third-party licenses remain intact.

### Gen V Source / Game File UX — implemented on draft branch

For Black, White, Black 2 and White 2, the active profile's X/Source menu now includes **Assign Save to This Profile**, **Browse Saves / Open Shared Editor**, **Assign Save File Manually**, **Forget Remembered Save** (Y confirmation) and Cancel. Assignment persists only app-owned profile/file metadata and returns to Games. A/Open on the game chooses/revalidates the exact BW/B2W2 Save Instance and Primary/Backup copy before opening the shared editor. The emulator source remains read-only. **This interaction still requires a Switch test**, especially after the emulator updates a previously assigned save.

### Art packaging and launch identity

Claude's [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) adds actual NRO-packaged asset validation and ROM cartridge-header exact-release checks. Older recovery packaging discarded regional backdrops/trainer portraits. Current code packages 10 region scenes, 20 trainer portraits and Gen I–IV cover art, and the newer exact-head native/Gen IV candidate gates have passed; full host verification and owner hardware testing are still required. Preserve the already accepted Quit & Return behavior.

[October 14 engineering snapshot](ENGINEERING_SNAPSHOT_20261014.md) · [Game support matrix](GAME_SUPPORT_MATRIX.md)
