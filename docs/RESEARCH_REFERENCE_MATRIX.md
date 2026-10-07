# PokeBank NX — Research / Reference Matrix

Last updated: 2026-09-08

> **Research-only / scope-control document.** This inventory exists to reduce duplicated reverse-engineering work and to preserve useful external references. It does **not** expand the current FRLG/RSE implementation task. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active coding work.

## Why this exists

PokeBank NX should not spend years independently rediscovering Pokémon formats, checksums, transfer rules, event formats, LAN behavior, collection UX, or tooling patterns that the community has already researched.

At the same time, PokeBank NX must not become an unmaintainable pile of copied projects or accidentally import code under incompatible/unclear licenses.

Every external project should therefore be classified before use:

```text
DIRECT-CANDIDATE
    Code may be suitable for direct integration after exact license,
    attribution, architecture and maintenance review.

PINNED-HOST-TOOL
    Useful in developer/test tooling but not a Switch runtime dependency.

REFERENCE
    Study behavior, file formats, algorithms, protocol facts, UX or tests;
    implement behind PokeBank-owned interfaces.

HISTORICAL
    Old/superseded implementation useful for independent regression checks,
    archaeology, old edge cases or test vectors.

IGNORE / OUT-OF-SCOPE
    Does not materially help the current product, or a better source exists.
```

## Licensing rule

Repository visibility is **not** permission to copy code.

- MIT/BSD/ISC or similarly permissive code may be a direct-integration candidate after exact file-level/license review and required notices.
- GPL code should default to `REFERENCE` or a deliberately separated developer/host tool unless PokeBank NX intentionally chooses a compatible licensing strategy.
- Repositories with no clear license should be treated as `REFERENCE` only: do not copy source code.
- Fair-code/non-commercial/custom licenses are reference-only unless their exact terms are reviewed and compatible.
- Data, sprites, event archives, ROM-derived tables, screenshots and other assets may have licenses/rights separate from the repository's source-code license.
- A useful idea, protocol fact, file offset or observed behavior can be independently implemented without importing another project's UI or architecture wholesale.

The preferred integration model remains:

```text
external research / mature implementation
            ↓
PokeBank adapter / generated table / test oracle
            ↓
PokeBank stable service/API
            ↓
PokeBank safety + provenance + transactions
            ↓
PokeBank controller-first UI
```

---

# Tier A — primary engineering references

These should be checked before implementing their corresponding PokeBank feature from scratch.

## kwsch/PKHeX

**Role:** primary host-side correctness/reference baseline.

Use for:

- Pokémon format field behavior;
- legality analysis;
- encounters;
- generation-aware creation;
- conversion;
- Mystery Gifts/events;
- normalized test vectors;
- save-format research.

PokeBank policy: pinned host oracle/reference, not a desktop process required by the Switch runtime. See `docs/PKHEX_ORACLE.md` and `docs/CREATE_POKEMON_VISION.md`.

## PKSM-Core / inherited PKSE research

**Role:** existing native-compatible engine/save research and current PokeBank foundation.

Use behind PokeBank engine/save-adapter boundaries rather than leaking concrete external types throughout UI/business logic.

## Striaton-Lab-Team/Poke_Transporter_GB

**Classification:** `DIRECT-CANDIDATE` for carefully selected permissively licensed logic; otherwise `REFERENCE`.

**License:** MIT for project code according to the repository.

**High value.** Actively maintained Gen I/II -> Gen III transfer tool. It supports RBY/GSC to RSE/FRLG and explicitly uses the Pokémon Community Conversion Standard (PCCS).

Use for:

- Gen I/II -> Gen III conversion policy;
- Transfer Lab behavior;
- mapping properties that do not exist across generations;
- move/species/trainer conversion decisions;
- language/version edge cases;
- transfer UX ideas;
- independent test vectors against PKHeX/PokeBank conversion;
- researching the linked `GearsProgress/Pokemon-Community-Conversion-Standard`.

Do **not** copy its destructive source-save behavior into PokeBank. PokeBank keeps its own staging/backups/provenance/transaction semantics.

## Manu098vm/Switch-Gift-Data-Manager

**Classification:** `REFERENCE`.

**License:** GPL-3.0.

**High value.** Creates custom/forged BCAT packages for restoring Switch-era Mystery Gift content through the games' own Mystery Gift flow.

Known supported families include:

```text
LGPE     .wb7full
SwSh     .wc8
BDSP     .wb8
PLA      .wa8
SV       .wc9
Z-A      .wa9
BCAT bundles
```

Use for:

- Event Library / EventDex research;
- Switch Wonder Card parsing/validation;
- BCAT structure/CRC/offset research;
- historical-event restoration workflows;
- per-game gift compatibility;
- Event Gallery integration research;
- distinguishing game-save edits from BCAT/event-delivery state.

Also research its upstream/reference sources, especially `projectpokemon/EventsGallery`, PKHeX and pkNX.

PokeBank must retain its provenance rule: a locally restored event is not falsely labeled as a witnessed official live-server redemption.

## kwsch/PokePiaSWSH

**Classification:** `REFERENCE`.

**License:** GPL-3.0.

**High value for future LAN work.** Implements Pokémon Sword/Shield LAN trade protocol flow over UDP, including matchmaking, host handshake, station connection, mesh join, initial trade broadcast capture and party `.pk8` dumping.

Use for:

- future Switch-to-Switch LAN research;
- game-native LAN discovery/handshake concepts;
- protocol test fixtures;
- understanding Pia behavior;
- future local exchange diagnostics.

PokeBank Gift/Trade still uses its own Vault transaction, ownership, provenance, replay and crash-recovery guarantees.

## JamieJQuinn/IR-GTS

**Classification:** `REFERENCE`.

**License:** GPL-3.0.

**High value for preservation/networking.** Hosts a custom Gen IV GTS + Wonder Card service to which retail DS games can connect after Nintendo's original services shut down.

Use for:

- post-official-shutdown service design;
- Gen IV GTS request/response research;
- DNS redirection/network flow concepts;
- send/receive Pokémon protocol behavior;
- Wonder Card delivery research;
- duplicate handling;
- future PokeBank Trade Board/GTS preservation ideas;
- self-hostable community-service philosophy.

Do not reuse its insecure legacy-network assumptions for PokeBank Connected; DS-era WEP/DNS requirements are game constraints, not a model for the new protocol.

## Reisyukaku/poke-client

**Classification:** `REFERENCE` / possible developer-tool inspiration.

**License:** GPL-2.0.

Runs on real Switch hardware and provides a Pokémon reverse-engineering environment based on exlaunch, including a hex editor, Lua VM, SD-card scripts/logging and game-specific TitleID configuration.

Use for:

- modern-game reverse engineering;
- developer diagnostics;
- live-memory observation during adapter research;
- Lua/scriptable research workflows;
- possible future PokeBank developer/debug tooling.

Do not make live-memory mutation the normal PokeBank user architecture; safe save/Vault workflows remain preferred.

## foohyfooh/PKHeXPluginPile (and user-supplied egzn fork)

**Classification:** `REFERENCE` until exact per-project licensing is clear.

The maintained upstream currently contains multiple focused PKHeX plugins such as:

- sorting;
- Special Pokémon Finder;
- Raid Importer;
- SwSh Rules Exporter;
- SV Vivillon tooling;
- SV profile picture viewer;
- fashion editor;
- roamer tooling;
- shared plugin utilities.

Use for:

- Bulk Box/Vault organization behavior;
- special-Pokémon queries;
- raid/event workflows;
- game-rule extraction ideas;
- profile/cosmetic metadata research;
- small solved-problem tests that should not be reinvented.

Prefer evaluating current upstream `foohyfooh/PKHeXPluginPile` rather than assuming an older fork is authoritative.

## kwsch/CoreConsole

**Classification:** `REFERENCE` / `PINNED-HOST-TOOL` inspiration.

**License:** GPL-3.0.

CLI interface for Auto Legality Mod / legality checking.

Use for:

- PKHeX Oracle command architecture;
- Quick Legal host-side research;
- driving Auto-Legality functionality without PKHeX desktop UI;
- machine-readable creator regression testing.

Do not make the old CLI itself the Switch runtime creator engine.

## kwsch/HomeRip

**Classification:** `REFERENCE`.

**License:** GPL-3.0.

Parses Pokémon HOME `*.perbin` FlatBuffers into more readable representations.

Use for:

- HOME local-data format preservation research;
- migration/import research where user-owned local HOME data is available;
- understanding metadata representation;
- future archival tooling.

Do not assume this provides access to a user's entire server-side HOME collection.

## kwsch/pk2pk

**Classification:** `HISTORICAL` / `REFERENCE`.

Transfer/"Time Capsule" tool built on PKHeX's class library. It explored both official forward transfers and best-effort unofficial/backward conversions.

Use for:

- Transfer Lab UX and policy ideas;
- lossy/backward conversion warnings;
- field-preservation comparisons;
- historical conversion edge cases.

Current PKHeX and PokeBank's own conversion model remain the primary correctness baseline.

## kwsch/XYSAV

**Classification:** `HISTORICAL` / `REFERENCE`.

Contains X/Y save-size logic, CRC16-CCITT code, block offsets/lengths and checksum validation.

Use for:

- independent Gen VI save validation;
- Rescue Mode research;
- checksum regression vectors;
- triangulation against current PKHeX/PKSM/PokeBank adapters.

Do not trust decade-old offsets blindly without comparison to current implementations.

## kwsch/UnityDPtools

**Classification:** `REFERENCE`.

BDSP Unity ROM-data parser/ripper.

Use for:

- BDSP game-rule/data extraction;
- species/evolution/move/encounter-related table research;
- generating compact PokeBank data tables rather than running the original tool in the NRO.

## carimatics/switch-poke-pilot

**Classification:** `REFERENCE`; permissive-code reuse may be evaluated case-by-case.

**License:** MIT.

Switch automation framework designed around easy APIs and independently loadable command directories.

Use for:

- future provider/plugin architecture;
- command isolation;
- developer automation;
- user-installable helper architecture ideas.

Do not turn PokeBank into a general game-input bot; the useful part is extensibility architecture.

---

# Tier B — feature / UX / data references

Useful for PokeBank features, but not core runtime engines.

## ItsMeJoji/PokeTrainerTools

**Classification:** `REFERENCE` (no clear repository license detected at review time).

Active 2026 collection of trainer utilities.

Useful concepts/data surfaces:

- Pokémon encounter lookup;
- location/encounter-rate calculator;
- catch-rate calculator;
- shiny-odds calculator across methods/generations;
- SOS move/PP tracking;
- PLA Massive Mass Outbreak permutations;
- Ribbon Tracker and Ribbon Master guide;
- privacy-focused client-side operation;
- sync/backup UX.

Maps to PokeBank's Living Dex Brain, hunt manager, Ribbon Master Mode, encounter intelligence and collection planning.

## PoshoDev/DexTool

**Classification:** `REFERENCE` (no clear repository license detected at review time).

Box-oriented Living Dex and shiny management tool with status tracking, quick generation jumps, screenshots/exports and automatic backups.

Use for:

- Living/Shiny Dex UX;
- box/grid progress visualization;
- obtained vs not-obtained states;
- fast navigation;
- progress export/card ideas;
- automatic-backup UX.

PokeBank has an advantage because its Dex state should be derived from actual Vault entities rather than manually maintained flags.

## vinnydiehl/pokelog

**Classification:** `REFERENCE` only.

**License:** custom/fair-code/non-commercial restrictions; do not reuse code without exact review.

EV-training tracker that tracks trainees, defeated species and resulting EV gains across a party.

Use for:

- EV training planner/tracker UX;
- party-aware EV accounting concepts;
- explaining EV mechanics;
- future "train this Pokémon" plans based on actual Vault entities.

## DigitalFlow/Pokemon-Team-Builder

**Classification:** `REFERENCE`; old MIT code may be reviewed if ever necessary.

**License:** MIT.

Old team-suggestion tool using Pokémon Global Link / Smogon usage statistics, tiers, threat analysis and Showdown export.

Use for:

- Team Builder recommendation UX;
- threat/coverage summaries;
- provider abstraction for competitive statistics;
- Showdown export ideas;
- "build from Pokémon I actually own" enhancement.

Do not rely on its old usage datasets as current competitive truth.

## gerrie333/VsRecorderTool

**Classification:** `REFERENCE` (no clear repository license detected at review time).

Reads Gen IV/V battle-video data from saves/extracted video files, with planned metadata/import/text-readout support.

Use for:

- optional future Battle History/archive feature;
- Gen IV/V save-section research;
- trainer/battle metadata extraction;
- provenance/archive extras.

This is not a v1 requirement.

## suloku/G6_pl_tool

**Classification:** `HISTORICAL` / `REFERENCE` (no clear repository license detected at review time).

Generation VI Pokémon Link dump/injection tooling for X/Y/ORAS-era Pokémon Link data.

Use for:

- Gen VI gift/event research;
- Pokémon Link payload/offset archaeology;
- EventDex regression cases;
- validating old distribution-state behavior.

Prefer current PKHeX/event research when available.

## vinnymac/PokeNurse

**Classification:** `HISTORICAL` / low-priority `REFERENCE`.

Old Pokémon GO desktop manager for sorting, transfer, evolution, favorites, power-up and templated renaming.

Useful concepts only:

- attribute-based sorting;
- evolution management UX;
- templated nickname/rename rules such as inserting IV/stat values into names.

Do not reuse its obsolete Pokémon GO API/authentication workflow. No clear repository license was detected at review time.

---

# Tier C — historical validation / archaeology

Keep these links available so future work can compare old independent implementations, but do not make them primary dependencies.

## kwsch/KeySAV / KeySAV2 / MassDumper

Historical X/Y/ORAS box and battle-video decryption/dumping tools.

Use for independent Gen VI crypto/box-layout regression and old edge cases only. Their Powersaves/xorpad workflows are obsolete for normal PokeBank usage.

## kwsch/PKHeXRNG

Modern-console/emulator memory/RPC PKHeX plugin.

Use as a live-memory/RNG/debugging research reference. Repository had no detected license at prior review, so no source reuse without clarification.

## kwsch/EggMoveEdit

BW/B2W2 Egg Move NARC editor.

Use for Gen V egg-move data archaeology and Verified Breeding rules cross-checks.

## kwsch/pkm2pkx

Old Gen V -> Gen VI converter whose own README warns that it is extremely outdated and not fully correct.

Use only for historical regression cases.

## kwsch/PKXBook

Simple `.pk6` byte-viewing utility. Low value now; useful only for format archaeology.

## kwsch/SVHelper

Early shiny-value helper explicitly superseded by later KeySAV/MassDumper tools. Keep only as shiny/PID/TID/SID math archaeology.

## kwsch/prc_bw2

Black 2/White 2 ROM changer. Useful only for occasional Gen V ROM-data research; not a save/Vault component.

## kwsch/NewSnap

New Pokémon Snap archive/save-data library.

Only revisit if PokeBank deliberately expands into New Pokémon Snap or spin-off archival support.

## kwsch/oras_dsf

Repository appeared effectively empty at prior review. Ignore unless meaningful content appears later.

---

# Tier D — low relevance / do not spend core time on these

## turtleisaac/Repair-Tool

Despite the name, this repairs **Gen IV ROM corruption caused by PPRE**, not normal Pokémon save corruption.

Do not use it as the foundation for PokeBank Save Rescue Mode. It may be useful only if PokeBank ever enters ROM-modding repair territory, which is not current scope.

## Jisagi/PokeTool

3DS randomizer workflow helper for X/Y, ORAS, Sun/Moon and USUM.

Mostly outside PokeBank's save/Vault mission. May contain occasional ROM-data workflow ideas, but current PKHeX/pk3DS/PokeBank adapters are more relevant.

## svfeplvce/ProjectSky

Scarlet/Violet **ROM/modding** editor for base stats, evolutions, learnsets and trainer teams; its README explicitly points users to PKHeX for save editing.

Reference only if PokeBank needs a specific SV game-data table. Do not make it a core dependency.

## JappaWakka/P3DToolsAndContentPacks

Large Pokémon 3D fan-game content/tool repository. Not materially related to PokeBank save/Vault architecture, and no clear repository license was detected at review time.

Do not import assets/content casually.

## tony-rowan/pokemon-name-generator

MIT tool for generating new fictional Pokémon names.

Not useful to core PokeBank behavior. At most an optional novelty/nickname inspiration feature someday; do not spend roadmap time on it.

---

# Additional upstream/reference projects worth pinning from this research

The user-supplied repositories point to several upstream data/research sources that are more valuable than some wrapper tools themselves:

```text
GearsProgress/Pokemon-Community-Conversion-Standard
projectpokemon/EventsGallery
kwsch/PKHeX
kwsch/pkNX
foohyfooh/PKHeXPluginPile
PokemonClassic / Gen-IV community-server research
pret/pokered
pret/pokeyellow
pret/pokegold
pret/pokecrystal
pret/pokeruby
pret/pokefirered
pret/pokeemerald
```

These should be evaluated/pinned when the corresponding generation/feature becomes active.

---

# Feature-to-reference lookup

Before implementing a feature from scratch, check this table.

| PokeBank feature | Research first |
|---|---|
| Create / Quick Legal / Expert Editor | PKHeX, Auto Legality/PKHeX plugins, CoreConsole |
| Gen I/II -> III Transfer Lab | Poke_Transporter_GB, PCCS, PKHeX, pk2pk |
| General conversion/loss preview | PKHeX, pk2pk, Poke_Transporter_GB |
| Switch Event Library / Mystery Gifts | Switch-Gift-Data-Manager, EventsGallery, PKHeX, pkNX |
| Gen VI gifts / Pokémon Link | G6_pl_tool, PKHeX/event research |
| Gen VI save health/checksums | PKHeX, XYSAV, KeySAV family |
| Gen IV GTS / post-shutdown preservation | IR-GTS, PokémonClassic/community-server docs |
| SWSH local LAN research | PokePiaSWSH |
| Modern Switch RE/debugging | poke-client, PKHeXRNG, pkNX/PKHeX research |
| Living/Shiny Dex UX | DexTool, PokeTrainerTools |
| Ribbon Master | PokeTrainerTools + game/PKHeX data |
| Shiny Hunt manager | PokeTrainerTools |
| EV training helper | pokelog |
| Team Builder | Pokemon-Team-Builder + current competitive providers |
| Bulk sorting / special search | PKHeXPluginPile |
| Plugin/provider architecture | switch-poke-pilot + PokeBank's own adapter model |
| Gen IV/V Battle archive | VsRecorderTool |
| HOME migration/preservation research | HomeRip |
| BDSP game tables | UnityDPtools + current PKHeX/pkNX |

---

# Engineering rule for future Codex sessions

When a roadmap item becomes active:

1. Search this matrix for existing references.
2. Check whether a newer upstream implementation now exists.
3. Record exact external commit/revision used for research or tests.
4. Record the external project's license before copying any code/data.
5. Prefer independent tests/normalized vectors over direct coupling.
6. Put integrations behind PokeBank interfaces.
7. Keep safety, ownership, provenance, Vault transactions and UI behavior PokeBank-owned.
8. Never let research/reference work silently expand the currently authoritative milestone.

This matrix is intentionally a living document. New useful Pokémon reverse-engineering/save/event/network projects should be added here when discovered rather than disappearing into chat history.
