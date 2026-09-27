<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

<h1 align="center">PokeBank NX</h1>

<p align="center">
  Offline-first Pokémon storage, save browsing and editing for CFW Nintendo Switch.
</p>

<p align="center">
  <img alt="Status: Alpha" src="https://img.shields.io/badge/status-alpha-orange" />
  <img alt="Platform: Nintendo Switch" src="https://img.shields.io/badge/platform-Nintendo%20Switch-E60012" />
  <img alt="CFW: Atmosphère" src="https://img.shields.io/badge/CFW-Atmosph%C3%A8re-blue" />
  <img alt="License: AGPL-3.0" src="https://img.shields.io/badge/license-AGPL--3.0-lightgrey" />
</p>

PokeBank NX is a native Nintendo Switch homebrew project for browsing, editing, organizing and preserving Pokémon without requiring a PC or cloud service for normal use.

The project is still in active **alpha** development. The goal is not to race through generation support; it is to build a bank and save-management foundation that can be trusted with Pokémon people actually care about.

**Last updated:** September 27, 2026

> **Original game saves are treated as immutable sources.** Editing happens in PokeBank-owned staged workspaces. Live writes to installed games, RetroArch saves and other emulator sources remain disabled.

---

## Current status

The **Generation I–III shared editor milestone is physically tested and device accepted** on Switch.

Since that milestone, most engineering work has focused on storage safety, crash recovery, conversion fidelity and byte-exact save behavior.

Current highlights:

- Red / Blue / Yellow, Gold / Silver / Crystal and Ruby / Sapphire / Emerald / FireRed / LeafGreen have the shared staged Pokémon editor;
- controller navigation, left-stick parity, held-repeat scrolling, themes and the accepted legacy editor UI have been tested on real hardware;
- PokeBank-owned storage uses verified durable replacement and retained recovery generations;
- Move transactions use a journal, SHA-256 evidence and restart recovery rather than assuming a write completed;
- the physical interruption/recovery harness passed **all 8 tests on a FAT32 Switch SD setup**;
- Sword/Shield ↔ Scarlet/Violet has completed its converter-level audit for all eight title directions;
- unsupported or unexplained conversion data now fails closed instead of being silently erased;
- Gen I–III has been re-audited against **PKSE 1.2** for format correctness and byte fidelity, with fixes landed for several real legacy-save edge cases;
- PKSE 1.2 is now also being treated as a high-value reference/port source for future Gen IV–VII format support.

**Cross-game product True Move is still disabled.** Converter readiness and transaction safety are prerequisites, not permission to remove a Pokémon from its source.

### At a glance

| Area | Status |
|---|---|
| Gen I Red / Blue / Yellow read + staged Pokémon editor | ✅ Device accepted |
| Gen II Gold / Silver / Crystal read + staged Pokémon editor | ✅ Device accepted |
| Gen III R/S/E/FR/LG read + staged Pokémon editor | ✅ Device accepted |
| Classic staged Inventory editor | ✅ Device accepted |
| Controller + left-stick navigation / held repeat | ✅ Device accepted |
| Themes / dark-light readability system | ✅ Device accepted |
| Durable Bank persistence foundation | ✅ Implemented / validated |
| Move journal + restart recovery | ✅ Implemented |
| Physical interruption/recovery on FAT32 | ✅ Device accepted — 8/8 tests |
| Sword/Shield ↔ Scarlet/Violet conversion layer | ✅ Converter ready, product route still locked |
| Gen I–III PKSE 1.2 correctness audit | ✅ Complete / CI verified |
| Cross-game True Move | 🔒 Disabled |
| BDSP true Move | 🔒 Disabled pending multi-file transaction support |
| Nintendo DS / 3DS | 🗺️ Next expansion family after the current audit gate |
| Master Vault | 🚧 Planned — not started |
| Live installed-game writes | 🔒 Hard disabled |
| Live emulator-source writes | 🔒 Hard disabled |

---

## Supported legacy games

### Generation I
- Pokémon Red
- Pokémon Blue
- Pokémon Yellow

### Generation II
- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

### Generation III
- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

These are currently treated as **read-only source saves**. PokeBank NX can create and edit staged copies without mutating the original file.

The editor is shared across generations rather than being rebuilt from scratch for every game:

~~~text
shared editor UI
        ↓
exact-game capability provider
        ↓
generation-native staged adapter
        ↓
strict serialization + validation
~~~

The format decides what exists. Gen I uses DVs and Stat Exp; Gen II adds Held Item, Friendship and Pokérus; Gen III uses IVs/EVs, Nature, Ability, richer origin data and PID-linked mechanics.

---

## What the PKSE 1.2 audit changed

PokeBank NX already had a substantial Gen I–III implementation, but PKSE 1.2 added enough old-generation work that it became worth doing a direct delta audit before moving to Gen IV.

That audit has already caught useful edge cases, including:

- the proper Gen III empty-party **no-mail sentinel**;
- preserving an existing Gen III 100-byte party tail instead of recalculating unrelated stored battle state;
- complete Gen III egg placeholder/language behavior;
- recognized emulator RTC-footer preservation;
- Gen I/II list-marker and text-decoding edge cases;
- language-aware Gen III text handling;
- exact Ruby/Sapphire/Emerald/FireRed/LeafGreen source context for legality feedback;
- byte-preserving Gen II Clone behavior for native OT/nickname data.

PokeBank NX does **not** blindly replace its architecture with PKSE.

The intended split is:

~~~text
PKSE / PKHeX / PKSM-Core
        ↓
format knowledge, tables, parsers, correctness oracles

PokeBank NX
        ↓
staging, source immutability, UI, storage,
durability, provenance, recovery and Move policy
~~~

If upstream behavior is safer or more correct, it can be ported or adapted. If PokeBank NX already has the safer design, it stays.

---

## Storage and recovery safety

PokeBank NX treats editing and source-write permission as separate concepts.

Permanent rules:

- **Original source saves stay immutable.**
- **Installed Switch saves are read-only sources.**
- **RetroArch and other emulator saves are read-only sources.**
- **PokeBank-owned staged workspaces may be edited.**
- **Unknown or unsupported layouts fail closed.**
- **A Move must prove the destination before source retirement can even be authorized.**
- **Recovery evidence is not an extra withdrawable Pokémon.**
- **Hardware acceptance belongs only to the exact artifact that was actually tested.**

The durable Move order is designed around:

~~~text
prepare transaction
→ write destination
→ read back + verify destination
→ authorize source retirement
→ retire source
→ read back + verify retirement
→ commit
~~~

If recovery cannot prove what happened, it stops instead of guessing.

### FAT32 physical durability checkpoint

The dedicated physical audit NRO was run through all eight guided interruption/restart cases on real Switch hardware using a FAT32 SD setup.

~~~text
Application SHA:
cf1e390f9f9cba13c64c6f500df0bf3ff5060048

NRO SHA-256:
122302db3189d532e82833400eab0453301c56dc7d48c3966648b02ec62c101a

Result:
8 / 8 PASS
journal = COMMITTED
idempotent recovery = PASS
product True Move = DISABLED
~~~

That acceptance proves the tested durability/recovery gate on that hardware/filesystem environment. It does not automatically enable a product Move route.

---

## Conversion work

PokeBank NX conversion is intentionally fail-closed.

For Sword / Shield ↔ Scarlet / Violet, all eight exact title directions have completed the current converter-level audit:

- Sword → Scarlet
- Sword → Violet
- Shield → Scarlet
- Shield → Violet
- Scarlet → Sword
- Scarlet → Shield
- Violet → Sword
- Violet → Shield

The converter now has explicit handling for destination stat reconstruction, HP/status changes, unsupported forms, reserved/unknown data and declared losses/adaptations.

**Converter ready does not mean True Move enabled.** The product gate remains locked until an individual route is intentionally approved.

---

## Move, Copy and Clone

These operations are deliberately separate.

**Move** — the same logical Pokémon changes active location. The source is retired only after the destination is durably committed and verified.

**Copy** — an explicit duplicate where the source remains active.

**Exact Clone** — intentional native-payload duplication as closely as the format allows.

**Derived / Legit Clone** — a separate related Pokémon with its own future Vault identity and lineage relationship rather than pretending it is the same encounter object.

Transaction evidence and backup bytes are recovery material, not extra active Pokémon.

---

## What comes next

The current engineering line is **PR #79 — `audit/full-project-hardening-20260923`**. The Gen I–III PKSE 1.2 correctness audit is complete and exact-head CI verified at `5b62e2ba`. PR #79 remains open/draft/unmerged. The next tranche is a **Gen IV file-by-file PKSE 1.2 reuse/delta audit** before any Gen IV implementation begins.

The immediate engineering sequence is:

~~~text
Gen I–III shared editor                     DEVICE ACCEPTED
        ↓
storage / transaction hardening             IMPLEMENTED
        ↓
FAT32 interruption / recovery               DEVICE ACCEPTED
        ↓
modern conversion fidelity                  CONVERTER CLOSURE COMPLETE
        ↓
Gen I–III vs PKSE 1.2 audit                 COMPLETE / CI VERIFIED
        ↓
PKSE 1.2 Gen IV reuse / delta audit
        ↓
Diamond / Pearl / Platinum / HGSS backend
        ↓
Black / White / B2W2
        ↓
X / Y / ORAS
        ↓
Sun / Moon / USUM
        ↓
Master Vault / broader v1 hardening
~~~

For Gen IV–VII, the plan is **not** to rebuild everything from zero. PKSE 1.2 already contains useful Pokémon-format, encryption, save-layout, checksum, trainer, inventory and data-table implementations. Those will be audited against PKHeX and adapted behind PokeBank NX's existing safety architecture.

See:
- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [Full Project Audit](docs/FULL_PROJECT_AUDIT_2026-09-22.md)
- [Reference Index](docs/REFERENCE_INDEX.md)

---

## Hardware-tested editor checkpoint

<details>
<summary>Exact accepted Gen I–III editor artifact</summary>

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
GENERATION III EDITOR MILESTONE ACCEPTED
~~~

Device acceptance applies only to that exact tested artifact.

</details>

---

## Development

PokeBank NX is built as a native Switch `.nro` using devkitPro/devkitA64.

The project uses:

- host regression tests;
- ASan / UBSan;
- native devkitA64 compile/link validation;
- byte-exact save fixtures;
- golden conversion fixtures;
- source-immutability tests;
- deliberate transaction interruption/fault injection;
- exact artifact hashes for hardware checkpoints.

The project remains an alpha and should be treated accordingly. Unique saves should always be backed up independently.

---

## License

PokeBank NX is licensed under **AGPL-3.0**. See [LICENSE](LICENSE).

PKHeX, PKSM / PKSM-Core, PKSE, pkmn-chest and other Pokémon tooling are used as references, compatibility oracles or selective upstream sources where appropriate. PokeBank NX keeps its own safety, storage, provenance and product-policy boundaries.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project. It is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK or Creatures Inc.

Pokémon and related trademarks, names and game assets are property of their respective owners.
