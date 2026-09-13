#ifndef POKEBANK_UI_GEN1_POKEMON_EDITOR_CONTRACT_H
#define POKEBANK_UI_GEN1_POKEMON_EDITOR_CONTRACT_H

#include <array>
#include <cstddef>
#include <cstdint>

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

// One authoritative action-capability model for the R/B/Y boxed editor. The Switch overlay and host
// contract tests use the same ordering so an empty slot can never accidentally inherit mutation
// actions intended for an occupied boxed Pokemon.
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
    Cancel,
    StageAdd,
};

struct AddDraftDecision {
    bool mutateStagedSave = false;
    bool leaveDraft = false;
};

// Draft navigation and field editing are overlay-local only. Only the explicit final StageAdd event
// is permitted to request a semantic staged-save mutation; Cancel leaves without one.
constexpr AddDraftDecision addDraftDecision(AddDraftEvent event) noexcept {
    switch (event) {
        case AddDraftEvent::Navigate: return {false, false};
        case AddDraftEvent::EditField: return {false, false};
        case AddDraftEvent::Cancel: return {false, true};
        case AddDraftEvent::StageAdd: return {true, true};
    }
    return {};
}

enum class EditorSection : uint8_t { Summary, Moves, DVs, StatExp, Trainer };

constexpr std::array<EditorSection, 5> editorSections() noexcept {
    return {EditorSection::Summary, EditorSection::Moves, EditorSection::DVs,
            EditorSection::StatExp, EditorSection::Trainer};
}

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
