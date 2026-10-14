# Request 01 (to Thread 2): register Gen VI in the shared editor and identity tables

Owner of the touched files: Thread 2 (`include/UI/**`, `include/Games/**` and its
sources). Thread 3 does not edit them. The patches below are against
`a17c503ad272cc55e69b8b3cbd925cec999567cc`.

## 1. `include/Games/GameIdentity.h`: a 3DS platform

```diff
     enum class Platform : uint8_t {
         GameBoy,
         GameBoyColor,
         GameBoyAdvance,
         NintendoDS,
         NintendoSwitch,
+        Nintendo3DS,
     };
```

Add four `GameDescriptor` rows with `SourceSupport::Planned` (ids `x_3ds`, `y_3ds`,
`omegaruby_3ds`, `alphasapphire_3ds`; dataGeneration 6; switchTitleId 0). The
3DS title IDs are in `Gen6::Layout::TitleId*` for display only. Any `switch`
over `Platform` needs a `Nintendo3DS` label ("Nintendo 3DS").

## 2. `include/UI/ExactSaveCapabilities.h`: PK6 format and families

```diff
 enum class SaveFormat : uint8_t {
     Unknown, RBYInternational, RBYJapanese, GSCInternational, GSCJapanese,
-    PK3GBA, PK4, PB7, PA8, PK9, PK5
+    PK3GBA, PK4, PB7, PA8, PK9, PK5, PK6
 };
 enum class SaveFamily : uint8_t {
     ...
-    LetsGo, LegendsArceus, ScarletViolet, BlackWhite, Black2White2
+    LetsGo, LegendsArceus, ScarletViolet, BlackWhite, Black2White2,
+    XY, OmegaRubyAlphaSapphire
 };
```

In `exactSaveCapabilities()`, before the final `return std::nullopt`:

```cpp
    if (game == "x_3ds" || game == "y_3ds" || game == "omegaruby_3ds" || game == "alphasapphire_3ds") {
        if (id.platform != Platform::Nintendo3DS || id.generation != Generation::Gen6 ||
            id.format != SaveFormat::PK6) return std::nullopt;
        const bool xy = game == "x_3ds" || game == "y_3ds";
        return ExactSaveCapabilities{id, xy ? SaveFamily::XY : SaveFamily::OmegaRubyAlphaSapphire,
            StatModel::IVEV, capabilitiesForGeneration(Generation::Gen6), 31, 252, true};
    }
```

In `capabilitiesForSourceId()`:

```cpp
    if (id == "x_3ds" || id == "y_3ds" || id == "omegaruby_3ds" || id == "alphasapphire_3ds")
        return exactSaveCapabilities({id,Platform::Nintendo3DS,Generation::Gen6,SaveFormat::PK6});
```

## 3. `include/UI/SharedPokemonEditorContract.h`: the Gen 6 field table

The shared table is per generation. PK6 also depends on the record region (party
records cache battle stats). Proposal: the shared table returns the **Box** column,
and the shared editor asks `Gen6EditorProvider::fieldState(field, RecordRegion::Party)`
for a party slot, the same way it already consults provider descriptors.

```cpp
    if (generation == Generation::Gen6) {
        // Box-region PK6 table; party slots must use Gen6EditorProvider::fieldState(..., Party),
        // which makes IV/EV/Nature ReadOnly and Level/CalculatedStats ReadOnly.
        switch (field) {
            case FieldIdentity::Nickname: case FieldIdentity::HeldItem:
            case FieldIdentity::Friendship: case FieldIdentity::Pokerus:
            case FieldIdentity::OriginalTrainer: case FieldIdentity::OriginalTrainerGender:
            case FieldIdentity::TrainerId: case FieldIdentity::SecretId:
            case FieldIdentity::Ball: case FieldIdentity::FatefulEncounter:
            case FieldIdentity::IV: case FieldIdentity::EV: case FieldIdentity::Nature:
                return FieldAccess::Editable;
            case FieldIdentity::Shiny: case FieldIdentity::Level:
                return FieldAccess::Derived;
            case FieldIdentity::DV: case FieldIdentity::StatExperience: case FieldIdentity::AV:
            case FieldIdentity::EffortLevel: case FieldIdentity::StatNature:
            case FieldIdentity::CalculatedStats:
                return FieldAccess::Hidden;
            default:
                return FieldAccess::ReadOnly;
        }
    }
```

`layoutFor(Generation::Gen6)` can reuse the Gen IV layout `{16, 11, 4, 6, 3}`.

Regression test to add in Thread 2's suite: for every `Gen6EditorProvider::AllFields`,
`fieldStateFrom(fieldAccessForGeneration(Gen6, f)) == Gen6EditorProvider::fieldState(f, RecordRegion::Box)`.

## 4. Editor descriptor

Once (1)–(3) land, `Gen6EditorProvider::descriptorForSource` can return the shared
`ExactFormatEditorDescriptor` directly. Thread 3 will make that switch in its lane
after the shared change is merged into its base.
