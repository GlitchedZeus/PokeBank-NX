#pragma once

#include <cstdint>

// Review Pending Changes -> Discard Staged for the classic Gen I and Gen II editors.
// Discarding every staged change in the workspace is destructive, so (as in the Gen III/IV
// shared surface) the first Discard press only opens a "Discard all staged changes?" step and
// only an explicit Y there discards. B cancels; every other input, A included, is swallowed
// while the step is open so it can never export, browse or discard by accident.
namespace PokeBank::UIModel::Gen12ReviewDiscard {

enum class Outcome : uint8_t {
    NotHandled,  // no confirmation open and no Discard request: the review handles the input
    AskConfirm,  // the first Discard press opened the confirmation step
    Swallowed,   // the confirmation is open and this input did nothing
    Cancelled,   // B closed the confirmation; nothing changed
    Discard,     // Y confirmed: the caller discards every staged change now
};

struct State {
    bool confirming = false;
};

constexpr Outcome step(State& state, bool discardPressed, bool yPressed, bool bPressed) noexcept {
    if (!state.confirming) {
        if (!discardPressed) return Outcome::NotHandled;
        state.confirming = true;
        return Outcome::AskConfirm;
    }
    if (bPressed) {
        state.confirming = false;
        return Outcome::Cancelled;
    }
    if (yPressed) {
        state.confirming = false;
        return Outcome::Discard;
    }
    return Outcome::Swallowed;
}

constexpr const char* kTitle = "Discard all staged changes?";

} // namespace PokeBank::UIModel::Gen12ReviewDiscard
