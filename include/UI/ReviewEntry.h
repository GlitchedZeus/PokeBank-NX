#pragma once

// Round 6 (B08, P1-RETURN, AUD-01, 4D-HELP follow-up): one visible way into the shared Review
// from every editor screen of every generation, and one rule for which button it is.
//
//   + Review   on every screen below, in Gen I-V. Gen V already used + for Review (Bag, Party,
//              Boxes, hub); Gen I-IV Boxes/Party had + doing nothing ("tools are on Product
//              Home"), and the Gen I-III Bag's + opened an Options menu whose only other rows
//              (Export Copy, Discard) are X and Y inside Review itself. So + now opens Review
//              directly everywhere and the Bag's Review is no longer buried.
//   Y Review   as well, where Y is free: Trainer Info (the accepted Gen V convention), Party,
//              and Gen V Boxes. Y is NOT taken from screens where it already does needed work:
//              Bag (Remove Item), Gen I-IV Boxes (Move / hold for Multi), Create and Edit
//              (re-roll, and Discard inside the exit confirmation). Those screens show + Review.
//
// Review opened from a screen returns to that screen on B or Close (ClassicReviewOrigin for
// Gen I-IV, Gen5SharedScreenState's return surface for Gen V). The four-option staged exit is
// unchanged. Pure and host-tested (tests/test_review_entry.cpp).

#include <cstdint>

namespace PokeBank::UIModel::ReviewEntry {

enum class Screen : unsigned char { Trainer, Party, Boxes, Bag, Create, Edit };

// libnx HidNpadButton values, restated so the rule is testable off-console.
inline constexpr std::uint64_t kButtonY = 1ull << 3;
inline constexpr std::uint64_t kButtonPlus = 1ull << 10;

// Y opens Review only where Y has no other job on that screen.
[[nodiscard]] constexpr bool yOpensReview(Screen screen, int generation) noexcept {
    switch (screen) {
        case Screen::Trainer:
        case Screen::Party:  return true;
        case Screen::Boxes:  return generation == 5;   // Gen I-IV Boxes: Y is Move / Multi
        case Screen::Bag:                              // Y is Remove Item
        case Screen::Create:                           // Y is re-roll / confirm Discard
        case Screen::Edit:   return false;
    }
    return false;
}

[[nodiscard]] constexpr bool plusOpensReview(Screen, int generation) noexcept {
    return generation >= 1 && generation <= 5;
}

// True when this frame's pressed buttons ask for Review on `screen`.
[[nodiscard]] constexpr bool opensReview(Screen screen, int generation, std::uint64_t down) noexcept {
    if (generation < 1 || generation > 5) return false;
    if ((down & kButtonPlus) && plusOpensReview(screen, generation)) return true;
    return (down & kButtonY) && yOpensReview(screen, generation);
}

struct Hint {
    const char* glyph;
    const char* label;
};

// The one footer entry each screen shows for Review.
[[nodiscard]] constexpr Hint footerHint(Screen screen, int generation) noexcept {
    return {yOpensReview(screen, generation) ? "Y" : "+", "Review"};
}

// Old-style text footers ("A: Edit | ... | B: Back") use this segment.
[[nodiscard]] constexpr const char* footerText(Screen screen, int generation) noexcept {
    return yOpensReview(screen, generation) ? "Y: Review" : "+: Review";
}

} // namespace PokeBank::UIModel::ReviewEntry
