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
    OpenLogicalSubEditor,
    CancelSubEditor,
    Cancel,
    StageAdd,
};

struct AddDraftDecision {
    bool mutateStagedSave = false;
    bool leaveDraft = false;
};

constexpr AddDraftDecision addDraftDecision(AddDraftEvent event) noexcept {
    switch (event) {
        case AddDraftEvent::Navigate:
        case AddDraftEvent::EditField:
        case AddDraftEvent::PreviewSpecies:
        case AddDraftEvent::JumpSection:
        case AddDraftEvent::OpenLogicalSubEditor:
        case AddDraftEvent::CancelSubEditor:
            return {false, false};
        case AddDraftEvent::Cancel: return {false, true};
        case AddDraftEvent::StageAdd: return {true, true};
    }
    return {};
}

enum class EditorSection : uint8_t { Summary, Moves, Stats, Trainer, Actions };
constexpr std::array<EditorSection, 5> editorSections() noexcept {
    return {EditorSection::Summary, EditorSection::Moves, EditorSection::Stats,
            EditorSection::Trainer, EditorSection::Actions};
}

// Cleanup pass #2 keeps one coherent editor but presents logical rows instead of the original
// raw 30-field list. Move PP/PP Ups and DV/Stat Exp details live in contextual sub-editors.
constexpr bool addUsesSingleScrollableWorkspace() noexcept { return true; }
constexpr bool addRequiresWizardPageNavigation() noexcept { return false; }
constexpr std::size_t legacyRawFieldCount() noexcept { return 30; }
constexpr std::size_t logicalWorkspaceRowCount() noexcept { return 13; }
constexpr std::array<std::size_t,5> logicalSectionStarts() noexcept { return {0,3,7,9,11}; }
constexpr std::size_t logicalMoveRowCount() noexcept { return 4; }
constexpr bool ppIsTopLevelEditorRow() noexcept { return false; }
constexpr bool ppUpsIsTopLevelEditorRow() noexcept { return false; }
constexpr bool dvsAreGrouped() noexcept { return true; }
constexpr bool statExperienceIsGrouped() noexcept { return true; }

inline std::string speciesPickerRow(uint16_t species, const char* name) {
    char prefix[16]{};
    std::snprintf(prefix, sizeof(prefix), "%03u - ", static_cast<unsigned>(species));
    return std::string(prefix) + (name ? name : "");
}

// Highlighting a species is preview-only. A commits; B keeps the previous committed species.
constexpr uint16_t speciesPreviewOnHighlight(uint16_t, uint16_t highlighted) noexcept {
    return highlighted;
}
constexpr uint16_t speciesAfterPickerClose(uint16_t committed, uint16_t highlighted,
                                           bool accepted) noexcept {
    return accepted ? highlighted : committed;
}
constexpr bool speciesHoverMutatesDraft() noexcept { return false; }
constexpr bool speciesHoverMutatesStagedSave() noexcept { return false; }

// Gen I Summary / editor stats are exactly five axes. Special is unified.
constexpr std::size_t gen1RadarAxisCount() noexcept { return 5; }
constexpr bool gen1HasSplitSpecial() noexcept { return false; }
constexpr std::array<const char*,5> gen1StatLabels() noexcept {
    return {"HP", "Attack", "Defense", "Speed", "Special"};
}

// Hardware-layout contract: scrolling rows occupy a clipped viewport that ends before the fixed
// row/section status strip. The fixed footer is outside the panel entirely.
struct EditorGeometry {
    int contentTop;
    int scrollBottom;
    int statusTop;
    int panelBottom;
    int footerTop;
};
constexpr EditorGeometry editorGeometry720p() noexcept {
    return {122, 566, 578, 652, 672};
}
constexpr bool editorGeometryHasNoKnownCollision() noexcept {
    constexpr auto g = editorGeometry720p();
    return g.contentTop < g.scrollBottom && g.scrollBottom < g.statusTop &&
           g.statusTop < g.panelBottom && g.panelBottom < g.footerTop;
}

constexpr bool emptySlotUsesCompactDialog() noexcept { return true; }
constexpr bool smallConfirmationUsesCompactDialog() noexcept { return true; }
constexpr bool cloneUsesVisualDestinationGrid() noexcept { return true; }
constexpr bool cloneBrowseMutatesStagedSave() noexcept { return false; }
constexpr bool cloneRequiresExplicitConfirm() noexcept { return true; }
constexpr bool cloneSourceMutatedByClone() noexcept { return false; }

enum class FooterAction : uint8_t {
    Navigate,
    Select,
    Edit,
    Apply,
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
    MoveEditor,
    DVEditor,
    StatExpEditor,
    CloneDestination,
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
        case FooterSurface::MoveEditor:
        case FooterSurface::DVEditor:
        case FooterSurface::StatExpEditor:
            return {{{FooterAction::Navigate, FooterAction::Edit, FooterAction::Apply,
                      FooterAction::Back}}, 4};
        case FooterSurface::CloneDestination:
            return {{{FooterAction::Navigate, FooterAction::PreviousNextBox,
                      FooterAction::ConfirmClone, FooterAction::Back}}, 4};
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
constexpr bool fullEncounterLegalityEngineEnabled() noexcept { return false; }

} // namespace PokeBank::UIModel::Gen1Editor

#endif
