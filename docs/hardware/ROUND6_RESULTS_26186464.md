# Round 6 hardware results — `PokeBank-NX-Gen1-5-Candidate-26186464.nro`

The owner tested this build on a Nintendo Switch on **October 26, 2026**. Their results are the authoritative record for this build.

## Build tested

The owner checked the hash before testing.

| Field | Value |
| --- | --- |
| NRO | `PokeBank-NX-Gen1-5-Candidate-26186464.nro` |
| Code SHA | `26186464d477d33fdc0afc2fe19ea1394035f49f` |
| NRO SHA-256 | `5b1a85c878e2999f0f8212a583f037643c3b42147a21fef631ea11a832d2a3bf` |
| Checklist | [RETEST_26186464.md](RETEST_26186464.md) |

## Source of this record

The owner's original report is `PokeBank-NX-Round6-Results-26186464.md`. It and its screenshots have **not yet been added to the repository**: the coordinator could not find them in the repository, on PR #307, in any issue, or in the session.

This record is built from the owner's Round 7 directive, which gives every result and quotes the comments. When the original file and screenshots are added, store them unchanged next to this file:

- `ROUND6_OWNER_REPORT_26186464.md`
- `round6-screenshots/`

The original file takes precedence over this summary.

Screenshots the owner referenced:

- Gen III Emerald Pokémon editor
- Gen IV Diamond Pokémon editor
- Gen V Black 2 Met Location picker, showing:
  - Pokémon Movie 13–20
  - Pokémon Festa 10–16
  - POKÉPARK 16–20
  - Worlds 2010–2016
  - WCS 2011–2018
  - VGE 2011–2018
  - Pokémon Center, PC Tokyo and PC Osaka
  - Entralink and transfer markers

## Summary

**49 PASS / 9 FAIL / 1 NOT TESTED** out of 59 checks.

## Results

### Group A — Round 5 passes (regression checks)

| # | ID | Result | Owner comment / follow-up |
| --- | --- | --- | --- |
| A01 | E03 | PASS | Origin & Legality uncertainty is correct |
| A02 | C07 | PASS | Backup deletion protection |
| A03 | M04 | PASS | |
| A04 | E02 | PASS | |
| A05 | R2-BUG-002 | PASS | |
| A06 | NEW-02 | PASS | |
| A07 | NEW-03 | PASS | |
| A08 | NEW-04 | PASS | |
| A09 | NEW-05 | PASS | |
| A10 | NEW-06 | PASS | |
| A11 | NEW-07 | PASS | |
| A12 | NEW-08 | PASS | |
| A13 | NEW-09 | PASS | Keep the green/red Nature colours |
| A14 | NEW-11 | PASS | |
| A15 | NEW-12 | PASS | Preserve encounter filtering |
| A16 | NEW-13 | PASS | |
| A17 | NEW-15 | PASS | |
| A18 | NEW-16 | PASS | |
| A19 | NEW-17 | PASS | |
| A20 | NEW-19 | PASS | Four-choice staged exit |
| A21 | NEW-20 | PASS | |
| **A22** | **R2-SPECIAL** | **FAIL** | The calculated Special values are still in the wrong place. Do not squeeze them into the numeric STATS column. Use the Gen I calculated-Special presentation as the reference, in the lower-middle of the centre STATS panel. |
| A23 | R2-REVIEWMOVE | PASS | Created a Gen III Pikachu with Safari Ball, exported it, then reopened the export from Save Backups with the edits intact |
| A24 | M05 | PASS | |
| A25 | S05 | PASS | |
| A26 | NEW-10 | PASS | Keep the compact popup |
| A27 | NEW-14 | PASS | |
| A28 | R2-EMUBACKUP | PASS | |
| A29 | C01 | PASS | |
| A30 | P3-TRAINER | PASS | Supported Trainer editing works. Keep it. |
| A31 | 4D-HELP | PASS | |

### Group B — Round 5 failures, repaired in Round 6

| # | ID | Result | Owner comment / follow-up |
| --- | --- | --- | --- |
| B01 | L06 | PASS | Gen III/V Pokérus and Held Item activation work |
| B02 | R2-TRAINER | PASS | The Gold/Silver gender overlap is gone. Still asks for the Gen V look (see B06). |
| B03 | B08 | PASS | Direct Bag Review works with +, but Bag Help still says + opens Inventory Options (see B04) |
| **B04** | **E05** | **FAIL** | Problems with Inventory Help and glyphs:<br>• Gen I/II/III Inventory Help has stale wording such as "X Add remains available".<br>• Bag Help describes + as Options although + opens Review.<br>• Gen V Bag Help is titled "Game Browser Controls" but should be "Inventory Controls".<br>• Gen IV/V Inventory is closer to the preferred layout. |
| **B05** | **R2-CHECK-011** | **FAIL** | The names now differ, but many rows still say "Event/special location; unverified". The owner wants to understand what each choice means and how it differs from the others. |
| **B06** | **NEW-18** | **FAIL** | Gen I–IV Trainer still looks like a hybrid:<br>• yellow values where Gen V uses neutral text;<br>• awkward boxes and inconsistent row backgrounds;<br>• layout pieces that look patched together.<br>Gen V Trainer is the standard. |
| **B07** | **R2-REROLL** | **FAIL** | Gen II Unown:<br>• Shiny restricts the available letters.<br>• Turning Shiny off and choosing a letter changes the DVs and the form.<br>• Turning Shiny on again changes the form.<br>• Reroll seems not to work, shiny or not.<br>• Choosing a letter does not explain whether the result should be shiny.<br>Overall it is confusing and feels broken. |
| **B08** | **P1-RETURN** | **FAIL** | Two separate defects:<br>(1) "While creating Unown in Gen II, opening Review displays what appears to be an invisible screen. The application appears frozen, but pressing B exits the invisible screen and returns it to normal."<br>(2) "Review also works by pressing + in the Party menu. This needs fixed to just the Y button, or whatever the correct button is in the current menu or window." |

### Group C — requests and late changes

| # | ID | Result | Owner comment / follow-up |
| --- | --- | --- | --- |
| **C01** | **7A** | **FAIL** | Same requirement as A22: Gen II Special belongs in the lower-middle STATS area, using the Gen I reference |
| C02 | 7B | PASS | |
| C03 | 7C | PASS | |
| C04 | 7D | PASS | Also wants tap-outside dismissal on the **Quick Games** panel and other ordinary popups |
| C05 | 7E | PASS | |
| **C06** | **7F** | **FAIL** | Trainer looks inconsistent with Gen V. The Y Review Changes button is too large or badly placed and overlaps rows. |
| C07 | 7G | PASS | |
| C08 | Held-item policy | PASS | On LeafGreen:<br>• Safari Ball and Berry Juice cannot be chosen as new held items.<br>• Safari Ball can be chosen as a caught Ball. |
| **C09** | **UNOWN-LETTER** | **FAIL** | Problems with the letter chooser and Shiny (see B07) |
| C10 | Tap-through | PASS | |
| C11 | View-only notice | PASS | |
| C12 | Gen V Pokérus | PASS | **New bug:** after exporting an edited Gen V save, the exported copy does not appear in Save Backups, unlike the other generations. This is the Round 6 audit's item G1. |
| C13 | Create Review | PASS | |
| C14 | Gen II Create moves | PASS | |
| C15 | Unreleased label | PASS | |
| C16 | No cross-save draft | PASS | |
| C17 | Source guard | PASS | |

### Group D — still untested

| # | ID | Result | Owner comment / follow-up |
| --- | --- | --- | --- |
| D01 | AUD-01 | PASS | The shared Review is much improved. Do not redesign it. |
| D02 | P1-GEN1 | PASS | |
| D03 | S06 | NOT TESTED | Deferred on purpose. Not permission for direct writing. |

### Other bugs and requests

- **Pokémon editor typography is inconsistent.** Older-generation View/Edit screens use large white headings, while newer ones use smaller coloured section labels. Affected labels include:
  - DETAILS
  - STATS
  - MOVES
  - Origin & Legality
  - supporting field labels

  The shared Gen IV/V editor is the reference. Keep the green/red Nature colours.

## Round 7 repair list

| Priority | Item | Hardware IDs |
| --- | --- | --- |
| P0 | Gen II Unown Create → Review shows an invisible screen | B08 (1) |
| P0 | Party opens Review with a hidden +. One visible Review key per screen, Y where free. | B08 (2) |
| P1 | Inventory Help and glyphs for Gen I–V:<br>• + Review is described correctly;<br>• the stale "X Add" line is removed;<br>• Gen V Help is titled "Inventory Controls". | B04, B03 follow-up |
| P1 | Gen I–IV Trainer looks exactly like Gen V: neutral values, no patched-in boxes, and a narrow Review button that overlaps nothing | B06, C06, B02/A30 follow-ups |
| P1 | Gen II Special in the lower-middle STATS area, using the Gen I reference | A22, C01 |
| P1 | Gen II Unown letter, Shiny, DVs and Reroll behave understandably and stay valid | B07, C09 |
| P1 | Gen V exports appear in Save Backups and open as a staged copy (G1) | C12 follow-up |
| P2 | Event and Met Location descriptions: meaningful and source-backed, with uncertainty stated honestly | B05 (A15/A26 preserved) |
| P2 | Pokémon editor typography the same across Gen I–V | Other |
| P2 | Tap outside also closes Quick Games and other ordinary popups | C04 follow-up |

Every Round 7 item is NOT DEVICE TESTED until the owner tests the Round 7 NRO.
