# PokeBank NX — Next Session Plan

> **DATED PLAN SNAPSHOT:** Use only after reading `docs/ENGINEERING_AUTHORITY.md` and re-fetching the live PR state. GitHub wins if any SHA, PR relationship, CI status, or integration note below has advanced.


Last updated: **2026-09-29**

Status: **CONTINUE ON PR #92, FINISH INTEGRATED CI/LAUNCH, PRODUCE ONE FINAL HARDWARE NRO**

## Recover live state first

Repository:
`GlitchedZeus/PokeBank-NX`

Active PR:
**#92 — G4-04: complete Gen IV shared editor — Create + field parity**

Branch:
`feature/gen4-full-editor-20260928`

Last verified head:
`710cf37a33ba7a9f29b09953c5d8aa224268e4fd`

PR #100 Product UI polish content is already **integrated into PR #92**; PR #100 itself is closed.

GitHub is authoritative. Re-fetch before modifying anything. Preserve every newer commit.

## What is already true

### Gen I–III

The current shared read/editor foundation is physically accepted.

### Gen IV

The first safe Party/Box View/Edit milestone is hardware accepted.

PR #92 carries the active G4-04 shared editor with Create, additional native fields, species/form/move handling, action parity, rollback and source immutability.

### Product UI

PR #92 now includes:

- modern Product Home;
- Party sprite presentation;
- distinct Vault/Pokédex presentation;
- real Gen I–IV Pokédex progress;
- Gen IV trainer-name propagation;
- game/gender-aware trainer portraits;
- cursor-memory foundation;
- Classic Game Sources;
- Backpack/Items quick intent;
- compact Items/Settings quick actions;
- two-pane Settings with remembered category/option cursor.

## Immediate first action

1. Re-fetch PR #92 live head and exact-head Actions.
2. Preserve anything newer than `710cf37a…`.
3. Product UI Native #61 and Gen IV Gate #137 are already green on that checkpoint.
4. Let Host Tests #1701 finish; fix any failure forward without reverting integrated editor/UI work.

## Remaining integration work

### Classic Games / Backpack / Settings

Hardware-check the integrated flows:

- Games opens familiar Game Sources;
- trainer name / portrait / Dex summary are coherent;
- Backpack enters the selected game’s Items flow through normal source/backup safety;
- Settings opens the two-pane category/options UI;
- backing out restores the remembered cursor.

### Emulator launch

Verify the actual direct-launch handoff for:

- DraStic;
- melonDS.

Reuse existing GameLauncher/provider/linking architecture.

If emulator + content cannot be proven, keep **Link Game File** and store the mapping only in PokeBank-owned configuration.

Never infer a ROM solely from a save path.

## CI and hardware stop condition

The final integrated candidate must have:

- Host Tests green;
- sanitizer/regression gates green where configured;
- Gen IV Candidate Gate green;
- Product UI Native/devkitA64 green;
- one exact Actions artifact ID/name;
- one exact NRO filename + SHA-256;
- no source-write policy regression.

Then STOP.

Do not call the new integrated candidate device accepted until that exact NRO is tested on the real Switch.

## After hardware acceptance

The next major frontend tranche is:

**FULL APP-WIDE TOUCH CONTROLS**

Do not start Gen V, Master Vault persistence, cross-game True Move, or live source writes before the integrated UI candidate is accepted.
