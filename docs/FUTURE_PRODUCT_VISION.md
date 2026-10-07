# PokeBank NX — Future Product Vision

> **Future-only document.** Nothing in this file expands the current coding milestone. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active implementation work.

This document records long-term product ideas for PokeBank NX after the current read-only game/source milestones are stable.

---

# 1. Profile-scoped Vaults

PokeBank NX should not have one giant shared Vault for the entire console.

Each Nintendo Switch user/profile gets an independent PokeBank workspace:

```text
PokeBank NX
├── Nintendo Profile: WILL
│   ├── WILL's Vault
│   ├── WILL's Banks
│   ├── WILL's source assignments
│   ├── WILL's history/provenance
│   └── WILL's backups/settings
│
└── Nintendo Profile: NIECE
    ├── Niece's Vault
    ├── Niece's Banks
    ├── Niece's source assignments
    ├── Niece's history/provenance
    └── Niece's backups/settings
```

Installed Switch-title saves remain Nintendo-user scoped where required.

Emulator/legacy save discovery is physically app-global because the files live on the SD card, but each discovered save can be assigned to a PokeBank profile. A save assigned to WILL does not appear in NIECE's normal workspace unless deliberately marked Shared.

Do not infer ownership from trainer name alone. First-time assignment should be explicit, with an optional "automatically assign newly discovered emulator saves to the current profile" preference.

Shared sources, if supported, should default to read-only.

---

# 2. Source-provider grouping and future emulator discovery

Separate two concepts:

```text
WHAT GAME IS THIS?
Pokémon SoulSilver

WHERE DID IT COME FROM?
DraStic / Tico / melonDS / Dekopon / RetroArch / etc.
```

The Pokémon save adapter should parse the game data independently of the emulator that produced the file.

The source layer should identify the provider, physical path, source kind, metadata, and display name.

Target hierarchy:

```text
Pokémon SoulSilver
├── Tico Saves
│   └── ETHAN — Main Save
├── DraStic Saves
│   └── WILL — Main Save
├── melonDS Saves
│   └── SILVER — Main Save
└── Manual Imports
```

Provider metadata should be data-driven where practical so future emulators can be added without rewriting Pokémon parsers.

Conceptual provider registry:

```text
source-providers/
├── retroarch.json
├── drastic.json
├── tico.json
├── dekopon.json
├── melonds.json
└── future-emulator.json
```

A provider can define:

- display name;
- known save roots;
- extensions;
- supported console families;
- nearby application/config fingerprints;
- source-kind rules;
- bounded traversal rules.

If a valid Pokémon save is discovered but its emulator cannot be identified confidently, show it under `Other Emulator Saves` / `Unknown Provider` rather than guessing.

Manual Import remains the universal fallback.

---

# 3. Save/Vault ownership boundary

Game saves communicate only with the Vault belonging to their assigned PokeBank profile.

Core rule:

```text
SAVE -> OWNER'S VAULT -> OTHER PROFILE'S VAULT -> THEIR SAVE
```

Never directly:

```text
WILL'S SAVE -> NIECE'S SAVE
WILL'S SAVE -> NIECE'S VAULT
NIECE'S SAVE -> WILL'S SAVE
```

Cross-profile movement happens only Vault-to-Vault.

This keeps game-source ownership understandable and gives the Vault a single safe transaction boundary.

---

# 4. Gift and Trade are separate operations

## Gift

One-way ownership transfer:

```text
WILL's Vault -> Niece's Vault
```

Recipient must explicitly accept the gift. The sender's record is not retired until the destination copy is safely stored and verified.

## Trade

Old-school one-for-one exchange. Both users MUST offer a Pokémon.

```text
WILL offers Charizard
        <->
NIECE offers Eevee
```

A Trade cannot complete with an empty offer on either side.

Potential later extension:

- 1-for-1 trade — primary/default;
- multi-trade — 2-for-2, 3-for-3, etc.;
- Gift — separate deliberate action, never silently substituted for Trade.

Trade UI should show both offers and require explicit confirmation.

Atomic transaction requirements:

1. validate both source Vault records;
2. lock/freeze both offers for the transaction;
3. both users review and confirm;
4. create both destination records;
5. verify destination hashes/data;
6. commit transaction;
7. retire previous ownership only after successful commit;
8. preserve complete provenance/history.

A crash/network interruption must not allow only one side to lose its Pokémon.

---

# 5. PokeBank Friends and Mailbox

PokeBank NX should have its own identity/friend system independent of Nintendo friend codes and Nintendo/Pokémon online services.

Example:

```text
WILL
PokeBank ID: PB-7K4M-29QX
```

Friend flow:

```text
Friends
-> Add Friend
-> Enter PokeBank ID
-> Send Request
```

The recipient receives the request in a central Mailbox and must accept or decline.

Mailbox concept:

```text
MAILBOX
├── Friend Requests
├── Trade Requests
├── Gifts
├── Completed Trades
└── System Messages
```

From a Pokémon Action Sheet:

```text
Trade to Friend...
Gift to Friend...
```

Then select an accepted friend.

Friends should expose only minimal information such as display name, PokeBank ID, friendship date, and optional trade/gift history. A friend must not automatically gain permission to browse another user's Vault.

---

# 6. Local and Internet PokeBank-to-PokeBank trading

The same Vault transaction engine should support three transport modes:

```text
Same Switch
-> profile-to-profile local transaction

Nearby
-> local Wi-Fi / LAN

Friends
-> Internet relay
```

The UI should stay consistent regardless of transport.

## Lightweight Internet relay

Do NOT host users' full Vaults or game saves.

The online service should behave like a small encrypted post office containing only what is required for:

- PokeBank public identity/public key;
- friend relationships;
- friend requests;
- pending Gift metadata/payloads;
- pending Trade metadata/payloads;
- transaction state;
- acknowledgements/expiry/replay protection.

Vaults, saves, artwork, backups, indexes, and normal browsing remain local on each Switch.

Target security properties:

- TLS transport;
- per-profile local private key;
- public-key identity;
- signed requests;
- encrypted Pokémon payloads where practical;
- transaction IDs;
- expiration timestamps;
- replay protection;
- hash verification;
- atomic trade commit semantics.

A serverless/free-tier backend should be preferred initially so a small friends-and-family or early community deployment can operate with negligible cost. Donations may later fund hosting if usage grows substantially.

Do not attempt to use or impersonate Nintendo/Pokémon private services.

---

# 7. Guided Create / Generate Pokémon wizard

Creating a Pokémon should be generation/game aware rather than exposing one giant unsafe editor.

Primary flow:

```text
Create Pokémon
1. Choose Pokémon
2. Choose target/origin game and generation context
3. Choose encounter/origin details
4. Configure core data
5. Configure game-specific data
6. Resolve missing required fields
7. Run legality validation
8. Review
9. Create in current profile's Vault
```

The selected game controls which fields exist and which values are possible.

Field states:

```text
REQUIRED
OPTIONAL
CONDITIONAL
```

Conditional examples:

- Egg -> egg received/hatch metadata becomes relevant;
- shiny selection -> generation/event shiny rules apply;
- form selection -> form-specific legality requirements apply;
- event encounter -> event-specific OT/ball/met/ribbon constraints apply;
- older games -> modern-only fields must not be invented.

If the user tries to finish with missing required information, creation must stop and show a focused completion screen listing every missing field.

Example:

```text
Cannot create Pokémon yet

Missing:
- Met Location
- Poké Ball
- Trainer ID
- Language

[Fill Missing Fields]
[Cancel]
```

Contradictions should be surfaced as early as possible rather than waiting until the final screen.

Examples:

- impossible encounter/game combination;
- invalid met level;
- unavailable move;
- unavailable Ability;
- impossible Poké Ball;
- unavailable form;
- shiny lock;
- event-only origin mismatch.

## Quick Legal mode

Offer a simplified creation path for users who do not want to configure every technical field:

```text
Quick Legal
-> Species
-> Game
-> Level
-> Shiny?
-> optional Nature / preferences
```

PokeBank fills reasonable valid defaults, clearly reports which fields were auto-filled, then performs the same legality/review gate.

Generated Pokémon should enter the current profile's Vault first. They should not be written directly into a game save.

---

# 8. Transfer planner and compatibility preview

Before moving a Pokémon from the Vault into a game, show exactly what will happen.

Potential preview:

```text
Transfer Eevee -> Pokémon Scarlet

Compatible: YES
Moves changed: 2
Ability preserved: YES
Ball preserved: YES
Form preserved: YES
Ribbons preserved: YES
Game-specific values converted: 3

[View Changes]
[Continue]
```

The user should be able to inspect:

- destination compatibility;
- moves that cannot exist in destination;
- relearn/move-pool options;
- form changes;
- Ability changes;
- stat-system conversions;
- ribbons/marks that remain meaningful;
- fields that are preserved but not displayed by the destination;
- irreversible transformations.

No silent destructive conversion.

---

# 9. Deep provenance / Pokémon Journey

PokeBank NX can go further than a normal storage box by preserving a human-readable history for every Pokémon.

Example:

```text
Charizard

Origin
Pokémon FireRed
RetroArch
Trainer: WILL

Journey
FireRed save
-> WILL's Vault
-> traded to NIECE for Eevee
-> Niece's Vault
-> Pokémon Violet
```

Preserve:

- original game/platform/source;
- source-save identity;
- original owner/trainer where meaningful;
- imports;
- clones/derived records;
- conversions;
- Vault ownership changes;
- Gift/Trade history;
- destination writes;
- legality state at important transitions.

A visual provenance graph/timeline is a long-term differentiator.

---

# 10. Collection intelligence / Dex planner

Go beyond simply displaying a National Dex.

Provide:

- National Dex;
- game-specific Dexes;
- Living Dex;
- Shiny Living Dex;
- forms;
- gender differences where meaningful;
- Gigantamax/Mega/regional forms;
- event/gift checklist;
- ribbon/mark collection;
- owned/missing counts;
- duplicate counts;
- source-game provenance coverage.

A "What am I missing?" planner can inspect the user's supported games and saves and answer:

```text
You are missing 17 Pokémon from your Living Dex.

Available in games you already have:
- 9 can be caught in Scarlet
- 4 can be evolved from Pokémon already in your Vault
- 2 are available in SoulSilver
- 2 require trade/event/another game
```

Never claim an encounter is available without a validated encounter/game data source.

---

# 11. Event / Mystery Gift Library

Post-v1 target.

Maintain a provenance-aware catalog of historical distributions and gifts rather than a generic cheat menu.

Potential capabilities:

- browse events by game/generation/region/date;
- show distribution conditions;
- track which events the current profile has collected;
- distinguish legitimate historical records from generated recreation;
- import verified event payloads where legally/technically appropriate;
- EventDex / GiftDex collection tracking;
- never falsely label generated recreation as historically received.

This remains separate from current parser milestones.

---

# 12. Save Timeline / Time Machine

Because PokeBank NX already requires safe backups before writes, expose that safety data as a useful user feature.

Potential workflow:

```text
SoulSilver · DraStic

Timeline
- Today 21:31     Current
- Today 18:02     Before Vault transfer
- Sep 6           Backup
- Aug 30          Imported snapshot
```

Capabilities:

- automatic pre-write snapshots;
- user-named snapshots;
- compare two snapshots;
- show changed Party/Boxes/items/trainer data;
- restore only through an explicit validated recovery flow;
- never silently replace a live source.

This could become one of PokeBank NX's strongest preservation features.

---

# 13. Duplicate / corruption / integrity tools

Provide Vault health tools:

- duplicate finder;
- exact-byte duplicate vs same-Pokémon-but-different-history distinction;
- damaged/corrupt record detection;
- missing provenance warnings;
- orphaned Bank reference repair;
- integrity hash verification;
- Vault index rebuild;
- transaction journal recovery.

Never silently deduplicate Pokémon solely because their raw payload hashes match; ownership/history may intentionally differ.

---

# 14. Battle / team tools

Optional later feature set:

- team builder from Vault Pokémon;
- side-by-side stat/IV/EV comparison;
- nature/Ability/move analysis;
- game-specific move availability;
- battle-ready checklist;
- competitive tags/teams;
- export/share team summaries;
- current-game legality checks.

Do not let competitive features distract from storage/transfer reliability milestones.

---

# 15. Challenges / achievements / collection goals

Pokémon HOME has Challenges/profile rewards; PokeBank NX can use the concept locally without requiring a paid service.

Potential local achievements:

- complete a generation Living Dex;
- complete a Shiny Living Dex;
- collect every Unown form;
- collect every Vivillon pattern;
- complete a Ribbon Master journey;
- preserve Pokémon originating from every supported generation;
- perform first local trade / online friend trade;
- recover a save safely from backup.

Rewards should be cosmetic/profile-oriented, not required for core functionality.

---

# 16. Optional artwork-pack system

Separate application functionality from artwork packs.

Concept:

```text
Artwork
├── Default / Minimal
├── HD Render Pack
├── DS Pixel Pack
├── Community Pack
└── Custom User Pack
```

Provide a documented pack format and integrity/version metadata.

PokeBank NX should not assume that every publicly available Pokémon sprite repository grants commercial redistribution rights. Licensing/provenance must be evaluated per pack.

Current development artwork does not need to be replaced during active save-engine milestones.

---

# 17. Android companion app

Long-term optional companion, not required for Switch operation.

The Switch `.nro` remains the primary offline product.

Potential Android companion capabilities:

- pair with a PokeBank profile using QR/code;
- browse a synchronized/read-only Vault index;
- search/filter collection from phone;
- view Pokémon summaries/provenance;
- receive Friend/Mailbox notifications;
- accept/decline friend requests;
- inspect/prepare Gift and Trade requests;
- manage Banks/tags where safely synchronized;
- collection/Dex planning;
- backup status/health overview;
- optional encrypted local-network sync.

Do not require cloud storage of the full Vault merely to provide a mobile app.

Future iOS/desktop/web companions can reuse the same protocol if worthwhile.

---

# 18. Physical hardware / real-cartridge bridge

Post-v1 stretch/companion project.

`PokeBank NX Link` can eventually bridge real GB/GBC/GBA hardware/cartridges to PokeBank NX using original hardware and an external device/cable/adapter.

Goals:

- read real-cartridge/save data safely;
- preserve original bytes and provenance;
- back up before any write;
- transfer through the user's Vault rather than unsafe direct mutation;
- support real old-school hardware trading workflows where technically feasible;
- keep hardware protocol work isolated from normal emulator/source support.

Do not implement until software Vault/write safety is mature.

---

# 19. Features worth matching from Pokémon HOME

As of 2026, useful Pokémon HOME concepts include:

- centralized Pokémon storage;
- compatible-game transfer;
- detailed Pokémon information;
- Judge/IV information;
- National Pokédex;
- game-specific Pokédexes;
- move selection when withdrawing to supported games;
- Mystery Gifts;
- Pokédex completion rewards;
- Challenges/profile customization;
- Friend Trade;
- Wonder-style trading;
- GTS-style requested trades;
- Room Trade;
- Battle Data / usage information;
- points/reward systems;
- collectible/tradable medals;
- mobile + Switch access to the same collection.

PokeBank NX does not need to clone all of these. The goal is to adopt useful interaction patterns where they fit an offline/open architecture.

---

# 20. Things PokeBank NX can do beyond Pokémon HOME

The strongest differentiators are the things an offline preservation-focused tool can do that an official cloud service generally does not expose:

- one workspace across GB/GBC/GBA/DS/3DS/Switch-era sources;
- emulator-aware source grouping;
- multiple independent save characters for the same game;
- profile-scoped family Vaults;
- user-controlled local storage;
- unlimited named logical Banks over one immutable Vault;
- deep origin/provenance history;
- exact source-save identity;
- save backups and timeline comparisons;
- read-only recovery/diagnostic tools;
- guided generation/game-aware Pokémon creation;
- PKHeX-style inspection/legality with guardrails;
- explicit COPY/MOVE/CLONE semantics;
- explicit Gift vs mandatory-exchange Trade semantics;
- same-Switch profile trading;
- local Wi-Fi trading;
- independent PokeBank friend/mailbox trading over the Internet;
- custom/optional artwork packs;
- physical legacy-hardware bridge;
- collection-gap planning based on the games/saves the user actually owns;
- local-first operation with no subscription required for the core Vault.

---

# 21. Public trade-network stretch ideas

Only after friend-to-friend trading is stable and abuse/security implications are understood:

- Wonder-style random Vault trade;
- GTS-like listing: offer X, request Y;
- private trade rooms;
- family/private groups;
- wishlist matching;
- public trade reputation/history that does not expose private Vault contents.

These features introduce moderation, abuse, hosting, and trust concerns. They are not v1 requirements and must not delay the core product.

---

# 22. Funding philosophy

Current preferred direction:

- free/open-source core application;
- no hidden ad traffic;
- no required subscription for local Vault functionality;
- optional donations / GitHub Sponsors / Ko-fi-style project funding;
- funding can support hardware, hosting, testing devices, domains, and long-term compatibility work.

Do not let funding mechanics compromise save safety, privacy, or user trust.

---

# Long-term product principle

PokeBank NX should feel like a user's permanent Pokémon preservation library rather than merely another save editor.

The priority order remains:

```text
preserve original data
-> understand provenance
-> organize safely
-> validate compatibility
-> transfer deliberately
-> trade/gift transactionally
-> add convenience/social features only after the foundation is trustworthy
```
