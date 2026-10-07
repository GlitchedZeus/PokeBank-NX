# Generation I Move Compatibility Reference

Status: pinned reference for the normal Gen I Create/Edit move picker. This is deliberately narrower than full encounter legality.

## Pinned upstream

PKHeX repository commit:

`77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5`

Relevant upstream inputs:

- `PKHeX.Core/PersonalInfo/PersonalTable.cs`
- `PKHeX.Core/PersonalInfo/Info/PersonalInfo1.cs`
- `PKHeX.Core/Legality/LearnSource/Sources/LearnSource1RB.cs`
- `PKHeX.Core/Legality/LearnSource/Sources/LearnSource1YW.cs`
- `PKHeX.Core/Resources/byte/personal/personal_rb`
- `PKHeX.Core/Resources/byte/personal/personal_y`
- `PKHeX.Core/Resources/byte/levelup/lvlmove_rb.pkl`
- `PKHeX.Core/Resources/byte/levelup/lvlmove_y.pkl`
- `PKHeX.Core/Resources/byte/evolve/evos_g1.pkl`

The compact table in `src/Integration/Gen1/Gen1MoveCompatibilityData.inc` was generated from that pinned data and is committed with permanent R/B/Y fixtures.

## Supported question

`MoveCompatibility` answers only:

> Can this species obtain this move in this Red/Blue/Yellow ruleset through the pinned normal Gen I initial/level-up/TM/HM model, including proven Gen I pre-evolution inheritance?

It is used for the kid-safe normal Create/Edit picker and for blocking a newly staged Add draft that still contains a known-incompatible move.

## Explicitly not claimed

This model is not the full encounter-legality engine. It does not claim complete coverage for event distributions, Stadium-only special cases, later-generation tradeback states, or every provenance-dependent edge case.

The UI must therefore keep these concepts separate:

- `Move compatibility: OK` may be shown when the four selected moves pass this model.
- `Encounter legality: Not checked yet` remains truthful until the separate legality milestone exists.
- Do not label a Pokemon simply `Legal` because its moves pass this table.

## Preservation rule

Existing source Pokemon are not normalized through this table. A hacked/unusual source move remains visible and byte-preserved unless the user intentionally edits that move field. The filtered picker applies to choosing a replacement/new move, not to silently rewriting source history.

## Regression fixtures

At the pinned reference:

- Charizard + Flamethrower: compatible in Red, Blue, Yellow.
- Charizard + Hydro Pump: incompatible in Red, Blue, Yellow.
- Charizard + Waterfall: incompatible in Red, Blue, Yellow.
- Charizard + Fly: incompatible in Red/Blue, compatible in Yellow.

These differences are intentional proof that the model is game-aware rather than type-based or modern-learnset-based.
