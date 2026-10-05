# PokeBank NX — Current Verified Engineering State

Last updated: **2026-10-05**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are checkpoints only: always re-fetch before modifying a lane, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Current verified head: `9e8f5e746d07879aaf96da4354df8ac3aaae14f5`
- Current tree: `b24ca7ebbe837793f43793f982e48603b6a2478b`
- Device state for the current combined head: **HARDWARE PENDING**

The active lane now contains the current Gen I–IV application, Gen IV G4-04 editor, Product Home, Games/source assignment work, current hardware-recovery fixes, and the completed forensic-audit remediation package.

### Current exact-head application CI

Normal application validation on `9e8f5e74...` is fully green:

- PokeBank NX Host Tests — **PASS** — run `37274894806`
- PokeBank NX Native PR Gate — **PASS** — run `37274894800`
- PokeBank NX Product UI Native — **PASS** — run `37274894827`
- Gen IV Shared Editor Candidate Gate — **PASS** — run `37274894872`
- Gen I/II Packed Move Focused — **PASS** — run `37274894959`
- Gen I/II Packed Multi-Move Focused — **PASS** — run `37274894900`

Some one-time patch/hotfix harness workflows attached to this SHA are historical repair machinery and are not the normal application gates above. They must not be used to describe the current application head as CI-red.

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
- direct GBA/DS launch routing that prefers a matching installed HOME forwarder before emulator fallback;
- normalized exact-release matching so near-name titles such as Red/FireRed, Gold/HeartGold, and Diamond/BDSP do not collide;
- Quick Games and Product Home data-freeze recovery work;
- preserved Items return, Search-left, profile and source behavior.

### Current physical retest focus

The next owner hardware pass should verify:

1. full Games tiles do not show trainer portraits;
2. `X` on an assigned Gen I–III game can assign another validated matching save to the current profile;
3. region scenery remains visible while hero text stays readable;
4. only the focused round destination uses the blue/cyan focus treatment;
5. a matching installed GBA HOME forwarder — especially Emerald — launches directly rather than falling through to RetroArch/file selection;
6. a matching installed DS HOME forwarder — especially Platinum — launches directly rather than falling through to DraStic/file selection;
7. Quick Games, Items exit, Search-left, Favorites, and Release Date ordering remain stable.

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
- Current live head at this status refresh: `a634460a22810d82e67a04dbf01d2f62f8d25d9c`
- Mode: **READ-ONLY ANALYSIS**

The legality lane now includes substantial Gen I–IV source-game, move/species, encounter/event, transfer, form/origin, PID/RNG, Gen III GameCube, and Gen IV WC4/PCD evidence.

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

The **current combined G4-04 + Product UI head is automated-green but not device accepted**.

## Source discovery and launch

Current source/provider foundation includes:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered source assignment;
- native Switch game identities;
- app-owned game/ROM launch bindings.

Current launch routing can prefer a matching installed HOME forwarder for supported GBA/DS games before emulator fallback. Exact-release matching is used to avoid near-name collisions. This current direct-launch behavior still requires physical verification on the owner’s Switch.

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
2. keep the six normal application validation lanes green;
3. produce one exact Actions-built current candidate after the UI/launch fixes settle;
4. physically retest Games/source assignment, Quick Games, Product Home readability, Items/Search/Favorites/sorting, and GBA/DS direct launch;
5. if that exact artifact passes hardware, mark the current integrated Gen IV + Product UI candidate accepted;
6. then begin full app-wide touch-control parity;
7. keep the read-only legality engine moving in parallel without weakening save safety.

**ACTIVE PRODUCT PRIORITY: physically accept the current integrated Gen I–IV + Product UI foundation before touch controls, Master Vault persistence, Gen V, or live source writes.**
