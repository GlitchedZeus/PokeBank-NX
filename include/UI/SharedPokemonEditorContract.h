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

enum class Action : uint8_t {
    View,
    Edit,
    Clone,
    Remove,
    LegalityProvenance,
    Add,
    Review,
    Close,
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

enum class DraftEvent : uint8_t {
    Navigate,
    BrowsePicker,
    AcceptPicker,
    EditField,
    Cancel,
    StageAdd,
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
