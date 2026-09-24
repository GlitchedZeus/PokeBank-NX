<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

<h1 align="center">PokeBank NX</h1>

<p align="center">
  An offline-first Pokémon bank, save manager and editor for CFW Nintendo Switch.
</p>

<p align="center">
  <img alt="Status: Alpha" src="https://img.shields.io/badge/status-alpha-orange" />
  <img alt="Platform: Nintendo Switch" src="https://img.shields.io/badge/platform-Nintendo%20Switch-E60012" />
  <img alt="CFW: Atmosphère" src="https://img.shields.io/badge/CFW-Atmosph%C3%A8re-blue" />
  <img alt="License: AGPL-3.0" src="https://img.shields.io/badge/license-AGPL--3.0-lightgrey" />
</p>

PokeBank NX is a native Switch homebrew project for browsing, editing, organizing and preserving Pokémon without depending on a PC or cloud service for normal use.

The project is still in active alpha development. The current focus is not adding more generations as fast as possible; it is making storage, transfers and recovery safe enough that the app can eventually be trusted with unique Pokémon.

> **Original game saves are treated as read-only sources.** PokeBank NX works through its own staged workspaces and storage. Live installed-game, RetroArch and other emulator-source writes remain disabled.

---

## Current status

The shared Gen I–III editor milestone is physically tested and accepted on Switch.

The durability audit that followed it has also made major progress on the active audit branch:

- crash-safer Bank persistence with verified replacement and preserved recovery generations;
- custody-safe rollback so a failed move cannot silently destroy a held Pokémon;
- destination conversion candidates kept separate from the authoritative held Pokémon until commit;
- profile + exact-game namespacing for mutable workspaces;
- durable single-file workspace saves for supported modern Switch formats;
- a versioned Move transaction journal with SHA-256 fingerprints and crash recovery;
- software-integrated true Move between PokeBank storage and supported PokeBank-owned workspaces;
- startup recovery, conflict detection and fail-closed transaction locking.

The true-Move layer is **CI verified but not yet power-loss accepted on physical hardware**. Cross-game true Move is intentionally disabled until the conversion-preservation audit is complete, and BDSP remains excluded until its two-file save generation can be committed atomically.

### At a glance

| Area | Status |
|---|---|
| Gen I Red / Blue / Yellow read + staged Pokémon editor | ✅ Device accepted |
| Gen II Gold / Silver / Crystal read + staged Pokémon editor | ✅ Device accepted |
| Gen III R/S/E/FR/LG read + staged Pokémon editor | ✅ Device accepted |
| Classic staged Inventory editor | ✅ Device accepted |
| Controller + left-stick navigation / held repeat | ✅ Device accepted |
| Themes / readability system | ✅ Device accepted |
| Durable Bank storage foundation | ✅ Implemented / CI verified |
| Profile-scoped mutable workspaces | ✅ Implemented / CI verified |
| Bank ↔ supported PokeBank workspace true Move | 🟨 Implemented / CI verified / hardware recovery test pending |
| Cross-game true Move | 🔒 Disabled pending conversion audit |
| BDSP true Move | 🔒 Disabled pending multi-file transaction support |
| Master Vault | 🚧 Planned after durability/conversion gates |
| DS / 3DS | 🗺️ Planned |
| Live installed-game writes | 🔒 Hard disabled |
| Live emulator-source writes | 🔒 Hard disabled |

---

## What PokeBank NX is building toward

The long-term goal is one Switch-native app that can:

- discover supported Pokémon saves and profiles;
- browse Party, Boxes, Trainer data and Inventory;
- stage edits without touching the original source;
- store Pokémon in PokeBank-owned storage;
- move Pokémon between supported workspaces with crash-safe transaction recovery;
- keep provenance, origin and transfer history;
- provide named Banks and an immutable Master Vault;
- support explicit Move, Copy, Exact Clone and Derived Clone semantics;
- grow into DS, 3DS and broader modern Switch support;
- add legality/conversion tooling without hiding data loss or unsupported cases.

The project deliberately separates **finding a save**, **editing a staged copy**, and **permission to write back to a source**. Those are not the same thing.

---

## Safety model

PokeBank NX is designed around a few rules that do not change just because a screen is editable:

- **Original source saves stay immutable.**
- **Installed Switch saves are read-only.**
- **RetroArch and other emulator sources are read-only.**
- **PokeBank-owned staged workspaces may be edited and validated.**
- **Unknown or unsupported save layouts fail closed.**
- **A normal Move must verify the destination before the source is retired.**
- **Recovery evidence is not another withdrawable Pokémon.**
- **Device acceptance belongs to the exact artifact that was physically tested.**

For durable Move operations, the intended order is:

~~~text
prepare transaction
→ write destination
→ read back + verify destination
→ authorize source retirement
→ retire source
→ read back + verify source retirement
→ commit
~~~

If recovery cannot prove what happened, the transaction locks instead of guessing.

---

## Move, Copy and Clone are different things

PokeBank NX keeps these concepts separate by design.

**Move** means the same logical Pokémon changes active location. A normal Move must not silently create a permanent duplicate.

**Copy** is an explicit duplication operation where the source stays active.

**Exact Clone** is the intentional byte-identical clone path where the native payload is preserved as closely as the format allows. The eventual Vault model will still give the clone its own PokeBank identity and lineage.

**Derived Clone** creates a separate related Pokémon and can regenerate generation-specific identity fields when required. It will be recorded as derived rather than pretending to have encounter history it never had.

Transaction/archive bytes are recovery evidence only. They are not active Pokémon and cannot be withdrawn as extra copies.

---

## Game support

### Device-accepted legacy editor milestone

**Generation I**
- Pokémon Red
- Pokémon Blue
- Pokémon Yellow

**Generation II**
- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

**Generation III**
- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

These legacy sources currently remain read-only at the source level; editing is staged inside PokeBank NX.

### Modern Switch workspace foundation

The current PokeBank-owned single-file workspace transaction layer supports the validated save-file path for:

- Pokémon: Let's Go, Pikachu! / Let's Go, Eevee!
- Pokémon Sword / Shield
- Pokémon Legends: Arceus
- Pokémon Scarlet / Violet
- Pokémon Legends: Z-A
- Pokémon FireRed / LeafGreen Switch releases

This does **not** mean PokeBank NX writes those installed-game saves. It means the app can operate on its own validated mutable workspace copies.

**Brilliant Diamond / Shining Pearl** remain excluded from true Move because <code>SaveData.bin</code> and <code>Backup.bin</code> must be treated as one recoverable generation.

FireRed/LeafGreen **GBA** and FireRed/LeafGreen **Switch** are separate game/platform identities throughout the project.

See [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md) for the engineering-level breakdown.

---

## Shared editor architecture

PokeBank NX uses one shared Pokémon editor rather than a different editor shell for every generation.

~~~text
shared editor UI + lifecycle
        ↓
exact-game capability/provider
        ↓
generation-native staged adapter
        ↓
strict serialization + validation
~~~

The save format decides which fields exist.

For example:

- Gen I uses DVs and Stat Exp, with no Held Item, Nature or Ability.
- Gen II adds Held Item, Friendship, Pokérus and its own DV/gender/shiny rules.
- Gen III uses IVs/EVs, Nature, Ability, richer met/origin data and PID-linked behavior.

Fields are hidden, derived, read-only or editable according to the actual game format instead of being invented for older games.

---

## What is being worked on now

The next engineering pass is the **conversion fidelity audit**.

The current transfer foundation can safely commit same-format/native-compatible moves between supported PokeBank-owned stores, but cross-game source retirement stays disabled until conversion behavior is proven with golden fixtures.

That work covers things such as:

- Gen III shiny/PID preservation;
- gender, nature and ability-slot correlations;
- Unown form preservation;
- EV legality across generations;
- nickname/language semantics;
- modern ↔ Gen III conversion loss reporting;
- explicit failure when required traits cannot be preserved.

After that come the remaining parser/recovery hardware gates and Master Vault persistence.

See [Current Status](CURRENT_STATUS.md) and the [v1 Roadmap](docs/V1_ROADMAP.md) for the technical plan.

---

## Roadmap

~~~text
Gen I–III shared editor                     DEVICE ACCEPTED
        ↓
storage / custody / transaction hardening   MOSTLY IMPLEMENTED
        ↓
conversion fidelity + golden fixtures       CURRENT NEXT
        ↓
parser hardening + physical recovery tests
        ↓
Master Vault + named Banks
        ↓
universal SaveSource adapters
        ↓
Nintendo DS / 3DS
        ↓
broader modern Switch validation
        ↓
legality / provenance / transfer tooling
        ↓
individually approved source-write adapters
        ↓
v1.0 hardening
~~~

There is no global "unsafe write" switch planned.

---

## Development

PokeBank NX is built as a native Switch <code>.nro</code> using the devkitPro/devkitA64 toolchain.

The project uses host regression tests, sanitizer builds and native devkitA64 CI before hardware acceptance. Storage and transaction work is also tested with deliberate failure injection so recovery behavior can be exercised without pretending CI is the same as pulling power on a real Switch.

Useful project documents:

- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [Full Project Audit](docs/FULL_PROJECT_AUDIT_2026-09-22.md)
- [Reference Index](docs/REFERENCE_INDEX.md)

### Hardware-tested editor checkpoint

<details>
<summary>Exact accepted Gen I–III / Gen III editor artifact</summary>

~~~text
PR:               #77
Branch:           feature/gen3-shared-pokemon-editor-20260919
Application SHA:  996e6aa40c96e4408282f3d55476dae8e64968b2
Tree SHA:         8826147ff5dc1b498b4b8505c9212243ed2f9498
NRO:              PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro
NRO SHA-256:      ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c
Actions run:      35825830004
Artifact ID:      10735208869

Status:
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
~~~

Device acceptance applies only to that exact tested artifact.

</details>

---

## License

PokeBank NX is licensed under **AGPL-3.0**. See [LICENSE](LICENSE).

External projects such as PKHeX, PKSM/PKSM-Core, PKSE, pkmn-chest and other Pokémon tooling are used as references, compatibility oracles, or selective sources only where their licensing and provenance allow it. See the [Reference Index](docs/REFERENCE_INDEX.md).

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project. It is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK or Creatures Inc.

Pokémon and related trademarks, names and game assets are property of their respective owners.
