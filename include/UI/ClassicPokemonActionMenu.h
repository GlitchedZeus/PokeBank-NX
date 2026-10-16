#pragma once

#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/NarrowPokemonActionMenu.h"
#include "UI/SharedPokemonEditorContract.h"

// The Gen I / Gen II Pokemon Actions sheet. It lists only the commands the staged editor really
// performs, in the shared order View, Edit, Clone, Release (then Legality & Provenance, Review,
// Cancel). Shiny is edited inside the editor, never from the sheet, and the greyed placeholders
// for features these games do not have (Make Shiny, Master Vault, Bank, Transfer) are not shown.
// Moving a Pokemon stays the box-grid hold-Y carry; the sheet has no Move command for Gen I/II.
//
// Each row keeps its generation's native action index, so controller order, touch targets
// (touchIdBase + native index) and dispatch are unchanged; only presentation is narrowed.
namespace PokeBank::UIModel::ClassicPokemonActionMenu {

namespace detail {
constexpr const char* sharedLabel(SharedPokemonEditor::Action action) noexcept {
    using A = SharedPokemonEditor::Action;
    switch (action) {
        case A::View: return "View";
        case A::Edit: return "Edit";
        case A::Clone: return "Clone";
        case A::Remove: return "Release";
        case A::LegalityProvenance: return "Legality & Provenance";
        case A::Add: return "Add Pokemon";
        case A::Review: return "Review Pending Changes";
        case A::Close: return "Cancel";
        default: return nullptr; // Move / None: not a Gen I/II sheet command
    }
}
constexpr const char* gen1Label(Gen1Editor::Action action) noexcept {
    using A = Gen1Editor::Action;
    switch (action) {
        case A::View: return "View";
        case A::Edit: return "Edit";
        case A::Clone: return "Clone";
        case A::Remove: return "Release";
        case A::LegalityProvenance: return "Legality & Provenance";
        case A::AddPokemon: return "Add Pokemon";
        case A::ReviewPendingChanges: return "Review Pending Changes";
        case A::Cancel: return "Cancel";
        default: return nullptr;
    }
}
} // namespace detail

// Gen II (and any caller using the shared action set).
[[nodiscard]] constexpr NarrowPokemonMenu::Model fromShared(
    const SharedPokemonEditor::ActionSet& native) noexcept {
    NarrowPokemonMenu::Model model{};
    for (int i = 0; i < static_cast<int>(native.count); ++i)
        if (const char* label = detail::sharedLabel(native[static_cast<std::size_t>(i)]))
            model.append(label, i);
    return model;
}

// Gen I. Its action table pads unused entries with Cancel; only the first `count` are real, and
// a repeated Cancel is listed once.
[[nodiscard]] constexpr NarrowPokemonMenu::Model fromGen1(const Gen1Editor::ActionSet& native) noexcept {
    NarrowPokemonMenu::Model model{};
    bool cancelListed = false;
    for (int i = 0; i < static_cast<int>(native.count); ++i) {
        const auto action = native[static_cast<std::size_t>(i)];
        if (action == Gen1Editor::Action::Cancel) {
            if (cancelListed) continue;
            cancelListed = true;
        }
        if (const char* label = detail::gen1Label(action)) model.append(label, i);
    }
    return model;
}

} // namespace PokeBank::UIModel::ClassicPokemonActionMenu
