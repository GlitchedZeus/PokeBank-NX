#pragma once

#include "UI/TouchInput.h"

#include <algorithm>
#include <cstdlib>

namespace UI {

struct TouchListVisual {
    int index = 0;
    int offset = 0;
    bool tracking = false;
};

// Draw-time direct manipulation for preserved screens whose semantic/controller cursor still lands
// on release. While the finger is down, calculate the virtual row/cell and keep the sub-row pixel
// remainder so content stays physically attached to the finger instead of waiting for release.
// `stride` is 1 for a normal list and the column count for a vertically scrolling grid.
inline TouchListVisual liveVerticalListVisual(int selected, int count, int rowStep,
                                              int x, int y, int w, int h,
                                              int stride = 1) noexcept {
    TouchListVisual out{std::max(0, selected), 0, false};
    if (count <= 0 || rowStep <= 0 || w <= 0 || h <= 0) return out;
    selected = std::clamp(selected, 0, count - 1);
    out.index = selected;

    const auto& touch = latestTouchGesture();
    const bool captured = touch.down &&
        touch.startX >= x && touch.startX < x + w &&
        touch.startY >= y && touch.startY < y + h;
    if (!captured) return out;

    stride = std::max(1, stride);
    const int selectedUnit = selected / stride;
    const int selectedColumn = selected % stride;
    const int maxUnit = (count - 1) / stride;
    const int requestedUnits = touch.deltaY < 0
        ? (-touch.deltaY) / rowStep
        : -(touch.deltaY / rowStep);
    const int visualUnit = std::clamp(selectedUnit + requestedUnits, 0, maxUnit);
    out.index = std::min(count - 1, visualUnit * stride + selectedColumn);
    const int appliedUnits = visualUnit - selectedUnit;
    out.offset = touch.deltaY + appliedUnits * rowStep;
    out.tracking = true;

    // Rubber-band the ends without ever disconnecting the content from the finger.
    const int edge = std::max(8, rowStep / 2);
    if (visualUnit == 0 && out.offset > edge)
        out.offset = edge + (out.offset - edge) / 4;
    if (visualUnit == maxUnit && out.offset < -edge)
        out.offset = -edge + (out.offset + edge) / 4;
    return out;
}

// Pixel-first touch scrolling for controller-oriented lists.
//
// The selection index remains the semantic/controller cursor, but the visible content carries a
// residual pixel offset while the finger is moving. Crossing one row advances the semantic cursor
// and keeps the residual pixels, so the list never snaps to row boundaries under the finger.
// A short velocity tail after release gives long lists a light phone/tablet-style coast without
// turning a drag into a tap or bypassing the existing controller action path.
class TouchScrollState {
public:
    void reset() noexcept {
        active_ = false;
        moved_ = false;
        coasting_ = false;
        last_ = 0;
        offset_ = 0;
        velocity_ = 0;
    }

    void stop() noexcept {
        active_ = false;
        moved_ = false;
        coasting_ = false;
        offset_ = 0;
        velocity_ = 0;
    }

    [[nodiscard]] int offset() const noexcept { return offset_; }

    bool updateVertical(const TouchInput& touch,
                        int x, int y, int w, int h,
                        int rowStep, int& index, int count,
                        int stride = 1) noexcept {
        return update(touch, true, x, y, w, h, rowStep, index, count, stride);
    }

private:
    bool active_ = false;
    bool moved_ = false;
    bool coasting_ = false;
    int last_ = 0;
    int offset_ = 0;
    int velocity_ = 0;

    static bool inside(int px, int py, int x, int y, int w, int h) noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    void rebalance(int step, int& index, int count, int stride) noexcept {
        if (count <= 0 || step <= 0) {
            stop();
            return;
        }
        stride = std::max(1, stride);
        index = std::clamp(index, 0, count - 1);

        // Fold whole row/cell crossings into the semantic cursor in O(1). For grids, stride is
        // the column count: only same-lane entries are valid vertical neighbours. Never clamp to
        // count-1 here, because a partially filled final row would silently shift the selection
        // sideways while the user was dragging vertically.
        if (offset_ <= -step) {
            const int requested = (-offset_) / step;
            const int available = std::max(0, (count - 1 - index) / stride);
            const int crossed = std::min(requested, available);
            index += crossed * stride;
            offset_ += crossed * step;
        }
        if (offset_ >= step) {
            const int requested = offset_ / step;
            const int available = std::max(0, index / stride);
            const int crossed = std::min(requested, available);
            index -= crossed * stride;
            offset_ -= crossed * step;
        }

        const bool atLeadingEdge = index - stride < 0;
        const bool atTrailingEdge = index + stride >= count;

        // Small rubber-band resistance at either end. It remains visibly attached to the finger,
        // but cannot be pulled far enough to expose large empty regions.
        const int edge = std::max(8, step / 2);
        if (atLeadingEdge && offset_ > edge) {
            offset_ = edge + (offset_ - edge) / 4;
            velocity_ /= 2;
        }
        if (atTrailingEdge && offset_ < -edge) {
            offset_ = -edge + (offset_ + edge) / 4;
            velocity_ /= 2;
        }
    }

    bool update(const TouchInput& touch, bool vertical,
                int x, int y, int w, int h,
                int step, int& index, int count, int stride) noexcept {
        if (count <= 0 || step <= 0) {
            stop();
            return false;
        }

        if (touch.justTouchedDown()) {
            active_ = inside(touch.x(), touch.y(), x, y, w, h);
            moved_ = false;
            coasting_ = false;
            velocity_ = 0;
            offset_ = 0;
            last_ = vertical ? touch.y() : touch.x();
            return false;
        }

        if (active_ && touch.isDown()) {
            const int now = vertical ? touch.y() : touch.x();
            const int delta = now - last_;
            last_ = now;
            if (delta == 0) return false;

            // Track from the first pixel of movement. Tap classification remains owned by TouchInput,
            // so a tiny wobble may visually move a few pixels but still resolves as a normal tap.
            offset_ += delta;
            velocity_ = (velocity_ * 3 + delta * 5) / 8;
            moved_ = moved_ || touch.dragged();
            rebalance(step, index, count, stride);
            return true;
        }

        if (active_ && touch.justReleased()) {
            active_ = false;
            const bool wasDrag = moved_ || touch.dragged();
            moved_ = false;
            if (!wasDrag) {
                offset_ = 0;
                velocity_ = 0;
                coasting_ = false;
                return false;
            }
            coasting_ = std::abs(velocity_) >= 2;
            return true;
        }

        if (!touch.isDown() && coasting_) {
            offset_ += velocity_;
            rebalance(step, index, count, stride);
            velocity_ = (velocity_ * 7) / 8;
            if (std::abs(velocity_) <= 1) {
                velocity_ = 0;
                coasting_ = false;
            }
            return true;
        }

        // Ease the remaining sub-row offset back to the semantic row once momentum is finished.
        if (!touch.isDown() && !active_ && offset_ != 0) {
            offset_ = (offset_ * 3) / 4;
            if (std::abs(offset_) <= 1) offset_ = 0;
            return true;
        }
        return false;
    }
};

} // namespace UI
