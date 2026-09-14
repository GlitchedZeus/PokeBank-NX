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
