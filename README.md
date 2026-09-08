<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The long-term goal is a controller-first, offline Pokémon HOME-style application that can safely browse supported game saves, collect Pokémon into permanent local profile-scoped Vaults, organize them into Banks, preserve origin/provenance, and eventually move compatible Pokémon between supported games without relying on Nintendo's private online services.

> **Alpha / development warning:** live installed-game save writing is **not** an approved current feature. Installed-game sources remain read-only until each individual write adapter passes explicit backup, staging, validation, readback, rollback, and physical-device safety gates.

---

## Current development status

Active development branch:

```text
feature/pokebank-playable
```

Current engineering focus:

```text
RetroArch FireRed / LeafGreen GBA read-only runtime integration
```

The UI has been accepted as **good enough for development** and is intentionally frozen until the app is much closer to completion. Current work is focused on Pokémon/save functionality rather than more visual redesign.

For the exact engineering state, see:

- [`CURRENT_STATUS.md`](CURRENT_STATUS.md)
- [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
- [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md)
- [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)

---

# ✅ What already works

These are the parts a normal homebrew user would actually notice or care about today.

### App / controls

- ✅ Native Nintendo Switch `.nro` boots and runs on real hardware.
- ✅ Controller-first navigation.
- ✅ D-pad navigation.
- ✅ Left Stick navigation, including the physical-input fix.
- ✅ Pokémon Action Sheet for deliberate actions instead of accidental instant mutation.
- ✅ Handheld testing completed on current development builds.
- ✅ HOME / sleep / wake / controller reconnect behavior has been exercised successfully.

### UI / presentation

- ✅ PokeBank NX branding and shell are visible in-app.
- ✅ Red PokeBank NX identity accepted for development.
- ✅ OLED Black, Dark, and Light themes.
- ✅ Theme persistence.
- ✅ HD Pokémon artwork packaged in the application.
- ✅ Pokémon artwork renders on real Switch hardware.
- ✅ Artificial inherited sprite "breathing" / bobbing effect removed.
- ✅ Permanent inherited left-side accent bar removed.
- ✅ Current UI is frozen so engineering effort can go into the actual Pokémon features.

### Save safety

- ✅ Installed Switch game sources are treated as read-only.
- ✅ Low-level live installed-save write path is hard-disabled.
- ✅ Inherited Release/Create/Move/Edit paths are blocked from directly mutating installed sources.
- ✅ Backup/staged representations remain separated from installed-source browsing.
- ✅ App-owned legacy Storage is identified separately from the future profile-scoped Vault system.
- ✅ Malformed / old Legends: Arceus input is handled with a graceful error instead of crashing the app.
- ✅ Source immutability is covered by regression tests for the new Gen III read path.

### Pokémon / save engine

- ✅ Stable current registry for 23 release/platform identities.
- ✅ FireRed GBA and LeafGreen GBA remain distinct from the separate Switch FireRed/LeafGreen identities.
- ✅ PKSM-Core pinned and integrated as a host-side Gen III correctness oracle.
- ✅ Strict FireRed/LeafGreen GBA Gen III save validation.
- ✅ Two rotating Gen III save slots handled.
- ✅ All 14 Gen III sectors validated and reassembled.
- ✅ Sector IDs, signatures, counters, checksums, and counter wraparound handled.
- ✅ Safe fallback to an older valid slot when a newer slot is corrupt.
- ✅ Party Pokémon enumeration.
- ✅ All 14 PC Boxes enumerated.
- ✅ PK3 records that cross a PC-sector boundary are reconstructed correctly.
- ✅ Species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname, and OT are extracted.
- ✅ Malformed / truncated / invalid Gen III input is rejected safely.
- ✅ Untouched boxed and party PK3 round trips are byte-identical in tests.
- ✅ Existing PKSE Gen III crypto independently agrees with the new adapter test data.
- ✅ Exception-free Switch-native Gen III backend builds under the normal `-fno-exceptions` toolchain.
- ✅ Host regression tests and ASan/UBSan coverage are in place.
- ✅ GitHub CI runs the host suite with recursive pinned PKSM-Core submodules.

---

# 🚧 In development right now

### RetroArch FireRed / LeafGreen GBA source support

The next user-facing milestone is to let PokeBank NX find and open real FireRed/LeafGreen saves from RetroArch on the Switch.

Target flow:

```text
RetroArch savefile_directory
        ↓
bounded read-only .sav / .srm discovery
        ↓
full FRLG structural validation
        ↓
FireRed GBA / LeafGreen GBA identity when reliable
        ↓
exception-free native Gen III backend
        ↓
Party / Boxes
        ↓
existing PokeBank browsing flow
```

Current design rules:

- read-only;
- no broad uncontrolled SD-card crawl;
- only known RetroArch save roots / configured save directory;
- bounded depth and candidate count;
- never trust filename alone;
- ambiguous FRLG sources remain unclassified rather than guessed;
- no save repair or writeback.

Tracked by issue **#6**.

---

# 🗺️ Planned / to be added

These are planned features, not claims that they already work. They are intentionally parked behind the current game-support/read-safety milestones unless a specific roadmap document promotes them into active scope.

### Profile-scoped Vaults and Banks

PokeBank NX is planned around **one independent Vault workspace per Nintendo Switch user/profile**, not one machine-wide shared Vault.

- ⬜ Each Nintendo profile gets its own permanent game-independent **Vault**.
- ⬜ Each profile gets its own Banks, favorites, collection views, history, settings, source assignments, and future backup state.
- ⬜ Immutable raw Pokémon payloads with SHA-256 integrity.
- ⬜ Origin / source / provenance history.
- ⬜ Stable Vault IDs and parent/derived lineage.
- ⬜ Named **Banks** built over Vault entries.
- ⬜ Living Dex / Shiny Living Dex / Favorites / Events / Competitive collections.
- ⬜ Search, filters, favorites, recent items, and Quick Jump.
- ⬜ Crash-safe Vault journal, recovery, and index rebuild tools.
- ⬜ Vault data is local to the selected PokeBank profile unless the user explicitly performs a Gift or Trade to another profile.

Conceptual storage layout:

```text
PokeBankNX/
├── shared/
│   ├── source_catalog/
│   ├── emulator_providers/
│   └── assets/
└── profiles/
    ├── <Nintendo-user-id-A>/
    │   ├── vault/
    │   ├── banks/
    │   ├── source_bindings/
    │   ├── backups/
    │   └── settings/
    └── <Nintendo-user-id-B>/
        ├── vault/
        ├── banks/
        ├── source_bindings/
        ├── backups/
        └── settings/
```

### Emulator / legacy source organization

Physical emulator saves live globally on the SD card, but PokeBank NX should separate **physical discovery** from **profile ownership/assignment**.

Planned model:

```text
shared physical source discovery
        ↓
identify game + provider + save instance
        ↓
assign/bind source to a Nintendo profile
        ↓
show only that profile's assigned sources in its workspace
```

Target UI hierarchy:

```text
Nintendo Profile: WILL
  ↓
Pokémon SoulSilver
  ├── Tico Saves
  │   └── Character A — Main Save
  ├── DraStic Saves
  │   └── Character B — Main Save
  ├── melonDS / Other Emulator Saves
  │   └── Character C — Main Save
  └── Manual Imports
```

Design rules:

- ⬜ Installed Nintendo Switch-title saves remain Nintendo-account scoped where required.
- ⬜ Emulator/file/legacy discovery remains app-global at the filesystem layer.
- ⬜ Discovered emulator saves can be assigned to a specific Nintendo profile, marked Shared, or left Unassigned.
- ⬜ A raw save identifies the Pokémon game; provider/path metadata identifies where it came from.
- ⬜ Multiple genuine saves of the same game remain separate save instances even when they use different emulators or contain the same trainer name.
- ⬜ Provider names should drive generated headers such as **RetroArch Saves**, **Tico Saves**, **DraStic Saves**, **Dekopon Saves**, etc.
- ⬜ Known emulator discovery should be implemented through reusable source-provider definitions rather than game-specific emulator parsers.
- ⬜ Future provider manifests should make it possible to teach PokeBank NX new save locations without rewriting Pokémon parsing logic.
- ⬜ If a future emulator cannot be identified confidently, use a safe **Other / Unknown Emulator Saves** label instead of guessing.
- ⬜ Manual save import remains the universal fallback for valid supported save formats.

Conceptual provider metadata:

```text
provider id      = drastic
provider name    = DraStic
platform         = Nintendo DS
known save roots = /switch/drastic/...
extensions       = .dsv / .sav
```

The Pokémon save parser remains independent of the emulator provider:

```text
DraStic ─┐
Tico ────┼─→ HeartGold/SoulSilver save adapter → common PokeBank model
melonDS ─┘
```

### Save ownership boundary

The long-term safety/ownership rule is:

```text
GAME SAVE → ITS ASSIGNED PROFILE'S VAULT
```

A profile should not directly push Pokémon from its game saves into another profile's Vault, and one profile should not directly write another profile's assigned saves.

Cross-profile movement instead uses:

```text
WILL SAVE
   ↓
WILL VAULT
   ↓
Gift / Trade
   ↓
NIECE VAULT
   ↓
NIECE SAVE
```

This keeps emulator/installed saves profile-local while still allowing family members and friends to exchange Pokémon deliberately.

### Vault-to-Vault Gift and Trade

Cross-profile exchange is planned as a first-class Vault feature.

**Gift**

```text
WILL Vault → Niece Vault
```

- ⬜ One-way transfer.
- ⬜ Recipient must explicitly accept the gift.
- ⬜ Sender's ownership is not retired until destination creation/validation succeeds.
- ⬜ Declined/expired/failed gifts leave the sender's Pokémon intact.

**Trade**

```text
WILL Vault ↔ Niece Vault
```

- ⬜ A trade requires **both sides to offer a Pokémon**; it is not a disguised free transfer.
- ⬜ Both users review the exact offered/received Pokémon before final confirmation.
- ⬜ Neither side loses ownership until both destination records are safely created and verified.
- ⬜ Interrupted/failed trades roll back safely so one side cannot lose a Pokémon while the other side receives theirs.
- ⬜ Trade provenance records who traded with whom and which Pokémon was received in exchange.
- ⬜ Future multi-Pokémon trade transactions may support matched 2-for-2, 3-for-3, etc. exchanges.

`Gift` and `Trade` remain intentionally different actions so the app preserves an old-school Pokémon trade feel while still allowing deliberate one-way gifts.

### PokeBank Friends, IDs, and Mailbox

Future PokeBank NX networking should use **PokeBank-owned identities**, not Nintendo friend codes or private Nintendo/Pokémon network protocols.

Each PokeBank profile may have a shareable code such as:

```text
PB-7K4M-29QX
```

Planned flow:

```text
Friends
  ↓
Add Friend
  ↓
enter PokeBank ID
  ↓
recipient receives Friend Request in Mailbox
  ↓
Accept / Decline
```

The Mailbox is planned as the central asynchronous inbox:

```text
MAILBOX
├── Friend Requests
├── Trade Requests
├── Gifts
├── Completed Trades
└── System Messages
```

From a Pokémon in the Vault, the Action Sheet may eventually expose:

```text
Trade to Friend...
Gift to Friend...
```

Then the user selects an accepted PokeBank friend.

Privacy boundary:

- ⬜ Friends cannot browse another user's Vault by default.
- ⬜ A friend sees only information deliberately included in a friend request, gift, or trade offer.
- ⬜ Cross-profile game-save access remains blocked; network exchange is Vault-to-Vault only.

### Same-Switch, local-network, and internet trading

The same Vault trade engine should support multiple transports while presenting one consistent user experience:

```text
Same Switch
  → profile-to-profile local Vault transaction

Nearby / Local Wi-Fi
  → Switch-to-Switch local-network transaction

Internet Friend
  → PokeBank friend/mailbox relay
```

A local friend and a remote friend should look the same from the user's perspective; only the transport changes underneath.

### Lightweight PokeBank online relay

Internet trading should **not** turn PokeBank NX into a cloud-hosted Pokémon HOME replacement. Vaults, game saves, artwork, and normal backups stay on each Switch.

The optional online service is planned as a small post-office/coordination layer containing only what is needed for features such as:

```text
PokeBank public identity / public key
friend relationships
pending friend requests
pending gifts
pending trade requests
small encrypted exchange payloads
transaction state / acknowledgements
```

Conceptual architecture:

```text
WILL'S SWITCH                         RELAY                         NIECE'S SWITCH
Will's local Vault
     │
     ├── friend request ─────────→ mailbox DB ─────────────────→ Friend Request
     ├── encrypted gift ─────────→ pending message ─────────────→ Niece's local Vault
     └── trade offer ────────────→ transaction coordinator ←──── Niece's offer
```

Security goals:

- ⬜ Generate a local keypair for each PokeBank profile.
- ⬜ Keep private keys local to that profile/Switch.
- ⬜ Sign friend/trade/gift requests.
- ⬜ Encrypt Pokémon exchange payloads for the intended recipient where practical.
- ⬜ Use transaction IDs, expiry, replay protection, hashes, and explicit acknowledgements.
- ⬜ Never silently move/remove a Pokémon because a network message was merely sent.
- ⬜ Finalize ownership only after both sides/destinations have validated the transaction.

The backend should be deployable on inexpensive/serverless infrastructure and designed to fit within free hosting tiers for small usage where practical. Online services remain optional; local Vault use must not depend on a server being available.

### More game support

- ⬜ Ruby / Sapphire / Emerald GBA.
- ⬜ Red / Blue / Yellow GB.
- ⬜ Gold / Silver / Crystal GBC.
- ⬜ Nintendo DS Gen IV / V games.
- ⬜ Nintendo 3DS Gen VI / VII games.
- ⬜ Pokémon Colosseum.
- ⬜ Pokémon XD: Gale of Darkness.
- ⬜ Pokémon Stadium / Stadium 2 as stretch archival targets.
- ⬜ Validation and hardening of modern supported Switch save adapters.

Planned source catalog target:

```text
23 current host-tested identities
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 Nintendo GameCube
+ 2 Nintendo 64 Stadium stretch
= 44 total target identities
```

### Pokémon tools

- ⬜ Professional Pokémon Summary / provenance view.
- ⬜ Vault-driven Pokédex.
- ⬜ Living Dex and shiny collection views.
- ⬜ PKHeX host-side correctness / legality oracle.
- ⬜ Legality-aware editing.
- ⬜ Clone / Make Shiny safeguards with provenance.
- ⬜ Historical Mystery Gift / event workflows.
- ⬜ Pokémon cry playback in Summary.

### Transfer system

- ⬜ Explicit **COPY**, **MOVE**, and **CLONE** semantics.
- ⬜ Cross-generation conversion / compatibility engine.
- ⬜ Safe staged destination representations.
- ⬜ Backup → mutate stage → validate → write → readback → rollback transaction pipeline.
- ⬜ Individually approved game write adapters.
- ⬜ True Move only after destination success is verified.
- ⬜ Supported-game bridge workflow compatible with official Pokémon HOME without impersonating private Nintendo/HOME protocols or forging tracker/history data.

### Artwork packs / distribution flexibility

The current development build uses a full HD Pokémon artwork set. Longer-term, presentation should be decoupled from the core save/Vault engine so artwork can evolve independently.

- ⬜ Default built-in presentation that does not make core functionality depend on one external art source.
- ⬜ Optional artwork-pack/import system.
- ⬜ HD render pack support.
- ⬜ DS/pixel-style pack support.
- ⬜ Custom user-installed packs.
- ⬜ Pack manifests/versioning/coverage diagnostics.
- ⬜ Missing-art fallback that never breaks save browsing.

### Project funding

PokeBank NX is planned as a free/open-source project. If project funding is added, the preferred direction is transparent voluntary support rather than hidden advertising, fake traffic, or mandatory feature paywalls.

Potential future support links may include:

```text
GitHub Sponsors
Ko-fi / one-time donations
voluntary project funding
```

Funding would support development, testing hardware, hosting for optional online relay services, documentation, and compatibility work. Core local Vault functionality should remain usable without paid online services.

### Final polish / release quality

- ⬜ Final title/icon/NACP/startup polish.
- ⬜ Final controller semantics and hints.
- ⬜ Docked-mode polish and release-candidate hardware testing.
- ⬜ Accessibility options such as text sizing and Reduced Motion.
- ⬜ Privacy-safe diagnostics / crash reports.
- ⬜ Large-Vault performance / cache optimization.
- ⬜ Persistent reproducible device-test artifact automation.
- ⬜ Release candidate, exact binary/hash preservation, and v1.0 release.

---

# Current accepted physical UI milestone

Accepted application source:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
ui: adopt red PokeBank identity accents
```

Accepted artifact:

```text
PokeBank-NX-Red-UI-af2acf04.nro
SHA-256: 898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Physical result for continued development:

| Area | Result |
|---|---|
| D-pad | ✅ PASS |
| Left Stick | ✅ PASS |
| HD Pokémon artwork | ✅ PASS |
| Red PokeBank NX identity | ✅ ACCEPTED FOR NOW |
| Left accent bar | ✅ REMOVED |
| Fake sprite breathing/bobbing | ✅ REMOVED |
| Old/problem PLA source | ✅ GRACEFUL ERROR / NO CRASH |
| Installed-source live write | 🔒 HARD DISABLED |

---

# Gen III engineering checkpoints

Host PKSM-Core adapter:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

Pinned Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Native exception-free backend:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Deterministic FRLG regression fixture:

```text
Size:    131072 bytes
SHA-256: b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

More technical detail lives in [`CURRENT_STATUS.md`](CURRENT_STATUS.md) and [`docs/PKSM_CORE_INTEGRATION.md`](docs/PKSM_CORE_INTEGRATION.md).

---

# Safety principles

- Installed Switch save sources are read-only today.
- RetroArch / legacy save sources are read-only during parser/import milestones.
- No parser should auto-repair or overwrite malformed source data.
- Each future Vault is app-owned storage scoped to a PokeBank/Nintendo profile, separate from original game saves.
- Physical emulator discovery may be shared, but source ownership/bindings are profile-scoped.
- Game saves communicate only with their assigned profile's Vault; cross-profile Pokémon movement is Vault-to-Vault through deliberate Gift/Trade workflows.
- Live writes will be approved **one game adapter at a time**, never by one global unsafe switch.
- `COPY`, `MOVE`, and `CLONE` remain intentionally different operations.
- PokeBank NX does not impersonate private Nintendo/Pokémon HOME protocols or forge HOME tracker/history data.
- Future PokeBank friend/online trading uses independent PokeBank identities and transport, not Nintendo friend codes/private Nintendo services.

---

# Development / Codex continuation

PokeBank NX custom work belongs only in:

```text
GlitchedZeus/PokeBank-NX
```

`kiasta/PKSE` remains **upstream only**. Never push custom PokeBank NX code upstream.

Future coding sessions can use the short launcher:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
