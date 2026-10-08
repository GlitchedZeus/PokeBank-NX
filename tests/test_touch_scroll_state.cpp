#include "UI/TouchScroll.h"

#include <cassert>
#include <cstdlib>
#include <iostream>

using UI::TouchInput;
using UI::TouchScrollState;

static void feed(TouchScrollState& state, TouchInput& touch,
                 int& firstRow, int count, int rowStep = 40) {
    state.updateVertical(touch, 0, 0, 400, 300, rowStep, firstRow, count);
}

static void testContinuousPixelAndBoundary() {
    TouchScrollState state;
    TouchInput touch;
    int first = 0;
    touch.contact(100, 200);
    feed(state, touch, first, 8);
    touch.contact(100, 193);
    feed(state, touch, first, 8);
    assert(first == 0 && state.offset() == -7);
    touch.contact(100, 160);
    feed(state, touch, first, 8);
    assert(first == 1 && state.offset() == 0);  // exactly one 40-pixel row
    touch.contact(100, 143);
    feed(state, touch, first, 8);
    assert(first == 1 && state.offset() == -17);
    // Draw location is -first*rowStep + residual, so the 57px movement is continuous.
    assert(first * 40 - state.offset() == 57);
    state.stop(); // controller input or screen transition cancels the whole gesture
    assert(state.offset() == 0);
}

static void testCoastReversalAndTapInterrupt() {
    TouchScrollState state;
    TouchInput touch;
    int first = 0;
    touch.contact(100, 200);
    feed(state, touch, first, 8);
    touch.contact(100, 160); // fast upward move, advances one row
    feed(state, touch, first, 8);
    touch.contact(100, 170); // short reverse: latest direction wins
    feed(state, touch, first, 8);
    assert(first == 1 && state.offset() == 10);
    touch.release();
    feed(state, touch, first, 8);
    const int residual = state.offset();
    touch.contact(100, 170);
    feed(state, touch, first, 8);
    assert(state.offset() == residual); // no jumping underneath a new finger
    touch.release();
    feed(state, touch, first, 8);
    assert(state.offset() == residual); // tap release preserves remaining pixels
}

static void testEmptySingleAndBothEnds() {
    TouchScrollState state;
    TouchInput touch;
    int first = 0;
    touch.contact(100, 250);
    feed(state, touch, first, 1);
    touch.contact(100, 0);
    feed(state, touch, first, 1);
    assert(first == 0 && std::abs(state.offset()) <= 40);
    touch.contact(100, 300);
    feed(state, touch, first, 1);
    assert(first == 0 && std::abs(state.offset()) <= 40);
    feed(state, touch, first, 0);
    assert(first == 0 && state.offset() == 0);
}

static void testDrawOnlyAndPixelBounds() {
    assert(UI::livePixelScrollVisual(80, 7, 400, 320) == 73);
    assert(UI::livePixelScrollVisual(0, 400, 400, 320) >= -72);
    auto& gesture = UI::writableFakeGesture();
    gesture.down = true;
    gesture.startX = 100;
    gesture.startY = 100;
    gesture.deltaY = 7;
    const auto mid = UI::liveVerticalListVisual(4, 8, 40, 0, 0, 400, 300);
    assert(mid.index == 4 && mid.offset == 7);
    gesture.deltaY = 900;
    const auto edge = UI::liveVerticalListVisual(0, 8, 40, 0, 0, 400, 300);
    assert(edge.index == 0 && std::abs(edge.offset) <= 40);
    gesture.down = false;
}

static void testStaleIndexClampedWithoutMotion() {
    TouchScrollState state;
    TouchInput touch;
    int row = -20;
    touch.contact(100, 200);
    feed(state, touch, row, 3);
    assert(row == 0);  // A new touch must not expose an invalid cursor.
    row = 99;
    touch.nextFrame();
    feed(state, touch, row, 3);
    assert(row == 2);  // Nor may an already-held stationary finger retain one.
}

static void testPartialGridStrideAtBoundary() {
    TouchScrollState state;
    TouchInput touch;
    int cell = 2;  // Column 2 is absent from the last row of a 5-item, 3-column grid.
    touch.contact(100, 200);
    state.updateVertical(touch, 0, 0, 400, 300, 40, cell, 5, 3);
    touch.contact(100, 100);
    state.updateVertical(touch, 0, 0, 400, 300, 40, cell, 5, 3);
    assert(cell == 2 && std::abs(state.offset()) <= 40);

    TouchScrollState other;
    TouchInput second;
    cell = 1;  // Column 1 does have a valid final-row neighbour.
    second.contact(100, 200);
    other.updateVertical(second, 0, 0, 400, 300, 40, cell, 5, 3);
    second.contact(100, 160);
    other.updateVertical(second, 0, 0, 400, 300, 40, cell, 5, 3);
    assert(cell == 4 && other.offset() == 0);
}

static void testRealIdleCoastAndTapCancellation() {
    TouchScrollState state;
    TouchInput touch;
    int row = 0;
    touch.contact(100, 200);
    feed(state, touch, row, 20);
    touch.contact(100, 140);
    feed(state, touch, row, 20);
    touch.release();
    feed(state, touch, row, 20);
    const int atRelease = row * 40 - state.offset();
    touch.nextFrame();  // The release edge is over; the next update is a true idle frame.
    feed(state, touch, row, 20);
    assert(row * 40 - state.offset() > atRelease);  // Momentum really advances content.

    const int atNewContact = state.offset();
    touch.contact(100, 140);
    feed(state, touch, row, 20);
    assert(state.offset() == atNewContact);  // A new touch interrupts coast without snapping.
    touch.release();
    feed(state, touch, row, 20);
    assert(state.offset() == atNewContact);
    touch.nextFrame();
    feed(state, touch, row, 20);
    assert(std::abs(state.offset()) <= std::abs(atNewContact));  // Idle settles the residual.
}

static void testIndependentPickerViewport() {
    UI::TouchPickerViewport window;
    window.ensure(17, 60, 9);
    assert(window.firstRow == 13);
    // Selection stays 17 while one real row moves underneath the finger.
    int selected = 17;
    TouchScrollState motion;
    TouchInput touch;
    const int oldY = (selected - window.firstRow) * 40;
    touch.contact(100, 200);
    motion.updateVertical(touch, 0, 0, 400, 360, 40, window.firstRow,
                          UI::TouchPickerViewport::maxFirst(60, 9) + 1);
    touch.contact(100, 160);
    motion.updateVertical(touch, 0, 0, 400, 360, 40, window.firstRow,
                          UI::TouchPickerViewport::maxFirst(60, 9) + 1);
    const int movedY = (selected - window.firstRow) * 40 + motion.offset();
    assert(selected == 17 && window.firstRow == 14 && motion.offset() == 0);
    assert(movedY == oldY - 40); // No selection-centered recenter/snap.
    window.ensure(selected, 60, 9);
    assert(window.firstRow == 14); // Draw-time ensure must not recentre after scroll.
    motion.stop();
    window.reveal(44, 60, 9);
    assert(window.firstRow == 36); // Controller navigation reveals focused row.

    UI::TouchPickerViewport grid;
    grid.ensure(1, 17, 3, 3);
    assert(grid.firstRow == 0);
    assert(UI::TouchPickerViewport::maxFirst(17, 3, 3) == 3);
    TouchScrollState gridMotion;
    TouchInput second;
    second.contact(100, 200);
    gridMotion.updateVertical(second, 0, 0, 400, 120, 40, grid.firstRow, 4);
    second.contact(100, 160);
    gridMotion.updateVertical(second, 0, 0, 400, 120, 40, grid.firstRow, 4);
    assert(grid.firstRow == 1); // Grid viewport scrolls by ROWS, not item indices.
    grid.ensure(1, 17, 3, 3);
    assert(grid.firstRow == 1);
    grid.reveal(16, 17, 3, 3);
    assert(grid.firstRow == 3); // Partially filled final row remains selectable.
    grid.reset();
    assert(grid.firstRow == -1);
    grid.ensure(1, 17, 3, 3);
    assert(grid.firstRow == 0); // New picker starts at its own focused value.
}

int main() {
    testContinuousPixelAndBoundary();
    testCoastReversalAndTapInterrupt();
    testEmptySingleAndBothEnds();
    testDrawOnlyAndPixelBounds();
    testStaleIndexClampedWithoutMotion();
    testPartialGridStrideAtBoundary();
    testRealIdleCoastAndTapCancellation();
    testIndependentPickerViewport();
    std::cout << "touch-scroll runtime geometry: PASS\n";
}
