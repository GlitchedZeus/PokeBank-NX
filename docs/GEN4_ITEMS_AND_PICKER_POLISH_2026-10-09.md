# PokeBank NX — Gen IV Items + Gen I–IV Shared Picker Polish

MAIN / Product UI lane, 2026-10-09. The source of truth is the **live PR #92 branch**, not the SHA recorded in this document. Do not reset or rebase; preserve all newer commits.

## Delivered implementation in this MAIN tranche

- Gen IV native bag decoding for **Diamond/Pearl, Platinum, and HGSS**: eight bounded, CRC-selected General-block pockets. Malformed bag data is quarantined independently of Trainer, Party and Boxes. Item labels and quantities use the existing shared Items screen.
- **Existing-stack quantity staging**: A on an occupied Gen IV item opens a small app-owned quantity draft (1..999). A keeps it to the staged General block; B cancels. Native IDs are immutable. Every accepted quantity change rechecks exact title assignment, refreshes the selected General CRC, reparses the save, rejects unrelated byte differences, refreshes the Trainer/Party/Box/Bag presentation atomically, and rolls back on failure. The original DraStic/melonDS/RetroArch source bytes remain unchanged.
- **Gen IV Add Item:** X on the native Items screen opens a bounded, paged picker filtered by a **pinned PKHeX Gen IV structural pouch catalog** (DP/Pt/HGSS distinct key and ball lists; unsupported and unreleased IDs absent). Each entry uses the actual named item sprite when the pinned asset exists. A adds one new stack to the app-owned staged image only; the full bag, selected General checksum, exact game identity, every original save byte outside the new slot/checksum, and shared presentation are revalidated with rollback on failure. A cannot replace an existing item ID.
- **Gen IV Remove Item:** Y on an occupied native bag row opens a separate confirmation with its matching item sprite. Y again confirms the staged removal; B cancels; A is never destructive. Native item identity/quantity, the entire resulting bag, the General checksum, and unrelated bytes are all checked; refresh rejection restores previous staged bytes.
- **Not implemented / still guarded:** moving items between pockets, arbitrary item ID retyping, unrestricted story/encounter legality assumptions, source writing/injection, and persistence across an exited no-write staged session. Add/Remove are currently awaiting native and hardware qualification. Do not label Gen IV Items device-accepted until the owner verifies an exact artifact.
- Product source header shows provider, game name, and source read-only status without internal `G4 P... B... T...` counters; native diagnostics remain in the Trainer model/logs.
- Gen IV Trainer ID / Secret ID shown separately; Gen IV PID label/read-only caption no longer jammed together.
- Compact, higher-contrast Language/Ball/Met Location/Nature/etc pickers for Gen III/IV; Gen IV location no longer appends `(current)`. Gen II/I native picker distinctions preserved.
- Shared move data formatter used by **Gen I, II, III and IV**, now adds `| Acc ... | Pwr ... | PP ...` readable separators and uses the generation-correct underlying tables.
- True per-ball icon art and per-held-item PNGs (not decorative substitutes): generated from the same pinned PokeAPI/sprites commit as HOME renders, mapped by canonical exact item name, cached in SpriteManager, drawn in Gen III/IV ball and Gen II/III/IV held-item pickers. Gen I does not have a native held item field. A missing sprite is not replaced with an unrelated item's sprite.
- The permanent RomFS recovery snapshot predates these icons. Current native PR, Product UI Native and Gen IV candidate workflows now regenerate pinned item art **after** copying that snapshot and before asset preflight. Preflight requires 16 matching ball-specific PNGs.

## Source/asset policy

Never commit a personal .dsv or user save, never modify the external source, never force-push or move the accepted hardware checkpoint. Art generator and manifests are tracked; the generated RomFS image is build-time output. The native NRO must contain its images; a preflight pass does not prove physical rendering.

## Verification still required

- Exact-head focused Gen IV host tests, full host regression tests, packed Gen I/II move tests, native PR gate, Product UI Native build and Gen IV candidate gate.
- Physical Switch test of Diamond/Pearl, Platinum, HGSS Items across every populated pocket, empty bags, malformed bags; quantify item count Keep/Cancel/Back; verify source SHA unchanged and staged presentation refresh.
- Gen I–IV moves: check header alignment, readable Acc/Pwr/PP, parity; Gen II/III/IV held-item sprites and Gen III/IV ball sprite identity (each row must show the selected item's **actual** sprite); native language/met-location dialog sizes.
- Header readability, separated Gen IV trainer TID/SID and PID row spacing.
- Keep one integrated device NRO for the next hardware test round. **DEVICE_ACCEPTED=false for every new SHA until the owner confirms that exact artifact.**

## Milestones not yet closed

- Gen IV native **Add/Remove** now exists in the app-owned draft, with separate native and host regressions. Full CI and physical Nintendo Switch validation remain pending. Source-saving/injection and cross-pocket transfer are NOT part of this feature.
- Pouch catalog provenance is PKHeX `66ef5983a5d349efa2dd615f2926a1b1ee97306e`, files `PKHeX.Core/Items/ItemStorage4*.cs` and `PKHeX.Core/Items/Bags/PlayerBag4*.cs`. This is **structural item eligibility**, not a full story legality judgement.
- Native sprite recovery is network-dependent in CI; preflight must pass and physical Switch must confirm distinct correct sprites and contrast.
- Test **X Add (empty and populated pockets), A Keep Count, Y Remove + confirmation, B Cancel, L/R pouch navigation, Back with staged changes, full app exit/discard**, and verify original .sav/.dsv SHA remains unchanged. Check across D/P, Pt, HGSS and other Gens I–III for picker/Items regressions.
- No Gen V, touch-controls, legality or Master Vault changes belong in this lane. `DEVICE_ACCEPTED=false` for new exact-head candidate builds.
