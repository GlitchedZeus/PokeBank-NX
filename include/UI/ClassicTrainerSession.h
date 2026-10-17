#pragma once
#include <cstdint>
#include <string>

// The Trainer page (HOME Trainer card) for every generation: rows 0 Name and 1 Money.
//  - Footer: "A: Edit" is offered only when those fields really can be edited in this session
//    (a mutable workspace, or a classic staged Trainer editor); a read-only Trainer page never
//    shows an edit affordance (R2-TRAINER).
//  - Gen I and Gen II stage Name / Money through their staged editors (never the source save);
//    both use the same session flow below (UI::ClassicTrainerSession::handle).
namespace PokeBank::UIModel::ClassicTrainerSession {

inline constexpr int kRows = 2; // 0 Name, 1 Money

constexpr const char* trainerPageInstructions(bool editable) noexcept {
    return editable ? "Up/Down: Select  |  A: Edit  |  B: Back  |  +: Exit App"
                    : "Up/Down: Select  |  B: Back  |  +: Exit App";
}

} // namespace PokeBank::UIModel::ClassicTrainerSession
