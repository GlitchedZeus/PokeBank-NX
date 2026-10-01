<p align="center">
  <img width="1672" height="941" alt="PokeBank NX" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a native Nintendo Switch homebrew application for browsing, editing, organizing, and preserving Pokémon across multiple generations.

It is being built as an offline, controller-friendly Pokémon bank and save-management front end for CFW Switch systems, with one non-negotiable rule:

> **Your original save is not the working copy.**

Supported sources are discovered and validated, edits are staged inside PokeBank NX, and live source writes stay locked until a source-specific write path has been separately designed, validated, and approved.

> **Project status:** active alpha development. Gen I–III are hardware accepted. Gen IV and the modern Product UI are feature-complete enough for the next integrated hardware pass, with final UI polish still in progress.

## ✨ What works today

- Native Nintendo Switch `.nro` application built with devkitA64/libnx.
- Hardware-accepted Gen I, II, and III save browsing and shared Pokémon editing.
- Staged Gen IV support for Diamond, Pearl, Platinum, HeartGold, and SoulSilver.
- Trainer, Party, Boxes, inventory, Pokémon details, View/Create/Edit, and generation-aware fields where supported.
- Native move pickers, compatibility-aware move presentation, forms, held items, Pokérus, met data, and exact-format handling.
- Modern **Product Home** with selected game, trainer/source information, Party preview, Pokédex progress, Open/Launch actions, and future-feature entry points.
- **Classic Game Sources** for users who prefer the familiar game-first workflow.
- Provider-aware source discovery for RetroArch, DraStic, melonDS, and manual/remembered assignments.
- App-owned launch/link metadata so emulator launching does not require writing into emulator save folders.
- Themes, D-pad + left-stick navigation, held-input repeat, cursor memory, contextual help, and continuing handheld readability polish.

## 🎮 Generation support

| Generation | Games | Current state |
|---|---|---|
| I | Red, Blue, Yellow | ✅ **Hardware accepted** read + staged shared editor |
| II | Gold, Silver, Crystal | ✅ **Hardware accepted** read + staged shared editor |
| III | Ruby, Sapphire, Emerald, FireRed, LeafGreen | ✅ **Hardware accepted** read + staged shared editor |
| IV | Diamond, Pearl, Platinum, HeartGold, SoulSilver | 🧪 Read + staged full-editor foundation implemented; integrated hardware pass pending |
| V+ | Later DS / 3DS / Switch families | 🚧 Planned, research, or validation foundation only |

PokeBank NX exposes fields according to the **real save format**. Older games are not padded with modern data that does not exist in the source.

## 🖥️ Current UI direction

The project has moved away from developer-style diagnostic screens toward a single Switch-native product experience.

Current Product UI work includes:

- a game-focused **Product Home** rather than an equal-card dashboard;
- region-aware hero backdrops for supported games;
- grounded Gen I–IV trainer portraits using proven game/gender identity;
- larger Party presentation and stronger handheld text hierarchy;
- real per-save Gen I–IV Pokédex progress;
- clear Open / Launch actions;
- compact Items and Settings access;
- a two-pane Settings screen with remembered cursor state;
- clean game/source/provider presentation;
- improved asset packaging so trainer/region artwork is carried into native hardware builds;
- remembered focus when returning from reconstructed screens.

The remaining work here is mostly **finishing polish + physical Switch acceptance**, not another UI rewrite.

## 🔒 Safe by design

PokeBank NX is deliberately conservative around save data.

- ✅ Original external saves remain immutable.
- ✅ Emulator source files remain read-only.
- ✅ Installed-title live writes remain disabled.
- ✅ Edits are staged before serialization.
- ✅ Unknown or ambiguous sources fail closed.
- ✅ Launch permission is separate from save-write permission.
- ✅ Destructive operations require deliberate confirmation.
- ✅ Hardware acceptance is tied to an exact build tested on a real Switch.
- ❌ No global unsafe-write switch.
- ❌ Cross-game True Move is not broadly enabled.
- ❌ Source injection is not enabled.

The long-term write model is:

`backup → app-owned working copy → staged mutation → strict validation → explicit source-specific write → readback / recovery`

Each write adapter will be approved independently.

## 🧰 Gen IV status

The Gen IV implementation now covers substantially more than the original read-only milestone:

- Party and Box View/Edit;
- empty Box Add/Create;
- DP / Platinum / HGSS handling;
- Held Item, Language, Ball, Pokérus, Met Location;
- native Gen IV move selection and PP handling;
- species-compatible move filtering;
- Species mutation with dependent-state reconciliation;
- generation/game-aware Form handling;
- trainer/origin inspection;
- Box/Party action parity;
- checksum refresh, strict reparse, and rollback;
- external emulator-source immutability.

The current Gen IV + Product UI head is green under host tests, native Switch compilation, and the Gen IV candidate gate. The next meaningful milestone is **physical acceptance of one exact integrated NRO** after the remaining UI finishing touches.

## 🛡️ Engineering hardening

The repository has also completed a full forensic review and remediation pass.

The current remediation package has:

- **43 findings verified**
- **1 finding deferred with justification**
- **0 open remediation findings**

That work strengthens parser boundaries, memory safety, backup/custody behavior, source-mutation policy, native build coverage, sanitizer coverage, and regression testing. It is intentionally kept separate from feature work until the current UI/hardware checkpoint is ready to integrate.

## 💾 Save sources and launching

PokeBank NX uses provider-aware discovery rather than assuming every save came from the same emulator.

Current foundation includes:

- RetroArch;
- DraStic;
- melonDS;
- manual / remembered source assignments;
- native Switch game identities;
- app-owned ROM/game-file launch bindings.

A save path is **not** treated as proof of a ROM path. If the content target cannot be proven, PokeBank NX asks for an explicit **Link Game File** instead of guessing.

Direct DraStic/melonDS launch handoff is still being verified on hardware.

## 🗺️ Road to v1.0

Near-term order:

1. finish current Product UI polish;
2. build one exact Gen I–IV + Product UI hardware candidate;
3. physically accept that candidate on Switch;
4. integrate the completed remediation package;
5. add full app-wide touch parity;
6. build Master Vault + named Banks;
7. expand providers and later generations;
8. finish collection, provenance, legality, transfer, and approved write workflows;
9. release hardening and v1.0.

Gen V and Master Vault persistence are **not** being started as side effects of the current Gen IV/UI work.

## 🏗️ Development

GitHub is authoritative for active engineering state. Development uses:

- host regression tests;
- ASan / UBSan sanitizer runs;
- source-immutability and custody contracts;
- generated-data and asset validation;
- clean devkitA64 compile/link gates;
- native `.nro` packaging;
- physical Switch testing.

For deeper engineering detail:

- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [v1.0 Roadmap](docs/V1_ROADMAP.md)
- [UI Ownership Status](docs/UI_OWNERSHIP_STATUS.md)

The README stays intentionally product-facing; branch SHAs, CI evidence, and audit details belong in the engineering documents.

## 🔧 Building

PokeBank NX targets Nintendo Switch homebrew using **devkitA64 / libnx**.

Development artifacts are not automatically release builds. A feature becomes hardware accepted only when the exact application commit and exact built NRO are physically tested on a real Switch.

## 📜 License and acknowledgements

PokeBank NX is licensed under **AGPL-3.0**.

The project builds on years of Pokémon save-format research and open-source work across the community. Third-party code, research, and reference projects retain their own licenses and attribution.

## ⚠️ Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc.

Pokémon and related trademarks and game assets are property of their respective owners.
