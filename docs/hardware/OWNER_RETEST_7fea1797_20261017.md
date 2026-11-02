# Owner Switch retest — Gen I–V candidate `7fea1797`

**Recorded October 17, 2026.** User-reported device results, not automated CI or a complete release certification.

| Field | Evidence |
|---|---|
| Artifact | [PokeBank-NX-Gen1-5-Candidate-7fea1797](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38946941097/artifacts/12066747065) |
| Source commit | `7fea17971906cae557a7207bfb7b0bed3548cea2` |
| NRO SHA-256 | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` |
| Exact-binary audit | [GitHub Actions 38947904299](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38947904299) PASS |
| Hardware report | **42 PASS / 54 targeted checks** (77.8%). Twelve not marked passed; **this does not mean twelve failed**. |
| Game/provider/controller matrix | Not specified by tester; cannot claim all 20 games or every emulator tested |
| Release acceptance | **OPEN** [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313); #307 remains unmerged |

## Confirmed hardware passes (42)

- **Controls (9):** C01 A selects; C02 B guarded; C03 Launch confirmation; C04 L/R focus; C05 Gen II Create A/B; C06 popup background input blocked; C07 tested backup deletion Y confirmation; C08 Pokémon Release/Remove Y confirmation; C09 second controller.
- **Moves (8):** M01–M05 shared Gen I–V Moves panels; M06 shared move chooser; M07 PP and PP Ups; M08 Gen I hidden-looking row.
- **Items (3):** B05 unique-item one-only limits; B07 Remove confirmed; B08 Bag edits reflected in Review and exported copy.
- **Editor (10):** E01 Make Shiny removed from quick Actions; E02 action ordering; E04 row counters removed; E05 shared A/B footers; E06 Shiny Yes/No presentation; E07 Pokérus inline; E08 Gender inline; E09 Gen III Language inline; E10 Y reroll immediate; E12 Pikachu L5 default; E13 Gen III PID/Unown editing. (**11 IDs**; this line enumerates 11 passes.)
- **Layout (5):** L01 Gen II data pane bounds; L02 Crystal picker; L03 Gen III met-location; L04 Gen IV/V location windows; L05 cursor/row clipping.
- **Navigation performance (4):** N01 initial scroll; N02 held repeat; N03 touch/stick parity; N04 long lists/keyboard.
- **Gen V/safety (2):** S02 Party/Boxes/Trainer display; S05 original files untouched.

> Counts per original report: 9 controls + 8 moves + 3 items + 11 editor + 5 layout + 4 navigation + 2 save/Gen V = **42**.

## Owner-reported problems to repair / investigate (6 unchecked IDs)

| ID | Report / expected outcome |
|---|---|
| B01 | Gen I item Add opens the numpad **before** placing the item in the Bag; Add should stage quantity 1 immediately, then allow editing the existing stack |
| B02 | Gen II same Add ordering problem |
| B03 | Gen III same Add ordering problem |
| B06 | Stackable quantity flow is part of the same Bag Add-state issue |
| S01 | Gen V PP / PP Ups captions too close together, affecting readability |
| S03 | Gen V left panel clips species/name text; other Create/Edit parity problems reported, with more detail still needed |

## Unverified / investigate; do NOT call these failed (6 unchecked IDs)

- **B04:** Gen IV/V Add behavior has not been fully exercised.
- **E03:** Owner did not confirm the Gen I–V “Origin & Legality” wording.
- **E11:** Owner questioned shiny/non-shiny identical stats. **Mechanics note:** Gen III–V shininess uses PID/Trainer IDs; IVs and EVs may legitimately match. Gen II DV relations must be enforced. Investigate invariants, not a blanket distinct-IV rule.
- **L06:** Party/summary tap routing not verified.
- **S04:** Owner suspects Gen V Review/Discard/Export parity, but supplied no isolated reproduction yet.
- **S06:** Exported edited save not reopened in the emulator. Staged Export Copy exists; **source/live writeback has not been authorized or enabled**.

## Owner-requested features and refinements for the next engineering round

1. **Compact shared menu geometry:** remove excessive padding in Bag/Key Item warnings, Remove confirmation, Poké Ball, Held Item and other pickers. Uniform menu *type* design across Gen I–V; content-driven heights can differ. Sprite/name spacing and selection hit boxes should align.
2. **Live species search:** incremental prefix filter, responsive to each typed character and Backspace/Clear; optional non-conflicting shortcut; available per-game species list and artwork.
3. **Gen I field styling:** read-only derived HP DV and non-editable stats consistently gold in View/Edit; investigate Gen II numeric Special versus `--`.
4. **Trainer editing:** Gen I Trainer Name/Money staged editing and consistent A Edit footer across supported Trainer screens, preserving native save constraints.
5. **Launch menu:** resize proportionally and show styled controller glyphs without changing Y-to-confirm safety.
6. **Emulator backups:** `+` currently appears to deny backup-management access as “read only source.” Distinguish immutable emulator source from app-owned deletable backups; Y confirmation before deleting an app-owned backup.
7. **Touch scrollbars:** drag/thumb and track scrubbing across scrollable app lists; avoid accidental row selection.
8. **Move chooser:** use available vertical space for one or two more *visible rows* (not move slots); fix Gen V PP labels.
9. **Crystal Met Time:** A cycles supported native states without a large popup.
10. **Gen V header:** show exact game/source staged-working-copy identity in Create/Edit as with Gen I–IV.
11. **Gen V export:** investigate/create/edit/review/discard/export/strict reparse and reopen `edited.sav` in the source emulator.
12. **Reroll mechanics:** preserve shiny/gender/Unown relations where encoded, without incorrectly requiring distinct IVs/EVs in Gen III–V.

## Next gate

Work on current app [PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307), keep [legality PR #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) separate, and deliver a single auditable new NRO. Repeat targeted hardware retest plus impacted regressions, then perform reviewed legality integration and a **new** full 20-release/every-action acceptance. Gen VI–IX app work stays deferred. No source-save overwrites.
