# PokeBank NX — Next Session Plan

Last updated: 2026-09-14

Status: **GEN II FINAL AUDIT IMPLEMENTED / FREEZE EXACT CANDIDATE / MATERIALIZE CI NRO / PHYSICAL G-S-C TEST NEXT**

## Recover this project state first

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
PR #68: OPEN / DRAFT / NOT MERGED
Audit tracker: #69
Latest pre-documentation implementation checkpoint: a3917f20e0b09b31473f073398a9b1f1a6adfb51
```

The exact candidate SHA is the newer PR head containing this documentation refresh. Preserve any newer head; never reset backward to the checkpoint above.

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

Do not rewrite historical acceptance evidence while validating the new Gen II editor.

## Generation II implementation now present

- exact G/S vs Crystal move compatibility tables and runtime selection;
- passive View and occupied-slot action-sheet View;
- live compatibility (`OK` / `Unusual preserved`) with encounter legality still `Not checked`;
- canonical Level/EXP transaction behavior and Create EXP serialization;
- immediate EXP -> Level/stat/radar synchronization;
- authentic DVs, Stat Exp, derived HP DV and one stored Special feeding split SpA/SpD display;
- six-stat / six-axis battle presentation;
- authentic Held Item picker;
- Friendship;
- named Species picker;
- named exact-game Move picker;
- user-facing Pokérus State/Strain/Days control;
- PP/PP Ups validation/clamping;
- Crystal caught/met decode into meaningful native fields;
- Party-only Current HP / Max HP / Status;
- gender/shiny DV semantics;
- local Create/Edit drafts;
- transactional Edit Keep / Discard-current-session / Continue;
- unusual existing move/PP/PP-Up preservation;
- contained passive Gen I/II touch Back;
- exact save/game/revision capability model;
- one authoritative verified Gen II staged-export transaction used by the real UI.

## Export contract

The UI must continue to call `publishVerifiedStagedEditorExport()` and must not regain a second direct save writer. Required behavior:

```text
finalize staged bytes
strict pre-write parse
fingerprint source + edited bytes
write original backup + edited save under an app-owned temp directory
flush/sync/close where supported
read both files back
verify exact bytes + SHA-256
strictly reparse the edited disk bytes
write provenance manifest
publish by same-filesystem rename
remove incomplete temp output on failure
never write the source .srm
```

Only a fully verified/published directory counts as success.

## Final exact-SHA validation

Do not add features. If a gate fails, fix only the demonstrated regression, commit once, and restart all exact-head evidence from the new SHA.

The dedicated workflow `.github/workflows/gen2-audit-candidate.yml` must pass on the exact final PR head and prove:

```text
git diff --check
Gen II compatibility regeneration
Gen II parser/discovery/bridge
Gen II editor/session/Create/Edit/Level/EXP
passive/action-sheet View
Crystal caught/met
Party/Box native presentation
Held Item
Species picker
Move picker
Pokérus
PP / PP Ups
gender
shiny
six battle stats
six-axis radar
verified export UI + transaction
corruption rejection
temporary cleanup
source immutability/write locks
Gen I regression
Gen III regression
Classic Inventory regression
full permanent host suite
ASan
UBSan
device asset preflight
RomFS recovery/completeness
clean devkitA64 compile
final NRO link
AArch64 native linkage
embedded application SHA
embedded RomFS
BUILD_MANIFEST.json
SHA256SUMS.txt
Gen2-Audit NRO + package ZIP
```

Standard PR workflows must also be green for that same SHA. Never reuse an older green result.

## Audit issue disposition

When exact-head tests are green, update #69 accurately:

```text
COMPLETE: Crystal caught/met decode
COMPLETE: Party-only HP/status View
COMPLETE: user-facing Species/Move/Pokérus controls
COMPLETE: staged export transaction hardening
COMPLETE: contained passive View touch Back foundation
DEFERRED -> #55: full touch-only v1
DEFERRED -> #26: global box/controller normalization + device retest
DEFERRED -> #27: Legacy Storage migration / Master Vault / true Move architecture
```

Leave #55, #26 and #27 open. Issue #69 may remain open for these deferred/global items.

## Artifact handoff

When the exact candidate workflow is fully green:

1. record application SHA, tree SHA, branch, PR state and workflow run;
2. record artifact name/ID/size/digest;
3. download the exact CI Actions artifact;
4. verify `BUILD_MANIFEST.json`, `SHA256SUMS.txt` and package hash;
5. verify standalone NRO filename/size/SHA-256;
6. verify packaged ZIP filename/size/SHA-256;
7. verify embedded application SHA and complete RomFS evidence;
8. materialize the exact CI-built `PokeBank-NX-Gen2-Audit-<shortsha>.nro` into the conversation;
9. give the owner a clickable link;
10. STOP CODING.

Do not substitute an older NRO or locally rebuilt binary.

## Physical Switch checklist — after artifact handoff

Run on **Gold, Silver and Crystal**, noting any game-specific differences:

- boot/open correct save;
- Party View;
- Party Current HP / Max HP;
- Party Status;
- Box View and no fabricated Party-only values;
- occupied-slot action-sheet View;
- passive touch Back and controller B Back;
- named Species picker;
- named exact-game Move picker;
- Pokérus State/Strain/Days editor;
- Held Item picker;
- Level;
- EXP;
- Level/EXP synchronization;
- PP;
- PP Ups;
- move compatibility / unusual preserved state;
- Crystal caught/met native values;
- Gold/Silver show no fake Crystal caught/met fields;
- gender;
- shiny;
- Friendship;
- six battle stats;
- six-axis radar;
- Create / Stage Add;
- Edit;
- Keep;
- Discard current Edit session;
- Continue editing;
- unusual existing move/PP preservation on unrelated edit;
- Export staged copy;
- exported `original_backup.srm` exists;
- exported `edited.srm` exists;
- exported `EDIT_MANIFEST.txt` exists;
- original RetroArch/source `.srm` remains byte-unchanged.

PR #68 stays OPEN / DRAFT / NOT MERGED and Gen II stays DEVICE TEST PENDING until the owner reports the result of this checklist for the exact delivered artifact.
