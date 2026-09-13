# NEXT CODEX PROMPT — GEN I STAGED EDITOR UX OVERHAUL

Continue PokeBank NX on `feature/pokebank-playable`.

Use MEDIUM reasoning by default; use deeper analysis only for a concrete UI/native defect.

## THIS IS A CONTINUATION TASK

Do **not** restart the Gen I editor.
Do **not** rewrite the working Gen I save/editor core.
Do **not** begin Gen III boxed editing, Master Vault, DS/3DS, emulator resolver implementation, or live writes.

The exact Gen I boxed staged editor was physically tested on a real Switch.

Hardware-tested application:

```text
Application source: 574d604b3aa2157942fb05d10d846426ffcdf099
Application tree: fc5c30d5d6db04f1e28cb5489ca214a49471f593
Validation run: 34749156054
NRO: PokeBank-NX-Gen1-Editor-574d604b.nro
NRO bytes: 161418993
NRO SHA-256: 76a3bbd93f06072e762821091d77f6f28538b4f47a43a4e85b382a8bc4424415
```

Owner hardware result:

```text
GEN I EDITOR FUNCTIONAL HARDWARE TEST: PASS
GEN I EDITOR UX ACCEPTANCE: FAIL / RETEST REQUIRED
GEN I BOXED STAGED EDITOR DEVICE ACCEPTED: NO
GEN I PARTY EDIT: DEFERRED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE INSTALLED-GAME WRITE: HARD DISABLED
```

Tracking issue: **#62 — Gen I staged editor UX overhaul — functional hardware pass, UI retest required**.

Read first:

- `CURRENT_STATUS.md`
- `PROJECT_STATUS.md`
- `docs/GEN1_EDITOR_HARDWARE_UX_FOLLOWUP_2026-09-13.md`
- issue #62
- existing Gen I editor/UI contract and tests

## FIRST ACTION — RECOVER, DO NOT ASSUME

1. Follow `docs/RECOVERY_CONTRACT.md`.
2. Recover the current remote `origin/feature/pokebank-playable` HEAD.
3. Record exact HEAD/tree before editing.
4. Confirm the hardware-tested application source `574d604b...` is in history.
5. Run the current focused Gen I editor/UI tests before changing UI behavior.
6. Preserve all newer docs/research and all accepted inventory/Gen II work.
7. Keep custom PokeBank NX work on `origin` only; never push it to `kiasta/PKSE`.

## CORE RULE — THIS IS UI POLISH, NOT SAVE-FORMAT REWORK

The hardware test proved that the Gen I staged editor functions on Switch. The problem is the human-facing experience.

Do not touch parser/serializer/editor-core logic unless a new test or reproducible hardware defect proves it necessary.

Preserve:

- immutable original source bytes;
- staged working bytes;
- semantic Pending Changes;
- Add as draft-only until explicit `Stage Add`;
- clone/remove behavior;
- strict export/finalize/reparse validation;
- international editable-layout gate;
- Japanese/unsupported editable layouts fail closed/read-only;
- live installed-game writes HARD DISABLED;
- live RetroArch writes HARD DISABLED;
- live other-emulator writes HARD DISABLED;
- party editing DEFERRED.

## OWNER HARDWARE UX FINDINGS — FIX ALL OF THESE

### 1. Replace the multi-page Add wizard feel

Current Add Pokémon creation feels like too many little windows/pages: choose a few values, move to another step, choose a few more, repeat.

The owner described it as janky and confusing.

Build **one coherent Gen I Pokémon editor workspace** instead of a long wizard chain.

Creation should feel like editing one Pokémon, not filling out 10–15 separate forms.

### 2. Remove confusing right-bumper-as-next-step behavior

The owner specifically reported that the right bumper acts as the next-step button and that this is not intuitive.

Do not preserve a hidden/sequential wizard progression merely because it exists now.

Preferred controller model:

```text
D-pad / Left Stick   Navigate fields/rows
A                    Select / Edit
B                    Back / Cancel
L / R                Previous / Next clearly labeled section/tab
+ or explicit row    Review / Stage action only where appropriate
```

The exact final mapping may differ if existing project conventions require it, but:

- controls must be visible;
- hints must match real behavior;
- next/previous navigation must not feel hidden;
- no accidental mutation may be tied to changing sections.

### 3. Show the Pokémon sprite LIVE while creating

As soon as the draft species changes, display that Pokémon's sprite/art immediately.

The owner should not need to complete `Stage Add` before seeing what Pokémon is being created.

Changing the visual preview must **not** stage save bytes.

Use existing project Pokémon visual assets/resource helpers where possible; do not introduce a network dependency.

### 4. Give creation/editing a persistent full Pokémon overview

The owner wants a full-window understanding of the Pokémon while editing.

The new workspace should keep a persistent preview/summary visible while moving between editable sections.

Generation-correct content should include, where applicable:

```text
Species
Nickname
Level
Experience
Move 1-4
PP
PP Ups
Attack DV
Defense DV
Speed DV
Special DV
HP DV (derived/read-only)
HP Stat Exp
Attack Stat Exp
Defense Stat Exp
Speed Stat Exp
Special Stat Exp
OT
TID
```

Also show calculated/read-only Gen I battle stats if they can be derived correctly from species/base stats + level + DVs + Stat Exp using existing trusted data.

Do **not** invent stored fields just to fill the page.

Still no:

```text
SID
Nature
Ability
Held Item
native shiny flag
native gender field
ribbons
marks
personality value
modern met-data
```

### 5. Fix the species picker layout

Current picker has the number at the far left and the species name at the far right with a huge empty gap.

Replace that presentation with a compact single-label row such as:

```text
001 - Bulbasaur
002 - Ivysaur
003 - Venusaur
...
151 - Mew
```

Number and name must stay visually together.

Keep Gen I species restricted to #001–151.

### 6. Clean up modal/header geometry

Hardware screenshots show presentation problems including crowded/overlapping title/subtitle text and large areas of dead whitespace.

Audit and fix:

- title/subtitle vertical spacing;
- overlapping text;
- clipping;
- modal width/height;
- row alignment;
- padding;
- footer placement;
- label/value hierarchy;
- content density.

The UI should look intentional on 1280×720 Switch handheld/docked output, not like debug menus stacked on top of one another.

### 7. Add and Edit should share the same polished workspace

Editing an existing boxed Pokémon and creating a new boxed Pokémon should use the same visual language and section layout wherever their fields overlap.

For Add, the transaction semantics remain different:

```text
open draft
→ modify draft fields
→ preview freely
→ Cancel = no staged mutation
→ explicit Stage Add = staged mutation
```

Do not call `stageAdd()` during navigation, field selection, species preview, tab changes, or Cancel.

### 8. Keep the complete draft understandable at a glance

A strong target layout is:

```text
┌──────────────────────────────────────────────────────────────┐
│ Gen I Pokémon Editor — DRAFT / Box X Slot Y                 │
├─────────────────────┬────────────────────────────────────────┤
│                     │ Species / Nickname / Lv / EXP         │
│    LIVE SPRITE      │ calculated stats preview              │
│                     │ compact draft status / pending state   │
├─────────────────────┴────────────────────────────────────────┤
│ Summary | Moves | DVs | Stat Exp | Trainer                  │
├──────────────────────────────────────────────────────────────┤
│ current section fields — editable without opening a new     │
│ full-screen modal for every tiny group                       │
├──────────────────────────────────────────────────────────────┤
│ A Edit  B Back/Cancel  L/R Section  Review / Stage Add      │
└──────────────────────────────────────────────────────────────┘
```

This is a UX direction, not a pixel-perfect mandate. Reuse existing project UI primitives and styles.

## DO NOT REGRESS EXISTING ACCEPTED WORK

Inventory hardware milestone remains DEVICE ACCEPTED.

Gen II ordinary item ID 5 remains `Poké Ball`; GS Ball remains separate.

Gen I/II/III legacy read-only paths remain accepted.

Generation II accepted overlay behavior must not regress.

One software-keyboard prompt per frame remains required.

Do not reopen working save-format behavior because of visual polish.

## REQUIRED TEST/CONTRACT UPDATES

Update permanent host/UI contracts so the new UX cannot silently regress.

At minimum cover:

- Add draft navigation does not mutate staged bytes;
- Cancel from draft does not mutate staged bytes;
- `Stage Add` is still explicit;
- species picker formatting is compact, e.g. `001 - Bulbasaur`;
- draft species change updates preview identity/sprite selection without staging;
- section/tab navigation is explicit and deterministic;
- Gen I-only field set remains enforced;
- HP DV remains derived/read-only;
- party edit remains disabled;
- source writes remain hard disabled.

Keep existing Gen I mutation/core tests unchanged unless a legitimate UI-contract integration requires additional coverage.

## VERIFICATION GATES BEFORE THE NEXT NRO

The exact new candidate must pass:

```text
git diff --check
focused Gen I staged editor tests
Gen I UI/UX contract tests
Add draft/cancel mutation safety
species picker formatting contract
preview-selection contract
RBY read-only regression
GSC / Gen II editor regression
FRLG regression
RSE regression
classic inventory regression
machine-label regression
source mutation policy
full permanent host suite
ASan
UBSan
native device asset preflight
devkitA64 clean compile
FINAL NRO LINK
embedded application source identity
embedded RomFS identity
exact artifact packaging
```

Do not weaken or remove a test to make the new UI green.

## HARDWARE RETEST TARGET

Package a new exact source-addressed NRO and stop for owner testing.

The owner should retest primarily:

1. Add Pokémon from an empty Gen I box slot.
2. Confirm the sprite appears immediately while still in draft.
3. Change species and confirm the sprite/identity preview updates immediately.
4. Confirm species rows look like `001 - Bulbasaur`, not distant left/right columns.
5. Navigate Summary / Moves / DVs / Stat Exp / Trainer naturally.
6. Confirm the whole draft remains understandable without a long wizard chain.
7. Cancel a draft and confirm nothing was added.
8. Stage Add and confirm the Pokémon appears staged.
9. Edit an existing Pokémon and confirm it uses the same polished layout.
10. Check for text overlaps, clipping, giant empty dialogs, confusing button hints, or awkward shoulder-button behavior.

## STOP CONDITION

Stop only after the UI overhaul is host/sanitizer/native green and an exact new NRO is ready for physical retest.

Report:

```text
GEN I EDITOR FUNCTIONAL BASELINE:
PRESERVED

GEN I EDITOR UX:
READY FOR HARDWARE RETEST

GEN I BOXED STAGED EDITOR DEVICE ACCEPTED:
NO — PENDING OWNER RETEST

PARTY EDIT:
DEFERRED

LIVE RETROARCH WRITE:
HARD DISABLED

LIVE INSTALLED-GAME WRITE:
HARD DISABLED

APPLICATION SOURCE SHA:
<exact sha>

APPLICATION TREE:
<exact tree>

NRO:
<filename>
<bytes>
<SHA-256>
```

Then stop. Do **not** start the next roadmap milestone until the owner accepts the cleaned-up Gen I editor UI on real hardware.
