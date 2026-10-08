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

int main() {
    testContinuousPixelAndBoundary();
    testCoastReversalAndTapInterrupt();
    testEmptySingleAndBothEnds();
    testDrawOnlyAndPixelBounds();
    testStaleIndexClampedWithoutMotion();
    testPartialGridStrideAtBoundary();
    testRealIdleCoastAndTapCancellation();
    std::cout << "touch-scroll runtime geometry: PASS\n";
}
