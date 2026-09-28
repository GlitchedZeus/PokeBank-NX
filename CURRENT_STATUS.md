# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-28

## GitHub-authoritative development state

Repository: `GlitchedZeus/PokeBank-NX`

Always re-fetch GitHub before continuing. Preserve every newer commit. Never reset/rebase backward, force-push, or restart from an older checkpoint.

### Active lanes

- **PR #79** — audit/source-architecture base — OPEN / DRAFT / NOT MERGED.
  - Accepted audit head: `00ee7a6ed7ac1b5a93c43246d70c252e135acec0`
- **PR #90** — combined Gen IV + final v1-polish integration baseline — OPEN / DRAFT / NOT MERGED.
  - Integration head: `8b3bcc16c804247bfe8d1314b686974ce73051d8`
  - G4-03 hardware acceptance remains anchored to exact application `84dae170deb2756d9b80aec32bf8ad512ce17c31`.
- **PR #92** — **ACTIVE MAIN DEVELOPMENT LANE** for Issue #95 / G4-04.
  - Branch: `feature/gen4-full-editor-20260928`
  - State: OPEN / DRAFT / NOT MERGED.
  - Code checkpoint immediately before this status refresh: `7eed3fa12f455c13bc03c9d55539eb6aa2a842c2`.
  - Re-fetch the PR before relying on that SHA.

Historical PR #87 and completed QoL PR #88 are preserved and must not be rewritten or re-merged blindly.

## Generation IV status

### Hardware accepted: G4-03 first staged editor milestone

The owner physically accepted the first safe Gen IV Party/Box View/Edit milestone on real Switch hardware.

Accepted application:
`84dae170deb2756d9b80aec32bf8ad512ce17c31`

Accepted behavior includes:

- Platinum DraStic source loading;
- Party and Box reachability;
- shared PokeBank NX View/Edit shell;
- staged PK4 mutation;
- General + Storage CRC refresh;
- strict full-save reparse and rollback;
- Party derived-stat coherence;
- dirty-session protection;
- external emulator source unchanged.

This does **not** mean G4-04 is device accepted.

### Active: G4-04 proper Gen IV editor

Canonical issue: **#95**
Canonical implementation PR: **#92**

Current implementation includes:

- existing Party + Box View/Edit;
- empty Box Add/Create;
- deterministic trainer-bound stored PK4 Create drafts;
- DP / Platinum / HGSS Create coverage;
- editable Held Item;
- editable Language with text-preservation checks;
- editable Ball;
- Pokérus None / Cured / Infected mapping;
- real Gen IV Met Location names + exact-game validation;
- native Gen IV move selection, IDs 1–467 only;
- move replacement resets PP to exact Gen IV base PP and clears PP Ups;
- transactional Species mutation with growth-rate, gender, ability, form and Party-stat reconciliation;
- Form editing with fail-closed exact-game/storage rules;
- Giratina/Arceus item-driven form coherence;
- boxed Shaymin Sky Forme rejection;
- inspectable read-only origin/trainer identity rows;
- source immutability and staged rollback retained.

Read-only until separately proven:

- OT name;
- TID;
- SID;
- direct PID;
- deeper origin/date/egg identity metadata.

## Current CI boundary

The latest code checkpoint fixed stale Gen IV hardware-surface source contracts that were blocking otherwise-passing backend tests.

Do not call G4-04 automated-green or hardware-ready from this document alone. Re-fetch PR #92 exact-head workflow results.

## Permanent safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN / AMBIGUOUS SOURCE: FAIL CLOSED
CROSS-GAME TRUE MOVE: LOCKED
GEN V: NOT STARTED
MASTER VAULT: NOT STARTED
```

Issue #89 owns the later backup → working copy → explicit Inject Save architecture. Do not mix direct source writing into G4-04.

## Continuation boundary

The next development session must:

1. re-fetch PR #92 live head and exact-head Actions;
2. preserve every newer commit;
3. continue Issue #95 on `feature/gen4-full-editor-20260928`;
4. finish software proof for Create, Forms, moves, field parity, navigation and rollback;
5. update PR #92 / Issue #95 with exact evidence;
6. produce only **one** combined Actions-built NRO when the full software matrix is green;
7. keep PR #79, PR #90 and PR #92 unmerged until explicitly authorized.

**ACTIVE MAIN DEVELOPMENT: PR #92 / ISSUE #95 / G4-04.**
