# PokeBank NX Project Status

Last updated: **2026-09-28**

## Headline

PokeBank NX now has a device-accepted Gen I-III shared staged editor, a strict read-only Gen IV foundation, and a hardware-tested multi-provider Save Instances browser across Gen I-IV.

The provider-neutral Save Instances backend is now device accepted. The next major product tranche can move into Gen IV staged editor support while preserving the proven generation-specific parsers and the external-source immutability boundary.

## Current development line

PR #79 — **OPEN / DRAFT / NOT MERGED**

Branch:
**audit/full-project-hardening-20260923**

Current head:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Completed:
**Issue #85 — Source browser: unified multi-provider Save Instances for classic games**

PR #77 remains untouched, open and draft.

## What works today

- Gen I Red / Blue / Yellow read + staged Pokémon editing
- Gen II Gold / Silver / Crystal read + staged Pokémon editing
- Gen III Ruby / Sapphire / Emerald / FireRed / LeafGreen read + staged Pokémon editing
- staged classic Inventory editing where already supported
- joystick parity with D-pad and held-repeat navigation
- strict read-only Gen IV support for Diamond / Pearl / Platinum / HeartGold / SoulSilver
- Gen IV Trainer / Party / Boxes / Pokémon detail browsing
- multi-provider Save Instances with provider provenance
- RetroArch / configured mGBA / bounded Tico support for Gen I-III
- RetroArch / DraStic / melonDS / Manual support for Gen IV
- Platinum DraStic .dsv read-only opening on real hardware
- source deduplication and newest-first ordering
- explicit profile claims and unassigned-source browsing
- immutable external source policy
- durable PokeBank-owned transaction / recovery foundations

## Hardware acceptance

The currently accepted Save Instances runtime checkpoint remains:

**d49efd0c16433aaa1aa171501671a6e811c9da64**

That exact candidate passed physical testing on Switch.

The provider-neutral architecture at 00ee7a6e changes shared rendering, stale-source validation and alias/profile claim behavior. Its exact-head automated gates passed and the owner hardware test also passed, so this exact candidate is now DEVICE ACCEPTED.

## Current architecture direction

~~~text
GAME IDENTITY
    ↓
SAVE INSTANCES
    ↓
PROVIDER
    ↓
VALIDATED SAVE
    ↓
OPEN READ ONLY / authorized staged workspace
~~~

The UI should not care which generation-specific parser created a validated instance.

Shared metadata/presentation covers provider identity, paths, physical identity, timestamps, trainer/party summaries, fingerprints, validation state, claims, sorting, dedupe and source details.

Strict Gen I, II, III and IV parsing remains generation-specific.

## Current limitations

- cross-game True Move remains disabled;
- Gen IV editing/Create/Delete/source writeback remain disabled;
- Gen V has not started;
- Master Vault has not started;
- DraStic .dss savestates are unsupported;
- no Tico DS save root has been verified or added;
- mGBA arbitrary ROM-directory crawling is intentionally forbidden;
- live writes to installed games or external emulator sources remain disabled.

## Frozen hardware-test candidate

- Application SHA: **00ee7a6ed7ac1b5a93c43246d70c252e135acec0**
- Tree SHA: **b0832910df44898114a413191ebad3228bded8af**
- Actions artifact ID: **10956604914**
- NRO: **PokeBank-NX-PhysicalAudit-00ee7a6e.nro**
- NRO SHA-256: **5fad07002c5074ffb3d1d2bdd91275ef29fbdf199f7db263f39c5a3a9f86ca25**

## Current verification state

At the frozen 00ee7a6e application head:

- Host Tests #1385: **PASS**
- host clean build: **PASS**
- host full suite: **PASS**
- focused RSE bridge regression: **PASS**
- ASan / UBSan: **PASS**
- Audit Hardening Native Validation #226: **PASS**
- Packed Multi-Move #201: **PASS**
- Packed Move #202: **PASS**

**AUTOMATED GATES: PASS**

**DEVICE ACCEPTANCE: PASS**

## Near-term sequence

~~~text
Gen I-III shared editor                     DEVICE ACCEPTED
        ↓
Gen IV strict read-only foundation          IMPLEMENTED
        ↓
multi-provider Save Instances               DEVICE ACCEPTED
        ↓
provider-neutral Save Instance backend      DEVICE ACCEPTED
        ↓
Gen IV staged editor foundation             NEXT
        ↓
Gen IV editor parity / stabilization
~~~

Another generation, Gen V, Master Vault and cross-game True Move are outside the current tranche.

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
