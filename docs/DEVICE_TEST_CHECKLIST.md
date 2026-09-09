# PokeBank NX — Physical Switch Test Checklist

## Corrected FRLG physical-test artifact — READY FOR DEVICE TEST

```text
Starting recovery/documentation SHA: c7e410d4d168d1173c9bd9e3ceb38489c865d404
Canonical application source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
Canonical application tree: ed5912093886384894c44538d569fe4955fd2e47
Application commit: legacy: bind FRLG sources to profiles and expose diagnostics
Application source changed during packaging: NO
Embedded version/source: 0.1.0-alpha / ea0b806b

Artifact: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
Artifact byte size: 156592377
Artifact SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
ZIP: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.zip
ZIP byte size: 149672676
ZIP SHA-256: 255ec20cd0f7965c9b25123cb83eedd0cebc83d10fd88313bcff672a0c1ced67
Manifest: PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro.manifest.txt
Manifest byte size: 900
Manifest SHA-256: a79ede73124f74ce4a6c85d6c151bf7f3e9ca732747a332caa68750626058952
SHA256SUMS: SHA256SUMS.txt
SHA256SUMS byte size: 352
SHA256SUMS SHA-256: d28ee7266c58b3cab2716211a0425b0af6562dab3f9d6768ac299b77d8342f55

HD renders: 3260 / 3260
Base species: 1025 / 1025
Type icons: 18 / 18
Fonts: 3 / 3
FRLG GBA cards: PASS
Asset preflight: PASS
Embedded RomFS: PASS — 3283 / 3283 files byte-identical, 148076683 bytes
Native devkitA64 -fno-exceptions build: PASS
Previously verified host tests: PASS — 13 suites
Previously verified ASan/UBSan: PASS
Previously verified git diff --check: PASS
GitHub CI for application source: PASS — run #245
GitHub prerelease published: NO — unavailable authentication/tooling
DEVICE TESTED: NO
DEVICE ACCEPTED: NO
```

The exact NRO, ZIP, manifest and checksum record were preserved outside the temporary build tree. Large artifacts were not committed to Git history. Parked RSE checkpoint `b5ef83b` remains isolated and untouched; `1a921515` was unavailable locally, with `b5ef83b` retained as the verified equivalent recovery state.



## Complete FRLG browser test build — 2026-09-08

```text
Application source: 5d3e5e23f352dda4900ae42b4f396d9d4a4b8b8e
Commit: gen3: complete FRLG read-only source browsing
Embedded: 0.1.0-alpha (5d3e5e23)
Artifact: PokeBank-NX-FRLG-Complete-5d3e5e23.nro
Size: 156249001 bytes
SHA-256: 8dd94277e609c96f37e82b0b0b47928bad50ce36fdb380520bd2521ab18ec78a
Host tests: PASS (12 suites)
Sanitizers: PASS
Native build: PASS (-fno-exceptions)
Assets: PASS (3260 HD renders; embedded RomFS 3283/3283 identical)
Device tested: NO
```

Test and leave PASS/FAIL notes for: launch; active RetroArch root; FireRed/LeafGreen parent cards; child save grouping; X Refresh Saves; changed/deleted-save handling; trainer name/gender/TID/SID/ID32/money; Items/Key Items/Poke Balls/TM Case/Berry Pouch/PC Items; Party? no, source is read-only; Party; all 14 Boxes; artwork; crashes. Confirm no source file timestamp/hash changes.

Status: PRE-ALPHA / READ-ONLY TEST PLAN  
Last updated: 2026-09-08

Use this checklist for physical-device testing of PokeBank NX `.nro` builds.

Current safety posture:

```text
LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED
```

Early hardware tests validate launch, rendering, controller behavior, source detection, read-only browsing, Action Sheet safety, themes, and device-only regressions.

---

## Exact-build rule

Before copying a build to the Switch, record:

```text
Application source branch:
Application source full SHA:
Application source commit message:
Build date:
.nro filename:
.nro size:
SHA-256:
Host tests: PASS / FAIL
Sanitizers: PASS / FAIL
git diff --check: PASS / FAIL
Native build: PASS / FAIL
```

Do not test an unidentified `.nro` and later guess which commit it came from.

If a later documentation/status commit exists, record it separately from the application source SHA.

Recommended SD location:

```text
/switch/PokeBank-NX/PokeBank-NX.nro
```

The actual target filename may still contain inherited/temporary branding. Record the file actually tested.

---

## Current known test candidates

### Action Sheet build

```text
PokeBank-NX-ActionSheet-82a0779.nro
Source: 82a0779a5143cca0690d0c7068946d84ebe9f107
Size: 9,695,669 bytes
SHA-256: 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
Status: NOT DEVICE TESTED
```

The current preferred test plan is to finish issue #13 first and test the newer combined Action Sheet + HOME-style controls/theme build.

If issue #13 cannot be recovered/completed promptly, the Action Sheet build remains a valid fallback first hardware test.

---

## Safety preparation

- keep independent backups of important saves;
- prefer non-critical/copy test saves for development workflows;
- do not patch around the live-write lock;
- ensure adequate SD-card free space;
- if any UI appears to offer direct live-save injection, stop that path and record a **SAFETY REGRESSION**.

---

## Device report header

```text
Application source SHA:
.nro filename:
.nro SHA-256:
Switch firmware:
Atmosphère/CFW version:
Launch method:
Handheld / Docked:
Test date/time:
```

For every failure:

```text
Screen/context:
Game/source:
Exact buttons/actions:
Expected:
Actual:
Crash/error code:
Screenshot/photo: yes/no
Reproducible: always/sometimes/once
```

---

# Test A — Launch / idle stability

- [ ] `.nro` appears in hbmenu
- [ ] application launches
- [ ] no immediate crash
- [ ] logo/header renders
- [ ] version renders
- [ ] abbreviated Git commit renders
- [ ] fonts/icons/assets render
- [ ] no obvious missing-resource failure
- [ ] app remains responsive for 60 seconds idle
- [ ] returning from one or more screens does not corrupt the shell

Result:

```text
PASS / FAIL
Notes:
```

---

# Test B — Core controller model

Test both D-pad and Left Stick.

### D-pad / Left Stick

- [ ] Up works
- [ ] Down works
- [ ] Left works
- [ ] Right works
- [ ] Left Stick reaches the same logical focus targets as D-pad
- [ ] held stick navigation repeats predictably if implemented
- [ ] held D-pad repeat is predictable if implemented
- [ ] repeat does not skip wildly or get stuck
- [ ] focus never disappears off-screen

### A / B

- [ ] A selects/opens the focused safe action
- [ ] B consistently backs/cancels
- [ ] B never performs mutation
- [ ] closing dialogs does not leave stuck input

### L / R

- [ ] previous/next box/tab/Pokémon behavior matches the screen context
- [ ] L/R does not unexpectedly activate an unrelated action

### ZL / ZR

- [ ] larger jumps/major navigation work where advertised
- [ ] unused contexts safely do nothing or omit the hint
- [ ] no crash from repeated ZL/ZR

### X / Y

- [ ] X behavior matches visible Filter/Search/context hint where wired
- [ ] Y behavior matches visible Sort/View/secondary hint where wired
- [ ] unused X/Y are not falsely shown in the hint bar
- [ ] X/Y do not trigger unintended mutation

### `+`

Expected contract:

```text
contextual More / Options
NOT global Exit
```

Verify:

- [ ] `+` does not unexpectedly exit the application
- [ ] `+` opens Options/More/Settings where advertised
- [ ] Pokémon Summary eventually reserves/prefers Compatible Games / Transfer Compatibility
- [ ] pressing `+` in an unsupported context is safe

### `-`

Expected contract:

```text
Help / Controls / Screen Info
read-only
```

Verify:

- [ ] `-` opens Help/Controls where advertised
- [ ] Help content is readable
- [ ] B closes Help
- [ ] Help performs no mutation

### Right Stick

- [ ] any implemented fast-scroll/secondary-pane behavior works
- [ ] core navigation remains usable without Right Stick

Result:

```text
PASS / FAIL
Notes:
```

---

# Test C — Bottom controller hint bar

On several screens:

- [ ] displayed hints match real behavior
- [ ] inactive buttons are not shown as fake controls
- [ ] A/B labels are correct
- [ ] `+`/`-` labels match context
- [ ] hints do not cover important content
- [ ] hint text remains readable in handheld mode
- [ ] hint bar theme matches the active theme

Result:

```text
PASS / FAIL
Notes:
```

---

# Test D — Themes / persistence

Run only when the tested build includes issue #13 theme work.

Required themes:

```text
OLED Black
Dark
Light
```

### OLED Black

- [ ] background appears true/near black
- [ ] raised surfaces remain distinguishable
- [ ] text has adequate contrast
- [ ] focus is obvious
- [ ] dialogs/hints remain readable

### Dark

- [ ] dark charcoal/navy surfaces are readable
- [ ] secondary text does not disappear
- [ ] selected/focused card is obvious

### Light

- [ ] light theme is not a broken inverted Dark palette
- [ ] primary/secondary text remains readable
- [ ] selected/focused card is obvious
- [ ] no washed-out warnings/errors

### Persistence

- [ ] select each theme successfully
- [ ] leave Settings/Options and continue browsing
- [ ] restart the app
- [ ] selected theme persists
- [ ] invalid/corrupt preference does not crash the app if testable safely

Result:

```text
PASS / FAIL
Notes:
```

---

# Test E — Select Game / source cards

- [ ] cards render correctly
- [ ] focused card is unmistakable
- [ ] focus movement is predictable
- [ ] game title is readable
- [ ] platform displays separately
- [ ] source/status text is readable
- [ ] missing artwork uses a safe fallback if encountered
- [ ] rapid navigation does not visibly lock/stutter excessively

For installed supported titles available on the test Switch, record:

```text
Game:
Detected: yes/no
Displayed title:
Displayed platform:
Expected stable game ID if known:
Duplicate/wrong card: yes/no
```

Specific regression checks:

- [ ] FireRed — Nintendo Switch is not treated as `firered_gba`
- [ ] LeafGreen — Nintendo Switch is not treated as `leafgreen_gba`
- [ ] FireRed and LeafGreen remain distinct
- [ ] unsupported titles are not falsely identified as supported Pokémon games

Result:

```text
PASS / FAIL
Notes:
```

---

# Test F — Read-only Party / Boxes browsing

Use at least one supported native title if available.

- [ ] source opens
- [ ] Party count looks plausible
- [ ] Party Pokémon render
- [ ] Boxes open
- [ ] box count/navigation looks plausible
- [ ] empty slots behave safely
- [ ] several sample species render correctly
- [ ] repeated L/R box switching is stable
- [ ] returning to Select Game preserves sane UI state
- [ ] focusing a Pokémon alone does not mutate it

This test is read-only.

Result:

```text
PASS / FAIL
Notes:
```

---

# Test G — Pokémon Action Sheet

Expected menu order:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

Run from every available path:

```text
Party
Boxes
Storage (if exposed in the tested flow)
```

Verify:

- [ ] A on occupied Pokémon opens Action Sheet
- [ ] A does not immediately edit/move/delete/clone/transfer
- [ ] menu order is correct
- [ ] focus highlight is obvious
- [ ] Up/Down navigation works
- [ ] B closes with zero mutation
- [ ] Cancel closes with zero mutation
- [ ] View opens a read-only details/summary path
- [ ] unfinished actions clearly say `Not yet supported` or are visibly disabled
- [ ] unfinished action cannot fall through into another action
- [ ] repeated open/close is stable
- [ ] switching themes does not break the Action Sheet if issue #13 is present

Result:

```text
PASS / FAIL
Notes:
```

---

# Test H — Live-write hard lock

This is a release-blocking safety test.

Expected state:

```text
NO DIRECT LIVE INSTALLED-GAME SAVE WRITING
```

Verify:

- [ ] no visible normal destination writes directly into an installed game's live save
- [ ] backup/session destinations remain the only current write destinations
- [ ] no `injectToGame` UI setting can be enabled
- [ ] restarting does not restore a legacy live-write option
- [ ] browsing performs no save write
- [ ] opening/closing Action Sheet performs no save write
- [ ] View performs no save write
- [ ] Help/Options/theme switching performs no save write
- [ ] unsupported Action Sheet actions perform no save write
- [ ] controller shortcuts do not reveal a hidden injection path

If a direct live-write path appears, record:

```text
FAIL — SAFETY REGRESSION
```

and stop testing that path.

---

# Test I — Summary shell

The current inherited Summary may not yet satisfy the final professional PokeBank NX design.

For current builds check only what is actually implemented:

- [ ] read-only Summary opens
- [ ] species/nickname/level basics look plausible
- [ ] B returns safely
- [ ] theme does not make text unreadable
- [ ] `+` behavior matches advertised context if compatibility view is implemented

When issue #9 is implemented, expand this test to full generation-aware stats/origin/legality/provenance coverage.

---

# Test J — Error / recovery behavior

Where safely reproducible:

- [ ] missing optional resource/source produces useful error, not crash
- [ ] unsupported action explains itself
- [ ] error states say whether data was modified when safety-sensitive
- [ ] returning from error restores usable navigation
- [ ] no unknown files are silently erased

Do not intentionally corrupt real game saves for testing.

---

# Test K — Handheld / docked readability

If both modes are available:

- [ ] handheld text readable
- [ ] docked text readable
- [ ] focus obvious at normal viewing distance
- [ ] no important content clipped by safe-area/overscan
- [ ] long names do not destroy layout
- [ ] Action Sheet sizing works
- [ ] bottom hints remain legible
- [ ] OLED/Dark/Light all remain usable

---

# Test report summary

```text
Application source SHA:
.nro SHA-256:

Launch: PASS / FAIL
Core controls: PASS / FAIL
Hint bar: PASS / FAIL / N/A
Themes: PASS / FAIL / N/A
Select Game: PASS / FAIL
Party/Boxes: PASS / FAIL
Action Sheet: PASS / FAIL
Live-write lock: PASS / FAIL
Summary: PASS / FAIL / PARTIAL
Handheld readability: PASS / FAIL
Docked readability: PASS / FAIL / NOT TESTED

Crashes:
Safety regressions:
UI/input issues:
Other notes:
```

---

# Promotion rules

A coding agent may mark:

```text
IMPLEMENTED
```

when code exists.

It may mark:

```text
HOST TESTED
```

only after relevant host verification passes.

It may mark:

```text
NRO BUILDS
```

only after the native Switch build succeeds.

It may mark:

```text
DEVICE TESTED
```

**only after a human physically runs the exact recorded binary on a Switch and reports the result.**

Device testing one commit does not validate a materially different commit.

A physical test may promote specific capabilities without declaring the entire application device-tested.

Example:

```text
Action Sheet: DEVICE TESTED
OLED Black theme: DEVICE TESTED
Legends Z-A save adapter: NOT DEVICE TESTED
```

That level of specificity is preferred.