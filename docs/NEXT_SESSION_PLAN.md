# PokeBank NX — Next Session Plan

Last updated: **2026-09-29**

Status: **CONTINUE ON PR #92, FINISH EXACT-HEAD CI/LAUNCH, PRODUCE ONE INTEGRATED HARDWARE NRO**

## Recover live state first

Repository:
`GlitchedZeus/PokeBank-NX`

Active MAIN PR:
**#92 — G4-04: complete Gen IV shared editor — Create + field parity**

Branch:
`feature/gen4-full-editor-20260928`

Application checkpoint incorporated by this documentation refresh:
`710cf37a33ba7a9f29b09953c5d8aa224268e4fd`

PR #100 Product UI work is already **merged into PR #92**.

GitHub is authoritative. Re-fetch before modifying anything and preserve every newer commit.

## Already integrated

### Gen I–III
The current shared read/editor foundation is physically accepted.

### Gen IV
The first safe Party/Box View/Edit milestone is hardware accepted. PR #92 carries the active G4-04 editor with Create, native fields, species/form/move handling, trainer identity work, rollback and source immutability.

### Product UI
PR #92 includes the modern Product Home, real Party sprites, real Gen I–IV Pokédex progress, Gen IV trainer names, grounded trainer portraits, Classic Game Sources, cursor-memory infrastructure, Backpack/Items quick intent, compact Items/Settings actions, two-pane Settings, and truthful current launch/control presentation.

### Forensic audit
Repository coverage is complete at evidence head `143c5e5c341d4f85af30e013808a37d6719560fe`.

Remediation lane: `fix/full-audit-remediation-20260929`.

Do not restart repository coverage. Continue remediation from the existing matrix and preserve MAIN.

## Immediate first action

1. Re-fetch PR #92 live head and exact-head Actions.
2. Let Host Tests, Product UI Native and Gen IV Candidate Gate resolve.
3. Fix any remaining failure forward without reverting integrated editor/UI work.

## Remaining integration work

### Product navigation
Hardware-check Games → Classic Game Sources, trainer/Dex presentation, Backpack → Items, two-pane Settings, and remembered cursor/focus.

### Emulator launch
Verify direct-launch handoff for DraStic and melonDS. Reuse current GameLauncher/provider/linking architecture. If emulator + content cannot be proven, keep **Link Game File** and store the mapping only in PokeBank-owned configuration. Never infer a ROM solely from a save path.

### Audit remediation
Keep the forensic evidence branch frozen. Apply remediation forward in risk order without weakening source immutability, staged editing, launch/write separation, or fail-closed source validation.

## CI and hardware stop condition

The final integrated candidate must have Host Tests green, sanitizer/regression gates green where configured, Gen IV Candidate Gate green, Product UI Native/devkitA64 green, one exact Actions artifact, and one exact NRO + SHA-256.

Then STOP.

Do not call the candidate device accepted until that exact NRO is physically tested.

## After hardware acceptance

Next major frontend tranche:

**FULL APP-WIDE TOUCH CONTROLS**

Do not begin Gen V, Master Vault persistence, cross-game True Move, or live source writes before the integrated candidate is accepted.
