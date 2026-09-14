# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-14

## Current repository state

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
Draft PR: #68 — OPEN / DRAFT / NOT MERGED
Current implementation head: 43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
Current implementation tree: e791463ef438b5795da28c636de7bcac1105806f
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

Do not reset the focused branch backward if newer commits exist. PR #68 must remain draft/unmerged until an exact hardware-test NRO is physically accepted.

## Physically accepted milestones

### Legacy read-only

```text
Red GB        DEVICE ACCEPTED
Blue GB       DEVICE ACCEPTED
Yellow GB     DEVICE ACCEPTED
Gold GBC      DEVICE ACCEPTED
Silver GBC    DEVICE ACCEPTED
Crystal GBC   DEVICE ACCEPTED
FireRed GBA   DEVICE ACCEPTED
LeafGreen GBA DEVICE ACCEPTED
Ruby GBA      DEVICE ACCEPTED
Sapphire GBA  DEVICE ACCEPTED
Emerald GBA   DEVICE ACCEPTED
```

Gold/Silver Trainer gender is fixed Male. Crystal Trainer gender is save-derived. Gen II SID does not exist.

### Classic staged Inventory

Hardware accepted. Authoritative editable control mapping:

```text
A Edit Amount
X Add Item
Y Remove Item
L/R Category
+ Options
- Help
B Back/Cancel
```

Exact-game/exact-pocket catalogs, empty valid categories, machine move labels, Key Item warnings, staged-only mutation, and source immutability remain the contract.

### Generation I boxed staged Pokémon editor

Issue #62 is CLOSED / DEVICE ACCEPTED.

```text
Accepted source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
Accepted NRO SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

Accepted behavior includes the shared capability-driven `DETAILS | VALUES | MOVES` editor, staged Create/Edit, transactional Edit exit, Y DV randomization, L/R panel navigation, truthful Gen I fields, semantic move compatibility colors, five-stat battle presentation/radar, shiny correctness, and immutable source bytes.

PR #67 was subsequently hardware accepted and merged to unify Party/Storage/action-sheet View Pokémon into one passive no-cursor read-only presenter.

## Current active milestone — Generation II shared Pokémon editor

PR #68 extends Gold/Silver/Crystal onto the same reusable foundation. The current implementation includes:

- passive Gen II three-panel View;
- exact-game Gold/Silver vs Crystal move compatibility;
- generated offline compatibility tables pinned to PKHeX reference commit `77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5`;
- live passive View compatibility (`OK` / `Unusual preserved`), with encounter legality still `Not checked`;
- clean passive View wording with editor-internal wording removed;
- shared Gen II Create/Edit shell;
- local drafts rather than immediate staged mutation while editing;
- authentic 0–15 Attack/Defense/Speed/Special DVs and derived HP DV;
- one Special DV feeding truthful split SpA/SpD battle presentation;
- six displayed battle stats and six-axis radar in `HP / Atk / Def / SpA / SpD / Spe` order;
- exact-domain Held Item picker;
- Friendship and Pokérus editing;
- Crystal-only caught/met presentation where actually stored;
- species/Attack-DV-derived gender semantics;
- DV-derived shiny semantics;
- transactional existing-Pokémon Edit snapshot/keep/discard/continue behavior;
- preservation of pre-existing unusual move/PP/PP-Up bytes during unrelated edits;
- separate Level and EXP input actions to avoid chained library applets;
- permanent compatibility/passive-view/editor-surface host and sanitizer gates.

## Important validation boundary

`43b8563d...` is an implementation checkpoint, **not** a device candidate and **not** device accepted.

Earlier CI results cannot be reused as final evidence for this later exact head. Before hardware handoff, freeze one exact current SHA and verify on that exact source:

```text
git diff --check
Gen II focused tests
Gen II compatibility-data verification
Gen I editor regressions
GSC read-only regressions
RBY regressions
FRLG/RSE regressions
inventory regressions
source mutation policy / source immutability
full permanent host suite
ASan
UBSan
device asset preflight
clean devkitA64 compile
final NRO link
embedded application SHA
complete embedded RomFS
exact artifact packaging / hashes
```

Then retrieve/materialize the exact CI-built `.nro` and stop for owner hardware testing on Gold/Silver/Crystal.

## Safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
PARTY MUTATION: DEFERRED WHERE NOT SEPARATELY PROVEN
STAGED/EXPORT WORKFLOW: REQUIRED
```

Physical acceptance of read or staged-edit milestones does not authorize live writeback.

## Immediate next step

1. Recover current PR #68 head; preserve any newer commits.
2. Update stale PR/status metadata if needed.
3. Run/fix all final gates on one frozen exact SHA.
4. Produce and independently verify one source-addressed Gen II hardware-test NRO.
5. Keep PR #68 open/draft/unmerged.
6. Wait for physical Gold/Silver/Crystal results before declaring acceptance.
