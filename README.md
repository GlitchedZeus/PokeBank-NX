<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, transfer, and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the goal is broader: one controller-first Switch application for browsing supported Pokémon saves, keeping a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and safely moving compatible Pokémon between games and generations.

> **Alpha warning:** live installed-game save writing is not an approved current feature. Installed game sources stay read-only until an individual adapter passes explicit backup, staged-write, validation, readback, rollback, and physical-device safety gates.

---

# Current status — September 3, 2026

Development branch:

```text
feature/pokebank-playable
```

Writable repository:

```text
GlitchedZeus/PokeBank-NX
```

PKSE remains **upstream only**. PokeBank NX changes must never be pushed to the original PKSE repository.

## First physical Switch milestone — COMPLETE

The first exact recorded PokeBank NX `.nro` has been physically run and stress-tested on Nintendo Switch hardware.

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Result:
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Current first-artifact hardware summary:

| Area | Result |
|---|---|
| Boot / relaunch | **PASS** |
| D-pad navigation + held repeat | **PASS** |
| Left Stick | **FAIL — no input at all** |
| A → Pokémon Action Sheet | **PASS** |
| Heavy Action Sheet repetition | **PASS** |
| B / Cancel | **PASS** |
| `+` Options / More | **PASS** |
| `-` Help / Controls | **PASS** |
| OLED Black | **PASS** |
| Dark | **PASS** |
| Light | **PASS** |
| Theme persistence | **PASS** |
| HOME → return | **PASS** |
| Sleep → wake | **PASS** |
| Controller reconnect | **PASS** |
| Handheld | **PASS** |
| Docked | **NOT TESTED** |
| Party / Boxes / Storage | **PASS WITH CAVEATS** |
| Sampled View data | **PASS / PARTIAL** |
| Pokémon visual in View | **MISSING IN THIS ARTIFACT** |
| One old Legends: Arceus save | **REPRODUCIBLE CRASH** |
| Visible PokeBank NX identity | **FAIL / INCOMPLETE ON THIS OLD BUILD** |

Reports:

- [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md)
- [`docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md)
- [`docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md`](docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md)

Issue **#8 is complete** because the first exact physical-test milestone happened. A device test can still be a partial pass.

---

# Important Sep 3 safety / Storage clarification

Extended testing physically reached inherited PKSE controls such as:

```text
Release
Create Pokémon
Menu / Move / Multi
editable Pokémon view
apply/save-style backup changes
```

The user also tested inherited app Storage with an Arbok. The exact observed sequence was:

```text
installed Pokémon Legends: Z-A source
        ↓ automatic backup
open Z-A BACKUP representation
        ↓
move Arbok from backup into inherited Storage
        ↓
return to PokeBank NX main menu
        ↓
open another supported game's backup/session
        ↓
open Storage
        ↓
Arbok is still present
```

The tester reports the **original installed Z-A save remained unchanged** during the exercised flow.

So the physically demonstrated transfer shape is:

```text
INSTALLED GAME — read / backup only
        ↓
MUTABLE BACKUP REPRESENTATION
        ↓
APP-OWNED LEGACY STORAGE (`PKSEBANK` / `bank.dat`)
        ↓
OTHER GAME'S LOADED BACKUP REPRESENTATION
```

Current evidence:

```text
LIVE INSTALLED SAVE WRITE OBSERVED:      NO
ORIGINAL INSTALLED SAVE CHANGED:         NO — tester-reported in exercised flow
USER-REACHABLE MUTATION UI:              YES
BACKUP/IN-MEMORY MUTATION:               YES
APP STORAGE PERSISTENCE:                 YES
CROSS-GAME STORAGE VISIBILITY:           YES
MASTER VAULT IMPLEMENTED:                NO
TRUE MOVE IMPLEMENTED:                   NO
```

The Arbok event is best described as a **persistent copy/import into app-owned legacy Storage from a mutable backup representation**. It is not a destructive live-save move and not the future Master Vault.

This is actually a useful proof-of-concept for future Game ↔ Vault transfer UX, but the inherited Storage architecture will not automatically become the authoritative Vault.

Tracked by **#23** and **#27**. Future true Move is **#20**.

---

# Current application checkpoint

Session 2.5 published:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that source.

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis
single-tap + held analog navigation
Select Game / Backups / Party / Boxes / Storage analog integration
visible PokeBank NX header/app identity
PokeBank NX chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

This source is **NOT DEVICE TESTED**.

The next coding session must preserve that work while fixing the safety/UI and PLA blockers. If application code changes, the replacement `.nro` must be tied to a **new exact application-source SHA**.

Current HIGH prompt:

[`docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md)

---

# Current blockers before device test #2

## #23 — installed-source safety / UI contract

A source-level audit already found that inherited Release/Create/Move/Edit operate on the loaded in-memory/backup representation, normal Save Changes targets backup destinations, legacy Storage is app-owned, and the low-level live-title restore path is hard-blocked before mounting the installed save while live writes are disabled.

The remaining problem is **user-facing ambiguity**.

The replacement build must make these states clear:

```text
INSTALLED SOURCE — READ ONLY
BACKUP / STAGED COPY — MUTABLE ONLY WHEN EXPLICIT
LEGACY STORAGE — APP-OWNED COMPATIBILITY STORAGE
MASTER VAULT — FUTURE AUTHORITATIVE STORAGE
LIVE GAME WRITE — LATER / PER-ADAPTER SAFETY GATE
```

Unsafe installed-source Release/Create/Move/Edit shortcuts must not remain exposed simply because a lower-level write guard would eventually stop the final live write.

Static audit: [`docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`](docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md).

## #24 — old / malformed Legends Arceus crash

One older PLA save reproducibly crashes the first artifact.

Valid supported saves should open. Old, malformed, unsupported, truncated, or unexpected saves must return a useful read-only error and safe navigation instead of crashing or being silently repaired/written.

## #19 — Left Stick navigation

The first hardware build receives **no Left Stick navigation input at all**. `361c6f55...` contains the reported real-libnx replacement path, but physical single-tap, held-repeat and diagonal testing is still required.

## #13 / #16 — visible PokeBank NX identity

The replacement build must preserve the PokeBank NX shell/chrome/Options/Help/Action Sheet styling and remove obvious normal-path PKSE product identity.

## #37 — required device visual assets

The inherited source already contains a Pokémon renderer and an offline RomFS sprite pipeline. The first physical artifact showed no Pokémon visual, likely because the generated gitignored sprite assets were not packaged.

Before the replacement device build, the asset generator/preflight must run.

Docs:

- [`docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md`](docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md)
- [`docs/DEVICE_BUILD_ASSET_GATE.md`](docs/DEVICE_BUILD_ASSET_GATE.md)

---

# Product model

## Installed game saves

Installed game saves are currently **read-only sources**.

PokeBank NX may create/read backup copies while parsers, Vault, conversion engines, staged saves and per-game write adapters are proven.

## Backup / staged copies

Backup representations may eventually be explicitly editable because they are not the live installed title. The UI must make that state obvious so users never confuse backup edits with live game edits.

## Legacy/app Storage

The inherited Storage area is already writable/persistent and can remain visible across changing game contexts.

It is **not automatically the future Master Vault**.

Issue **#27** tracks whether it becomes a migration source, compatibility area, or is retired after Vault v1.

## Master Vault

The planned Master Vault is the permanent game-independent storage/provenance layer:

```text
immutable raw Pokémon payloads
stable Vault IDs
SHA-256 integrity
origin/source provenance
active/current location separate from origin
parent/derived lineage
transaction journal + crash recovery
named Banks as organization/reference layers
rebuildable search/metadata indexes
```

Specification: [`docs/MASTER_VAULT_SPEC.md`](docs/MASTER_VAULT_SPEC.md).

## Banks

Banks organize Vault entities without blindly duplicating raw payloads.

Examples:

```text
Living Dex
Shiny Living Dex
Favorites
Events
Competitive
Gen III
Scarlet / Violet
```

---

# Copy, Move, and Clone

Long-term semantics are explicit:

```text
COPY
= intentionally keep the source active and create a destination representation

MOVE
= relocate the active Pokémon; source stops being active only after destination success is verified

CLONE
= deliberately create a duplicate with clone provenance
```

Desired end-state:

```text
Game A  →  Master Vault / Bank  →  Game B
```

A real Move should feel like Pokémon Bank/HOME relocation. The source stops being active only after destination creation, validation, write/readback verification and rollback safety succeed.

The old backup→legacy Storage→other-backup flow is **not** true Move, but it confirms that an offline cross-game transfer workspace is viable on-device.

True Move is **later, not now**. Issue **#20** blocks it on independently approved per-game write adapters.

Specification: [`docs/TRANSFER_MODEL.md`](docs/TRANSFER_MODEL.md).

---

# Controller-first UI

## Pokémon Action Sheet — COMPLETE foundation

Press **A** on an occupied Pokémon:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

Opening/navigating/canceling the sheet remained stable during heavy physical repetition.

Installed-source `Edit` behavior is covered by #23 because the inherited writable view is still reachable.

## Intended controller contract

| Control | Intended behavior |
|---|---|
| D-pad | precise navigation |
| Left Stick | navigation + held repeat |
| A | Select/Open; Pokémon → Action Sheet |
| B | Back/Cancel |
| X | Filter/Search/context |
| Y | Sort/View/secondary context |
| L/R | previous/next box, Pokémon, or nearby tab |
| ZL/ZR | larger jumps / major navigation |
| `+` | contextual More/Options |
| `-` | Help/Controls/Screen Info |
| Right Stick | optional future Summary model rotation / secondary behavior |

Extended hardware testing exposed inherited differences such as L/R account switching, inactive ZL/ZR in one context, X Dex-sort and Y Menu/Move/Multi. Issue **#26** tracks normalization.

Full contract: [`docs/CONTROLS.md`](docs/CONTROLS.md).

## Theme feedback

All three themes and persistence passed physical testing.

```text
OLED Black
Dark
Light
```

Additional Sep 3 feedback:

- bottom controller/button hints need better contrast in OLED Black and Dark;
- Dark could be slightly darker;
- Light is liked substantially as-is;
- OLED Black currently gives the preferred Action Sheet appearance.

Visual direction: [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md).

---

# Summary / Pokémon visuals / cries

Sampled View data for Bulbasaur, Alolan Meowth, Mewtwo and Mew reported correct level/gender/shiny information and safe B-return behavior.

The first artifact did not show the expected Pokémon image.

Future Summary/View should explicitly show both:

```text
Species: Bulbasaur
Type: Grass / Poison
```

rather than ambiguous labels.

Planned presentation layers:

```text
Box / Vault grid      → compact modern Pokémon icon
View / Summary        → large HOME-style render
Ball / held item      → compact icon
Ribbon / Mark         → compact icon
Cry                   → optional play-on-open + replay
Optional later 3D     → only if practical on Switch
```

Relevant work:

- **#9** professional Summary + provenance
- **#25** Pokémon visual/render support
- **#35** Pokémon cry support
- **#7** Vault-driven Pokédex / Living Dex

Visual research:

- [`docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md`](docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md)
- [`docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md`](docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md)

PokeBank NX should stay offline at runtime; online datasets are build/reference inputs, not required runtime services.

---

# Target game identities

PokeBank NX currently has **23 host-tested identities in source**. That is the implementation checkpoint, not the complete product target.

The intended catalog is:

```text
23 current host-tested identities
+ 9 Nintendo DS identities
+ 8 Nintendo 3DS identities
+ 2 Nintendo GameCube RPG identities
+ 2 Nintendo 64 Stadium identities
= 44 total release/source targets
```

The DS/3DS/GameCube/N64 IDs are **planned, not yet identity-tested**. Stadium is a v1 stretch target and must not block the core release if it becomes disproportionately expensive.

### Game Boy / Game Boy Color

- Red
- Blue
- Yellow
- Gold
- Silver
- Crystal

### Game Boy Advance

- Ruby
- Sapphire
- Emerald
- FireRed
- LeafGreen

### Nintendo 64 — planned legacy/stretch

- Pokémon Stadium
- Pokémon Stadium 2

PKHeX has dedicated Stadium save handlers with real box/storage/checksum behavior, so read-only import is technically realistic. Forward conversion into later formats must preserve the historical transfer discontinuity.

Tracked by **#34**.

### Nintendo GameCube — planned

- Pokémon Colosseum
- Pokémon XD: Gale of Darkness

These are strong targets because they are Gen III sources and historically interacted with compatible GBA games.

Tracked by **#33**.

### Nintendo DS — planned

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver
- Black
- White
- Black 2
- White 2

Stable identities/source discovery: **#30**. Read-only Gen IV/V adapters: **#31**.

### Nintendo 3DS — planned

- X
- Y
- Omega Ruby
- Alpha Sapphire
- Sun
- Moon
- Ultra Sun
- Ultra Moon

Stable identities/source discovery: **#30**. Read-only Gen VI/VII adapters: **#32**.

3DS Virtual Console R/B/Y/G/S/C should initially be treated as source/platform variants of their original GB/GBC identities unless technical evidence requires distinct IDs.

### Nintendo Switch

- FireRed
- LeafGreen
- Let's Go, Pikachu!
- Let's Go, Eevee!
- Sword
- Shield
- Brilliant Diamond
- Shining Pearl
- Legends: Arceus
- Scarlet
- Violet
- Legends: Z-A

GBA FireRed/LeafGreen and Switch FireRed/LeafGreen are intentionally separate identities.

Detection, source import, parsing, conversion, staging, live writing and physical validation are separate capabilities.

See [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md).

---

# Full roadmap to v1.0

Canonical detailed roadmap: [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)  
Master release tracker: **#29 — Track PokeBank NX v1.0 roadmap and release gates**.

The checklist below is intentionally visible in the README so the repository homepage answers both **what have we done?** and **what is left?**

## Foundation / playable shell

- [x] Repository recovery + Git/upstream safety discipline
- [x] Stable 23-game current identity registry
- [x] Low-level live installed-save write hard lock
- [x] Controller-first Pokémon Action Sheet (#2)
- [x] HOME-style controller/theme foundation
- [x] First exact physical Switch `.nro` test (#8)
- [x] Extended first-build torture test/report
- [x] Sep 3 follow-up: sleep/resume/reconnect/handheld/Summary/Storage safety evidence
- [x] Initial static mutation-persistence audit
- [x] Device visual-asset preflight tooling
- [x] Exact-artifact packaging helper
- [ ] **IN PROGRESS** — visible PokeBank NX shell physical acceptance (#13/#16)
- [ ] **IN PROGRESS** — full Left Stick physical navigation (#19)
- [ ] **IN PROGRESS** — installed-source mutation UI safety contract (#23)
- [ ] **IN PROGRESS** — old/malformed PLA crash hardening (#24)
- [ ] **IN PROGRESS** — required visual assets present in next device build (#37)
- [ ] Exact second `.nro` packaged, hashed, preserved and physically tested
- [ ] Final controller semantics/hints normalized (#26)

## Target catalog expansion

- [ ] Add 9 Nintendo DS stable identities (#30)
- [ ] Add 8 Nintendo 3DS stable identities (#30)
- [ ] Add Pokémon Colosseum + XD GameCube identities (#33)
- [ ] Add Pokémon Stadium + Stadium 2 N64 identities (#34)
- [ ] Host-test uniqueness/stability of the expanded registry
- [ ] Keep all existing 23 stable IDs unchanged

## Historical / legacy Pokémon-save engine

- [ ] PKSM-Core Gen III `PK3` / `Sav3` spike and integration decision (#4)
- [ ] FireRed / LeafGreen GBA production read adapter
- [ ] Ruby / Sapphire / Emerald read adapters
- [ ] Colosseum / XD read-only GameCube adapter (#33)
- [ ] Gen I Red/Blue/Yellow read adapters
- [ ] Gen II Gold/Silver/Crystal read adapters
- [ ] Pokémon Stadium 1/2 read-only container adapters (#34, stretch)
- [ ] RetroArch/legacy save discovery + manual fallback (#6)
- [ ] Nintendo DS Gen IV/V read adapters (#31)
- [ ] Nintendo 3DS Gen VI/VII read adapters (#32)
- [ ] malformed/truncated/checksum/version rejection across legacy adapters

## Validation / regression tooling

- [ ] Golden Pokémon/save fixture corpus (#17)
- [ ] malformed/corrupt fixture variants
- [ ] PKHeX host Oracle: inspect / legality / convert (#5)
- [ ] versioned machine-readable cross-engine comparisons
- [ ] untouched-save round-trip expectations where the format permits them
- [ ] GameCube memory-card / `.gci` fixture coverage
- [ ] Stadium storage/checksum fixture coverage if #34 enters v1

## Master Vault / Banks

- [ ] Master Vault v1 immutable entities (#3)
- [ ] stable Vault IDs + SHA-256
- [ ] origin + active-location + history separation
- [ ] parent/derived provenance
- [ ] transaction journal + crash recovery
- [ ] rebuildable metadata/search indexes
- [ ] named Banks over Vault IDs (#3)
- [ ] legacy writable Storage migration/import decision (#27)
- [ ] optional PKSMBANK importer if useful

## Collection / Summary / quality-of-life

- [ ] professional Summary + provenance (#9)
- [ ] Pokémon sprite/artwork/render support (#25)
- [ ] Pokémon cries + replay/mute controls (#35)
- [ ] Pokédex-style description / encounter-location presentation where data is available
- [ ] search/filter
- [ ] Box Quick Jump
- [ ] Favorites
- [ ] Recently Viewed / Recently Added / Continue
- [ ] Vault-driven Pokédex (#7)
- [ ] Living Dex
- [ ] Shiny Living Dex
- [ ] meaningful forms/gender distinctions
- [ ] Alpha/Shiny Alpha where applicable

## Modern Switch read support

Validate individually before any writes (#11):

- [ ] Switch FireRed / LeafGreen
- [ ] Let's Go Pikachu / Eevee
- [ ] Sword / Shield
- [ ] Brilliant Diamond / Shining Pearl
- [ ] Legends: Arceus
- [ ] Scarlet / Violet
- [ ] Legends: Z-A
- [ ] unsupported/malformed/version-mismatch saves fail gracefully

## Conversion / transfer engine

- [ ] compatibility result: `SUPPORTED / SUPPORTED_WITH_CHANGES / UNSUPPORTED / UNKNOWN`
- [ ] historical → later Pokémon conversion paths
- [ ] GameCube Gen III source conversion/provenance
- [ ] Stadium/Gen I-II forward conversion with explicit historical-break provenance
- [ ] parent/derived provenance on conversion
- [ ] PKHeX comparison where available
- [ ] `.pk*` export
- [ ] staged destination-save representation without live write (#10)
- [ ] dedicated Game ↔ Vault/Bank Transfer Workspace
- [ ] one-Pokémon selection
- [ ] multi-select
- [ ] whole-box operations where safe/supported

## Legality / editing / generation / events

- [ ] legality-aware Vault editing
- [ ] `Make Shiny` obeys shiny locks/rules
- [ ] conservative `UNKNOWN != LEGAL`
- [ ] encounter-driven legal-generation path where supported
- [ ] generated/edited entities retain provenance
- [ ] Mystery Gift / Wondercard workflows
- [ ] historical event database/provenance plan
- [ ] advanced cross-generation compatibility/change preview

## Staged writes / approved live writes / real Move

- [ ] generic backup + fingerprint + staged-save transaction architecture
- [ ] checksum/container repair + reparse validation
- [ ] exact unintended-change comparison
- [ ] durable rollback/recovery state
- [ ] first **individually approved** live-write adapter
- [ ] additional adapters approved one-by-one
- [ ] real `Game → Vault/Bank MOVE` (#20)
- [ ] real `Vault/Bank → Game MOVE` (#20)
- [ ] Copy remains separate
- [ ] Clone remains separate
- [ ] one/multi/whole-box Move uses the same transaction safety model

## Official HOME bridge workflow

- [ ] supported game-save bridge workflow documented/tested where applicable
- [ ] preserve genuine HOME tracker/history
- [ ] no private HOME/Nintendo protocol impersonation
- [ ] no credential extraction or tracker forging
- [ ] no guaranteed-ban-safety claims

PokeBank NX may support **offline source formats that official HOME does not directly import**. Any path into official HOME still goes through a supported modern game and preserves genuine tracker/history rules.

## Native `.nro` reliability / diagnostics / accessibility

Tracked by #21 / [`docs/NRO_QUALITY_ROADMAP.md`](docs/NRO_QUALITY_ROADMAP.md):

- [x] device-asset preflight helper
- [x] source-addressed artifact packaging helper
- [ ] durable CI/release artifact automation (#15)
- [ ] Diagnostics screen + privacy-safe diagnostic export
- [ ] Applet/constrained-memory detection
- [ ] privacy-safe crash/error logs
- [ ] clear READ ONLY / BACKUP / LEGACY STORAGE / VAULT / STAGED / active-location badges
- [ ] Vault storage-health / Verify Vault / rebuild index
- [ ] interrupted-transaction recovery UI
- [ ] virtualized huge Vault/Dex grids
- [ ] bounded artwork/sprite caches
- [ ] lazy raw-entity loading
- [ ] intentional missing-resource fallbacks
- [ ] text-size option
- [ ] Reduced Motion
- [ ] color-independent focus
- [ ] readable controller-hint contrast in dark themes
- [ ] optional original UI sounds
- [ ] optional restrained rumble

## Final product identity / scale / release

- [ ] final splash/startup stages tied to real work (#16)
- [ ] final title/icon/NACP metadata
- [ ] no normal-path PKSE product identity
- [ ] visible version + source SHA
- [ ] large synthetic Vault performance soak
- [ ] handheld + docked readability pass
- [ ] sleep/resume/controller-reconnect regression pass
- [ ] malformed-save corpus torture pass
- [ ] release-candidate transaction recovery torture test
- [ ] exact release `.nro`, source SHA, size and SHA-256 preserved
- [ ] advertised support matrix matches actual verified capability
- [ ] release notes distinguish read-only vs approved live-write adapters
- [ ] **v1.0 tag/release**

## Current critical path

```text
#23 safety/UI contract
#24 PLA crash hardening
+ preserve #19 analog fix
+ preserve #13/#16 PokeBank NX UI
+ #37 asset preflight
        ↓
freeze new exact application source if changed
        ↓
host tests + ASan/UBSan + native build
        ↓
replacement .nro + SHA-256
        ↓
PHYSICAL DEVICE TEST #2
        ↓
#4 PKSM-Core PK3/Sav3
        ↓
Gen III read adapter
        ↓
Master Vault + Banks
        ↓
GameCube / Gen I-II / DS / 3DS legacy reads
        ↓
Summary / Oracle / modern Switch reads
        ↓
conversion / Dex / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
release hardening
        ↓
v1.0
```

Stadium support is intentionally allowed to slip past v1 if it is disproportionately expensive; it must not hold the rest of the project hostage.

---

# Build / device-test helpers

Before a device-test build:

```bash
python tools/check_device_assets.py
```

After freezing the exact clean application source and building `PokeBankNX.nro`:

```bash
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

See:

- [`docs/DEVICE_BUILD_ASSET_GATE.md`](docs/DEVICE_BUILD_ASSET_GATE.md)
- [`docs/DEVICE_ARTIFACT_PACKAGING.md`](docs/DEVICE_ARTIFACT_PACKAGING.md)
- [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md)

Packaging never means device-tested. Only a human physical run of that exact recorded hash can add device evidence.

---

# Safety model

**LIVE INSTALLED-GAME SAVE WRITING IS NOT AN APPROVED CURRENT FEATURE.**

Known lower-level protections include:

- safe/backup destination posture;
- generic save API cannot request title injection in the normal path;
- low-level restore rejects live-title writes before mounting save data;
- legacy `injectToGame=1` is disabled/rewritten.

The Sep 3 Arbok test provides physical supporting evidence that backup/app-Storage operations can persist while the installed source stays unchanged.

That does **not** mean user-facing Release/Create/Move/Edit controls are acceptable on an installed source. #23 keeps UI/source-state safety separate from the low-level hard lock.

Future live writes are **per adapter**, not one global switch.

Required shape:

```text
read
backup + fingerprint
stage cloned save
apply intended mutation
repair checksums/containers
reparse + validate
write
read back
verify exact intended change
retain rollback/recovery state
```

If verification fails, abort/rollback rather than guessing.

See [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md).

---

# Upstream / reference stack

PokeBank NX checks mature Pokémon homebrew/research projects before rebuilding difficult infrastructure from scratch.

| Project | Role |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation; **upstream only** |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | Native C++ historical Pokémon/save engine candidate |
| [PKSM](https://github.com/FlagBrew/PKSM) | Mature Bank, backup, migration, event and PKSM-Core integration reference |
| [Pokémon Chest](https://github.com/Universal-Team/pkmn-chest) | Gen III–V Bank and second Nintendo-platform PKSM-Core integration reference |
| [PHBank](https://github.com/gocario/PHBank) | Historical Game-PC ↔ offline-Bank transfer UX reference |
| [PHBankGBC](https://github.com/0xb01u/PHBankGBC) | Secondary Gen I/II save-layout reference; verify independently |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation, including Colosseum/XD/GameCube memory cards and Stadium handlers |
| [PKHeX-Plugins / Auto Legality](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save-behavior reference |
| [pkDex](https://github.com/Insektaure/pkDex) | Pokédex UX/data-organization reference |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/transaction architecture reference |
| [PokeAPI/sprites](https://github.com/PokeAPI/sprites) | Pokémon media organization/reference; inherited PKSE sprite generator source |
| [PokéSprite](https://github.com/msikma/pokesprite) | Compact box/item/form sprite metadata reference |
| [PokeAPI/cries](https://github.com/PokeAPI/cries) | Cry file/ID organization reference for future #35 |

Pinned revisions/licenses/classifications: [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md).  
Additional audits:

- [`docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`](docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md)
- [`docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md`](docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md)
- [`docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md`](docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md)

PKSM/Chest/PHBank do **not** replace the Master Vault design. They give us proven ideas around parsing, backups, migrations, Game↔Bank interaction and PKSM-Core integration.

External Pokémon media remains subject to its own copyright/redistribution terms even when the surrounding repository code is permissively licensed.

---

# Development start point

Read in this order:

1. [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
2. [`docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md`](docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md)
3. [`docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`](docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md)
4. [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)
5. [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
6. [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md)
7. [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md)
8. [`docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md)
9. [`docs/SESSION_RUNBOOK.md`](docs/SESSION_RUNBOOK.md)
10. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
11. [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md)
12. current issue-specific spec/prompt

---

## License / attribution

PokeBank NX is distributed under the repository's **GNU Affero General Public License v3**. See [`LICENSE`](LICENSE).

The project began from PKSE and preserves required upstream/license attribution. External projects retain their own licenses/notices; direct reuse decisions must be reviewed and recorded rather than assumed from a GitHub link.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.

Unofficial fan-made homebrew project • © 2026 PokeBank NX Contributors