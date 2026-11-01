// R2-LAUNCH: the leave decision ("Quit PokeBank NX?" / "Launch <game>?") is sized by
// SharedMenuGeometry::LaunchConfirm and every line fits it, measured with the bundled font:
// - all four emulator quit-route notices and long Gen I-V game names fit without clipping;
// - the frame sits above the footer, its button row holds two buttons (B Cancel, Y Quit / Launch;
//   Round 11 QUIT removed the redundant A Stay) inside the frame, each wide enough for its glyph and
//   label, and the frame is far smaller than the former fixed 1040x250 box;
// - the decision semantics stay frozen (only Y commits, A / B stay; HomeQuit::decide).
#include "UI/GameLaunchModel.h"
#include "UI/HomeQuitConfirm.h"
#include "UI/LeaveDecisionLayout.h"

#include <cassert>
#include <iostream>
#include <string>

namespace L = PokeBank::UIModel::LeaveDecisionLayout;
namespace G = PokeBank::UIModel::SharedMenuGeometry;
namespace Q = PokeBank::UIModel::HomeQuit;
using UI::GameLaunchQuitRoute;
using UI::gameLaunchQuitRouteNotice;

int main() {
    const G::Layout layout = G::layout(G::MenuType::LaunchConfirm, 0);
    assert(G::wellFormed(layout));
    assert(layout.rule.bodyLines == L::kLines);
    assert(layout.frame.w < 1040 && layout.frame.h <= 250); // proportional, not the fixed box
    for (int i = 0; i < 2; ++i) {
        const G::Rect b = G::buttonRect(layout, i, 2);
        assert(b.w >= 150 && b.inside(layout.frame));
        if (i) assert(!b.overlaps(G::buttonRect(layout, i - 1, 2)));
        // Glyph badge (~40 px) plus the longest label on either button fits with padding.
        for (const char* label : {"Cancel", "Launch", "Quit"})
            assert(G::fitsWidth(label, G::Text::Body, b.w - 64));
    }
    // Message lines sit between the header and the button row.
    assert(G::bodyLineTop(layout, L::kLines - 1) + G::kBodyLineH <= layout.buttons.y);

    const char* games[] = {"Pokemon Red", "Pokemon Crystal", "Pokemon LeafGreen",
                           "Pokemon Emerald", "Pokemon HeartGold", "Pokemon SoulSilver",
                           "Pokemon Platinum", "Pokemon Black 2", "Pokemon White 2"};
    for (const auto route : {GameLaunchQuitRoute::None, GameLaunchQuitRoute::LoaderDefault,
                             GameLaunchQuitRoute::EmulatorLauncher, GameLaunchQuitRoute::ReturnsToPokeBank}) {
        for (const char* game : games) {
            const L::Text t = L::build(false, game, gameLaunchQuitRouteNotice(route));
            if (!t.fits) std::cerr << "does not fit: " << game << " / " << gameLaunchQuitRouteNotice(route) << "\n";
            assert(t.fits);
            assert(t.heading == "Launch " + std::string(game) + "?");
            // The emulator-destination warning is kept verbatim across its wrapped lines.
            std::string joined = t.lines[1] + (t.lines[2].empty() ? "" : " " + t.lines[2]);
            assert(joined == gameLaunchQuitRouteNotice(route));
        }
    }
    const L::Text quit = L::build(true, "", "");
    assert(quit.fits && quit.heading == "Quit PokeBank NX?" && !quit.lines[1].empty());

    // A notice that cannot fit three lines is reported, never silently clipped.
    const L::Text overflow = L::build(false, "Pokemon Red", std::string(400, 'W'));
    assert(!overflow.fits);
    // A long real notice (Lane L's unproven-core warning + the quit route) keeps its start and
    // ends the second notice line with an ellipsis that still fits the column.
    const std::string longNotice =
        "The RetroArch core could not be proven from this save (per-core folder and file name disagree); "
        "PokeBank NX keeps its usual core choice, which may not load this save. Quitting the emulator "
        "returns to the Homebrew Menu; PokeBank NX does not reopen by itself.";
    const L::Text cut = L::build(false, "Pokemon SoulSilver", longNotice, "RetroArch");
    assert(!cut.fits);
    assert(cut.lines[1].rfind("The RetroArch core could not be proven", 0) == 0);
    assert(cut.lines[2].size() >= 3 && cut.lines[2].compare(cut.lines[2].size() - 3, 3, "\xE2\x80\xA6") == 0);
    assert(G::fitsWidth(cut.lines[2], G::Text::Caption, L::textColumnWidth()));

    // Frozen semantics: only Y commits; A and B stay; a mashed A+Y / B+Y stays.
    assert(Q::decide(Q::kButtonY) == Q::Decision::Quit);
    assert(Q::decide(Q::kButtonA) == Q::Decision::Stay && Q::decide(Q::kButtonB) == Q::Decision::Stay);
    assert(Q::decide(Q::kButtonA | Q::kButtonY) == Q::Decision::Stay);
    std::cout << "R2-LAUNCH leave decision: SharedMenuGeometry LaunchConfirm frame, every notice and game name "
                 "fits, B Cancel / Y buttons in frame, Y-only commit PASS\n";
}
