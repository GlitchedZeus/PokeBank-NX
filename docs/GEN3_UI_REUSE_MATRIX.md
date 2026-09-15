# Issue #71 — Gen III shared UI/reuse matrix

Status: **FREEZE CANDIDATE AFTER MULTI-PASS AUDIT**  
Scope: architecture/reuse planning only; **no Gen III product implementation**.

PokeBank NX presents one coherent product. Proven backends/references sit behind PokeBank-owned interfaces, capabilities, provenance, validation and safety boundaries.

| Shared component | Current owner | Generation assumptions | Safe to reuse as-is | Needs generalization | Do not generalize | Proposed extension point | Risk Gen I | Risk Gen II |
|---|---|---|---|---|---|---|---|---|
| Create/Edit/View lifecycle | `PokemonEditorFoundationContract` | none beyond staged/read-only modes | **YES** | no | native serializer rules | adapter provides staged session | LOW | LOW |
| Action sheet / action geometry | `SharedPokemonEditorContract` | capability-driven | **YES** | no | generation-specific action forks | exact capability provider | LOW | LOW |
| DETAILS/STATS/MOVES shell | shared editor + accepted Gen I/II surfaces | field sets vary | **YES** | field/schema descriptors | native stat math | exact-format provider | LOW | LOW |
| Species picker | `SharedSpeciesPicker` | contiguous species IDs + shared sprites | **YES** | provider for allowed IDs/forms when needed | duplicate Gen3 picker | species selection provider | LOW | LOW |
| Move picker | Gen I/II exact compatibility implementations + shared UX | current providers are generation-specific | UX yes | **YES** provider interface | one universal learnset table | `canLearn(game,species,form,move)` + preservation status | LOW | MEDIUM |
| Move row / PP / PP Ups | accepted shared editor | PP representation varies by format | UX yes | schema/provider | raw encoding | move-slot descriptor | LOW | LOW |
| Stat/radar presentation | shared radar + Gen1/Gen2 presentation | Gen I five stats; Gen II native DV/Stat Exp semantics | geometry yes | **YES** labels/stored-vs-derived schema | DV/IV/EV unification | `StatPresentationSchema` | MEDIUM | MEDIUM |
| Pokémon field capability model | `SharedEditor::Capabilities` | currently Gen I/II-complete, partial future vocabulary | core yes | **YES** exact-format vocabulary | save operation rights | exact-game capability provider | LOW | LOW |
| Save-operation capability model | `SaveEdit::Capabilities` | save/editor operation rights | **YES** | bridge only | merge with field capabilities | explicit adapter-to-UI bridge | LOW | LOW |
| Trainer presentation | base + Gen II accepted overlays | title-specific fields | shell yes | **YES** descriptor | one universal mutable trainer struct | trainer field descriptor | LOW | MEDIUM |
| Native boxes | Gen I/II staged editors + shared box UI | Gen I/II are packed | accepted behavior yes | **YES** explicit storage semantics | converting Gen I/II to sparse | `StorageSemantics::{PackedNative,SparseOwned,...}` | HIGH | HIGH |
| Party presentation | read-only bridges/panels | party-native HP/status differs | yes with capability gating | descriptor | party mutation | party-field capability provider | LOW | LOW |
| Classic item catalog | `ClassicInventoryCatalog` | exact classic game + pocket | **YES** | mapping/provider only | duplicate Gen3 catalog | exact `ClassicGame` adapter | LOW | LOW |
| Gen III read model | `PKSMGen3Adapter` / native adapter | read-only FRLG/RSE, PK3 concepts | **YES** | adapter to shared normalized view | expose PKSM types in UI | PokeBank-owned Gen3 provider | NONE | NONE |
| Gen I staged Pokémon writer | `Gen1::StagedPokemonEditor` | PK1 packed/native semantics | **YES, leave native** | no | generic mutation struct | provider wrapper above it | HIGH | NONE |
| Gen II staged Pokémon writer | `Gen2::StagedEditor` | PK2 packed/native semantics | **YES, leave native** | no | generic mutation struct | provider wrapper above it | NONE | HIGH |
| Staged export / source safety | SourceMutationPolicy + generation transaction | no live source writes | **YES** | shared result/provenance descriptor if useful | bypass/one global write switch | staged transaction interface | HIGH | HIGH |
| Provenance | PokeBank architecture/Vault/action surfaces | origin != current location | **YES** | normalized read-only view | fabricated origin/history | provenance provider | LOW | LOW |
| Legality state | shared UI + future legality service | `UNKNOWN/Not checked` is valid | **YES** | explicit result provider later | assume legal because parse succeeds | legality result service | LOW | LOW |

## Per-feature research/reuse records

### Shared editor/provider boundary

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** `POKEMON_EDITOR_UI_CONTRACT`, `ARCHITECTURE`, `UPSTREAM_AUDIT`, `PKSM_CORE_INTEGRATION`
- **EXTERNAL SOURCE/PROJECT:** PKSE, PKSM-Core, PKHeX
- **CURRENT IN-TREE IMPLEMENTATION:** shared contracts + generation-native staged editors + Gen III read-only adapter
- **DECISION:** **REFACTOR/EXTEND IN PLACE**
- **WHY:** UI lifecycle is already generation-neutral; serialization/edit structs are truthfully generation-specific.
- **LICENSE/REDISTRIBUTION:** keep PKSM-Core/other backend types behind PokeBank boundaries; preserve notices.
- **TEST/SAFETY:** compile-time/text contracts plus existing Gen I/II permanent suites; no source writes.

### Species picker

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** Gen I hardware acceptance/followups, `POKEMON_EDITOR_UI_CONTRACT`
- **EXTERNAL SOURCE/PROJECT:** inherited PKSE visual conventions only as background
- **CURRENT IN-TREE IMPLEMENTATION:** `SharedSpeciesPicker.h`
- **DECISION:** **REUSE DIRECTLY**
- **WHY:** already extracted from accepted Gen I and used as shared infrastructure.
- **LICENSE/REDISTRIBUTION:** existing repository licensing applies.
- **TEST/SAFETY:** preserve hover-only preview; commit only to draft/staged state.

### Move compatibility / picker

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** `GEN1_MOVE_COMPATIBILITY_REFERENCE`, `GEN2_AUDIT_RESEARCH`, `POKEMON_EDITOR_UI_CONTRACT`, `PKHEX_ORACLE`
- **EXTERNAL SOURCE/PROJECT:** PKHeX as generator/oracle; generation-specific native tables in PokeBank
- **CURRENT IN-TREE IMPLEMENTATION:** exact RBY/GSC providers, hardware-filtered picker
- **DECISION:** **REFACTOR interface, REUSE behavior**
- **WHY:** exact-game filtering is accepted; tables/semantics must remain per format.
- **LICENSE/REDISTRIBUTION:** generated data provenance must remain documented.
- **TEST/SAFETY:** existing unusual source move must remain preservable; browsing cannot mutate staged/source bytes.

### Stats

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** Gen I radar/device docs, `GEN2_AUDIT_RESEARCH`, `POKEMON_EDITOR_UI_CONTRACT`
- **EXTERNAL SOURCE/PROJECT:** PKHeX/PKSM-Core as correctness references
- **CURRENT IN-TREE IMPLEMENTATION:** five-stat Gen I, six calculated Gen II, per-generation math
- **DECISION:** **REFACTOR presentation schema only**
- **WHY:** values/labels can be normalized for display; DV/IV/Stat Exp/EV semantics cannot.
- **TEST/SAFETY:** Gen I one Special and Gen II one Special DV/Stat Exp remain invariant.

### Classic item lane

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** `CLASSIC_SAVE_EDITOR_ARCHITECTURE`, `PROJECT_RESOURCE_INDEX`, `UPSTREAM_AUDIT`
- **EXTERNAL SOURCE/PROJECT:** PKSE/PKSM/PKHeX as references
- **CURRENT IN-TREE IMPLEMENTATION:** `ClassicInventoryCatalog`, `Gen3StagedInventoryEditor`, Gen III read model
- **DECISION:** **REUSE DIRECTLY + ADAPT**
- **WHY:** one exact-game item catalog already spans RBY/GSC/RSE/FRLG.
- **TEST/SAFETY:** story-sensitive key-item and exact-pocket rules remain enforced; held-item serialization stays in Pokémon adapter.

### Gen III backend

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** `PKSM_CORE_INTEGRATION`, `SAVE_ENGINE_REFERENCE_AUDIT`, `UPSTREAM_AUDIT`, `PKHEX_ORACLE`
- **EXTERNAL SOURCE/PROJECT:** PKSM-Core (GPLv3), PKHeX oracle/reference
- **CURRENT IN-TREE IMPLEMENTATION:** `PKSMGen3Adapter` read-only PokeBank boundary plus exception-free native slice
- **DECISION:** **REUSE/ADAPT EXISTING BOUNDARY**
- **WHY:** it already exposes PokeBank-owned records for FRLG/RSE and hides concrete PKSM types.
- **TEST/SAFETY:** remain read-only until a later explicit staged-Pokémon-editor milestone; independent oracle/golden comparisons required.

## Explicit unresolved items

The following are **UNRESOLVED / later milestone**, not guessed in #71:

- exact Gen III editable PID/nature/ability coupling policy for Expert mode;
- exact legality/autolegal runtime architecture;
- live save write enablement;
- party mutation;
- ribbon collection editor implementation details;
- future Marks semantics;
- cross-generation conversion behavior beyond the existing research freeze.

None block the P0 shared-provider descriptors because P0 must represent unsupported/unknown capabilities explicitly.
