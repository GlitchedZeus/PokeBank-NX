#pragma once

#include <cstdint>

// "Y Random DVs" in the Gen I and Gen II Edit/Create workspaces rewrites every DV at once, which
// can also change shininess, gender (Gen II), stats and Hidden Power. It is a bulk change, so Y
// first opens a confirmation naming those consequences; only A or Y there re-rolls the draft, and
// B cancels with nothing changed. Every other input is swallowed while the prompt is open.
namespace PokeBank::UIModel::Gen12RerollDVs {

enum class Outcome : uint8_t {
    NotHandled,  // no prompt open and no Y: the workspace handles the input
    AskConfirm,  // Y opened the prompt
    Swallowed,   // prompt open, input ignored
    Cancelled,   // B closed the prompt; nothing changed
    Reroll,      // A or Y confirmed: re-roll the working draft's DVs now
};

struct State {
    bool confirming = false;
};

constexpr Outcome step(State& state, bool yPressed, bool aPressed, bool bPressed) noexcept {
    if (!state.confirming) {
        if (!yPressed) return Outcome::NotHandled;
        state.confirming = true;
        return Outcome::AskConfirm;
    }
    if (bPressed) {
        state.confirming = false;
        return Outcome::Cancelled;
    }
    if (aPressed || yPressed) {
        state.confirming = false;
        return Outcome::Reroll;
    }
    return Outcome::Swallowed;
}

constexpr const char* kTitle = "Re-roll all DVs?";
constexpr const char* kConsequence = "Shiny/gender may change.";

} // namespace PokeBank::UIModel::Gen12RerollDVs
