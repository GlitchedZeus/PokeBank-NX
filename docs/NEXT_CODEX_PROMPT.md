# NEXT CODEX PROMPT — FULL PROJECT AUDIT / ORGANIZATION / DURABILITY HARDENING

Continue PokeBank NX exactly from the device-accepted Gen I–III milestone.

## Authoritative accepted checkpoint

```text
Repository:
GlitchedZeus/PokeBank-NX

PR:
#77

Branch:
feature/gen3-shared-pokemon-editor-20260919

Accepted application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Accepted tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

Accepted NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Gen III Actions run:
35825830004

Artifact ID:
10735208869

Status:
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
```

PR #77 is still OPEN / DRAFT / NOT MERGED. Do not merge it unless explicitly instructed.

## Before any write

1. Re-fetch GitHub.
2. Preserve any newer head.
3. Never reset/rebase backward.
4. Do not create a feature branch just to restart already accepted editor work.
5. Do not weaken source immutability or write locks.

## Permanent safety

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE RETROARCH WRITE: HARD DISABLED
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
```

## Primary task

The next milestone is **issue #69: full project audit / organization / durability hardening**.

Do **not** start Gen IV, DS/3DS, broad UI expansion, or live-write work.

### First deliverable — exact-current audit map

Audit the live source and repository organization. Reverify the already-confirmed findings and classify every item as:

```text
CONFIRMED
FIXED ALREADY
STALE / NO LONGER REPRODUCES
NEEDS FIXTURE
NEEDS HARDWARE TEST
DEFERRED WITH REASON
```

Current confirmed backlog to begin with:

- A01 Bank persistence not atomic/durable.
- A02 held Pokémon can be lost on failed return-to-origin.
- A03 conversion mutates custody before final placement/commit.
- A04 Bank + destination save are not one durable transaction.
- A05 fixed unreadable-bank casualty path can destroy older recovery evidence.
- A06 truncated BDSP input can reach unchecked fixed offsets.
- A07 unsupported larger Bank can later be truncated on save.
- A08 mutable backup workspaces are not namespaced by Switch profile.
- A09 mutable backup save files are overwritten in place.

### Test-first rule

For each confirmed defect:

1. add the smallest reproduction/regression test;
2. prove it fails for the expected reason;
3. make only the narrow fix;
4. rerun relevant focused + permanent regressions;
5. keep source-write locks unchanged.

### Durable persistence target

Unify Bank, profile bindings, mutable backup workspaces, and later Vault persistence behind one reviewed primitive.

Required conceptual contract:

```text
serialize
-> strict pre-write validation
-> unique temporary generation
-> write/flush/close/check
-> reopen/verify
-> preserve prior known-good generation
-> promote target
-> verify promoted target
-> deterministic rollback/recovery
```

For multi-file formats, use a journal/state machine so a generation is committed as one logical unit.

### Custody/transfer rule

Never destroy or replace the authoritative in-memory/source representation until a destination has been durably committed and verified.

Original payload and destination-converted candidate must remain distinct until commit.

### Conversion re-audit

Re-run the older F05–F13 conversion questions against exact-current code using golden fixtures. Do not blindly inherit old findings.

### Parser hardening

Every fixed-offset parser should have:

- minimum whole-layout guard before indexed reads;
- revision/version validation;
- malformed-count bounds;
- truncated fixtures;
- unknown-version fail closed;
- ASan/UBSan coverage.

### Repository organization

Audit:

- stale/superseded PRs and branches;
- duplicate/outdated status docs;
- reference/license provenance;
- recovery assets;
- generated data ownership;
- build/test duplication;
- active vs historical documentation.

Do not delete branches/history/recovery assets without explicit approval.

## Completion gate

The audit phase is complete only when:

- A01–A09 each have a clear tested disposition;
- durable storage/recovery is shared and tested;
- conversion/parser re-audit has current fixtures;
- full host + ASan/UBSan are green;
- devkitA64/native build is green;
- physical Switch recovery testing passes;
- source writes remain hard disabled.

Then hand off to Master Vault / named Banks hardening (#3/#27).

Do not merge PR #77 or begin the next major feature unless explicitly instructed.
