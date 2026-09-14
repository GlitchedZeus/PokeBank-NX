# Gen I summary and radar hardware correction

PR #66: OPEN / DRAFT / DO NOT MERGE. Issue #62: OPEN. Device accepted: NO.

## Root causes and changes

1. `Pokemon1ReadOnly::isShiny` returned false unconditionally. It now delegates to `ShinyDVs::isShiny` with the record's four stored DVs. Existing sprite consumers inherit the correction without another shiny implementation.
2. `drawBoxSummaryPanel` rendered modern neutral getters unconditionally. The exact PK1 wrapper tag now selects the same Gen I presentation/capabilities as the permanent editor. No Nature, Ability, Held Item or other modern-only row is rendered for PK1. Type badges use the same native type source as the editor.
3. The summary read six stat getters; boxed PK1 records have no stored calculated stats, so those getters deliberately return zero. They remain untouched for compatibility. The summary now obtains the editor's actual five calculated stats through `presentGen1Pokemon`, backed by the existing `calculateBattleStats` helper.
4. The foundation drew a full-width radar and text before a later rectangle covered only part of that region. Its `statusY + 278` text extended past the cover. That old draw is removed; the split renderer is authoritative.
5. The supplemental pane remains on the left; the radar receives more width and height on the right. The production renderer measures all five raw-value labels and places them around HP/Atk/Def/Spe/Spc vertices. A recording-framebuffer regression checks label overlap and geometry bounds.
6. A shared presentation-only scale uses a common denominator and rounded local headroom buckets. The raw values and their ratios are unchanged. `Scale N` means the outer ring represents N. The radius responds to actual integer stat changes; a DV change that rounds to the same battle stat cannot truthfully move that axis. Larger uniform changes can cross a scale bucket, so graph size is not an absolute cross-level comparison.

## Preserved behavior

Species and move picker input/drawing ownership is unchanged. Calculated Stat cells remain read-only and unfocusable, with DV → Stat Exp → Moves navigation. Staging, Clone, Pending Changes, export-copy behavior, move compatibility and weird-source preservation remain. The shared legacy radar helper uses the same new scale; reference overlay implementations remain compiled.

Original save bytes are immutable. Live RetroArch, installed-game and other-emulator writes remain HARD DISABLED. Party editing is deferred. This is not Gen III serialization work.

## Exact replacement test

Use only the replacement whose application SHA and NRO hash match its CI manifest. The previous `5adec3ab` / `1c5cdbbd44e8ec0c20b56e3de18723d4de8a78a2d846f091387f6bb2d5e40ce0` NRO is not this replacement.

1. Create or make shiny Bulbasaur; confirm shiny artwork in editor, box thumbnail and large Summary.
2. Confirm Summary hides Nature/Ability/Held Item and uses the editor's real HP/Atk/Def/Spe/Spc values and populated five-axis graph.
3. Confirm the editor radar is larger; labels and values fit, with no duplicate or stray Spe/Special text.
4. Change DVs, Stat Exp and level; compare raw values and graph changes, observing the labeled local scale.
5. Confirm calculated Stat cells cannot receive focus, including returning left from Moves.
6. Open Species directly with A; browse, select and cancel. Confirm sprite/types/stats/move compatibility update and cancel retains the committed species.
7. Open Level/EXP with A; only its dialog appears. Back out and confirm no stacked or hidden picker.
8. Confirm the original source save is untouched.

STOP after delivery. Await the owner's physical Switch result before acceptance or merge.
