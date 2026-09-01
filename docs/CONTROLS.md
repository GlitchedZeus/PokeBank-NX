# PokeBank NX — Controller Map

Status: TARGET INPUT CONTRACT  
Last updated: 2026-09-01

PokeBank NX should use a control scheme that feels immediately familiar to players of the Nintendo Switch version of Pokémon HOME.

This is a behavioral inspiration, not a requirement to clone every exact HOME binding on every screen. PokeBank NX has additional concepts such as Master Vault, provenance, Banks, source adapters, and safety preflights, so some controls must be contextual.

The rule is: **familiar first, consistent second, clever never.**

---

# Global map

```text
D-pad              precise focus/navigation
Left Stick         focus/navigation with held repeat
A                  Select / Open / Confirm deliberate action
B                  Back / Cancel / Close
X                  Filter / Search / context action
Y                  Sort / View Mode / Marking-style secondary action
L                  Previous box / tab / Pokémon
R                  Next box / tab / Pokémon
ZL                 Large jump backward / previous major section
ZR                 Large jump forward / next major section
+                  More / Options / Compatible Games / screen-specific utility
-                  Help / Controls / Screen Info
Right Stick        fast scroll or secondary-pane navigation where useful
Left Stick Click   unassigned by default
Right Stick Click  unassigned by default
```

Stick-click actions should remain unused until there is a genuinely useful function that cannot be expressed more clearly elsewhere.

---

# Core rules

## A

A is the primary select/open button.

When a Pokémon is focused:

```text
A -> Pokémon Action Sheet
```

A never performs an immediate mutation from a browsing grid.

## B

B is always the safest exit path:

```text
modal -> cancel/close
summary -> previous screen
subscreen -> previous screen
```

B never commits a pending mutation.

## D-pad / Left Stick

Both operate the same focus graph.

D-pad gives precise movement; Left Stick supports comfortable held-repeat.

## L / R

Prefer these for the navigation pattern Pokémon HOME users expect:

```text
previous/next box
previous/next Pokémon
previous/next adjacent tab
```

Exact use depends on screen context and must be shown in the bottom hint bar.

## ZL / ZR

Use for accelerated navigation only:

```text
jump several boxes
previous/next major category
page jump through large lists
regional Pokédex section jump
```

Ordinary navigation must remain possible without ZL/ZR.

---

# + Button

The + button must always have a useful, labeled purpose on major screens.

It is the **More / Options** button, but becomes more contextually useful where appropriate.

## Home / Select Game

```text
+ -> Quick Options
```

Quick Options can contain:

```text
Appearance / Theme
Refresh Sources
Storage / Vault Status
Settings
About / Version
```

## Party / Boxes / Master Vault / Banks

```text
+ -> Screen Options
```

Possible options:

```text
Appearance
Refresh / Rescan
Jump to Box
Storage Info
Screen Help
```

Do not place destructive actions in this menu merely because space is available.

## Pokémon Summary

Use a Pokémon HOME-like behavior:

```text
+ -> Compatible Games / Transfer Compatibility
```

This should show where the selected Pokémon can currently be transferred according to PokeBank NX's supported rules.

Result states:

```text
SUPPORTED
SUPPORTED WITH CHANGES
UNSUPPORTED
UNKNOWN
```

During early development this view may be partial, but it must distinguish UNKNOWN from supported.

This is especially useful for the future official-game HOME Bridge workflow.

## Pokédex

```text
+ -> Species Options / Compatibility
```

Useful contents may include:

```text
Compatible Games
View Vault Specimens
Forms
Regional Dex Membership
```

---

# - Button

The - button is the **Help / Controls / Screen Info** button.

Every major screen should be able to open a lightweight overlay explaining:

```text
what this screen is
what the controls do
whether the current source is read-only
what selected platform/source means
relevant safety state
```

Examples:

## Home

```text
- -> Controls + source legend
```

## Game Boxes

```text
- -> Controls + read-only/write-capability status
```

## Master Vault

```text
- -> Explain immutable originals, derived records, Banks
```

## Pokémon Summary

```text
- -> Explain legality/provenance labels and controls
```

## Pokédex

```text
- -> Explain owned/missing/shiny/form markers
```

This keeps Help useful rather than making - a dead button.

---

# X / Y conventions

Exact behavior may be contextual, but keep the mental model stable.

Preferred:

```text
X = Filter / Search / Context
Y = Sort / View Mode / Marking-style action
```

Examples:

## Home

```text
X Filter by platform/source
Y Sort games
```

## Boxes / Vault

```text
X Search / Filter
Y Sort or change view mode
```

## Pokédex

```text
X Filter
Y Sort
```

If a screen has no sensible X or Y function, omit that hint instead of inventing filler.

---

# Right Stick

Right Stick is optional and must never be required for core functions.

Good uses:

```text
fast scrolling through very long lists
scroll a secondary detail pane while left stick keeps grid focus
rotate a future 3D Pokémon preview
```

Do not bind irreversible actions to stick movement/clicks.

---

# Screen examples

## Select Game

```text
D-pad / LS   Navigate cards
A            Select
B            Back
X            Filter
Y            Sort
+            Quick Options
-            Help
```

## Party / Boxes

```text
D-pad / LS   Navigate Pokémon
A            Actions
B            Back
L/R          Previous / Next Box
ZL/ZR        Jump Boxes
X            Filter / Search
Y            Sort / View
+            Options
-            Help / Source Status
```

## Pokémon Action Sheet

```text
D-pad / LS   Navigate actions
A            Select
B            Cancel
-            Help (optional if useful)
```

Do not overload the modal with unnecessary shortcuts.

## Pokémon Summary

```text
D-pad / LS   Move within page if needed
B            Back
L/R          Previous / Next Pokémon
ZL/ZR        Previous / Next Summary Tab
+            Compatible Games
-            Help / Label Explanation
```

## Master Vault / Bank

```text
D-pad / LS   Navigate
A            Actions
B            Back
L/R          Previous / Next Box
ZL/ZR        Jump Boxes / Sections
X            Filter / Search
Y            Sort / View
+            Options
-            Help / Vault Explanation
```

## Pokédex

```text
D-pad / LS   Navigate species
A            Open species / Vault matches
B            Back
L/R          Previous / Next Dex tab
ZL/ZR        Jump section
X            Filter
Y            Sort
+            Species Options / Compatible Games
-            Help / Legend
```

---

# Bottom hint bar

The on-screen button strip is part of the control contract.

Examples:

```text
A Select    B Back    X Filter    Y Sort    + Options    - Help
```

```text
A Actions   B Back   L/R Box   ZL/ZR Jump   X Filter   + Options   - Help
```

```text
B Back    L/R Pokémon    ZL/ZR Tab    + Compatible Games    - Help
```

Do not show inactive buttons.

---

# Safety rule

No control mapping may bypass the application safety architecture.

Especially:

```text
A
X
Y
+
-
L/R
ZL/ZR
stick clicks
```

must never become hidden shortcuts to live-save mutation.

All mutation-capable workflows enter through a deliberate action, preflight, and the capability/safety model documented elsewhere in the project.

---

# Device-test acceptance

On physical Switch, explicitly test:

- D-pad and Left Stick feel interchangeable;
- held-stick repeat is not too fast;
- A/B behavior feels Pokémon HOME-like;
- L/R box navigation is natural;
- ZL/ZR jumps do not cause accidental context changes;
- X/Y meanings are understandable from hints;
- + has a useful contextual action on every major screen;
- - reliably opens useful Help/Info;
- Right Stick does not interfere with normal navigation;
- no button performs an undocumented mutation.
