#pragma once
#include "UI/NarrowPokemonActionMenu.h"

// The Gen I / Gen II Pokemon Actions sheet is the shared narrow menu (UI/NarrowPokemonActionMenu.h):
// one vocabulary and one order for every generation (View, Edit, Clone, Release, Origin &
// Legality, Review Pending Changes, Cancel; empty slot: Add Pokemon, Review, Cancel). Only commands
// the staged editor really performs are listed. Moving a Pokemon stays the box-grid hold-Y carry;
// the Gen I/II native action sets have no Move command.
//
// Each row keeps its generation's native action index, so controller order, touch targets
// (touchIdBase + native index) and dispatch are unchanged. These are thin adapters kept so the
// Gen I/II call sites name their native action set.
namespace PokeBank::UIModel::ClassicPokemonActionMenu {

// Gen II (and any caller using the shared action set).
[[nodiscard]] constexpr NarrowPokemonMenu::Model fromShared(
    const SharedPokemonEditor::ActionSet& native) noexcept {
    return NarrowPokemonMenu::classic(native);
}

// Gen I. Its action table pads unused entries with Cancel; the shared builder lists it once.
[[nodiscard]] constexpr NarrowPokemonMenu::Model fromGen1(const Gen1Editor::ActionSet& native) noexcept {
    return NarrowPokemonMenu::gen1(native);
}

} // namespace PokeBank::UIModel::ClassicPokemonActionMenu
