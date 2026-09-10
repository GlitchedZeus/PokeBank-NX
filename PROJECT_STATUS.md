# PokeBank NX Project Status

Last updated: 2026-09-10

For the shortest engineering handoff, read `CURRENT_STATUS.md` and then `docs/NEXT_CODEX_PROMPT.md`.

## Project identity

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

Verification vocabulary remains:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
DEVICE ACCEPTED
```

`DEVICE ACCEPTED` is only used when the user physically tested the exact relevant artifact and reported the milestone working.

---

# Current headline — Gen III legacy read-only milestone accepted

The current controller-first Switch app has physically accepted legacy GBA support for:

```text
Ruby
Sapphire
Emerald
FireRed
LeafGreen
```

All five use normal read-only battery-save sources. Savestates are not required as canonical sources.

```text
FireRed GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Ruby GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Sapphire GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Emerald GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

The user also confirmed the existing Switch save paths still open normally in the accepted RSE build.

---

# Accepted FRLG milestone

Acceptance-record checkpoint:

```text
8172ebd9c067bd69df63815dbe865207f905eac6
```

Accepted FRLG runtime source:

```text
d78b76503f02ae26309855970fc5ce0b35c12bcb
```

Physical acceptance covered save discovery/open, profile-scoped assignment, assignment persistence after restart, profile isolation, Trainer, Items, Party, Boxes, Refresh after normal RetroArch in-game save changes, and continued health of the original source saves.

The FRLG path uses strict Gen III rotating-sector validation and an exception-free native backend. Do not reopen FRLG without new physical evidence of a defect.

---

# Accepted RSE milestone

Final accepted application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Exact accepted artifact:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
bytes: 158120837
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Companion ZIP:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.zip
bytes: 151195235
SHA-256: 847788db890523d1ebce070d1427f6230fd89a60ca14986525fd6d2b634d0788
```

Build metadata:

```text
Packaging manifest SHA-256: 3dfa38be0fabde6f45edfb1ee52d87d2f9b1513ec89ba3fa576b0b0ed8da55f9
BUILD_MANIFEST.json SHA-256: 335c1a62b45c2b7c3f2282c424c7c2cd089a6b1178c92fec2bccb027401070ee
SHA256SUMS.txt SHA-256: 33f461dbb17fc2ddbd1ab116d682fcb9871e49e33f0c77465cbed7cff489f664
Actions artifact id: 10143216443
Actions artifact digest: sha256:2daecd936c60719364f5b35b66fa41500be6b5a607146fab70535cbbe5e0fec7
```

Clean host verification:

```text
checkpoint: e5df0e237ba1b7ad0dd78e37a1e1aac609071c3f
run: 34453208654
full host suite: PASS
focused Ruby/Sapphire/Emerald open: PASS
valid R/S/E inventory: PASS
inventory failure rejects whole save: NO
FRLG regression: PASS
source mutation/write policy: PASS
ASan/UBSan: PASS
git diff --check: PASS
```

Native artifact verification:

```text
run: 34454555232
recovery restore: PASS
exact application source/tree: PASS
device asset preflight: PASS
native devkitA64 compile/final link: PASS
embedded application identity: PASS
embedded RomFS: 3286/3286 PASS
packaging/artifact upload: PASS
```

## RSE defects resolved during physical acceptance

The first RSE device build proved the core parser worked but exposed two missing user-facing pieces: RSE Items and R/S/E card artwork. Both were implemented. A follow-up physical build then exposed a stale `FRLGReadOnlyTrainer::populate()` guard which incorrectly treated newly populated RSE inventory as an error and returned to the main menu.

That bridge guard is fixed. Optional inventory-model failure is now isolated from the structurally valid save-open path. Critical Gen III save validation remains strict and independent: sector IDs, signatures, counters, checksums, coherent slot selection and source-family validation still gate the save itself.

The final `a2df4c1a` artifact physically passed Ruby/Sapphire/Emerald opening, browsing, inventory and artwork.

---

# Permanent recovery state

Current complete private GitHub RomFS snapshot:

```text
2321fa488668e32392de25afed84e38919fbd21f
```

Verified baseline:

```text
HD Pokémon renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed / LeafGreen / Ruby / Sapphire / Emerald artwork: PASS
RomFS files: 3286
```

Normal future sessions restore this snapshot through the deterministic recovery tooling. Do not redownload/regenerate the complete sprite set during ordinary coding work.

---

# Current architecture / safety rules

- Stable identity includes release + platform; GBA and Switch FRLG remain separate identities.
- Legacy emulator source identity is based on provider/path/save-container, not automatically the active Nintendo user.
- Profile binding is organizational ownership for legacy saves unless a provider supplies a trustworthy account identity.
- One physical legacy battery save is one source/virtual cartridge.
- Savestates are not canonical save sources.
- Original source bytes remain untouched during read-only milestones.
- Critical save validation and optional UI/read submodels have separate failure states.
- `GameVersion != StructuralRevision`; unknown revisions are never assumed writable.
- Horizon installed-title save transactions and ordinary SD/emulator-file transactions remain separate architectures.
- Live writing stays HARD DISABLED until individually approved adapter safety milestones.

---

# Current support progression

```text
Gen III GBA read-only    PHYSICALLY ACCEPTED
        ↓
Gen I RBY read-only      NEXT
        ↓
Gen II GSC read-only
        ↓
Vault / Banks and wider source coverage continue per roadmap
```

The RetroArch per-Switch-user save-directory modification discussed during testing is a useful later quality-of-life/provider project, but it is intentionally parked so it does not interrupt RBY.

---

# Immediate next milestone — Red / Blue / Yellow

Target:

```text
Pokémon Red
Pokémon Blue
Pokémon Yellow
Game Boy / RetroArch battery saves
READ ONLY
```

The next coding session should use the existing source-provider architecture and PokeBank-owned read models, with pinned PKSM-Core Gen I support as the first correctness reference and independent PKHeX/reference checks where useful.

Required first milestone:

- bounded `.sav`/`.srm` battery-save discovery;
- stable R/B/Y identities;
- strict real Gen I layout/size/checksum validation;
- structural/language variants handled explicitly rather than guessed;
- Trainer;
- Party;
- PC storage/Boxes;
- PK1/Pokémon view mapping;
- Refresh;
- malformed/truncated rejection;
- source-byte immutability;
- Gen III regressions preserved;
- host + sanitizer verification;
- full native devkitA64 build;
- one exact RBY NRO for physical Switch testing.

Do not begin GSC in the same session unless RBY is complete and the user explicitly authorizes it.

---

# Major future product targets

Still planned after source coverage milestones:

- profile-scoped immutable Master Vault;
- named Banks;
- Living/Shiny/Form/Event collection tracking;
- provenance/Journey history;
- search/filter/sort/favorites;
- DS/3DS mainline save support;
- modern Switch per-game validation/hardening;
- legality/oracle integration;
- guided Create Pokémon / Quick Legal;
- explicit Copy/Move/Clone semantics;
- staged write/rollback framework;
- individually accepted write adapters;
- Colosseum/XD and later Stadium preservation work;
- local trading;
- Android companion / Friends / Mailbox / connected exchange in later releases.

---

# Compact next-session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md, docs/CODEX_SESSION.md and docs/NEXT_CODEX_PROMPT.md. This is NOT recovery. Gen III FRLG+RSE is physically accepted; preserve it. Execute only the next Red/Blue/Yellow read-only milestone, push coherent checkpoints early, and never push custom code upstream.
```
