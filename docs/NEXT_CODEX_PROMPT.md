# PokeBank NX — Authoritative Next Codex Prompt

> **This file is the complete task for the next coding session.**
>
> The chat launcher should stay short. Do not expand scope from README or future-roadmap documents.

## Short launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Before touching refs, preserve all local/recovery work. Read CURRENT_STATUS.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```

---

# 0 — Mission

Finish the **same FireRed / LeafGreen GBA physical-test blocker milestone**.

Do not begin Ruby/Sapphire/Emerald or any later roadmap feature until a new full-asset FRLG NRO is built and the user physically retests it.

The active task is **FRLG blockers D–G**, while preserving already-published A–C behavior.

Future product documents are planning only and must not expand this session:

```text
docs/FUTURE_PRODUCT_VISION.md
docs/FEATURE_FEASIBILITY_MATRIX.md
docs/V1_ROADMAP.md
docs/V2_ROADMAP.md
docs/PRODUCT_DECISIONS.md
README.md
```

`CURRENT_STATUS.md` plus this file are authoritative for active implementation.

---

# 1 — Remote / published state

The latest published FRLG application-code checkpoint is:

```text
9e72e2732fd1cd30a3467e9a9a3f7a4f62ca8024
gen3: fix RetroArch grouping scope and artwork
```

A–C were checkpointed there.

After that application checkpoint, documentation-only commits were added for future product/release planning. The docs history immediately before this prompt refresh included:

```text
2c0dac8e0eea801ed917cfefd131693c6af410b6
docs: lock release scope and verified breeding policy
```

The current branch head may be newer because this prompt itself is committed to the same branch. Always inspect `origin/feature/pokebank-playable` rather than assuming the application code moved beyond `9e72e273`.

Important distinction:

```text
published docs head != published FRLG D–G application work
```

The interrupted D–G work described below was NOT known to be published at the time of the timeout.

---

# 2 — Recovery first — mandatory

A previous coding session timed out after implementing substantial D–G work locally.

That session explicitly reported local implementation of:

- strict FRLG trainer metadata through the PokeBank-owned Gen III read-only adapter;
- all six FRLG inventory containers through that strict adapter;
- deterministic encrypted-money / security-keyed-count tests;
- byte-immutability tests;
- authoritative configured RetroArch `savefile_directory` precedence over the conventional fallback;
- host/native build/test integration work was underway when the session ended.

Before ANY operation that could discard or rewrite state, inspect and preserve everything useful.

Do this BEFORE:

```text
sync
reset
clean
checkout/switch that changes refs
restore
rebase
worktree deletion
branch replacement
hard reset
pruning recovery state
```

Inspect at minimum:

```text
git status
git branch --all --verbose
git worktree list
git stash list
git reflog
untracked files
local recovery branches/commits
```

If the interrupted FRLG D–G tree exists locally:

1. identify it;
2. preserve the exact useful tree in a dedicated local recovery commit/ref if it is not already committed;
3. do not discard or reimplement working code merely because remote is behind;
4. only after preservation, reconcile it onto the current documentation history.

If the interrupted D–G tree no longer exists in the restored environment, do not fail the session. Preserve whatever local/recovery work does exist, then continue implementing D–G from the published A–C checkpoint and the requirements in this file.

## Parked RSE work

RSE must remain parked.

Known/equivalent recovery refs may include:

```text
b5ef83b
1a921515
```

Preserve either/both or any equivalent recovered RSE checkpoint if present.

Do NOT:

- merge RSE;
- resume RSE;
- reimplement RSE;
- push RSE;
- begin Ruby/Sapphire/Emerald.

Custom PokeBank NX work may be pushed only to:

```text
origin/feature/pokebank-playable
```

Never push PokeBank NX custom code to upstream `kiasta/PKSE`.

---

# 3 — Preserve already-working FRLG A–C

A–C are already published at `9e72e273` and should not be redone except to fix a regression introduced while finishing D–G.

## A — RetroArch/file legacy sources are app-global

Required existing behavior:

```text
installed Switch title save -> Nintendo-user scoped when required
RetroArch/file/legacy save   -> app-global
```

RetroArch itself is not a Nintendo user/profile.

## B — one parent per FRLG game identity with child save instances

Required existing hierarchy:

```text
Pokémon FireRed
  -> Main Save / physical save instances

Pokémon LeafGreen
  -> Main Save / physical save instances
```

Preserve:

- one FireRed parent and one LeafGreen parent for the source family/game identity;
- canonical alias/overlapping-route dedupe;
- genuinely separate physical save files remain separate children;
- never dedupe merely by game ID, trainer name, or raw hash;
- future Save State 0/1/2/3 and backups belong under the same parent but actual `.state#` parsing remains out of scope.

## C — correct GBA artwork

Preserve:

```text
firered_gba   -> FireRed GBA card artwork
leafgreen_gba -> LeafGreen GBA card artwork
```

Keep these distinct from the separate official Switch FRLG identities.

---

# 4 — D: Refresh / Rescan Saves

Legacy sources cannot remain an immutable app-start snapshot.

Required behavior:

1. bounded source scan on app start;
2. refresh the active approved RetroArch source root when opening a FireRed/LeafGreen game parent;
3. manual **Refresh Saves** / **Rescan Sources** action in the parent/save-instance flow;
4. stable source identity plus normalized physical path and useful file metadata such as size/mtime or equivalent;
5. changed child invalidates stale cached parsed/read-model state;
6. selecting a changed child rereads and strictly revalidates the source before exposing Trainer/Items/Party/Boxes;
7. deleted/missing children disappear safely;
8. stale selections fail gracefully;
9. do NOT hot-swap a source while already inside Trainer/Items/Party/Boxes; refresh only at a clear navigation/action boundary;
10. remain strictly read-only.

Refresh does not make an old RetroArch savestate newer. Real Save State slot support is a future adapter.

---

# 5 — E: authoritative active RetroArch battery-save root

Production BATTERY_SAVE support remains `.sav` / `.srm` only.

Required precedence:

1. a usable configured `savefile_directory` from `sdmc:/retroarch/retroarch.cfg` is authoritative;
2. `sdmc:/retroarch/cores/savefiles` is fallback-only when no usable configured save root exists;
3. never scan configured + conventional roots additively merely because both exist;
4. preserve bounded traversal under the selected root;
5. retain normalized physical source path plus useful metadata for refresh/diagnostics;
6. diagnostics/device report must identify the selected physical root/path;
7. never silently choose a stale copy outside RetroArch's active configured battery-save root.

---

# 6 — F: truthful FRLG trainer metadata

Physical testing of the older artifact showed placeholders:

```text
Name: (none)
Money: 0
Trainer ID: missing/zero
Full TID/SID: zero placeholders
```

Expand/use the strict **PokeBank-owned Gen III read-only adapter** so `FRLGReadOnlyTrainer` receives real trainer metadata.

Do not route production reading through mutable/permissive `Trainer3FRLG` as a shortcut.

Required values:

```text
trainer name with proper Gen III decoding
gender
TID16
SID16
ID32 / combined Gen III trainer-ID semantics used by the existing Trainer model
security key internally as needed
money decrypted correctly
exact source-game identity
```

Verified FRLG semantics to cross-check:

```text
trainer name = small sector + 0x00, length 7
gender       = byte + 0x08 & 1
TID16        = LE16 + 0x0A
SID16        = LE16 + 0x0C
ID32         = LE32 + 0x0A
security key = LE32 small block + 0xF20
money        = LE32 large block + 0x290 XOR security key
```

Required tests:

- deterministic fixture expectations for exact name;
- gender;
- TID16;
- SID16;
- ID32/current Trainer-model semantics;
- money;
- source-game identity;
- source bytes remain unchanged after reads.

Do not invent modern IDs that Gen III does not store.

---

# 7 — G: real read-only FRLG inventory

The older physical artifact showed `Invalid category` because the read-only trainer did not populate inventory containers.

Expose exactly six FRLG inventory containers in this order:

```text
1. Items
2. Key Items
3. Poké Balls
4. TM Case
5. Berry Pouch
6. PC Items
```

Verified layout:

```text
Items        0x310   42 slots   keyed
Key Items    0x3B8   30 slots   keyed
Poké Balls   0x430   13 slots   keyed
TM Case      0x464   58 slots   keyed
Berry Pouch  0x54C   43 slots   keyed
PC Items     0x298   30 slots   NOT keyed
```

Each slot:

```text
u16 item ID
u16 count
```

For keyed pouches, decode counts using the low 16 bits of the FRLG security key.

PC Item counts remain plaintext/non-keyed.

Required behavior:

- exact Gen III item IDs expected by existing naming/UI paths;
- correct counts;
- correct security-key decoding;
- zero/empty slots omitted rather than turned into fake entries;
- malformed/out-of-range data fails safely;
- Items remains strictly read-only;
- `updateItemBlock()` remains inert;
- no source serializer/writeback is introduced;
- deterministic fixture tests cover pouch count/order and representative item IDs/counts;
- source bytes remain unchanged.

---

# 8 — Preserve current read model and safety

Do not regress:

- pinned PKSM-Core Gen III host oracle;
- exception-free native Gen III backend;
- strict rotating-slot/sector/signature/counter/checksum selection;
- bounded `.sav/.srm` discovery;
- strict FRLG structural validation;
- Party enumeration;
- all 14 Boxes;
- Pokémon View;
- exact validated catalog-entry routing;
- A–C profile/grouping/artwork behavior;
- read-only source policy;
- accepted red PokeBank UI identity.

This milestone remains read-only.

Do NOT implement or enable:

- RetroArch source writeback;
- repair/resign;
- installed-title writes;
- trainer/item editing back into source;
- clone-to-save;
- transfer-to-save/live write;
- true Move;
- conversion;
- Profile Vault/Banks implementation;
- Verified Breeding implementation;
- Guided Create implementation;
- RSE;
- Gen I/II;
- DS/3DS;
- Colosseum/XD;
- Stadium/Stadium 2;
- friends/mailbox;
- Android;
- Internet/LAN trade work;
- events/Mystery Gifts;
- artwork-pack work;
- physical-link hardware;
- arbitrary `.state#` parsing.

Disabled mutation actions must remain blocked and must never fall through.

---

# 9 — Checkpoint policy

Once recovered/implemented D–G source work is coherent and basic tests pass, commit and push a **source checkpoint early** to:

```text
origin/feature/pokebank-playable
```

Do not wait for full asset restoration/packaging before publishing the application-source checkpoint.

Suggested application commit concept:

```text
gen3: complete FRLG read-only source browsing
```

Keep documentation changes minimal and subordinate to finishing the application milestone.

---

# 10 — Required verification

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native application remains `-fno-exceptions`.

Explicitly verify where practical:

- RetroArch sources visible under the normal Nintendo profile;
- installed Switch-title user scoping remains unaffected;
- one FireRed parent and one LeafGreen parent when present;
- configured active save root wins;
- conventional root is fallback-only;
- alias routes do not duplicate the same physical child;
- genuinely separate physical files remain separate children;
- opening a game parent refreshes disk state;
- manual Refresh Saves / Rescan Sources works;
- changed/deleted battery saves invalidate safely;
- selected physical path/root is diagnosable;
- FireRed/LeafGreen GBA artwork still resolves;
- trainer fixture values match expected name/gender/TID16/SID16/ID32/money;
- Items exposes exactly six FRLG containers in the required order;
- representative item IDs/counts decode correctly including keyed counts;
- Party/Boxes/View remain correct;
- source bytes remain byte-identical after Trainer/Items/Party/Boxes reads;
- all source mutation actions remain blocked.

---

# 11 — Full-asset physical-test artifact

Only after all seven FRLG blockers A–G pass verification:

1. restore/verify the complete pinned HD artwork set;
2. verify 3,260 Pokémon renders;
3. verify 1,025 / 1,025 base species coverage;
4. perform the established full RomFS verification;
5. build a clean exact-source native `.nro`;
6. package the full-asset physical-test artifact.

Final report must include:

```text
starting remote SHA
recovered interrupted FRLG D–G local ref/commit (or explicit statement it was absent)
preserved RSE refs/status including b5ef83b / 1a921515 / equivalents
application/source commit SHA(s)
selected active RetroArch root/path
source grouping result
refresh/rescan result
FireRed/LeafGreen GBA artwork result
trainer metadata result
six-pouch inventory result
host-test result
ASan/UBSan result
git diff --check result
native -fno-exceptions build result
CI/status
asset render count
base-species coverage
NRO filename
exact NRO byte size
exact NRO SHA-256
DEVICE TESTED: NO
```

Then **STOP for physical Switch retesting**.

Do not begin RSE or any later roadmap feature.

---

# 12 — Required physical retest after this coding session

The user will test the new artifact on real hardware.

Required retest flow:

```text
launch under normal Nintendo user
-> FireRed / LeafGreen RetroArch cards visible
-> exactly one parent card per game
-> correct FireRed / LeafGreen GBA artwork
-> only active configured battery-save-root children listed
-> open parent / child list refreshes
-> manual Refresh Saves works
-> choose Main Save
-> Trainer shows correct name, gender, IDs and money
-> Items opens normally
-> all six FRLG inventory containers browse correctly
-> Party works
-> Boxes 1-14 work
-> View Pokémon works
-> Edit/Clone/Transfer/Move/Save/writeback remain blocked
```

Do not begin Ruby/Sapphire/Emerald until the user physically accepts the new artifact.
