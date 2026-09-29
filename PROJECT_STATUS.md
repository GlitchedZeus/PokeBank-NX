# PokeBank NX Project Status

Last updated: 2026-09-28

For the shortest engineering recovery handoff, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Repository: `GlitchedZeus/PokeBank-NX`
- Current integration baseline: PR #90
- Active implementation lane: **PR #92**
- Active milestone: **Issue #95 — G4-04 Complete Gen IV shared editor: Create + remaining fields**
- Active branch: `feature/gen4-full-editor-20260928`
- Merge state: OPEN / DRAFT / NOT MERGED

GitHub is authoritative. Recorded SHAs are checkpoints, never instructions to reset backward.

## Headline

Generation I–III remain the accepted shared-editor foundation.

Generation IV has moved beyond the first hardware-accepted Party/Box View/Edit milestone and is now in the **proper shared-editor completion tranche**.

G4-03 hardware acceptance is anchored to:
`84dae170deb2756d9b80aec32bf8ad512ce17c31`

PR #90 integrates that accepted Gen IV base plus the completed v1 polish tranche at:
`8b3bcc16c804247bfe8d1314b686974ce73051d8`

PR #92 is now the canonical live implementation lane for G4-04.

## G4-04 implemented surface

The current PR #92 branch includes:

- shared Party + Box View/Edit;
- empty Box Add/Create;
- valid stored PK4 draft creation for DP / Pt / HGSS;
- staged empty-slot-only Create transaction;
- trainer identity initialization for Create;
- Held Item picker with exact Gen IV domains;
- Language picker with nickname/OT preservation checks;
- exact DP/Pt/HGSS Ball domains;
- Pokérus mode editing;
- named exact-game Met Location picker;
- native Gen IV move picker;
- exact Gen IV base-PP table;
- move selection resets PP and PP Ups coherently;
- Species mutation with dependent state reconciliation;
- Form mutation with exact-game and storage constraints;
- Giratina / Arceus item-driven form handling;
- Shaymin boxed-form fail-closed behavior;
- read-only-but-inspectable trainer/origin identity information;
- source immutability, strict reparse and rollback.

The code checkpoint immediately before this status refresh was:
`7eed3fa12f455c13bc03c9d55539eb6aa2a842c2`

Always re-fetch PR #92 because it may already be newer.

## Still required before G4-04 device acceptance

- exact-head Host Tests green;
- exact-head Gen IV Candidate Gate green;
- exact-head native compile/link/package green;
- regression gates green;
- PR/issue evidence refreshed;
- one combined Actions-built NRO;
- owner physical Switch test of Create + full field parity.

Do not mark G4-04 DEVICE ACCEPTED before that exact NRO is tested.

## Expected next hardware test

The eventual combined candidate should cover:

- Party View/Edit;
- Box View/Edit;
- empty Box → Add/Create;
- Species;
- Held Item;
- Language;
- Ball;
- Pokérus;
- Met Location;
- Form where valid;
- move selection / PP / PP Ups;
- read-only origin inspection;
- fixed/variable/genderless behavior;
- one-/dual-ability behavior;
- Shedinja;
- staged Keep and re-entry;
- dirty-exit protection;
- unchanged external emulator source.

## Safety / scope locks

- external emulator/installed-game writes remain disabled;
- ordinary editing stays app-owned and staged;
- cross-game True Move remains locked;
- Gen V must not start;
- Master Vault must not start;
- Issue #89 is the separate future Inject Save lane.

## Repository lane map

- PR #79 — audit/source architecture base — preserve, do not merge.
- PR #90 — integrated Gen IV + v1-polish baseline — preserve, do not merge.
- PR #92 — **active G4-04 implementation** — preserve, do not merge until owner acceptance.
- PR #87 — historical first Gen IV editor lane — preserved.
- PR #88 — completed QoL lane — integrated content preserved; do not blindly merge again.

## Continuation rule

Start from the live PR #92 head, audit forward, and keep working until G4-04 is software-proven enough to justify one scarce hardware test.

**CURRENT PRODUCT PRIORITY: finish the proper Generation IV shared editor before Gen V, Master Vault, or source injection.**
