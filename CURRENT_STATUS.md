# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-29**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are checkpoints only: always re-fetch before modifying a lane, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Last verified head: `710cf37a33ba7a9f29b09953c5d8aa224268e4fd`
- Latest integrated changes: Classic Game Sources, safe Backpack/Items intent, cursor-memory state, real Gen I–IV Pokédex progress, Gen IV trainer-name propagation, grounded trainer portraits, two-pane Settings, and truthful emulator-launch/help presentation
- PR #100 Product UI polish: **CONTENT INTEGRATED INTO THIS LANE; PR #100 CLOSED**

Current exact-head CI boundary at the time of this update:

- Product UI Native #61 — **PASS** — run `36531602414`
- Gen IV Shared Editor Candidate Gate #137 — **PASS** — run `36531602291`
- Host Tests #1701 — **RUNNING** — run `36531602274`

The current integrated head is **not yet hardware accepted**. It becomes a hardware candidate only when the remaining exact-head host/sanitizer gate is green and one exact Actions-built NRO is selected for physical testing.

## Hardware-accepted foundation

### Generation I

Red / Blue / Yellow:

- read support;
- Trainer / Party / Boxes;
- generation-correct Pokémon details;
- staged inventory editing;
- shared View / Create / Edit;
- exact-game move handling;
- controller navigation and source immutability.

**Current Gen I shared-editor foundation is device accepted.**

### Generation II

Gold / Silver / Crystal:

- read support;
- Trainer / Party / Boxes;
- Held Item, Friendship, Pokérus and native Gen II fields;
- staged shared View / Create / Edit;
- exact G/S vs Crystal behavior;
- DVs / Stat Exp / shiny and gender semantics;
- current shared-editor UX physically accepted.

**Current Gen II shared-editor foundation is device accepted.**

### Generation III

Ruby / Sapphire / Emerald / FireRed / LeafGreen:

- rotating-sector save validation;
- Trainer / Party / Boxes;
- inventory;
- staged shared View / Create / Edit;
- Gen III-native fields, moves and compatibility-aware presentation;
- source immutability.

**Current Gen III shared-editor foundation is device accepted.**

## Generation IV — active full editor

Generation IV has moved well beyond the original read-only preview.

Supported identities:

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver

### Hardware-accepted Gen IV base

The first safe Gen IV Party/Box View/Edit milestone was physically accepted on real hardware.

Accepted application checkpoint:

`84dae170deb2756d9b80aec32bf8ad512ce17c31`

That acceptance remains historical evidence for that exact milestone only.

### G4-04 current implementation

PR #92 currently carries:

- Party and Box View/Edit;
- empty Box Add/Create;
- deterministic trainer-bound PK4 Create drafts;
- DP / Platinum / HGSS coverage;
- Held Item editing;
- Language editing with text-preservation checks;
- exact-game Ball domains;
- Pokérus None / Cured / Infected mapping;
- named exact-game Met Location handling;
- native Gen IV move picker;
- species-compatible move filtering;
- exact Gen IV base PP with coherent PP/PP Up reset on move replacement;
- transactional Species mutation;
- gender, ability, growth-rate and Party-stat reconciliation;
- Form editing with exact-game/storage restrictions;
- Giratina / Arceus item-driven form coherence;
- boxed Shaymin Sky fail-closed handling;
- editable OT name / Trainer ID where currently supported;
- read-only SID / PID / deeper origin inspection;
- Box/Party action parity with earlier generations;
- compact move-picker parity and cleaner unavailable-action behavior;
- strict full-save reparse, checksum refresh and rollback;
- unchanged external emulator source.

G4-04 is **not device accepted yet**. It requires one exact integrated green Actions-built NRO and physical testing.

## Integrated Product UI state

The old equal-card developer-dashboard direction has been retired.

The current integrated application includes:

- selected game as the Product Home focus;
- real profile/avatar context;
- game art and source/provider information;
- Party preview using the existing Pokémon sprite pipeline;
- real trainer names and game-appropriate trainer portraits based on proven game/gender data;
- Open and Launch actions;
- distinct Master Vault and Pokédex feature cards;
- teal/cyan non-destructive product focus;
- player-facing copy instead of engineering-status badges;
- real Gen I–IV per-save Pokédex progress plumbing;
- remembered Product Home / shell cursor state across reconstructed menus;
- familiar Classic Game Sources;
- safe Backpack/Items open intent through the normal backup/source flow;
- compact Items and Settings quick actions;
- two-pane Settings with remembered category/option cursor.

Later customizable protagonists deliberately remain generic until exact appearance reconstruction is backed by real save-format data.

## Source discovery and launch

Current source architecture is provider-aware and keeps discovery separate from write permission.

Implemented/foundation providers include:

- RetroArch;
- DraStic;
- melonDS;
- manual/remembered Gen IV assignments;
- native Switch title identities.

Launch infrastructure already supports app-owned binding metadata and game-file linking. Direct DraStic/melonDS content handoff still needs exact integration verification; ambiguous ROM/content mapping must continue to fail closed and request an explicit link.

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
GEN V: NOT STARTED
MASTER VAULT PERSISTENCE: NOT STARTED
```

Issue #89 remains the separate future backup → working copy → explicit Inject Save architecture.

## Next integration boundary

Continue directly on PR #92:

1. re-fetch the live PR #92 head and preserve anything newer than `710cf37a…`;
2. let exact-head Host Tests #1701 finish and fix any failure forward;
3. preserve the now-green Product UI Native #61 and Gen IV Gate #137 behavior;
4. hardware-check Product Home, Classic Game Sources, real trainer/Dex presentation, Settings cursor memory, and Backpack/Items quick entry;
5. verify DraStic/melonDS direct launch handoff while retaining Link Game File as the fail-closed fallback;
6. keep source saves immutable and launch/write authorization separate;
7. produce one combined Actions-built NRO with its exact artifact ID and SHA-256;
8. stop for physical Switch testing.

After that integrated UI candidate passes hardware, the next major frontend tranche is **full app-wide touch controls**.

**ACTIVE PRODUCT PRIORITY: one integrated Gen I–IV + Product UI hardware candidate before touch controls, Gen V, Master Vault persistence, or live source writes.**
