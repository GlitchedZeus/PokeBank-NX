# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-28**

GitHub is authoritative. Re-fetch live heads before new work and preserve anything newer than the checkpoints below.

## Active main-development line

### PR #92 — G4-04 full Gen IV shared editor + Create

State:
**OPEN / DRAFT / NOT MERGED**

Branch:
**feature/gen4-full-editor-20260928**

Base:
**integration/gen4-polish-hardware-20260928**

Current exact head at this documentation update:
**d72e1f0fbb3c0be0d24c42ebb73b44f5c6984ee8**

Canonical tracking:
**Issue #95 — G4-04 Complete Gen IV shared editor: Create + remaining fields**

Current exact-head workflow state:

- Gen I/II Packed Multi-Move #289 — **PASS**
- PokeBank NX Host Tests #1583 — **IN PROGRESS**
- Gen IV Shared Editor Candidate Gate #94 — **IN PROGRESS**

Re-fetch before claiming full automated acceptance.

## Accepted Gen IV G4-03 checkpoint

The owner physically accepted:

- Application SHA: **84dae170deb2756d9b80aec32bf8ad512ce17c31**
- Tree SHA: **7733004cd7b4aebffc3ba687ad0f7df22eea8aa8**
- Artifact ID: **10986964856**
- NRO SHA-256: **313b6ed5f209b0fba797deee010d73b753d25294dd3d1c39f279c61df13fe6de**

Accepted scope:

- Gen IV Party + Box staged View/Edit;
- shared editor routing on real Platinum / DraStic;
- full 0xEC Party PK4 staged mutation;
- boxed PK4 staged mutation;
- General + Storage CRC refresh;
- strict full-save reparse / exact target verification;
- rollback on failed validation;
- Party stat refresh;
- dirty-session exit protection;
- independent-audit remediation;
- source unchanged during ordinary editing.

Status:
**DEVICE ACCEPTED FOR G4-03 SCOPE**

## Integrated baseline — PR #90

PR #90 remains:

**OPEN / DRAFT / NOT MERGED**

Branch:
**integration/gen4-polish-hardware-20260928**

Current head:
**8b3bcc16c804247bfe8d1314b686974ce73051d8**

This line contains:

- the accepted G4-03 implementation;
- all independent audit fixes/regressions;
- the complete final v1 polish tranche from PR #88 through
  **c8c98d5dd2816fe6bf1b9557c24b3fb09a5dc7c3**.

Exact-head PR #90 workflows are green:

- Host Tests #1537 — PASS
- Gen IV Candidate Gate #51 — PASS
- v1 Polish Native #35 — PASS
- Packed Move #250 — PASS
- Packed Multi-Move #252 — PASS

Hardware acceptance belongs specifically to the exact tested **84dae170...** NRO. The later **8b3bcc16...** integration head adds final QoL/UI work and is the base for G4-04.

## Historical / completed lanes

PR #87:
**CLOSED WITHOUT MERGE**
Historical G4-03 implementation branch. Work carried forward into #90.

PR #88:
**CLOSED WITHOUT MERGE**
Completed v1 polish branch. Final head:
**c8c98d5dd2816fe6bf1b9557c24b3fb09a5dc7c3**
Work carried forward into #90.

PR #79:
**OPEN / DRAFT / NOT MERGED**
Accepted audit/source-architecture base:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

## Current game/editor state

| Area | Current state |
|---|---|
| Gen I R/B/Y | Device-accepted staged Create/View/Edit |
| Gen II G/S/C | Device-accepted staged Create/View/Edit |
| Gen III R/S/E/FRLG | Device-accepted staged Create/View/Edit |
| Gen IV D/P/Pt/HG/SS | G4-03 Party/Box View/Edit device accepted; G4-04 full editor active |
| Classic Inventory | Device accepted where supported |
| Save Instances | Device accepted for Gen I–IV |
| Gen IV Create | Implemented on active G4-04 branch; hardware acceptance pending |
| Gen IV remaining field parity | Active G4-04 work |
| Emulator Inject Save | Planned in Issue #89; disabled |
| Cross-game True Move | Disabled |
| Master Vault | Planned / not started |
| Gen V | Not started |

## G4-04 implemented surface

PR #92 currently includes work for:

- Box Create transaction;
- native stored PK4 Create drafts;
- DP/Pt/HGSS Create defaults;
- shared Create/Draft flow;
- inspectable read-only detail rows;
- exact Held Item picker;
- exact PK4 Language picker;
- exact Ball picker;
- Pokérus picker;
- real Gen IV Met Location catalog/setter;
- native Gen IV move picker;
- transactional Species editing;
- Species dependent-state reconciliation;
- native move IDs 1–467 with Gen V IDs rejected;
- strict staged mutation/reparse/rollback inherited from G4-03.

Remaining G4-04 work includes:

- exact persistent Form editing where applicable;
- verify move change refreshes PP to native maximum;
- finish exact-head CI;
- build one combined Actions NRO;
- owner physical hardware acceptance for Create + full field parity.

## Save-session backup / injection direction

Issue #89 owns the future writeback model:

~~~text
current emulator save
        ↓
automatic immutable backup
        ↓
PokeBank working copy
        ↓
edit + validate
        ↓
explicit Inject Save
        ↓
validated source replacement
while backup remains available
~~~

Current builds do not inject into emulator or installed-game saves.

## Modern Switch game support

Issue #11 tracks modern Nintendo Switch save adapters.

Current policy is:

**read-only, source-specific validation first**

before staged mutation or source-write work.

Tracked families include:

- LGPE
- Sword / Shield
- BDSP
- Legends: Arceus
- Scarlet / Violet
- Legends Z-A
- Switch FireRed / LeafGreen

References include pkHouse, PKHeX and existing PKSE behavior, but product support still requires PokeBank NX-specific validation, malformed-input rejection and source policy.

## Permanent safety rules

- ordinary editing targets app-owned staged/working data;
- emulator and installed-game injection remain disabled in current builds;
- unknown / ambiguous saves fail closed;
- remembered-source substitution is forbidden;
- dirty staged work cannot be silently discarded;
- A-button browsing actions remain non-destructive;
- cross-game True Move stays locked until route-specific approval;
- hardware acceptance belongs only to the exact NRO physically tested.

## Next main gate

Continue G4-04 on PR #92 / Issue #95.

Before hardware acceptance:

1. finish Form/move-PP details;
2. pass exact-head Host + sanitizer gates;
3. pass Gen IV native candidate gates;
4. preserve Gen I–III regressions;
5. produce one exact Actions NRO;
6. owner physically tests Create + full Gen IV field parity.
