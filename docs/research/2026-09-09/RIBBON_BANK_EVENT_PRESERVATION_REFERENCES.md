# Ribbon / Bank-exclusive / DNS-event preservation references — 2026-09-09

> Research/planning only. These sources do not change the active FRLG milestone and are not automatically authoritative legality data.

This note preserves three useful community references found during the September 9 research session:

- Hallowed Tower DNS Exploit guide
- Athis' Ribbon Handbook
- r/PokemonHome Pokémon Bank Exclusives Masterpost

## 1. Hallowed Tower DNS Exploit guide

Source:
https://hallowedtower.net/pages/guides/dnsExploit

Useful for:

- identifying historical Gen IV/V Mystery Gift distributions preserved through community DNS services;
- documenting which gifts community servers currently re-deliver;
- EventDex / preservation research;
- distinguishing official historical distributions from community re-delivery and explicitly unofficial/unreleased gifts.

Important observations from the reviewed page:

- the guide says Nintendo Wi-Fi Connection shut down for DS on May 20, 2014;
- it documents a fan DNS/Wiimmfi-style route for Gen IV/V online/Mystery Gift functionality;
- the current guide notes the server now returns random gifts rather than date-selected gifts;
- its gift tables contain both historical official distributions and entries explicitly marked `Unofficial`, including unreleased material such as Azure Flute / Pokétch apps in some tables.

PokeBank provenance rule:

```text
historical official distribution identity
!=
community DNS re-delivery observed by PokeBank
!=
local materialization from archived event data
!=
unreleased/unofficial distribution
```

The DNS guide is therefore useful as a **preservation/catalogue reference**, but it must not be imported blindly as a legality or official-redemption oracle.

## 2. Athis' Ribbon Handbook

Source:
https://sites.google.com/view/athis-ribbon-handbook/home

The handbook is a strong future reference for PokeBank's planned Ribbon Master Mode because it organizes the practical cross-generation ribbon journey and has already been updated around the Pokémon Bank shutdown deadline.

Its Emergency Prep page specifically calls out:

- Bank shutdown as a hard deadline for official legacy-to-HOME movement;
- practical Gen III/IV contest and Battle Tower planning;
- ribbons that can be delayed to later games vs ones that are safer to obtain before transfer;
- the **Footprint Ribbon** as a good example of a ribbon whose acquisition condition changes across generations and can become missable depending on transfer timing;
- community practice for continuing Ribbon Master challenges after Bank closes.

PokeBank should treat this as a **human workflow/planning reference**, then validate exact ribbon availability/conditions against game data, PKHeX/PKSM and primary research before encoding rules.

Potential future Ribbon Master features informed by this source:

```text
Ribbon checklist per Pokémon
current obtainable ribbons
future-destination availability
missable-before-transfer warnings
recommended route / next game
Battle Tower / Contest planning notes
Bank-shutdown preservation warning
ribbon provenance by game/generation
```

## 3. Pokémon Bank Exclusives Masterpost

Source:
https://www.reddit.com/r/PokemonHome/comments/1apiuee/pok%C3%A9mon_bank_exclusives_masterpost/

This community masterpost is useful as a **preservation/checklist lead generator** for things that may become harder/impossible to acquire after Bank closes. It organizes material by generation and calls out categories such as:

```text
origin marks / generation-specific provenance
boxable forms / gender variants / regional variants
notable shinies
notable in-game gifts/trades
HOME challenges
legacy or removed moves
abilities
ribbons
ball combinations
special notes / event-only cases
```

Examples in the reviewed post include legacy ribbons from Gen III/IV/VII, removed moves, special Demo gifts, Battle Bond Greninja, GameCube Earth/National Ribbon material, and DNS-preserved events.

However, this remains a Reddit/community source. Some entries explicitly contain uncertainty or community corrections, and some claims depend on current game/HOME behavior.

Therefore classify it as:

```text
REFERENCE / CHECKLIST INPUT
not
PRIMARY LEGALITY ORACLE
```

Every hard rule promoted into PokeBank should be independently verified.

## Combined product value

These three references fit naturally into future PokeBank subsystems:

```text
Event Library / EventDex
    <- DNS preservation tables + verified event sources

Ribbon Master Mode
    <- Athis workflow guidance + validated ribbon rules

Legacy Preservation / What am I missing?
    <- Bank-exclusive checklist + verified game/HOME data
```

They also strengthen a future `Legacy Preservation` view that can tell a user, before Bank closes, which owned Pokémon/ribbons/origin combinations are worth moving or documenting.

## Bank deadline

The community pages should not be our authoritative date source. The official Pokémon page independently confirms that Pokémon Bank service ends February 25, 2027 at 7:00 p.m. PST, after which Bank -> HOME transfer is no longer possible.

Official source:
https://www.pokemon.com/us/pokemon-video-games/pokemon-bank/
