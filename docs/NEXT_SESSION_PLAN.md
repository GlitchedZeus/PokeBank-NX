# PokeBank NX — Next Session Plan

Last updated: **2026-09-23**

Status: **GEN I–III DEVICE ACCEPTED / AUDIT + ORGANIZATION PHASE NEXT**

## Recover exact accepted state

```text
Repository: GlitchedZeus/PokeBank-NX
Default/docs branch: main
Production branch: feature/pokebank-playable

PR #77:
OPEN / DRAFT / NOT MERGED

Branch:
feature/gen3-shared-pokemon-editor-20260919

Accepted application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Accepted tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

Accepted NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

Accepted NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Actions:
35825830004

Artifact:
10735208869
```

GitHub is authoritative. Re-fetch PR #77 and any branch you intend to modify before writing. Preserve newer work and never reset/rebase backward.

## Permanent accepted boundary

Do not reopen Gen I–III editor behavior without new evidence.

Preserve:

- source immutability;
- all live-write locks;
- one shared editor architecture;
- exact-game capability providers;
- Gen I/II packed source movement semantics;
- Gen III sparse/exact-format semantics;
- D-pad/Left Stick parity;
- themes/readability;
- accepted Gen III shiny/EXP/move-picker/PID behavior.

## Primary task next session

**Start the full project audit/hardening milestone tracked by issue #69.**

Do not begin Gen IV/DS/3DS or Master Vault feature expansion first.

### Stage A — repository and evidence organization

1. Re-fetch main, production, PR #77, issue #69 and issue #29.
2. Inventory active/open PRs and branches; classify ACTIVE / ACCEPTED / SUPERSEDED / HISTORICAL.
3. Do not delete branches or rewrite history.
4. Preserve recovery assets.
5. Confirm README/status/roadmap/reference docs agree.
6. Record exact external reference revisions/licenses where code/data is reused.

### Stage B — turn confirmed findings into tests

Create narrow reproduction/regression tests for:

```text
A01 Bank non-atomic persistence
A02 held Pokémon loss on failed rollback
A03 conversion mutates custody before commit
A04 Bank + destination lack one durable transaction
A05 unreadable-bank casualty overwrite
A06 truncated BDSP whole-layout boundary
A07 unsupported larger Bank can later truncate
A08 backup workspace profile/account collision
A09 mutable backup saves overwritten in place
```

Do not fix a finding until the test demonstrates it against the live source.

### Stage C — durable persistence primitive

Extract/generalize one PokeBank-owned safe replacement layer based on the strongest existing in-tree pattern:

```text
write temp
-> flush/fsync where available
-> close/check
-> reopen/validate
-> preserve prior generation
-> promote
-> verify promoted target
-> rollback/recover
```

Apply it first to Bank and mutable backup workspaces. Multi-file formats need a transaction journal.

### Stage D — conversion + parser re-audit

Reverify older conversion findings against exact-current code with golden fixtures. Harden all fixed-offset parsers to fail closed on truncated/unknown layouts.

### Stage E — physical recovery testing

On real Switch storage, deliberately test:

- interrupted writes where safely simulatable;
- corrupt target + valid prior generation;
- target/temp/backup combinations;
- full SD / short write failure paths where practical;
- multiple Switch profiles;
- recovery UX/data preservation.

## Completion gate for the audit milestone

Do not declare the audit phase complete until:

- known P1/P2 findings have tests and disposition;
- durable persistence is shared rather than duplicated;
- source-write locks remain hard disabled;
- sanitizer/permanent suite is green;
- native build is green;
- storage/recovery paths have physical evidence;
- Master Vault can safely depend on the new persistence layer.

## After that

Proceed to issue #3 / #27 Master Vault + named Banks hardening/migration.

No live source write work is authorized by this plan.
