# PokeBank NX — First-Build Hardware Follow-Up

Status: **DEVICE TESTED — PARTIAL PASS / SAME EXACT FIRST ARTIFACT**  
Date: 2026-09-03  
This is a follow-up to `docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`, not a test of a new binary.

## Exact tested build

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
Size: 9,707,957 bytes
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
```

The tester continued stress-testing the same artifact and completed more of the extended checklist.

---

# New / clarified hardware evidence

## Stability remains strong outside the known PLA case

Previously observed and reconfirmed during continued use:

```text
5-minute idle                       PASS
10-minute normal browsing           PASS
5 repeated launches                 PASS
Action Sheet heavy repetition       PASS (~100 opens/closes, no lag/stuck input)
HOME -> app resume                   PASS
sleep -> wake -> app                 PASS
Joy-Con detach/reconnect             PASS
controller disconnect/reconnect      PASS
input after reconnect                PASS
stuck focus after resume             NONE OBSERVED
```

The only known reproducible crash remains one older Pokémon Legends: Arceus save. Issue #24 remains open.

---

# Input clarification

## D-pad

Held D-pad remains a strong hardware pass in all four directions.

## Left Stick

No change from the earlier result on this old binary:

```text
single tap Up/Down/Left/Right        FAIL — no input/action
hold Up/Down/Left/Right              FAIL — no input/action
diagonal                              FAIL — no input/action
```

Issue #19 remains open. The later `361c6f55...` source contains the replacement analog implementation but is still not device-tested.

## Right Stick

No action is currently implemented/observed. This is not classified as a navigation regression because Right Stick is not part of the current required navigation contract. A future optional use is Pokémon model/render rotation in Summary if #25 later adopts a practical 3D path.

## Other inherited behavior

```text
X in Boxes             sorts by Dex
Y in Boxes             Menu / Move / Multi modes
Y on Pokémon           grab/move behavior
L/R in tested context  account switching
ZL/ZR in tested context no visible action
```

Controller normalization remains #26.

---

# Action Sheet / View / Edit

Party, Boxes and inherited Storage continue to pass the shared Action Sheet open/order/cancel behavior.

Unsupported actions correctly report `Not yet supported`.

`View Pokémon` opens and returns safely but still lacks the expected Pokémon visual in this artifact.

`Edit` remains reachable and writable against the in-memory/backup representation. An IV edit was saved in the app flow, reopened successfully, then restored to the original value. This reinforces the #23 source classification that the inherited editor is real and must be blocked or made explicitly copy-based for installed-source browsing during the read-only phase.

---

# Summary data sample

Four Pokémon were explicitly sampled:

```text
Bulbasaur
Alolan Meowth
Mewtwo
Mew
```

For those samples, the tester reported:

```text
View opens            PASS
level                  PASS
gender                 PASS
shiny display          PASS
B returns safely       PASS
nickname               N/A for tested Pokémon
```

The checklist field named `SPECIES` was interpreted by the tester as if it meant Pokémon type. Future Summary UX should make both concepts explicit instead of relying on ambiguous labels:

```text
Species: Bulbasaur
Type: Grass / Poison
```

The tester also wants the future View/Pokédex experience to include the Pokémon visual, cry and a short Pokédex-style description/location information. Those remain product/UX work under #9/#25/#35/#7 rather than blockers for the second safety artifact.

---

# Theme torture / readability

Theme switching was cycled repeatedly and passed without crashes or broken panels.

```text
OLED Black              PASS
Dark                    PASS
Light                   PASS
Action Sheet OLED       PASS
Action Sheet Dark       PASS
Action Sheet Light      PASS
Theme persistence       PASS after restart/sleep
```

New visual feedback:

- bottom controller/hint graphics are too low-contrast in OLED Black and Dark;
- D-pad and ABXY hints should stand out more while remaining professional;
- OLED Black currently gives the tester the preferred dark Action Sheet appearance;
- Dark could be approximately 10% darker;
- Light color treatment should remain substantially as-is.

This is polish/accessibility input for #13/#21/#26 and must not derail the current safety/crash gate.

---

# Lifecycle / handheld evidence

```text
HOME -> return                   PASS
sleep -> wake                    PASS
Joy-Con detach/reconnect         PASS
controller disconnect/reconnect  PASS
input after reconnect            PASS
handheld text readability        PASS
handheld focus visibility        PASS
```

Docked mode was **NOT TESTED** in this pass.

The checklist's `hint bar` wording was unclear to the tester. Future device-test sheets should call it the **bottom controller-hint / button-hint bar**.

---

# Game/source detection evidence

Explicit cards recorded during this follow-up include:

```text
Pokémon Shining Diamond  — Nintendo Switch — installed — opens — no duplicate
Let's Go Pikachu         — Nintendo Switch — installed — opens — no duplicate
```

The tester additionally reported:

```text
FireRed Switch not identified as GBA by mistake      PASS
LeafGreen Switch not identified as GBA by mistake    PASS
FireRed/LeafGreen Switch identities remain distinct  PASS
wrong platform                                         NONE OBSERVED
wrong save                                             NONE OBSERVED
duplicate card                                         NONE OBSERVED
non-Pokémon false positive                             NONE OBSERVED
```

Legacy GB/GBC/GBA saves stored under RetroArch were not discovered automatically. This is expected unfinished work, not a regression in the current native-Switch discovery path. RetroArch/manual legacy discovery remains issue #6.

DS/3DS support is planned separately and is not expected from this first artifact.

---

# Most important safety clarification

The exact observed Arbok flow was clarified by the tester:

```text
1. Select Pokémon Legends: Z-A installed source.
2. PokeBank NX creates/uses its backup copy.
3. Open the Z-A BACKUP representation.
4. Move Arbok from that backup representation into inherited app Storage.
5. Exit back to the PokeBank NX main menu.
6. Select another supported game and open its backup/session.
7. Open inherited Storage.
8. Arbok is still present in Storage.
9. The UI appears capable of placing Arbok into the other game's loaded backup representation when compatible.
```

The source-side installed Z-A save itself was **not removed from or changed by that Storage transfer** according to the tester's follow-up check.

This means the physically demonstrated transfer pipeline is:

```text
INSTALLED GAME
   |
   | read / automatic backup
   v
BACKUP REPRESENTATION
   |
   | mutable inherited PKSE operation
   v
APP-OWNED LEGACY STORAGE (PKSEBANK / bank.dat)
   |
   | persists across leaving one game and opening another
   v
OTHER GAME'S LOADED BACKUP REPRESENTATION
```

It is **not**:

```text
installed Z-A save -> destructive true Move -> Storage
```

and it is **not** the future PokeBank NX Master Vault.

## Current classification

Combined with the source-level audit in `docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md`, current evidence is:

```text
LIVE INSTALLED SAVE WRITE OBSERVED:      NO
ORIGINAL INSTALLED SAVE CHANGED:         NO — tester reports unchanged in exercised flow
LOW-LEVEL LIVE-WRITE HARD LOCK:          SUPPORTED BY SOURCE + THIS PHYSICAL CHECK
USER-REACHABLE MUTATION UI:              YES
BACKUP/IN-MEMORY SAVE MUTATION:          YES
APP-OWNED LEGACY STORAGE WRITE:          YES
LEGACY STORAGE CROSS-GAME PERSISTENCE:   YES
STORAGE -> OTHER BACKUP UI PATH:         REACHABLE WHEN COMPATIBLE; final persistence not exhaustively tested here
ARBOK IN LEGACY STORAGE:                 PERSISTS
MASTER VAULT IMPLEMENTED:                NO
TRUE MOVE IMPLEMENTED:                   NO
```

## Arbok interpretation

The Arbok event is best described as a **persistent copy/import into app-owned legacy Storage from a mutable backup representation**.

The word `move` is appropriate for the inherited UI operation inside that backup/storage workspace, but it must not be confused with the product-level `MOVE` defined for future PokeBank NX true relocation semantics.

Current observed result:

```text
original installed Z-A save  -> still contains original source state
Z-A backup representation    -> can be mutated independently
legacy Storage               -> contains persistent Arbok
```

Therefore the user can effectively use the old PKSE foundation as an offline intermediate bank between backup representations without touching the installed source save. This is useful proof-of-concept behavior for future Game ↔ Vault transfer UX, but the legacy storage engine is not trusted as the final Vault architecture.

## Why #23 still remains a blocker

The low-level hard lock behaving correctly does not make the current UI acceptable.

While installed-game browsing is advertised as read-only, the user can still reach inherited:

```text
Release
Create Pokémon
Move / Multi
writable Edit
backup save/apply flows
```

Session 2.6 must still block or clearly separate those paths so a user cannot mistake backup/in-memory mutation for an approved live-save editor.

The better long-term UX should distinguish clearly between:

```text
INSTALLED SOURCE — READ ONLY
BACKUP / STAGED COPY — MUTABLE WHEN EXPLICIT
LEGACY STORAGE — APP-OWNED / COMPATIBILITY
MASTER VAULT — FUTURE AUTHORITATIVE STORAGE
LIVE GAME WRITE — LATER, PER-ADAPTER SAFETY GATE
```

---

# Updated first-artifact physical summary

```text
BOOT / RELAUNCH                  PASS
GENERAL STABILITY                PASS except known old-PLA crash
D-PAD                            PASS
LEFT STICK                       FAIL — no navigation input
ACTION SHEET                     PASS / heavy repetition stable
VIEW DATA                        PASS on sampled Pokémon
VIEW POKÉMON VISUAL              FAIL / missing in artifact
THEMES                           PASS
THEME PERSISTENCE                PASS
DARK/OLED BOTTOM-HINT CONTRAST   NEEDS POLISH
HOME / SLEEP / RESUME            PASS
CONTROLLER RECONNECT             PASS
HANDHELD                         PASS
DOCKED                           NOT TESTED
NATIVE SWITCH SAVE DISCOVERY     PASS / PARTIAL based on sampled titles
RETROARCH LEGACY DISCOVERY       NOT IMPLEMENTED / not detected
USER-FACING READ-ONLY CONTRACT   FAIL / inherited mutation UI remains
LIVE INSTALLED SAVE WRITE        NOT OBSERVED
ORIGINAL SAVE UNCHANGED          TESTER-REPORTED PASS in exercised flow
APP LEGACY STORAGE PERSISTENCE   PASS / physically observed
CROSS-GAME STORAGE VISIBILITY    PASS / Arbok visible after switching games
OLD PLA SAVE                     REPRODUCIBLE CRASH
VISIBLE POKEBANK NX IDENTITY     still old-build failure/incomplete
```

---

# Effect on Session 2.6

The next coding session remains the same critical gate, but it now starts with better evidence:

1. #23 — preserve the low-level hard lock and remove/disable confusing installed-source mutation UI;
2. #24 — harden old/malformed PLA loading;
3. #19 — preserve and physically validate the newer Left Stick implementation;
4. #13/#16 — preserve the newer visible PokeBank NX shell;
5. #37 — run the device asset preflight so View Pokémon artwork is not accidentally omitted from the replacement build;
6. preserve the useful concept of an app-owned cross-game intermediate storage layer without treating inherited PKSE Storage as the future Master Vault;
7. package/hash/preserve the exact new `.nro` and stop for physical test #2.

Do not reinterpret this follow-up as authorization for live writes, true Move, Master Vault work, PKSM-Core work, or a broad UI redesign.
