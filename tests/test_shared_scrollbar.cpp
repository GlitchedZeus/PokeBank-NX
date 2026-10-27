// Behavioural tests for include/UI/SharedScrollbar.h: the one draggable scrollbar every PokeBank NX
// list draws (Phase 8). Host-only; no libnx.
#include "UI/SharedScrollbar.h"
#include "UI/TouchInput.h"   // tests/touch_fakes: deterministic contact()/release()/nextFrame()

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace UI::SharedScrollbar;

namespace {

struct FakeFramebuffer {
    struct Rect { int x, y, w, h; UI::Color color; };
    std::vector<Rect> rects;
    void drawFilledRoundedRect(int x, int y, int w, int h, int, UI::Color c) { rects.push_back({x, y, w, h, c}); }
};

// A picker-like owner: rows as units, a selection that the scrollbar must never touch.
struct Owner {
    int total = 0, visible = 0, first = 0, selected = 0;
    State bar{};
    Owner(int t, int v, int f, int sel) : total(t), visible(v), first(f), selected(sel) {}
    TouchResult frame(UI::TouchInput& touch, int trackX = 600, int trackY = 100, int trackH = 400) {
        // handleTouch() runs with last frame's geometry, exactly like the app (update, then draw).
        const TouchResult r = handleTouch(bar, sample(touch), rows(total, visible, first));
        if (r.changed) first = r.scroll;
        FakeFramebuffer fb;
        drawAndPublish(fb, bar, trackX, trackY, kTrackWidth, trackH, rows(total, visible, first));
        return r;
    }
    void prime() { UI::TouchInput idle; frame(idle); }   // one draw so geometry exists
};

void testGeometryFirstMiddleLast() {
    const Layout top = layout(600, 100, 6, 400, rows(100, 10, 0));
    assert(top.visible && top.thumbY == 100 && top.thumbH == 40 && top.maxScroll == 90);
    const Layout last = layout(600, 100, 6, 400, rows(100, 10, 90));
    assert(last.thumbY + last.thumbH == 500);                 // thumb ends exactly at the track end
    const Layout mid = layout(600, 100, 6, 400, rows(100, 10, 45));
    assert(mid.thumbY == 100 + 180);                          // half the travel for half the range
    // Thumb position round-trips to the row it represents.
    for (int first = 0; first <= 90; first += 9) {
        const Layout l = layout(600, 100, 6, 400, rows(100, 10, first));
        assert(scrollForThumbTop(l, l.thumbY) == first);
    }
    // Pixel panes use the same math.
    const Layout px = layout(10, 20, 6, 300, pixels(900, 300, 600));
    assert(px.visible && px.thumbH == 100 && px.thumbY == 20 + 200);
    // Over-range scroll is clamped, never drawn past the track.
    const Layout over = layout(600, 100, 6, 400, rows(100, 10, 500));
    assert(over.thumbY + over.thumbH == 500);
    std::cout << "geometry first/middle/last ok\n";
}

void testShortListFillsNothingAndNeverDrags() {
    const Layout fits = layout(600, 100, 6, 400, rows(8, 10, 0));
    assert(!fits.visible && fits.thumbH == 0);
    const Layout exact = layout(600, 100, 6, 400, rows(10, 10, 0));
    assert(!exact.visible);
    FakeFramebuffer fb;
    paint(fb, fits);
    assert(fb.rects.empty());                                 // nothing drawn when the content fits

    Owner o{8, 10, 0, 3};
    o.prime();
    UI::TouchInput touch;
    touch.contact(602, 150);
    TouchResult r = o.frame(touch);
    assert(!r.consumed && !r.changed && !o.bar.capturing());
    touch.contact(602, 400);
    r = o.frame(touch);
    assert(!r.consumed && o.first == 0 && o.selected == 3);
    touch.release();
    r = o.frame(touch);
    assert(!r.consumed && !r.changed);
    // Minimum thumb: a very long list still gets a graspable thumb that stays inside the track.
    const Layout huge = layout(600, 100, 6, 400, rows(100000, 10, 99990));
    assert(huge.thumbH == kMinThumb && huge.thumbY + huge.thumbH == 500);
    std::cout << "short list ok\n";
}

void testThumbDragScrollsContinuouslyAndNeverSelects() {
    Owner o{100, 10, 0, 4};
    o.prime();
    assert(o.bar.track.visible && o.bar.track.thumbY == 100 && o.bar.track.thumbH == 40);
    UI::TouchInput touch;
    touch.contact(603, 120);                                  // finger on the thumb (grab 20px in)
    TouchResult r = o.frame(touch);
    assert(r.consumed && !r.changed && o.bar.dragging);
    touch.contact(603, 156);                                  // 36px of 360px travel = 9 rows
    r = o.frame(touch);
    assert(r.consumed && r.changed && o.first == 9);
    assert(o.bar.track.thumbY == 136);                        // thumb drawn under the finger
    touch.contact(603, 300);                                  // 180px = 45 rows
    r = o.frame(touch);
    assert(r.changed && o.first == 45);
    touch.contact(603, 290);                                  // dragging back works too
    r = o.frame(touch);
    assert(r.changed && (o.first == 42 || o.first == 43));
    const int before = o.first;
    touch.release();                                          // release over row content
    r = o.frame(touch);
    assert(r.consumed && !r.changed && !o.bar.dragging);      // consumed: no row tap may fire
    assert(o.first == before && o.selected == 4);             // nothing selected, viewport kept
    touch.nextFrame();
    r = o.frame(touch);
    assert(!r.consumed);                                      // idle frame: nothing owned
    std::cout << "thumb drag ok\n";
}

void testFastDragClampsAtBothEnds() {
    Owner o{100, 10, 0, 0};
    o.prime();
    UI::TouchInput touch;
    touch.contact(603, 110);
    o.frame(touch);
    touch.contact(603, 5000);                                 // one-frame fling far past the track
    TouchResult r = o.frame(touch);
    assert(r.changed && o.first == 90);                       // clamped at the last page
    assert(o.bar.track.thumbY + o.bar.track.thumbH == 500);
    touch.contact(603, -400);
    r = o.frame(touch);
    assert(r.changed && o.first == 0);
    touch.release();
    r = o.frame(touch);
    assert(r.consumed && o.first == 0);
    std::cout << "fast drag ok\n";
}

void testTrackTapPagesPredictably() {
    Owner o{100, 10, 40, 40};
    o.prime();
    const int thumbY = o.bar.track.thumbY;
    UI::TouchInput touch;
    touch.contact(603, thumbY + 120);                         // below the thumb
    TouchResult r = o.frame(touch);
    assert(r.consumed && o.bar.armedTrack && !r.changed);
    touch.release();
    r = o.frame(touch);
    assert(r.consumed && r.changed && o.first == 50);         // one viewport down
    touch.nextFrame();
    o.frame(touch);
    touch.contact(603, 101);                                  // above the thumb
    o.frame(touch);
    touch.release();
    r = o.frame(touch);
    assert(r.changed && o.first == 40);                       // one viewport up
    assert(o.selected == 40);                                 // selection untouched throughout
    // Paging near the end clamps instead of overshooting.
    o.first = 85; o.prime();
    touch.nextFrame();
    touch.contact(603, 499);
    o.frame(touch);
    touch.release();
    r = o.frame(touch);
    assert(o.first == 90);
    std::cout << "track tap paging ok\n";
}

void testTrackPressThatDragsJumpsThenFollows() {
    Owner o{100, 10, 0, 0};
    o.prime();
    UI::TouchInput touch;
    touch.contact(603, 400);                                  // well below the thumb
    TouchResult r = o.frame(touch);
    assert(r.consumed && o.bar.armedTrack);
    touch.contact(603, 430);                                  // past tap slop: becomes a drag
    r = o.frame(touch);
    assert(r.changed && o.bar.dragging && !o.bar.armedTrack);
    // Thumb centre lands under the finger: top = 430 - 20 = 410 -> (310/360)*90 = 77.5 -> 78
    assert(o.first == 78);
    touch.release();
    r = o.frame(touch);
    assert(r.consumed && !r.changed && o.first == 78);
    std::cout << "track press-drag jump ok\n";
}

void testTouchOutsideTrackIsNotOurs() {
    Owner o{100, 10, 0, 0};
    o.prime();
    UI::TouchInput touch;
    touch.contact(300, 150);                                  // on list rows, far from the track
    TouchResult r = o.frame(touch);
    assert(!r.consumed && !o.bar.capturing());
    touch.contact(300, 400);
    r = o.frame(touch);
    assert(!r.consumed && !r.changed && o.first == 0);
    touch.release();
    r = o.frame(touch);
    assert(!r.consumed);
    // Just inside the slop on the left counts; beyond it does not.
    touch.nextFrame();
    touch.contact(600 - kTouchSlop, 110);
    r = o.frame(touch);
    assert(r.consumed);
    touch.release(); o.frame(touch); touch.nextFrame();
    touch.contact(600 - kTouchSlop - 1, 110);
    r = o.frame(touch);
    assert(!r.consumed);
    std::cout << "outside track ok\n";
}

void testSimultaneousControllerInputDoesNotFightTheFinger() {
    Owner o{100, 10, 0, 0};
    o.prime();
    UI::TouchInput touch;
    touch.contact(603, 110);
    o.frame(touch);
    touch.contact(603, 200);                                  // 90px of 360px travel -> 22.5 -> 23
    TouchResult r = o.frame(touch);
    assert(r.changed && o.first == 23);
    // A D-pad press between two drag frames moves the viewport and selection by the owner's rules.
    o.selected = 80; o.first = 71;
    touch.contact(603, 201);                                  // finger barely moves
    r = o.frame(touch);
    assert(o.bar.dragging && r.consumed);
    assert(o.first == 23);                                    // the finger wins the viewport again
    assert(o.selected == 80);                                 // the controller's selection survives
    touch.release();
    r = o.frame(touch);
    assert(r.consumed && !r.changed);
    // The list shrinks under a capture: the capture is dropped, nothing explodes.
    touch.nextFrame();
    touch.contact(603, o.bar.track.thumbY + 5);
    o.frame(touch);
    assert(o.bar.dragging);
    o.total = 5;
    touch.contact(603, 300);
    r = o.frame(touch);
    assert(!r.consumed && !o.bar.capturing() && r.scroll == 0);
    std::cout << "simultaneous controller input ok\n";
}

void testOwnsHelperAndDraw() {
    State s;
    s.track = layout(600, 100, 6, 400, rows(100, 10, 0));
    Touch t;
    assert(!owns(s, t));
    s.dragging = true; t.down = true;
    assert(owns(s, t));
    t.down = false; t.justReleased = true;
    assert(owns(s, t));
    t.justReleased = false;
    assert(!owns(s, t));

    FakeFramebuffer fb;
    paint(fb, s.track, false);
    assert(fb.rects.size() == 2);                             // track + thumb
    assert(fb.rects[0].h == 400 && fb.rects[1].h == 40 && fb.rects[1].y == 100);
    assert(fb.rects[1].color == UI::Colors::TextDim);
    fb.rects.clear();
    paint(fb, s.track, true);
    assert(fb.rects[1].color == UI::Colors::Accent);         // captured thumb is highlighted
    std::cout << "owns/draw ok\n";
}

void testPixelPaneDrag() {
    // A free-form pane (content 1200px, viewport 300px) dragged by the thumb.
    State s;
    int scroll = 0;
    auto step = [&](const Touch& t) {
        const TouchResult r = handleTouch(s, t, pixels(1200, 300, scroll));
        if (r.changed) scroll = r.scroll;
        s.track = layout(400, 50, 6, 300, pixels(1200, 300, scroll));
        return r;
    };
    s.track = layout(400, 50, 6, 300, pixels(1200, 300, scroll));
    assert(s.track.thumbH == 75);
    assert(s.track.model.content == 1200 && s.track.model.viewport == 300);
    TouchResult r = step({true, true, false, 402, 60, 402, 60});
    assert(r.consumed && s.dragging);
    r = step({true, false, false, 402, 60 + 225, 402, 60});   // full travel
    assert(r.changed && scroll == 900);
    r = step({false, false, true, 402, 285, 402, 60});
    assert(r.consumed && !s.dragging && scroll == 900);
    std::cout << "pixel pane ok\n";
}

void testScrollOnlyOverloadUsesPublishedExtents() {
    State s;
    int scroll = 30;
    s.track = layout(600, 100, 6, 400, rows(100, 10, scroll));
    TouchResult r = handleTouch(s, {true, true, false, 603, s.track.thumbY + 10, 603, s.track.thumbY + 10}, scroll);
    assert(r.consumed && s.dragging);
    r = handleTouch(s, {true, false, false, 603, s.track.thumbY + 10 + 36, 603, s.track.thumbY + 10}, scroll);
    assert(r.changed && r.scroll == 39);                      // +36px of 360px travel = +9 rows
    scroll = r.scroll;
    r = handleTouch(s, {false, false, true, 603, 0, 603, 0}, scroll);
    assert(r.consumed && !r.changed && r.scroll == 39);
    // Without a published geometry nothing is captured.
    State fresh;
    r = handleTouch(fresh, {true, true, false, 603, 120, 603, 120}, 0);
    assert(!r.consumed && !r.changed);
    std::cout << "scroll-only overload ok\n";
}

} // namespace

int main() {
    testGeometryFirstMiddleLast();
    testShortListFillsNothingAndNeverDrags();
    testThumbDragScrollsContinuouslyAndNeverSelects();
    testFastDragClampsAtBothEnds();
    testTrackTapPagesPredictably();
    testTrackPressThatDragsJumpsThenFollows();
    testTouchOutsideTrackIsNotOurs();
    testSimultaneousControllerInputDoesNotFightTheFinger();
    testOwnsHelperAndDraw();
    testPixelPaneDrag();
    testScrollOnlyOverloadUsesPublishedExtents();
    std::cout << "test_shared_scrollbar: all passed\n";
    return 0;
}
