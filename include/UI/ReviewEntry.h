#pragma once

// Round 6 (B08, P1-RETURN, AUD-01, 4D-HELP follow-up): one visible way into the shared Review
// from every editor screen of every generation, and one rule for which button it is.
// Round 7 (owner Round 6 B08 (2): "Review also works by pressing + in the Party menu. This needs
// fixed to just the Y button, or whatever the correct button is in the current menu or window"):
// exactly ONE Review key per screen, and it is the key that screen's footer and Help show.
//
//   Y Review   where Y is free: Trainer Info (the accepted Gen V convention), Party, and Gen V
//              Boxes. + does nothing for Review there.
//   + Review   where Y already does needed work: Bag (Remove Item), Gen I-IV Boxes (Move / hold
//              for Multi), Create and Edit (re-roll, and Discard inside the exit confirmation).
//              Y keeps that job there and never opens Review.
//
// (The Gen V save hub keeps its + Review; the hub is not one of these screens. The Gen I-IV hub
// reaches Review through the staged exit menu.)
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

// + opens Review only where Y cannot (one key per screen).
[[nodiscard]] constexpr bool plusOpensReview(Screen screen, int generation) noexcept {
    return generation >= 1 && generation <= 5 && !yOpensReview(screen, generation);
}

// True when this frame's pressed buttons ask for Review on `screen`.
[[nodiscard]] constexpr bool opensReview(Screen screen, int generation, std::uint64_t down) noexcept {
    if (generation < 1 || generation > 5) return false;
    return yOpensReview(screen, generation) ? (down & kButtonY) != 0 : (down & kButtonPlus) != 0;
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
