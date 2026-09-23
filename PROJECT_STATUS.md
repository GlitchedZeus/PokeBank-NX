# PokeBank NX Project Status

Last updated: **2026-09-23**

## Headline

PokeBank NX has completed the first major editor foundation milestone:

**Generation I, II, and III shared staged Pokémon editing is DEVICE ACCEPTED.**

Accepted Gen III checkpoint:

```text
source: 996e6aa40c96e4408282f3d55476dae8e64968b2
tree:   8826147ff5dc1b498b4b8505c9212243ed2f9498
NRO:    PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro
SHA256: ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c
run:    35825830004
artifact: 10735208869
status: CI VERIFIED / DEVICE ACCEPTED
```

PR #77 remains OPEN / DRAFT / NOT MERGED.

## What is complete

| Milestone | Status |
|---|---|
| Native Switch runtime | DEVICE ACCEPTED foundation |
| Gen I read-only | DEVICE ACCEPTED |
| Gen II read-only | DEVICE ACCEPTED |
| Gen III GBA read-only | DEVICE ACCEPTED |
| Classic staged Inventory | DEVICE ACCEPTED |
| Gen I boxed staged editor | DEVICE ACCEPTED |
| Gen II shared staged editor | DEVICE ACCEPTED |
| Gen III shared staged editor | DEVICE ACCEPTED |
| Gen I/II packed move + multi-select | DEVICE ACCEPTED |
| Universal editor/reuse architecture freeze (#71) | COMPLETE |
| Live source writing | HARD DISABLED |

## Current phase

The project is now in a **full audit / organization / storage-durability phase**, not a new-generation feature sprint.

Primary tracker: **issue #69**.

The audit exists because the next major feature—Master Vault—must not become the authoritative home of unique Pokémon until the underlying persistence, custody, rollback, conversion, parser, and recovery behavior has stronger guarantees than the inherited Bank/backup implementation currently provides.

### Confirmed audit work

- atomic/durable Bank persistence;
- custody-safe held Pokémon rollback;
- destination conversion candidates separated from immutable source representation;
- cross-store transaction journal/recovery;
- preservation of multiple corrupt/unreadable generations;
- malformed/truncated parser boundaries;
- read-only handling of unsupported newer/larger Bank formats;
- Switch-account/profile namespacing for mutable workspaces;
- durable replacement of mutable backup saves;
- conversion re-audit using exact-current fixtures;
- repository/reference/license organization;
- physical Switch recovery tests.

## After the audit

```text
audit + durable persistence
        ↓
Master Vault + named Banks hardening
        ↓
legacy Storage migration/import
        ↓
universal SaveSource adapters
        ↓
DS / 3DS
        ↓
modern Switch validation
        ↓
Summary / Dex / provenance / search
        ↓
conversion / legality / transfer workspace
        ↓
approved per-source writes only after proof
        ↓
v1.0
```

## Permanent architecture rules

### One editor

```text
shared editor UI/lifecycle
+ exact-game capabilities/provider
+ generation-native staged adapter
```

Do not create parallel generation-specific editor products.

### Source safety

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
```

### Device acceptance

`DEVICE ACCEPTED` applies only to the exact artifact physically accepted by the owner. It is not inherited by later SHAs.

## Canonical recovery files

- `README.md`
- `CURRENT_STATUS.md`
- `PROJECT_STATUS.md`
- `docs/FULL_PROJECT_AUDIT_2026-09-22.md`
- `docs/REFERENCE_INDEX.md`
- `docs/V1_ROADMAP.md`
- `docs/NEXT_SESSION_PLAN.md`
- `docs/NEXT_CODEX_PROMPT.md`
- `docs/GAME_SUPPORT_MATRIX.md`

Issue #29 remains the v1 master roadmap. Issue #69 is the current engineering gate.
