# PokeBank NX Project Status

Last updated: **2026-09-24**

## Headline

PokeBank NX has two major foundations in place:

1. **Generation I–III shared staged editing is DEVICE ACCEPTED.**
2. **The storage/custody/transaction hardening pass is largely implemented in software on draft PR #79.**

The physically accepted Gen III editor checkpoint remains:

~~~text
Application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Status:
CI VERIFIED
DEVICE ACCEPTED
~~~

The current audit line is newer and is **not** device accepted.

## Current audit line

~~~text
PR #79 — OPEN / DRAFT / NOT MERGED
Branch: audit/full-project-hardening-20260923
Head: 59ced7c81db457ce4e59cd8b15268d6a2296537d

Host Tests:
35961226077 / #1124 / SUCCESS

Native Validation:
35961222076 / #33 / SUCCESS
~~~

## Completed or substantially hardened

- durable verified Bank replacement and preserved recovery generations;
- custody-safe held Pokémon rollback;
- immutable held source representation + separate destination conversion candidate;
- profile/account + exact-game mutable workspace namespacing;
- durable supported single-file workspace persistence;
- versioned SHA-256 Move transaction journal;
- crash/idempotent recovery and conflict refusal;
- production Bank ↔ PokeBank-workspace true-Move integration for supported single-file routes;
- startup transaction recovery and mutation locking;
- bidirectional Moves serialized as separate committed transactions;
- strict continued exclusion of installed-game / RetroArch / other emulator writes.

## Current limitations

- physical Switch power-loss recovery has not yet accepted A04;
- cross-game true Move is disabled pending conversion fidelity fixtures;
- BDSP true Move is disabled pending a recoverable multi-file save generation;
- N06 directory-copy promotion remains a separate durability issue;
- conversion findings F05–F13 still need exact-current golden fixtures;
- parser hardening remains incomplete across all families;
- Master Vault is not yet ready to become authoritative storage.

## Current phase

The project remains in **audit / durability / conversion validation**, but the center of gravity has moved.

The next software task is no longer basic Bank durability. It is:

**prove generation-to-generation conversion behavior before permitting irreversible cross-game source retirement.**

Main tracker: issue #69.

## Critical path

~~~text
Gen I–III shared editor             DEVICE ACCEPTED
        ↓
A01–A09 storage/custody hardening   MOSTLY IMPLEMENTED
        ↓
F05–F13 conversion fidelity audit   NEXT SOFTWARE TRANCHE
        ↓
parser hardening + hardware recovery
        ↓
Master Vault + named Banks
        ↓
universal SaveSource adapters
        ↓
DS / 3DS
        ↓
broader modern Switch validation
        ↓
legality / provenance / transfer tooling
        ↓
approved source-write adapters
        ↓
v1.0
~~~

## Permanent architecture rules

### One shared editor

~~~text
shared editor UI/lifecycle
+ exact-game capabilities/provider
+ generation-native staged adapter
~~~

### Source safety

~~~text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
~~~

### Move semantics

Normal Move means one logical Pokémon changes active location. Destination verification happens before source retirement.

Copy/Exact Clone/Derived Clone remain explicit operations and must never be accidental side effects of Move.

### Device acceptance

DEVICE ACCEPTED applies only to the exact artifact physically accepted by the owner. It does not automatically transfer to later SHAs.
