# PokeBank NX — Feature Feasibility Matrix

> **Future-only / scope-control document.** This is an engineering reality check, not an active implementation prompt. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for current work.

The long-term feature backlog mixes straightforward application work, large but realistic engineering projects, and a few research ideas that should never be promised as guaranteed product features.

## Legend

- **GREEN — Clearly feasible:** known technology/data model; normal engineering effort.
- **YELLOW — Feasible but substantial:** technically realistic, but needs major data, platform, security, compatibility, or maintenance work.
- **ORANGE — Research / partial:** plausible, but hardware/protocol/data limitations mean scope must stay experimental until proven.
- **RED — Do not promise:** impossible to guarantee from the information/control PokeBank NX actually has.

---

# Release discipline

PokeBank NX must not wait for this entire catalog before shipping 1.0.

## v1.0 target philosophy

`1.0 = safe, useful, locally owned Pokémon Vault + supported save integration`, not `every dream feature finished`.

Good v1.0 targets:

- profile-scoped local Vaults and Banks;
- multiple save instances and source providers;
- reliable read/import support for a documented game set;
- Pokémon Summary / Judge-style data;
- provenance;
- search/filter/sort;
- basic National/game Dex tracking;
- legality status;
- safe COPY/import/export semantics;
- staged backups and individually approved write adapters;
- recovery/integrity tools;
- reproducible physical-tested releases.

Internet trading, Android, public GTS/Wonder services, hardware cartridge bridges, battle analytics, medals, and every possible emulator/game are **not required for 1.0**.

---

# Full feature audit

| # | Feature | Feasibility | Engineering reality / limitation | Suggested era |
|---|---|---|---|---|
| 1 | Profile Vaults | **GREEN** | Local per-Nintendo-profile directories, ownership rules, Banks/history/settings are ordinary application architecture. | v1.0 |
| 2 | Universal game/save library | **GREEN** | We can represent many games, providers, and multiple independent saves of the same game. Parser coverage is the time-consuming part. | v1.x |
| 3 | Source Provider Registry | **GREEN** | Data-driven roots/extensions/provider metadata are straightforward. Unknown providers can safely fall back to Manual/Other. | v1.0/v1.x |
| 4 | National/regional/game Dex Hub | **GREEN** | Mostly species/form/game datasets plus Vault indexing. Large dataset, not a technical blocker. | v1.0/v1.x |
| 5 | Collection Intelligence / “what am I missing?” | **YELLOW** | Very feasible if we maintain accurate encounter/evolution/version-exclusive datasets. Bad data would produce bad recommendations, so correctness matters. | v1.x |
| 6 | Living Dex Planner | **GREEN** | Vault queries + species/form requirements + progress tracking. | v1.x |
| 7 | PokeBank Analyzer / Judge | **GREEN** | IV/EV/nature/PID/EC/met/ribbon/etc. interpretation is known for supported formats. Legality explanations depend on legality data. | v1.0/v1.x |
| 8 | Guided Create / Quick Legal | **GREEN** | Feasible with generation-aware field schemas and legality templates. The hard part is building/maintaining complete encounter rules, not the wizard UI. | v1.x |
| 9 | Advanced controller-friendly editor | **YELLOW** | Clearly possible, but every generation has different fields/rules. Broad, polished coverage is a major project. | v1.x |
| 10 | Transfer Lab / conversion preview | **YELLOW** | Cross-generation conversion is proven possible, but exact destination behavior, irreversible fields, forms, moves, ribbons, and modern rules require extensive per-generation testing. | v1.x |
| 11 | Per-game moveset memory | **YELLOW** | App-side moveset histories are easy; perfectly matching every modern official transfer behavior requires careful compatibility research. | v1.x |
| 12 | Deep Pokémon Journey / provenance | **GREEN** | PokeBank can reliably record events that happen after import. Historical data that predates PokeBank can only use what source data actually contains. | v1.0/v1.x |
| 13 | Catch Calendar / Memories | **GREEN** | Straightforward when met/catch dates or PokeBank transaction dates are available. Older games without dates cannot magically provide them. | v1.x |
| 14 | Save Time Machine | **GREEN** | Snapshot/backup/journal architecture is standard and aligns with our mandatory safe-write model. Storage management is the main concern. | v1.x |
| 15 | Save comparison | **GREEN** | Parse two known snapshots and diff trainer/Party/Boxes/items/Dex. Requires per-game diff adapters for best presentation. | v1.x |
| 16 | Duplicate Detective | **YELLOW** | Exact byte duplicates are easy. “Likely clone” can only be a heuristic; independent Pokémon can share many fields. Never claim mathematical certainty without evidence. | v1.x |
| 17 | Vault Integrity Center | **GREEN** | Hashes, journals, indexes, orphan detection, recovery state and validation are normal database/storage work. | v1.0/v1.x |
| 18 | Save Health Center | **GREEN** | Checksums/slot state/parser results/backup age/source metadata can be exposed for each supported adapter. | v1.0/v1.x |
| 19 | Explicit COPY / MOVE / CLONE | **GREEN** | Transaction semantics are completely under our control. MOVE must remain staged until destination verification succeeds. | v1.0/v1.x |
| 20 | Historical Event / Mystery Gift Library | **YELLOW** | Technically feasible; the challenge is curating verified distribution data/payloads, licensing/redistribution, region/language variants, and provenance wording. | post-v1 |
| 21 | EventDex / GiftDex | **GREEN** | Easy once the event catalog exists: checklist/index/provenance states. | post-v1 |
| 22 | Wonder Card / Mystery Gift archive and import | **YELLOW** | Reading/archiving known gift formats is realistic. Writing/importing must be implemented and validated per game/generation. | post-v1 |
| 23 | PokeBank Friends / IDs | **GREEN** | Public-key identity + short friend code + request database is standard networking/application work. | v2 |
| 24 | Mailbox | **GREEN** | Friend requests, gifts, trades and system messages are ordinary queued records. | v2 |
| 25 | Gift to Friend | **GREEN** | Vault-to-Vault staged ownership transfer with explicit recipient acceptance is technically straightforward. | v2 |
| 26 | Real Friend Trade | **GREEN** | Two-sided offer, confirmation, transaction state and provenance are completely feasible. Must design crash/retry semantics carefully. | v2 |
| 27 | Same-Switch trading | **GREEN** | Easiest trade transport: two local profile Vaults, no network required. | v1.x |
| 28 | Nearby / LAN trading | **GREEN with caveat** | TCP/UDP networking is available to Switch homebrew. Direct QR/code/address pairing is realistic. Seamless multicast/mDNS discovery may be less reliable, so discovery should not depend on it. | v1.x/v2 |
| 29 | Internet Friend trading | **YELLOW** | Technically normal HTTPS/WebSocket/server work, but requires real security engineering, key recovery/revocation, abuse handling, server maintenance and uptime. | v2 |
| 30 | Wonder Exchange | **YELLOW** | Random asynchronous exchange is easy technically; operating a public pool adds cheating, spam, legality-policy, abuse and moderation problems. | v2+ |
| 31 | Trade Board / GTS-style listings | **YELLOW** | Search/listing backend is straightforward. Public service quality, fraud/abuse controls, generated-Pokémon disclosure, moderation and hosting are the real work. | v2+ |
| 32 | Link / Room Trade | **YELLOW** | Feasible real-time server/session design, but multiplayer state, disconnects, fairness and public-room moderation raise complexity. | v2+ |
| 33 | Trainer Passport / profile card | **GREEN** | Pure local/profile data and cosmetics. | v1.x |
| 34 | Challenges / achievements | **GREEN** | Rules evaluated against Vault/Dex/provenance state. | v1.x |
| 35 | Achievement cosmetic rewards | **GREEN** | App-side themes/titles/badges are under our control. Asset licensing still applies. | v1.x |
| 36 | Vault Points | **GREEN** | Local points/achievement currency is trivial technically. Keep it app-side rather than pretending to award official in-game currencies. | v1.x |
| 37 | Collector Medals | **GREEN** | App-owned collectible records/profile decorations are easy. Clearly distinguish them from official HOME medals. | v1.x/v2 |
| 38 | Dex completion rewards | **GREEN** | App-side cosmetics/titles/badges can unlock from completion state. | v1.x |
| 39 | Battle Lab / usage data | **YELLOW** | Analysis of the user's own teams is easy. Global/current usage statistics require a reliable external/community data source or our own population, plus ongoing updates. | v2+ |
| 40 | Team Builder | **GREEN** | Type/stat/move/ruleset analysis and Vault candidate search are straightforward with good game data. | v1.x |
| 41 | Team deployment to a game | **YELLOW** | The team builder is easy; writing six entities safely requires destination-specific write adapters, backups and post-write verification. | v1.x+ |
| 42 | Breeding Planner | **GREEN** | Generation-aware egg groups/inheritance/egg-move/IV rules can be data driven. | v1.x |
| 43 | Evolution Planner | **GREEN** | Evolution requirements and “which owned game can do this?” are data/query problems. | v1.x |
| 44 | Ribbon Master Planner | **YELLOW** | Feasible, but accurate cross-generation ribbon availability/transfer paths are a large curated dataset with edge cases. | v1.x/post-v1 |
| 45 | Shiny Hunter tools | **GREEN** | Counters, targets, odds, phases and collection tracking are ordinary local tools. Game-specific RNG automation would be a separate scope. | v1.x |
| 46 | Pokémon comparison | **GREEN** | Side-by-side normalized data comparison is straightforward. | v1.x |
| 47 | Smart Banks / advanced search | **GREEN** | Query/index architecture can support dynamic collections and rich filters. | v1.0/v1.x |
| 48 | Pokémon lineage / family tree | **ORANGE / PARTIAL** | Existing Pokémon save records generally do **not** preserve identities of both breeding parents. We can record lineage when PokeBank witnessed/managed it, but cannot reconstruct true historical parents from old saves. | experimental |
| 49 | Android companion | **YELLOW** | Absolutely buildable, but it is a second application plus pairing, sync protocol, key management, conflict handling, notifications and release maintenance. | v2 |
| 50 | Desktop/Web/iOS companions | **YELLOW** | Technically routine once a stable protocol exists, but every client multiplies maintenance/security/UI work. | v2+ |
| 51 | Physical PokeBank NX Link — cartridge backup/restore/import | **YELLOW** | Existing hardware proves GB/GBC/GBA ROM/save backup and restore are practical. Switch homebrew also exposes USB host services, so integration is plausible, but hardware/protocol support needs a dedicated bridge project and physical testing. | post-v1/research |
| 52 | Real-hardware Trade Station / link-cable partner | **ORANGE** | Not fantasy, but a genuine hardware/protocol research project. GB serial/link behavior is documented; game-specific Pokémon trade protocols, GBA timing/electrical interfaces and reliable hardware all need proof-of-concept work before promising it. | research/stretch |
| 53 | Stadium / Colosseum / XD integration | **YELLOW** | File/save support is realistic and existing tooling handles GameCube Pokémon saves. Physical N64/GC hardware/memory-card workflows are separate harder projects. | post-v1 |
| 54 | Artwork/theme pack system | **GREEN technically** | Pack manifests/themes are easy. The limiting issue is copyright/licensing for redistributed Pokémon artwork, not code. | v1.x |
| 55 | Pokémon cries / sound packs | **YELLOW legally, GREEN technically** | Audio playback and pack support are easy; redistribution rights for official cries/audio require care. User/community packs are safer when properly licensed. | v1.x |
| 56 | Local-first forever | **GREEN** | This is an architecture choice: network features optional, local Vault/Dex/save tools remain functional without a server. | core principle |
| 57 | Export / no lock-in | **GREEN** | Export Vault metadata, Pokémon entities, Banks, histories and reports using documented formats. | v1.0/v1.x |
| 58 | Disaster recovery | **GREEN** | Journals, snapshots, checksums, rebuildable indexes and explicit rescue flows are standard reliability engineering. | v1.0/v1.x |
| 59 | Privacy/security controls | **GREEN** | Local-only mode, no telemetry by default, encryption, friend blocking, key revocation and minimal server data are all feasible design choices. | core/v2 |

---

# The actual “starting fire with water” limits

These are the places where PokeBank NX must be honest instead of pretending software can know things that the data does not contain.

## RED — Cannot prove historical legitimacy with certainty

A Pokémon's bytes may be perfectly legal and indistinguishable from a legitimately obtained specimen while having been generated later.

PokeBank NX can say:

```text
LEGAL
matches a possible encounter/event
```

or:

```text
PROVENANCE VERIFIED BY POKEBANK NX
we observed/imported/traded this record through our own history
```

But for an old imported file it cannot universally prove:

```text
THIS EXACT POKÉMON WAS REALLY CAUGHT AT THIS HISTORICAL TIME BY THIS PERSON
```

unless independent provenance exists.

## RED — Cannot make digital Pokémon impossible to clone

Users control their SD cards, backups and CFW environment. A user can back up a Vault/save, trade something, and restore an older copy.

We can provide:

- transaction integrity;
- duplicate detection;
- provenance;
- honest generated/clone markings inside PokeBank;

but we cannot create Nintendo-server-style authoritative scarcity for local files.

## RED — Cannot reconstruct missing history

If an old game/save does not store information such as:

- breeding parent identities;
- exact catch date;
- which emulator previously held it;
- prior owners that left no encoded trace;

PokeBank NX cannot recover that history from nothing.

It can begin trustworthy history **from the moment PokeBank sees the entity/source**.

## RED — Cannot identify arbitrary emulator ownership from Pokémon bytes

The save can often identify the game. It generally cannot tell us “this belonged to WILL and came from Tico.” Provider identity comes from filesystem/config/app context and profile assignment.

## RED — Do not depend on Nintendo/HOME private protocols

PokeBank NX should not promise a private HOME clone, official friend network integration, fake HOME trackers, or access to Nintendo's private services. Our networking must be our own protocol.

---

# Important feasibility evidence / engineering precedents

- **Switch networking is real:** libnx exposes BSD-style sockets and Nintendo network-interface services, so LAN/Internet PokeBank protocols are technically viable.
- **Broad Pokémon save/entity manipulation is proven:** existing open-source Pokémon tooling supports many save formats, individual Pokémon formats, Mystery Gift formats, GameCube saves, and cross-generation entity conversion. PokeBank NX still needs its own safe/native architecture and must obey dependency licenses.
- **Physical cartridge save bridging is real:** existing GB/GBC/GBA cartridge hardware can backup and restore SRAM/Flash/EEPROM saves over USB.
- **Real Game Boy link communication is documented:** serial/link-cable electrical and timing behavior is documented, which makes a future hardware trade bridge conceivable, but that is not the same as already having the game-specific Pokémon trading protocol solved.

---

# Practical priority verdict

## Build confidently

The majority of the product — Vaults, Banks, Dexes, Judge, guided creation, editor, backups, provenance, planners, same-Switch trading, LAN trading, friends/mailbox, Android companion, exports, integrity and most collection tools — is **realistic**.

## Build later and carefully

Public Internet exchange systems, advanced cross-generation conversion, historical event catalogs, global battle usage, Android/web ecosystems and physical cartridge integration are **real but expensive**.

## Prototype before promising

Real hardware trade-station behavior, seamless physical link integration and retrospective lineage reconstruction need proof-of-concepts first.

## Never promise impossible certainty

Do not claim perfect historical authenticity, impossible anti-cloning guarantees, reconstruction of data never stored, or Nintendo/HOME private-service equivalence.

This matrix should be revised whenever a prototype changes one of these confidence levels.
