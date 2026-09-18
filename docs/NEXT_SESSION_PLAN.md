# PokeBank NX — Next Session Plan

Last updated: 2026-09-18

Status: **GEN I/II PACKED MOVE + MULTI-SELECT DEVICE ACCEPTED / PR #74 MERGED / PRODUCTION SYNCHRONIZED**

## Recover this project state first

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Accepted milestone merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
Accepted source checkpoint: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted source tree: 88d2ad46d57346df346453698ee11792f7df4f1f
PR #74: MERGED
```

GitHub is authoritative. Re-fetch production before any new modification. If GitHub contains a newer production head, preserve it; never reset/rebase backward to the checkpoint above.

## Exact frozen hardware checkpoint

```text
Milestone: Gen I/II packed move + multi-select
Status: DEVICE ACCEPTED
Physical result: PASS / NO BUGS FOUND
Accepted source: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted tree: 88d2ad46d57346df346453698ee11792f7df4f1f
Accepted parent: 99aacea1d9c5bd392b1eb779d175e49d3f99179c
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-c24859ce.nro
Accepted NRO SHA-256: 0eeef3c4752fc52240332567bf3919099271be4985fda346da26d66018fff82f
Accepted Actions run: 35316216883
Accepted artifact: Gen1-UX4-Retest-Candidate
Accepted artifact ID: 10535653625
Accepted artifact digest: sha256:15c9a98c37c05f7ce4427791b33968faa43e0a9cb7442e57eb0a8b537ef0f6c1
Merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
```

Do not rewrite this acceptance evidence when starting a later milestone.

## Accepted frozen behavior

Preserve all of the following:

- Gen I Red/Blue/Yellow native 20-slot source boxes;
- Gen II Gold/Silver/Crystal native 20-slot source boxes;
- PokeBank Legacy Storage remains 30 slots;
- Gen I/II Y tap single packed move;
- Y-hold rectangular multi-select with D-pad expansion/contraction;
- deterministic group selection/order;
- same-box and cross-box packed movement;
- atomic insufficient-capacity rejection;
- B exact staged restore;
- A Actions;
- Release only through A Actions + explicit confirmation;
- no bulk Release shortcut;
- Gen I/II fullscreen View/Edit/Create;
- Gen II Empty Slot -> Add Pokemon immediately opens Create;
- Gen II Review Pending Changes opens Review;
- no hidden Gen II Create/View/Edit workspace behind the legacy action sheet;
- accepted controller behavior and touch ownership;
- source save immutability.

## Safety invariants — permanent

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
```

Read/editor/device acceptance never grants permission to weaken those locks.

## First action next session

1. Re-fetch `feature/pokebank-playable`.
2. Record the exact current production head/tree/parent.
3. Preserve any newer production work.
4. Confirm the accepted `c24859ce...` checkpoint remains in production ancestry.
5. Only then select a new branch/PR for the owner's explicitly requested next milestone.

## Next milestone boundary

No new major feature is selected by this handoff.

**Do NOT start Gen III automatically.**

Do not reopen PR #74 for unrelated work. A future feature should receive its own focused branch/PR once the owner states what to build next.

No further physical testing is required for PR #74 unless a future change intentionally touches one of its accepted behaviors.
