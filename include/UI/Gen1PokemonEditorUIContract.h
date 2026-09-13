#ifndef POKEBANK_UI_GEN1_POKEMON_EDITOR_CONTRACT_H
#define POKEBANK_UI_GEN1_POKEMON_EDITOR_CONTRACT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

namespace PokeBank::UIModel::Gen1Editor {

enum class Action : uint8_t {
    View,
    Edit,
    Clone,
    Remove,
    LegalityProvenance,
    AddPokemon,
    ReviewPendingChanges,
    Cancel,
};

struct ActionSet {
    std::array<Action, 7> values{};
    std::size_t count = 0;

    constexpr Action operator[](std::size_t index) const noexcept { return values[index]; }
};

// One authoritative action-capability model for the R/B/Y boxed editor. Empty slots never inherit
// mutation actions intended for an occupied boxed Pokemon.
constexpr ActionSet actionsForSlot(bool occupied) noexcept {
    if (occupied) {
        return {{{Action::View, Action::Edit, Action::Clone, Action::Remove,
                  Action::LegalityProvenance, Action::ReviewPendingChanges, Action::Cancel}}, 7};
    }
    return {{{Action::AddPokemon, Action::ReviewPendingChanges,
              Action::LegalityProvenance, Action::Cancel,
              Action::Cancel, Action::Cancel, Action::Cancel}}, 4};
}

enum class AddDraftEvent : uint8_t {
    Navigate,
    EditField,
    PreviewSpecies,
    JumpSection,
    Cancel,
    StageAdd,
};

struct AddDraftDecision {
    bool mutateStagedSave = false;
    bool leaveDraft = false;
};

// The draft is UI-local until the explicit StageAdd event. Scrolling, editing draft fields, changing
// the live sprite/identity preview, and section jumps are all guaranteed non-mutating operations.
constexpr AddDraftDecision addDraftDecision(AddDraftEvent event) noexcept {
    switch (event) {
        case AddDraftEvent::Navigate: return {false, false};
        case AddDraftEvent::EditField: return {false, false};
        case AddDraftEvent::PreviewSpecies: return {false, false};
        case AddDraftEvent::JumpSection: return {false, false};
        case AddDraftEvent::Cancel: return {false, true};
        case AddDraftEvent::StageAdd: return {true, true};
    }
    return {};
}

enum class EditorSection : uint8_t { Summary, Moves, DVs, StatExp, Trainer, Actions };

constexpr std::array<EditorSection, 5> editorSections() noexcept {
    return {EditorSection::Summary, EditorSection::Moves, EditorSection::DVs,
            EditorSection::StatExp, EditorSection::Trainer};
}

// Hardware UX acceptance requires one continuous field list. L/R may jump between these starts but
// is never required to progress through a wizard.
constexpr bool addUsesSingleScrollableWorkspace() noexcept { return true; }
constexpr bool addRequiresWizardPageNavigation() noexcept { return false; }
constexpr std::size_t addWorkspaceFieldCount() noexcept { return 30; }
constexpr std::array<std::size_t, 6> addSectionStarts() noexcept {
    return {0, 4, 16, 21, 26, 28};
}

inline std::string speciesPickerRow(uint16_t species, const char* name) {
    char prefix[8]{};
    std::snprintf(prefix, sizeof(prefix), "%03u - ", static_cast<unsigned>(species));
    return std::string(prefix) + (name ? name : "");
}

enum class FooterAction : uint8_t {
    Navigate,
    Select,
    Edit,
    Back,
    CancelDraft,
    StageAdd,
    Add,
    Remove,
    PreviousNextBox,
    PreviousNextSection,
    Page,
    ConfirmRemove,
    ConfirmClone,
    Export,
    Discard,
};

enum class FooterSurface : uint8_t {
    Boxes,
    ActionMenu,
    PokemonEditor,
    AddDraft,
    SpeciesPicker,
    MovePicker,
    CloneConfirm,
    RemoveConfirm,
    Review,
    Provenance,
};

struct FooterSet {
    std::array<FooterAction, 7> values{};
    std::size_t count = 0;
    constexpr FooterAction operator[](std::size_t index) const noexcept { return values[index]; }
};

constexpr FooterSet footerForSurface(FooterSurface surface, bool editable = true) noexcept {
    switch (surface) {
        case FooterSurface::Boxes:
            if (editable)
                return {{{FooterAction::Select, FooterAction::Add, FooterAction::Remove,
                          FooterAction::PreviousNextBox, FooterAction::Back}}, 5};
            return {{{FooterAction::Select, FooterAction::PreviousNextBox, FooterAction::Back}}, 3};
        case FooterSurface::ActionMenu:
            return {{{FooterAction::Navigate, FooterAction::Select, FooterAction::Back}}, 3};
        case FooterSurface::PokemonEditor:
            return {{{FooterAction::Navigate, FooterAction::Edit,
                      FooterAction::PreviousNextSection, FooterAction::Back}}, 4};
        case FooterSurface::AddDraft:
            return {{{FooterAction::Navigate, FooterAction::Edit, FooterAction::StageAdd,
                      FooterAction::CancelDraft, FooterAction::PreviousNextSection}}, 5};
        case FooterSurface::SpeciesPicker:
        case FooterSurface::MovePicker:
            return {{{FooterAction::Navigate, FooterAction::Select, FooterAction::Page,
                      FooterAction::Back}}, 4};
        case FooterSurface::CloneConfirm:
            return {{{FooterAction::PreviousNextBox, FooterAction::ConfirmClone, FooterAction::Back}}, 3};
        case FooterSurface::RemoveConfirm:
            return {{{FooterAction::ConfirmRemove, FooterAction::Back}}, 2};
        case FooterSurface::Review:
            return {{{FooterAction::Navigate, FooterAction::Export, FooterAction::Discard,
                      FooterAction::Back}}, 4};
        case FooterSurface::Provenance:
            return {{{FooterAction::Back}}, 1};
    }
    return {};
}

constexpr bool footerTopmostSurfaceOverridesParent() noexcept { return true; }
constexpr bool footerRestoresParentOnClose() noexcept { return true; }
constexpr bool footerMayAdvertiseUnavailableAction() noexcept { return false; }

constexpr bool hpDVIsStoredEditableField() noexcept { return false; }
constexpr bool hasSIDField() noexcept { return false; }
constexpr bool hasNatureField() noexcept { return false; }
constexpr bool hasAbilityField() noexcept { return false; }
constexpr bool hasHeldItemField() noexcept { return false; }
constexpr bool partyEditingEnabled() noexcept { return false; }
constexpr bool liveRetroArchWritingEnabled() noexcept { return false; }
constexpr bool liveInstalledGameWritingEnabled() noexcept { return false; }

} // namespace PokeBank::UIModel::Gen1Editor

#endif
