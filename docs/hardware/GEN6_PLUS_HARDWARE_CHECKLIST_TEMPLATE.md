# Gen VI / VII (3DS titles) hardware checklist (template)

**Status: NOT DEVICE TESTED.** Plain Markdown; fill it in offline with any editor. Partial results are fine.
This template is for the `development/gen6-plus` lane only. It is **not** a Gen I–V round and must not be
confused with `RETEST_ROUND<N>_TEMPLATE.md`; Gen I–V acceptance stays on its own checklists.

Everything below runs through the ONE shared PokeBank NX application (Home → Game card → Save Instances →
Party / Boxes → Pokémon Actions → shared editor → Trainer / Bag → Review → Export Copy). The Gen VI/VII titles
are served by the shared Modern adapter (`include/Integration/Modern/NativeSession.h`,
`src/UI/ModernSharedSurface.inc`); there is no separate Gen VI editor. What the adapter cannot verify is shown
as read-only or refused with a reason on screen; a refusal with a correct reason is a PASS for that item.

| Identity | Value |
| --- | --- |
| NRO file | (filled in at binding) |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| Native build run | (filled in at binding) |

## Before you start

- Back up your SD card. PokeBank NX never writes a source save; still, test on **copies**.
- Sources it can see: a **Dekopon** in-emulator save (`sdmc:/switch/dekopon/sdmc/Nintendo 3DS/.../title/00040000/<id>/data/00000001/main`),
  a **Checkpoint-layout** export copied to `sdmc:/3ds/Checkpoint/saves/0x<UNIQUE> <Title>/<backup>/main`
  (X `0x0055D`, Y `0x0055E`, Omega Ruby `0x011C4`, Alpha Sapphire `0x011C5`, Sun `0x01648`, Moon `0x0175E`,
  Ultra Sun `0x01B50`, Ultra Moon `0x01B51`), or a **manually linked** `main` file.
- Record the SHA-256 of every source file before testing (e.g. `sha256sum main` on a PC). Repeat after testing:
  **they must be identical** (G0).
- Tick exactly one box per item: `[x] PASS`, `[x] FAIL` (say what happened), `[x] NOT TESTED / NO FIXTURE`.

## Group G0 · Safety (every title)

### G0 · Source immutability
**Steps:** Record SHA-256 of each source `main` before the session. Do everything below. Record again.
**Expected:** Every source hash unchanged, even after Export Copy, Discard All and B-exit.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

## Group G6 · Pokémon X / Y / Omega Ruby / Alpha Sapphire (repeat the table once per title you own)

Title tested: ________ (X / Y / OR / AS)   Source kind: ________ (Dekopon / Checkpoint copy / manual)

### G6-01 · Dashboard · Game card and identity
**Steps:** Home → Games. Find the card.
**Expected:** Card shows the title's own name, "Nintendo 3DS", its cover art (or the shared text card if art
is not packaged) and, once a save is assigned, provider "Dekopon" / "Checkpoint (3DS backup)" / "Manual file",
trainer name and current party strip. Never another game's art.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-02 · Save Instances · Assign a validated save
**Steps:** On the card: + → Source / Game File → "Assign Save to This Profile" (discovery) or "Assign Save
File Manually" (type the path).
**Expected:** Only this title's saves are listed, newest first, with provider, trainer name and party count. A
wrong-title file is refused with the reason naming the actual title. Assignment is remembered per profile;
another profile does not see it.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-03 · Save Instances · Direct open and stale-source refusal
**Steps:** A on the card opens the assigned save. Then, on a PC, change one byte of the source and press A again.
**Expected:** First open succeeds into the loaded-game hub. After the change, open is refused ("contents changed"
/ validation failure); nothing is "repaired".
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-04 · Party
**Steps:** Hub → Party.
**Expected:** Every party member with correct species, form, gender, level, shiny mark and sprite; empty party
slots not shown; A opens Actions.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-05 · Boxes
**Steps:** Hub → Pokémon. L/R change box, ZL/ZR jump 5, D-pad moves; touch a slot twice.
**Expected:** 31 boxes × 30 slots with the game's box names; all stored Pokémon visible with forms (e.g.
Furfrou trims) and shinies; empty slots empty; touch and controller reach the same slot.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-06 · Actions sheet
**Steps:** A on an occupied slot, then on an empty box slot.
**Expected:** Occupied: View, Edit, Clone, Move (box), Remove (box), Origin & Legality, Cancel. Empty slot:
no "Add Pokemon"; the sheet states why Create is unavailable for Gen VI (EC/PID/personal/met/handler/memory
defaults are not verifiable). Party slot: no Move/Remove (party removal unverified) with that reason.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-07 · View
**Steps:** Actions → View.
**Expected:** Shared editor in view mode: species, nickname, OT, IDs, level/EXP, IVs, EVs, nature, ability,
moves with PP, held item, ball, met location/level/date, friendship, Pokérus, Origin & Legality rows; nothing
editable; B returns to the grid.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-08 · Edit · Editable fields
**Steps:** Actions → Edit. Change Nickname, Held Item (picker), Friendship, one IV, one EV, Nature, Ball,
Pokérus toggle, OT Gender toggle. Keep.
**Expected:** Each change is accepted and shown immediately; the picker lists only the game's items (XY ≤ 717,
ORAS ≤ 775 ids, released items only); party stats recompute after IV/EV/nature/level edits; Keep returns to
the grid with the sprite/level updated. Species, Form, Gender, Shiny, Ability, Moves, Language, Met data show
as read-only with a reason when pressed.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-09 · Edit · Discard draft
**Steps:** Edit, change Friendship, press B, choose Discard.
**Expected:** The value returns to what it was before this Edit (including a previously kept edit); nothing is
staged from the discarded draft.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-10 · Clone
**Steps:** Actions → Clone on a box Pokémon; accept the suggested empty slot.
**Expected:** A copy appears in that slot; the original is unchanged; cloning onto an occupied or staged slot is
refused with a reason.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-11 · Move
**Steps:** Actions → Move on a box Pokémon to an empty slot in another box.
**Expected:** The Pokémon appears only in the destination; the source slot is empty; Review shows one Move row.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-12 · Remove
**Steps:** Actions → Remove, confirm with Y.
**Expected:** Y is required (A does not release); the slot becomes empty; Y on the Actions sheet undoes it.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-13 · Bag
**Steps:** Hub → Items. Change a quantity, Remove a stack, Add a released item (×1 then a quantity).
**Expected:** Five pockets (Items, Key Items, TMs/HMs, Medicine, Berries) with the real contents; Items /
Medicine / Berries editable within the pocket cap (999/999/999); Key Items and TMs/HMs read-only with a
reason; Add lists only this game's released items.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-14 · Trainer and Pokédex
**Steps:** Hub → Trainer; A on Pokédex.
**Expected:** Name, game, TID/SID, gender, language, play time and boxes unlocked as the game shows them;
every row read-only; Pokédex seen/caught counts of 721.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-15 · Review
**Steps:** Press the screen's Review key (Y on Party/Boxes/Trainer, + on the hub/Bag/editor).
**Expected:** One shared Review listing every staged change with slot and before → after, paged; Bag and
Trainer changes included; B returns where you were.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-16 · Discard All
**Steps:** Review → Y → confirm.
**Expected:** All staged changes gone; Party/Boxes/Bag show the source again.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-17 · Export Copy
**Steps:** Stage at least one change → Review → X → Y.
**Expected:** A new folder under `sdmc:/switch/PokeBank-NX/exports/gen6/<game>_<timestamp>/` with
`original_backup.sav` (byte-identical to the source), `edited.sav` and `EXPORT_MANIFEST.txt`; the source is
untouched (G0). With no staged change, X is unavailable and says so.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-18 · Reopen the export in the game
**Steps:** Copy `edited.sav` back as a `main` file into Dekopon (or a 3DS via Checkpoint) **on a copy of the
game's save** and load the game.
**Expected:** The game loads, the edited Pokémon shows the new values, saving in-game works. Record the exact
title, emulator/console and version.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-19 · Exit guard
**Steps:** With staged changes, press B on the hub.
**Expected:** The shared exit dialog (Continue / Review / Export Copy / Discard & Exit); nothing is lost without
an explicit choice.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-20 · Historical backup
**Steps:** Home → Save Backups → open an earlier export's `original_backup.sav`.
**Expected:** Opens read-only in the same shared screens; editing stages into app memory only.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G6-21 · Touch parity
**Steps:** Repeat G6-04/05/06/13 with touch only.
**Expected:** Every control reachable by touch has a controller equivalent and vice versa; no tap-through.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

## Group G7 · Pokémon Sun / Moon / Ultra Sun / Ultra Moon

Title tested: ________ (S / M / US / UM)   Source kind: ________

### G7-01 · Signature-verified open
**Steps:** Assign and open a save (as G6-02/03).
**Expected:** A game-written save opens; a save whose MemeCrypto signature or block CRC is wrong is refused
with the reason. 32 boxes × 30 slots.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G7-02 · Shared workflows
**Steps:** Repeat G6-04 … G6-16 and G6-19 … G6-21 on the Gen VII save.
**Expected:** Same behaviour through the same screens; seven Bag pockets (Items, Medicine, TMs, Berries, Key
Items, Z-Crystals, and Roto Loto items on Ultra only); Z-Crystals/TMs/Key Items read-only; Hyper Training
shown read-only in Origin & Legality.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

### G7-03 · Export is blocked, honestly
**Steps:** Stage a change → Review → X.
**Expected:** No Export Copy: the screen states that SAV7 re-signing has no retail acceptance evidence; no
folder appears under `exports/`; Discard All still works; B-exit still guards.
- [ ] PASS  - [ ] FAIL  - [ ] NOT TESTED
Comment: ________________

## Totals

| Count | Number |
|---|---:|
| PASS | ____ |
| FAIL | ____ |
| NOT TESTED | ____ |

A host test passing is never a hardware PASS. Only the owner's own result on the exact bound NRO records
acceptance, per `docs/ENGINEERING_PROTOCOL.md`.
