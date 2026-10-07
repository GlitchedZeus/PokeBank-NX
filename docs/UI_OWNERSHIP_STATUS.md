# PokeBank NX UI Ownership Status

> **CURRENT UI SNAPSHOT, NOT BRANCH AUTHORITY:** Re-fetch GitHub before acting on this dated ownership table. Active lane/task routing is defined by `docs/ENGINEERING_AUTHORITY.md` plus the owner's current handoff.


Last updated: **2026-09-29**

This file tracks the visible product layer separately from proven save/editor backends.

| Surface | Current ownership | Backend retained | Remaining visible work |
|---|---|---|---|
| Product Home | **POKEBANK-OWNED** | profile/title/source state, launch model, real per-save Dex/party data | integrated hardware acceptance, touch parity |
| Classic Game Sources | **POKEBANK-OWNED / RESTORED WORKFLOW** | current provider/source model | hardware validation, touch parity |
| Settings | **POKEBANK-OWNED** | persisted settings model | two-pane UI + remembered category/option cursor implemented; hardware validation + touch remain |
| Trainer | MIXED → PokeBank presentation | proven trainer/save models | touch and remaining legacy chrome |
| Party | MIXED → PokeBank presentation | party model + Pokémon entities | touch parity and remaining shared-screen polish |
| Boxes | MIXED | proven box models / staged move safety | PokeBank-owned storage/box presentation over time |
| Inventory / Items | MIXED | proven per-game pouch/item models | bottom Backpack quick-entry uses normal safe open flow; hardware validation + touch remain |
| Pokémon Summary | MIXED | proven parsing / move / compatibility data | finish unified product presentation |
| Pokémon Editor | POKEBANK SHARED SHELL + PROVEN BACKENDS | generation-native staged adapters | Gen IV integrated acceptance, touch parity |
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

Bottom quick actions
  ├─ Backpack / Items
  └─ Settings gear

Settings
  └─ category list + option pane
```

Normal focus uses the PokeBank teal/cyan product accent. Red is reserved for semantic danger/destructive states or restrained brand use.

## Near-term UI boundary

Before the next hardware candidate:

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
