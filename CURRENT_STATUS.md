# PokeBank NX — Current Verified Engineering State

Last updated: **2026-10-01**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are checkpoints only: always re-fetch before modifying a lane, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Current verified head: `941ac9d7b3d4e3b7ab6f0e9195df5588d76ecb3e`

Latest integrated work includes:

- region-aware Product Home hero backdrops;
- game → region identity plumbing;
- grounded trainer portrait atlas support;
- larger Party presentation and stronger handheld typography;
- stronger Party-name hierarchy for handheld scanning;
- branch trainer/region asset packaging in native Product UI and Gen IV candidate builds;
- Classic Game Sources;
- safe Backpack/Items intent;
- real Gen I–IV Pokédex progress;
- Gen IV trainer-name propagation;
- two-pane Settings;
- cursor-memory state;
- truthful emulator-launch/help presentation.

Exact-head CI on `941ac9d7...`:

- Product UI Native — **PASS** — run `36822999872`
- Gen IV Shared Editor Candidate Gate — **PASS** — run `36823000256`
- Host Tests — **PASS** — run `36822999941`

This head is automated-green but **not yet hardware accepted**. Final UI finishing touches are still being made before one exact Actions-built NRO is selected for physical Switch acceptance.

## Audit remediation lane

### PR #101 — forensic finding closure

- Branch: `fix/full-audit-remediation-20260929`
- State: **OPEN / DRAFT / NOT MERGED**
- Current branch head: `408a56446f9dda611732c8186d6ce4148100d65f`
- Fully validated application/reconciliation head beneath the docs-only closure commit: `7b7c2b77c28695c1bf16c3e3004e293edf0d47e4`

Finding disposition:

- **43 VERIFIED**
- **1 DEFERRED WITH JUSTIFICATION — AUDIT-043**
- **0 OPEN**

Exact-head application validation on `7b7c2b77...`:

- Host Tests + ASan/UBSan — **PASS** — run `36823473885`
- Native PR Gate — **PASS** — run `36823473777`
- Product UI Native — **PASS** — run `36823473820`
- Gen I/II Packed Move Focused — **PASS** — run `36823473794`
- Gen I/II Packed Multi-Move Focused — **PASS** — run `36823473781`

The frozen forensic evidence checkpoint remains untouched at:

`143c5e5c341d4f85af30e013808a37d6719560fe`

PR #101 should remain separate until the owner approves integration after the current UI/hardware checkpoint.

## Product/UI parallel lane

PR #97 (`ui/product-shell-round2-20260928`) is now superseded by MAIN integration work.

- Current head: `5e4cf038787557e631197e55dda83ce12bef9419`
- It has no unique commits relative to current PR #92.
- Do not merge it into MAIN simply to preserve history; its useful content has already been superseded/integrated.

## Hardware-accepted foundation

### Generation I

Red / Blue / Yellow:

- Trainer / Party / Boxes;
- generation-correct Pokémon data;
- staged inventory editing;
- shared View / Create / Edit;
- exact-game move handling;
- controller navigation;
- source immutability.

**Device accepted.**

### Generation II

Gold / Silver / Crystal:

- Trainer / Party / Boxes;
- Held Item, Friendship, Pokérus;
- staged shared View / Create / Edit;
- G/S vs Crystal behavior;
- DVs / Stat Exp / shiny / gender semantics;
- controller navigation;
- source immutability.

**Device accepted.**

### Generation III

Ruby / Sapphire / Emerald / FireRed / LeafGreen:

- rotating-sector save validation;
- Trainer / Party / Boxes;
- inventory;
- staged shared View / Create / Edit;
- Gen III-native fields and moves;
- source immutability.

**Device accepted.**

## Generation IV — current full-editor foundation

Supported identities:

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver

The first safe Gen IV Party/Box View/Edit milestone is already physically accepted.

Current G4-04 implementation adds:

- Party / Box View/Edit;
- empty Box Add/Create;
- Held Item / Language / Ball / Pokérus / Met Location;
- native move picker and PP handling;
- species-compatible move filtering;
- Species mutation and dependent-state reconciliation;
- exact-game Form restrictions;
- trainer/origin inspection;
- Box/Party action parity;
- strict full-save reparse/checksum/rollback;
- unchanged external emulator source.

The current **full G4-04 + Product UI combination is hardware pending**.

## Source discovery and launch

Current provider-aware foundation includes:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered source assignment;
- native Switch game identities.

Launch/link metadata is stored by PokeBank NX. Direct DraStic/melonDS content handoff still needs exact hardware verification. If content identity cannot be proven, the app must request an explicit **Link Game File** instead of guessing.

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

1. finish the current Product UI finishing touches on PR #92;
2. keep PR #92 exact-head CI green;
3. produce one exact Actions-built integrated NRO;
4. hardware-test Product Home, Classic Game Sources, Gen IV View/Create/Edit, trainer/region presentation, Party hierarchy, Settings, Items, and launch/link behavior;
5. verify DraStic/melonDS direct launch handoff;
6. if the exact hardware candidate passes, approve integration of PR #101 remediation into the MAIN lane;
7. only then move to full app-wide touch controls.

**ACTIVE PRODUCT PRIORITY: finish and physically accept the integrated Gen I–IV + Product UI foundation before touch controls, Gen V, Master Vault persistence, or live source writes.**
