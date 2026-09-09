# PokeBank NX — Trainer Plaza Vision

> **Future-only product/design document.** This does not expand the current FRLG milestone, v1.0 requirements, or any active Codex task. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active implementation.

## Decision

**Trainer Plaza is the preferred future social/online direction for PokeBank NX Connected.**

It supersedes the earlier Pokémon Ranch-style social idea as the primary connected/community concept. A Ranch-like local visualizer may still exist someday as an optional cosmetic/local feature, but it is no longer the preferred social shell and must not take priority over Trainer Plaza.

The inspiration is the public open-source project **Insektaure/NX-Plaza**, especially its lightweight local identity, card/pass exchange, collection of encountered users, radar/presence presentation, background sync, privacy controls, and small-server philosophy.

PokeBank NX must implement its own Pokémon-native design and protocol. NX-Plaza is GPL-2.0; do not copy substantial source code, assets, or implementation wholesale into PokeBank NX unless the project's licensing is deliberately made compatible. Prefer clean-room implementation based on documented product concepts and independently designed data formats/APIs.

---

# Product goal

Turn the existing v2.0 Friends + Mailbox + Internet Gift/Trade plan into a memorable Pokémon social layer instead of a plain account list.

Conceptually:

```text
PokeBank NX Connected
        |
        +-- Trainer Plaza
        +-- Trainer Card / Passport
        +-- Partner Pokémon
        +-- Friends
        +-- Mailbox
        +-- Poké Radar
        +-- TrainerDex
        +-- Trade Center
        +-- Android companion
```

The Vault remains the serious local storage/provenance core. Trainer Plaza is the optional social presentation around it.

---

# Core principles

1. **Local-first** — the Switch Vault, Banks, saves, provenance and backups remain local and useful without an Internet service.
2. **Opt-in social identity** — Connected features are optional and use a PokeBank-owned identity, not Nintendo account credentials.
3. **Minimal disclosure** — viewing a Trainer Card never grants remote access to the user's Vault.
4. **Partner Pokémon is display metadata** — public cards do not expose reconstructable Pokémon payloads.
5. **Trade is transactional; encounter is social** — automatic Trainer encounters never transfer ownership of a Pokémon.
6. **Gift and Trade remain distinct** — Gift is deliberate one-way transfer; Trade requires one offered Pokémon from each side.
7. **Crash-safe exchange** — no Pokémon ownership retirement until both sides and the destination transaction are verified.
8. **No fake Nintendo/HOME protocol** — use a PokeBank-owned protocol and relay.
9. **Privacy by design** — never publish Nintendo account ID, friend code, Switch serial, MAC, IP, save paths, TID/SID, private Vault contents or secret authentication material.
10. **No release hostage** — Trainer Plaza must not block v1.0 or v1.x local functionality.

---

# PokeBank identity

On first Connected setup, generate an independent identity using a cryptographically secure random source.

Conceptual identity:

```text
Public internal ID: random 128-bit+ identifier
Human PokeBank ID: PBX-K4M-92X7 style short code
Secret auth token/key: random 256-bit+ secret
QR representation: public pairing/friend data only
```

Requirements:

- never derive identity from console serial, MAC, Nintendo account ID or IP;
- secret auth material never appears in QR friend codes;
- identity can be backed up/migrated deliberately;
- resetting Connected identity does not alter local Vault ownership;
- profile-scoped identity: separate PokeBank profiles can have separate Connected identities;
- block/remove/recovery flows must be designed before public deployment.

---

# Trainer Card / Trainer Passport

Every Connected profile can create a public Trainer Card.

Suggested fields:

```text
Display name
PokeBank ID
Trainer avatar / portrait
Partner Pokémon
Optional public team showcase (display-only)
Greeting/status
Favourite Pokémon
Favourite generation/game
Recently played game, only when explicitly enabled
Living Dex progress, optional
Shiny collection count, optional
Collector medals, optional
Trade availability/status
Trainer Card theme/background
```

Public Pokémon display metadata must be intentionally limited. For a Partner Pokémon, publish only what is needed to render the card, for example:

```text
species
form
shiny flag
optional nickname
optional gender
safe cosmetic badges
```

Do **not** expose full PK structures, PID, encryption constants, TID/SID, IVs, private OT metadata, complete provenance payloads, save locations or anything sufficient to clone/reconstruct a private Pokémon from the public card.

---

# Partner Pokémon

A user may choose a Pokémon from their Vault as their **Partner Pokémon**.

The Partner is the central visual identity for Trainer Plaza and can appear on:

- Trainer Card;
- Friends list;
- TrainerDex;
- encounter cards;
- Poké Radar;
- Plaza scene;
- Android notifications/profile.

The relationship should reference the local Vault entity internally, while the online representation remains a restricted display snapshot.

If the local Pokémon is moved, deleted, quarantined or becomes unavailable, the app should fail gracefully and allow choosing a new Partner rather than leaking or reconstructing stale private data.

---

# Trainer Plaza

Trainer Plaza is the main Connected social landing page.

Instead of a static friend list, it should feel like a Pokémon social hub containing the user's own profile plus people they have met or befriended.

Possible visual model:

```text
Trainer Plaza

        Sarah + Sylveon

Alex + Gengar       YOU + Umbreon

        Jamie + Eevee
```

The first implementation does not need a complicated 3D world. A performant controller-first 2D plaza using the existing HD Pokémon artwork is acceptable and likely preferable.

Opening another trainer can offer:

```text
View Trainer Card
Add Friend / Friend Status
Send Gift
Request Trade
View Encounter History
Block
Report
```

Actions unavailable because of privacy/settings/status should be hidden or clearly disabled.

---

# Trainer Encounters

Trainer Encounters are the Pokémonized equivalent of a lightweight StreetPass-style pass exchange.

When explicitly enabled, the app may exchange a very small **Trainer Pass** containing only the user's selected public information.

Example:

```text
NEW TRAINER ENCOUNTER

ALEX
Partner: Shiny Gengar
Greeting: "Gen III is peak."
Living Dex: 988 / 1025   [if shared]
Favourite Game: Emerald  [if shared]
Met: 1 time
```

Requirements:

- encounters do not move Pokémon;
- encounters do not expose Vault content beyond opt-in aggregate/display data;
- repeat encounters increment local relationship history;
- users can star/favourite, forget or block encountered trainers;
- public/worldwide encounter mode must be opt-in;
- location/nearby design must not publish precise location;
- rate limits and abuse prevention are mandatory for Internet encounters.

A later version may support coarse encounter scopes such as:

```text
Friends only
Same/local network
Nearby/coarse network area
Worldwide
```

Exact privacy/network semantics require a separate security review before implementation.

---

# Poké Radar

Poké Radar is a presence/discovery screen inspired by the radar concept in NX-Plaza.

Possible modes:

```text
Friends
Nearby / Local
Worldwide [opt-in]
```

The radar displays small Trainer + Partner representations and connection/presence state. It must not imply precise physical distance unless the system can truthfully establish it.

Poké Radar is for discovering/viewing Trainer Passes and initiating friend/social actions. It must never automatically exchange Vault Pokémon.

---

# TrainerDex

TrainerDex is the user's local collection/history of people encountered through PokeBank NX.

Suggested stats:

```text
Unique Trainers Met
Friends
Repeat Encounters
Trades Completed
Gifts Sent/Received
Favourite Trainers
First Met / Last Met
```

Suggested sorting:

```text
Recent
Name
Most Met
Friends
Favourite
Trade Partners
```

TrainerDex should remain useful offline using locally cached public-card snapshots and history.

---

# Friends

Friends remain explicit mutual/accepted relationships, separate from passive encounters.

Required actions:

```text
Add by PokeBank ID
Add by QR
Friend request
Accept / decline
Remove
Block
Report
View public Trainer Card
Request Trade
Send Gift
```

Friends do not gain automatic Vault browsing rights.

A later deliberate permission model could share selected Banks/showcases, but private Vault browsing is **not** a baseline Friend capability.

---

# Mailbox

Mailbox is the durable async interaction layer.

Categories:

```text
Friend Requests
Trade Requests
Gifts
Completed Trades
System Messages
```

Mailbox must work across Switch and Android companion when Connected is enabled.

Incoming Pokémon are not written directly into live game saves. They enter the safe Vault transaction/staging system first.

---

# Trade Center

Friend Trade uses the existing mandatory one-for-one product rule.

Conceptual review:

```text
YOUR OFFER
Pokémon A

     <->

THEIR OFFER
Pokémon B
```

Before final confirmation, both sides should see safe human-readable summaries including legality/provenance state as available.

Transaction concept:

```text
lock offers
-> stage immutable exchange payloads
-> authenticate both sides
-> assign transaction ID
-> both sides confirm
-> destination validation
-> commit destination Vault entries
-> acknowledgements
-> retire source ownership only after verified success
-> append provenance/trade history
```

The network/service must support idempotent retry, expiry and replay protection. A crash, Wi-Fi loss, server restart or app close must not destroy both sides' only copy.

---

# Gifts

Gift is explicit one-way transfer and remains a different action from Trade.

Flow:

```text
Sender Vault
-> staged Gift
-> encrypted pending mailbox payload
-> recipient reviews
-> recipient accepts
-> recipient Vault validation/commit
-> sender receives completion acknowledgement
-> provenance updated
```

Exact ownership semantics must be decided with the transaction engine. Do not silently treat a Gift as a Clone.

---

# Lightweight Connected relay

The online backend should be a small identity/friend/mailbox/transaction relay, not a mandatory hosted Vault.

Server-side data should be limited to what is operationally required, such as:

```text
PokeBank public identity/key material
public Trainer Card snapshot
friend relationships/requests
block/abuse state
presence/check-in state
mailbox metadata
encrypted pending Gift/Trade payloads
transaction state/acknowledgements
expiry/replay/rate-limit metadata
```

Keep local by default:

```text
full Vault
Banks
game saves
backups
complete Pokémon collection database
private provenance records
private trainer/save identifiers
secret authentication keys
```

Server deployment should be replaceable/self-hostable where practical so the application is not permanently dependent on one operator.

---

# Android companion integration

Trainer Plaza gives the v2.0 Android companion a clear social role.

Potential Android tabs:

```text
Home
Vault / Dex
Trainer Plaza
Friends
Mailbox
Trades
Notifications
```

Notifications may include:

```text
New friend request
Incoming Gift
Trade request
Trade accepted/completed
New Trainer encounter summary
System/security notice
```

The Android companion should not need the user's full raw Vault uploaded to the relay merely to provide these features.

---

# Social progression / cosmetics

Optional later additions can make encounters enjoyable without affecting Pokémon legality or creating pay-to-win mechanics.

Examples:

- Trainer medals for first encounter, 10/100/1000 trainers, first trade, multi-generation trade history, etc.;
- regional/generation PokéStamp pages;
- Trainer Card themes/backgrounds;
- Partner Pokémon frames/badges;
- collector titles;
- achievement-linked cosmetics.

No real-money gambling, loot-box progression or legality-affecting rewards are part of this design.

---

# Relationship to the older Pokémon Ranch idea

Product direction as of 2026-09-08:

```text
OLD preferred social idea:
Pokémon Ranch-style shared/social presentation

NEW preferred social idea:
Trainer Plaza + Partner Pokémon + TrainerDex + Poké Radar
+ Friends + Mailbox + safe Gift/Trade
```

Rationale:

- Trainer Plaza naturally uses the Friends/Mailbox/Trade infrastructure already planned for v2.0;
- it gives every user a recognizable social identity through a Partner Pokémon;
- it works with Switch and Android;
- it can be useful with very little server storage;
- it provides social discovery without granting access to private Vault data;
- it scales from simple cards/lists to a richer plaza later;
- it is more directly connected to PokeBank's collection and trading purpose than a standalone Ranch visualization.

A private/local Ranch-style box visualization remains an optional long-term cosmetic feature only if it is still desirable after the core product ships.

---

# NX-Plaza inspiration boundary

Reference project:

```text
https://github.com/Insektaure/NX-Plaza
```

Useful concepts to study independently:

- random public identity + private bearer/secret credential;
- short human-readable identity representation;
- lightweight outgoing public card/pass;
- background check-in/sync worker;
- local cached collection of encountered identities;
- presence/radar UI;
- block/forget queue that survives restarts;
- configurable backend URL;
- minimal server model;
- explicit privacy controls;
- rate-limited encounters;
- offline-first cached presentation.

Do not assume NX-Plaza protocol or implementation is automatically suitable for Pokémon ownership exchange. PokeBank Gift/Trade requires much stronger transactional, provenance, replay-protection and crash-recovery guarantees.

---

# Release placement

Preferred sequencing:

```text
v1.0
safe local Vault/save platform

v1.x
expanded games + local tools + same-Switch/LAN exchange foundation

v2.0
PokeBank NX Connected foundation
- Android companion
- PokeBank IDs
- Trainer Card / Partner Pokémon
- Friends
- Mailbox
- safe Internet Gift/Friend Trade
- initial Trainer Plaza shell

v2.x
- richer Trainer encounters
- Poké Radar discovery modes
- TrainerDex expansion
- public/worldwide encounter layer
- Wonder Exchange / Trade Board / Room Trade if still desired
- richer Plaza cosmetics/social progression
```

The exact split between initial Trainer Plaza in v2.0 and advanced encounter features in v2.x may change based on complexity, security and server cost. The direction itself is now the preferred Connected/social vision.

---

# Non-goals for early implementation

Do not begin Trainer Plaza during the current FRLG/RSE/mainline save-adapter work.

Early Trainer Plaza does not require:

- public Vault browsing;
- cloud-hosted full Vaults;
- Nintendo account integration;
- Nintendo friend-code integration;
- fake Pokémon HOME services;
- precise geolocation;
- voice chat;
- public text chat;
- a 3D open world;
- automatic Pokémon exchange;
- public GTS/Wonder/Room systems on day one.

The first Connected release should prove identity, privacy, friends, mailbox and safe transactions before expanding into a broad public network.
