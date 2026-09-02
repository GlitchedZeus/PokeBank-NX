# PokeBank NX — Session Log — 2026-09-01

This dated log records the major recovery, documentation, implementation, and physical-device milestones reached during the September 1 workday. `PROJECT_STATUS.md` remains authoritative when this historical log and current state differ.

## Recovery / repository work

- Recovered the interrupted 23-game identity work.
- Preserved distinct GBA/Switch FireRed and LeafGreen identities.
- Re-established the writable `origin` / upstream-only PKSE separation.
- Confirmed the requested older commit `1932cf0` was not recoverable from available refs/reflogs/stashes/unreachable history/archive.
- Established permanent recovery/session/runbook documentation.

## Safety milestone

Published source milestone:

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes
```

Live installed-game save writing remains hard disabled.

## Session 1 — Action Sheet

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet
```

Result:

- shared nine-item controller-first Action Sheet across Party/Boxes/Storage;
- A opens deliberate actions rather than mutating immediately;
- B/Cancel safely close;
- View read-only;
- unfinished actions safe/not-yet-supported;
- host tests PASS;
- sanitizers PASS;
- native `.nro` BUILDS.

Issue #2 was closed/completed.

Recorded artifact:

```text
PokeBank-NX-ActionSheet-82a0779.nro
9,695,669 bytes
SHA-256 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
```

The Action-Sheet-only artifact was not separately physically tested.

## Session 2 — interrupted then recovered

The first Session 2 run ran out of usage after implementing substantial local HOME-style control/theme work.

A later recovery/continuation session found/recovered the work and produced the verified application source:

```text
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
fix: restore complete controller UI source
```

Implemented foundation included:

- semantic OLED Black / Dark / Light palettes;
- persisted theme selection;
- context-aware typed button hints;
- contextual `+` Options / More behavior;
- read-only `-` Help;
- held navigation repeat model;
- focus/card/panel/modal primitives;
- Action Sheet visual integration foundation;
- ZL/ZR box jumps;
- host tests expanded to six suites.

Verification:

```text
host tests        PASS
ASan/UBSan        PASS
git diff --check  PASS
native .nro       BUILDS
```

Session 2 artifact:

```text
PokeBank-NX-UI-Theme-3be4de6.nro
9,707,957 bytes
SHA-256 df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
```

## FIRST PHYSICAL SWITCH HARDWARE MILESTONE

The exact `3be4de6b` Session 2 binary was physically run on Nintendo Switch hardware.

Result:

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK + HOLD        FAIL
A ACTION SHEET           PASS
B/CANCEL                 PASS
L/R                      PASS
ZL/ZR                    PASS
+                        PASS
-                        PASS
OLED BLACK               PASS
DARK                     PASS
LIGHT                    PASS
THEME PERSISTENCE        PASS
PARTY                    PASS
BOXES                    PASS
STORAGE                  PASS
CRASHES                  NONE
```

Major product finding:

The functional/control/theme foundation works substantially better than the visible product identity. The application still looks overwhelmingly like PKSE, including obvious inherited PKSE branding/logo and screen identity.

Therefore:

```text
Issue #8   CLOSED — first exact physical .nro test completed
Issue #13  REOPENED — visible PokeBank NX shell incomplete
Issue #19  OPEN — held Left Stick navigation repeat failure
Issue #16  OPEN / elevated — visible branding/startup/NRO identity
```

Permanent physical report:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
```

No live installed-save write/safety regression was reported during this hardware pass.

## Next execution target

HIGH session:

```text
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

Goal:

- fix held Left Stick repeat;
- remove obvious PKSE product branding from normal tested flow;
- make Select Game/top-level shell visibly PokeBank NX;
- preserve all physical passes;
- produce a second device-test `.nro`.

After the replacement hardware pass is stable enough, move to MAX for issue #4 PKSM-Core PK3/Sav3 Gen III integration.

## Documentation / project-management work completed today

Created or updated permanent documentation for:

- project status;
- project map;
- session runbook;
- next-session plan;
- controller contract;
- UI flow/style guide;
- architecture;
- save safety;
- build records;
- device test checklist/report;
- PKSM-Core integration plan;
- PKHeX Oracle;
- Master Vault;
- transfer model;
- Pokédex;
- modern Switch reference research;
- release/artifact discipline.

Supporting issues created during the day include artifact preservation, branding/startup identity, golden fixtures, and the physical Left Stick regression.
