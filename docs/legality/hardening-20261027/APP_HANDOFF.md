# Application integration handoff to Claude

Application snapshot: PR307 `d6cc878a9652b78a6cdfd8a31cdb5797172d6c9d`. Legality implementation checkpoint: PR314 `0a9ad2073b2ab678e5213b63b6fab60925e23172`. Both PRs remain draft, with their original bases. No PR307 source/branch, source save or Pokémon was modified. This is a reviewed-interface proposal and host boundary evidence, **not an integrated build or device acceptance**.

## Gen V boundary ready for selective integration

Import `Gen5AppAdapter.{h,cpp}`, `Gen5WorkspaceAdapter.h`, `ReportPresentation.h` and their reviewed engine dependencies from the legality lane. Keep the app's current native `Gen5StagedPokemonWorkspace`, PK5 decoding and shared chrome.

At the **current** `Gen5SharedReadOnlySurface.inc::drawLegality5`, evaluate:

```cpp
const auto checked = Legality::Gen5App::analyzeWorkspace(workspace, state.target());
const auto verdict = checked.report.verdict();
```

The adapter reads current staged party/box bytes and `workspace.trainerPresentation()`, derives OriginalSlot/StagedSlot from the actual selected record, and returns the checked PID/species/TID/SID/checksum/origin/language/party identity. It does not fall back to an original trainer after a staged trainer decode failure. Unknown title, unknown slot, malformed trainer or unknown provenance leaves a nonempty report Incomplete. Contradictory136/220-byte slot labels cannot invent a box-only Invalid. Bad records retain Invalid when context is also missing.

Recompute when the panel is opened or its selected record/trainer changes. Do not cache using only species/slot, or display another record's report after a stage/discard/navigation action. Identity binds the checked bytes; it is not a distribution receipt. The84-case actual-workspace harness covers allfour exact title IDs, native party/box geometry, used PP, staged invalid item, trainer-only edit, corresponding OT edit, bounds and unchanged original save.

Render `Presentation::headline(checked.report)`, allnine `coverageRows`, and every non-Info issue from `Presentation::lines`. `NoProblemsFound` is the only clean state. `Report::ok()` and zero warning count are insufficient: a Partial coverage area with zero warnings must still display Incomplete/Unknown. Invalid outranks incomplete context.

The current `SharedEditorChrome::drawOriginLegalityPanel` hardcodes “full legality analysis has not run” and shrinks all rows into columns. Extend that **existing shared renderer** with an explicit evaluated-report mode and issue pagination/wrapping, plus a matching session page state/input route. Keep its existing stored-facts mode for callers that have not run an analyzer. Do not truncate report issues to fit the present modal, relabel a placeholder clean, or restore the obsolete private GenV renderer.

`app-presentation.patch` is a smaller **two-file** prerequisite against this exact app snapshot. It fixes the generic modal and R-entry uses of `ok()` and zero-warningIncomplete. It is not the full GenV wiring patch and does not claim to solve the active classic surfaces. Before/after hashes are in `app-presentation.json`. `git apply --check` passes. The older five-file `completion-session2/app-integration.patch` does not apply to this app head and must not be advertised as current.

## Twenty-title call-site audit of the current app snapshot

| Titles | Active panel at d6cc878a | Current state | Required owner integration |
|---|---|---|---|
| Red, Blue, Yellow | Gen1PokemonDetailsPresentation.cpp | NotChecked | Call current selected native record analyzer with exact source and raw regional names; render all report states. |
| Gold, Silver, Crystal | Modals/Gen2PokemonDetailsModal.cpp | NotChecked | Carry Korean/Japanese raw name bytes and native source title; connect report rather than merely structure status. |
| Ruby, Sapphire, Emerald, FireRed, LeafGreen | Gen3SharedPokemonSurface.inc | NotChecked | Use current staged native record and source-owned Gen3 berry evidence. Bank records must not borrow an unrelated open save's berry. |
| Diamond, Pearl, Platinum, HeartGold, SoulSilver | Gen4SharedPokemonSurface.inc | Incomplete placeholder | Preserve exact host title, current bytes and original/box/party context; expose transferred origins and unresolved evidence. |
| Black, White, Black2, White2 | Gen5SharedReadOnlySurface.inc | Incomplete placeholder | Use the current workspace bridge above and shared paged report presentation. |

The legality branch's older GenI–IV call graph is distinct from the current app snapshot's active paths. Generic modal and TrainerView R-entry calls still exist but do not establish that every modern classic surface executes them. Host title-matrix tests establish API dispatch for twenty titles; they do not establish UI deployment or per-title real-world accuracy. Source save identity is absent from the GB differential corpus and inferred/assumed for public records; do not publish per-save-title accuracy for those inputs.

## Isolated merge rehearsal and conflict ownership

A real `git merge-tree --write-tree` rehearsal against d6cc878a reports13 conflicted paths. It writes only temporary merge objects, creates no merge commit and changes no branch. The raw output/tree/stages are preserved in the evidence bundle. No combined build is claimed while conflicts remain.

| Paths | Resolution obligation |
|---|---|
| .gitignore | Retain app-generated asset policy and legality host-evidence exclusions. No history reset. |
| Makefile.host | Union registered legality suites/dependencies and the current app suites; retain all compile/sanitizer flags and assertions. |
| include/Pokemon/Pokemon3FRLG.h; src/Pokemon/Pokemon3FRLG.cpp | Keep current staged/editing behavior and legality raw native/source evidence accessors. Do not reintroduce stale catch-all mutation or silently synthesize source context. |
| src/UI/Gen1PokemonDetailsPresentation.cpp; src/UI/Modals/Gen2PokemonDetailsModal.cpp | Keep current classic layout/navigation; reconnect native analyzer/result rendering explicitly. |
| src/UI/Gen3SharedPokemonSurface.inc; src/UI/Gen4SharedPokemonSurface.inc | Keep current shared workspace UI; reintroduce only reviewed exact-source/berry/report hooks. |
| src/UI/Modals/PokemonDetailsModal.cpp; src/UI/TrainerViewScreenBase.inc | Preserve current staged session ownership and explicit verdict semantics. The two-file patch is a review aid, not a blind merge. |
| tests/test_gen1_pokemon_editor_ui_contract.cpp; tests/test_gen3_hardware_surface_contract.cpp; tests/test_passive_pokemon_view_contract.cpp | Preserve both user-flow guards and legality semantics. Do not weaken either side to obtain green tests. |

Before acceptance, the owner must resolve/rehearse integration, run the combined host and native build, then exercise alltwenty titles on the actual app. Include staged trainer+Pokémon changes, malformed context, zero-warningIncomplete, invalid correlated fields, source selection after navigation, long multi-page issues, Bank ownership, discard/reopen and unchanged original saves. Device rendering, NRO build/deployment and independent reviewer approval are outstanding. No source-write/export authority is added by this handoff.
