# PokeBank NX — v2.0 Roadmap

> **Future-only / scope-control document.** This does not expand the current FRLG milestone or the v1.0 release requirements. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active implementation work.

# v2.0 theme: PokeBank NX Connected

PokeBank NX v2.0 is the target release for the Android companion and the first full connected/social layer.

The Switch `.nro` remains the authoritative local-first product. v2.0 networking must enhance the local Vault, not replace it with mandatory cloud storage.

## Headline v2.0 feature — Android companion

The Android app is a first-class v2.0 target rather than an undefined post-v1 stretch goal.

Target capabilities:

- pair with a PokeBank NX profile by QR code / PokeBank code;
- browse a synchronized or read-only Vault index;
- search/filter Pokémon and Banks;
- view Pokémon summaries, Judge data, legality status and provenance;
- National/game/Living/Shiny/Form Dex views and collection planning;
- Trainer Passport / profile view;
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
- optional Internet relay support for mailbox/friend features.

The Android app must not require uploading the user's full Vault to a server merely to function.

## Friends and Mailbox

v2.0 targets the PokeBank-owned social identity system:

- per-profile PokeBank ID;
- add friend by code/QR;
- friend request → Mailbox → accept/decline;
- remove/block friend;
- minimal public profile information;
- no automatic remote Vault browsing.

Mailbox categories:

```text
Friend Requests
Trade Requests
Gifts
Completed Trades
System Messages
```

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
- signed requests;
- encrypted payloads where practical;
- hashes and transaction IDs;
- expiry/replay protection;
- retry/recovery handling;
- no ownership retirement until destination verification succeeds;
- complete provenance/history.

## Lightweight relay service

The v2.0 online backend should act as a small mailbox/transaction relay, not a hosted Pokémon HOME replacement.

Store only what is necessary for:

- public PokeBank identity/key material;
- friend relationships/requests;
- mailbox state;
- small encrypted Gift/Trade payloads;
- transaction status and acknowledgements.

Vaults, game saves, artwork, backups and normal collection data remain local by default.

Prefer inexpensive/serverless infrastructure and a design capable of operating inside free tiers for small usage.

## What can ship before 2.0

The following do not need to wait for the Android app and may appear during 1.x when the underlying Vault transaction engine is mature:

- same-Switch profile-to-profile Trade/Gift;
- nearby/LAN Switch-to-Switch Trade/Gift;
- QR/local pairing;
- local Trainer Passport;
- local achievements/challenges;
- local Vault/Dex planners.

These should reuse the same transaction/data model later used by v2.0 Internet and Android features.

## Not required for v2.0

Do not hold the Android/Friends release hostage for every public-community feature.

These can come after 2.0:

- Wonder Exchange / Wonder Box-style public pool;
- GTS/Trade Board public listings;
- Room Trade / large public rooms;
- global battle-usage analytics;
- desktop/iOS/web clients;
- physical cartridge/link hardware experiments.

## Release principle

```text
v1.0 = PokeBank NX works as a safe, useful local Pokémon Vault/save platform.

v1.x = deepen local features, game coverage, planners, editor/transfer tools, and local/LAN exchange.

v2.0 = PokeBank NX Connected: Android + Friends + Mailbox + Internet Gift/Trade.

v2.x+ = public exchange systems and additional companion clients.
```

The Android companion is therefore a committed **v2.0 target**, not a requirement for v1.0 and not an undefined someday feature.
