# PokeBank NX — Next Session Plan

Last updated: 2026-09-02  
Status: ACTIVE EXECUTION PLAN

`PROJECT_STATUS.md` is authoritative.

## Current position

Completed:

```text
#2  Controller-first Pokemon Action Sheet
#8  First exact physical PokeBank NX .nro test
```

First physically tested build:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

That test exposed:

```text
LEFT STICK + HOLD          FAIL
VISIBLE POKEBANK NX UI    FAIL / INCOMPLETE
```

Session 2.5 then produced/pushed a new application source checkpoint:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on this exact source.

The session ended before the clean exact-source rebuild, artifact hash/preservation, docs closeout, and second hardware handoff.

---

# Block A — HIGH — finish/package Session 2.5

Use:

```text
docs/PROMPT_SESSION2_5_FINISH.md
```

Do **not** redo the visible-shell/analog implementation unless verification finds a real regression.

Required application source for the replacement artifact:

```text
361c6f551496470db305948d702944c6ed9889c1
```

Required work:

```text
verify exact source
host tests
ASan/UBSan
git diff --check
clean native build from exact source
preserve actual .nro
record filename / embedded SHA / size / SHA-256
update #13/#16/#19
update build/status docs
READY FOR SECOND DEVICE TEST
```

Keep #13 and #19 open until physical acceptance.

Do not merge a later docs-only SHA into the artifact identity.

---

# Block B — second physical Switch test

Target:

```text
BOOT                              PASS
D-PAD                             PASS
LEFT STICK                        PASS
LEFT STICK + HOLD                 PASS
A/B ACTION SHEET                  PASS
L/R + ZL/ZR                       PASS
+ / -                             PASS
OLED BLACK / DARK / LIGHT         PASS
THEME PERSISTENCE                 PASS
PARTY / BOXES / STORAGE           PASS
NO CRASHES                        PASS
NO LIVE-WRITE REGRESSION          PASS
VISIBLY POKEBANK NX               PASS
OBVIOUS PKSE BRANDING REMOVED     PASS
```

If the replacement build has a device-only regression, do the smallest coherent HIGH fix/build/retest loop.

---

# Block C — MAX — PKSM-Core Gen III spike

Start only after the Session 2.5 replacement build is stable enough on hardware.

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

Issue #4 target:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
party / box extraction
active save-slot / sector behavior
PK3 encrypt/decrypt/checksum behavior
untouched round-trip strategy
adapter/dependency decision
```

Issue #17 supports reproducible fixtures. Do not let fixture perfection block the first useful integration decision.

No live writes.

---

# Core product order after PKSM-Core

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokedex / Living Dex
#10 Conversion / transfer without live writes
#20 True Move semantics after verified safe-write adapters
```

Supporting work:

```text
#15 persistent .nro artifact automation
#16 final branding/startup/icon/NACP
#17 golden fixture corpus
#21 NRO reliability/diagnostics/performance/QoL
```

---

# Transfer semantics going forward

Product behavior is now explicit:

```text
COPY  = source stays active; destination representation is created
MOVE  = destination becomes active; source stops being active after verification
CLONE = deliberate duplicate with clone provenance
```

Current alpha remains read-only against installed games and therefore uses safe Copy/import/staged behavior.

True Move is **later**, not now. It requires per-adapter backup, stage, checksum/container repair, reparse, write, readback, verification, rollback, and physical hardware gates.

See:

```text
docs/TRANSFER_MODEL.md
GitHub issue #20
```

---

# NRO quality / polish plan

Detailed backlog:

```text
docs/NRO_QUALITY_ROADMAP.md
GitHub issue #21
```

High-value items include:

```text
Diagnostics + diagnostic export
Applet/constrained-memory warning
privacy-safe error logs
real startup stages
READ ONLY / Vault / staged / active-location badges
Vault recovery + storage health
search/filter
Quick Jump
Favorites
recent items
text sizing
Reduced Motion
optional original sounds/rumble
virtualized large grids
bounded artwork caches
intentional missing-resource fallback
```

Recommended balance after the second device UI milestone:

```text
70% core functionality
20% hardware testing / regression fixes
10% polish / infrastructure
```

---

# Do not do prematurely

Do not begin these during the Session 2.5 packaging continuation:

```text
PKSM-Core
Master Vault
full Pokedex
full legality engine
true Move/live writes
all generations at once
another large UI redesign
```

Finish the exact replacement artifact and physically test it first.