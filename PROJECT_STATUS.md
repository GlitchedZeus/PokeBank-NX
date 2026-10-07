# PokeBank NX Project Status

Last updated: **2026-10-06**

For exact active heads and CI evidence, read `CURRENT_STATUS.md` first.

## Project identity

- Product: **PokeBank NX**
- Repository: `GlitchedZeus/PokeBank-NX`
- Platform: Nintendo Switch homebrew
- State: active alpha
- Active application / hardware lane: **PR #92**
- Active touch lane: **PR #122**
- Active read-only legality lane: **PR #103**
- Active legality child integration: **PR #149**
- Full forensic remediation: **completed and merged into PR #92 via PR #101**

GitHub is authoritative. Checkpoint SHAs are evidence, never instructions to move a branch backward.

## Headline

PokeBank NX now has:

- a hardware-accepted Gen I–III read/editor foundation;
- a substantially complete staged Gen IV editor for D/P/Pt/HG/SS;
- a modern Product Home and Games/source-management frontend;
- shared in-app keyboard/numpad support with controller + touch input;
- hardware-working Quick Games/preview retention, fast legacy save opening, tested Gen I–III launch resolution and Nintendo DS direct launch;
- a separate full app-wide direct/native touch-control lane with exact-head CI green and hardware acceptance pending;
- established host/native/editor/sanitizer validation lanes;
- a completed full-repository forensic audit and remediation package integrated into the active app lane;
- an actively growing, read-only Gen I–IV legality evidence engine in a separate lane.

The project is currently in **final integrated Product UI / preview / launch-lifecycle hardware polish**, with the touch lane already implemented separately and waiting for physical acceptance/integration order.

## Current application state

PR #92 is the current Gen I–IV application lane.

Current implementation includes:

- Product Home selected-game presentation;
- full Games browser and Quick Games flow;
- game/save/profile source assignment;
- source/provider-aware save identity;
- region-aware hero artwork and trainer presentation;
- larger handheld Party presentation with normalized `Current save party` status;
- real per-save Gen I–IV Pokédex progress;
- sorting / Release Date ordering foundation;
- Favorites foundation;
- Items/Settings/Search navigation work;
- exact-release launch matching for supported legacy/DS flows;
- hardware-working Red/Blue and tested Gen I–III RetroArch launch resolution;
- hardware-working Nintendo DS direct launch without the previous freeze/normal chooser dead end;
- Quick Games cached preview retention;
- shared on-screen keyboard and numeric keypad;
- FireRed/LeafGreen HOME-forwarder launch identity separated from the assigned GBA save identity used by preview/open;
- corrected SWSH/SV/Z-A Current Box SC preflight type handling;
- full staged Gen IV G4-04 editor implementation.

The exact PR #92 checkpoint `2d2d7bc2df57d79fe32c3bc0bfb24f80915b2efd` is fully green across the established application CI matrix and is the current **complete hardware candidate**.

It adds a PokeBank-owned RetroArch return host so normal RetroArch Quit can return through an explicit PokeBank reload path instead of relying on stock RetroArch to remember its caller. The host preserves child `NextLoadPath` requests and only reloads PokeBank after a normal final return.

The current combined application is **not hardware accepted yet** until this exact NRO passes on device.

## Generation status

### Gen I — ✅ device accepted

Red / Blue / Yellow read, inventory and shared Pokémon editor workflows are hardware accepted.

Current Product UI launch testing also confirms Gen I content resolution including Red/Blue is working; the remaining RetroArch issue is return lifecycle after normal Quit, not ROM matching.

### Gen II — ✅ device accepted

Gold / Silver / Crystal read, inventory and shared Pokémon editor workflows are hardware accepted.

Gen II RetroArch launch resolution is also working in the current Product UI lane; normal RetroArch Quit still follows the same unresolved return-lifecycle limitation.

### Gen III — ✅ device accepted

Ruby / Sapphire / Emerald / FireRed / LeafGreen read, inventory and shared Pokémon editor workflows are hardware accepted.

FireRed/LeafGreen forwarder cards now keep installed HOME title identity for launch while preview/open uses the real assigned GBA battery-save source.

### Gen IV — 🧪 implemented / hardware pending

Diamond / Pearl / Platinum / HeartGold / SoulSilver have the staged full-editor foundation.

The earlier safe Party/Box View/Edit milestone is physically accepted. Current G4-04 adds Create, native field parity, species/form reconciliation, move handling, action parity, strict checksum/reparse/rollback behavior, provider-neutral source assignment and the current Product UI/source architecture.

Nintendo DS direct launch now works on hardware; Diamond specifically passed. DraStic's own `Quit to Launcher` behavior is respected rather than treated as a PokeBank return requirement.

The full G4-04 + current frontend integration still requires its own exact physical acceptance pass.

### Gen V+ — ⏳ not a production workflow yet

Gen V is not started as a product editor. Later DS/3DS/modern Switch work remains provider, identity, validation, preview, or research groundwork until explicitly advanced.

Modern Switch Product Home groundwork currently includes LGPE/native identity paths and SC-family preview validation work for SWSH/SV/Z-A, but these are not generally accepted product workflows yet.

## Touch controls

PR #122 is the dedicated **touch-control lane**, stacked on current MAIN.

Current exact touch head: `ec89f704e69995f543c7df30f166ca3788a5ca5d`.

Implemented coverage includes:

- Product Home / selected-game card / Game Library / Quick Games;
- profile picker, file/source/setup/Save Instance surfaces;
- loaded-game Home destinations and party cards;
- modern boxes/storage and trainer surfaces;
- Settings / Themes / Diagnostics / More;
- classic inventory and item/stat edit dialogs;
- shared keyboard and number entry;
- action sheets, confirmations and backup selection;
- legality/ribbon report scrolling;
- Gen I–IV editor workspaces and picker families;
- screen-owned direct touch targets instead of queued multi-frame synthetic D-pad walking.

Exact-head touch validation is green across Touch Input Gating, Host Tests, Product UI Native, Native PR Gate and both Gen I/II focused lanes. Physical Switch acceptance is still pending, and the lane remains isolated until MAIN is ready for forward integration.

## Audit / hardening

The repository-wide forensic audit is complete and frozen.

PR #101 remediation was merged into the active PR #92 branch on 2026-10-05.

Final remediation disposition:

- **43 VERIFIED / FIXED**
- **1 DEFERRED WITH JUSTIFICATION — AUDIT-043**
- **0 OPEN**

The audit/remediation work strengthens parser boundaries, memory safety, durability, backup/custody behavior, source-mutation policy, generated-data checks, native build coverage, and regression confidence without enabling unsafe source writes.

AUDIT-043 remains intentionally deferred until a real failing disposable Gen IV save is available; the app must not manufacture trainer identity from filenames.

## Legality engine

PR #103 is the separate **read-only Gen I–IV legality analysis lane**.

Current accepted legality head: `0b16a0bdaf778379cda2847c3bb9a713f75c63e4`.

Current implemented/accepted foundation includes:

- exact source-game profiles and generation ceilings;
- exact-game move compatibility;
- Gen I/II encounter/trade/history evidence;
- extensive Gen III event/PID/IV/RNG evidence;
- Colosseum/XD/e-Reader/GameCube-specific evidence;
- Gen III → IV Pal Park evidence;
- extensive Gen IV wild/static/trade/PokéWalker/WC4/PCD evidence;
- state-aware Gen IV egg/static/gift provenance;
- Method J/K lead/history evidence under conservative bounded semantics;
- accepted Gen III/IV language-domain production enforcement;
- coverage-aware verdict semantics.

PR #149 is the active isolated child integration for **Gen III/IV Ball-ID generation domains**. Its candidate head `ff1b15ca67539b4c8cf799db86cef4c76c0fa829` is six commits forward from the accepted PR #103 head and its exact-head Host Tests run `37427992436` passes.

The Ball-ID integration remains conservative:

- exact Gen III nonzero values above 12 are generation/format impossible;
- exact Gen IV nonzero values above 24 are generation/format impossible;
- zero remains unresolved under the current sentinel contract;
- in-domain values are not automatically encounter-legal;
- source-less analysis keeps generic behavior.

The legality lane is analysis only. Unknown, unsupported, unreconstructable history and bounded-search exhaustion stay **Incomplete**; the engine does not auto-fix Pokémon or grant any write permission.

## Current hardware milestone

The next complete PR #92 owner candidate should validate the current hardware-visible work together, not as another partial round:

- stable RetroArch **Quit → PokeBank NX** return lifecycle;
- preserve working Red/Blue and Gen I–III content resolution;
- preserve Nintendo DS direct launch and no-freeze/no-normal-chooser behavior;
- Shield / SV / Z-A Product Home preview no longer fails on the stale Current Box SC type assumption;
- FireRed / LeafGreen forwarder cards use the assigned GBA save for preview/open while retaining the installed forwarder for launch;
- successful cards consistently show user-facing party status such as `Current save party`;
- Quick Games retains trainer/dex/party state;
- keyboard/numpad remain correct;
- GB/GBC/GBA save opening remains fast.

Do not issue another complete owner-test NRO while a known MAIN blocker is intentionally unresolved.

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

1. physically test the exact `2d2d7bc2...` Gen I–IV + Product UI candidate, especially RetroArch normal Quit → PokeBank NX return;
2. verify the current SC/party/FireRed preview fixes in that same exact build;
3. if the exact artifact passes, record it as the integrated hardware-accepted application checkpoint;
6. physically validate and integrate PR #122 touch parity forward;
7. then build Master Vault + named Banks;
8. continue provider/later-generation expansion and product-facing legality/provenance tooling;
9. only enable source-specific write transactions after backup, validation, readback and recovery are independently proven.

## Hardware acceptance rule

CI green is necessary, but it is not hardware acceptance.

A feature becomes device accepted only when the **exact Actions-built NRO for the exact application commit** is physically tested on a real Switch.
