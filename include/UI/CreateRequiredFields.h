#pragma once

// Shared Create required-fields rule, Generation I-V (Round 11, owner H07 "this needs to be added
// for all gens", H28, Extra 6 "universal rule").
//
// A new Pokemon is not added until it has:
//  - a Met Location the user explicitly CHOSE from the exact game's own list, in Gen III, IV and V
//    (Gen V's H07 behaviour, now everywhere it applies). "Chosen" is a flag the Met Location picker
//    sets, never "value != 0": a real native Pokemon can carry location 0 (Gen III R/S/E index 0)
//    or a special / "----" / trade / event entry (Gen IV / V). Gen I has no met data and
//    Gold/Silver have none; Crystal's caught data stays optional because "no caught data" is
//    native for a traded Pokemon. No field is invented.
//  - at least one move, in every generation. This is a PokeBank APP POLICY, not a legality or
//    native-format rule: the formats store a moveless Pokemon without complaint, but PokeBank wants
//    every Pokemon it adds to be usable in battle. A new Create draft starts with no moves (the
//    user picks them), so this is checked at Keep/Add, and the message only asks for a choice; it
//    never calls the Pokemon illegal.
// There is deliberately NO move-count or "moves for this level" rule: the games generate up to
// four level-up moves and nothing limits a level 100 Pokemon to one or two.
//
// An existing Pokemon is never refused for history it already has; its Keep is refused only when
// an edit in this session took its last move away (a species change that left nothing usable).
//
// Pure functions: each generation's Keep/Add entry point asks before staging anything, and a
// refusal leaves the staged copy and the draft exactly as they were.

#include <cstddef>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::CreateRequiredFields {

struct Missing {
    bool metLocation = false;
    bool move = false;
    [[nodiscard]] constexpr bool any() const noexcept { return metLocation || move; }
};

template <class Moves>
[[nodiscard]] constexpr bool hasAnyMove(const Moves& moves) noexcept {
    for (std::size_t i = 0; i < 4; ++i)
        if (moves[i] != 0) return true;
    return false;
}

// Gen III-V store a Met Location every new Pokemon must have; Gen I and Gen II do not require one.
[[nodiscard]] constexpr bool metLocationRequired(int generation) noexcept {
    return generation >= 3 && generation <= 5;
}

template <class Moves>
[[nodiscard]] constexpr Missing forCreate(int generation, bool metLocationChosen, const Moves& moves) noexcept {
    return {metLocationRequired(generation) && !metLocationChosen, !hasAnyMove(moves)};
}

// Edit: refused only when this session emptied a moveset that had a move.
template <class Before, class After>
[[nodiscard]] constexpr Missing forEdit(const Before& before, const After& after) noexcept {
    return {false, hasAnyMove(before) && !hasAnyMove(after)};
}

// One status naming everything missing, e.g. "Choose a Met Location and at least one move before Keep".
// Empty when nothing is missing. The wording asks for a choice (app policy / required native
// field); it never implies the Pokemon is illegal. `action` is the generation's own commit word ("Keep" / "Add").
[[nodiscard]] inline std::string message(Missing missing, std::string_view action = "Keep") {
    if (!missing.any()) return {};
    std::string text = "Choose ";
    if (missing.metLocation) text += "a Met Location";
    if (missing.metLocation && missing.move) text += " and ";
    if (missing.move) text += "at least one move";
    text += " before ";
    text += action;
    return text;
}

} // namespace PokeBank::UIModel::CreateRequiredFields
