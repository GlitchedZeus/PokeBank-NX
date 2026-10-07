#include <cassert>

#include "UI/ControllerModel.h"

int main() {
    using namespace PokeBank::UIModel;

    const ControllerBinding* plus = findBinding(ControllerContext::SelectGame, ControllerButton::Plus);
    assert(plus && plus->intent == ControllerIntent::Options);
    const ControllerBinding* minus = findBinding(ControllerContext::SelectGame, ControllerButton::Minus);
    assert(minus && minus->intent == ControllerIntent::Help);

    plus = findBinding(ControllerContext::PokemonSummary, ControllerButton::Plus);
    assert(plus && plus->intent == ControllerIntent::TransferCompatibility);
    plus = findBinding(ControllerContext::CarryingPokemon, ControllerButton::Plus);
    assert(plus && plus->intent == ControllerIntent::More);

    // Context-aware hints never advertise inactive controls.
    assert(findBinding(ControllerContext::SelectGame, ControllerButton::X) == nullptr);
    assert(findBinding(ControllerContext::PokemonActionSheet, ControllerButton::Plus) == nullptr);
    assert(findBinding(ControllerContext::PokemonActionSheet, ControllerButton::Minus) == nullptr);
    assert(controllerBindings(ControllerContext::SelectGame).size() == 3);
    assert(controllerBindings(ControllerContext::SelectGameMultipleUsers).size() == 4);

    for (const ControllerBinding& binding : controllerBindings(ControllerContext::PokemonActionSheet))
        assert(!binding.label.empty());
}
