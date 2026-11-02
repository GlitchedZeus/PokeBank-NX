# PR #122 / #163 / #280: behaviour and test comparison against main

Date: 2026-11-02. Baselines: `origin/main` = `9ce7f737` (Merge #330), active Gen I–V repair
branch `fix/round13-gen15-repairs` = `a5dcea57`. PR heads were checked against the GitHub API:
#122 `36c0d1d6`, #163 `79679950`, #280 `dcd855a7`. All three are still open. None of them was
closed, retitled or merged as part of this review.

How the review was done:

* Unique commits: `git log origin/main..<head>`. Changed files: `git diff $(merge-base) <head>`,
  then each file compared with its main version.
* Tests: every line the PR adds to a test was checked for a verbatim match in main's test tree.
  Lines with no match were classified by hand as covered, covered differently, or MISSING.
* The PR test files were also run against a scratch worktree of `origin/main`:
  * C++ tests: the PR's file was built with the `Makefile.host*` rules.
  * Python contracts: run through a tolerant runner that records every failing statement and token
    instead of stopping at the first one. The runner was first checked against main's own copies of
    the three touch contracts, and reported 0 failures on each.

## Verdicts

| PR | Verdict |
|----|---------|
| #280 empty-slot tap/A parity (9 commits) | **SUPERSEDED AFTER PORT** |
| #163 G5-01 read-only PK5 foundation (85 commits) | **SUPERSEDED** (safe to archive-tag + close) |
| #122 complete touch-control parity (391 commits incl. 26 main-forward merges) | **SUPERSEDED AFTER PORT** |

Ported on `r13/prport` (commit `b3f19116`, created from `fix/round13-gen15-repairs`). All ported
assertions pass in normal and `_sanitize` builds. Each one was also checked against a deliberate
mutation of the product code, and each one failed as intended. No regression was found.

Before closing, the owner may want archive tags so the histories stay reachable:

* `archive/pr-122` → `36c0d1d6`
* `archive/pr-163` → `79679950`
* `archive/pr-280` → `dcd855a7`

---

## PR #280: Gen III–IV empty-slot touch/A action parity

### Unique commits

| Feature | Commits | Main equivalent | Status |
|---|---|---|---|
| Gen III empty-cell tap opens the same Actions (Add/Cancel) menu as A | 1529b04a, a64a4439, 5d47d6d9 | `src/UI/Gen3SharedPokemonSurface.inc` `handleInput`. The Boxes tap route assigns the tapped cell, then runs `if (boxEntrySurface(screen)) { beginActions(screen); … }` with no occupancy gate, and restores the cursor when blocked (main L2690–2711, r13 L2758–2780). | Superseded by a better design (see below) |
| Gen IV empty-cell tap opens the same menu as A | 3a1f74b6, db4ebad9, e02995b2, dcd855a7 | `src/UI/Gen4SharedPokemonSurface.inc` `handleInput`. `selectsTappedBoxSlot` → assign → `boxEntrySurface` guard → `beginActions`, with no occupancy gate (main L2364–2386). | Equivalent, plus a full modal guard and cursor restore |
| Gen IV valid-tap bounding, no occupied-only gating | 2f5cfa56 | Same route. `Gen4BoxTouchActions::selectsTappedBoxSlot(tapped, slots)` | Equivalent. The route-scoped assertion was **ported** |
| Cross-generation Python contract | 0c75d061 | `tests/test_touch_appwide_surface_contract.py` "Empty box cells must enter the same guarded action popup as A" loop (main L217–226) | Covered |

The intentional design difference is in Gen III. #280 kept `if (!editor(screen)) return false;`
in the tap route, which sent a tap with no staged editor back to the base View, while A still went
through `boxActivation`. On main and r13, a tap and A both open the Gen III sheet whatever the
editor state. That is strict tap/A parity, and the sheet reports when the editor is unavailable.

Also intentional: #280's "X stays a direct Add shortcut" was replaced in Round 11 (X-IMPEXP,
owner request). On r13, X on a box cell means Export/Import to file, and Add lives only on the A
sheet (`boxActivation(occupied, A, X, fileCapabilities)`).

### Test assertions

| PR assertion | Main / r13 | Status |
|---|---|---|
| `static_assert(boxActivation(false,true,false)==Actions)` ×5 set (gen3 + gen4 tests) | main `tests/test_gen3_hardware_surface_contract.cpp:36-40`, `tests/test_gen4_hardware_surface_contract.cpp:69-73`, `tests/test_shared_pokemon_editor_contract.cpp:110-113`. r13 has the 4-argument X-IMPEXP form at gen3 :38-43 and gen4 :71-76 | Covered (X row covered differently on r13, by design) |
| Gen IV box-tap route contains `beginActions`, no `if (selectedOccupied(screen))` | main gen4 test :470, :474-479 (`routeBetween`) | Covered |
| Gen IV box-tap route contains `selectsTappedBoxSlot` (bounding inside the route) | main only checks it file-wide (`contains`, gen4 test :464). No main test checked it inside the route or before `beginActions` | **MISSING → ported** |
| Python Gen III tap slice: `beginActions`, no occupancy gate | main appwide :217-226 and gen3 test :108-113 | Covered |
| Python Gen III tap slice keeps `if (!editor(screen)) return false;` | Not on main | Intentionally superseded (strict parity) |
| Python Gen IV tap slice: `beginActions`, no gate, `Gen4BoxTouchActions::selectsTappedBoxSlot` | appwide :205-215 (file-wide token at :211), gen4 test :464 and :474-479 | Covered. The route-scoped part was ported |
| "Tap never creates a Pokémon by itself" (comment-level contract in both surfaces) | main asserts `beginCreate` is absent from the tap route (gen3 :112, gen4 :478). Nothing asserted that the Round 11 X actions (Import/Export) stay off the tap route | **MISSING → ported, adapted** |

Running #280's added assertions against main:

* The static_asserts compile and pass.
* The verbatim text anchors (`if (touch.justTapped()) {`, `if (boxEntry && touch.justTapped())`)
  no longer exist on main because the code was reworked.
* The same slices re-run with main's anchors all pass. The one exception is the read-only fallback
  above, which was dropped by design.
* Running #280's full test files fails on unrelated drift since 2026-10-11, for example
  `ClassicInventoryOverlay` tokens and Gen IV static_assert tables.

### Unique improvements main lacks

None in product code. #280 adds no data or research docs.

### Ported (r13/prport `b3f19116`)

* `tests/test_gen3_hardware_surface_contract.cpp`, Boxes tap route:
  * `if (tapped >= 0 && tapped < slots) {` comes before `screen.selectedItemIndex = tapped;`,
    which comes before `beginActions(screen);`.
  * The route contains no `PokemonFileMenus::`, `beginImport` or `exportPokemon`.
* `tests/test_gen4_hardware_surface_contract.cpp`, Boxes tap route:
  * `PokeBank::UIModel::Gen4BoxTouchActions::selectsTappedBoxSlot(tapped, slots)` comes before
    the assignment, which comes before `beginActions`.
  * Same file-action exclusions as Gen III.

Both are already registered in `Makefile.host.editor` and `Makefile.host.gen4` with `_sanitize`
targets. Both pass.

---

## PR #163: G5-01 read-only PK5 format foundation

### Unique commits (85), by feature

| Feature | Representative commits | Main equivalent | Status |
|---|---|---|---|
| PK5 record and crypto foundation (checksums, 32 shuffles, quarantine) | 1a71dd39, 0da22e65, b7ab33d1 | `include/Pokemon/Pokemon5ReadOnly.h`, `Pokemon5ReadOnlyView.h`, `include/Encryption/Encryption5.h` (identical) | Superseded (superset) |
| BW/B2W2 SAV5 checksum partitions, backup geometry, copy selection | da7e2eba, 6d95e827, 8c385173, 8b1b18f4 | `include/Integration/Gen5/Gen5SaveBlockLayout.h` (identical), `Gen5ReadOnlySave.h` (+60/-2) | Superseded |
| Read-only discovery, fingerprints, physical identity, SD-root and traversal rejection | f675f3fc, 9cc57bd3, 6f74dcbf, 6a1d8586 | `src/Integration/Gen5/Gen5SourceDiscovery.cpp` (+418/-56), `Gen5AssignedSource.*`, plus new `Gen5SourceSelection.h` and `Gen5SourcePathIdentity.h` | Superseded |
| Save Instances adapter, profile catalog, alias isolation, trainer-name validation | d2f84eab, f9fbe1fa, ccaac567 | `Gen5SaveInstanceAdapter.h`, `Gen5GameSourceCatalog.h`, `Gen5TrainerName.h` | Superseded |
| Staged PK5 workspace and four-field shared editor (View/Edit/Keep/Review/exit guards, atomic presentation) | 03a9846c … 4893f713 | `Gen5StagedPokemonWorkspace.h` (+464), `Gen5StagedPokemonRecord.h` (+395), `Gen5SharedScreenState.h` (+604), `Gen5SharedPokemonSession.h`, `src/UI/Gen5SharedReadOnlySurface.inc` (2308 lines, using the shared `SharedPokemonEditorSurface.inc` with Gen IV) | Superseded by the full Gen V shared editor (all fields, Create/Clone/Export/Import) |
| Independently verified per-field review | 1933ae7c, 381a9b5c | `Gen5SharedReview.h` (+215) plus shared `SharedEditorChrome::drawReviewPanel` | Superseded |
| App-memory SAV5 export candidate and safety matrix | 6d22f82d, 76b80260, 37a72dd3 | `Gen5StagedSaveCandidate.h` (+135/-5) | Superseded (superset) |
| Game identities, Games card and preview routing | 0eb28001, e56c9542, bfe84cd7, f7c5aabb | `src/Games/GameIdentity.cpp`, `Gen5GameCardPreview.h`, `SaveSelectScreen.cpp`, `UI.cpp handleGen5View` | Superseded |
| CI: gen5-focused-host and native NRO gates | fd71197b, c10b1714, e90ce6dc, 79679950 | `.github/workflows/gen5-focused-host.yml` (+13/-2), `product-ui-native.yml`, `Makefile.host.gen5` (441 lines vs 85) | Superseded |

All 44 files the PR touches exist on main. Four are byte-identical: `docs/GEN5_DEVELOPMENT.md`,
`Encryption5.h`, `Gen5SaveBlockLayout.h` and `ExactSaveCapabilities.h`. Every other file is a
main-side superset.

### Test assertions

The PR adds or changes `tests/test_gen5_pk5_readonly.cpp`, `test_gen5_readonly_save.cpp`,
`test_gen5_staged_save_candidate.cpp`, `test_gen5_readonly_ui_contract.py` and
`test_game_identity.cpp`. Main's copies of all five are supersets (+289, +1309, +359, +1128 and +3
lines). The only lines main removed or changed are listed here:

| PR assertion | Main | Status |
|---|---|---|
| `Species/Ability/HeldItem/Shiny` are not editor targets; `PokemonCreation` unsupported; `fieldAccess(Species)!=Editable`; `actions.count==4` / stale `{Review, Close}` | `test_gen5_readonly_save.cpp` asserts these ARE editable or supported, `count==7` with Clone/Legality/Export, and stale empty → Add/Import/Review/Cancel | Covered differently: read-only limits deliberately lifted by the Gen V editor |
| `reopenValidatedSource(first)` not ready ("no automatic guessing") | Now ready: an explicitly chosen copy is preserved, Primary/Backup mismatch is still rejected | Covered differently (intentional) |
| `sourceAliases[0]==probe.sourceIdentity` | `sourceIdentity==probe…`, `sourceAliases[0]==samePhysical…` | Covered differently |
| `boxes[0][0]->level()==0` | `level()==1`, plus real base and display stats (owner finding 35cbf163) | Covered differently (bug fix) |
| `ui.adjustField(±1)` | `ui.editDraft().stage(GField::Nature,…)` | Covered differently (API replaced) |
| `!Gen5ReadOnlySave::parse(empty)` | `!parse(empty,{},&err)` plus no invented "unverified" hint | Covered (stronger) |
| PK5 fixture `form()==2` for Pikachu | form 0 (Pikachu has no Gen V form 2; main rejects invalid native forms) | Covered differently (fixture corrected) |
| `games.size()==32`; Gen V card artwork empty | 42 (ten localized FRLG Switch releases); pinned retail artwork | Covered differently (catalog grew) |
| UI contract tokens `drawVerifiedSummary`, `(void)touch;`, `trainerScreen.detailViewActive=true;`, `reopenValidatedSource(chosen)` | Shared `drawSharedWorkspace`, real touch, the loaded-game hub (fa9cfcea), `assignValidatedSource` | Covered differently (intentional) |

Running #163's own test files against main:

| PR test file | Result on main |
|---|---|
| `test_gen5_staged_save_candidate` | **PASS** unchanged |
| `test_gen5_pk5_readonly` | Fails only because its fixture Pikachu has an invalid form 2. With that one fixture byte corrected (form 0, also `form()==0`), the PR's whole PK5 file **PASSES** on main |
| `test_gen5_readonly_save` | Does not build: 7 errors, all from the removed `adjustField` API. Main's evolved copy covers every scenario |
| `test_game_identity` | Fails only on `size()==32` (now 42) |
| UI contract (`.py`) | Fails only on replaced read-only tokens |

Main's own copies of all five pass.

### Unique improvements main lacks

None. The data and research doc `docs/GEN5_DEVELOPMENT.md` is byte-identical on main. Nothing
needs porting.

---

## PR #122: UI complete touch-control parity

### Unique commits (391; 26 merges of main forward), by feature

| Feature (≈commit count) | Main equivalent | Status |
|---|---|---|
| Touch core: release-confirmed taps, slop, drag classification, live gesture snapshot, `TouchScrollState` direction lock (~100) | `include/UI/TouchInput.h`, `TouchGesture.h`, `TouchScroll.h`, `src/UI/TouchInput.cpp`, `PKSEFramebufferTouchTransform.cpp`. All byte-identical to #122. `tests/test_touch_scroll_state.cpp` and `tests/touch_fakes/UI/TouchInput.h` are identical too | Equivalent (landed via #208/#267/#271) |
| Touch-down focus (`touchedButtonDownId`) in Gen I/II/III/IV editors and pickers | Main deliberately removed it in favour of focus on release. Main tests assert its absence: `test_touch_pokemon_editor_direct_rows_contract.py:53,108,129,135,165`, `test_touch_appwide_surface_contract.py:260,584`, `test_touch_modal_list_contract.py:99` | Superseded (better: a swipe never moves focus) |
| Per-surface Review list finger scrolling (Classic Inventory, Gen I, Gen II, Gen III `reviewScroll` / `reviewTouchScroll` / `reviewViewport`) | One shared paged Review (`TrainerViewScreen::openStagedReview`, `SharedEditorChrome::drawReviewPanel`), with swipe paging and vertical-intent gating in `include/UI/SharedReviewTouch.h::swipePage`. Tested in `test_gen12_review_parity.cpp`, `test_classic_party_review_path.cpp` and the appwide/modal contracts | Superseded |
| Picker viewport ownership, off-screen "brought into view" confirmation, fingertip rows, hit clipping (Gen I–IV, Classic Inventory, Gen II Location/Move/Held) | `TouchPickerViewport` in `ClassicInventoryOverlay.cpp`, `Gen1PokemonEditorOverlayUXCleanup3.inc`, `Gen2PokemonEditorFoundation.inc` (`itemScroll`/`itemViewport`), `Gen2PokemonPickerOverlay.inc` (`locationViewport`, "Location brought into view"), `Gen2HardwarePickerFix.inc`, `Gen3SharedPokemonSurface.inc`, `SharedPokemonEditorSurface.inc` (Gen IV/V), `SharedHeldItemInput` | Equivalent / superseded (shared helpers) |
| Gen II picker Left/Right/L/R ±10 (`stepList(±10)`) | `SharedPickerPaging::pagedRow` (Up/Down wrap, Left/L/Right/R ±10, ZL/ZR ±50) in `Gen2PokemonPickerOverlay.inc` | Superseded |
| Live details visual offset (`liveVerticalListVisual` in Gen II/III/IV details) | `detailsTouchScroll.offset()` (Gen II `Gen2HardwareFinalFix.inc:132`), `SharedPokemonShell::drawScrollableDetails` (Gen III/IV/V). `liveVerticalListVisual` is still used in the GSC/Picker/Details modals | Superseded |
| Gen II details/review vertical-intent gating | Review: `Gen2HardwareWorkspaceFix.inc:275-276`. Details: `TouchScrollState` direction lock (identical core) | Equivalent |
| Classic action sheets with direct row hitboxes (ids 300/500, 31600, 41600) | Shared `drawNarrowPokemonActions` / `ScreenChrome` narrow popup (enabled-only touch ids), Gen II/III 31600 routes | Superseded |
| Quick Games drawer swipe (`g_quickGamesDrawerSwipe` in ScreenChrome) | `include/UI/QuickGamesDrawerMotion.h` (physical edge pull, settle, vertical lock) | Superseded |
| Product Home / Games / Profile / Backup lists follow the finger | `SaveSelectScreen.cpp` (+2000), `BackupSelectionScreen.cpp`. Covered by main's appwide "Product Home" and "Backup selection" families | Equivalent |
| Held Item "Showing a–b / n" caption while scrolling | Main computes `firstVisibleItem`/`lastVisibleItem` but marks them `(void)` and does not draw them, after the held-item search redesign (`Gen3SharedPokemonSurface.inc:2038-2041`, `SharedPokemonEditorSurface.inc:942-945`) | Intentionally dropped (presentation choice, not a behaviour gap) |
| CI: touch-input-gating and native-PR workflows | All six workflow files exist on main as supersets | Superseded |

All 60 files #122 touches exist on main. Nine are byte-identical, including the whole touch core and `tests/test_touch_scroll_state.cpp`.

### Test assertions

* **Matched verbatim in main's test tree:** `test_ci_repository_safety_contract.py` 8/8,
  `test_gen1_pokemon_editor_ui_contract.cpp` 1/1, `test_inventory_ui_contract.cpp` 3/3,
  `test_touch_scroll_state.cpp` 253/253 (identical file), `touch_fakes/UI/TouchInput.h` 50/50.
  Also matched: 479/526 lines of `test_touch_appwide_surface_contract.py`, 151/157 of
  `test_touch_modal_list_contract.py`, 124/154 of `test_touch_pokemon_editor_direct_rows_contract.py`,
  9/16 of `test_gsc_editor_surface_contract.cpp`, 15/18 of `test_gsc_workspace_render.cpp` and
  2/6 of `test_gen3_hardware_surface_contract.cpp`.
* **Tolerant run of #122's Python contracts on main:** 37, 6 and 31 failing tokens or statements.
  Every one maps to a row in the feature table above: superseded idioms (shared review, release
  focus, shared paging and picker helpers, drawer motion). None is a missing behaviour.
* **#122's C++ tests built against main:**
  * `test_touch_scroll_state` **PASS**.
  * `test_inventory_ui_contract` **PASS**.
  * `test_gsc_editor_surface_contract` stops on the replaced `stepList(-10)` token.
  * `test_gen3_hardware_surface_contract` and `test_gen1_pokemon_editor_ui_contract` stop on
    drifted tokens.
  * `test_gsc_workspace_render` does not build because the `drawList` signature changed.
* **The one assertion with no main equivalent:** `test_gsc_workspace_render.cpp`
  `assert(!clipActive)` after `Gen2HeldItemPickerPresentation::drawList`. Clipping must be balanced
  after each draw. Main's `SharedHeldItemPickerPresentation::drawGrid` does
  `setClipRect … clearClip`, but no test asserted it. **MISSING → ported** (mutation-checked:
  removing `fb.clearClip()` now fails the test).

### Unique improvements main lacks

None that need integrating. The only product-visible item #122 has and main does not is the Held
Item range caption, which main dropped on purpose. #122 adds no data or research docs.

---

## Commands to reproduce

```sh
git fetch origin pull/122/head:refs/remotes/pr/122   # likewise 163, 280
git log --oneline origin/main..pr/280
git diff $(git merge-base origin/main pr/163) pr/163 --stat
git diff pr/163 origin/main -- tests/                # main-side supersets
# on r13/prport:
make -f Makefile.host build-host/test_gen3_hardware_surface_contract{,_sanitize} \
  build-host/test_gen4_hardware_surface_contract{,_sanitize} \
  build-host/test_gsc_workspace_render{,_sanitize}
```
