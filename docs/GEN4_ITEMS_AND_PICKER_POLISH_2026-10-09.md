# PokeBank NX — Gen IV Items + Gen I–IV Shared Picker Polish

MAIN / Product UI lane, 2026-10-09. The source of truth is the **live PR #92 branch**, not the SHA recorded in this document. Do not reset or rebase; preserve all newer commits.

## Delivered implementation in this MAIN tranche

- Gen IV native bag decoding for **Diamond/Pearl, Platinum, and HGSS**: eight bounded, CRC-selected General-block pockets. Malformed bag data is quarantined independently of Trainer, Party and Boxes. Item labels and quantities use the existing shared Items screen.
- **Existing-stack quantity staging**: A on an occupied Gen IV item opens a small app-owned quantity draft (1..999). A keeps it to the staged General block; B cancels. Native IDs are immutable. Every accepted quantity change rechecks exact title assignment, refreshes the selected General CRC, reparses the save, rejects unrelated byte differences, refreshes the Trainer/Party/Box/Bag presentation atomically, and rolls back on failure. The original DraStic/melonDS/RetroArch source bytes remain unchanged.
- **Not implemented / still guarded:** creation, removal, moving items between pockets, held item ID mutation by this bag editor, source writing/injection, and persistence across an exited no-write staged session. Do not describe read-only inventory as full Items parity or silently promote source writes.
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

Native sprite recovery is network-dependent in CI and must pass; item creation/removal requires separate proof and consent gates. No Gen V, touch-controls, legality or Master Vault changes belong in this lane.
