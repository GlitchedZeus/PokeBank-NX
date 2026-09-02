# PokeBank NX — Session Log — 2026-09-02

Historical daily log. `PROJECT_STATUS.md` remains authoritative for current verified state.

---

## 1. First physical Switch milestone recorded

Exact tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
Size: 9,707,957 bytes
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

The short pass verified boot, D-pad, Action Sheet, B/Cancel, shoulder controls in exercised contexts, `+`, `-`, all three themes, theme persistence, Party/Boxes/Storage and no crash during that shorter pass.

Known failures from the short pass:

```text
Left Stick held navigation  FAIL
visible PokeBank NX shell   FAIL / incomplete
```

Issue #8 was completed because the first exact physical test occurred. #13 and #19 remained open for the failed gates.

---

## 2. Session 2.5 visible-shell / physical-input source published

Application source checkpoint:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

Reported/source-reviewed work includes:

```text
real libnx Left Stick position handling
analog deadzone / hysteresis
single-tap + held navigation path
diagonal stability work
PokeBank NX visible header/chrome/cards
Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

GitHub host CI passed.

The coding session ended before the final clean exact-source artifact was packaged/preserved. Therefore this source remains **NOT DEVICE TESTED**.

---

## 3. Extended hardware torture test of first binary

A longer pass was completed on the same exact `3be4de6b...` artifact.

Permanent report:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

New stability evidence:

```text
5-minute idle                 PASS
10-minute browsing            PASS
5 relaunches                  PASS
heavy Action Sheet repetition PASS
held D-pad                    PASS
```

Left Stick failure clarified:

```text
single taps all directions  FAIL — no input/action
held all directions         FAIL — no input/action
diagonal                    FAIL — no input/action
```

This changed #19 from a held-repeat-only bug into a full physical Left Stick navigation failure on the old binary.

---

## 4. New safety evidence from inherited PKSE UI

Physical testing proved these mutation-oriented controls remain reachable in game-save views:

```text
Release
Create Pokémon
Move / Multi
editable Pokémon view
apply/save-style flow
```

The tester changed an IV through the inherited editor, applied it, reopened it, then restored the original value and applied again.

The test did **not** complete the final external verification against the actual original Pokémon game's installed save, so the evidence boundary is:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

A source-level audit/blocker issue was created:

```text
#23 Audit and lock inherited live-save mutation UI before second device build
```

Current-alpha requirement: installed-game sources must present read-only behavior. Unsafe Release/Create/Move/Edit/save paths must not remain reachable while live writes are disabled.

---

## 5. Legacy/app Storage persistence confirmed

An Arbok moved into inherited/app Storage remained present later.

This proves Storage is real persistent app-owned or otherwise persistent state, but does not yet prove how its source-side move semantics interact with installed saves.

Issue created:

```text
#27 Clarify writable app Storage versus future Master Vault / Banks
```

Future Vault work should preserve/migrate legitimate legacy Storage data where feasible rather than silently discarding it.

---

## 6. Legends Arceus old-save crash found

One older Legends: Arceus save reproducibly crashes the first tested binary while another save does not show the same immediate failure.

Issue created:

```text
#24 Handle old / malformed Legends Arceus saves without crashing
```

Required behavior is graceful read-only failure, no automatic repair/write, useful error, and safe return to UI.

---

## 7. Additional UI/controller observations

Extended testing observed:

```text
X in Boxes       Dex-sort behavior
Y in Boxes       inherited Menu / Move / Multi
L/R repeated     account switching in tested context
ZL/ZR            no visible action in tested context
Right Stick      no current action
```

Issue created:

```text
#26 Normalize box navigation/controller semantics across game saves and Storage
```

Summary/View also lacked a Pokémon visual/model, leading to:

```text
#25 Add Pokémon render/model support to Summary and View screens
```

3D model rotation with Right Stick is a later optional QoL idea only if practical; a strong sprite/artwork Summary is sufficient.

---

## 8. Session 2.6 blocker prompt created

Permanent current prompt:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Order before the second artifact:

```text
#23 source-level mutation safety audit
#24 PLA crash hardening
preserve #19 Session 2.5 analog work
preserve #13/#16 PokeBank NX visible shell
        ↓
new application-source checkpoint if code changes
        ↓
host tests / sanitizer / diff check / native build
        ↓
exact replacement .nro + hash/preservation
        ↓
second physical Switch test
```

Do not begin PKSM-Core until this hardware gate is stable enough.

---

## 9. True Move semantics made permanent

Product semantics were clarified and recorded:

```text
COPY  = intentionally keep source + create destination representation
MOVE  = relocate active Pokémon; source removed/retired only after verified destination success
CLONE = deliberate duplicate with clone provenance
```

Issue:

```text
#20 Implement true Move semantics after validated live-write adapters
```

True Move is intentionally later. Current installed saves remain read-only.

---

## 10. NRO reliability / QoL backlog made permanent

Issue #21 and `docs/NRO_QUALITY_ROADMAP.md` track:

```text
Diagnostics + export
Applet/constrained-memory handling
privacy-safe crash logs
real startup stages
READ ONLY / VAULT / STAGED badges
Vault recovery / storage health
search/filter / Quick Jump / Favorites / recent items
text size / Reduced Motion / non-color-only focus
optional original sounds / restrained rumble
virtualized grids / bounded artwork caches / lazy entity loading
```

Recommended post-second-device effort balance remains approximately:

```text
70% core functionality
20% hardware validation / bug fixing
10% polish / infrastructure
```

---

## 11. Additional bank/save-manager research audited

New reference projects inspected:

```text
FlagBrew/PKSM
Universal-Team/pkmn-chest
gocario/PHBank
0xb01u/PHBankGBC
```

Permanent audit:

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
```

Important findings:

- PKSM full-app BankFile/versioning/corruption/backups are useful references for Vault parser hardening and optional legacy import.
- Pokémon Chest demonstrates another Nintendo-platform application wrapping PKSM-Core and gives useful Game↔Bank interaction patterns.
- PHBank closely matches the desired offline Game-PC ↔ Bank UX and motivates a future dedicated Transfer Workspace with one/multi/whole-box operations.
- PHBankGBC is a secondary Gen I/II offsets/save reference only; its historical constants must be verified against stronger sources/fixtures.

These references do not replace PokeBank NX's Master Vault/provenance architecture.

---

## 12. Full v1.0 roadmap published

Canonical roadmap:

```text
docs/V1_ROADMAP.md
```

Master tracking issue:

```text
#29 Track PokeBank NX v1.0 roadmap and release gates
```

README now includes a public full checklist showing completed, in-progress and planned milestones through v1.0.

Roadmap coverage includes:

```text
recovery/safety
Action Sheet + UI/device gates
PKSM-Core / Gen III
Gen I/II / RetroArch
Master Vault + Banks
Summary/provenance
PKHeX Oracle / fixtures
modern Switch reads
conversion/staging
Pokédex/Living Dex
legality/edit/generation/events
staged-save architecture
per-game live writes
true Move
official-game → official HOME bridge workflow
NRO diagnostics/accessibility/performance
release-candidate torture testing
v1.0 release
```

---

## End-of-day handoff

Application source has **not** changed during the documentation/research sweep.

Useful Session 2.5 application source remains:

```text
361c6f551496470db305948d702944c6ed9889c1
```

It remains:

```text
IMPLEMENTED
GITHUB HOST CI PASS
NOT DEVICE TESTED
```

The branch contains later documentation/research commits. Do not confuse branch HEAD with application-source identity.

Next engineering action remains:

```text
Read PROJECT_STATUS.md
Read docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
Execute docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Stop that engineering session only after the replacement second-device `.nro` is safely built/preserved and labeled:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```