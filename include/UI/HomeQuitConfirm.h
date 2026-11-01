#pragma once

#include <cstdint>

// Product Home quit guard (P0 #320). Leaving PokeBank NX is never a single unconfirmed
// button: B at the Home root only OPENS a visible "Quit PokeBank NX?" decision, and only an
// explicit Y inside that decision quits (the same Y-confirms-a-final-action rule as Release and
// Discard). A never exits the application: in the decision A and B both mean "Stay". Host-tested;
// the button bits are libnx HidNpadButton values (switch/services/hid.h: A=BIT(0), B=BIT(1),
// Y=BIT(3)) so this header needs no libnx include.
namespace PokeBank::UIModel::HomeQuit {

inline constexpr std::uint64_t kButtonA = 1ull << 0;
inline constexpr std::uint64_t kButtonB = 1ull << 1;
inline constexpr std::uint64_t kButtonY = 1ull << 3;

enum class Decision : std::uint8_t {
    None, // no decision this frame; the dialog stays open and nothing else runs
    Stay, // close the dialog, remain on Product Home
    Quit, // the user explicitly confirmed leaving PokeBank NX
};

// One decision per frame from the newly pressed buttons (edges, never held state).
// Y wins only when it is the sole decision button pressed: a mashed A+Y or B+Y frame is
// ambiguous and resolves to the safe choice, Stay.
[[nodiscard]] constexpr Decision decide(std::uint64_t pressed) noexcept {
    const bool stay = (pressed & (kButtonA | kButtonB)) != 0;
    const bool quit = (pressed & kButtonY) != 0;
    if (stay) return Decision::Stay;
    if (quit) return Decision::Quit;
    return Decision::None;
}

// Touch parity: the dialog's two glyph buttons (B Cancel, Y Quit / Launch) are its touch targets;
// a tap maps to that button through the shared glyph-hit path, so a tap is exactly a button press.
// Round 11 QUIT: the dialog draws no footer of its own (an empty nav bar covers Home's) and no
// "Stay" button; A remains an unlabelled Stay.

} // namespace PokeBank::UIModel::HomeQuit
