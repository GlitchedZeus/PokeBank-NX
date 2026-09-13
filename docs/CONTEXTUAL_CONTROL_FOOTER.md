# PokeBank NX — Contextual Control Footer Contract

Status: REQUIRED APP-WIDE UI CONTRACT

This document defines the permanent control-hint/footer behavior for PokeBank NX.

## Core rule

Every interactive screen, editor, picker, modal, dialog, overlay, list, and confirmation view must show the controls that are valid **for that exact current UI state** in the persistent bottom control footer.

The user must never have to guess what A, B, X, Y, L, R, ZL, ZR, +, -, D-pad, or Left Stick do on the current screen.

This is an app-wide rule, not a Gen I-only special case.

## Footer behavior

When the active screen/modal changes, the footer must update immediately to describe the new active controls.

When that modal closes, the footer must restore the controls for the newly revealed parent screen.

Nested modals must always show the controls for the topmost active modal, not stale controls from the screen behind it.

Do not display a control that is unavailable, disabled, read-only, or does nothing in the current state.

Examples:

```text
Boxes — editable staged source
A Actions   X Add   Y Remove   L/R Box   B Back

Boxes — read-only source
A View   L/R Box   B Back

Pokémon editor
D-pad/Stick Navigate   A Edit   L/R Section   B Back

Add Pokémon draft workspace
D-pad/Stick Scroll   A Edit/Select   L/R Jump Section   X Stage Add   B Cancel Draft

Species picker
D-pad/Stick Navigate   A Select   L/R Page   B Cancel

Clone dialog
Left/Right Destination Box   A Stage Clone   B Cancel

Confirmation dialog
A Confirm   B Cancel
```

The exact labels can vary when a screen has different capabilities, but the displayed footer must match the actual input handling exactly.

## Read-only behavior

If a source is read-only, the footer must not advertise unavailable edit actions.

For example, do not show `A Edit`, `X Add`, or `Y Remove` on a source where those actions cannot be performed.

Read-only state and staged-edit state must be visually and behaviorally distinct.

## Control-label correctness

A footer hint is part of the UI contract. If the footer says:

```text
X Add
```

then X must actually perform Add in that UI state.

If the action changes because a modal opened, the footer changes with it.

No stale hints.
No hidden primary controls.
No important action that exists only in code with no visible hint unless it is a deliberately advanced/non-primary shortcut.

## Gen I editor application

Issue #62 requires this footer contract during the Gen I staged-editor UX overhaul.

The redesigned Add/Edit workspace should use a single coherent scrollable editor rather than a multi-page wizard. The footer must make the current controls obvious throughout that workspace.

Recommended draft editor footer:

```text
D-pad/Stick Scroll   A Edit/Select   L/R Jump Section   X Stage Add   B Cancel Draft
```

Recommended existing-Pokémon editor footer:

```text
D-pad/Stick Scroll   A Edit   L/R Jump Section   B Back
```

If implementation chooses a different final mapping, the mapping must still follow this contract: visible, current, truthful, controller-native, and deterministic.

## Reusable implementation direction

Prefer one reusable footer/control-hint component or data model rather than hard-coding unrelated strings independently in every screen.

A strong architecture would allow each active screen/modal to publish a list of control hints, for example conceptually:

```text
ControlHints {
    Navigate,
    A: Edit,
    X: Add,
    Y: Remove,
    L/R: Section,
    B: Back
}
```

The renderer should then draw only the controls supplied by the active UI state.

The exact C++ API is implementation-defined; this document specifies behavior, not a required class name.

## Required correctness cases

Permanent UI tests/contracts should cover at least:

- footer changes when entering a modal;
- footer restores when leaving a modal;
- read-only sources do not advertise edit controls;
- staged editable screens advertise only actions that actually exist;
- Add draft shows explicit Stage Add and Cancel Draft controls;
- species picker shows Select/Cancel/navigation controls;
- clone/remove confirmations expose truthful Confirm/Cancel controls;
- no stale parent-screen controls remain visible over a child modal;
- button labels match actual input handlers.

## Safety

This footer work is presentation/input discoverability only.

It must not weaken the existing save safety model:

```text
LIVE RETROARCH WRITE: HARD DISABLED
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
PARTY EDIT: DEFERRED
ORIGINAL SOURCE SAVES: IMMUTABLE
```

A visible control hint never authorizes a write path that is otherwise disabled.

## Acceptance rule

A UI screen is not considered polished/accepted if a normal user can reasonably land on it and ask:

> What are the controls for this page?

The current window must answer that question itself through its bottom control footer.
