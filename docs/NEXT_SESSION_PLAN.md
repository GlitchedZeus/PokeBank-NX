# PokeBank NX — Next Session Plan

Last updated: 2026-09-22

Status: **PR #77 EXACT CI CANDIDATE IS UNDER PHYSICAL DEVICE RETEST — DO NOT CHANGE THE CANDIDATE WHILE TESTING**

## CURRENT PLAN — 2026-09-22

```text
Repository: GlitchedZeus/PokeBank-NX
Production: feature/pokebank-playable
Active branch: feature/gen3-shared-pokemon-editor-20260919
PR #77: OPEN / DRAFT / NOT MERGED

Exact candidate:
2e4780412377abab3ffbe4fc2e4757339214a90f

Tree:
cdbf0b8faae901c8d765d06b95cac3140bd69e75

NRO:
PokeBank-NX-Gen1-UX4-Retest-2e478041.nro

NRO SHA-256:
32b08c1cf589252022e68bf50fe0847fea7cbf1b86835178e7d2a2c268e3b43c
```

Current state:

```text
CI VERIFIED
DEVICE RETEST REQUIRED
NOT DEVICE ACCEPTED
```

### While the owner is testing

- do not push cleanup/refactor commits to PR #77;
- do not merge PR #77;
- do not change branches or restart Gen III;
- do not delete branches, commits, recovery assets, or old audit evidence;
- documentation/issue cleanup belongs on `main` and must not alter the tested candidate SHA;
- log any newly discovered hardware bug against the exact NRO/SHA.

### If the exact candidate passes

Record the physical result against the exact NRO/hash first.

Then decide whether the whole Gen I/II/III editor milestone is ready to freeze or whether known Gen II/Gen III behavior still needs another exact candidate.

Before major feature expansion, execute the safety work in:

`docs/FULL_PROJECT_AUDIT_2026-09-22.md`

Priority begins with:

1. atomic/durable Bank persistence;
2. no-loss carried-Pokémon rollback;
3. destination-first transfer custody;
4. cross-Bank/save transaction journal;
5. recovery-generation preservation;
6. malformed/truncated save boundaries;
7. exact-current conversion revalidation.

After those fixes, re-run full host/sanitizer/native gates and physical storage/recovery testing before trusting Master Vault with unique Pokémon.

### If the exact candidate fails

1. record the exact hardware reproduction;
2. re-fetch PR #77 live head;
3. preserve any concurrent newer commits;
4. make only the narrow demonstrated fix on the same branch;
5. create a new SHA;
6. run the entire required exact-SHA gate;
7. produce a new exact Actions-built NRO;
8. classify it DEVICE RETEST REQUIRED;
9. never combine CI/device evidence from different SHAs.

### Reference map

Before implementing a new parser, conversion, legality rule, bank format, event subsystem, or DS/3DS adapter, check:

- `docs/REFERENCE_INDEX.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`
- `docs/ISSUE71_UNIVERSAL_EDITOR_REUSE_FREEZE.md`

---

## HISTORICAL 2026-09-14 NEXT-SESSION PLAN — RETAINED FOR PROVENANCE

The old PR #68 plan below is intentionally preserved as historical evidence. It is not the current continuation point.


Last updated: 2026-09-14

Status: **GEN II SHARED POKÉMON EDITOR IMPLEMENTED ON PR #68 / FINAL EXACT-HEAD VALIDATION + DEVICE CANDIDATE NEXT**

## Recover this exact project state first

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
PR #68: OPEN / DRAFT / NOT MERGED
Known implementation head: 43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
Known tree: e791463ef438b5795da28c636de7bcac1105806f
```

If the PR head has advanced, preserve the newer head. Do not reset backward.

## Accepted baselines that must remain frozen

```text
Gen I RBY read-only: DEVICE ACCEPTED
Gen II GSC read-only: DEVICE ACCEPTED
Gen III GBA read-only: DEVICE ACCEPTED
Classic Inventory editor: DEVICE ACCEPTED
Gen I boxed staged Pokémon editor: DEVICE ACCEPTED
Gen I passive View unification: DEVICE ACCEPTED
Live installed-game writes: HARD DISABLED
Live emulator-source writes: HARD DISABLED
```

## What the current Gen II branch already contains

- exact G/S vs Crystal move compatibility tables and runtime selection;
- passive View live compatibility (`OK` / `Unusual preserved`);
- encounter legality still `Not checked`;
- clean passive View wording;
- shared Gen II Details/Values/Moves View/Create/Edit;
- authentic 0–15 DVs and derived HP DV;
- one Special DV feeding split SpA/SpD display;
- six-stat / six-axis battle presentation;
- Held Item picker;
- Friendship / Pokérus;
- Crystal-specific caught/met handling;
- species/Attack-DV gender;
- DV-derived shiny;
- local Add/Edit drafts;
- transactional Edit keep/discard/continue with exact pre-edit staged baseline;
- unusual existing move preservation on unrelated edits;
- separate Level and EXP applet invocations;
- permanent compatibility/passive-view/editor-surface tests.

## Next task

Do not start another feature milestone. Freeze the current exact PR #68 head and finish validation on that exact SHA.

Required final gates:

```text
git diff --check
Gen II focused staged-editor tests
Gen II compatibility data verifier
Gen II passive-view contract
Gen II editor-surface/session contract
Gen I accepted editor regressions
GSC read-only regressions
RBY regressions
FRLG/RSE regressions
inventory regressions
source mutation policy / source immutability
full permanent host suite
ASan
UBSan
device asset preflight
clean devkitA64 build
final NRO link
embedded application source identity
complete embedded RomFS verification
exact package manifest + SHA256SUMS
```

If any gate fails, fix only the real regression, commit/push, and restart exact-head verification from the new SHA.

## Hardware handoff

When all exact-head gates are green:

1. retrieve the CI-produced artifact for the exact candidate SHA;
2. verify artifact digest/size;
3. verify `BUILD_MANIFEST.json` and `SHA256SUMS.txt`;
4. verify standalone NRO and packaged ZIP hashes;
5. verify inner ZIP NRO is byte-identical to standalone NRO;
6. verify embedded source SHA and full RomFS;
7. provide the actual `.nro` for download;
8. STOP coding for owner physical Gold/Silver/Crystal testing.

Do not merge PR #68 or declare Gen II device accepted before that physical result.
