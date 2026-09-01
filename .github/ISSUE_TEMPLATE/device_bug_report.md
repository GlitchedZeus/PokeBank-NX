---
name: Physical Switch bug report
about: Report a reproducible PokeBank NX issue from an exact .nro build
labels: ''
assignees: ''
---

## Exact build identity

```text
Application source SHA:
.nro filename:
.nro size:
.nro SHA-256:
Version shown in app:
```

Do not report an unidentified binary if the source/hash cannot be determined.

## Switch environment

```text
Switch firmware:
Atmosphère / CFW version:
Launch method (hbmenu/forwarder/etc.):
Handheld or docked:
SD card filesystem if relevant:
```

## Game / source involved

```text
Game:
PokeBank stable game ID if known:
Source type (installed save / backup / imported file / other):
```

Do not attach personal save data publicly unless it has been deliberately sanitized and is safe/authorized to share.

## What happened

### Exact steps

1.
2.
3.

### Expected result

Describe what should have happened.

### Actual result

Describe what happened instead.

## Controller/input details

List the exact buttons/sticks used immediately before the problem, for example:

```text
D-pad Right
A
+
B
```

## Reproducibility

- [ ] Always
- [ ] Sometimes
- [ ] Happened once

If intermittent, estimate frequency.

## Crash / error evidence

```text
Crash/error code:
Error text:
Screenshot/photo available: yes/no
Atmosphère crash report available: yes/no
```

## Safety check

- [ ] No source Pokémon/save changed unexpectedly.
- [ ] Unexpected mutation occurred — STOP testing that path and describe exactly what changed.
- [ ] A live installed-save write path appeared unexpectedly — treat as a SAFETY REGRESSION.

## UI/theme state if relevant

```text
Theme: OLED Black / Dark / Light
Screen: Select Game / Party / Boxes / Action Sheet / Summary / Help / other
Focus visible: yes/no
Bottom hints correct: yes/no
Theme persisted after restart: yes/no/not relevant
```

## Additional notes

Anything else needed to reproduce the issue.

Use `docs/DEVICE_TEST_CHECKLIST.md` for the full physical-test procedure.