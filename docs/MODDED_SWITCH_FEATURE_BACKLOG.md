# PokeBank NX — Modded Switch Feature Backlog

> **Future-only product backlog.** This file does not expand the current FRLG/RSE implementation task. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active coding work.

## Product position

PokeBank NX should aim to become the **Pokémon hub / operating system for a modded Nintendo Switch**, not merely a free clone of Pokémon HOME.

Its durable advantages should be local ownership, broad game/save-source support, collection intelligence, safe save tooling, provenance, offline operation, user-controlled export/backup, and an optional social layer.

The following existing features remain non-negotiable and are NOT replaced by this backlog:

- guided **Create Pokémon** flow;
- **Quick Legal** creation;
- advanced PKHeX-style controller-friendly editing;
- legality-aware editing and Make Shiny safeguards;
- explicit provenance for created/generated Pokémon;
- Event / Mystery Gift workflows;
- local Event Library / EventDex;
- ability to materialize a historically defined event/gift into the PokeBank local ecosystem without requiring official Pokémon HOME or Nintendo/Pokémon online services, while labeling its provenance honestly;
- normal local Vault/save workflows remain independent of official HOME compatibility.

A generated/restored event must never falsely claim it was actually received from an official live distribution server when it was created from archived event-definition data. Legality and historical provenance are separate concepts.

---

# 29 high-value modded-Switch features

## 1. Unlimited local Vault

Use local storage as the practical capacity limit. Support profile-scoped Vaults plus unlimited named Banks such as Living Dex, Shiny Dex, Events, Ribbon Masters, Competitive, Trade Extras, and user-defined collections.

## 2. Universal Save Scanner

Automatically discover supported Pokémon saves across installed Switch games and approved emulator/backup providers such as RetroArch, DS/3DS/GameCube emulators, JKSV/Checkpoint-style backups, and manual sources. Show truthful source/provider/profile metadata and never guess ownership.

## 3. Pokémon Rescue Mode / Save Health

Inspect supported saves for structural/checksum/slot damage, report health clearly, and offer previewable recovery only through verified safe paths. Never silently repair or overwrite a live source.

## 4. Save Time Machine

Automatic pre-write snapshots, manual snapshots, named checkpoints, diff views, and safe restore. Compare Party, Boxes, Items, Trainer data, Dex state, and other supported structures.

## 5. Pokémon Journey / Provenance Timeline

Track the life of a Pokémon across PokeBank-observed imports, transfers, trades, gifts, edits, breeding, and game destinations. Separate game-reported history, PokeBank-observed history, and unknown/imported history. Never invent missing history.

## 6. Transfer Lab

Preview a transfer before committing it: destination compatibility, forms, abilities, moves, held items, ribbons/marks, irreversible changes, legality impact, and resulting representation.

## 7. Universal Compatibility Map

For any selected Pokémon, show every supported destination game and whether transfer is allowed, conditional, lossy, or impossible, with an explanation.

## 8. Offline Legality Engine

Provide `LEGAL / INVALID / UNKNOWN` with human-readable explanations, generation-aware encounter checks, and optional safe suggestions. Core legality use should not require a remote server.

## 9. Living Dex Brain

Track National/game Living Dex completion, forms, shinies, genders/variants where useful, and answer `What am I missing?` using the user's actual Vault and discovered saves.

## 10. Evolution Planner

Use owned Pokémon and duplicates to recommend efficient paths for filling missing evolutions/forms, while respecting game-specific evolution rules.

## 11. Duplicate Detective

Find exact payload duplicates, likely copied instances, same-species extras, potential clones, and review candidates. Never auto-delete; provide safe review/organize actions.

## 12. Event Library / EventDex

Maintain an offline catalog of historical Event/Mystery Gift distributions, ownership tracking, legality/provenance metadata, and local materialization/import workflows where technically supported.

This feature must remain useful if official distribution services or Pokémon HOME are unavailable. Event materialization from archived definitions must be labeled honestly and must not forge a claim that an official live server delivered it.

## 13. Ribbon Master Mode

Track ribbons/marks a Pokémon has, which are still obtainable, destination/game requirements, and recommended next steps using the user's available saves.

## 14. Shiny Hunt Manager

Optional hunt journal for species/game/method/encounters/eggs/resets, with integration into the Vault when a shiny is eventually imported.

## 15. Team Builder From My Vault

Build teams from Pokémon the user actually owns. Surface the best matching Vault candidates and indicate missing team members.

## 16. Bulk Box / Bank Tools

Previewable organization by National Dex, generation, type, origin, shiny, form, event, ribbon count, alphabetical order, etc. Include compact boxes, Living Dex layouts, Shiny Dex layouts, and review-bank workflows.

## 17. Powerful Cross-Save Search

Search the Vault and supported discovered saves together with rich filters such as species, shiny, gender, origin game, generation, nature, ribbons, marks, legality, provenance, and Bank.

## 18. Verified Breeding Family Trees

Keep the existing `FOUNDER VERIFIED`, `LINEAGE VERIFIED`, and `LINEAGE UNKNOWN` model. Record true PokeBank-witnessed parent Vault IDs and provide navigable family trees without inventing historical parents.

## 19. Trainer Passport

Create a local/social profile with Partner Pokémon, collection stats, Dex completion, oldest Pokémon, games connected, medals, trades, gifts, and other user-selected public/private stats.

## 20. Trainer Plaza

Preferred Connected social shell: Partner Pokémon, Trainer Cards, Friends, Mailbox, TrainerDex, Poké Radar, encounters, safe Gifts, and mandatory one-for-one Friend Trades. See `docs/TRAINER_PLAZA_VISION.md`.

## 21. LAN Mode Without PokeBank Servers

Two supported devices on the same local network should eventually be able to pair, exchange Trainer Cards, add friends, compare selected public collection stats, and perform safe Gift/Trade transactions without the Internet relay.

## 22. QR Everything + Human-Enterable Codes

QR is a convenience layer, not a dependency.

Where practical provide QR representations for pairing, friend identity, trade requests, Trainer Cards, teams, filters, and companion setup.

**Every essential social pairing flow must also have a short human-enterable code path**, especially PokeBank friend IDs, so two people standing together can add each other using only their Switches. The Android companion must never be required merely to add a friend.

## 23. Android Companion / Remote Planning

Later companion features may browse a safe Vault index, plan transfers, manage Banks, build teams, inspect Dex progress, review Mailbox/Gifts/Trades, and queue actions for the Switch. The phone should not bypass Switch-side transaction safety.

## 24. Full Export / No Lock-In

Provide a documented export of Pokémon payloads, originals, provenance, Banks, Dex state, profiles, snapshots, and manifests so users retain their data even if PokeBank NX development or servers stop.

## 25. Automatic Encrypted Backup

Support optional backup targets such as local SD/USB, LAN PC, Android, NAS, WebDAV/SFTP or self-hosted targets where practical. Networking is optional; local use must remain independent.

## 26. Disaster Recovery Center

Provide a guided recovery view for Vault/backups/snapshots, showing what is recoverable before changing anything.

## 27. Automatic Updates

Support stable/beta/tester/nightly channels where appropriate, and independently update app/game definitions/legality/event/compatibility/artwork data when architecture allows. Updates must be verifiable and rollback-safe.

## 28. Source Provider / Plugin System

Keep source discovery modular so RetroArch, DS/3DS/GameCube emulators, backup managers, manual imports, and future community providers can be added without rewriting the application core. Provider trust/write capabilities remain explicit.

## 29. Offline-First Guarantee

Without Internet, users should still retain the core product:

```text
Vault             YES
Banks             YES
Save Browser      YES
Dex               YES
Legality          YES
Create/Edit       YES
Event Library     YES
Transfer Lab      YES
Save Backups      YES
Living Dex        YES
Trainer Stats     YES
```

Only genuinely network-dependent features such as Internet Friends presence, remote Mailbox delivery, Internet Trainer encounters, and Internet Gift/Trade should become unavailable.

---

# Create Pokémon / PKHeX-style editor policy

This remains a core PokeBank NX feature and must not be dropped while adding collection/social features.

Target flow:

```text
Create Pokémon
  -> choose destination game/generation/ruleset
  -> required fields first
  -> optional/advanced fields
  -> real-time legality feedback
  -> Quick Legal or Advanced mode
  -> explicit Generated/Created provenance
  -> stage into Vault or approved destination representation
```

Creation must be controller-friendly, game-aware, and transparent about what PokeBank generated versus what came from a real source save/event.

The editor should ultimately support the useful PKHeX-style fields appropriate to each generation while enforcing PokeBank's save-safety and provenance rules.

---

# Event / Mystery Gift independence policy

PokeBank NX's Event Library is intended as a preservation/local-use feature, not a fake connection to official distribution infrastructure.

A historical event definition may be used to create/import the corresponding event Pokémon or gift representation into PokeBank NX when supported, even if that Pokémon can never be sent through official Pokémon HOME.

PokeBank should record something equivalent to:

```text
Event definition: <distribution identity>
Source: PokeBank Event Library / archived distribution data
Official live redemption witnessed by PokeBank: NO
Legality result: <LEGAL / INVALID / UNKNOWN>
PokeBank provenance: generated/restored locally from event definition
```

Do not falsify server redemption history, dates, HOME trackers, Nintendo account data, or unavailable official provenance.

---

# Friend-code policy

PokeBank IDs must be designed for both QR and manual entry.

Requirements:

- short enough to read/type comfortably with a controller;
- case-insensitive where feasible;
- avoid ambiguous characters such as `0/O` and `1/I/l` where practical;
- include a checksum/check digit or equivalent typo detection;
- QR encodes the same public identity/pairing information rather than a secret credential;
- manual code entry remains fully supported even before the Android companion exists;
- local/LAN pairing may optionally confirm a nearby device after code entry;
- never expose private authentication tokens in a friend code.

Example style only (final format TBD):

```text
PBX-K4M-92X7
```

---

# Scope discipline

These features form the long-term backlog. They must not be allowed to derail the current release sequence.

```text
current
FRLG physical acceptance

then
RSE and mainline save/Vault roadmap

later
creation/events/transfer depth + v1.x tools

then
PokeBank NX Connected / Trainer Plaza / Android
```

The backlog is ambitious by design, but each release should ship useful completed slices rather than waiting for all 29 ideas at once.
