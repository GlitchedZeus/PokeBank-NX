# PokeBank NX — v2.0 Roadmap

> **Future-only / scope-control document.** This does not expand the current FRLG milestone or the v1.0 release requirements. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active implementation work.

# v2.0 theme: PokeBank NX Connected

PokeBank NX v2.0 is the target release for the Android companion and the first full connected/social layer.

The Switch `.nro` remains the authoritative local-first product. v2.0 networking must enhance the local Vault, not replace it with mandatory cloud storage.

## Preferred connected/social shell — Trainer Plaza

**Trainer Plaza is now the preferred social direction for PokeBank NX Connected.** It supersedes the earlier Pokémon Ranch-style concept as the primary community/social presentation.

The design is inspired by the lightweight identity/pass/radar/collection philosophy of `Insektaure/NX-Plaza`, but PokeBank NX should use an independently designed Pokémon-native implementation and protocol rather than copying NX-Plaza code or assets wholesale.

Core Trainer Plaza concepts:

- per-profile PokeBank identity and short human-readable PokeBank ID;
- public Trainer Card / Trainer Passport;
- user-selected Partner Pokémon as the primary social visual identity;
- optional public collection statistics chosen by the user;
- Friends list with Partner Pokémon presentation;
- Mailbox;
- Trainer encounters using small opt-in public Trainer Passes;
- TrainerDex of people encountered/met;
- Poké Radar presence/discovery presentation;
- Friend Gift and mandatory one-for-one Friend Trade;
- Android notifications and social access;
- minimal relay service rather than a mandatory cloud Vault.

The first v2.0 release does not need the complete advanced encounter system. A basic Trainer Plaza shell, Trainer Cards/Partner Pokémon, Friends, Mailbox and safe online exchange may ship first, while richer Poké Radar/TrainerDex/world encounter features can expand through v2.x.

Detailed vision: `docs/TRAINER_PLAZA_VISION.md`.

## Headline v2.0 feature — Android companion

The Android app is a first-class v2.0 target rather than an undefined post-v1 stretch goal.

Target capabilities:

- pair with a PokeBank NX profile by QR code / PokeBank code;
- browse a synchronized or read-only Vault index;
- search/filter Pokémon and Banks;
- view Pokémon summaries, Judge data, legality status and provenance;
- National/game/Living/Shiny/Form Dex views and collection planning;
- Trainer Passport / Trainer Card;
- Partner Pokémon display;
- Trainer Plaza;
- Friends list and PokeBank IDs;
- Mailbox notifications;
- accept/decline friend requests;
- review incoming Gifts;
- review/respond to Trade requests;
- prepare outgoing Gift/Trade requests;
- view trade/gift history;
- achievements/challenges/medals where implemented;
- backup/Vault health overview;
- encrypted local-network sync when both devices are nearby;
- optional Internet relay support for mailbox/friend/Trainer Plaza features.

The Android app must not require uploading the user's full Vault to a server merely to function.

## PokeBank identity, Trainer Card and Partner Pokémon

v2.0 targets a PokeBank-owned social identity separate from Nintendo account identity.

Target properties:

- random public PokeBank identity;
- short shareable PokeBank ID;
- private authentication secret/key material;
- add/pair by code or QR;
- no identity derived from Switch serial, MAC, Nintendo account ID or IP;
- profile-scoped Connected identity;
- explicit backup/migration/reset behavior.

Trainer Card may expose only user-approved public fields such as:

```text
Display name
Partner Pokémon
Greeting/status
Favourite Pokémon/game/generation
Optional Living Dex progress
Optional shiny count
Collector medals/themes
Trade availability
```

Public Partner Pokémon data is display metadata only. It must not expose a reconstructable Pokémon payload or private Vault/save identifiers.

## Friends and Mailbox

v2.0 targets the PokeBank-owned social relationship system:

- per-profile PokeBank ID;
- add friend by code/QR;
- friend request → Mailbox → accept/decline;
- remove/block/report friend;
- minimal public Trainer Card only;
- Partner Pokémon display;
- no automatic remote Vault browsing.

Mailbox categories:

```text
Friend Requests
Trade Requests
Gifts
Completed Trades
System Messages
```

## Trainer Encounters / TrainerDex / Poké Radar

Trainer encounters are a future Pokémonized StreetPass-style social layer built on small opt-in Trainer Passes.

A Trainer Pass may include only public information explicitly chosen by the user. It must never automatically contain raw Pokémon payloads, Vault contents, Nintendo account data, precise location, TID/SID or private save metadata.

Potential Connected surfaces:

```text
Trainer Plaza
Poké Radar
TrainerDex
Encounter Cards
Friends
```

Requirements:

- encounters never transfer Pokémon ownership;
- repeat encounters can increment local relationship history;
- users can favourite, forget, block or report encountered trainers;
- worldwide/public discovery is opt-in;
- nearby/local semantics must not pretend to know precise distance/location;
- Internet discovery must be rate limited and abuse resistant;
- cached TrainerDex/history remains useful offline.

Advanced encounter/radar/worldwide discovery may ship after the initial v2.0 Connected foundation in v2.x.

## Internet Gift and Friend Trade

v2.0 targets remote Vault-to-Vault exchange using PokeBank NX's own protocol, independent of Nintendo/Pokémon private services.

Gift:

```text
Sender Vault -> pending Gift -> recipient accepts -> recipient Vault
```

Trade:

```text
Sender offers Pokémon A
Recipient must offer Pokémon B
Both review and confirm
Atomic Vault-to-Vault exchange
```

Trade remains distinct from Gift. A normal Trade requires an offered Pokémon from both sides.

Required properties:

- staged transaction state;
- signed/authenticated requests;
- encrypted payloads where practical;
- hashes and transaction IDs;
- expiry/replay protection;
- retry/recovery handling;
- idempotent completion/acknowledgement behavior;
- no ownership retirement until destination verification succeeds;
- complete provenance/history.

Trainer encounters and public cards are never an automatic transfer route into this transaction engine.

## Lightweight relay service

The v2.0 online backend should act as a small identity/presence/mailbox/transaction relay, not a hosted Pokémon HOME replacement.

Store only what is necessary for:

- public PokeBank identity/key material;
- public Trainer Card snapshot;
- friend relationships/requests;
- block/report/abuse controls;
- limited presence/check-in state;
- mailbox state;
- small encrypted Gift/Trade payloads;
- transaction status and acknowledgements;
- expiry/replay/rate-limit metadata.

Vaults, game saves, artwork, backups and normal collection data remain local by default.

Prefer inexpensive/serverless infrastructure and a design capable of operating inside free tiers for small usage. Where practical, keep the service replaceable/self-hostable instead of permanently tying the app to one operator.

## What can ship before 2.0

The following do not need to wait for the Android app and may appear during 1.x when the underlying Vault transaction engine is mature:

- same-Switch profile-to-profile Trade/Gift;
- nearby/LAN Switch-to-Switch Trade/Gift;
- QR/local pairing;
- local Trainer Passport;
- local Partner Pokémon / Trainer Card presentation;
- local achievements/challenges;
- local Vault/Dex planners.

These should reuse the same transaction/data model later used by v2.0 Internet and Android features.

## Not required for initial v2.0

Do not hold the Android/Friends/Trainer Plaza foundation hostage for every public-community feature.

These can come after 2.0:

- richer Trainer Plaza scenery/animation;
- public/worldwide Trainer encounter expansion;
- advanced Poké Radar discovery modes;
- deeper TrainerDex progression/medals/stamps;
- Wonder Exchange / Wonder Box-style public pool;
- GTS/Trade Board public listings;
- Room Trade / large public rooms;
- global battle-usage analytics;
- desktop/iOS/web clients;
- physical cartridge/link hardware experiments.

## Pokémon Ranch idea status

The older Pokémon Ranch-style social idea is no longer the preferred Connected social direction.

```text
Preferred social direction:
Trainer Plaza + Partner Pokémon + Trainer Card
+ Friends + Mailbox + TrainerDex + Poké Radar
+ safe Gift/Trade
```

A private/local Ranch-like visualization may remain a later cosmetic idea if still desirable, but it does not take priority over Trainer Plaza and must not block a release.

## Reference/licensing boundary

`Insektaure/NX-Plaza` is useful as a design/reference project for lightweight identity, pass exchange, presence/radar, cached encounters, background sync, privacy controls and minimal backend architecture.

NX-Plaza is GPL-2.0. PokeBank NX should prefer a clean-room implementation of these concepts with its own protocol, data model, UI and ownership-transaction guarantees. Do not copy substantial NX-Plaza source/assets into PokeBank NX casually.

## Release principle

```text
v1.0 = PokeBank NX works as a safe, useful local Pokémon Vault/save platform.

v1.x = deepen local features, game coverage, planners, editor/transfer tools, and local/LAN exchange.

v2.0 = PokeBank NX Connected: Android + Trainer Plaza foundation + PokeBank IDs + Friends + Mailbox + Internet Gift/Trade.

v2.x+ = richer Trainer encounters/Poké Radar/TrainerDex + public exchange systems + additional companion clients.
```

The Android companion and Connected foundation are therefore committed **v2.0 targets**, while the deeper Trainer Plaza social world can grow incrementally without delaying earlier releases.
