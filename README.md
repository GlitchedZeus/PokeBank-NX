<p align="center">
  <img width="1672" height="941" alt="PokeBank NX" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a native Nintendo Switch homebrew application for browsing, editing, organizing, and preserving Pokémon across multiple generations.

The project is built around one rule: **the original save is never treated as a disposable working copy**. PokeBank NX reads supported sources, stages edits inside the app, validates what it can prove, and keeps live source writes locked until a source-specific write path has been separately designed and accepted.

> Current state: **active alpha development**. Gen I–III workflows are hardware accepted. Generation IV now has a staged shared editor and is integrated with the current Product Home, Classic Game Sources, trainer/Dex presentation, and launch/source architecture; that combined build is awaiting its next physical hardware acceptance pass.

## What PokeBank NX does today

- Native Switch `.nro` application with controller-first navigation.
- Hardware-accepted Gen I, II, and III save browsing and shared Pokémon editing.
- Active Gen IV support for Diamond, Pearl, Platinum, HeartGold, and SoulSilver, including staged Party/Box editing, Create, generation-native fields, moves/forms, checksum repair, strict reparse, and rollback.
- Trainer, Party, Boxes, Pokémon details, inventory, Create/Edit workflows, and staged mutation where supported across the current Gen I–IV foundation.
- Generation-aware fields, move compatibility, forms, items, Pokérus, met data, and exact-format presentation.
- Product Home with selected game, trainer/source information, grounded game/gender trainer portrait, real Gen I–IV Pokédex progress, Party sprites, Open/Launch actions, Master Vault preview, and Pokédex preview.
- Familiar **Game Sources** view for users who prefer the classic game-first workflow.
- Compact Items and Settings shortcuts, plus a category-based two-pane Settings screen.
- Provider-aware save discovery and assignment for sources including RetroArch, DraStic, melonDS, and manual files.
- App-owned game-launch/link metadata so emulator launch support can grow without modifying emulator save folders.
- Themes, D-pad/left-stick navigation, held-navigation repeat, contextual help, and ongoing UI polish.

## Supported generations

| Generation | Games | Current state |
|---|---|---|
| I | Red, Blue, Yellow | **Hardware accepted** read + staged shared editor |
| II | Gold, Silver, Crystal | **Hardware accepted** read + staged shared editor |
| III | Ruby, Sapphire, Emerald, FireRed, LeafGreen | **Hardware accepted** read + staged shared editor |
| IV | Diamond, Pearl, Platinum, HeartGold, SoulSilver | Read + staged editor implemented; current full-editor/UI integration is **hardware pending** |
| V+ | Later DS / 3DS / Switch families | Planned or validation foundation only; not advertised as complete |

PokeBank NX exposes fields according to the **actual save format**. It does not invent modern fields for older games.

## Safe by design

The safety model is deliberately conservative:

- original external saves remain immutable;
- emulator source files remain read-only by default;
- live installed-game writing is disabled;
- edits are staged before serialization;
- malformed or ambiguous sources fail closed;
- launch permission is separate from write permission;
- source selection is explicit and provenance-aware;
- destructive actions require deliberate confirmation;
- a feature is not called hardware accepted until the exact built artifact is tested on a real Switch.

The long-term write architecture is **backup → staged working copy → strict validation → source-specific approved write path**. There is no global unsafe-write switch.

## Current interface direction

PokeBank NX is moving toward one consistent Switch-native product shell rather than a collection of developer screens.

Current development includes:

- a modern **Product Home** centered on the selected Pokémon game;
- real Party Pokémon presentation using the existing sprite pipeline;
- trainer names and game-appropriate trainer portraits using only proven game/gender information;
- per-save Pokédex progress where supported;
- a classic **Game Sources** view for direct game-first navigation;
- compact quick access to Games, Banks, Backups, Search, future features, Items, and Settings;
- a two-pane Settings screen organized around user-facing categories;
- distinct visual identities for the future **Master Vault** and **Pokédex**;
- cursor/focus restoration when returning from rebuilt menus where the underlying selection still exists.

The immediate release-engineering target is one exact, fully green Gen I–IV + Product UI NRO for physical Switch testing. After that integrated candidate is accepted, the next major UI milestone is **full app-wide touch control parity**.

## Master Vault and Banks

The future Master Vault is intended to become the authoritative PokeBank-owned Pokémon library.

Planned capabilities include:

- stable Vault IDs and immutable original payloads;
- source game/save/platform provenance;
- transfer history;
- byte-for-byte Clone;
- Legit Clone lineage with parent relationships;
- named Banks and collection views;
- Living Dex / Shiny Living Dex organization;
- recovery/journal behavior.

These systems are intentionally shown as **Coming Soon** until their persistence model is ready. The UI does not fabricate Vault records or counts.

## Save sources and launching

PokeBank NX uses provider-aware source discovery rather than assuming every save came from the same emulator.

Current work covers or is actively integrating:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered source assignment;
- native Switch title identities;
- app-owned ROM/game-file links for launch workflows.

A save path alone is **not** treated as proof of a ROM path. When a launch target cannot be proven, PokeBank NX asks the user to link the game file instead of guessing.

## Development

This repository is under active development and is tested with host regression suites, sanitizers, devkitA64 native builds, source-immutability checks, and physical Switch testing.

For current engineering state, exact active branches, CI boundaries, and hardware checkpoints, see:

- [CURRENT_STATUS.md](CURRENT_STATUS.md)
- [PROJECT_STATUS.md](PROJECT_STATUS.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [v1.0 Roadmap](docs/V1_ROADMAP.md)
- [UI Ownership Status](docs/UI_OWNERSHIP_STATUS.md)

Those files carry the implementation detail so this README can stay focused on the product.

## Building

PokeBank NX targets Nintendo Switch homebrew using devkitA64/libnx. GitHub Actions is used for repeatable host validation and native artifact builds.

Development builds are not automatically considered release builds. Hardware acceptance is tied to an exact application commit and exact NRO hash.

## License and acknowledgements

PokeBank NX is licensed under **AGPL-3.0**. The project builds on years of Pokémon save-format research and open-source work across the community; third-party code and reference projects retain their own licenses and attribution.

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
