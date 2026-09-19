# Issue #71 — Universal editor / reuse architecture freeze

Status: **AUDIT COMPLETE / GEN III IMPLEMENTATION NOT AUTHORIZED**  
Audit date: 2026-09-18  
Production base: `feature/pokebank-playable@94efe4dda341e623538b3e6fe895a9dba04ed195`  
Production tree: `a501f4929570bad3f5c0ff216d84cbbdb1ffee5c`  
Accepted Gen I/II source: `c24859ce17d33040685ea19b9aff068ba378d8ae`  
Accepted Gen I/II tree: `88d2ad46d57346df346453698ee11792f7df4f1f`

This document freezes the architecture required by issue #71 before any Generation III Pokémon editor work begins.

The permanent product rule is:

```text
ONE Pokémon editor
next generation = the accepted shared editor + only the capabilities/mechanics the exact format adds or changes
```

This audit does **not** authorize a Gen III Create/Edit route, PK3 mutation/serialization, party mutation, trainer mutation, live source writes, or a parallel generation-specific editor shell.

## 1. Frozen accepted baseline

The accepted Gen I/II device checkpoint remains immutable historical evidence. Future work must preserve:

- Red/Blue/Yellow and Gold/Silver/Crystal native 20-slot source boxes;
- PokeBank-owned Legacy Storage at 30 slots;
- Add/Create, View/Edit/Create, A Actions, confirmed Release;
- Y single packed move and Y-hold rectangular multi-select;
- deterministic same-box/cross-box packed movement;
- atomic destination-capacity rejection;
- B exact staged restore;
- accepted controller/fullscreen/action-sheet ownership behavior.

Permanent source-write contract:

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN / UNSUPPORTED SAVE VARIANTS: FAIL CLOSED
```

## 2. Current external/reference state

The project keeps reproducible research pins. A newer upstream head does not silently replace a reviewed pin.

| Project | Role | Reviewed/project pin | Current default-head observed 2026-09-18 | Freeze decision |
|---|---|---|---|---|
| `kiasta/PKSE` | inherited Switch/UI/editor/storage foundation | `1133aedffeeffb5862f35162597e249233903597` | same `1133aed...` | direct inherited code may be reused where it already lives in-tree; new mutation behavior must still pass PokeBank safety/provenance rules |
| `kwsch/PKHeX` | correctness/data/legality/conversion oracle | `e15d2467b32da7bc26ce7cc8e5c4ede32740e20a` | `8ad201e80244f630ab5a46922ab72fb79c5ad4f4` | host/reference only; do not float runtime behavior with upstream |
| `Insektaure/pkHouse` | modern Switch save/container/storage UX reference | `4e288f4a66acf2e58822ad1bb372a6803d3c46dd` | `a3717e2e2e3a211ecd0b5571dddbf13b8676c01b` | reference/reimplement only; no direct pkHouse runtime dependency |
| `FlagBrew/PKSM-Core` | historical format oracle/reuse candidate | `aa22d7a4f87c0351baf7da5962ba5acd01039a7c` | `d9bca6fcdf622760a0aa963565b102276e2ae2a3` | retain the reviewed Gen III boundary/pin until a separate repin is explicitly tested |

Important current-upstream observations:

- PKSE 1.1.3 already contains a reusable `PickerDialog` vocabulary covering Nature, Gender, Move, Item/ItemG3, Level, Friendship, Ball, Ability, Language, Origin, MetLevel, MetLocation, Form, StatNature, Species and pouch items.
- PKSE already contains `CursorMode::Move` / `CursorMode::Multi`, single pickup, rectangular selection and a dual-pane save↔Bank storage interaction.
- PKHeX continues to change legality/data behavior; a current September 2026 change added a Gen III/IV EV/minimum-EXP legality check. That reinforces the rule that PokeBank must pin oracle revisions and record engine versions.
- pkHouse has advanced beyond the reviewed pin, including later Wondercard/bank work. Issue #11 must explicitly repin/revalidate modern save behavior when that milestone starts.
- PKSM-Core has also advanced beyond the accepted Gen III research pin. Its current changes do not authorize changing the device-accepted read-only adapter.

## 3. Current PokeBank shared-editor component map

### Product-level shared contracts — MUST REUSE

`include/UI/SharedPokemonEditorContract.h`

Owns the product-level one-editor rules:

- one `Surface` model for Boxes / Actions / View / Edit / CreateDraft / Picker / Review;
- shared Action model;
- common box activation semantics;
- shared semantic `FieldIdentity` vocabulary;
- `FieldAccess = Hidden / Derived / ReadOnly / Editable`;
- generation-aware layout/focus helpers;
- explicit guards that generations do not own separate editor UI and future generations extend the same foundation.

`include/UI/PokemonEditorFoundationContract.h`

Owns accepted lifecycle/focus/presentation foundations and generation capability defaults. Its generation defaults are convenience values only; exact-save capability must remain authoritative. Its Gen I-shaped DV/Stat Exp focus geometry is **not** permission to force later generations into Gen I storage semantics.

`include/UI/SharedSpeciesPicker.h`

Already extracted shared picker behavior. Reuse rather than create a `Gen3SpeciesPicker`.

### Generation-native mutation/storage — KEEP NATIVE

- `Gen1::StagedPokemonEditor`
- `Gen2::StagedEditor`
- Gen I/II packed move implementations

Do not genericize PK1/PK2/PK3 binary mutation structures into one universal writer. The shared editor talks to exact-format adapters/providers; the exact adapter owns native validation and serialization.

### Gen III boundary — READ ONLY

`include/Integration/Gen3/PKSMGen3Adapter.h`

The current public Gen III boundary is intentionally PokeBank-owned and read-only. It exposes no PKSM-Core types and no staged mutation/finalization API.

Current `PokemonRecord` already provides useful normalized read data:

- species;
- PID;
- TID/SID;
- EXP;
- held item;
- four moves + PP;
- six IVs;
- six EVs;
- nickname / OT;
- original bytes;
- checksum/round-trip state.

It currently does **not** expose a complete Gen III editor model for Nature, Ability slot, Ball, language, met/origin fields or ribbons. Those are future adapter/provider work, not values to fabricate in shared UI.

### Source safety — MUST REUSE

`include/Safety/SourceMutationPolicy.h`

Non-View mutation remains limited to staged/backup or app-owned storage. Installed-game and RetroArch sources do not gain mutation permission because an editor field exists.

## 4. PKSE reuse inventory and classification

The classifications apply to the upstream/inherited behavior, not to already accepted PokeBank behavior.

| PKSE area | Classification | Why / PokeBank boundary |
|---|---|---|
| single Move pickup / put-down interaction | **REFACTOR INTO SHARED POKEBANK COMPONENT** | interaction/state-machine concepts are useful; direct legacy mutation is not the accepted source-write path |
| Multi rectangular selection | **REFACTOR INTO SHARED POKEBANK COMPONENT** | reuse rectangle/carry concepts for future staged storage organization; PR #74 already owns accepted Gen I/II behavior |
| box/block organization | **REFACTOR INTO SHARED POKEBANK COMPONENT** | future shared storage controller may reuse the model; exact adapter owns packed/sparse mutation |
| legacy direct save/Bank put-down mutation routine | **REJECT** | can bypass generation-native staged semantics and source immutability |
| Pokémon Creator lifecycle | **BACKEND / REFERENCE ONLY** | PokeBank already has accepted CreateDraft/Stage semantics; reuse useful lifecycle lessons, not a second creator shell |
| edit snapshot / revert behavior | **REUSE DIRECTLY** through current PokeBank staged editors | exact Gen I/II snapshot/restore behavior is already accepted; upstream is supporting reference only |
| generic PickerDialog interaction model | **REFACTOR INTO SHARED POKEBANK COMPONENT** | useful common list/search/selection plumbing, but exact domain/filtering belongs to provider |
| Species picker | **REUSE DIRECTLY** | `SharedSpeciesPicker` already exists; future provider may restrict exact species/forms |
| Move picker | **REUSE DIRECTLY + REFACTOR provider boundary** | accepted exact-game filtering/preservation UX stays; game/species/form learnability must remain exact-format data |
| Held Item picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | UI interaction is generic; item ID spaces and legal domains are exact-game/native |
| Nature / Stat Nature picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | generic 25-value UX is reusable; native/PID/mint semantics are exact-format |
| Gender picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | species/form availability and derived-vs-stored semantics are adapter-owned |
| Ability picker | **BACKEND / REFERENCE ONLY** for legality/order; shared picker plumbing may be reused | ability slot/PID relationships are format-specific; do not copy “allow illegal” policy blindly |
| Ball picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | selection UX shared; exact game Ball domain/provider owns truth |
| Friendship picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | simple shared numeric/list UX; exact format decides existence/editability |
| Language picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | exact format/game validates allowed values and encoding |
| Origin Game picker | **DEFER** editable semantics; **BACKEND / REFERENCE ONLY** for display/domain | origin/provenance is safety-sensitive and must not become freely editable merely because upstream exposes a picker |
| Met Level / Met Location picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | shared chooser possible; exact origin/version/location tables and edit rules stay adapter-owned |
| Form picker | **REFACTOR INTO SHARED POKEBANK COMPONENT** | reuse storable-form filtering concept; exact game presence/battle-only rules remain provider data |
| legality/ribbon overlay concepts | **BACKEND / REFERENCE ONLY** | PokeBank owns legality/provenance state and future collection UI; no upstream overlay becomes the product contract |
| cross-game conversion gates/warnings | **BACKEND / REFERENCE ONLY / DEFER to #10** | conversion is a transfer-engine concern, not normal editor mutation |

## 5. PR #74 versus inherited Move/Multi

Issue #71 predates the accepted PR #74 milestone. The accepted implementation is now the authority for Gen I/II.

Already solved and frozen by PokeBank:

- Y tap single packed pickup/drop;
- Y-hold rectangular multi-select;
- deterministic selected/group order;
- same-box and cross-box packed movement;
- all-or-nothing capacity rejection;
- exact B staged restore;
- native 20-slot source capacity;
- accepted controller/fullscreen ownership.

PKSE still teaches useful future organization concepts:

- one carried-block model for single and multi;
- dual-pane game/storage navigation;
- destination preview;
- potential swap/reorder interactions;
- shared cursor/selection state independent from native mutation.

Future naming/semantics must stay distinct:

```text
Reorder / Move Slot
  = staged organization inside one exact native/app-owned container

Transfer Copy
  = source remains active; compatible destination representation is created

Transfer Move
  = destructive active-location relocation only after #20 safety gates
```

Do not replace PR #74 merely to make the code resemble PKSE.

## 6. PKHeX responsibility map

PKHeX is the primary host-side correctness/data oracle, not a Switch UI dependency.

| Concern | PKHeX role | PokeBank runtime role |
|---|---|---|
| personal/species/forms | oracle / table source | pinned/generated offline provider data |
| move presence / learnsets | oracle / generator input | exact-game offline compatibility provider |
| base PP / PP rules | oracle / generator input | exact-game tables; never one global latest-gen table |
| abilities / ability slots | oracle | exact-format provider + native serializer later |
| held items / Ball domains | oracle | exact-game provider/catalog |
| met locations / game/version presence | oracle | exact identity/revision provider |
| growth / EXP | correctness oracle | native deterministic runtime logic |
| PID relationships, gender, shiny, Nature | correctness/legality oracle | generation-native adapter semantics |
| encounter legality | oracle | `LEGAL / INVALID / UNKNOWN` result service later |
| ribbons / marks | oracle/data reference | exact-format collection capability later |
| events / Mystery Gifts | reference/oracle | separate event/provenance milestone |
| conversion | golden/reference output | future #10 native conversion/staging |
| save revision/version differences | oracle/reference | exact release+platform+revision adapters |

**Compatibility is not encounter legality.** A move or species being representable/present does not mean the specimen is encounter-legal.

## 7. pkHouse responsibility map

pkHouse remains **REFERENCE ONLY**.

High-value reference areas:

- modern Switch title IDs/save filenames;
- family dispatch and container/block models;
- modern box/slot sizing;
- Switch FR/LG outer save behavior;
- SCBlock family handling;
- BDSP/LGPE family-specific behavior;
- bounded box cache/lazy-load ideas;
- dual-pane bank/storage UX;
- bulk/multi-select workflow ideas;
- Handling Trainer / game Pokédex side effects;
- modern revision/update behavior.

PokeBank rules:

- independently reimplement behind PokeBank interfaces;
- keep `firered_gba` and `firered_switch` distinct;
- no pkHouse direct-write behavior authorizes a PokeBank write;
- issue #11 must pin and test every modern family/revision it claims.

## 8. PKSM-Core / in-tree responsibility map

PKSM-Core role:

- reviewed historical PK entity/save oracle/reuse source;
- current Gen III host correctness path at the pinned revision;
- format behavior and round-trip cross-check.

PokeBank in-tree role:

- narrow public PokeBank adapter API;
- strict validation before backend use;
- exception-free native Gen III read backend;
- product UI/capabilities/provenance/safety;
- source immutability.

Do not expose PKSM-Core concrete types to shared UI and do not expose its write capabilities merely because the upstream library has them.

## 9. Universal capability contract — frozen

The existing shared product contract already provides the primary state vocabulary:

```text
HIDDEN
DERIVED
READ_ONLY
EDITABLE
```

Exact semantics:

- **HIDDEN** — field is absent, meaningless, not representable, or intentionally unsupported for this exact format/revision. Do not synthesize a fake value.
- **DERIVED** — value is calculated from native stored fields and is not independently stored. A separate semantic action may safely transform underlying fields without changing this classification.
- **READ_ONLY** — value can be truthfully displayed, but the exact adapter/milestone cannot safely stage/validate/serialize it.
- **EDITABLE** — the exact adapter can validate, stage, serialize and reparse the field safely without mutating the original source.

Field state is evaluated for **exact release + platform + revision**, not merely by generation. Generation-level defaults are convenience only.

### Required semantic field families

Identity:
- Species, Nickname, Form, Gender, Shiny, Language.

Progression:
- Level, EXP, Friendship.

Stats:
- DV / IV;
- Stat Exp / EV / AV / Effort Levels;
- Nature;
- Stat Nature;
- Hyper Training where native;
- exact-game modifiers.

Moves:
- four active moves;
- PP / PP Ups;
- relearn moves where native;
- exact-game move-state mechanics.

Traits:
- Ability;
- Held Item;
- Pokérus;
- later native traits such as Tera Type / Dynamax / mastery.

Origin / met:
- OT;
- TID;
- SID where native;
- Origin Game;
- Ball;
- Met Level / Location / Date;
- Egg data.

Collection:
- Ribbons;
- Marks;
- exact-game cosmetic state.

Validation / provenance:
- field validity;
- move compatibility;
- encounter legality;
- original origin;
- current location;
- original bytes;
- current bytes;
- source-save identity;
- parent/clone/derived links;
- hashes;
- conversion/derivation state.

Fields not yet represented by `FieldIdentity` are future vocabulary extensions only when an exact supported adapter needs them. Do not enlarge the enum speculatively.

### Capability layers must remain separate

```text
field access/capability
!=
source operation/write capability
```

An `EDITABLE` staged field never implies that the live source may be written. Source mutation policy remains a separate hard gate.

## 10. Exact identity rule

Never collapse releases/platforms because payloads look similar.

Examples that remain distinct:

```text
firered_gba
leafgreen_gba
firered_switch
leafgreen_switch
```

Every provider/adapter decision is keyed by exact game identity and, where technically relevant, save revision/update/DLC/container variant.

## 11. Gen III delta — DOCUMENTED ONLY

Generation III is an extension of the accepted shared editor, not a new shell.

Required Gen III capability delta relative to Gen I/II:

- six IVs instead of DVs;
- six EVs instead of Stat Exp;
- Nature;
- Ability / ability-slot semantics;
- Held Item through Gen III's native item space;
- Friendship;
- language;
- TID + SID;
- PID and its exact relationships to gender/shiny/Nature/Ability behavior;
- Ball;
- richer origin/met data;
- Gen III ribbons where native;
- 14 native PC boxes × 30 slots in the current GBA read model;
- exact Ruby/Sapphire/Emerald versus FireRed/LeafGreen differences.

The future Gen III milestone must first map these through shared field access/provider descriptors and only then add generation-native staged PK3 mutation.

No `Gen3PokemonEditor`, Gen III action menu, Gen III Create wizard, or duplicate picker family is permitted.

## 12. Architecture guard strategy

Permanent regression guard for #71 should prove:

1. no `Gen3PokemonEditor` file/namespace appears under normal UI;
2. `SharedPokemonEditorContract` still states one shared editor and future generations extend it;
3. field-state semantics remain Hidden/Derived/ReadOnly/Editable;
4. Gen III public adapter remains read-only and exposes no staged-finalization API;
5. installed-game and RetroArch mutation remain denied by `SourceMutationPolicy`.

Existing Gen I/II focused, packed-move, shared-editor, host and sanitizer tests remain the behavior regression authority. The #71 guard does not replace those tests.

## 13. Deferred / explicitly not decided by #71

- exact editable PID/Nature/Ability coupling policy;
- legality/autolegal runtime architecture;
- Ribbon/Marks editor UX;
- party mutation;
- trainer mutation;
- cross-generation conversion execution;
- true destructive Transfer Move;
- live save write enablement;
- broad modern Switch revision support.

Those belong to their tracked milestones and must remain unsupported/unknown until proven.

## Freeze decision

Issue #71 is complete enough to freeze the next-generation editor direction:

```text
PokeBank shared editor/lifecycle/action/picker surface
        ↓
exact-game capability/data provider
        ↓
generation-native read/staged adapter
        ↓
reviewed oracle/reference engines
```

Gen III implementation may begin only after explicit owner approval, on a separate implementation milestone, while preserving the exact Gen I/II hardware-accepted checkpoint.
