<p align="center">
  <img width="1672" height="941" alt="PokeBank NX" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a native Nintendo Switch homebrew application for browsing, editing, organizing, and preserving Pokémon across multiple generations.

The project is built around one rule: **the original save is never treated as a disposable working copy**. Supported sources are discovered and validated, edits are staged inside PokeBank NX, and direct source writes stay locked unless a future source-specific write path earns its own safety approval.

> **Status:** active alpha. Gen I–III are hardware accepted. Generation IV and the current Product UI are integrated and preparing for one combined hardware-test build.

## Highlights

- Native Nintendo Switch `.nro`
- Controller-first interface with D-pad / left-stick parity
- Hardware-accepted Gen I, II, and III read/edit workflows
- Active Gen IV support for Diamond, Pearl, Platinum, HeartGold, and SoulSilver
- Trainer, Party, Boxes, inventory, Pokémon View/Create/Edit, moves, forms, items, Pokérus, met data, and generation-aware fields
- Modern **Product Home** centered on the selected game and save
- Familiar **Game Sources** grid for a classic game-first workflow
- Real Party Pokémon sprites, trainer names, trainer portraits, and per-save Pokédex progress
- Quick **Items/Backpack** and **Settings** access
- Two-pane Settings with remembered cursor state
- Provider-aware discovery for RetroArch, DraStic, melonDS, and remembered/manual sources
- App-owned game-file links for emulator launching without modifying emulator save folders
- Multiple themes, contextual help, and ongoing Switch-native UI polish

## Supported generations

| Generation | Games | Status |
|---|---|---|
| I | Red, Blue, Yellow | **Hardware accepted** read + staged shared editor |
| II | Gold, Silver, Crystal | **Hardware accepted** read + staged shared editor |
| III | Ruby, Sapphire, Emerald, FireRed, LeafGreen | **Hardware accepted** read + staged shared editor |
| IV | Diamond, Pearl, Platinum, HeartGold, SoulSilver | Full staged editor and Product UI integration in active hardware-candidate preparation |
| V+ | Later DS / 3DS / Switch families | Planned or validation/research foundation only |

PokeBank NX exposes fields according to the **actual save format**. Older generations do not gain fake modern fields simply because newer games have them.

## Safe by design

PokeBank NX deliberately separates reading, editing, launching, and writing.

- Original external saves remain immutable.
- Emulator source files remain read-only by default.
- Live installed-game writing is disabled.
- Edits are staged and validated before export/serialization.
- Ambiguous or malformed sources fail closed.
- Game launch permission does **not** grant save-write permission.
- ROM/content links are stored by PokeBank NX rather than written into emulator folders.
- Hardware acceptance is tied to the exact NRO that was physically tested.

The long-term write model is:

```text
source save
  ↓ read-only
staged working copy
  ↓
validation / checksum / reparse
  ↓
explicit source-specific transaction
  ↓
readback / rollback
```

There is no global unsafe-write switch.

## Product experience

The current top-level design is intentionally Pokémon-first rather than file-manager-first.

### Product Home

The selected game is the main focus, with game artwork, trainer identity, source/provider state, real per-save Pokédex progress where supported, the active six-Pokémon Party, Open/Launch actions, Master Vault and Pokédex previews, and compact navigation for Games, Banks, Backups, Search, More, Items, and Settings.

### Classic Game Sources

Users who prefer the older direct workflow can open **Games** and use the familiar game/source grid, then enter the same proven Trainer / Party / Boxes / Inventory / editor paths underneath.

### Settings and quick actions

Settings uses a two-pane category/options layout. The shell also provides compact Items/Backpack and Settings shortcuts, contextual Help, and remembered navigation state when returning from rebuilt menus.

The next major UI milestone after the integrated candidate passes on hardware is **full app-wide touch control parity**.

## Master Vault and Banks

The future **Master Vault** is planned as the authoritative PokeBank-owned Pokémon library, with stable Vault IDs, immutable original payloads, provenance, transfer history, Clone/Legit Clone lineage, named Banks, collection views, and recovery/journal behavior.

These systems remain **Coming Soon** until the persistence model is ready. The UI does not fabricate Vault records or counts.

## Save sources and game launching

PokeBank NX uses provider-aware source discovery rather than assuming every save came from one emulator.

Current source/launch work includes RetroArch, DraStic, melonDS, remembered/manual source assignment, native Switch title identities, and app-owned game-file links.

A save path alone is never treated as proof of a ROM path. If the game content cannot be proven, PokeBank NX asks the user to **Link Game File** instead of guessing.

## Development quality

Development is backed by host regression suites, sanitizers, devkitA64 native builds, source-immutability checks, exact artifact identity checks, and physical Nintendo Switch testing.

A full tracked-repository forensic review was completed in September 2026. Remediation is tracked separately from product development so repository hardening can continue without turning the public front page into an audit log.

For exact engineering state and roadmap detail, see:

- [Current Engineering State](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [v1.0 Roadmap](docs/V1_ROADMAP.md)
- [UI Ownership Status](docs/UI_OWNERSHIP_STATUS.md)

## Building

PokeBank NX targets Nintendo Switch homebrew with **devkitA64 / libnx**. GitHub Actions is used for repeatable host validation and native artifact builds.

Development artifacts are not automatically considered releases. A candidate is only called device accepted after the exact built NRO is physically tested.

## License and acknowledgements

PokeBank NX is licensed under **AGPL-3.0**.

The project builds on years of Pokémon save-format research and open-source work across the community. Third-party code and reference projects retain their own licenses and attribution.

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
