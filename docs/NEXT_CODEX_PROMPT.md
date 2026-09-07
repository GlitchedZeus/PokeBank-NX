# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

## Current verified application source

```text
f6a3052daeffe7cd30d7acceba81a5dfda7615ee
gen3: expose RetroArch FRLG game sources
```

The exact device-tested artifact was:

```text
NRO size:    155174825 bytes
NRO SHA-256: 809c94c842a3c385d23907c61e5ecfa201b24f08e8ac935e87a4add60770282d
Assets:      3260 HD renders / 1025 base species / 3281 RomFS files verified
```

Physical testing proved the main FRLG read-only browser path works, but device acceptance is blocked by two integration bugs. Do not call this artifact PASS/DEVICE-ACCEPTED.

## First: preserve local work

Before syncing/resetting/cleaning/restoring/changing refs, inspect and preserve useful local state:

```text
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -40
git reflog -30
git stash list
git worktree list
git diff
git diff --cached
git submodule status --recursive
```

A previous interrupted RSE checkpoint may exist locally at:

```text
1a921515
```

Preserve it if present, but do not resume/merge/push RSE work in this session.

Writable destination:

```text
origin / feature/pokebank-playable
```

Never push custom PokeBank code upstream to `kiasta/PKSE`.

## Single mission: fix the two FRLG hardware blockers

### Blocker A — legacy RetroArch sources are wrongly user/profile scoped

Observed on physical Switch: RetroArch appears as its own user/profile context, and the user must switch to that "account" to see the legacy FRLG saves.

Required architecture:

```text
installed Switch save source
    -> Nintendo user/account scoped when required

RetroArch / file / legacy source
    -> app-global
    -> visible regardless of currently selected Nintendo user
```

Do not model RetroArch itself as a Switch user. Do not require selecting a synthetic/legacy profile to expose file-based legacy sources.

Integrate the app-global legacy cards into the existing Game Sources experience with the smallest coherent change. Preserve existing installed-title user handling.

### Blocker B — identical legacy saves appear multiple times

Observed on physical Switch:

```text
2 FireRed cards for the same save
3 LeafGreen cards for the same save
```

Inspect the actual cause first. Candidate causes include configured + conventional roots resolving to overlapping locations, root aliases, duplicate traversal roots, symlinks/path aliases, or source-card assembly duplicating the same catalog record.

Required behavior:

- one card for one underlying logical save source;
- multiple genuinely distinct FireRed or LeafGreen save files are allowed and must remain separately selectable;
- never dedupe merely by game ID/species/trainer name;
- strict FRLG validation still happens before a source becomes selectable;
- normalize/deduplicate approved roots before traversal where appropriate;
- dedupe discovered sources using robust source identity. Prefer canonical/normalized path or filesystem identity when available; content hashing may be used as a secondary signal, but do not accidentally collapse intentionally separate physical files solely because their bytes currently match unless product semantics explicitly define them as aliases of the same source;
- if a single file is reachable through multiple aliases/roots, show it once.

Add host-testable coverage for overlapping roots/aliases and source-card uniqueness where practical.

## Preserve everything that already works

Do not redo or regress:

- PKSM-Core Gen III host oracle;
- exception-free native Gen III backend;
- bounded `.sav` / `.srm` scanning;
- strict FRLG structural validation;
- `firered_gba` / `leafgreen_gba` identity separation from Switch FRLG;
- existing Party and all 14 Boxes read model;
- exact validated catalog-entry routing;
- `Game Boy Advance` / `RETROARCH` labeling;
- read-only action policy;
- accepted broad UI identity.

The user reported that aside from the two blockers, the physical FRLG browse path seems to work.

## Safety

This remains strictly read-only.

Do not implement/enable:

- RetroArch save modification;
- repair/resign/writeback;
- installed-title writes;
- editing back into source saves;
- clone-to-save;
- Move;
- conversion;
- Master Vault/Banks;
- RSE;
- Gen I/II;
- events/mystery gifts;
- physical-link hardware.

Disabled actions must remain safely blocked.

## Verification

Run all existing coverage plus focused tests for both fixes:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native application must remain `-fno-exceptions`.

Specifically prove where practical:

- legacy source availability is independent of Nintendo user selection;
- installed-title user scoping is not accidentally broken;
- overlapping configured/conventional roots do not duplicate the same source;
- the same file reached through path aliases does not produce multiple cards;
- two genuinely distinct save files may both appear;
- FireRed and LeafGreen identities remain exact;
- Party/Boxes/View still work;
- source bytes remain unchanged;
- blocked actions remain blocked.

## Checkpoint policy

Commit/push coherent source work early to `origin/feature/pokebank-playable` only.

Suggested commit concept:

```text
gen3: fix RetroArch source scope and dedupe
```

Update issue #6 and the minimal status/handoff docs after verification.

## Device artifact gate

After fixes pass host/sanitizer/native verification, restore/use the full pinned 3260-render asset set and package a **new exact device-test NRO**.

Report:

```text
application/source SHA
NRO filename
NRO size
NRO SHA-256
asset render count
host tests
ASan/UBSan
git diff --check
native build
CI/status
DEVICE TESTED: NO
```

Then STOP for physical Switch retest.

Required retest:

```text
launch under normal Nintendo user
-> FRLG RetroArch sources are visible without switching to a RetroArch profile
-> exactly one card per underlying FRLG save
-> FireRed/LeafGreen GBA identities correct
-> Party works
-> Boxes 1-14 work
-> View Pokémon works
-> Edit/Clone/Transfer/Move/Save/writeback blocked
```

Do not begin Ruby/Sapphire/Emerald until the user physically accepts the new artifact.

## Stop condition

Stop after one coherent pushed FRLG blocker-fix checkpoint and a newly hashed full-asset NRO ready for device retest.

Do not resume the parked `1a921515` RSE work in this session.

End report:

```text
starting remote SHA
recovered/preserved local refs
root cause of user/profile bug
root cause of duplicate cards
implementation SHA(s)
source-scope behavior
source-deduplication key/logic
host tests
ASan/UBSan
git diff --check
native build
CI/status
NRO filename/size/SHA-256
asset verification
DEVICE TESTED: NO
remaining blocker
exact physical retest steps
```
