# Issue #71 — inherited Move / Multi storage reuse plan

Status: architecture decision only; no new mutation path is authorized.

## Current inherited behavior found

`TrainerViewScreenBase.inc` already contains a mature PKSE-derived storage interaction model:

- `CursorMode::Menu -> Move -> Multi` cycling;
- single-slot pickup represented as a 1x1 carried block;
- rectangular Multi selection with an anchor and second-step grab;
- one carried-block representation shared by single and multi operations;
- box/pane-aware cursor movement while carrying;
- B/cancel behavior that can return or abandon an in-progress selection safely;
- copy-vs-remove distinction (`grabSelection(false/true)`);
- destination bounds checks;
- blocked/locked cells stay in the carried block rather than silently disappearing;
- save-side and Bank-side pane concepts.

This is exactly the kind of solved interaction architecture #71 exists to recover.

## Classification

**DECISION: REFACTOR INTO SHARED POKEBANK COMPONENT — DO NOT REUSE THE LEGACY MUTATION ROUTINE DIRECTLY.**

Why:

1. The interaction/state-machine behavior is valuable and already solved.
2. The current implementation is coupled to legacy `TrainerViewScreen`, `Trainer`, `Bank`, conversion and mutable-workspace paths.
3. Gen I/II accepted native save editing now has its own staged-editor/source-immutability boundary.
4. Reusing direct legacy put-down/mutation logic would risk bypassing packed-native semantics and staged transaction ownership.

## Reuse boundary

Extract/reuse only generation-neutral interaction/model concepts:

```text
Selection anchor
rectangle geometry
carried block
source pane/box/slot
cursor while carrying
cancel/return
copy vs reorder intent
destination geometry validation
```

The destination adapter owns actual mutation:

```text
Shared Move/Multi controller
        ↓
StorageOperationProvider
        ↓
exact source/destination capabilities
        ↓
generation-native staged transaction OR PokeBank-owned Bank transaction
```

## Required operation vocabulary

Avoid overloading cross-game destructive `MOVE` terminology.

Use separate semantic operations:

- `ReorderSlot` — rearrange within the same native staged container;
- `ReorderBlock` — rectangular in-container organization where format supports it;
- `CopyBlock` — non-destructive duplicate into a compatible destination;
- `TransferCopy` — future cross-source copy handled by transfer engine;
- `TransferMove` — future destructive cross-source operation, separately gated and **not** the storage cursor mode.

## Packed-native rule

Gen I and Gen II boxes are packed lists. Their staged reorder provider must operate in terms of logical occupied-order semantics rather than treating visual empty cells as persistent sparse slots.

PokeBank-owned Banks/Vault views may use sparse/reference semantics independently.

The shared Move/Multi controller therefore must query `StorageSemantics` and never assume all grids support the same put-down algorithm.

## P0/P1 sequencing

### P0

- define `StorageSemantics` and operation/capability descriptors;
- freeze Move/Multi model types without routing them into accepted Gen I/II product screens;
- tests prove no legacy direct source mutation is enabled.

### P1 / separate organization milestone

- extract/refactor the carry/rectangle controller from inherited behavior;
- connect it to generation-native staged reorder providers;
- separately connect PokeBank-owned Bank/Vault organization providers;
- preserve accepted controller conventions;
- add exact packed-list regression tests before enabling for Gen I/II.

This is **not required to begin the Gen III field-editor implementation once P0 is complete**, but the architecture is frozen now so Gen III must not invent a second box movement system.

## Research/reuse record

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** issue #71, `POKEMON_EDITOR_UI_CONTRACT`, `ARCHITECTURE`, `PRODUCT_UI_PHILOSOPHY`, `UPSTREAM_AUDIT`
- **EXTERNAL SOURCE/PROJECT:** inherited PKSE behavior
- **CURRENT IN-TREE IMPLEMENTATION:** `TrainerViewScreenBase.inc` Move/Multi/carry logic
- **DECISION:** REFACTOR INTO SHARED POKEBANK COMPONENT
- **WHY:** interaction is solved, mutation ownership is not compatible with the accepted staged-source boundary as a direct dependency.
- **LICENSE/REDISTRIBUTION:** already inherited under repository/upstream license obligations; preserve notices/history.
- **TEST/SAFETY:** source immutable, packed native semantics, blocked cells preserved, no direct live write, explicit destination capability.
