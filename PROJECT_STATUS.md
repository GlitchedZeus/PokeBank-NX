# PokeBank NX Project Status

Last updated: 2026-09-20

For the shortest recovery handoff, read `CURRENT_STATUS.md` and `docs/NEXT_SESSION_PLAN.md`.

## Project identity

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Accepted milestone merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
Accepted source checkpoint: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted source tree: 88d2ad46d57346df346453698ee11792f7df4f1f
PR #74: MERGED
Issue #71: COMPLETE
PR #75: MERGED
Issue #71 merge commit: fb0f3c6573eac12d2350253d5c225dae8fc02277
Writable remote: origin
Upstream/reference: kiasta/PKSE
Active development PR: #77 — Generation III: extend the shared Pokemon editor
Active development branch: feature/gen3-shared-pokemon-editor-20260919
PR #77 state: OPEN / DRAFT / NOT MERGED
Last observed PR #77 head: be030f31690cfbedaa1fc5763bd23fbfebd112eb (observation only; always re-fetch)
```

`DEVICE ACCEPTED` is reserved for an exact artifact physically tested and accepted by the owner.

## Headline

- **Issue #71 universal editor / reuse architecture freeze: COMPLETE.**
- PR #75 is merged; `docs/ISSUE71_UNIVERSAL_EDITOR_REUSE_FREEZE.md` is now the architecture authority for future editor generations.
- **Gen I/II packed move + multi-select milestone: DEVICE ACCEPTED.**
- PR #74 is merged into `feature/pokebank-playable`.
- Red/Blue/Yellow and Gold/Silver/Crystal source boxes present their native 20 valid slots.
- PokeBank-owned Legacy Storage remains 30 slots per box.
- Gen I/II staged Y movement, Y-hold rectangular multi-select, packed group movement, Release, and fullscreen View/Edit/Create are hardware accepted.
- The Gen II empty-slot action-sheet/fullscreen ownership bug is physically confirmed fixed.
- Original source saves remain immutable and every live source-write path remains HARD DISABLED.
- **Generation III development is ACTIVE in PR #77.** Do not create another Gen III branch or restart from an older checkpoint.
- PR #77 is in an active hardware-fix/retest cycle and is **NOT DEVICE ACCEPTED** unless the owner explicitly accepts an exact Actions-built NRO for an exact SHA.
- The PR #77 head is intentionally treated as dynamic. Every recovery session must re-fetch GitHub before using a recorded SHA.
- Production remains `feature/pokebank-playable`; PR #77 has not been merged.

## Exact hardware-accepted checkpoint

```text
Milestone: Gen I/II packed move + multi-select
Status: DEVICE ACCEPTED
Physical result: PASS / NO BUGS FOUND
Accepted source: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted tree: 88d2ad46d57346df346453698ee11792f7df4f1f
Accepted parent: 99aacea1d9c5bd392b1eb779d175e49d3f99179c
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-c24859ce.nro
Accepted NRO size: 161681137 bytes
Accepted NRO SHA-256: 0eeef3c4752fc52240332567bf3919099271be4985fda346da26d66018fff82f
Accepted Actions workflow: Gen I Cleanup3 Candidate Gate
Accepted Actions run: 35316216883
Accepted artifact: Gen1-UX4-Retest-Candidate
Accepted artifact ID: 10535653625
Accepted artifact digest: sha256:15c9a98c37c05f7ce4427791b33968faa43e0a9cb7442e57eb0a8b537ef0f6c1
Merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
```

The independent Gen II candidate pipeline produced the same application NRO SHA-256, so the accepted application binary identity is unambiguous.

## Accepted milestone behavior

### Generation I — Red / Blue / Yellow

DEVICE ACCEPTED:

- native 20-slot source-box presentation;
- PokeBank Legacy Storage remains 30 slots;
- empty-slot Add Pokemon;
- fullscreen View / Edit / Create;
- A-button Actions;
- Release behind A -> Actions -> Release -> explicit confirmation;
- Y tap single packed move;
- Y hold rectangular multi-select;
- same-box and cross-box packed movement;
- deterministic selected/group ordering;
- atomic destination-capacity rejection;
- B exact staged restore;
- controller behavior and fullscreen surface ownership.

### Generation II — Gold / Silver / Crystal

DEVICE ACCEPTED:

- native 20-slot source-box presentation;
- PokeBank Legacy Storage remains 30 slots;
- empty-slot Add Pokemon immediately opens Create;
- Review Pending Changes opens Review rather than exposing a hidden Create workspace;
- no action sheet remains on top of View/Edit/Create;
- fullscreen View / Edit / Create;
- A-button Actions;
- Release behind A -> Actions -> Release -> explicit confirmation;
- Y tap single packed move;
- Y hold rectangular multi-select;
- same-box and cross-box packed movement;
- deterministic selected/group ordering;
- atomic destination-capacity rejection;
- B exact staged restore;
- controller behavior and fullscreen surface ownership.

## Previously accepted baselines

These historical milestones remain accepted and must not be rewritten as untested:

| Milestone | Status |
|---|---|
| Red / Blue / Yellow read-only | DEVICE ACCEPTED |
| Gold / Silver / Crystal read-only | DEVICE ACCEPTED |
| FireRed / LeafGreen / Ruby / Sapphire / Emerald read-only | DEVICE ACCEPTED |
| Classic staged Inventory | DEVICE ACCEPTED |
| Gen I boxed staged Pokemon editor | DEVICE ACCEPTED |
| Gen I passive View unification | DEVICE ACCEPTED |
| Gen I/II packed move + multi-select / Release / native source capacity | DEVICE ACCEPTED |

Generation II read semantics remain:

```text
Gold Trainer gender: Male
Silver Trainer gender: Male
Crystal Trainer gender: save-derived
Gen II SID: N/A — does not exist
```

Historical Gen I editor acceptance evidence remains valid:

```text
Source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

## Permanent safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
READ ACCEPTANCE DOES NOT AUTHORIZE SOURCE WRITES
UNKNOWN SAVE VARIANTS: FAIL CLOSED
```

The accepted PR #74 milestone does not weaken any source-write lock.

## Continuation boundary

Production contains the frozen accepted Gen I/II history and remains separate from the active Generation III development line.

PR #77 is the **only active Generation III implementation line**:

```text
PR: #77 — Generation III: extend the shared Pokemon editor
Branch: feature/gen3-shared-pokemon-editor-20260919
State: OPEN / DRAFT / NOT MERGED
Hardware state: ACTIVE FIX / RETEST CYCLE
Device acceptance: NOT FROZEN FOR THE CURRENT DYNAMIC HEAD
```

The recorded PR head is only an observation. GitHub is authoritative because concurrent hardware-fix work may advance the branch at any time.

At the beginning of the next development session:

1. re-fetch PR #77 and `feature/pokebank-playable` before making any assumption about the current head;
2. preserve any newer PR #77 head and never reset/rebase backward to a handoff SHA;
3. continue the same PR #77 / same branch — do not create another Gen III branch and do not start over;
4. preserve all accepted Gen I/II behavior and every source-write safety invariant above;
5. do not merge PR #77 until the owner explicitly accepts an exact fully-green Actions-built NRO for an exact SHA;
6. treat older PR #77 candidates and older handoff SHAs as historical evidence only unless they are still the live GitHub head.

**GEN III IS ACTIVE IN PR #77 — RE-FETCH THE LIVE HEAD BEFORE CONTINUING.**
