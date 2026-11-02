# Hardware retest — PokeBank-NX-Gen1-5-Candidate-7fea1797

**Status: NOT DEVICE TESTED.** Not hardware accepted until the owner tests this exact file on a Switch.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-7fea1797.nro` |
| Code SHA | `7fea17971906cae557a7207bfb7b0bed3548cea2` |
| Tree SHA | `29bd15749dd9ce70d6aa0a6304fd29f48155dece` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38946941097 (run `38946941097`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-7fea1797` (id `12066747065`) |
| NRO SHA-256 | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` |
| NRO size | 175685297 bytes |

Only this artifact is the candidate. A later docs-only commit on #307 (this file and
`docs/qa/CANDIDATE_UNDER_AUDIT.env`) also triggers the candidate workflow; that second artifact
is **not** the candidate.

Before testing, check that the downloaded file's SHA-256 matches the value above.

## 0. Optional: input trace (recommended for any input problem)

1. On the SD card, create an empty file: `sdmc:/switch/PokeBank-NX/config/input_trace.enable`.
2. Start PokeBank NX and reproduce the problem.
3. Send `sdmc:/switch/PokeBank-NX/logs/input-trace.log`.

The trace records only button names, screen/overlay/focus tags, elapsed milliseconds and
open/exit/launch origins. It contains no save data, names, ids or paths. It is bounded at 4000
lines and flushed per line, so a crash keeps everything before it (a final
`EVENT=terminate` line marks an internal abort). Delete the marker file to turn it off.

## 1. P0 #320 — no single press leaves PokeBank NX

Every way out now opens one visible decision. **Only Y commits; A and B stay/cancel.**

- [ ] Product Home, press **B**: "Quit PokeBank NX?" appears. **A** stays. **B** stays. **Y** quits.
- [ ] Product Home footer reads **B Quit**; Help (−) reads "B Quit PokeBank NX (asks first; Y confirms)".
- [ ] Product Home, press **ZR** (or tap the hero's **ZR Launch** button): "Launch <game>?" with the
      emulator quit-route notice. **A**/**B** cancel back to Home. **Y** launches.
- [ ] Current Game menu (+), **ZR**: same Launch decision; cancel returns to the Current Game menu.
- [ ] Full Games browser (Games dock), **ZR**: same Launch decision; cancel returns to the browser.
- [ ] A game with several emulator saves, Launch → choose a save with **A**: the Launch decision
      opens (A never launches directly). **B** cancels back to the save list.
- [ ] **L** / **R** on Product Home change the game and never open or launch anything.
- [ ] Top-right **Settings**, press **A** (and tap it): Settings opens; the app never exits.
      **B** in Settings returns to Home.
- [ ] While any dialog is open, tapping the Home footer or hero buttons underneath does nothing.
- [ ] Mash A+Y together in the Quit dialog: it stays (an ambiguous press is treated as Stay).

## 2. Gen II Create / Edit (Thread C)

- [ ] Create a Pokémon in Gold/Silver/Crystal: **A** selects/advances, **B** goes back one layer,
      never exits the app or discards unexpectedly.
- [ ] The footer always matches the topmost layer (no "A Edit | Y Random DVs | B Exit" under a modal).
- [ ] **Y** re-rolls DVs immediately (no confirm dialog); shiny, Gen II gender and Unown letter follow.
- [ ] Inline Shiny / Pokérus / Gender fields edit in place; the Data pane stays inside its bounds.
- [ ] Crystal Met Location chooser is proportional and shows names only (no raw ids).

## 3. Scroll speed (phase 8)

- [ ] Hold the D-pad or stick in any list (Home dock, Games browser, Boxes, pickers, keyboard):
      first move is immediate, repeat starts after about ¼ s, then moves quickly and steadily.
- [ ] No row is skipped when the screen is busy; D-pad and stick feel the same.
- [ ] Keyboard: holding a key / backspace repeats at the same cadence.

## 4. Campaign phases

- [ ] Actions sheet (Gen I–V) lists only real commands, in one order: View, Edit, Clone, Move
      (where supported), Release, Origin & Legality, Review Pending Changes, Cancel. No Make Shiny or
      greyed placeholder rows.
- [ ] "Origin & Legality" is the label everywhere (action, modal, provenance pane).
- [ ] Moves (Gen I–V) use the one shared Moves panel / Move chooser / picker footer; move types for
      Gen I/II come from the game data; max PP follows the game's era.
- [ ] Party and summary-panel taps open the native sheet (no generic View flash).
- [ ] Gen III PID-derived fields and the Unown letter are editable; Gen V Shiny is editable.
- [ ] Item quantities follow the per-game policy (Gen IV unique items have no quantity editor;
      Gen IV/V bag footer shows the rule).
- [ ] Exit-with-changes SaveConfirm uses the shared footer.
- [ ] Export Copy still writes only the verified working-copy export under
      `sdmc:/switch/PokeBank-NX/exports/`; the original save is unchanged.

## 5. Documented limitations (not regressions)

- Party in Gen I–III opens the native read-only View; the Gen I and Gen II staged editors are
  box-only. To edit, move the Pokémon into a box.
- Gen IV-format Bank records are refused in a Gen IV session.
- Gen V Language is read-only.
- In the full Games browser, **L/R switch Profile** (the footer shows "L/R Profile" when more than one
  profile exists). On Product Home, L/R change game.
- No live emulator or installed-game writes; Inject Save stays disabled.

## Result

Report pass/fail per box above, with the input trace for any failure. This file is the checklist
only. Acceptance is recorded by the owner after testing this exact NRO.
