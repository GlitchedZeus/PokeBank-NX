#pragma once
// Deterministic host-only touch samples for the shared scroll model. Never linked into Switch.
namespace UI {
struct TouchGestureSnapshot {
    bool down = false;
    int startX = 0;
    int startY = 0;
    int deltaY = 0;
};
inline TouchGestureSnapshot& writableFakeGesture() noexcept {
    static TouchGestureSnapshot value{};
    return value;
}
inline const TouchGestureSnapshot& latestTouchGesture() noexcept {
    return writableFakeGesture();
}
class TouchInput {
public:
    void contact(int x, int y) noexcept {
        const bool wasDown = down_;
        previousDown_ = down_;
        down_ = true;
        x_ = x;
        y_ = y;
        if (!wasDown) {
            firstX_ = x;
            firstY_ = y;
            maxSquared_ = 0;
        } else {
            const int dx = x - firstX_;
            const int dy = y - firstY_;
            const int distance = dx * dx + dy * dy;
            if (distance > maxSquared_) maxSquared_ = distance;
        }
    }
    void release() noexcept {
        previousDown_ = down_;
        down_ = false;
    }
    // Simulate a later frame without a new touch event. Without this transition the fake reports
    // justReleased() forever, preventing behavioral tests from exercising real idle/coast frames.
    void nextFrame() noexcept { previousDown_ = down_; }
    [[nodiscard]] bool isDown() const noexcept { return down_; }
    [[nodiscard]] bool justTouchedDown() const noexcept { return down_ && !previousDown_; }
    [[nodiscard]] bool justReleased() const noexcept { return !down_ && previousDown_; }
    [[nodiscard]] bool dragged() const noexcept { return maxSquared_ > 22 * 22; }
    [[nodiscard]] int x() const noexcept { return x_; }
    [[nodiscard]] int y() const noexcept { return y_; }
    [[nodiscard]] int deltaX() const noexcept { return x_ - firstX_; }
    [[nodiscard]] int deltaY() const noexcept { return y_ - firstY_; }
private:
    bool down_ = false;
    bool previousDown_ = false;
    int x_ = 0;
    int y_ = 0;
    int firstX_ = 0;
    int firstY_ = 0;
    int maxSquared_ = 0;
};
} // namespace UI
