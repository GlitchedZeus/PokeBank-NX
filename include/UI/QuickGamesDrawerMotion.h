#ifndef UI_QUICK_GAMES_DRAWER_MOTION_H
#define UI_QUICK_GAMES_DRAWER_MOTION_H
#include <algorithm>
#include <cstdlib>

namespace PokeBank::UIModel {
// Pure presentation/gesture model; never opens games or edits save data.
class QuickGamesDrawerMotion {
public:
    static constexpr int Width = 520;
    static constexpr int EdgeCapture = 128;
    int offset() const noexcept { return offset_; }
    bool dragging() const noexcept { return dragging_; }
    bool armed() const noexcept { return armed_; }
    bool settling() const noexcept { return settling_; }
    bool interactive() const noexcept { return offset_ == 0 && !dragging_ && !settling_; }
    bool fullyClosed() const noexcept {
        return offset_ == Width && target_ == Width && !armed_ && !settling_;
    }
    void snapOpen() noexcept {
        offset_ = target_ = 0;
        armed_ = dragging_ = settling_ = false;
    }
    void snapClosed() noexcept {
        offset_ = target_ = Width;
        armed_ = dragging_ = settling_ = false;
    }
    void openAnimated() noexcept {
        armed_ = dragging_ = false;
        target_ = 0;
        settling_ = offset_ != target_;
    }
    void closeAnimated() noexcept {
        armed_ = dragging_ = false;
        target_ = Width;
        settling_ = offset_ != target_;
    }
    // Closed: grab only physical right edge. Open: grab header, lip or tiles.
    bool arm(bool visible, int startX, int startY, int screenW, int contentBottom) noexcept {
        const int left = visible ? screenW - Width + offset_ : screenW - EdgeCapture;
        armed_ = screenW > Width && startX >= left && startX < screenW &&
                 startY >= 64 && startY < contentBottom;
        dragging_ = false;
        if (armed_) {
            initialX_ = startX;
            initialY_ = startY;
            initialOffset_ = offset_;
            resumeTarget_ = target_;
            settling_ = false;
            target_ = offset_;
        }
        return armed_;
    }
    bool track(int x, int y) noexcept {
        if (!armed_) return false;
        const int dx = x - initialX_;
        const int dy = y - initialY_;
        if (!dragging_) {
            if (std::abs(dx) < 14 || std::abs(dx) * 4 <= std::abs(dy) * 5)
                return false;
            if ((initialOffset_ == Width && dx > 0) ||
                (initialOffset_ == 0 && dx < 0))
                return false;
            dragging_ = true;
        }
        // Source position plus current displacement permits reversal mid-press.
        offset_ = std::clamp(initialOffset_ + dx, 0, Width);
        return true;
    }
    bool release(int x, int y) noexcept {
        if (!armed_) return false;
        const bool handled = dragging_;
        if (handled) {
            offset_ = std::clamp(initialOffset_ + x - initialX_, 0, Width);
            target_ = offset_ < Width / 2 ? 0 : Width;
            settling_ = offset_ != target_;
        } else {
            // A tap/vertical scroll interrupting an automatic animation must
            // not leave the drawer frozen halfway across the screen.
            target_ = resumeTarget_;
            settling_ = offset_ != target_;
        }
        armed_ = dragging_ = false;
        (void)y;
        return handled;
    }
    void advance() noexcept {
        if (!settling_ || armed_) return;
        const int delta = target_ - offset_;
        const int step = std::max(10, std::abs(delta) / 4);
        if (std::abs(delta) <= step) {
            offset_ = target_;
            settling_ = false;
        } else offset_ += delta > 0 ? step : -step;
    }
private:
    int offset_ = Width;
    int target_ = Width;
    int initialOffset_ = Width;
    int resumeTarget_ = Width;
    int initialX_ = 0;
    int initialY_ = 0;
    bool armed_ = false;
    bool dragging_ = false;
    bool settling_ = false;
};
} // namespace PokeBank::UIModel
#endif
