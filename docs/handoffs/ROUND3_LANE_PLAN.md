# Round 3 lane plan — Gen I–V hardware repair (owner directive 2026-10-19)

Base: #307 `claude/gen5-integration-20261014` at `0faf7e9c14a90ad771358478a381aaf0b87a0a96`
(code = round-2 candidate `e10f307f`, hardware-tested by the owner). Forward merges only; 2R integrates.
No force-push, no reset, no merge to main / #314 / Gen VI–IX lanes. #307 stays draft.

## Owner's round-2 hardware result on `e10f307f`

Confirmed PASS (now hardware-accepted, must not regress): **B01, B02, B03, B06, S01, S03**.
Round-1 42 PASS stay regression requirements (not all retested on round-2 hardware).
Still NOT VERIFIED (do not classify as broken): B04, E03, E11, L06, S04, S06.

New reports: R2-BUG-001…007, R2-BUG-012, R2-BUG-013 (defects); R2-BUG-008, R2-CHECK-009/010/011
(investigate first — establish native behaviour before changing code).

## Coordinator root-cause notes (source audit of 0faf7e9c)

- **R2-BUG-004 (species change keeps moves).** Gen I `ux3ApplySpeciesPicker`
  (`src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc:178`) deliberately keeps moves
  ("Moves stay intact and are revalidated visibly below"). Other apply paths:
  Gen II `Gen2PokemonEditorFoundation.inc:456` (`state.setSpecies`), Gen III
  `Gen3SharedPokemonSurface.inc` (species picker), Gen IV `Gen4SharedPokemonSurface.inc:748`
  (`working->setSpecies`), Gen V `Gen5SharedReadOnlySurface.inc:647` (`applySpecies`).
  The repo has **no per-level learnset data** for Gen I–V (only learnable-pool bitsets:
  Gen1MoveCompatibility, Gen2 compat, Gen3LearnsetTable, Gen4MoveCompatibility); Pikachu's
  Create default is hard-coded from pret (`Gen1StagedPokemonEditor.h:23`).
- **R2-BUG-002 (Gen III/IV Trainer edit).** `ClassicTrainerEditability.h` returns
  `mutableWorkspace` for every group except RBY/GSC; Gen III/IV have staged Pokémon and inventory
  editors but **no staged Trainer editor**, so Name/Money edits have no export path.

## Lanes and exact ownership

| Lane | Session | Branch | Owns |
| --- | --- | --- | --- |
| R | session_01LE7d2kdkMRGYK7Er3cmZrg | `claude/reroll-mechanics-20261017` | **R2-BUG-004** end to end: new shared `SpeciesMoveDefaults` (data generated from pret pokered/pokeyellow, pokegold/pokecrystal, pokeruby/pokeemerald/pokefirered level-up tables; PKHeX lvlmove resources for DPPt/HGSS/BW/B2W2 — pinned revisions, generator script in `tools/`), the shared species-change policy, and ONE call per apply function in the five apply paths above. Tests across all 20 games. |
| C | session_012Qk8aPZvVGKZhMUyUC6rya | `claude/gen14-campaign-20261015` | **R2-BUG-002** (Gen III/IV staged Trainer Name/Money editors + export, 10 games), **R2-BUG-001** (Trainer "A: Edit" footer, Gen I gap, all gens), **R2-CHECK-009** (Gen I Box Actions Move/Review), **R2-BUG-008** (Gen III met location vs level). |
| S | session_019GeN9mKTkgfpfuqDGPzWhz | `claude/shared-editor-gen35-20261016` | **R2-BUG-012** shared boxed-row modal style (Pokémon Actions = reference) applied to every listed interactive popup, all gens; **R2-BUG-003** Gen V Trainer layout; **R2-CHECK-011** Gen V met-location labels; S04 Gen V trace refresh. |
| P | session_01UKYxj88TVKN6B3ScHi7Pmh | `claude/pickers-search-scrollbar-20261017` | **R2-BUG-006** shared compact grid item picker (≈4 columns, rows fitted to 1280×720); **R2-BUG-007** Move-chooser type badges reusing the summary badge renderer; renders R2-BUG-005 details from I's table. |
| I | session_01WwfzZtJWqDsNSrMV866RJE | `claude/item-quantity-policy-20261016` | **R2-BUG-005** data: per-game TM/HM → move table (source-backed) + move PWR/ACC/PP/type lookup API for P; **R2-CHECK-010** Key Item / PC Items pocket staging; **B04** verification. |
| 2R | session_01Gn6iaKx8qUJWbAydeQ6ST6 | `claude/gen5-integration-20261014` (#307) | Integrator; **R2-BUG-013** coloured controller glyphs everywhere (help windows, modal-local text, footer agreement); Part 9 items 3/4/9 (provider labels, launch/exit, Home/Profile) reproduce-before-fix. |
| QA | session_01VXXPj238uqedMPANHRVViz | `audit/app-qa-20261014` | Hardware-accepted gate for the 6 round-2 passes; new contracts (species change resets moves, boxed rows, glyphs, type badges, grid picker, Gen III/IV trainer export); E03/E11/L06/S06 host verification; Part 9 source reproduction audit; round-3 header-dependency build fix onto #307; QA-F9 audit of the one round-3 candidate; RETEST checklist. |

Shared-file rule: the five species-apply functions belong to R this round (C and S do not edit
them). `SharedMenuGeometry.h` / boxed-row renderer belong to S; P consumes them. Glyph renderer
belongs to 2R; S/P/C call it, never re-implement it.

## Integration

Batches onto #307 by 2R after each lane's full gate (CG --candidate 0 failing, py contracts,
GCC+Clang host-test/sanitize/qa from clean build dirs, devkitA64). ONE candidate NRO at the end,
bound in `docs/qa/CANDIDATE_UNDER_AUDIT.env`, QA-F9 exact-binary audit, coordinator reads the log.
