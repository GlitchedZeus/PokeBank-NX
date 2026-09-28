# PokeBank NX Project Status

Last updated: **2026-09-28**

## Headline

PokeBank NX has a device-accepted Gen I-III shared staged Pokémon editor, a device-accepted Gen I-IV provider-neutral Save Instances/source-browser foundation, and an active Gen IV boxed Pokémon View/Edit milestone.

The current product goal is not another new editor shell. Generation IV is being plugged into the same shared editor already proven across Gen I-III.

## Active development

PR #87 — **OPEN / DRAFT / NOT MERGED**

Branch:
**feature/gen4-shared-pokemon-editor-20260928**

Current head:
**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

Tracking:
**Issue #86 — G4-03 Gen IV shared staged Pokémon editor**

Stacked on accepted PR #79 / branch:
**audit/full-project-hardening-20260923**

Accepted base head:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

## What works today

- Gen I Red / Blue / Yellow read + staged Pokémon editing
- Gen II Gold / Silver / Crystal read + staged Pokémon editing
- Gen III Ruby / Sapphire / Emerald / FireRed / LeafGreen read + staged Pokémon editing
- staged classic Inventory editing where supported
- joystick/D-pad parity and held-repeat navigation
- strict Gen IV Trainer / Party / Boxes / Pokémon-detail browsing
- multi-provider Save Instances across Gen I-IV
- RetroArch / configured mGBA / bounded Tico support for Gen I-III
- RetroArch / DraStic / melonDS / Manual support for Gen IV
- Platinum DraStic .dsv read-only opening on real hardware
- provider provenance, source dedupe, newest-first ordering and profile claims
- immutable external source policy
- durable PokeBank-owned transaction/recovery foundations

## Gen IV editor state

The current draft adds **boxed Pokémon View/Edit only**.

It reuses:

- the shared three-panel editor;
- existing themes and modal style;
- shared numpad/keyboard/pickers;
- inline Gender toggle;
- direct Shiny toggle;
- shared move editor;
- joystick/D-pad behavior;
- dirty-edit confirmation.

Gen IV-specific backend work adds:

- PK4 mutable serialization;
- exact DP/Pt/HGSS capabilities;
- constrained PID-linked edits;
- Storage CRC refresh;
- strict staged-save reparse;
- rollback on failed validation;
- unrelated-byte preservation.

Create, Party mutation, Inventory editing and source writeback remain disabled.

## Hardware acceptance

Accepted provider-neutral source-browser checkpoint:

**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Status:
**DEVICE ACCEPTED**

Current Gen IV editor head:

**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

Status:
**EXACT ACTIONS NRO BUILT / HARDWARE TEST PENDING / NOT DEVICE ACCEPTED**

Confirmed green on the current Gen IV head:

- focused Gen IV host tests;
- focused Gen IV sanitizers;
- Gen IV shared UI contract;
- real devkitA64 shared UI compile;
- Packed Move;
- Packed Multi-Move.

Gen IV candidate workflow #11 is fully green, including full devkitA64 compile/link and packaging.

Exact candidate:

- Application SHA: **5e79e9f8e038f2f940070edd92a70b692df7a7b5**
- Tree SHA: **c636127b2e52792371e8e55abac8fe1d5cfaf99f**
- Artifact ID: **10960797638**
- NRO SHA-256: **8011ef64d269ae42331306c13692d60494fb7ea936619d61e1abc7d7906829d5**

Broad Host Tests #1427 are now **PASS**.

Automated state:

**ALL REQUIRED AUTOMATED GATES: PASS**

Still pending:

- owner physical Switch test of the exact candidate NRO.

## Current limitations

- cross-game True Move remains disabled;
- Gen IV Create remains disabled;
- Gen IV Party mutation remains disabled;
- Gen IV Inventory editing remains disabled;
- Gen IV source writeback remains disabled;
- Gen V has not started;
- Master Vault has not started;
- DraStic .dss savestates are unsupported;
- no Tico DS save root has been verified;
- mGBA arbitrary ROM-directory crawling remains forbidden;
- live writes to installed games or emulator sources remain disabled.

## Near-term sequence

~~~text
Gen I-III shared editor                     DEVICE ACCEPTED
        ↓
Gen IV strict read-only foundation          IMPLEMENTED
        ↓
multi-provider Save Instances               DEVICE ACCEPTED
        ↓
provider-neutral source backend             DEVICE ACCEPTED
        ↓
Gen IV boxed shared View/Edit               ACTIVE / CI CANDIDATE
        ↓
owner Switch hardware acceptance
        ↓
Gen IV editor stabilization
        ↓
Gen IV Create only after Edit is proven
~~~

## Small parallel polish work

Small, low-risk work can proceed beside G4-03 when it does not invalidate the active hardware candidate. Good candidates are tracked in issues #21, #26, #70 and #16, including:

- remove stale/redundant control-help text;
- clean game-card visual metadata;
- improve shared bottom-control legend/readability;
- diagnostics/version/build-SHA presentation;
- missing-art fallbacks;
- final PokeBank NX branding/NRO metadata cleanup;
- normalize non-destructive controller semantics where no save mutation is involved.

Large new storage, conversion, generation or writeback work should not be mixed into the Gen IV editor candidate.

## Permanent project rules

~~~text
ORIGINAL EXTERNAL SOURCE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: DISABLED
LIVE EMULATOR WRITEBACK: DISABLED
UNKNOWN / AMBIGUOUS SOURCE: FAIL CLOSED
REMEMBERED SOURCE SUBSTITUTION: FORBIDDEN
A BUTTON: NON-DESTRUCTIVE
CROSS-GAME TRUE MOVE: LOCKED
~~~
