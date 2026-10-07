# PokeBank NX — Extended First-Device Hardware Report

Status: **DEVICE TESTED — PARTIAL PASS / NEW SAFETY + CRASH FINDINGS**  
Date: 2026-09-02  
This extends, rather than replaces, `docs/DEVICE_TEST_REPORT_2026-09-01.md`.

## Exact tested build

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
Size: 9,707,957 bytes
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
```

The tester performed a longer exploratory/torture pass on the same first physically tested binary.

---

## Stability

```text
5 minutes idle                 PASS
10 minutes normal browsing     PASS
5 repeated relaunches          PASS
Action Sheet ~100 open/close   PASS / no lag / no stuck input
Old PLA save                   REPRODUCIBLE CRASH
Other PLA save                 opens more successfully
```

The old Legends: Arceus save crash is tracked by issue #24.

---

## D-pad / Left Stick clarification

Held D-pad behavior is strong on hardware:

```text
Held D-pad Up       PASS
Held D-pad Down     PASS
Held D-pad Left     PASS
Held D-pad Right    PASS
```

The earlier `LEFT STICK + HOLD: FAIL` result is now clarified. The Left Stick provides **no navigation input at all** on the tested `3be4de6b` binary:

```text
single tap Up       FAIL — no input/action
single tap Down     FAIL — no input/action
single tap Left     FAIL — no input/action
single tap Right    FAIL — no input/action
hold Up             FAIL — no input/action
hold Down           FAIL — no input/action
hold Left           FAIL — no input/action
hold Right          FAIL — no input/action
diagonal             FAIL — no input/action
```

Issue #19 has been updated accordingly.

The later Session 2.5 source `361c6f55...` contains a reported real-libnx analog path but remains **NOT DEVICE TESTED**.

---

## X / Y / inherited controller behavior

Observed on the first build:

- `X` on Select Game: no visible action.
- `X` in Boxes: sorts by Dex.
- `X` on a Pokémon in an inherited Pokémon view can expose **Release**.
- `Y` on Select Game: no visible action.
- `Y` in Boxes: exposes inherited `Menu / Move / Multi` behavior.
- `Y` can grab/move a Pokémon around a box.
- Right Stick: no current action.

Desired controller normalization is tracked by issue #26.

A future nice-to-have is Right Stick model rotation in Summary/View if 3D rendering is practical; issue #25 tracks Pokémon visual presentation.

---

## Action Sheet

Party, Boxes, and Storage all opened the shared nine-item Action Sheet successfully.

```text
View Pokémon              PASS
Add to Master Vault       Not yet supported
Add to Bank...            Not yet supported
Transfer to Game...       Not yet supported
Edit                       opens inherited editable view
Clone                      Not yet supported
Make Shiny                 Not yet supported
Legality & Provenance      Not yet supported
Cancel                     PASS
B to close                 PASS
```

Menu order was correct in Party / Boxes / Storage.

Repeated open/close remained stable with no stuck input or crash.

### Pokémon visual presentation

`View Pokémon` / inherited editable details show data but no Pokémon image, sprite, or model in the tested path. Issue #25 tracks proper Summary/View Pokémon visuals.

---

## Important mutation/safety observations

Extended testing exposed inherited PKSE mutation controls that require a source-level audit before the second device build is handed over.

Observed:

- `X` can expose **Release** on a Pokémon.
- `Y` can expose **Move / Multi** and move Pokémon within a box.
- `A` on an empty game-save slot can expose **Create Pokémon**.
- Action Sheet `Edit` opens an editable Pokémon view.
- The tester changed an IV, applied/saved it within the app flow, reopened it, then restored the original value and applied it again.
- exiting with changes can present an unsaved-changes dialog.
- inherited/app `Storage` is writable/persistent; an Arbok moved from a Legends Z-A context into Storage remained present.

### Evidence boundary

The extended checklist did **not** complete the final external verification step of launching the original game and proving the installed title save changed or remained unchanged.

Therefore:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

Do not describe this as a confirmed live-save write regression until the final persistence target is traced or physically verified.

However, these inherited mutation paths are inconsistent with the current alpha UX contract and must be audited/blocked where appropriate before the second device-test artifact.

Tracked by issue #23.

Legacy/app Storage versus future Master Vault/Banks is tracked by issue #27.

---

## Empty slots

Observed:

```text
A on empty game-save slot      exposes Create Pokémon
X on empty game-save slot      no action
Y on empty game-save slot      no action
+                              Settings
-                              Controls overlay
```

In app Storage, X/Y can still expose inherited sort / Menu-Move-Multi behavior.

No crash was observed from empty-slot testing.

---

## Navigation torture

Observed in the tested context:

```text
R repeatedly          switches accounts
L repeatedly          switches accounts
ZR repeatedly         no visible action
ZL repeatedly         no visible action
rapid mix             stable; only L/R visibly active
box/focus corruption  NONE OBSERVED
content disappearance NONE OBSERVED
stutter               NONE
crash                  NONE
```

This differs from the intended PokeBank NX controller contract in some contexts and is tracked by issue #26.

---

## Game detection evidence from this pass

Two cards were explicitly recorded during the extended pass:

```text
Pokémon Shining Diamond — Nintendo Switch — installed — opens — no duplicate
PLGP / Let's Go Pikachu — Switch — installed — opens — no duplicate
```

The extended checklist did not finish the full FireRed/LeafGreen identity or all-installed-game detection matrix, so do not infer those items passed from this report.

---

## Not completed in this extended pass

The uploaded checklist left these larger sections mostly incomplete:

```text
full Party/Boxes data sanity matrix
five-Pokémon Summary data validation
theme torture loop
HOME/resume/sleep/controller reconnect
handheld/docked comparison
full live-write safety sanity matrix
launch-original-game final unchanged-save verification
final summary fields
```

The earlier first-device report still provides physical evidence for themes, Party/Boxes/Storage browsing, Action Sheet, +/-, L/R/ZL/ZR in the originally exercised contexts, and no crash during that shorter pass.

This extended report supersedes only the specific facts it clarifies, especially:

```text
Left Stick = no input at all on 3be4de6b
one old PLA save = reproducible crash
inherited mutation UI = physically reachable
```

---

## Next device-build blockers

Before handing over the second replacement `.nro`:

1. issue #23 — trace/block inherited live-save mutation UI and classify persistence targets;
2. issue #24 — prevent old/malformed PLA save crash or fail gracefully;
3. issue #19 — preserve/test the new analog input fix;
4. issue #13/#16 — preserve the visible PokeBank NX shell/identity;
5. host tests + ASan/UBSan + `git diff --check` + native build;
6. record a new exact application source SHA and artifact SHA-256.

If #23 or #24 requires application-source changes, `361c6f55...` must no longer be presented as the final second-device application source. Create a new application-source checkpoint and build/hash from that exact commit.
