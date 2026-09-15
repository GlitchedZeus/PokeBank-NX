#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel::SharedPokemonEditor {

// Permanent product rule: PokeBank NX has one Pokemon editor shell. Generations
// provide data/capability adapters; they do not own separate action/view/edit/create UI.
enum class Generation : uint8_t { Gen1 = 1, Gen2 = 2, Gen3 = 3, Gen4 = 4, Gen5 = 5,
                                  Gen6 = 6, Gen7 = 7, Gen8 = 8, Gen9 = 9 };

enum class Surface : uint8_t {
    Boxes,
    Actions,
    View,
    Edit,
    CreateDraft,
    Picker,
    Review,
};

enum class PassiveViewOrigin : uint8_t {
    Box,
    Party,
    Storage,
    ActionSheet,
};

constexpr Surface passiveViewSurfaceFor(PassiveViewOrigin) noexcept {
    return Surface::View;
}

enum class Action : uint8_t {
    View,
    Edit,
    Clone,
    Remove,
    LegalityProvenance,
    Add,
    Review,
    Close, // internal close/cancel semantic; product copy is always "Cancel".
    None,
};

struct ActionCapabilities {
    bool canClone = true;
    bool canRemove = false;
    bool hasLegalityProvenance = false;
};

struct ActionSet {
    std::array<Action, 8> values{};
    std::size_t count = 0;

    constexpr Action operator[](std::size_t index) const noexcept {
        return index < count ? values[index] : Action::None;
    }
};

constexpr ActionSet actionsForSlot(bool occupied, ActionCapabilities capabilities = {}) noexcept {
    ActionSet result{};
    const auto append = [&result](Action action) constexpr {
        result.values[result.count++] = action;
    };

    if (!occupied) {
        append(Action::Add);
        append(Action::Review);
        if (capabilities.hasLegalityProvenance) append(Action::LegalityProvenance);
        append(Action::Close);
        return result;
    }

    append(Action::View);
    append(Action::Edit);
    if (capabilities.canClone) append(Action::Clone);
    if (capabilities.canRemove) append(Action::Remove);
    if (capabilities.hasLegalityProvenance) append(Action::LegalityProvenance);
    append(Action::Review);
    append(Action::Close);
    return result;
}

constexpr const char* actionLabel(Action action) noexcept {
    switch (action) {
        case Action::View: return "View";
        case Action::Edit: return "Edit";
        case Action::Clone: return "Clone";
        case Action::Remove: return "Remove";
        case Action::LegalityProvenance: return "Legality & Provenance";
        case Action::Add: return "Add Pokemon";
        case Action::Review: return "Review Pending Changes";
        case Action::Close: return "Cancel";
        case Action::None: return "";
    }
    return "";
}

struct ActionMenuGeometry {
    int rowStart = 86;
    int rowHeight = 42;
    int rowStep = 46;
};
constexpr ActionMenuGeometry actionMenuGeometry() noexcept { return {}; }

constexpr Surface surfaceForAction(Action action) noexcept {
    switch (action) {
        case Action::View: return Surface::View;
        case Action::Edit: return Surface::Edit;
        case Action::Add: return Surface::CreateDraft;
        case Action::Review: return Surface::Review;
        case Action::Clone:
        case Action::Remove:
        case Action::LegalityProvenance:
        case Action::Close:
        case Action::None:
            return Surface::Actions;
    }
    return Surface::Actions;
}

// Box interaction is shared across classic generations: A opens the contextual
// Empty Slot/Pokemon Actions menu, while X is a direct Add shortcut only on an
// empty visual cell. Packed native formats still choose the real append slot.
enum class BoxActivation : uint8_t { None, Actions, Add };
constexpr BoxActivation boxActivation(bool occupied, bool pressA, bool pressX) noexcept {
    if (pressX && !occupied) return BoxActivation::Add;
    if (pressA) return BoxActivation::Actions;
    return BoxActivation::None;
}

struct PostAddSelection {
    int box = -1;
    int slot = -1;
    bool openActions = false;
};
constexpr PostAddSelection postAddSelection(int currentBox, int packedSlot) noexcept {
    return {currentBox, packedSlot, false};
}

enum class DraftEvent : uint8_t {
    Navigate,
    BrowsePicker,
    AcceptPicker,
    EditField,
    Cancel,
    StageAdd, // internal transaction semantic; user-facing command is Add.
};

struct DraftDecision {
    bool mutateStagedSave = false;
    bool leaveDraft = false;
};

constexpr DraftDecision draftDecision(DraftEvent event) noexcept {
    switch (event) {
        case DraftEvent::Navigate:
        case DraftEvent::BrowsePicker:
        case DraftEvent::AcceptPicker:
        case DraftEvent::EditField:
            return {false, false};
        case DraftEvent::Cancel:
            return {false, true};
        case DraftEvent::StageAdd:
            return {true, true};
    }
    return {};
}

// Shared field identities are deliberately broader than any one generation.
// Merely naming a field never makes it visible: the active exact-save adapter
// decides whether the field is hidden, derived, read-only or editable.
enum class FieldIdentity : uint8_t {
    Species,
    Nickname,
    Form,
    Gender,
    Shiny,
    Language,
    Level,
    Experience,
    Friendship,
    DV,
    IV,
    StatExperience,
    EV,
    AV,
    EffortLevel,
    Nature,
    StatNature,
    CalculatedStats,
    Ability,
    HeldItem,
    Pokerus,
    OriginalTrainer,
    TrainerId,
    SecretId,
    PersonalityId,
    OriginGame,
    Ball,
    MetLevel,
    MetLocation,
    MetDate,
    Egg,
    EggLocation,
    EggDate,
    MoveCompatibility,
    EncounterLegality,
    Provenance,
    OriginalTrainerGender,
};

enum class FieldAccess : uint8_t { Hidden, Derived, ReadOnly, Editable };

constexpr FieldAccess fieldAccessForGeneration(Generation generation, FieldIdentity field,
                                                bool crystalCaughtData = false) noexcept {
    if (generation == Generation::Gen1) {
        switch (field) {
            case FieldIdentity::Species:
            case FieldIdentity::Nickname:
            case FieldIdentity::Level:
            case FieldIdentity::Experience:
            case FieldIdentity::DV:
            case FieldIdentity::StatExperience:
            case FieldIdentity::OriginalTrainer:
            case FieldIdentity::TrainerId:
                return FieldAccess::Editable;
            case FieldIdentity::Shiny:
                return FieldAccess::Derived;
            case FieldIdentity::CalculatedStats:
            case FieldIdentity::OriginGame:
            case FieldIdentity::MoveCompatibility:
            case FieldIdentity::EncounterLegality:
            case FieldIdentity::Provenance:
                return FieldAccess::ReadOnly;
            default:
                return FieldAccess::Hidden;
        }
    }
    if (generation == Generation::Gen2) {
        switch (field) {
            case FieldIdentity::Species:
            case FieldIdentity::Nickname:
            case FieldIdentity::Level:
            case FieldIdentity::Experience:
            case FieldIdentity::Friendship:
            case FieldIdentity::DV:
            case FieldIdentity::StatExperience:
            case FieldIdentity::HeldItem:
            case FieldIdentity::Pokerus:
            case FieldIdentity::OriginalTrainer:
            case FieldIdentity::TrainerId:
                return FieldAccess::Editable;
            case FieldIdentity::Gender:
            case FieldIdentity::Shiny:
                return FieldAccess::Derived;
            case FieldIdentity::CalculatedStats:
            case FieldIdentity::OriginGame:
            case FieldIdentity::MoveCompatibility:
            case FieldIdentity::EncounterLegality:
            case FieldIdentity::Provenance:
                return FieldAccess::ReadOnly;
            case FieldIdentity::MetLevel:
            case FieldIdentity::MetLocation:
            case FieldIdentity::OriginalTrainerGender:
                return crystalCaughtData ? FieldAccess::ReadOnly : FieldAccess::Hidden;
            default:
                return FieldAccess::Hidden;
        }
    }
    // Future-generation identities are hooks only in this milestone. Their exact
    // activation belongs to the audited adapters, not to a guessed generic default.
    return FieldAccess::Hidden;
}

// Generic selected-row scrolling model. Current classic panels that fit remain
// stationary; future/native capability rows can exceed capacity without clipping.
struct ScrollWindow {
    std::size_t first = 0;
    std::size_t count = 0;
    bool scrolls = false;
};

constexpr ScrollWindow scrollWindow(std::size_t totalRows, std::size_t visibleCapacity,
                                    std::size_t focusedRow) noexcept {
    if (totalRows == 0 || visibleCapacity == 0) return {};
    if (visibleCapacity >= totalRows) return {0, totalRows, false};
    if (focusedRow >= totalRows) focusedRow = totalRows - 1;
    std::size_t first = focusedRow >= visibleCapacity ? focusedRow - visibleCapacity + 1 : 0;
    const std::size_t maxFirst = totalRows - visibleCapacity;
    if (first > maxFirst) first = maxFirst;
    return {first, visibleCapacity, true};
}

// The shell owns one three-panel focus language. Generation adapters only vary
// the row domain and which capability rows are meaningful.
enum class Panel : uint8_t { Details, Values, Moves };

struct Layout {
    uint8_t detailsRows = 5;
    uint8_t valuesRows = 7;
    uint8_t movesRows = 4;
    uint8_t valueStatRows = 5;
    uint8_t valueColumns = 3;
};

constexpr Layout layoutFor(Generation generation) noexcept {
    if (generation == Generation::Gen2)
        return {/*details*/5, /*values*/10, /*moves*/4, /*stat rows*/5, /*columns*/3};
    return {/*details*/5, /*values*/7, /*moves*/4, /*stat rows*/5, /*columns*/3};
}

struct Focus {
    Panel panel = Panel::Details;
    uint8_t row = 0;
    uint8_t column = 0;

    constexpr bool operator==(const Focus& other) const noexcept {
        return panel == other.panel && row == other.row && column == other.column;
    }
    constexpr bool operator!=(const Focus& other) const noexcept { return !(*this == other); }
};

constexpr uint8_t rowsFor(Generation generation, Panel panel) noexcept {
    const auto layout = layoutFor(generation);
    if (panel == Panel::Details) return layout.detailsRows;
    if (panel == Panel::Values) return layout.valuesRows;
    return layout.movesRows;
}

constexpr Focus normalize(Generation generation, Focus focus) noexcept {
    const auto layout = layoutFor(generation);
    const uint8_t rows = rowsFor(generation, focus.panel);
    if (rows != 0) focus.row = static_cast<uint8_t>(focus.row % rows);
    if (focus.panel == Panel::Details) {
        focus.column = 0;
    } else if (focus.panel == Panel::Values) {
        if (focus.row >= layout.valueStatRows)
            focus.column = 0;
        else if (focus.column >= layout.valueColumns)
            focus.column = static_cast<uint8_t>(layout.valueColumns - 1);
    } else {
        // Move, PP and PP Ups share one row.
        if (focus.column >= 3) focus.column = 2;
    }
    return focus;
}

constexpr Focus moveVertical(Generation generation, Focus focus, int direction) noexcept {
    focus = normalize(generation, focus);
    const int rows = rowsFor(generation, focus.panel);
    focus.row = static_cast<uint8_t>((static_cast<int>(focus.row) + direction + rows) % rows);
    return normalize(generation, focus);
}

constexpr Focus switchPanel(Generation generation, Focus focus, int direction) noexcept {
    focus = normalize(generation, focus);
    int panel = static_cast<int>(focus.panel);
    panel = (panel + direction + 3) % 3;
    focus.panel = static_cast<Panel>(panel);
    return normalize(generation, focus);
}

constexpr Focus moveColumn(Generation generation, Focus focus, int direction) noexcept {
    focus = normalize(generation, focus);
    if (focus.panel == Panel::Details) return focus;
    const auto layout = layoutFor(generation);
    const int maxColumn = focus.panel == Panel::Moves ? 2 :
        (focus.row < layout.valueStatRows ? static_cast<int>(layout.valueColumns) - 1 : 0);
    focus.column = static_cast<uint8_t>(direction < 0
        ? (focus.column == 0 ? 0 : focus.column - 1)
        : (focus.column >= maxColumn ? maxColumn : focus.column + 1));
    return focus;
}

struct Geometry720p {
    int x;
    int y;
    int width;
    int height;
    int headerHeight;
    int footerTop;
    int detailsX;
    int detailsWidth;
    int valuesX;
    int valuesWidth;
    int movesX;
    int movesWidth;
};

// Geometry family extracted from the accepted Gen I three-panel workspace.
constexpr Geometry720p geometry720p() noexcept {
    return {40, 31, 1200, 610, 82, 672, 64, 330, 404, 390, 804, 412};
}

constexpr const char* titleFor(Surface surface) noexcept {
    switch (surface) {
        case Surface::View: return "View Pokemon — READ ONLY";
        case Surface::Edit: return "Edit Pokemon — STAGED";
        case Surface::CreateDraft: return "Create Pokemon — DRAFT";
        case Surface::Actions: return "Pokemon Actions";
        case Surface::Review: return "Review Pending Changes";
        case Surface::Boxes: return "Pokemon Boxes";
        case Surface::Picker: return "Choose";
    }
    return "Pokemon";
}

// Recovery/reference code may remain compiled while the refactor settles, but none
// of these legacy Gen II paths are allowed to own the normal production Pokemon UX.
enum class Gen2LegacyPath : uint8_t {
    StageAddPromptWizard,
    LegacyTwentyEightRowEditor,
    LegacyPokemonActionsModal,
    RawSpeciesIdCreatePrompt,
    RawMoveIdCreatePrompt,
    RawHeldItemIdCreatePrompt,
    LegacyHandleInput,
};

constexpr bool gen2LegacyPathProductionReachable(Gen2LegacyPath) noexcept { return false; }
constexpr bool gen2ExternalPassiveViewUsesSharedSurface() noexcept { return true; }

constexpr bool passiveViewHasFieldCursor() noexcept { return false; }
constexpr bool passiveViewAllowsEditing() noexcept { return false; }
constexpr bool passiveViewAllowsPanelSwitching() noexcept { return false; }
constexpr bool createBrowsingMutatesStagedSave() noexcept { return false; }
constexpr bool editBrowsingMutatesSource() noexcept { return false; }
constexpr bool stageAddRequiresExplicitAction() noexcept { return true; }
constexpr bool oneTopLevelPokemonSurfaceOwnsFrame() noexcept { return true; }
constexpr bool generationOwnsSeparateEditorUI() noexcept { return false; }
constexpr bool futureGenerationsExtendSameFoundation() noexcept { return true; }

} // namespace PokeBank::UIModel::SharedPokemonEditor
