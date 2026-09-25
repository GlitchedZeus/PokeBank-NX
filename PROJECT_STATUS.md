# PokeBank NX Project Status

Last updated: **2026-09-25**

## Headline

PokeBank NX now has three major foundations in place:

1. **Generation I–III shared staged editing is DEVICE ACCEPTED.**
2. **Storage/custody/transaction hardening is largely implemented in software.**
3. **The broad conversion-fidelity foundation is substantially complete, including persisted loss/provenance evidence and a full SWSH ↔ S/V exact-pair audit.**

The physically accepted Gen I–III editor checkpoint remains:

~~~text
Application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Status:
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
~~~

Later audit heads are newer and **not device accepted**.

## Current audit line

~~~text
PR #79 — OPEN / DRAFT / NOT MERGED
Branch: audit/full-project-hardening-20260923

Head:
be39c6f1eb422a3b5410ddaa9004294e1e709650

Tree:
e3ac245fafde113e472d441cb4961a9e0cc3cb5d

Host Tests:
36103080464 / #1183 / SUCCESS

Native Validation:
36103077129 / #84 / SUCCESS
~~~

## Completed or substantially hardened

- durable verified Bank replacement and preserved recovery generations;
- custody-safe held Pokémon rollback;
- immutable source representation + separate destination conversion candidate;
- profile/account + exact-game mutable workspace namespacing;
- durable supported single-file workspace persistence;
- versioned SHA-256 Move transaction journal;
- crash/idempotent recovery and conflict refusal;
- production Bank ↔ PokeBank-workspace true-Move integration for supported single-file routes;
- startup transaction recovery and mutation locking;
- explicit Loss / Adaptation conversion reporting;
- production conversion preflight;
- persisted PBCE conversion evidence;
- explicit user acknowledgement binding for future loss-bearing source retirement;
- F13 historical-origin / current-location provenance separation;
- F05–F13 production golden-fixture foundation;
- route-level modern conversion corpus;
- exact Sword/Shield ↔ Scarlet/Violet title-pair audit.

## Current limitations

- **all cross-game true-Move routes remain disabled**;
- SWSH↔SV still has closure blockers around unknown/reserved PK8/PK9 bytes, events, ribbons/marks, special forms/balls and text boundaries;
- physical Switch FAT32/exFAT power-loss recovery is not yet accepted;
- malformed/truncated parser hardening remains incomplete across some families;
- BDSP true Move remains disabled pending a recoverable two-file save generation;
- N06 directory-generation durability remains open;
- Master Vault is not yet authoritative storage.

## Current phase

The project remains in **audit / durability / conversion validation**, but it is now in the later part of that phase.

The broad question is no longer:

> Can PokeBank NX convert between these formats at all?

The current question is:

> Have we closed every meaningful semantic and recovery gap strongly enough to permit one exact route to retire the source?

For Sword/Shield ↔ Scarlet/Violet, the answer is **not yet**. All eight exact directions remain disabled, but the remaining blockers are now specific and narrow enough to attack directly.

## How much audit work remains?

For the **core safety audit before Master Vault / broader expansion**, the remaining work is concentrated into four major buckets:

1. PK8/PK9 unknown/reserved-byte + event/ribbon/mark closure.
2. Remaining malformed/truncated parser hardening.
3. Physical Switch FAT32/exFAT interruption/recovery acceptance.
4. BDSP two-file durability + N06 directory-generation decision/implementation.

After those, the project should move away from one giant audit and into **route-by-route qualification**, where each exact conversion route is either proven and considered for enablement or left fail-closed.

That means the project is much closer to the end of the broad audit than the beginning, but there is still meaningful proof work before source-retiring cross-game Move should be turned on.

## Critical path

~~~text
Gen I–III shared editor                     DEVICE ACCEPTED
        ↓
storage / custody / transaction safety       SOFTWARE FOUNDATION BUILT
        ↓
conversion fidelity / provenance foundation  SUBSTANTIALLY COMPLETE
        ↓
PK8/PK9 closure + parser/recovery gates       CURRENT
        ↓
exact-route enablement decisions
        ↓
Master Vault + named Banks
        ↓
SaveSource / DS / 3DS / modern expansion
        ↓
legality / provenance / approved writes
        ↓
release hardening
        ↓
v1.0
~~~

## Permanent architecture rules

### Source safety

~~~text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
~~~

### Move semantics

Normal Move means one logical Pokémon changes active location. Destination verification happens before source retirement.

Copy / Exact Clone / Derived Clone remain explicit operations and must never be accidental side effects of Move.

### Device acceptance

DEVICE ACCEPTED applies only to the exact artifact physically accepted by the owner. It does not automatically transfer to later SHAs.
