# NEXT CODEX PROMPT — FINISH GEN II SHARED EDITOR DEVICE CANDIDATE

Continue PokeBank NX from the existing Gen II work. Do not restart, create a replacement branch, or reset newer commits.

## Repository state

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
PR #68: OPEN / DRAFT / NOT MERGED
Known implementation head: 43b8563d4177b58ae0f341c1ebd58f86e2ee4fdb
Known implementation tree: e791463ef438b5795da28c636de7bcac1105806f
```

First re-fetch PR #68. If it has advanced, preserve the newer head and continue from it.

## Accepted milestones — do not reopen without evidence

- Gen I RBY read-only: DEVICE ACCEPTED
- Gen II GSC read-only: DEVICE ACCEPTED
- Gen III GBA read-only: DEVICE ACCEPTED
- Classic staged Inventory: DEVICE ACCEPTED
- Gen I boxed staged Pokémon editor: DEVICE ACCEPTED
- Gen I passive View unification: DEVICE ACCEPTED

Accepted Gen I editor artifact:

```text
source 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

## Current Gen II implementation

The focused branch already contains the shared Gen II View/Create/Edit work. Do not redo research or regenerate compatibility data unless a failing test proves the existing data is wrong.

Implemented:

- Gold/Silver vs Crystal exact-game move compatibility;
- pinned offline compatibility tables derived from PKHeX reference commit `77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5`;
- passive View live compatibility: green `OK` / orange `Unusual preserved`;
- encounter legality remains `Not checked`;
- passive View does not show editor-internal `PKSE three-panel workspace` wording;
- shared Gen II `DETAILS | VALUES | MOVES` View/Create/Edit;
- authentic DVs / derived HP DV / Stat Exp;
- one Special DV feeding split SpA/SpD battle presentation;
- six-stat and six-axis radar presentation;
- Held Item picker using the Gen II item domain;
- Friendship and Pokérus;
- Crystal-only caught/met data where actually stored;
- species + Attack-DV gender;
- DV-derived shiny;
- local Add/Edit drafts;
- Y random 0–15 DVs;
- L/R previous/next panel;
- Create explicit Stage Add;
- transactional Edit baseline + Keep/Discard/Continue;
- discard restores the exact staged state that existed when Edit opened;
- unrelated edits preserve pre-existing unusual move/PP/PP-Up bytes;
- Level and EXP use separate applet invocations;
- permanent compatibility/passive-view/editor-surface host/sanitizer tests.

## Non-negotiable safety

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
PARTY MUTATION: DEFERRED WHERE NOT SEPARATELY PROVEN
```

## Primary task — freeze and validate one exact candidate

Do not start Gen III editor, Vault, DS/3DS, or another roadmap milestone.

On the exact current PR #68 head:

1. run `git diff --check`;
2. run all focused Gen II staged/editor/compatibility/passive-view tests;
3. run Gen I accepted editor regressions;
4. run GSC/RBY/FRLG/RSE/inventory regressions;
5. run source mutation/immutability gates;
6. run the full permanent host suite;
7. run ASan and UBSan;
8. run device asset preflight;
9. perform a clean devkitA64 compile and final NRO link;
10. verify embedded application SHA;
11. verify complete embedded RomFS;
12. produce source-addressed artifact packaging with manifest and SHA256SUMS.

Earlier CI results from older SHAs are not final-candidate evidence.

If anything fails, inspect the exact failure, fix only the real regression, commit/push, and restart verification from the new exact SHA.

## PR metadata

Update PR #68 body/status summary so it no longer claims Create/Edit, Held Item, compatibility, or transactional editing are still unimplemented. Keep PR #68 OPEN / DRAFT / NOT MERGED.

## Exact hardware handoff

Once every required gate is green on one frozen SHA, retrieve the CI-produced physical-test artifact and independently verify:

- application SHA + tree;
- workflow/run identity;
- artifact name/id/size/digest;
- `BUILD_MANIFEST.json`;
- `SHA256SUMS.txt`;
- standalone NRO size + SHA-256;
- packaged ZIP size + SHA-256;
- ZIP NRO byte-identical to standalone NRO;
- embedded source identity;
- complete RomFS count.

Then provide the actual downloadable `.nro` and STOP coding.

Physical test checklist must cover Gold/Silver/Crystal View/Create/Edit, move compatibility, Held Item, Friendship, Pokérus, Crystal caught/met, six stats/radar, shiny/gender/DVs, L/R/Y controls, transactional discard, invalid Add move blocking, unusual existing move preservation, and unchanged source `.srm`.

Do not merge PR #68 or declare Gen II DEVICE ACCEPTED before the owner reports the physical result.
