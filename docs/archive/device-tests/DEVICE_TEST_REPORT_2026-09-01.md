# PokeBank NX — Physical Switch Test Report — 2026-09-01

Status: **DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES**

This is the first recorded physical Nintendo Switch hardware test of an exact PokeBank NX `.nro` with a published application-source SHA and binary SHA-256.

## Exact tested build

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Application commit:
fix: restore complete controller UI source

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Embedded commit:
3be4de6b
```

The artifact was physically launched and exercised on the project owner's Nintendo Switch.

## Test matrix

| Test | Result |
|---|---|
| Boot / launch | PASS |
| D-pad navigation | PASS |
| Left Stick + held repeat | **FAIL** |
| A → Pokémon Action Sheet | PASS |
| B / Cancel | PASS |
| L / R navigation | PASS |
| ZL / ZR navigation | PASS |
| `+` contextual Options | PASS |
| `-` Help / Controls | PASS |
| OLED Black theme | PASS |
| Dark theme | PASS |
| Light theme | PASS |
| Theme persistence after restart | PASS |
| Party browsing | PASS |
| Boxes browsing | PASS |
| Storage browsing | PASS |
| Crashes | NONE OBSERVED |

## Action Sheet hardware result

The controller-first Pokémon Action Sheet from issue #2 was successfully exercised on physical hardware through this combined build.

Observed successful behavior includes:

- A opens the Action Sheet on Pokémon;
- B / Cancel close it safely;
- Party / Boxes / Storage paths are reachable;
- no crash was observed;
- no accidental live installed-save mutation was reported.

This provides physical evidence for the Action Sheet interaction path even though the earlier Action-Sheet-only binary itself was not separately tested.

## Theme / controller hardware result

Successful physical checks:

- D-pad navigation;
- L/R;
- ZL/ZR;
- contextual `+` Options;
- read-only `-` Help;
- OLED Black;
- Dark;
- Light;
- persisted theme selection.

Failed physical check:

```text
LEFT STICK + HOLD: FAIL
```

Tracked by:

```text
GitHub issue #19 — Fix held Left Stick navigation repeat on physical Switch
```

The next input fix should investigate analog thresholds/deadzones and whether sustained stick direction reaches the repeat model correctly on real hardware.

## Major visual/product finding

Although the semantic themes and controller behavior are present, the user-visible application still appears overwhelmingly like inherited PKSE.

Physical observation:

- PKSE visual structure remains dominant;
- PKSE branding/logo remains visible;
- inherited screens look materially the same as PKSE;
- the strongest visible evidence that the new binary is PokeBank NX is currently the new `+` Options behavior/text;
- the theme infrastructure is not yet enough to make the product visibly distinct.

This is considered a **failed/incomplete product-shell acceptance result**, not a reason to throw away the underlying theme/control work.

Issue #13 was reopened.

Issue #16 branding/startup work is elevated into the next visible-shell milestone.

## Safety result

No safety regression was reported during this physical pass.

The live installed-game save-write hard lock remains mandatory.

No result in this report authorizes live writes.

## Tracking outcome

```text
Issue #8   CLOSED — first exact physical .nro test completed
Issue #13  REOPENED — visible PokeBank NX shell incomplete
Issue #19  OPEN — held Left Stick repeat hardware bug
Issue #16  OPEN — PokeBank NX branding/startup/NRO metadata; now near-term visual dependency
```

## Next device build acceptance target

The replacement `.nro` should satisfy all of:

```text
Boot                              PASS
D-pad                             PASS
Left Stick + held repeat          PASS
A/B Action Sheet                  PASS
L/R + ZL/ZR                       PASS
+ / -                             PASS
OLED Black / Dark / Light         PASS
Theme persistence                 PASS
Party / Boxes / Storage           PASS
No crashes                        PASS
No live-write safety regression   PASS
Visibly PokeBank NX               PASS
Obvious PKSE branding removed     PASS
```

The next UI pass should focus on the top-level visible identity rather than attempting to redesign every future screen at once.