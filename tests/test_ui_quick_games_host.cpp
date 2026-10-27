// Round 7 C04 (owner Oct 26: "also wants tap-outside dismissal on the Quick Games panel and other
// ordinary popups"): through the REAL SaveSelectScreen (Product Home) update / draw on the host
// harness. Y opens Quick Games; a completed tap outside the drawer closes it like B, nothing under
// the tap reacts (no other overlay, no exit, no menu destination), and a tap inside the drawer
// leaves it open. Product Home Help (Minus) closes the same way. The harness has no account and no
// saves (tests/host-ui/switch.h), so Product Home shows its empty state.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

struct Driver {
    UI::SaveSelectScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void frames(int n) { for (int i = 0; i < n; ++i) frame(); }
    void press(u64 button) {
        frame(button);
        frame();
    }
    void tapAt(int x, int y) {
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {true, static_cast<u32>(x + 2), static_cast<u32>(y + 1)};
        frame();
        HostUi::touch = {false, static_cast<u32>(x + 2), static_cast<u32>(y + 1)};
        frame();
    }
    bool drawn(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t.find(text) != std::string::npos) return true;
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

// Nothing under a dismissing tap may react: no other overlay, no exit, no menu destination.
void nothingUnderneath(const Driver& d, const UI::SaveSelectScreen::MainMenuDestination before,
                       const std::string& what) {
    check(!d.drawn("SWITCH PROFILE"), what + ": no profile picker opened underneath");
    check(!d.drawn("GAME LAUNCH"), what + ": no game-file picker opened underneath");
    check(!d.drawn("POKEBANK NX / HELP"), what + ": no Help opened underneath");
    check(!d.screen.shouldExit() && !d.screen.hasRequestedAppExit(), what + ": nothing exits");
    check(d.screen.getRequestedMainMenuDestination() == before, what + ": no destination chosen");
}

void quickGamesAndHelp() {
    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    PokeVault::Legacy::LegacySourceBindings bindings;
    UI::SaveSelectScreen screen(legacy, bindings);
    Driver d{screen, {}, {}};
    d.frames(3);
    const auto destination = screen.getRequestedMainMenuDestination();

    // Y opens Quick Games; let the card settle open.
    d.press(HidNpadButton_Y);
    d.frames(40);
    d.dump("Quick Games open");
    check(d.drawn("QUICK GAMES"), "Y opens Quick Games");
    // A tap inside the drawer, on its header above the game tiles (a tile tap opens that game),
    // leaves it open.
    d.tapAt(1000, 40);
    d.dump("after inside tap (immediate)");
    d.frames(40);
    d.dump("after inside tap");
    check(d.drawn("QUICK GAMES"), "a tap inside Quick Games leaves it open");
    // A tap on the Home card left of the drawer closes it, like B.
    d.tapAt(300, 300);
    d.frames(40);
    d.dump("after outside tap");
    check(!d.drawn("QUICK GAMES"), "a tap outside closes Quick Games");
    nothingUnderneath(d, destination, "Quick Games");
    // Product Home Help (Minus) closes the same way.
    d.press(HidNpadButton_Minus);
    d.frames(2);
    d.dump("after Minus");
    check(d.drawn("POKEBANK NX / HELP"), "Minus opens Product Home Help");
    d.tapAt(30, 40);
    d.frames(2);
    check(!d.drawn("POKEBANK NX / HELP"), "a tap outside closes Product Home Help");
    check(!d.drawn("QUICK GAMES"), "the Help-dismissing tap opens nothing underneath");
    nothingUnderneath(d, destination, "Help");
}

} // namespace

int main() {
    quickGamesAndHelp();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "C04 Quick Games / Product Home Help close on an outside tap through the real "
                 "SaveSelectScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
