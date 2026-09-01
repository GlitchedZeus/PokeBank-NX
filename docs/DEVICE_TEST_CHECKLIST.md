# PokeBank NX — Physical Switch Test Checklist

Status: PRE-ALPHA / READ-ONLY TEST PLAN  
Last updated: 2026-09-01

Use this checklist for the first and subsequent physical-device tests of PokeBank NX `.nro` builds.

The current safety posture is intentional: **live game-save writing is hard disabled**. Early device testing is for launch, navigation, source detection, read-only browsing, action behavior, Vault behavior when implemented, and crash/regression discovery.

---

## Before copying the build to the Switch

Record the exact build identity:

```text
Branch:
Commit SHA:
Build date:
.nro filename:
.nro size:
SHA-256:
Host tests: PASS / FAIL
Sanitizers: PASS / FAIL
Native build: PASS / FAIL
```

Do not test an unlabeled `.nro` if its source commit cannot be identified.

Recommended SD location:

```text
/switch/PokeBank-NX/PokeBank-NX.nro
```

The actual filename may temporarily still use the inherited/temporary target name until branding is cleaned up. Record what was tested rather than renaming evidence after the fact.

---

## Safety preparation

Before first hardware testing:

- keep independent backups of any important saves
- prefer copied/non-valuable test saves for any workflow that touches save-management screens
- do not enable or patch around the live-write lock
- do not test a feature that claims to write a live title unless the project status explicitly says the safe-write pipeline has been enabled for testing
- keep enough free SD-card space for logs/backups

---

## Test report template

For every failure, capture:

```text
Build commit:
Switch firmware:
Atmosphère/CFW version:
Launch method:
Docked or handheld:
Game/source involved:
Exact buttons/actions:
Expected result:
Actual result:
Crash code/error text:
Screenshot/photo available: yes/no
Reproducible: always/sometimes/once
```

If a crash occurs, stop repeating destructive-looking actions until the cause is understood.

---

# Test A — Application launch

- [ ] `.nro` appears in hbmenu
- [ ] application launches
- [ ] no immediate crash
- [ ] header/logo renders
- [ ] version string renders
- [ ] abbreviated Git commit renders
- [ ] fonts/icons render correctly
- [ ] no obvious missing-resource error
- [ ] app remains responsive for at least 60 seconds idle

Result:

```text
PASS / FAIL
Notes:
```

---

# Test B — Controller navigation

Test both D-pad and analog stick where supported.

- [ ] move focus up/down/left/right
- [ ] A selects focused item
- [ ] B returns/back behavior is consistent
- [ ] L/R changes expected tab/box where available
- [ ] ZL/ZR shortcuts do not crash
- [ ] X/Y context actions do not trigger unintended mutation
- [ ] holding/repeating directional input does not skip unpredictably
- [ ] focus never disappears off-screen
- [ ] no stuck input after opening/closing dialogs

Result:

```text
PASS / FAIL
Notes:
```

---

# Test C — Installed game detection

For every installed supported title available on the test Switch, record:

```text
Game:
Detected: yes/no
Displayed title:
Displayed platform:
Expected stable game ID if known:
Wrong duplicate card: yes/no
```

Specific regression checks:

- [ ] FireRed — Nintendo Switch does not present itself as `firered_gba`
- [ ] LeafGreen — Nintendo Switch does not present itself as `leafgreen_gba`
- [ ] FireRed and LeafGreen remain distinct
- [ ] platform label is shown separately from game title
- [ ] unsupported titles are not falsely identified as supported Pokémon games

---

# Test D — Read-only game browsing

For each available supported native title:

- [ ] source opens
- [ ] party count looks plausible
- [ ] party Pokémon render
- [ ] box browser opens
- [ ] box count/navigation looks plausible
- [ ] empty slots behave correctly
- [ ] Pokémon species display correctly for several samples
- [ ] switching boxes repeatedly does not crash
- [ ] returning to Home/source list does not corrupt UI state

Do **not** judge save-writing functionality in this phase. This test is read-only.

---

# Test E — Live-write lock regression

This section is critical.

Expected state: **there must be no path that writes directly to an installed game's live save.**

Verify:

- [ ] no visible destination named as a live installed title in backup/restore flows
- [ ] only allowed backup/session destinations are shown where applicable
- [ ] no legacy `injectToGame` setting can be enabled through the UI
- [ ] restarting the app does not restore a legacy live-write option
- [ ] selecting normal browse/actions never writes a game save
- [ ] opening/closing Pokémon details never writes a game save
- [ ] failed/unsupported actions do not write a game save

If any visible action appears capable of live injection, mark the build **FAIL — SAFETY REGRESSION** and stop testing that path.

---

# Test F — Pokémon A-button action sheet

Run after the action-sheet milestone exists in the tested build.

Highlight a Pokémon and press A.

Expected menu:

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

During early implementation, unavailable actions may be visibly disabled or show an explicit not-yet-supported message.

Verify:

- [ ] pressing A opens a deliberate menu
- [ ] A does not immediately edit/move/delete/clone/transfer
- [ ] Cancel returns without mutation
- [ ] B returns without mutation
- [ ] menu focus is controller-friendly
- [ ] unsupported actions fail safely
- [ ] repeated open/close does not leak/crash visibly

---

# Test G — Pokémon summary

When available, inspect Pokémon from multiple games/generations.

- [ ] species
- [ ] National Dex number
- [ ] nickname
- [ ] level
- [ ] gender where applicable
- [ ] shiny state
- [ ] form
- [ ] type(s)
- [ ] nature where applicable
- [ ] ability where applicable
- [ ] held item where applicable
- [ ] moves
- [ ] IVs/EVs where applicable
- [ ] OT
- [ ] TID/SID where applicable
- [ ] origin game/platform
- [ ] current location
- [ ] met/Ball/language where applicable
- [ ] legality state
- [ ] provenance

Generation-awareness regression:

- [ ] old-generation records do not invent fields that did not exist
- [ ] unsupported data is shown as unavailable/unknown rather than fabricated

---

# Test H — Master Vault read-only insertion

Run only after Vault v1 is implemented.

Use a non-critical test Pokémon.

- [ ] `Add to Master Vault` succeeds
- [ ] source Pokémon remains unchanged
- [ ] Vault entity appears
- [ ] app restart preserves the entity
- [ ] species/metadata still parse correctly
- [ ] stored SHA-256 exists
- [ ] source game/platform is correct
- [ ] source box/slot is correct where available
- [ ] adding to a Bank does not remove from Vault
- [ ] same Vault entity can be referenced by multiple Banks
- [ ] clone creates a new Vault ID
- [ ] clone parent relationship is visible/recorded

---

# Test I — Storage/error behavior

Where practical using a test SD environment:

- [ ] missing optional folders are created or handled gracefully
- [ ] corrupted optional metadata gives a clear error instead of crash
- [ ] unavailable source save gives a clear error
- [ ] insufficient free-space condition is detected before partial persistent writes
- [ ] app does not silently erase unknown files

Do not intentionally corrupt real game saves for this test.

---

# Test J — Docked/handheld UI

- [ ] handheld text readable
- [ ] docked text readable
- [ ] no clipped buttons
- [ ] no important text behind overscan/safe-area boundaries
- [ ] long game/Pokémon names do not destroy layout
- [ ] dialog/action-sheet sizing works in both modes

---

# Promotion rules

A coding agent may mark a feature:

```text
IMPLEMENTED
```

when code exists.

It may mark:

```text
HOST TESTED
```

only after the relevant host tests pass.

It may mark:

```text
NRO BUILDS
```

only after the native Switch build succeeds.

It may mark:

```text
DEVICE TESTED
```

**only after a human physically runs that exact build/commit on a Switch and reports the result.**

Device testing of commit A does not automatically validate a materially different commit B.

---

# First-device-test acceptance target

The first recovery-era device test is successful if:

```text
app boots
+
controller navigation works
+
supported installed titles are identified correctly
+
party/box read-only browsing works for at least one real title
+
no live-write path is exposed
+
no crash occurs during the basic path
```

That is enough to promote the tested baseline to an initial hardware-verified foundation. It is **not** approval to enable live writes.
