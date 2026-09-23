# PokeBank NX — Pokémon Identity, Custody, Move and Clone Contract

Status: architecture contract for issue #69 / future Master Vault  
Frozen: 2026-09-23

This document defines product semantics. It does **not** enable live source writes.

## Core invariant

```text
ONE logical Pokémon identity
ONE active location during normal ownership
MANY immutable provenance/history records
```

A historical raw payload is evidence. It is not automatically another active Pokémon.

## Identity layers

PokeBank NX must keep these concepts separate:

1. **Vault/entity identity** — stable PokeBank-owned ID for one logical Pokémon.
2. **Native-format identity** — generation-specific bytes/identity fields such as DVs, PID, EC, etc.
3. **Active location** — the one place from which the logical Pokémon can currently be withdrawn/used.
4. **Archival representation** — immutable historical bytes/hash/provenance from an earlier location or format.

A normal cross-generation Move may require a new native-format representation. That does **not** create a second logical Pokémon. The same Vault/entity identity advances to the destination representation while the prior bytes remain archival history.

## MOVE

Move means the **same logical Pokémon changes active location**.

Conceptual transaction:

```text
retain active source
-> derive destination representation if conversion is required
-> validate destination candidate
-> durably write destination
-> read back destination
-> verify exact required semantics
-> mark destination active
-> retire source active location
-> commit journal
```

If anything fails before source retirement, the source remains active.

If destination is verified but source retirement cannot complete, enter a recoverable transaction state. Never silently resolve this by deleting either side or treating both as permanently active.

After:

```text
Game A -> PokeBank
Game A: no active Pokémon
PokeBank: active Pokémon

PokeBank -> Game B
PokeBank: no withdrawable active Pokémon
Game B: active Pokémon
PokeBank: retains Pokédex/provenance/archive only
```

## COPY

Copy, if offered, is an explicit duplication command. It never happens as a side effect of Move.

A Copy creates another active logical entity and therefore requires a new PokeBank/Vault entity ID plus a provenance relation such as `COPIED_FROM`.

If the destination format differs, the copied descendant may require conversion. Any loss/change must be disclosed.

## EXACT CLONE

Exact Clone is an explicit user-requested duplication.

At the instant of cloning:

- the native Pokémon payload is byte-for-byte identical wherever the source format permits;
- the clone receives a new Vault/entity ID;
- provenance records `EXACT_CLONE_OF <parent>`;
- both entities may be active independently;
- no native field is deliberately regenerated merely to make the clone different.

Example:

```text
V0001 Pikachu — ORIGINAL
└── V0002 Pikachu — EXACT_CLONE_OF V0001

native payload(V0002) == native payload(V0001)
Vault ID(V0002) != Vault ID(V0001)
```

Exact Clone requires explicit confirmation. Normal Move must never invoke it.

## DERIVED / LEGAL CLONE

Derived Clone creates a **new related individual**, not a claim that another historical encounter occurred.

Preferred product wording is **Derived Clone** or **Legal Clone**. Do not use provenance wording that falsely claims the generated descendant was legitimately encountered in history.

A Derived Clone:

- receives a new Vault/entity ID;
- records `DERIVED_FROM <parent>`;
- preserves user-requested traits where the target format/rules allow;
- regenerates identity-linked values where necessary;
- applies generation-aware legality/compatibility rules;
- records every intentional change/loss.

Gen I/II do not have Gen III+ PID/EC semantics. Derivation must use the identity mechanics native to the exact target format.

## CONVERSION IS NOT CLONING

Conversion during a normal Move creates a destination **representation** of the same logical Pokémon.

```text
V0001 active representation: PK3
Move to later game
V0001 archived representation: PK3
V0001 active representation: PK8/PK9/etc.
```

There is still one active entity.

If conversion cannot preserve required semantics, the Move fails explicitly. Do not silently substitute a fallback PID/form/gender/shiny/ability/nickname state.

## ARCHIVE / PROVENANCE

Archival evidence may retain:

- exact original bytes;
- SHA-256;
- origin game/platform/save/profile/slot;
- prior active locations;
- conversion inputs/outputs;
- editor derivation events;
- parent/clone relationships;
- transaction IDs;
- validation/legality results.

Archived evidence:

- is not shown as another withdrawable Pokémon;
- does not count as a second active collection entity;
- cannot be transferred independently unless the user explicitly creates a new entity from it through a Clone/Restore operation with provenance.

## Pokédex

Pokédex/collection registration is historical ownership evidence, not active custody.

Moving a Pokémon out of PokeBank may leave its Pokédex entry and provenance intact while removing the active/withdrawable Bank entity.

## Required provenance event classes

At minimum:

```text
ORIGINAL_IMPORT
MOVE
COPY
EXACT_CLONE
DERIVED_CLONE
CONVERT
EDIT
EXPORT
ARCHIVE
RESTORE
```

A Restore from archival evidence must never masquerade as a normal Move; it creates/reactivates an entity only through an explicit recovery policy and must be journaled.

## Transaction states

Future true Move should support a crash-recoverable state machine equivalent to:

```text
PREPARED
DESTINATION_WRITTEN
DESTINATION_VERIFIED
SOURCE_RETIRE_PENDING
SOURCE_RETIRED
COMMITTED
```

Recovery must be idempotent from every state.

## Current implementation boundary

The current inherited/legacy Storage and staged box movement do **not** yet satisfy this complete contract.

Therefore:

- legacy `Move` UI must not be treated as approved true Move;
- current Clone UI must not automatically be treated as final Vault Exact Clone;
- Master Vault must not become authoritative until custody + durable transaction requirements pass issue #69;
- live installed-game/RetroArch/other-emulator writes remain HARD DISABLED.
