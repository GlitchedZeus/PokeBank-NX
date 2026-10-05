# PokeBank NX Project Status

Last updated: **2026-10-05**

For exact active heads and CI evidence, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Repository: `GlitchedZeus/PokeBank-NX`
- Platform: Nintendo Switch homebrew
- State: active alpha
- Active application / hardware lane: **PR #92**
- Active read-only legality lane: **PR #103**
- Active legality child validation: **PR #120**
- Full forensic remediation: **completed and merged into PR #92 via PR #101**

GitHub is authoritative. Checkpoint SHAs are evidence, never instructions to move a branch backward.

## Headline

PokeBank NX now has:

- a hardware-accepted Gen I–III read/editor foundation;
- a substantially complete staged Gen IV editor for D/P/Pt/HG/SS;
- a modern Product Home and Games/source-management frontend;
- established host/native/editor validation lanes with the latest application/input integration being revalidated as the active branch advances;
- a completed full-repository forensic audit and remediation package integrated into the active app lane;
- an actively growing, read-only Gen I–IV legality evidence engine in a separate lane.

The project is currently in **final integrated UI / input / launch / hardware-polish territory before full touch controls**.

## Current application state

PR #92 is the current Gen I–IV application lane.

Current implementation includes:

- Product Home selected-game presentation;
- full Games browser and Quick Games flow;
- game/save/profile source assignment;
- source/provider-aware save identity;
- region-aware hero artwork and trainer presentation;
- larger handheld Party presentation;
- real per-save Gen I–IV Pokédex progress;
- sorting / Release Date ordering foundation;
- Favorites foundation;
- Items/Settings/Search navigation work;
- exact-release launch matching and provider/installed-forwarder routing for supported GBA/DS flows;
- hotpath work that avoids unnecessary synchronous source/launch scans during Quick Games and normal legacy opens;
- full staged Gen IV G4-04 editor implementation;
- active shared on-screen-keyboard integration work based on the merged PKSE keyboard contribution.

The active branch is moving while that final integration is validated. Recent runtime checkpoints have passed the normal native/Product UI/Gen IV/focused regression gates, while the latest exact Host/sanitizer cycles must be allowed to finish before a new combined head is called fully green. `CURRENT_STATUS.md` records the current checkpoint/run boundary.

The current combined application is **not hardware accepted yet**.

## Generation status

### Gen I — ✅ device accepted

Red / Blue / Yellow read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen II — ✅ device accepted

Gold / Silver / Crystal read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen III — ✅ device accepted

Ruby / Sapphire / Emerald / FireRed / LeafGreen read, inventory and shared Pokémon editor workflows are hardware accepted.

### Gen IV — 🧪 implemented / hardware pending

Diamond / Pearl / Platinum / HeartGold / SoulSilver have the staged full-editor foundation.

The earlier safe Party/Box View/Edit milestone is physically accepted. Current G4-04 adds Create, native field parity, species/form reconciliation, move handling, action parity, strict checksum/reparse/rollback behavior, and the current Product UI/source architecture.

The full G4-04 + current frontend/input integration requires its own exact physical acceptance pass.

### Gen V+ — ⏳ not a production workflow yet

Gen V is not started as a product editor. Later DS/3DS/modern Switch work remains provider, identity, validation, or research groundwork until explicitly advanced.

## Audit / hardening

The repository-wide forensic audit is complete and frozen.

PR #101 remediation was merged into the active PR #92 branch on 2026-10-05.

Final remediation disposition:

- **43 VERIFIED / FIXED**
- **1 DEFERRED WITH JUSTIFICATION — AUDIT-043**
- **0 OPEN**

The audit/remediation work strengthens parser boundaries, memory safety, durability, backup/custody behavior, source-mutation policy, generated-data checks, native build coverage, and regression confidence without enabling unsafe source writes.

AUDIT-043 remains intentionally deferred until a real failing disposable Gen IV save is available; the app must not manufacture trainer identity from filenames.

The older isolated hardening PRs #98 and #99 are now closed as superseded because their AUDIT-013/AUDIT-012 fixes are already verified and integrated through the completed remediation lane. The old docs refresh PR #102 is also closed as superseded by the current documentation state; its branch/history remains available.

## Legality engine

PR #103 is the separate **read-only Gen I–IV legality analysis lane**.

Current implemented foundation includes:

- exact source-game profiles and generation ceilings;
- exact-game move compatibility;
- Gen I/II encounter/trade/history evidence;
- extensive Gen III event/PID/IV/RNG evidence;
- Colosseum/XD/e-Reader/GameCube-specific evidence;
- Gen III → IV Pal Park evidence;
- extensive Gen IV wild/static/trade/PokéWalker/WC4/PCD evidence;
- state-aware Gen IV egg/static/gift provenance;
- coverage-aware verdict semantics.

PR #120 is the current isolated child validation for Gen IV Method J/K lead predicates. It remains evidence-only and is not a MAIN/application merge.

The legality lane is analysis only. Missing evidence stays **Incomplete**; it does not auto-fix Pokémon or grant any write permission.

## Current hardware milestone

The next exact candidate should validate the current hardware-visible work together:

- Quick Games selection remains cached/non-blocking;
- normal Gen I–III open validates the selected cached source instead of rescanning every provider;
- Red/Blue and other near-name releases remain exact-match safe;
- DS launch uses the validated provider path before expensive fallback discovery when appropriate;
- full Games browser has no trainer portraits in game tiles;
- `X = Save / Source` can assign another validated matching Gen I–III save;
- Gen IV provider-neutral setup remains correct;
- Product Home region scenery stays readable;
- round destinations are neutral until focused;
- Items exit, Search-left, Favorites and Release Date ordering remain stable;
- shared on-screen keyboard integration accepts/cancels text safely without regressing controller input.

A previous Product UI candidate at `dee2ad47...` was physically rejected and remains historical evidence only. Current fixes/integrations must be accepted as a new exact artifact.

## Safety / scope locks

- original source saves stay immutable during ordinary editing;
- external emulator saves remain read-only;
- installed-game live writes remain disabled;
- staged editing stays app-owned;
- Launch never grants write permission;
- ambiguous source/content matching fails closed;
- cross-game True Move remains locked;
- source injection remains locked;
- Gen V is not started;
- Master Vault persistence is not started.

## Near-term order

1. settle current Product Home / Games / Quick Games / direct-launch / keyboard integration work;
2. finish one exact-head full application CI pass;
3. package one exact current NRO;
4. physically accept that exact integrated Gen I–IV + Product UI build;
5. begin full app-wide touch-control parity;
6. then build Master Vault + named Banks;
7. continue provider/later-generation expansion and product-facing legality/provenance tooling;
8. only enable source-specific write transactions after backup, validation, readback and recovery are independently proven.

## Hardware acceptance rule

CI green is necessary, but it is not hardware acceptance.

A feature becomes device accepted only when the **exact Actions-built NRO for the exact application commit** is physically tested on a real Switch.
