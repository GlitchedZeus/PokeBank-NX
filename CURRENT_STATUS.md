# PokeBank NX — Current Verified Engineering State

Last updated: **2026-10-05**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are checkpoints only: always re-fetch before modifying a lane, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Current verified head at this refresh: `5bf7aa38d52f8cb56e94b0d32a05ca724501c92e`
- Current tree: `70c4d4d587a293c099e3133066d663e1348c4358`
- Runtime hotpath fix parent: `894e5f89071407dab4531573fec95c6234704670`
- Regression-contract parent: `8dcd1d36d53f08f920f255fdcf5767740bc58a86`
- Device state for the current combined head: **HARDWARE PENDING**

The active lane contains the current Gen I–IV application, Gen IV G4-04 editor, Product Home, Games/source assignment work, current hotpath/hardware-recovery fixes, and the completed forensic-audit remediation package.

### Current exact-head application CI

Normal application validation on `5bf7aa38...` at this refresh:

- PokeBank NX Product UI Native — **PASS** — run `37280003888`
- PokeBank NX Native PR Gate — **PASS** — run `37280003895`
- Gen IV Shared Editor Candidate Gate — **PASS** — run `37280003858`
- Gen I/II Packed Move Focused — **PASS** — run `37280003790`
- Gen I/II Packed Multi-Move Focused — **PASS** — run `37280003872`
- PokeBank NX Host Tests — **IN PROGRESS** — run `37280003796`

Five of the six normal exact-head application lanes are green; the full Host Tests lane is still running at this status refresh. The Host lane includes the long host/sanitizer/regression pass and must finish before the exact head is described as fully automated-green.

Several one-time direct-launch / Quick Games patch-harness workflows attached to the same SHA are historical repair machinery. Their failures are **not** the normal application validation matrix above and are not hardware candidates.

## Current Product UI / hardware-fix state

Recent PR #92 work includes:

- full six-column Games browser as the game/save/profile assignment surface;
- no trainer portraits inside full Games tiles;
- `X = Save / Source` assignment flow for exact game/profile sources;
- Gen I–III source reassignment when another validated matching save exists;
- provider-neutral Gen IV setup/assignment flow;
- Product Home region scenery with readability retuning;
- neutral idle round destination controls with blue/cyan focus only;
- sorting and Release Date ordering foundation;
- Favorites foundation;
- direct GBA/DS launch routing with exact-release matching and installed-forwarder/provider fallback logic;
- Quick Games and Product Home data-freeze recovery work;
- preserved Items return, Search-left, profile and source behavior;
- hotpath work that removes synchronous source discovery from Quick Games selection and narrows normal Gen I–III open to the selected cached source.

### Current physical retest focus

The next owner hardware pass should verify the exact current candidate, including:

1. Quick Games `A` selection remains cached/navigation-only and does not synchronously rediscover saves or resolve launch targets;
2. normal Gen I–III `A` open validates only the selected cached source rather than rescanning every configured legacy provider;
3. RetroArch Red/Blue matching is release-aware and unique while preserving Red vs FireRed and other collision guards;
4. DS launch resolution prefers an already validated emulator/provider path before an expensive installed-forwarder fallback scan;
5. previously passing Yellow, Gold, Silver, Crystal, Ruby, Sapphire, FireRed, LeafGreen and Emerald launch/return behavior remains intact;
6. full Games tiles still omit trainer portraits and `X = Save / Source` can assign another validated matching save;
7. region scenery/readability, destination focus styling, Items exit, Search-left, Favorites and Release Date ordering remain stable.

Do not transfer hardware acceptance from an older SHA to this current head. The owner previously rejected the `dee2ad4745a8d83d3d804a3e244e33cfd2c39525` Product UI candidate on hardware; the current recovery/fix series is newer and must be tested as its own exact artifact.

## Audit remediation — COMPLETE / INTEGRATED

PR #101 (`fix/full-audit-remediation-20260929`) is now **MERGED into the active PR #92 branch**.

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
- Current live head at this status refresh: `fa4013103e6d418ff3e7268be7bb3f3c5951c4bf`
- Mode: **READ-ONLY ANALYSIS**

The legality lane now includes substantial Gen I–IV source-game, move/species, encounter/event, transfer, egg-state, form/origin, PID/RNG, Gen III GameCube, and Gen IV WC4/PCD evidence. Recent work includes state-aware Gen IV static/gift egg provenance while keeping state-unknown callers conservative.

Its verdict model intentionally distinguishes:

- **Invalid** — available evidence proves a contradiction;
- **No problems found** — checks that actually ran found no contradiction;
- **Incomplete** — evidence coverage is not sufficient for a stronger conclusion.

Missing evidence and bounded-search exhaustion remain `Incomplete`, never silently `Invalid` or “legal.”

Safety boundaries for this lane:

- no auto-fix;
- no source writes;
- no staged mutation;
- no write-permission changes;
- no Gen V work;
- no Master Vault dependency.

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

The first safe Gen IV Party/Box View/Edit milestone is physically accepted. The later G4-04 implementation adds Create, additional native fields, forms, species mutation/reconciliation, move handling, action parity, checksum/reparse/rollback behavior, and Product UI integration.

The **current combined G4-04 + Product UI head remains not device accepted**. Five normal exact-head gates are currently green and Host Tests are still running at this refresh.

## Source discovery and launch

Current source/provider foundation includes:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered source assignment;
- native Switch game identities;
- app-owned game/ROM launch bindings.

Current launch routing uses exact-release matching to avoid near-name collisions and can choose validated provider/emulator routes or matching installed HOME forwarders according to the current game/provider evidence. The current hotpath specifically avoids doing expensive launch/source resolution synchronously from Quick Games selection and prioritizes an already validated DS provider path before a fallback installed-forwarder scan.

If content identity cannot be proven, the app must continue to fail closed and use an explicit source/content selection or link fallback instead of guessing.

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
2. finish the exact-head Host Tests lane and keep all six normal application validation lanes green;
3. produce one exact Actions-built current candidate after the hotpath/UI/launch fixes settle;
4. physically retest Quick Games, selected-source open, Red/Blue matching, DS launch routing, Games/source assignment, Product Home, Items/Search/Favorites/sorting, and previously passing Gen I–III routes;
5. if that exact artifact passes hardware, mark the current integrated Gen IV + Product UI candidate accepted;
6. then begin full app-wide touch-control parity;
7. keep the read-only legality engine moving in parallel without weakening save safety.

**ACTIVE PRODUCT PRIORITY: physically accept the current integrated Gen I–IV + Product UI foundation before touch controls, Master Vault persistence, Gen V, or live source writes.**
