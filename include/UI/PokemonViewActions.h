#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeBank::UIModel {
// A passive presenter exposes one semantic action. Controller and future touch
// dispatch use the same action; editor buttons never become passive actions.
enum class PassiveViewAction : uint8_t { None, Back };
constexpr int passiveBackTarget = 9001;
constexpr PassiveViewAction passiveViewAction(bool controllerBack, int touchTarget = -1) noexcept {
    return controllerBack || touchTarget == passiveBackTarget ? PassiveViewAction::Back : PassiveViewAction::None;
}
namespace Gen2PokemonEditor {
enum class SlotAction : uint8_t { View, Edit, Clone, Shiny, Add, Review, Close, None };
constexpr std::array occupiedActions{SlotAction::View, SlotAction::Edit, SlotAction::Clone,
    SlotAction::Shiny, SlotAction::Add, SlotAction::Review, SlotAction::Close};
constexpr std::array emptyActions{SlotAction::Add, SlotAction::Review, SlotAction::Close};
constexpr size_t slotActionCount(bool occupied) noexcept {
    return occupied ? occupiedActions.size() : emptyActions.size();
}
constexpr SlotAction slotActionAt(bool occupied, size_t row) noexcept {
    if (row >= slotActionCount(occupied)) return SlotAction::None;
    return occupied ? occupiedActions[row] : emptyActions[row];
}
} // namespace Gen2PokemonEditor
} // namespace PokeBank::UIModel
