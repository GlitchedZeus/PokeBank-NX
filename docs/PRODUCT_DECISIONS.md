# PokeBank NX — Product Decisions

> **Future/product-policy document.** This does not expand the current FRLG coding milestone. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active implementation work.

This file records product decisions that should stay stable across future roadmap discussions.

---

## 1. v1.0 game scope = mainline handheld/current lineage

PokeBank NX v1.0 targets the mainline Pokémon game history across:

- GB / GBC;
- GBA;
- Nintendo DS;
- Nintendo 3DS;
- Nintendo Switch.

Current identity accounting:

```text
23 current host-tested identities
+ 9 Nintendo DS identities
+ 8 Nintendo 3DS identities
= 40 target v1.0 identities
```

GameCube does not block v1.0.

### v1.x game expansion

Prioritize:

1. Pokémon Colosseum;
2. Pokémon XD: Gale of Darkness.

These are meaningful Pokémon save/provenance sources and should be developed before Stadium.

### Later v1.x / stretch archival targets

- Pokémon Stadium;
- Pokémon Stadium 2.

Stadium support is useful archival/team/storage work, but must not delay the mainline v1.0 release or the GameCube expansion.

---

## 2. Android companion = v2.0 headline feature

PokeBank NX v2.0 is the target for **PokeBank NX Connected**:

- Android companion app;
- PokeBank IDs;
- Trainer Plaza foundation;
- Trainer Card / Trainer Passport;
- Partner Pokémon;
- Friends;
- Mailbox;
- Internet Gifts;
- mandatory-exchange Friend Trades;
- lightweight encrypted relay service.

The Android app is not required for v1.0 or v1.x.

Same-Switch and nearby/LAN Gift/Trade may arrive in v1.x because they reuse the local Vault transaction model without requiring the Internet service.

---

## 3. Verified Breeding / lineage rule

PokeBank NX must never invent historical parents that a Pokémon save did not store.

Instead, future breeding uses explicit lineage states.

### Founder Verified

A Pokémon can enter the verified breeding pool as a **Founder Verified** entity when:

- it is imported/read from a supported, validated game save or other approved source;
- its PokeBank provenance begins at that source;
- no historical parent identities are claimed.

A founder does not need known parents because it is the root of a PokeBank-tracked family tree.

### Lineage Verified

A Pokémon becomes **Lineage Verified** when PokeBank NX manages/witnesses the breeding transaction and records both parent Vault IDs.

Example:

```text
Founder Eevee #001 + Founder Ditto #002
                 ↓
          Eevee #003
          Lineage Verified
          parents: #001 / #002
```

Later descendants can build a real multigeneration family tree because every parent relationship was recorded by PokeBank NX.

### Lineage Unknown

An imported Pokémon whose historical parents are not stored/known remains:

```text
LINEAGE UNKNOWN
```

PokeBank NX does not guess its parents.

### Breeding eligibility

For the strict **Verified Breeding** feature, only:

- Founder Verified Pokémon; and
- Lineage Verified Pokémon

may be selected as parents, subject to the selected game's real breeding rules.

`Lineage Unknown` Pokémon are excluded from Verified Breeding unless a future trustworthy provenance source can establish the missing relationship.

This converts the useful family-tree feature from retrospective guesswork into an app-owned, trustworthy lineage system.

Breeding mechanics themselves must be generation/game aware: Egg Groups, Ditto behavior, inheritance, abilities, natures, IVs, Egg Moves and species restrictions must follow the selected ruleset.

---

## 4. Physical link-cable Trade Station = non-goal / research only

A real GB/GBC/GBA link-cable trade partner is **not a committed product feature**.

It may remain documented as a developer/research experiment, but:

- no v1.x, v2.x or later release waits for it;
- no hardware purchase is assumed;
- no physical Game Boy/cartridge collection is required for PokeBank NX;
- emulator/save-file support remains the practical priority;
- code/protocol research may be accepted later if a contributor wants to explore it.

Physical cartridge backup/import bridges may be considered separately if useful hardware becomes available, but they are also post-v1 optional work.

---

## 5. Release discipline

The long-term feature catalog is a backlog, not a release checklist.

```text
v1.0
Safe local core + mainline game coverage

v1.x
Deeper local tools + Colosseum/XD + Stadium stretch + local/LAN exchange

v2.0
Android + Trainer Plaza foundation + Friends + Mailbox + Internet Gift/Trade

v2.x+
Richer Trainer encounters/Poké Radar/TrainerDex + Wonder/GTS/Room/public-community systems and other companion clients

Research
Physical hardware/link experiments and anything requiring proof-of-concept first
```

PokeBank NX should ship a strong 1.0 before attempting to complete every future idea.

---

## 6. Trainer Plaza supersedes Pokémon Ranch as the preferred social direction

As of 2026-09-08, the preferred Connected/community concept is:

```text
Trainer Plaza
+ PokeBank ID
+ Trainer Card / Trainer Passport
+ Partner Pokémon
+ Friends
+ Mailbox
+ Trainer Encounters
+ TrainerDex
+ Poké Radar
+ safe Gift / mandatory one-for-one Friend Trade
```

This replaces the earlier Pokémon Ranch-style concept as the primary social shell.

Why:

- it directly builds on the Friends/Mailbox/Trade infrastructure already planned for v2.0;
- Partner Pokémon gives each trainer a strong Pokémon-native visual identity;
- Trainer Cards and encounters work naturally on both Switch and Android;
- the server can remain lightweight instead of hosting full Vaults;
- social discovery can exist without exposing private Vault contents;
- the experience can start simple and expand later without blocking the local core.

The product may still include a private/local Ranch-style visualization someday as a cosmetic or collection-view feature, but it is optional and must not take priority over Trainer Plaza.

The reference inspiration is `Insektaure/NX-Plaza`, particularly its lightweight identity/pass/radar/encounter philosophy. NX-Plaza is GPL-2.0, so PokeBank NX should use a clean-room Pokémon-native implementation with its own protocol/data model rather than casually copying substantial source or assets.

Detailed design is recorded in:

```text
docs/TRAINER_PLAZA_VISION.md
```

This is future-only and must not expand the current FRLG/RSE implementation scope.
