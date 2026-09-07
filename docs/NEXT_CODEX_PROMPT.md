# PokeBank NX — Next Codex Prompt

Use this file as the authoritative task prompt for the next coding session.

## Launcher

The user should only need to send:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

---

## Current verified starting point

The completed RetroArch FRLG runtime-discovery implementation is:

```text
54cb86892d290ae1c80f447af427ff17192681f9
gen3: wire RetroArch FRLG read-only sources
```

Its parent is the prior synchronized documentation handoff:

```text
cde442764962d7dc61084c6f91c46f50fb6f0e22
docs: harden Codex recovery handoff
```

Do **not** redo the interrupted-work recovery. The useful work is already committed and pushed.

Do **not** redo Session 3A or the exception-free native Gen III backend.

Relevant earlier verified checkpoints remain:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a  PKSM-Core FRLG host adapter
283073a5215a471ef0ad07619b4856409658cfdc  recursive PKSM-Core CI checkout
43f3a9f90a3314725979d59afdd68f19ee159009  exception-free native Gen III backend
54cb86892d290ae1c80f447af427ff17192681f9  RetroArch FRLG runtime discovery/lifecycle wiring
```

## Before editing

Preserve local state first.

Inspect at minimum:

```text
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git remote -v
git log --all --oneline --decorate --graph -40
git reflog -40
git stash list
git worktree list
git diff
git diff --cached
git submodule status --recursive
```

If there is useful local/uncommitted work, preserve it before syncing or changing refs.

Never run destructive cleanup/reset commands until useful local state is known safe.

Writable destination remains:

```text
origin / feature/pokebank-playable
```

Never push PokeBank NX custom code to `kiasta/PKSE` upstream.

## What is already implemented

`54cb8689...` already provides:

- configured RetroArch `savefile_directory` handling;
- `.sav` / `.srm` only;
- bounded traversal (depth <= 2, 256 candidates by default);
- strict FRLG structural validation;
- identity hints considered only after structural validation;
- ambiguous FRLG-family files remain unclassified;
- `firered_gba` / `leafgreen_gba` identity separation from Switch FRLG;
- existing native Gen III Party/Boxes read model;
- application-lifecycle ownership of the resulting `legacyFRLGSources` catalog;
- real runtime invocation/retention of the provider;
- read-only source behavior.

Do not create another parser/provider to solve work that is already complete.

## Current honest boundary

The app now discovers, validates, classifies where reliable, parses and retains RetroArch FRLG sources, but those validated records are **not yet exposed as normal selectable Game Sources cards/entries in the existing user-facing source browser**.

That is the entire mission for this session.

## Mission

Wire the already-populated validated `legacyFRLGSources` records into the existing PokeBank source-selection/browser lifecycle so a user can select a RetroArch FireRed/LeafGreen GBA source and browse its Party/Boxes through the normal application flow.

Target architecture:

```text
RetroArch savefile_directory
        -> existing bounded FRLG discovery
        -> validated FRLGSource / ReadOnlySave
        -> existing PokeBank source/card descriptor model
        -> user-facing FireRed GBA / LeafGreen GBA source card
        -> select card
        -> existing Party / Boxes browsing path
```

Use the existing UI/source infrastructure. The goal is integration, not a new browser.

## Source-card requirements

For each `Ready` source that has a reliable identity:

- expose a normal selectable source/card entry in the existing Game Sources flow;
- clearly identify the game as **FireRed GBA** or **LeafGreen GBA**;
- make the platform/source origin clear enough that it cannot be confused with official Switch FireRed/LeafGreen;
- retain the exact validated `ReadOnlySave`/source model rather than reparsing on every screen where avoidable;
- selecting the source must lead to the existing Party/Boxes browsing experience;
- source data must remain read-only.

Potential label concepts are acceptable if they match existing UI conventions, for example:

```text
FireRed — Game Boy Advance
RetroArch Save
```

or equivalent existing source metadata fields. Do not redesign the card system solely for this milestone.

## Invalid / ambiguous behavior

Do not expose an invalid source as a selectable Pokémon game source.

Do not guess FireRed vs LeafGreen.

For `AmbiguousIdentity`, malformed, unreadable or scan-limit cases, preserve safe behavior. If the existing UI has an appropriate passive diagnostics/status path, use it minimally; otherwise leave these records non-selectable and keep the implementation honest.

Do not add a second diagnostics browser just for this session.

## UI rule

Broad UI work is still frozen.

Only make the minimum UI/source-routing changes required to make the existing validated RetroArch FRLG sources genuinely selectable and browsable.

Do not:

- redesign the home screen;
- start final branding/startup work;
- implement Right Stick work;
- replace the accepted red identity;
- create a second legacy-save browser.

## Safety

Everything remains read-only.

Do not implement or enable:

- RetroArch save modification;
- save repair/resign/writeback;
- installed Switch title writes;
- true Move;
- conversion UI;
- Master Vault/Banks;
- event/mystery-gift work;
- physical hardware/link-cable work;
- unrelated generations.

Preserve the existing installed-save hard lock.

Issues #46 and #47 are explicitly parked post-v1/later and must not be started here.

## Verification

Preserve all existing host coverage and add focused tests for the new source-card/descriptor/routing seam where practical.

At minimum verify:

- validated `Ready` FRLG records become eligible source entries;
- FireRed GBA and LeafGreen GBA remain distinct;
- neither can collide with `firered_switch` / `leafgreen_switch`;
- ambiguous/invalid records do not become valid selectable cards;
- selecting/routing a legacy FRLG entry reaches the existing Party/Boxes read model;
- no write capability is introduced;
- source bytes remain unchanged;
- no duplicate reparsing/provider path was added without need.

Run:

```text
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Native build must remain `-fno-exceptions`.

If a device-testable `.nro` is produced, record:

```text
source SHA
artifact filename
artifact size
SHA-256
```

Do not call it `DEVICE TESTED` until that exact artifact is physically run on Switch.

## Checkpoint policy

Commit/push coherent work early to:

```text
origin / feature/pokebank-playable
```

Suggested implementation commit:

```text
gen3: expose RetroArch FRLG game sources
```

Update only the minimal handoff/status files and issue #6 as needed after the implementation is actually verified.

## Stop condition

Stop after the RetroArch FRLG sources are genuinely selectable through the existing PokeBank source browser and Party/Boxes can be reached through that normal flow, with tests/native build clean and the checkpoint pushed.

Do **not** continue into Ruby/Sapphire/Emerald in the same session.

Do **not** start Master Vault/Banks, Gen I/II, DS/3DS, events, physical-link hardware, final UI polish or live writes.

End report should include:

```text
starting remote SHA
implementation SHA(s)
source-card/descriptor files changed
routing path from FRLGSource to existing browser
visible labels/platform distinction
Party/Boxes browse result
ambiguous/invalid behavior
source immutability
host tests
ASan/UBSan
git diff --check
native build
GitHub CI/status if available
NRO filename/size/SHA-256 if produced
remaining blocker
exact next coding task
```
