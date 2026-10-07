# PokeBank NX — Current Verified Engineering State

Last updated: **2026-10-06**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are checkpoints only: always re-fetch before modifying a lane, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Current exact application head: `2d2d7bc2df57d79fe32c3bc0bfb24f80915b2efd`
- Application tree: `91c4b7484bf11d9408d0818a92c38ad93156916f`
- Product UI artifact workflow: `37497307544`
- Product UI artifact: `PokeBank-NX-Product-UI-37497307544`
- Artifact ID: `11427853807`
- NRO SHA-256: `785b99733495fa2f0c46ffeef0c80e8987cfbfce170464fa9a07c5ac2524f048`
- Device state: **READY FOR EXACT HARDWARE TEST / DEVICE_ACCEPTED=false**

The exact `2d2d7bc2...` application is fully CI-green and combines the current Product Home/save-preview repairs with the PokeBank-owned RetroArch return host. This is the next complete owner hardware round. `DEVICE_ACCEPTED=false` remains authoritative until this exact NRO passes on Switch.

### Exact-head application CI

For `2d2d7bc2df57d79fe32c3bc0bfb24f80915b2efd`:

- PokeBank NX Product UI Native — **PASS** — run `37497307544`
- PokeBank NX Native PR Gate — **PASS** — run `37497307608`
- PokeBank NX Host Tests — **PASS** — run `37497307470`
- Gen IV Shared Editor Candidate Gate — **PASS** — run `37497307909`
- Gen I/II Packed Move Focused — **PASS** — run `37497307879`
- Gen I/II Packed Multi-Move Focused — **PASS** — run `37497307798`

Green CI does not replace physical acceptance of this exact hardware candidate.

## Current Product UI / hardware-fix state

Current PR #92 behavior includes:

- full six-column Games browser as the game/save/profile assignment surface;
- Quick Games cached/non-blocking selection with trainer / Pokédex / Party presentation retained;
- shared in-app keyboard and numeric keypad with controller + touch support;
- fast normal Gen I–III save opening without the earlier full-provider hotpath rescan;
- Gen I/II/III RetroArch content resolution including Red/Blue release-safe matching;
- Nintendo DS direct launch without the earlier freeze and normal chooser dead end;
- provider-neutral Gen IV setup/assignment flow;
- Product Home party presentation normalized to user-facing states such as `Current save party`;
- FireRed / LeafGreen HOME-forwarder launch identity separated from the real assigned GBA battery-save identity used by preview/open;
- SWSH/SV/Z-A Current Box SC preflight corrected to the native one-byte type rather than the stale UInt32 assumption;
- Product Home region scenery, Items/Search behavior, sorting / Release Date and Favorites foundation;
- source/write safety unchanged.

### Current physical hardware evidence

Hardware PASS / working behavior to preserve:

1. Gen I / II / III RetroArch games launch successfully, including Red and Blue;
2. Nintendo DS direct launch works; Diamond specifically passed;
3. DS launch no longer freezes;
4. Quick Games / Quick View retains trainer, dex and party presentation;
5. shared keyboard and numpad presentation/input passed;
6. GB/GBC/GBA save opening is fast again.

RetroArch return-lifecycle repair now included in the exact candidate:

- PokeBank captures the exact currently running NRO path.
- RetroArch/core launch routes through bundled `PokeBankReturnHost.nro`.
- Child `NextLoadPath` requests are preserved and honored inside the host.
- A normal final RetroArch return with no child scheduled causes the host to schedule the exact PokeBank NRO back through the outer loader.
- The final Product UI NRO has been checked to contain the embedded return-host payload.

This path is CI- and packaging-validated but still requires physical Switch proof.

Nintendo DS quit-to-PokeBank is **not** an acceptance requirement because DraStic exposes `Quit to Launcher`; DS acceptance is direct exact-ROM boot without the earlier chooser/freeze behavior.

### Preview / Product Home retest work already landed

The intermediate `3d2ded8b...` checkpoint contains:

- normalized party status labels;
- native SC Current Box type correction for SWSH/SV/Z-A;
- FireRed/LeafGreen forwarder save-source split;
- exact-head host fixtures aligned with the corrected SC type.

These are ready to be included in the next complete owner hardware round together with the RetroArch return fix.

## Touch controls lane

### PR #122 — full app-wide touch parity

- Branch: `feature/touch-controls-20261005`
- State: **OPEN / DRAFT / NOT MERGED / DEVICE_ACCEPTED=false**
- Exact touch head: `ec89f704e69995f543c7df30f166ca3788a5ca5d`
- Stacked MAIN base: `3d2ded8ba350e541211ca2c2af0019f018044d78`
- Product UI Native artifact run: `37426422772` — **PASS**
- NRO SHA-256: `295825eee1a82303b147a791ef8475e15829ca20931f13d36fd258369e64f1d2`

The dedicated touch lane now implements direct/native screen-owned touch behavior across Product Home, Games/Quick Games, source assignment, editor/picker families, storage, modal controls, keyboard/numpad, reports and major AppShell surfaces. The old generic multi-frame synthetic D-pad/card-tap path has been removed from runtime.

Exact-head touch validation is green across Touch Input Gating, Host Tests, Product UI Native, Native PR Gate and both Gen I/II focused lanes. Physical Switch acceptance is still pending. Touch stays isolated until MAIN is ready for forward integration.

## Audit remediation — COMPLETE / INTEGRATED

PR #101 (`fix/full-audit-remediation-20260929`) is **MERGED into the active PR #92 branch**.

- PR #101 merged: **2026-10-05**
- Merge commit into PR #92: `46e3af0a2d023de9587a297d9cc93ba78a184591`
- Frozen forensic evidence checkpoint: `143c5e5c341d4f85af30e013808a37d6719560fe`
- Final finding disposition: **43 VERIFIED / FIXED, 1 DEFERRED WITH JUSTIFICATION, 0 OPEN**

The deferred item remains **AUDIT-043**: device-observed Gen IV save rows can lose trainer-name presentation despite synthetic parser coverage. It remains deferred until a real failing disposable Gen IV save is available. Do not manufacture trainer identity from a filename.

The frozen audit evidence remains historical/frozen and must not be rewritten merely because the remediation was integrated.

## Legality engine lane

### PR #103 — Gen I–IV evidence-aware legality analysis

- Branch: `feature/legality-engine-gen1-4-20260929`
- State: **OPEN / DRAFT / NOT MERGED**
- Current accepted legality head: `0b16a0bdaf778379cda2847c3bb9a713f75c63e4`
- Mode: **READ-ONLY ANALYSIS**

The accepted legality lane includes substantial Gen I–IV source-game, move/species, encounter/event, transfer, egg-state, form/origin, PID/RNG, Gen III GameCube and Gen IV Method J/K/history evidence. Gen III/IV language-domain enforcement is accepted on exact sources while preserving the project rule that unwired/unknown zero values remain unresolved rather than fabricated.

### PR #149 — active legality child integration

- Branch: `feature/legality-gen34-ball-domain-production-20261006`
- State: **OPEN / DRAFT / NOT MERGED**
- Candidate head: `ff1b15ca67539b4c8cf799db86cef4c76c0fa829`
- Base: accepted PR #103 head `0b16a0bdaf778379cda2847c3bb9a713f75c63e4`
- Host Tests run `37427992436` — **PASS**

PR #149 integrates source-backed Gen III/IV Ball-ID generation domains into the production analyzer:

- exact Gen III nonzero Ball IDs above 12 are format/generation-impossible;
- exact Gen IV nonzero Ball IDs above 24 are format/generation-impossible;
- Ball 0 remains unresolved/skipped under the current sentinel contract;
- in-domain values are not automatically encounter-legal;
- source-less analysis retains conservative generic behavior.

PR #149 remains a child validation/integration PR until explicitly promoted. It is not merged into MAIN.

The legality verdict contract remains:

- **Invalid** — available source-backed evidence proves a contradiction;
- **No problems found** — checks that actually ran found no contradiction;
- **Incomplete** — evidence coverage is insufficient for a stronger conclusion.

Unknown, unsupported, unreconstructable history and bounded-search exhaustion remain `Incomplete`.

Safety boundaries for this lane:

- no auto-fix;
- no source writes;
- no staged mutation;
- no write-permission changes;
- no Gen V work;
- no Master Vault dependency;
- no MAIN merge without explicit owner authorization.

## Hardware-accepted foundation

### Generation I — DEVICE ACCEPTED

Red / Blue / Yellow:

- read support;
- Trainer / Party / Boxes;
- generation-correct Pokémon details;
- staged inventory editing;
- shared View / Create / Edit;
- exact-game move handling;
- controller navigation;
- source immutability.

### Generation II — DEVICE ACCEPTED

Gold / Silver / Crystal:

- Trainer / Party / Boxes;
- Held Item, Friendship, Pokérus;
- DVs / Stat Exp / shiny / gender semantics;
- staged shared View / Create / Edit;
- generation-specific behavior;
- source immutability.

### Generation III — DEVICE ACCEPTED

Ruby / Sapphire / Emerald / FireRed / LeafGreen:

- rotating-sector validation;
- Trainer / Party / Boxes;
- inventory;
- staged shared View / Create / Edit;
- Gen III-native fields and moves;
- source immutability.

## Generation IV — full-editor implementation / hardware pending

Supported identities:

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver

The first safe Gen IV Party/Box View/Edit milestone is physically accepted. The later G4-04 implementation adds Create, additional native fields, forms, species mutation/reconciliation, move handling, action parity, checksum/reparse/rollback behavior, Product UI integration, provider-neutral save assignment, direct DS launch and shared text input.

The current combined G4-04 + Product UI application remains **not device accepted**. The next complete hardware candidate is being held until the RetroArch return lifecycle is solved and combined with the current preview fixes.

## Source discovery and launch

Current source/provider foundation includes:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered source assignment;
- native Switch game identities;
- app-owned game/ROM launch bindings.

Hardware now confirms direct launch resolution for tested Gen I/II/III content and Nintendo DS/Diamond. Exact-release matching prevents near-name collisions, and bounded discovery avoids the earlier blocking scans and chooser dead ends.

If content identity cannot be proven, the app must continue to fail closed and use an explicit source/content selection or link fallback instead of guessing.

The remaining legacy-launch issue is **return lifecycle**, not ROM identification: normal RetroArch Quit currently exits to Nintendo HOME instead of returning to PokeBank NX.

## Permanent safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE DRASTIC / MELONDS WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN / AMBIGUOUS SOURCE: FAIL CLOSED
LAUNCH PERMISSION != WRITE PERMISSION
CROSS-GAME TRUE MOVE: LOCKED
SOURCE INJECTION: LOCKED
GEN V: NOT STARTED
MASTER VAULT PERSISTENCE: NOT STARTED
```

## Next integration boundary

1. preserve the current PR #92 head or any newer forward-only work;
2. implement and validate a stable RetroArch normal-Quit return contract without regressing working Gen I–III launch resolution;
3. keep the current SC/party/FireRed preview repairs intact;
4. run the full normal application CI matrix on the resulting exact head;
5. produce **one** combined owner-test NRO only after the known MAIN blockers are resolved;
6. physically retest RetroArch launch/return, Shield/SV/Z-A preview, FireRed/LeafGreen forwarder preview/open, Quick Games, keyboard/numpad, DS direct launch and fast legacy opening;
7. if that exact artifact passes hardware, mark the integrated Gen IV + Product UI candidate accepted;
8. then physically validate and integrate PR #122 touch parity forward;
9. keep the read-only legality engine moving in parallel without weakening save safety.

**ACTIVE PRODUCT PRIORITY: finish and physically accept one complete Gen I–IV + Product UI foundation before Master Vault persistence, Gen V, or live source writes.**
