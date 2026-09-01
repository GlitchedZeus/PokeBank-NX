# PokeBank NX — Controller / UI Flow Contract

Status: TARGET UX CONTRACT  
Last updated: 2026-09-01

PokeBank NX is a Nintendo Switch application first. Primary flows must be comfortable with Joy-Con/Pro Controller input, predictable with A/B navigation, and safe against accidental mutation.

---

# Global controls

Target conventions:

```text
D-pad / Left Stick  Navigate
A                   Select / open deliberate action
B                   Back / cancel
L / R               Previous / next box or tab
ZL / ZR              Larger jumps where useful
X / Y                Screen-specific context/filter actions
+                    Optional screen/settings action where explicitly labeled
```

Never assign a destructive action to a button without an intermediate confirmation/menu.

Button hints shown on screen must match actual behavior.

---

# Home

Home presents **sources/games**, not a desktop file picker.

Card concept:

```text
[art/icon]
Pokémon FireRed
Game Boy Advance
RetroArch
```

or:

```text
[art/icon]
Pokémon Scarlet
Nintendo Switch
Installed Save
```

Each card should distinguish:

```text
game title
platform
source kind
```

Do not use display title alone as identity.

Selecting a source:

```text
Home
  ↓ A
Game overview / Party + Boxes
```

---

# Game overview

Preferred first usable layout:

```text
<Game / Platform>

[ Party ] [ Boxes ]

current selected box / party grid
```

The exact visual layout can evolve, but Party and Boxes should feel like one browsing system rather than separate editing utilities.

Navigation:

```text
L/R       previous/next box
ZL/ZR     larger box jump if useful
A         Pokémon action sheet
B         Home/source list
```

---

# Pokémon focus

Moving onto a Pokémon highlights it and can show lightweight preview information.

**Merely focusing a Pokémon never changes data.**

Pressing A never executes a mutation directly.

Instead:

```text
Focused Pokémon
      ↓ A
Pokémon Action Sheet
```

This is the current issue #2 contract.

---

# Pokémon action sheet

Target order:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

The order can receive minor UX adjustment if device testing shows a strong reason, but `View Pokémon` should remain the obvious safe/default first action and `Cancel` must be available.

## Behavior rules

- opening the action sheet performs no mutation
- B closes with no mutation
- Cancel closes with no mutation
- moving focus between actions performs no mutation
- unavailable features are disabled or give an explicit not-yet-supported explanation
- an unavailable option must not fall through into another action
- source capabilities determine what can be offered
- live write state is never inferred solely from game detection

---

# View Pokémon

Safe read-only action.

Flow:

```text
Action Sheet
  ↓
Summary
```

Summary can have tabs/pages:

```text
Overview
Stats
Moves
Origin
Legality & Provenance
```

Fields must be generation-aware.

B returns to the previous context without mutation.

---

# Add to Master Vault

Future flow once Vault v1 exists:

```text
Action Sheet
  ↓
preflight source/raw representation
  ↓
Vault transaction
  ↓
Success: Added to Master Vault
```

Source remains untouched.

If an identical raw hash already exists, apply the Vault duplicate policy rather than silently discarding/overwriting.

Early UI should favor a quick safe copy; it does not need a giant confirmation dialog for every normal Vault addition unless a warning/error exists.

---

# Add to Bank…

If source Pokémon is not yet a Vault entity:

```text
Add to Bank…
   ↓
add/import to Vault safely
   ↓
choose Bank/Box
   ↓
create Bank reference
```

If already in Vault:

```text
choose Bank/Box
   ↓
add reference
```

Default semantic is copy/reference, not removal from source.

---

# Transfer to Game…

This action always begins with planning, never immediate writing.

```text
Transfer to Game…
      ↓
Destination list
      ↓
Compatibility/preflight
      ↓
Changes/warnings
      ↓
Convert/create derived entity
      ↓
Export / stage / future verified write
```

While live writing is hard disabled, destination actions that would require it must clearly stop before modifying an installed save.

Do not hide the safety lock by silently exporting somewhere and calling that a completed game transfer.

---

# Edit

Editing should operate on a working/derived representation.

For a Vault entity, preferred semantics:

```text
source Vault entity
    ↓ Edit
working copy
    ↓ Save edit
validate
    ↓
new derived Vault entity
```

For a Pokémon viewed directly from a game source, do not mutate the game in place during the read-only alpha. Either require Add to Vault first or clearly create a derived Vault copy.

B/cancel exits editor without committing.

---

# Clone

Clone should make the provenance semantics obvious without becoming annoying.

```text
Clone
  ↓
new Vault ID
same initial raw bytes
parent = source Vault ID
operation = clone
```

If source is not in Vault, import source first or use a combined safe flow.

Clone never means "duplicate directly into the live game slot beside it" during read-only alpha.

---

# Make Shiny

Flow:

```text
Make Shiny
    ↓
check source/origin/generation
    ↓
shiny locked?
  yes -> refuse/explain
  no/known supported -> generate derived representation
  unknown -> conservative warning/refusal depending on support
    ↓
legality check
    ↓
new derived Vault entity
```

Never simple one-press in-place bit flipping.

---

# Legality & Provenance

Read-only view.

Show at minimum when known:

```text
legality: LEGAL / INVALID / UNKNOWN
origin game
origin platform
source kind/save fingerprint
Vault ID
raw format/hash
parent Vault ID
current location
transform history
transfer history
warnings
unsupported checks
```

Generated Pokémon must remain labeled as generated.

---

# Vault Home

Target top-level Vault flow:

```text
Home
  ├── Games / Sources
  ├── Master Vault
  ├── Banks
  ├── Pokédex
  └── Settings
```

The exact navigation shell can evolve, but these are separate concepts:

- Master Vault = authoritative persistent Pokémon entities
- Banks = organization references
- Pokédex = indexed collection view
- Games/Sources = external/read-only or future writable save contexts

---

# Master Vault browsing

Expected controls should mirror game box browsing where possible so users do not learn two completely different interfaces.

Useful context actions:

```text
View
Add to Bank
Clone
Transfer
Edit derived copy
Make Shiny derived copy
Provenance
Favorite/tag
```

Permanent deletion should not be a prominent one-button action.

---

# Banks

A Bank can use familiar box-grid UX.

Potential flow:

```text
Banks
  ↓
Bank list
  ↓
Bank boxes
  ↓
Vault entity
  ↓
action sheet
```

Removing an entry from a Bank removes the reference, not the Master Vault entity.

Label that distinction clearly.

---

# Pokédex

Primary views:

```text
National
Regional
Living
Shiny
Missing
```

Selecting an owned species should lead to actual matching Vault specimen(s).

No manual `Caught` toggle should override Vault ownership.

---

# Settings

Planned categories:

```text
Appearance
Storage / Vault paths
Source discovery
Backups / integrity
About / version
Developer diagnostics (when appropriate)
```

Theme targets:

```text
OLED Black
Dark
Light
```

Do not bury live-write enablement behind a settings toggle. Future write capability is an adapter/safety feature, not a dangerous global switch.

---

# Startup

Startup should show real progress stages, not a fake timer.

Target stages:

```text
settings
storage
services
game registry
Pokémon data
sprites
Vault
Banks
save scan
ready
```

Failure of an optional source should not necessarily prevent opening the Vault; show graceful warnings and continue when safe.

---

# Error/dialog rules

Good error:

```text
Pokémon FireRed save could not be opened.
The save was not modified.

Details: invalid/missing Gen III sectors.
```

Bad error:

```text
Error 17
```

For safety-sensitive operations, always say whether source/live data was modified.

---

# Current action-sheet implementation acceptance tests

Issue #2 should test at least these behaviors:

```text
A on occupied Pokémon -> action sheet
A on empty slot -> no Pokémon mutation/action sheet as appropriate
B from action sheet -> original screen, no mutation
Cancel -> original screen, no mutation
View -> details/read-only flow
unsupported option -> safe disabled/message
repeated open/close -> stable
Party path -> same menu semantics
Boxes path -> same menu semantics
Storage path -> same menu semantics where applicable
```

Add a regression test or separable action-dispatch model so future refactors cannot quietly turn A back into an instant mutation.

---

# Device-test UX questions

When the first `.nro` is tested, explicitly note:

- Does the focused item make it obvious what A will act on?
- Is the action sheet readable at handheld distance?
- Is View the natural first choice?
- Is B consistently Back?
- Are L/R box switches comfortable?
- Does any button feel dangerously easy to hit for a mutation?
- Are game/platform/source labels understandable without explanation?
- Are dialogs too desktop-like or too text-heavy?

Use those observations to refine UI without weakening the safety contract.
