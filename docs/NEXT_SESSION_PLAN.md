# PokeBank NX — Next Session Plan

Last updated: 2026-09-20

Status: **GEN III ACTIVE IN PR #77 / SAME BRANCH / HARDWARE FIX + RETEST CYCLE / NOT MERGED**

## Recover this project state first

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Accepted milestone merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
Accepted source checkpoint: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted source tree: 88d2ad46d57346df346453698ee11792f7df4f1f
PR #74: MERGED
Issue #71: COMPLETE
PR #75: MERGED
Issue #71 merge commit: fb0f3c6573eac12d2350253d5c225dae8fc02277
Active PR: #77 — Generation III: extend the shared Pokemon editor
Active branch: feature/gen3-shared-pokemon-editor-20260919
PR #77 state: OPEN / DRAFT / NOT MERGED
Last observed pre-cleanup head: be030f31690cfbedaa1fc5763bd23fbfebd112eb (observation only; always re-fetch)
```

GitHub is authoritative. **Re-fetch PR #77 and production before any new modification.** The PR head is dynamic while hardware fixes are underway. Preserve every newer head and never reset/rebase backward to a recorded checkpoint.

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

## Completed issue #71 architecture freeze

`docs/ISSUE71_UNIVERSAL_EDITOR_REUSE_FREEZE.md` is merged and authoritative.

Future generations extend the accepted shared editor through exact-game capabilities/providers and generation-native adapters. Do not create a parallel Gen III editor shell.

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

1. Re-fetch **PR #77** and `feature/pokebank-playable`.
2. Record the exact live PR #77 head/tree/parent and current exact-head workflows.
3. Preserve any newer PR #77 or production work; never reset/rebase backward to the SHA recorded in an older handoff.
4. Continue the **same** `feature/gen3-shared-pokemon-editor-20260919` branch. Do not create another Gen III branch and do not start over.
5. Confirm the accepted `c24859ce...` Gen I/II checkpoint and merged #71 architecture freeze remain preserved.
6. Keep PR #77 OPEN / DRAFT / NOT MERGED until the owner explicitly accepts an exact fully-green Actions-built NRO.
7. If the owner reports bugs from hardware, treat those findings as authoritative for the next fixes without transferring acceptance from an older SHA to a newer one.

## Active milestone boundary

Generation III is already active in PR #77. The current work is an ongoing hardware fix/retest cycle, not a future milestone waiting to start.

Do not reopen PR #74 or PR #75 for unrelated work. Do not resume stale PR #72 as the Gen III implementation line. Older PR #77 SHAs are historical evidence only unless GitHub still reports them as the live head.

Because PR #77 has touched shared Gen I/II surfaces while implementing Gen III, the exact final candidate must preserve the accepted Gen I/II behavior and should receive the required hardware regression coverage before merge.

No future session may claim DEVICE ACCEPTED solely from CI success; acceptance belongs only to the exact artifact/SHA physically accepted by the owner.
