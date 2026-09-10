<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a controller-first, local-first Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The long-term goal is one Switch app for legacy emulator saves and modern Switch saves, with personal Vaults, named Banks, collection tracking, safe transfers, legality/provenance tools, and eventually carefully approved staged writing — while keeping the core product local and user-owned.

> **Alpha safety rule:** live installed-game and RetroArch save writing is **HARD DISABLED**. Current legacy milestones are read-only. Writing is only allowed later per adapter after backup, staging, validation, readback, rollback, corruption testing, and physical-device acceptance.

---

# Project status at a glance

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Controller-first UI | ✅ Working |
| HD Pokémon artwork | ✅ 3,260/3,260 verified |
| Modern Switch save browsing foundation | ✅ Working on hardware; per-game production validation continues |
| FireRed GBA | ✅ **Physically accepted** |
| LeafGreen GBA | ✅ **Physically accepted** |
| Ruby GBA | ✅ **Physically accepted** |
| Sapphire GBA | ✅ **Physically accepted** |
| Emerald GBA | ✅ **Physically accepted** |
| Gen III RetroArch read-only milestone | ✅ **Physically accepted** |
| Profile-scoped legacy save assignment | ✅ Working/persistent for accepted Gen III path |
| Gen I Red/Blue/Yellow | ▶️ **Next milestone** |
| Gen II Gold/Silver/Crystal | ⬜ After RBY |
| DS / 3DS mainline games | ⬜ Planned |
| Profile-scoped Vaults | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Transfers / editor / legality expansion | ⬜ Planned |
| Live save writing | 🔒 **HARD DISABLED** |

Development branch:

```text
feature/pokebank-playable
```

Current next milestone:

```text
Red / Blue / Yellow
Game Boy / RetroArch battery saves
strict read-only support
```

---

# Gen III milestone — accepted on real Switch hardware

PokeBank NX now has a physically accepted read-only legacy path for all five mainline Generation III GBA releases:

```text
Pokémon Ruby
Pokémon Sapphire
Pokémon Emerald
Pokémon FireRed
Pokémon LeafGreen
```

## What the accepted Gen III path does

- [x] Discovers normal RetroArch in-game battery saves (`.srm` / supported raw save forms).
- [x] Does not require emulator savestates.
- [x] Keeps distinct physical save files as distinct source instances.
- [x] Uses stable release/platform identities such as `firered_gba`, `ruby_gba`, etc.
- [x] Keeps GBA FireRed/LeafGreen separate from official Switch FireRed/LeafGreen identities.
- [x] Strictly validates Generation III rotating save slots and sectors.
- [x] Validates sector IDs, signatures, counters and checksums.
- [x] Falls back to the older coherent slot when a newer slot is corrupt.
- [x] Reads trainer information.
- [x] Reads Party Pokémon.
- [x] Reads PC Boxes, including PK3 data crossing sector boundaries.
- [x] Reads Pokémon details such as species, PID, TID/SID, EXP, held item, moves/PP, IVs/EVs, nickname and OT where represented.
- [x] Reads Gen III Items/inventory with game-specific layouts.
- [x] Handles Emerald security-key-obfuscated bag quantities correctly.
- [x] Refreshes the current battery save after normal in-game changes.
- [x] Supports profile-scoped assignment/persistence for legacy save sources.
- [x] Preserves source saves as read-only/unchanged.
- [x] Includes FireRed, LeafGreen, Ruby, Sapphire and Emerald game-card artwork.

## FRLG physical acceptance

FireRed and LeafGreen were tested through the real Switch app for:

- save discovery/open;
- trainer, Items, Party and Boxes;
- source assignment;
- assignment persistence after fully closing/reopening PokeBank NX;
- profile isolation and switching back;
- normal RetroArch save changes followed by PokeBank Refresh;
- original save remaining healthy/playable.

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
```

FRLG acceptance checkpoint:

```text
8172ebd9c067bd69df63815dbe865207f905eac6
```

## RSE physical acceptance

Ruby, Sapphire and Emerald were then taken through the same real-device read-only flow. During that work two useful regressions were found and fixed:

1. The first RSE build opened real saves but had empty Items and missing R/S/E game-card art.
2. After inventory was added, a stale legacy bridge guard treated **successful non-empty RSE inventory** as an error and bounced back to the main menu. The bridge was fixed, and optional inventory-model failure can no longer reject an otherwise structurally valid RSE save.

Critical save validation remains strict; this only isolates an optional Items-model error from the core Trainer/Party/Boxes path.

Final accepted RSE runtime source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Exact physically accepted NRO:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
158,120,837 bytes
SHA-256 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

```text
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
GEN III LEGACY READ-ONLY MILESTONE: PHYSICALLY ACCEPTED
```

The user also confirmed the existing Switch save paths continued to open normally while testing the accepted RSE build.

---

# Verification behind the accepted RSE build

Clean host verification:

```text
GitHub Actions run: 34453208654
full host suite: PASS
focused Ruby open: PASS
focused Sapphire open: PASS
focused Emerald open: PASS
valid Ruby/Sapphire/Emerald inventory: PASS
inventory failure rejects whole RSE save: NO
FRLG regression: PASS
source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
```

Exact native/device build:

```text
GitHub Actions run: 34454555232
recovery restore: PASS
application source/tree identity: PASS
device asset preflight: PASS
native devkitA64 compile + final link: PASS
embedded application identity: PASS
embedded RomFS: 3286/3286 PASS
packaging: PASS
artifact upload: PASS
```

---

# Permanent GitHub recovery baseline

The complete current RomFS recovery supply is stored in private GitHub history so a fresh coding session does not have to rebuild/download thousands of assets.

Current snapshot:

```text
2321fa488668e32392de25afed84e38919fbd21f
```

Verified contents:

```text
HD Pokémon renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed card art: PASS
LeafGreen card art: PASS
Ruby card art: PASS
Sapphire card art: PASS
Emerald card art: PASS
RomFS files: 3286
```

Normal recovery is deterministic from GitHub. Generated/manual asset fixes are not supposed to live only in a temporary workspace.

---

# Save safety

Current product contract:

```text
installed Switch source        READ ONLY
RetroArch / legacy source      READ ONLY
backup/staged workspace        separate from live source
live installed save writing    HARD DISABLED
live RetroArch save writing    HARD DISABLED
true Move                       NOT IMPLEMENTED
```

Additional safety work already in place includes:

- source-byte immutability regression tests;
- malformed/truncated input rejection;
- strict Gen III structural validation before browsing;
- graceful handling for malformed/old Legends: Arceus input instead of crashing;
- staged/backup architecture separated from live-source mutation;
- no global unsafe-write switch.

---

# Next: Generation I — Red / Blue / Yellow

The next narrow source milestone is **read-only Game Boy Red/Blue/Yellow support** through normal RetroArch battery saves.

Target scope:

- [ ] `red_gb`, `blue_gb`, `yellow_gb` source discovery/identity.
- [ ] Real Gen I save size/layout/checksum validation.
- [ ] Account for language/region/structural differences rather than assuming one layout fits everything.
- [ ] Trainer data without inventing later-generation fields.
- [ ] Party Pokémon.
- [ ] PC Boxes/storage.
- [ ] Pokémon detail view mapped truthfully from PK1 data.
- [ ] Refresh after normal RetroArch in-game saves.
- [ ] Read-only source-byte immutability.
- [ ] Host + sanitizer regression coverage.
- [ ] Native devkitA64 build and one exact RBY physical-test NRO.

After RBY passes physical testing, the intended next legacy milestone is **Gold / Silver / Crystal**.

The previously discussed RetroArch-per-Switch-user save-routing modification is useful future work, but it is intentionally **not** allowed to interrupt the current RBY milestone.

---

# Main roadmap

## v1.0 — local PokeBank core

- [ ] Profile-scoped Master Vault.
- [ ] Named Banks and collection organization.
- [ ] Immutable original Pokémon payload/hash/provenance model.
- [ ] Search/filter/sort/favorites/recent views.
- [ ] National + game-specific Dex tracking.
- [ ] Living/Shiny/Form/Event collection views.
- [x] Generation III GBA read-only legacy support physically accepted.
- [ ] Generation I GB read-only support.
- [ ] Generation II GBC read-only support.
- [ ] DS Gen IV/V support.
- [ ] 3DS Gen VI/VII support.
- [ ] Production validation/hardening of modern Switch adapters.
- [ ] Safe Copy / Move / Clone semantics.
- [ ] Backup → stage → validate → write → readback → rollback framework.
- [ ] Individually approved write adapters only after hardware safety acceptance.
- [ ] Release-candidate hardware torture pass.

## Later expansion

Planned later work includes Colosseum/XD, Stadium archival support, Create Pokémon/Quick Legal, advanced editor/legality, Transfer Lab, Save Time Machine, event preservation, collection planners, local trading, Android companion, Friends/Mailbox and connected trades.

None of those features are allowed to block finishing the current core milestones.

---

# Project principles

- **Local-first:** core Vault/save/Dex tools should keep working without a server.
- **Read safely first:** mutation comes later and only per individually accepted adapter.
- **No lock-in:** users should be able to export Pokémon and metadata.
- **Never invent history:** unknown provenance stays unknown.
- **Preserve originals:** raw source/Pokémon representation is kept sacred where practical.
- **Trade ≠ Gift:** they remain distinct product operations.
- **Generated Pokémon are labeled honestly:** legality is not the same thing as historical legitimacy.
- **No fake Nintendo/HOME tracker or server history.**
- **No release waits for every future idea.**

---

# Developer handoff

This README is the human-facing dashboard. Engineering sessions should start with:

```text
CURRENT_STATUS.md
↓
docs/CODEX_SESSION.md
↓
docs/NEXT_CODEX_PROMPT.md
↓
one relevant docs/PROJECT_RESOURCE_INDEX.md chunk
```

Do not automatically read every history/research file. Keep coherent source/test/doc checkpoints pushed to `origin/feature/pokebank-playable`.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
