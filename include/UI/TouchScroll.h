#pragma once

#include "UI/TouchInput.h"

#include <algorithm>
#include <cstdlib>

namespace UI {

struct TouchListVisual {
    int index = 0;
    int offset = 0;
};

// Visual-only pixel scroll for free-form information panes. The committed scroll remains exact and
// bounded; while a finger is held, the renderer may exceed either edge only through a short
// resistance-limited rubber band. This prevents a long drag from pulling the entire clipped pane
// off-screen and never mutates the committed model state.
inline int livePixelScrollVisual(int committedScroll, int fingerDelta,
                                 int maxScroll, int viewportH) noexcept {
    maxScroll = std::max(0, maxScroll);
    committedScroll = std::clamp(committedScroll, 0, maxScroll);
    const int requested = committedScroll - fingerDelta;
    const int edgeLimit = std::max(12, std::min(72, std::max(0, viewportH) / 4));
    if (requested < 0)
        return -std::min(edgeLimit, (-requested + 3) / 4);
    if (requested > maxScroll)
        return maxScroll + std::min(edgeLimit, (requested - maxScroll + 3) / 4);
    return requested;
}

// Draw-time direct manipulation for preserved one-column lists whose semantic/controller cursor
// still lands on release. While the finger is down, calculate the virtual row and keep the sub-row
// pixel remainder so content stays physically attached to the finger instead of waiting for release.
inline TouchListVisual liveVerticalListVisual(int selected, int count, int rowStep,
                                              int x, int y, int w, int h) noexcept {
    TouchListVisual out{std::max(0, selected), 0};
    if (count <= 0 || rowStep <= 0 || w <= 0 || h <= 0) return out;
    selected = std::clamp(selected, 0, count - 1);
    out.index = selected;

    const auto& touch = latestTouchGesture();
    const bool captured = touch.down &&
        touch.startX >= x && touch.startX < x + w &&
        touch.startY >= y && touch.startY < y + h;
    if (!captured) return out;

    const int requestedRows = touch.deltaY < 0
        ? (-touch.deltaY) / rowStep
        : -(touch.deltaY / rowStep);
    out.index = std::clamp(selected + requestedRows, 0, count - 1);
    const int appliedRows = out.index - selected;
    out.offset = touch.deltaY + appliedRows * rowStep;

    // Rubber-band the ends without ever disconnecting the content from the finger.
    const int edge = std::max(8, rowStep / 2);
    if (out.index == 0 && out.offset > edge)
        out.offset = edge + (out.offset - edge) / 4;
    if (out.index == count - 1 && out.offset < -edge)
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
        coasting_ = false;
        last_ = 0;
        offset_ = 0;
        velocity_ = 0;
    }

    void stop() noexcept { reset(); }

    [[nodiscard]] int offset() const noexcept { return offset_; }

    void updateVertical(const TouchInput& touch,
                        int x, int y, int w, int h,
                        int rowStep, int& index, int count,
                        int stride = 1) noexcept {
        update(touch, x, y, w, h, rowStep, index, count, stride);
    }

private:
    bool active_ = false;
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

    void update(const TouchInput& touch,
                int x, int y, int w, int h,
                int step, int& index, int count, int stride) noexcept {
        if (count <= 0 || step <= 0) {
            stop();
            return;
        }

        if (touch.justTouchedDown()) {
            active_ = inside(touch.x(), touch.y(), x, y, w, h);
            coasting_ = false;
            velocity_ = 0;
            // Stop momentum without snapping residual pixels out from under a new touch.
            // The existing offset is where the list was ACTUALLY drawn last frame.
            last_ = touch.y();
            return;
        }

        if (active_ && touch.isDown()) {
            const int now = touch.y();
            const int delta = now - last_;
            last_ = now;
            if (delta == 0) return;

            // Track from the first pixel of movement. Tap classification remains owned by TouchInput,
            // so a tiny wobble may visually move a few pixels but still resolves as a normal tap.
            offset_ += delta;

            // Live content remains exactly attached to the physical finger, but momentum must not
            // trust an arbitrarily large one-frame sample. A scheduler hiccup or noisy coordinate
            // must never turn release into a multi-page fling. Clamp only the velocity sample;
            // offset_ above deliberately keeps the full physical delta.
            const int maxVelocity = std::max(12, std::min(96, step * 2));
            const int velocitySample = std::clamp(delta, -maxVelocity, maxVelocity);
            // Respect the latest finger direction. A fast upward flick followed by a short
            // downward correction must not keep coasting upward on release simply because the
            // smoothed velocity still remembers the older, stronger motion.
            const bool reversed =
                (velocity_ < 0 && velocitySample > 0) ||
                (velocity_ > 0 && velocitySample < 0);
            velocity_ = reversed ? velocitySample : std::clamp(
                (velocity_ * 3 + velocitySample * 5) / 8,
                -maxVelocity, maxVelocity);
            rebalance(step, index, count, stride);
            return;
        }

        if (active_ && touch.justReleased()) {
            active_ = false;
            if (!touch.dragged()) {
                // A tap may stop a coast but must not jump the list on release either.
                // Ordinary idle easing below settles this small residual afterward.
                velocity_ = 0;
                coasting_ = false;
                return;
            }
            coasting_ = std::abs(velocity_) >= 2;
            return;
        }

        if (!touch.isDown() && coasting_) {
            offset_ += velocity_;
            rebalance(step, index, count, stride);
            velocity_ = (velocity_ * 7) / 8;
            if (std::abs(velocity_) <= 1) {
                velocity_ = 0;
                coasting_ = false;
            }
            return;
        }

        // Ease the remaining sub-row offset back to the semantic row once momentum is finished.
        if (!touch.isDown() && !active_ && offset_ != 0) {
            offset_ = (offset_ * 3) / 4;
            if (std::abs(offset_) <= 1) offset_ = 0;
        }
    }
};

} // namespace UI
