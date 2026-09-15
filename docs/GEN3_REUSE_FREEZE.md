# Issue #71 — Pre-Gen-III reuse freeze

Status: **AUDIT FREEZE — NO GEN III IMPLEMENTATION AUTHORIZED**

This freeze follows the corpus, external-capability, current-tree, subsystem-gap, contradiction and reverse-sanity passes required by issue #71.

## Product integration rule

One PokeBank NX UX sits above PokeBank-owned interfaces/capabilities/safety. Proven external implementations are reused, adapted, or used as oracles/references underneath when license/runtime constraints allow. Do not duplicate solved work merely to make it PokeBank-native, and do not expose a patchwork of upstream UIs.

## Universal field state contract

Every exact-game editor provider must report each semantic field as exactly one primary state:

```text
HIDDEN      field does not exist / is not meaningful for this exact format
DERIVED     value is calculated from native fields and is not an independent stored field
READ_ONLY   native/displayable, but this adapter/milestone does not safely mutate it
EDITABLE    staged adapter can validate, serialize and reparse the field safely
```

A derived concept can still have a separate semantic action. Example: Gen I/II Shiny remains `DERIVED` from native DVs even if a `Make Shiny` action is available that safely transforms the underlying native DVs. Do not mark a derived value `EDITABLE` merely to expose an action; field storage truth and action capability are separate.

Unsupported/unknown data remains HIDDEN or READ_ONLY as appropriate. Never synthesize a fake default simply to fill the shared UI.

## Gen III delta — architecture only

Gen III must be expressed as capabilities layered onto the accepted shared editor, not as a new editor shell.

Expected native/delta concepts to map through the provider include:

- six IVs instead of Gen I/II DVs;
- six EVs instead of Stat Exp;
- Nature (native relationship to PID; exact editing policy deferred to Gen III milestone);
- Ability / ability-slot behavior;
- Held Item;
- Friendship;
- language;
- OT/TID/SID;
- PID and PID-derived relationships such as shiny/gender/nature where applicable;
- Ball and richer origin/met fields;
- Gen III ribbons where applicable;
- exact RSE vs FRLG game capability differences.

The current read-only Gen III adapter already exposes a useful subset: species, PID, TID/SID, EXP, held item, moves/PP, six IVs, six EVs, nickname, OT, original bytes and checksum/round-trip state. Missing shared presentation/edit concepts must be added through a PokeBank provider, not by exposing PKSM-Core objects.

Exact PID/nature/ability/gender/shiny mutation coupling remains **UNRESOLVED until the separately authorized Gen III implementation pass** and must be validated against pinned native/oracle behavior rather than guessed in P0.

## MUST REUSE

- `PokemonEditorFoundationContract` Create/Edit/View lifecycle and staged/read-only semantics.
- `SharedPokemonEditorContract` action model, three-panel workspace, geometry, focus/navigation and capability-driven field/action selection.
- `SharedSpeciesPicker` rather than a Gen III copy.
- accepted Gen I/II filtered-move UX: normal picker offers Empty + exact-game-compatible choices; unusual existing source moves remain preservable.
- existing radar/presentation geometry while allowing generation-specific stat schemas.
- `ClassicInventoryCatalog` as the exact classic-game item/pocket catalog.
- existing Gen III read-only adapter boundary (`PKSMGen3Adapter` / native slice); do not replace it with direct PKSM objects in UI.
- inherited PKSE Move/Multi **interaction model**, via a future PokeBank-owned staged storage controller; do not directly reuse its legacy mutation routine.
- exact game identity and source-discovery boundaries.
- `SourceMutationPolicy`, write-policy tests, staged export/verification, immutable source rules.
- provenance distinction between original source/origin and current/staged location.
- legality state where `UNKNOWN` / `Not checked` is truthful.

## MUST EXTEND IN PLACE — P0 BEFORE GEN III

1. **Exact-format editor provider descriptor** above generation-native editors. It must provide game identity, UI field states/capabilities, storage semantics, stat-presentation schema, move-compatibility hook and read-only normalized presentation data without owning serialization.
2. **Field state vocabulary** (`HIDDEN`, `DERIVED`, `READ_ONLY`, `EDITABLE`) plus separate semantic-action capabilities.
3. **Storage semantics descriptor** so shared UI can say `PackedNative` for Gen I/II and avoid assuming future formats are identical. Do not change accepted packed behavior.
4. **Stat presentation schema** that describes labels/count/stored-vs-derived concepts. It must not translate DV↔IV or Stat Exp↔EV into one fake structure.
5. **Move compatibility provider contract** keyed by exact game/species/form/move, with an explicit preservation result for already-present unusual moves. Existing Gen I/II providers remain implementations/oracles; no universal giant table in UI.
6. **Trainer field descriptor** for support/editability of name, TID, SID, gender, money and future exact-game fields. Descriptor only; no Gen III trainer mutation in P0.
7. **Capability vocabulary audit/extension** only where a concept is needed by the shared UI (for example Ball, Language, Marks/collection availability, SID/met/origin distinctions). Unsupported remains absent; no fake defaults.
8. **Bridge save-operation rights to UI separately**. `SaveEdit::Capabilities` and `SharedEditor::Capabilities` remain separate layers; add an explicit adapter bridge rather than merging their meanings.
9. **Storage-operation descriptor** sufficient to host inherited Move/Multi concepts later without routing legacy `TrainerViewScreen` mutation directly.
10. **Freeze contracts/tests** proving accepted Gen I/II routes still use the same semantics and all original-source mutation remains denied.

## FORBIDDEN DUPLICATION

- a separate Gen III Pokémon editor shell;
- a Gen III species picker clone;
- a Gen III action-sheet/navigation/focus clone;
- a second Gen III item/pocket catalog;
- a second box Move/Multi implementation;
- direct PKSM-Core concrete types in shared UI/business code;
- copying the legacy Gen II 28-row/prompt wizard as a Gen III template;
- a generic cross-generation Pokémon edit struct that pretends PK1/PK2/PK3 storage semantics are identical;
- a second source-write/export pipeline.

## PROTECTED SAFETY SURFACES — DO NOT TOUCH DURING P0

- `SourceMutationPolicy` deny-by-default behavior;
- live RetroArch writes HARD DISABLED;
- live installed-game writes HARD DISABLED;
- live other-emulator writes HARD DISABLED;
- exact accepted Gen I runtime/editor behavior;
- exact accepted Gen II runtime/editor behavior;
- Gen I and Gen II packed native box semantics;
- accepted physical milestone SHA/tree as historical evidence;
- current Gen III read-only product behavior.

## BLOCKED UNTIL LATER MILESTONE

- Gen III Pokémon Create/Edit product routing;
- Gen III boxed Pokémon mutation/serialization;
- Gen III trainer mutation;
- party mutation;
- live save writes;
- legality/autolegal claims beyond truthful read-only status;
- transfer/conversion execution;
- Vault/Banks redesign under unrelated issues.

## Reverse sanity audit — planned feature to source decision

| Planned surface | Source/reuse decision |
|---|---|
| Create/Edit/View | shared foundation contract — reuse |
| Action sheet | shared editor contract — reuse |
| Species picker | shared picker — reuse |
| Move picker | accepted UX + provider interface — extend |
| Stats/radar | shared drawing + exact stat schema — extend |
| Held item/item picker | ClassicInventoryCatalog + exact Pokémon adapter — reuse/adapt |
| Nature/Ability/IV/EV | exact-format provider over Gen III read model — later product wiring |
| Trainer | field descriptor — P0; mutation later |
| Boxes | explicit storage semantics — P0 |
| Move/Multi | inherited PKSE state machine — refactor later behind staged storage provider |
| Party | read-only presentation capabilities — reuse; mutation blocked |
| Provenance | PokeBank provenance service model — reuse |
| Legality | PKHeX/oracles later; `UNKNOWN` remains real — reference/backend |
| Serialization | generation-native staged adapter — do not genericize |
| Source write | denied by policy — protected |

## Freeze condition

Issue #71 architecture/reuse planning is complete enough to begin **P0 foundation-only work**. P0 is not Gen III implementation. After P0, rerun accepted Gen I/II host/native gates before any Gen III product route is opened.
