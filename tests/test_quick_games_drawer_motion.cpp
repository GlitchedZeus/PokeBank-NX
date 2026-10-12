#include <cassert>
#include <iostream>
#include "UI/QuickGamesDrawerMotion.h"
using PokeBank::UIModel::QuickGamesDrawerMotion;
static void settle(QuickGamesDrawerMotion& m) {
    for (int i = 0; i < 100 && m.settling(); ++i) m.advance();
    assert(!m.settling());
}
int main() {
    QuickGamesDrawerMotion m;
    assert(m.fullyClosed());
    assert(m.arm(false, 1260, 190, 1280, 664));
    assert(!m.track(1262, 195));
    assert(m.track(940, 195) && m.offset() == 200);
    assert(m.track(1100, 195) && m.offset() == 360); // reverse in same press
    assert(m.track(890, 195) && m.offset() == 150);
    assert(m.release(890, 195)); settle(m);
    assert(m.interactive());
    assert(m.arm(true, 780, 185, 1280, 664));
    assert(m.track(960, 185) && m.offset() == 180);
    assert(m.release(960, 185)); settle(m);
    assert(m.interactive()); // short pull snaps open
    assert(m.arm(true, 900, 210, 1280, 664));
    assert(m.track(1200, 210) && m.offset() == 300);
    assert(m.release(1200, 210)); settle(m);
    assert(m.fullyClosed());
    m.snapOpen();
    assert(m.arm(true, 950, 260, 1280, 664));
    assert(!m.track(960, 410)); // vertical list scroll not hijacked
    assert(!m.release(960, 410) && m.offset() == 0);
    assert(!m.arm(false, 850, 120, 1280, 664)); // outside physical edge
    assert(!m.arm(false, 1270, 690, 1280, 664)); // footer
    m.snapClosed(); m.openAnimated(); m.advance();
    const int mid = m.offset();
    assert(!m.arm(true, 1000, 210, 1280, 664)); // behind card
    assert(m.arm(true, 1190, 210, 1280, 664));
    assert(m.track(1260, 210) && m.offset() == mid + 70);
    assert(m.release(1260, 210)); settle(m);
    std::cout << "quick_games_drawer_motion PASS\n";
}
