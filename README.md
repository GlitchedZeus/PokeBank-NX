<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a controller-first, local-first Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The idea is simple: one app that can eventually manage Pokémon across the mainline generations, multiple emulator saves, modern Switch saves, personal profile-scoped Vaults, Banks, collection tracking, safe transfers, and optional local/online trading — while keeping the core product owned locally by the user.

> **Current alpha rule:** live save writing is still locked down. Sources stay read-only until a specific adapter has passed backup, staging, validation, readback, rollback, and real-device safety testing.

---

# Project status at a glance

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Controller-first UI | ✅ Working |
| Red PokeBank NX visual identity | ✅ Accepted for development |
| HD Pokémon artwork | ✅ Working on hardware |
| Strict Gen III FRLG parser | ✅ Working |
| FRLG Party + all 14 Boxes | ✅ Working |
| FRLG parent/child save grouping | ✅ Checkpointed |
| FRLG GBA artwork | ✅ Checkpointed |
| FRLG refresh / active-root / trainer / inventory fixes | 🚧 Current milestone |
| Profile-scoped Vaults | ⬜ Planned |
| Banks | ⬜ Planned |
| Mainline GB/GBC/GBA/DS/3DS/Switch coverage | ⬜ v1.0 target |
| Colosseum + XD | ⬜ v1.5 target |
| Stadium + Stadium 2 | ⬜ v1.5 stretch |
| Android companion | ⬜ v2.0 target |
| Friends + Mailbox | ⬜ v2.0 target |
| Internet Gift / Friend Trade | ⬜ v2.0 target |

Current development branch:

```text
feature/pokebank-playable
```

Current milestone:

```text
Finish FireRed / LeafGreen GBA physical-test blockers before moving to RSE.
```

---

# What already works

## App and controls

- [x] Native `.nro` boots and runs on a real Switch.
- [x] D-pad navigation.
- [x] Left-stick navigation.
- [x] Controller-first Action Sheet for deliberate Pokémon actions.
- [x] Handheld testing.
- [x] HOME / sleep / wake / controller reconnect behavior exercised.

## UI and presentation

- [x] PokeBank NX shell and branding.
- [x] Red PokeBank NX identity accepted for development.
- [x] OLED Black, Dark, and Light themes.
- [x] Theme persistence.
- [x] HD Pokémon artwork visible on real hardware.
- [x] Old inherited sprite bobbing/breathing removed.
- [x] Permanent inherited left accent bar removed.
- [x] Broad UI redesign frozen for now so development can focus on Pokémon/save functionality.

## Save safety

- [x] Installed Switch save sources treated as read-only.
- [x] Low-level installed-save writing hard-disabled.
- [x] Old mutation routes blocked from silently writing into installed sources.
- [x] Staged/backup representations kept separate from live sources.
- [x] Malformed/old Legends: Arceus input fails gracefully instead of crashing.
- [x] Source-byte immutability regression tests exist for the Gen III read path.

## Gen III / FireRed / LeafGreen

- [x] Stable release/platform identity system.
- [x] GBA FireRed/LeafGreen stay separate from official Switch FireRed/LeafGreen identities.
- [x] Pinned PKSM-Core host-side correctness oracle.
- [x] Strict FRLG rotating-slot and 14-sector validation.
- [x] Sector IDs, signatures, counters, checksums, and wraparound handling.
- [x] Fallback to older valid slot when the newest slot is damaged.
- [x] Party enumeration.
- [x] All 14 PC Boxes.
- [x] PK3 records crossing a PC-sector boundary.
- [x] Core Pokémon data extraction: species, PID, TID, SID, EXP, item, moves, PP, IVs, EVs, nickname, OT.
- [x] Safe malformed/truncated-save rejection.
- [x] Byte-identical untouched boxed/party PK3 round trips in tests.
- [x] Exception-free native Switch Gen III backend.
- [x] Host tests + ASan/UBSan coverage.

Latest published FRLG application checkpoint:

```text
9e72e2732fd1cd30a3467e9a9a3f7a4f62ca8024
gen3: fix RetroArch grouping scope and artwork
```

That checkpoint includes:

- [x] RetroArch/file legacy sources are app-global instead of tied to a Nintendo profile.
- [x] One FireRed parent and one LeafGreen parent with child save instances.
- [x] Alias dedupe without collapsing genuinely separate physical saves.
- [x] FireRed/LeafGreen GBA card artwork through the shared asset system.

---

# Current FRLG milestone

Before Ruby/Sapphire/Emerald begins, the remaining FRLG device blockers need to be finished and physically retested.

- [x] App-global RetroArch source scope.
- [x] Parent/child hierarchy.
- [x] FRLG GBA artwork.
- [ ] Startup/open-parent/manual **Refresh Saves / Rescan Sources**.
- [ ] Changed-save cache invalidation and safe reread.
- [ ] Deleted/missing-save handling.
- [ ] Configured RetroArch `savefile_directory` is authoritative; conventional path is fallback-only.
- [ ] Truthful FRLG trainer name, gender, TID/SID, combined ID semantics, and money.
- [ ] All six read-only FRLG inventory containers.
- [ ] Full host tests, sanitizers, native build, and diff checks.
- [ ] Restore/verify all 3,260 HD renders.
- [ ] Package a new exact full-asset `.nro`.
- [ ] Physical Switch retest and acceptance.

**RSE stays parked until this FRLG milestone passes on hardware.**

---

# What changed today — September 8, 2026

A lot of long-term product planning was cleaned up so the project can grow without making v1.0 impossible to finish.

- [x] Mainline **GB/GBC/GBA/DS/3DS/Switch** support kept in the v1.0 vision.
- [x] **Colosseum + XD** moved to the v1.5 expansion target.
- [x] **Stadium + Stadium 2** kept as v1.5 stretch/archival targets.
- [x] **Android companion** promoted to a first-class v2.0 feature.
- [x] **PokeBank Friends + Mailbox + Internet Gift/Friend Trade** set as v2.0 targets.
- [x] Wonder/GTS/Room-style public trading moved to later v2.x work.
- [x] Guided **Create Pokémon / Quick Legal** flow designed.
- [x] Full feature catalog and feasibility audit recorded.
- [x] **Verified Breeding** policy defined.
- [x] Physical link-cable trading downgraded to research/non-goal so it never blocks a release.
- [x] Human-facing README and Codex engineering instructions separated.

---

# Release roadmap

## v1.0 — PokeBank NX Core

**Goal:** release a serious, safe, local Pokémon Vault/save platform without waiting for every future idea.

### Core Vault and collection system

- [ ] One independent Vault per Nintendo Switch profile.
- [ ] Named Banks.
- [ ] Stable Vault IDs.
- [ ] Immutable original Pokémon payloads + hashes.
- [ ] Provenance / Pokémon Journey history.
- [ ] Search, filter, sort, favorites, recent, Quick Jump.
- [ ] Smart Bank/query foundation.
- [ ] Vault health, recovery, and journal system.
- [ ] Export/no-lock-in support.

### Mainline game coverage target

- [ ] Red / Blue / Yellow — GB.
- [ ] Gold / Silver / Crystal — GBC.
- [ ] Ruby / Sapphire / Emerald — GBA.
- [x] FireRed / LeafGreen strict parser foundation — GBA.
- [ ] FireRed / LeafGreen full production integration — GBA.
- [ ] Diamond / Pearl / Platinum — DS.
- [ ] HeartGold / SoulSilver — DS.
- [ ] Black / White — DS.
- [ ] Black 2 / White 2 — DS.
- [ ] X / Y — 3DS.
- [ ] Omega Ruby / Alpha Sapphire — 3DS.
- [ ] Sun / Moon — 3DS.
- [ ] Ultra Sun / Ultra Moon — 3DS.
- [ ] Let's Go Pikachu / Eevee — Switch validation.
- [ ] Sword / Shield — Switch validation.
- [ ] Brilliant Diamond / Shining Pearl — Switch validation.
- [ ] Legends: Arceus — Switch validation.
- [ ] Scarlet / Violet — Switch validation.
- [ ] Legends Z-A — Switch validation.
- [ ] Official Switch FireRed / LeafGreen validation in project scope.

### Save-source support

- [x] RetroArch source-provider foundation through FRLG.
- [ ] Source Provider Registry.
- [ ] Multiple independent saves of the same game.
- [ ] Profile assignment for emulator/legacy saves.
- [ ] DraStic source provider.
- [ ] Tico source provider.
- [ ] Dekopon source provider.
- [ ] Checkpoint-style backup provider.
- [ ] Manual Import fallback.
- [ ] Unknown/Other provider fallback.

### Collection and analysis

- [ ] Professional Pokémon Summary.
- [ ] Judge-style IV/EV/stat display.
- [ ] National Dex.
- [ ] Game-specific Dexes.
- [ ] Living Dex tracking.
- [ ] Shiny/Form collection tracking.
- [ ] `LEGAL / INVALID / UNKNOWN` legality state.
- [ ] Human-readable provenance timeline.

### Safe transfer/write system

- [ ] Explicit `COPY`, `MOVE`, and `CLONE` semantics.
- [ ] Safe staged destination representations.
- [ ] Backup → stage → validate → write → readback → rollback framework.
- [ ] Individually approved write adapters.
- [ ] True `MOVE` only after destination success is verified.
- [ ] No global unsafe write switch.

### v1.0 release quality

- [ ] Save Health / Vault Health UI.
- [ ] Interrupted-transaction recovery.
- [ ] Handheld + docked testing.
- [ ] Accessibility and Reduced Motion options.
- [ ] Large-Vault performance/caching work.
- [ ] Reproducible exact `.nro` artifacts.
- [ ] Release-candidate hardware torture pass.
- [ ] **v1.0 release.**

---

## v1.5 — Expanded Collection & Legacy Console Edition

**Goal:** deepen the local/offline product after 1.0 without waiting for the connected ecosystem.

### More games

- [ ] **Pokémon Colosseum**.
- [ ] **Pokémon XD: Gale of Darkness**.
- [ ] Shadow Pokémon / purification / GameCube provenance.
- [ ] **Pokémon Stadium** — stretch.
- [ ] **Pokémon Stadium 2** — stretch.
- [ ] More emulator/source providers as needed.

### Guided creation and editing

- [ ] Game/generation-aware **Create Pokémon** wizard.
- [ ] Required / Optional / Conditional field system.
- [ ] Missing-field blocker before creation.
- [ ] Immediate impossible-value warnings.
- [ ] **Quick Legal** mode.
- [ ] Advanced controller-friendly editor.
- [ ] Clone / Make Shiny safeguards with provenance.

### Transfer Lab

- [ ] Destination compatibility preview.
- [ ] Move/form/ability/ribbon change preview.
- [ ] Irreversible-change warnings.
- [ ] Per-game moveset memory where practical.
- [ ] Cross-generation conversion validation.

### Save Time Machine

- [ ] Automatic pre-write snapshots.
- [ ] Manual/named snapshots.
- [ ] Save-to-save comparison.
- [ ] Party / Boxes / Items / Trainer / Dex diffs.
- [ ] Explicit safe restore flow.

### Collection intelligence

- [ ] “What am I missing?” planner.
- [ ] Living Dex planner.
- [ ] Shiny/Form/Gender/Regional planners.
- [ ] Evolution Planner.
- [ ] Ribbon Master Planner.
- [ ] Shiny-hunt tracking.
- [ ] Team Builder using Vault Pokémon.
- [ ] Pokémon comparison.
- [ ] Duplicate Detective.

### Verified Breeding

- [ ] `FOUNDER VERIFIED` starting Pokémon.
- [ ] `LINEAGE VERIFIED` offspring with known parent Vault IDs.
- [ ] `LINEAGE UNKNOWN` for imported offspring whose historical parents cannot be proven.
- [ ] Only Founder Verified / Lineage Verified Pokémon can participate in Verified Breeding.
- [ ] Generation-aware breeding/inheritance rules.
- [ ] Breeding Planner.
- [ ] Navigable family tree for PokeBank-managed generations.

### Local trading

- [ ] Same-Switch Gift between profiles.
- [ ] Same-Switch mandatory one-for-one Trade.
- [ ] Nearby/LAN Gift and Trade between two Switches.
- [ ] QR/code local pairing.
- [ ] Full crash-safe trade provenance.

### Events and extras

- [ ] Historical Event / Mystery Gift catalog foundation.
- [ ] EventDex / GiftDex.
- [ ] Challenges / achievements.
- [ ] Trainer Passport / profile customization.
- [ ] Vault Points and cosmetic rewards.
- [ ] Collector medals.
- [ ] Artwork/theme-pack system.
- [ ] Pokémon cries/sound packs where licensing allows.

---

## v2.0 — PokeBank NX Connected

**Goal:** add the Android companion and the first real connected/social layer while keeping the Switch Vault local-first.

### Android companion

- [ ] Pair to a PokeBank profile by QR/code.
- [ ] Browse/search a synchronized or read-only Vault index.
- [ ] View Summary / Judge / legality / provenance.
- [ ] Dex and collection-planning views.
- [ ] Trainer Passport.
- [ ] Friends and Mailbox.
- [ ] Trade/Gift notifications.
- [ ] Accept/decline friend requests.
- [ ] Review incoming Gifts and Trades.
- [ ] Prepare outgoing Gifts/Trades.
- [ ] Trade/Gift history.
- [ ] Backup/Vault health overview.
- [ ] Encrypted local-network sync.

### PokeBank Friends and Mailbox

- [ ] PokeBank profile ID/code.
- [ ] Add friend by code/QR.
- [ ] Friend request → Mailbox → accept/decline.
- [ ] Remove/block friend.
- [ ] Minimal public profile only.
- [ ] Friends cannot browse your Vault by default.

Mailbox categories:

```text
Friend Requests
Trade Requests
Gifts
Completed Trades
System Messages
```

### Internet exchange

- [ ] Internet Gift to Friend.
- [ ] Mandatory one-for-one Internet Friend Trade.
- [ ] Atomic Vault-to-Vault transaction flow.
- [ ] Signed requests / hashes / transaction IDs.
- [ ] Encrypted payloads where practical.
- [ ] Expiry / replay protection / retry handling.
- [ ] No ownership retirement until destination verification succeeds.

### Lightweight relay

- [ ] Small mailbox/transaction backend.
- [ ] No mandatory cloud copy of the full Vault.
- [ ] Server stores only minimal identity/friend/mailbox/transaction data.
- [ ] Small encrypted exchange payloads.
- [ ] Designed for inexpensive/free-tier hosting at small scale.

### Not required for the first 2.0 release

These can come later in v2.x:

- [ ] Wonder Exchange / Wonder Box-style public pool.
- [ ] GTS-style Trade Board.
- [ ] Room Trade.
- [ ] Global Battle Lab / usage statistics.
- [ ] Desktop/web/iOS companion clients.

---

# Long-term ideas that do NOT block releases

These are cool, but they are not allowed to hold v1.0/v1.5/v2.0 hostage.

- Historical Mystery Gift/Event preservation expansion.
- Advanced battle analytics.
- More companion clients.
- More emulator providers.
- Additional side-game integrations.
- Experimental physical cartridge/save bridges.
- Physical link-cable trade-station research.

The physical link-cable idea is explicitly **research/non-goal** for normal releases.

---

# Project principles

- **Local-first:** core Vault/save/Dex tools keep working without a server.
- **No lock-in:** users should be able to export their Pokémon and metadata.
- **Read safely first:** writing comes later and only per verified adapter.
- **Never invent history:** unknown provenance stays unknown.
- **Trade ≠ Gift:** Trade requires something from both sides; Gift is deliberate one-way transfer.
- **Save ownership stays local to the profile:** cross-profile movement happens Vault-to-Vault.
- **Generated Pokémon are labeled honestly:** legality and historical legitimacy are different concepts.
- **No fake Nintendo/HOME protocols or tracker data.**
- **No release waits for every future feature.**

---

# Developer note

This README is intentionally the **human-facing project dashboard**.

Detailed Codex/recovery/build/test instructions are kept separately under `docs/` so the project roadmap stays readable and coding sessions have their own authoritative handoff files.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
