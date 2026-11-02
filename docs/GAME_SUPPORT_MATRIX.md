# PokeBank NX — Game Support / Verification Matrix

Last updated: **2026-10-17** (verified project status; historical generation detail below retained).

**Gen I–V application baseline:** [PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) is an unmerged draft. The owner's exact [`7fea1797` NRO retest](hardware/OWNER_RETEST_7fea1797_20261017.md) recorded **42 PASS / 54 targeted checks**, not per-title exhaustive acceptance. Six entries have reported problems/investigations and six other items remain unverified. Tested game/emulator/controller names were not supplied, so this matrix does **not** mark all 20 games accepted on that build. [All-release gate #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) stays OPEN.

| Generation | Games | Read / source state | Staged editing state | Physical acceptance evidence |
|---|---|---|---|---|
| I | Red / Blue / Yellow | RetroArch / provider-aware validated legacy sources | Shared native View/Create/Edit, Trainer/Items foundation | Earlier accepted foundation; current shared Moves, controls and scroll among owner's targeted PASS; Bag Add and Gen I Trainer enhancement pending |
| II | Gold / Silver / Crystal | RetroArch / validated legacy sources | Shared native DV/Stat Exp, View/Create/Edit and inventory | Earlier accepted foundation; Gen II Create A/B and Data pane PASS in latest report; Bag Add and Crystal inline Met Time pending |
| III | Ruby / Sapphire / Emerald / FireRed / LeafGreen | RetroArch / validated provider-aware legacy sources | Shared Gen III editor and Bag staged edits | Earlier accepted foundation; current Move/PP and location layout PASS; Bag Add workflow needs repair |
| IV | Diamond / Pearl / Platinum / HeartGold / SoulSilver | DraStic / melonDS / remembered/manual sources | Shared PK4 staged View/Edit/Create/Review/Export Copy | Earlier milestone; current Moves/location targeted PASS; emulator Bag Add and export/reopen not fully verified |
| V | Black / White / Black 2 / White 2 | Exact BW/B2W2 source instances, profile assignment, DraStic/melonDS/RetroArch/manual paths | Shared PK5 Create/Edit, Trainer/Party/Boxes, Bag, Review/Export Copy | Party/Boxes/Trainer display PASS; left-side clipping and PP/PP Ups spacing reported; Create/Review/Discard/Export need dedicated retest |
| VI–IX | Later 3DS and Switch families | Separate research/future integration | No owner-accepted Gen VI–IX shared editor | **ON HOLD** until exact Gen I–V owner acceptance |

**Legality is separate.** [PR #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) contains production-wired Gen I–IV analysis in its own branch and an engine-only Gen V analyzer; it has **not** been integrated into the latest hardware NRO. Differential corpus success is not a universal historical legality certificate.

**Permanent source safety:** original emulator/installed saves are read-only; editor work stays in the app-owned staged copy; export generates an independent result; no source injection authorized.

The generation-specific material below records earlier accepted foundation and implementation details; treat the newer snapshot above as the active acceptance status.

## Generation I

Hardware-accepted current foundation includes:

- Trainer / Party / Boxes;
- generation-correct Pokémon data;
- Bag/PC Items;
- strict source validation;
- staged shared View/Create/Edit;
- exact-game move handling;
- source immutability.

Current Product Home/Games integration adds source assignment, sorting/favorites foundations, real per-save Pokédex progress, and grounded trainer presentation on top of this accepted editor foundation. Those newer shell changes remain part of the current integrated hardware pass.

## Generation II

Hardware-accepted current foundation includes:

- Trainer / Party / Boxes;
- Held Item, Friendship and Pokérus;
- DVs / Stat Exp;
- gender/shiny semantics;
- Crystal-specific native fields;
- staged shared View/Create/Edit;
- inventory;
- source immutability.

Gold/Silver do not fabricate a modern SID. Crystal-specific data is only shown where the exact format stores it.

## Generation III

Hardware-accepted current foundation includes:

- strict rotating-sector validation;
- Trainer / Party / Boxes;
- PK3 browsing/editing;
- inventory;
- exact Gen III-native fields;
- staged shared View/Create/Edit;
- source immutability.

Current integrated development adds real per-save Pokédex progress, game/gender-aware trainer presentation, source reassignment from the Games browser, and installed-forwarder launch preference where a matching title can be proven.

## Generation IV

Supported identities:

- Diamond
- Pearl
- Platinum
- HeartGold
- SoulSilver

Source/provider foundation includes:

- DraStic;
- melonDS;
- remembered/manual source assignment;
- provider-neutral setup/assignment;
- fail-closed validation before assignment/open.

The first staged Party/Box View/Edit milestone is physically accepted.

Current G4-04 implementation includes:

- Party / Box View/Edit;
- empty Box Add/Create;
- native Held Item / Language / Ball / Pokérus / Met Location;
- native Gen IV move picker with species compatibility;
- exact base PP and PP Up reset behavior on move replacement;
- Species mutation and dependent-state reconciliation;
- supported Form editing and exact-game restrictions;
- editable OT / Trainer ID where currently supported;
- trainer/origin inspection;
- strict save reparse/checksum/rollback;
- external source immutability.

The integrated Product UI carries real Gen IV trainer-name propagation, real Gen IV Pokédex progress, game/gender-aware trainer presentation, Games/source assignment, current sorting/favorites work, and direct-launch routing.

The full current G4-04 + Product UI head is **automated-green but not yet device accepted**.

## Product Home / Games / launch integration

Current integrated application work includes:

- full Games artwork browser as a game/save/profile assignment surface;
- `X = Save / Source` source assignment flow;
- provider-neutral Gen IV setup;
- sorting and Release Date ordering foundation;
- Favorites foundation;
- region-scene Product Home presentation with readability treatment;
- neutral idle destination controls with focused blue/cyan state;
- installed HOME-forwarder preference for matching GBA/DS launch targets;
- exact-release matching to reduce similar-name collisions;
- provider/file/link fallback when the target cannot be proven.

The current GBA/DS forwarder-preference and latest Quick Games/Product Home recovery fixes still require physical verification.

## Read-only legality analysis

A separate PR #103 lane now implements a substantial **Gen I–IV evidence-aware legality foundation**.

Current evidence work includes exact source-game profiles, move/species ceilings and compatibility, Gen I/II encounter/history evidence, extensive Gen III event/PID/IV/GameCube evidence, Gen III→IV transfer evidence, and extensive Gen IV wild/static/trade/event/RNG/form/origin evidence.

This does **not** change the support table above into a blanket legality guarantee. The analysis reports one of:

- **Invalid** when available evidence proves a contradiction;
- **No problems found** when the checks that actually ran find no contradiction;
- **Incomplete** when evidence coverage is insufficient.

The legality lane is analysis-only: no auto-fix, no source writes, no staged mutation, and no write-permission changes.

## Audit / hardening state

The full forensic remediation package from PR #101 was merged into the active PR #92 application lane on 2026-10-05.

Final disposition remains:

- **43 VERIFIED / FIXED**
- **1 DEFERRED WITH JUSTIFICATION — AUDIT-043**
- **0 OPEN**

The frozen forensic checkpoint remains historical evidence and is not rewritten by this integration.

## Source and launch policy

Discovery and launching are separate from write authorization.

Current provider-aware work includes RetroArch, DraStic, melonDS and manual/remembered bindings. Game launch metadata is stored by PokeBank NX rather than written into emulator save directories.

For supported GBA/DS identities, the current application can prefer a matching installed HOME forwarder before emulator fallback. If a ROM/content/installed target cannot be proven, the UI must request an explicit source/content selection or **Link Game File** rather than infer it from a save path.

## Permanent safety policy

```text
installed Switch source       READ ONLY unless separately approved later
RetroArch / emulator source   READ ONLY unless separately approved later
staged workspace              app-owned
live installed save writing   HARD DISABLED
live emulator-source writing  HARD DISABLED
launch permission             DOES NOT GRANT WRITE ACCESS
ambiguous source/content      FAIL CLOSED
cross-game True Move          LOCKED
source injection              LOCKED
```

Malformed or unsupported sources must never be silently repaired, normalized, reassigned, or overwritten.

## October 14 Gen V implementation and source state

The [Gen V experimental app PR #300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) at `58bd564ba479745f6129f57167aeac6616297753` has all six exact-head checks green and publishes a [development NRO](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607). The production-owner acceptance flag is still **false**. The owner's previous Switch run could not see Gen V saves although they exist in the **same emulator directories as Gen IV**. Current source discovery, artwork and assignment fixes have **not yet been accepted on the device**.

Implemented Gen V draft coverage includes strict BW/B2W2 70/74 native checksum blocks, two independently validated save copies, real 24 PC boxes, Trainer/Party, PK5 stage/Create/Review and app-owned working-copy export, immutable original source data, manually chosen/emulator Save Instances, and an active-Switch-profile **Assign Save to This Profile** action. Reopening must validate exact game/title/copy and source content. FAT/exFAT path normalization keeps profile ownership stable across routine emulator-save writes; old content-based links may need one deliberate relink.

The stronger, separately tested [Gen V source-hardening PR #304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) includes additional persisted copy/profile and fair provider budget work; it remains a **stacked draft**, not automatically part of #300.

## October 14 Gen I–IV, touch and launch

[Claude finalization PR #306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) adds verified in-NRO RomFS packaging (10 regions, 20 trainers, game art), fair Gen IV multi-provider discovery and ROM header title verification before game launching. At the newer checked HEAD `3ac0ae29`, the Gen I–IV hardware candidate, Product UI native, Gen IV, native and touch checks passed. Full host testing was still running; owner device acceptance remains pending. The Gen IV native capture Ball `max(0x83,0x86)` correction is independently tracked in [#305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305). Full touch-only workflow parity and Items/exit navigation require fresh tests.

## Legality limits

Gen I–IV evidence-aware legality is a separate five-lane research/implementation effort ([#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), [Claude #303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303)); later-generation envelope/domain groundwork is [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301). Neither original-game provenance coverage nor full PKHeX historical parity is complete; no production source-save write permission is implied.

See the [cross-lane release-gate snapshot](ENGINEERING_SNAPSHOT_20261014.md).
