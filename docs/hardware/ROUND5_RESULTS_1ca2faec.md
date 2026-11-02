# Round 5 hardware results: `1ca2faec`, plus the Round 6 work list

## Source

These are the owner's results from a Nintendo Switch test on **October 25, 2026**. They are the authoritative record for this round.

## Build tested

| Field | Value |
| --- | --- |
| NRO | `PokeBank-NX-Gen1-5-Candidate-1ca2faec.nro` |
| Code SHA | `1ca2faec55ee64702eac6e0779daed3a1c8b3377` |
| NRO SHA-256 | `b8966cdbd108530ae1c056a07e318d2f05d0cdee30d910b361ab29f381e41511` |

The owner checked the NRO hash before testing.

## Summary

**31 PASS / 8 FAIL / 3 NOT TESTED** (42 tests).

### FAIL (8)

| ID | Owner finding |
| --- | --- |
| L06 | Pokérus works in Gen II and IV but not in Gen III or V. Held Item selection is unreliable: in Gen III, neither A nor touch selected an item, and some taps do nothing. |
| R2-TRAINER | The Gold/Silver Trainer Gender rows overlap: an old "Male" label sits under the new row. Older Trainer layouts are inconsistent. |
| B08 | Bag Review is not directly discoverable. It is buried in the `+` options and the save-exit flow. |
| E05 | Controller glyphs are missing or inconsistent: Game Browser Controls, Party footers, and stale Review lines in Game Browser Help. |
| R2-CHECK-011 | Gen V historical event and location choices cannot be told apart: several rows read POKÉPARK, and Festa rows differ only by number. |
| NEW-18 | Older Trainer cards do not match the Gen V design. Gen II Gender overlaps. |
| R2-REROLL | Gen II Unown: making it shiny changed A to something that looks like I or V. It is unclear how letter, shiny state and DVs relate. |
| P1-RETURN | Review is hard to find from Bag and the other editor screens, and there is no visible control prompt for it. |

### NOT TESTED (3)

| ID | Note |
| --- | --- |
| AUD-01 | Complete Gen I–V Review consistency. |
| P1-GEN1 | Gen I Review overlay presentation. |
| S06 | Exported save in an emulator. The owner deferred this on purpose. It is not permission for direct writing or injection. |

### PASS (31)

E03, C07, M04, E02, R2-BUG-002, NEW-02, NEW-03, NEW-04, NEW-05, NEW-06, NEW-07, NEW-08, NEW-09, NEW-11, NEW-12, NEW-13, NEW-15, NEW-16, NEW-17, NEW-19, NEW-20, R2-SPECIAL, R2-REVIEWMOVE, M05, S05, NEW-10, NEW-14, R2-EMUBACKUP, C01, P3-TRAINER, 4D-HELP.

Some of these passes carry follow-up requests. A PASS does not close the request attached to it.

## Round 6 work list

### Priorities

| Priority | Item | Hardware IDs |
| --- | --- | --- |
| P1 | Pokérus in Gen III and V. Held Item selection works with both A and touch at the top, middle and bottom of the list. | L06 |
| P2 | Gen I–IV Trainer Info rebuilt to the Gen V design. Gold/Silver overlap fixed. No invented Gen I or Gold/Silver gender. | R2-TRAINER, NEW-18, P3-TRAINER follow-up |
| P3 | A visible Review entry in Trainer, Create, Edit, Bag, Party and Boxes, returning to the screen it was opened from. Create Review shows details instead of just "Empty → PIKACHU Lv 25". | B08, P1-RETURN, AUD-01, P1-GEN1, 4D-HELP follow-up |
| P4 | Glyphs, Help and footers: HOME, Game Browser, Party, Boxes, Trainer, Bag, Create, Edit, Review, Backups. | E05 |
| P5 | Repeated POKÉPARK and Festa event/met-location rows made distinguishable. Unknown provenance is shown honestly. | R2-CHECK-011, NEW-10 follow-up |
| P6 | Gen II Unown: letter, shiny state, DVs and sprite always agree. Optional letter selector that picks a valid DV combination. | R2-REROLL |

### Follow-ups

| Item | Request | Source |
| --- | --- | --- |
| 7A | Better Gen II Special SpA/SpD placement. | R2-SPECIAL |
| 7B | The Gen V "Learnable" label uses the same OK treatment as Gen I–IV. | M05 |
| 7C | Remove the redundant Gen V Language toast. | NEW-14 |
| 7D | Tap outside closes non-destructive popups, with no tap-through. | NEW-02 |
| 7E | Audit save safety and backups against PKSE principles. | S05, R2-EMUBACKUP |
| 7F | Gen V Trainer visual standard (same work as P2). | P3-TRAINER |
| 7G | Bag Review discoverability (same work as P3). | 4D-HELP |

### Gen III held items (owner decision)

- Safari Ball (5) and Berry Juice (44) are refused as **held** items, following PKHeX `ItemStorage3RS.Unreleased`.
- Safari Ball stays valid as a **caught Ball**.
- Bag validation is separate.
- A pre-existing record that holds one of these items is kept and labelled honestly.
- Merged into #307 at `a5db3368`. Its fix commit is `15c77226`.

## Status of every Round 6 item

NOT DEVICE TESTED until the owner tests the Round 6 NRO.
