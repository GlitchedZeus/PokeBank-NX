# PokeBank NX — Create Pokémon / Advanced Editor Vision

> **Future-only product/design specification.** This does not expand the current FRLG/RSE implementation task. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for active coding work.

## Product decision

PokeBank NX's **Create Pokémon** and advanced editor should use **PKHeX-level capability and correctness as the functional baseline**, then add PokeBank NX's own controller-first guidance, safety, provenance, destination awareness, previews, history and recovery on top.

The goal is not a simplified replacement for PKHeX.

The goal is:

```text
PKHeX-class power
+ easier controller-first workflows
+ Quick Legal automation
+ generation/game-aware guidance
+ human-readable legality explanations
+ provenance/history
+ destination compatibility checks
+ staging / preview / undo / snapshots
+ event integration
+ PokeBank Vault integration
= PokeBank NX Create Pokémon
```

PKHeX remains a pinned host-side oracle/reference for correctness and regression testing. The Nintendo Switch runtime remains PokeBank NX's native implementation unless a future licensing/architecture decision explicitly changes that boundary.

---

# Three creation/editing modes

## 1. Quick Legal

For users who know the Pokémon they want but do not want to fill every technical field.

Example flow:

```text
Create Pokémon
  -> Quick Legal
  -> Species: Eevee
  -> Destination/ruleset: Pokémon FireRed
  -> Optional: Shiny / Level / Nature / Gender / Ball / Moves
  -> Generate Legal Candidate
  -> Review
  -> Add to Vault
```

PokeBank NX selects a compatible encounter/template and fills required technical values using the chosen game/generation rules.

Quick Legal should explain any compromise it makes, for example:

```text
Requested ball unavailable in FireRed -> changed to Poké Ball
Requested move unavailable at this level -> removed
Requested shiny state impossible for selected event -> shiny disabled
```

The user always gets a final preview before creation.

## 2. Guided Create

A step-by-step expert assistant that exposes more control while preventing accidental contradictions.

Suggested stages:

```text
1. Species / form
2. Origin game / generation / encounter
3. Trainer / OT context
4. Level / met data / ball
5. Nature / ability / gender / shiny
6. IVs / EVs / stats
7. Moves / relearn / PP where represented
8. Held item
9. Ribbons / marks / contest data where supported
10. Language / nickname / friendship and other format fields
11. Legality + compatibility review
12. Provenance declaration
13. Create into Vault / staged destination
```

Fields should be classified as:

```text
REQUIRED
OPTIONAL
CONDITIONAL
DERIVED / AUTO
UNAVAILABLE IN THIS FORMAT
```

The interface should tell the user *why* a field is required or impossible instead of merely refusing it.

## 3. Advanced / Expert Editor

This is the PKHeX-class power-user mode.

Expose every meaningful field supported by the underlying Pokémon format, grouped cleanly for controller navigation.

Examples include, where applicable:

```text
species / form
PID
EC
TID / SID
OT / HT
language
nickname
nature
minted nature / stat nature where represented
ability / ability slot
level / EXP
met level / location / date / game
ball
gender
shiny state / shiny type
IVs
EVs
contest stats
moves
PP / PP Ups
relearn moves
held item
friendship / affection
ribbons
marks
memories
handler data
hyper training
battle-ready / game-specific flags
tera type / dynamax / alpha / mastery / other generation-specific fields
checksum / serialization-relevant fields where appropriate
```

PokeBank NX should not hide advanced fields merely because most users do not need them.

---

# Safeguards must add intelligence, not remove expert control

The editor should distinguish between:

```text
LEGAL
INVALID
UNKNOWN / NOT FULLY CHECKED
LOSSY FOR DESTINATION
UNSUPPORTED
```

Safety behavior:

- never silently change a field without showing the user;
- show errors next to the exact conflicting field;
- explain why the combination is impossible;
- offer one-tap safe fixes where a deterministic fix exists;
- allow users to compare requested vs corrected values;
- preserve the original request in transaction/history metadata when useful;
- never claim legality proves authentic historical acquisition;
- created/generated Pokémon always have explicit PokeBank provenance.

Advanced users should retain the ability to construct deliberately invalid/test Pokémon for research, emulator, homebrew or debugging purposes when technically supported. Such entities must be unmistakably labeled `INVALID / USER-CREATED` and must never be silently presented as naturally obtained.

A destination write adapter may impose additional safety rules. PokeBank NX can permit an invalid entity to exist in the Vault while still refusing or warning against writing it into a particular live save unless the destination workflow explicitly allows that operation.

---

# Destination-first intelligence

Creation should optionally begin with the intended destination game.

Example:

```text
Create for: Pokémon Emerald
```

Then the editor automatically knows which fields, species, forms, moves, balls, abilities and mechanics exist in that ruleset.

Conversely, users can choose:

```text
Create as canonical Vault entity first
```

and later use Transfer Lab to produce a destination-specific representation.

Before creation/transfer, show:

```text
Can exist in destination?          YES / NO / CONDITIONAL
Can be represented losslessly?     YES / NO
Legality result                    LEGAL / INVALID / UNKNOWN
Fields that will change            list
Fields that will be lost           list
Irreversible changes               list
```

---

# PKHeX Oracle integration

The existing `docs/PKHEX_ORACLE.md` remains the correctness/testing reference.

The host-side oracle should eventually support creator regression tests for:

```text
encounter candidates
generation/game legality
PID / shiny relationships where relevant
ability/form constraints
move legality
ball legality
met/origin constraints
conversion output
serialized format correctness
event templates
edge cases and known-invalid samples
```

PokeBank NX should compare normalized data/results rather than permanently coupling its UI/data model to PKHeX class names or text output.

Pinned versions and reproducible golden vectors are required so a future upstream PKHeX change cannot silently redefine PokeBank behavior.

---

# Create from Event Library

Event Library / EventDex should feed directly into the creator.

Example:

```text
Event Library
  -> Aura Mew (2007)
  -> View distribution definition
  -> Create / Restore Local Event Instance
  -> choose compatible trainer/context fields where historically variable
  -> legality check
  -> provenance review
  -> Add to Vault
```

PokeBank must distinguish:

```text
Historical event definition: known
Local instance generated/restored from archive: yes
Official live redemption witnessed by PokeBank: no
Legality: independent result
```

Do not fabricate official server receipt, HOME tracker data, Nintendo account provenance or history that PokeBank did not witness.

---

# Create from existing Pokémon

Advanced workflows should include:

```text
Clone
Clone and Edit
Use as Template
Make Shiny
Change Form
Change Destination Ruleset
Rebuild as Legal Candidate
Compare Before / After
```

Every operation should preserve a parent/reference relationship in provenance when appropriate.

Example:

```text
Vault #00421 — Eevee
     |
     +-- Clone -> Vault #01022
             provenance: cloned from #00421 by user
```

---

# Human-readable legality assistant

Do not stop at a red legality icon.

Example:

```text
INVALID — 3 problems

1. Moon Ball is unavailable in Pokémon FireRed.
   [A] Use Poké Ball

2. Wish is not obtainable from the selected encounter/moveset path.
   [A] Show legal move options

3. Met level 2 is impossible for this encounter.
   Valid range: 5-7
   [A] Set to 5
```

For advanced users, provide a deeper technical view containing the raw normalized rule/check identifiers useful for debugging.

---

# Preview, staging and undo

Creation itself should never require immediate live-save mutation.

Default flow:

```text
requested Pokémon
  -> candidate model
  -> legality/compatibility analysis
  -> final before/after preview
  -> create immutable Vault entity
  -> optional Bank placement
  -> optional staged destination operation
```

For edits to existing Vault entities, retain immutable prior payload/history or create a new revision/entity according to the Vault model rather than destructively erasing provenance.

Before any approved live-save write:

```text
snapshot / backup
-> stage
-> validate
-> write
-> readback
-> verify
-> commit history
-> rollback on failure
```

---

# Controller-first UX

Power should not require a mouse.

Suggested controls follow the established PokeBank NX patterns:

```text
D-pad / Left Stick  navigate
A                   select / edit / confirm
B                   back
X                   filter / search / context
Y                   quick action / toggle where safe
L / R               previous / next section
ZL / ZR             jump sections/pages
+                   more / advanced actions
-                   contextual help
```

Long field lists should use sections, tabs, search, favourites/recent values and smart defaults instead of forcing hundreds of linear rows.

---

# Templates and recipes

Allow reusable user templates such as:

```text
Competitive Singles
Competitive Doubles
6x31 IV starter
Breeding parent
Shiny hunt target
Living Dex placeholder plan
Event recreation template
My OT / Trainer profile
```

Templates should store user intent/defaults, not fake provenance.

---

# Bulk creation

Later power-user workflows may support:

```text
Create Living Dex candidates
Create normal + shiny test corpus
Create box from template
Generate legality regression fixtures
```

Bulk creation must be explicitly labeled and previewable, with rate/storage checks and clear provenance for every generated entity.

---

# Relationship to official Pokémon HOME

Official Pokémon HOME acceptance is not the product boundary.

PokeBank NX-created Pokémon can be useful for:

```text
local Vault storage
supported emulator saves
supported local Switch saves
local/LAN PokeBank exchange
PokeBank Internet Gift/Trade
collection planning/testing
homebrew/research workflows
```

PokeBank NX must not promise that a generated entity will be accepted by official Pokémon HOME, official online battles, Nintendo services or anti-cheat systems.

Where compatibility information is known, show it as an advisory rather than forging official status.

---

# Product shorthand

The intended experience can be summarized as:

```text
PKHeX is the capability baseline.
PokeBank NX adds the guardrails, explanations, automation, provenance,
destination intelligence, Vault integration and controller-first workflow.
```

Or more simply:

**PKHeX-style creation on steroids, without sacrificing expert control.**
