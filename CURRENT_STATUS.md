# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-29**

Repository: `GlitchedZeus/PokeBank-NX`

GitHub is authoritative. Recorded SHAs are evidence checkpoints only. Always re-fetch live GitHub, preserve newer commits, and never reset/rebase backward or force-push over newer work.

## Active MAIN lane

### PR #92 — Gen IV + integrated Product UI

- Branch: `feature/gen4-full-editor-20260928`
- State: **OPEN / DRAFT / NOT MERGED**
- Application checkpoint incorporated by this documentation refresh: `710cf37a33ba7a9f29b09953c5d8aa224268e4fd`
- PR #100 Product UI polish: **MERGED INTO PR #92**

The integrated application now includes the modern Product Home, Classic Game Sources, real Party sprites, real Gen I–IV Pokédex progress, Gen IV trainer-name propagation, grounded trainer portraits, Backpack/Items quick entry, cursor restoration, two-pane Settings, and current emulator-launch/control cleanup.

### Current CI boundary

For application checkpoint `710cf37a...`:

- Host Tests #1701 — **RUNNING** when this status was written.
- Product UI Native #61 — **RUNNING**.
- Gen IV Shared Editor Candidate Gate #137 — **QUEUED**.

Earlier Product UI failures at this stage were stale text/navigation contracts, not evidence of native compile failure. The current contracts now follow real Pokédex progress, Classic Game Sources, and the seven-item Product Home dock. Exact-head CI remains authoritative.

## Hardware-accepted foundation

### Generation I

Red / Blue / Yellow read support, Trainer / Party / Boxes, generation-correct Pokémon details, staged inventory, shared View/Create/Edit, exact-game move handling, and source immutability are device accepted.

### Generation II

Gold / Silver / Crystal read support, Trainer / Party / Boxes, Held Item, Friendship, Pokérus, DVs / Stat Exp, generation-native shiny/gender behavior, shared View/Create/Edit, and source immutability are device accepted.

### Generation III

Ruby / Sapphire / Emerald / FireRed / LeafGreen rotating-sector validation, Trainer / Party / Boxes, inventory, shared View/Create/Edit, generation-native fields/moves, and source immutability are device accepted.

## Generation IV — active full editor

Supported identities: Diamond, Pearl, Platinum, HeartGold, SoulSilver.

The first safe Gen IV Party/Box View/Edit milestone was physically accepted on real hardware at application checkpoint `84dae170deb2756d9b80aec32bf8ad512ce17c31`. That acceptance applies only to that milestone.

Current G4-04 work includes Party/Box View/Edit, empty Box Add/Create, trainer-bound PK4 Create drafts, DP/Pt/HGSS coverage, Held Item, Language, Ball, Pokérus, Met Location, native move selection, species-compatible move filtering, exact PP/PP-Up behavior, Species mutation, gender/ability/growth/stat reconciliation, exact-game Form handling, trainer/origin inspection, action parity, strict reparse/checksum/rollback, and unchanged external emulator sources.

The full G4-04 + Product UI combination is **hardware pending**.

## Integrated Product UI

Current integrated UI includes:

- selected game as Product Home focus;
- profile/avatar context;
- game artwork and source/provider state;
- real Party sprites through the existing sprite pipeline;
- real trainer names;
- game/gender-grounded trainer portraits;
- real Gen I–IV per-save Pokédex progress;
- Open and Launch actions;
- distinct Master Vault and Pokédex feature identities;
- teal/cyan non-destructive interaction focus;
- Classic Game Sources;
- remembered navigation/cursor state;
- safe Backpack/Items quick intent through normal source/backup flow;
- compact Items and Settings shortcuts;
- two-pane Settings with remembered category/option cursor.

Later customizable protagonists remain generic until exact appearance reconstruction is backed by real save-format data.

## Source discovery and launch

Current provider-aware source work includes RetroArch, DraStic, melonDS, remembered/manual Gen IV assignment, and native Switch title identities.

Game launching is separate from save-writing authorization. PokeBank-owned game-file links are used when content cannot be proven automatically.

Current emulator-launch cleanup keeps visible controls truthful and retains fail-closed linking. Direct DraStic/melonDS content handoff still needs exact integrated hardware verification.

## Full forensic repository audit

The tracked-repository forensic review is complete on frozen evidence branch `audit/full-repository-line-by-line-20260928` at `143c5e5c341d4f85af30e013808a37d6719560fe`.

Coverage:

- **746 / 746** tracked paths accounted;
- **711 / 711** text files fully read;
- **35 / 35** non-text entries inspected;
- **0** pending ledger entries;
- **0** duplicate finding IDs.

The audit is closed as a coverage exercise. Remediation is tracked separately on `fix/full-audit-remediation-20260929`, last observed at `31a2c742aef5d89d84165a0629e9de05a7373eee` before this refresh.

The remediation matrix records **44 findings**: 2 P1, 17 P2, 20 P3, and 5 P4. Early remediation has already retired the destructive legacy PKSE import workflow and reconciled the historical Product UI compile blocker. Remaining fixes must be applied forward without weakening current safety invariants.

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

## Immediate integration boundary

Continue on live PR #92:

1. re-fetch the exact head and Actions;
2. keep the merged Product UI and current Gen IV editor intact;
3. resolve any exact-head CI failure forward;
4. verify cursor restoration and Backpack/Items quick entry;
5. verify DraStic/melonDS launch handoff while keeping Link Game File fail-closed;
6. reconcile high-priority audit remediation without resetting MAIN;
7. run full host/sanitizer/Gen-IV/native gates;
8. produce one exact combined Actions-built NRO;
9. stop for physical Switch testing.

After that integrated candidate passes hardware, the next major frontend tranche is **full app-wide touch controls**.

**ACTIVE PRODUCT PRIORITY: one integrated Gen I–IV + Product UI hardware candidate, while audit remediation proceeds forward without weakening save safety.**
