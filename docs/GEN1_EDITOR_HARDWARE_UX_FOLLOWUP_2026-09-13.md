# Gen I Staged Editor — Hardware UX Follow-up

Date: 2026-09-13

Tracking issue: #62

## Exact hardware-tested application

```text
Application source: 574d604b3aa2157942fb05d10d846426ffcdf099
Application tree: fc5c30d5d6db04f1e28cb5489ca214a49471f593
Validation run: 34749156054
NRO: PokeBank-NX-Gen1-Editor-574d604b.nro
NRO bytes: 161418993
NRO SHA-256: 76a3bbd93f06072e762821091d77f6f28538b4f47a43a4e85b382a8bc4424415
```

## Owner hardware result

```text
GEN I EDITOR FUNCTIONAL HARDWARE TEST: PASS
GEN I EDITOR UX ACCEPTANCE: FAIL / RETEST REQUIRED
GEN I BOXED STAGED EDITOR DEVICE ACCEPTED: NO
PARTY EDIT: DEFERRED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE INSTALLED-GAME WRITE: HARD DISABLED
```

The owner confirmed that the Gen I boxed staged editor works on a physical Switch, but the creation/edit experience is confusing and visually rough. The next task is therefore a focused UI/interaction overhaul, not a parser/editor-core rewrite.

## Hardware UX findings

### 1. Add Pokémon feels like a multi-page wizard

The current draft creation flow is split across too many separate pages/modals. Moving through a series of small field groups makes the process feel janky and makes it difficult to understand the Pokémon being created as one object.

Required direction: replace the wizard feel with one coherent editor workspace using clearly labeled tabs/sections.

### 2. Step navigation is confusing

The owner reported that the right bumper is effectively used as the next-step control. This is not intuitive and does not feel like normal Switch editor navigation.

Required direction: use visible, controller-native navigation. Prefer L/R for clearly labeled tabs/sections, A for select/edit, B for back/cancel. Footer hints must describe the controls that actually operate the current screen.

### 3. Draft Pokémon has no live sprite preview

A selected species does not visibly become a Pokémon until after creation is staged.

Required direction: as soon as draft species changes, immediately show its sprite/art in the creation workspace. The preview must remain draft-only and must not stage bytes merely because the preview changed.

### 4. No persistent complete draft overview

The owner wants to see the Pokémon as a complete object while creating it instead of opening a chain of isolated field windows.

The main editor should provide a persistent overview with, where generation-correct:

- species;
- nickname;
- level;
- experience;
- moves / PP / PP Ups;
- Attack / Defense / Speed / Special DVs;
- derived HP DV;
- HP / Attack / Defense / Speed / Special Stat Exp;
- OT;
- TID;
- calculated/read-only Gen I battle-stat preview where correct and useful.

Calculated preview stats are presentation only; do not invent stored fields.

### 5. Species picker row layout is poor

Current layout visually separates the dex number on the far left from the species name on the far right.

Required format family:

```text
001 - Bulbasaur
002 - Ivysaur
003 - Venusaur
...
151 - Mew
```

Number and species name should read as one row label rather than two distant columns.

### 6. Modal geometry and text hierarchy need cleanup

Hardware screenshots show crowded/overlapping title/subtitle text in some action dialogs and excessive dead space in others.

Audit:

- title/subtitle vertical spacing;
- text clipping/overlap;
- row alignment;
- modal width/height;
- footer placement;
- useful content density;
- consistent padding;
- visual priority between labels and values.

### 7. Add and Edit should share one visual language

Existing-Pokémon Edit and new-Pokémon Add should use the same polished Gen I editor workspace wherever the editable field set overlaps.

Add differs only in transaction semantics: it is a draft and must not mutate staged save bytes until explicit `Stage Add`.

## Target interaction model

A good target is a single large editor workspace with:

```text
[ Sprite / identity / Lv ]    [ live full draft summary ]

Summary | Moves | DVs | Stat Exp | Trainer
------------------------------------------------
current section fields
------------------------------------------------
A Edit/Select   B Back/Cancel   L/R Section   X Review (if useful)
                         explicit Stage Add only for a draft
```

Exact geometry is implementation-dependent, but the user must be able to understand the whole Pokémon without navigating through a long chain of modal steps.

## Safety invariants — unchanged

- Original source `.srm` remains immutable.
- Live RetroArch writes remain HARD DISABLED.
- Live installed-game writes remain HARD DISABLED.
- Other emulator-source writes remain HARD DISABLED.
- Add remains draft-only until explicit `Stage Add`.
- Cancel before `Stage Add` performs no staged mutation.
- Gen I species remain #001-151 only.
- Gen I move IDs remain generation-correct.
- HP DV remains derived/read-only.
- No SID, Nature, Ability, Held Item, ribbons, marks, personality value or modern met-data fields.
- Party editing remains deferred.
- Unsupported editable layouts fail closed/read-only.
- Preserve Gen II accepted behavior and all classic inventory acceptance.
- Do not weaken source-safety, sanitizer or native gates.

## Retest acceptance target

The next exact NRO should be physically retested for usability after host/sanitizer/native validation.

Do not mark `GEN I BOXED STAGED EDITOR: DEVICE ACCEPTED` until the owner confirms the cleaned-up UI is comfortable and understandable on the physical Switch.

Do not start Gen III boxed editing, Master Vault, DS/3DS work, emulator resolver implementation, or live writeback before this UX follow-up is accepted.
