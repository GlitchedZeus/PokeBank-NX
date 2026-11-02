#pragma once
// One scrollbar for every scrolling surface in PokeBank NX (Phase 8, owner round-2 requirement).
//
// The component is a pure model plus one draw routine:
//   * Model     — the owner's scroll position in its own units (rows for row lists, pixels for
//                 free-form panes): content extent, viewport extent, current offset.
//   * layout()  — track + thumb rectangles from (first, visible, total). The thumb length is the
//                 viewport share of the content (never below kMinThumb); its position is the exact
//                 share of the remaining travel, so the thumb always reports the list position.
//   * State     — the geometry published by the last draw (hit-tested on the next frame, the same
//                 contract as touchButtons) plus the drag capture.
//   * handleTouch() — touch-and-hold the thumb and drag: the viewport follows the finger continuously.
//                 A tap on the track pages one viewport towards the finger. A press on the track
//                 that turns into a drag grabs the thumb at its centre (jump + drag). Releasing never
//                 selects anything: the owner only ever receives a new scroll offset, and the frame's
//                 touch is reported consumed so the rows under the finger cannot activate.
//   * paint()  — track + thumb; the thumb brightens while captured.
//
// Controller input is untouched: owners keep NavigationRepeat / D-pad / stick paths and may move
// the scroll between two drag frames; the next drag sample re-derives the offset from the finger,
// so a simultaneous button press never fights the thumb. Short lists (content <= viewport) publish
// no geometry at all: nothing is drawn and nothing is captured.
//
// Names: TrainerViewScreen.h and TrainerViewScreenCompositeOverlay.cpp define `update` and `draw` as
// macros around their includes, so this header deliberately uses handleTouch() / paint().
//
// Platform-neutral on purpose: no libnx, no TouchInput. Owners convert their TouchInput with
// sample() (a template, so the host fake and the Switch class both work).

#include "UI/Common.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>

namespace UI::SharedScrollbar {

inline constexpr int kTrackWidth = 6;    // drawn width of the track
inline constexpr int kMinThumb = 18;     // same minimum the legacy scrollThumb() used
inline constexpr int kTouchSlop = 18;    // finger hit area extends this far either side of the track
inline constexpr int kTapSlop = 22;      // same slop TouchInput uses for tap-vs-drag

struct Model {
    int content = 0;   // total extent (rows or pixels)
    int viewport = 0;  // visible extent (rows or pixels)
    int scroll = 0;    // current offset, nominally in [0, content - viewport]

    [[nodiscard]] constexpr int maxScroll() const noexcept { return std::max(0, content - viewport); }
    [[nodiscard]] constexpr bool overflows() const noexcept { return viewport > 0 && content > viewport; }
    [[nodiscard]] constexpr int clamped() const noexcept { return std::clamp(scroll, 0, maxScroll()); }
};

// Row lists: (total rows, visible rows, first visible row).
constexpr Model rows(std::size_t total, std::size_t visible, std::size_t first) noexcept {
    return {static_cast<int>(total), static_cast<int>(visible), static_cast<int>(first)};
}
constexpr Model rows(int total, int visible, int first) noexcept {
    return {std::max(0, total), std::max(0, visible), std::max(0, first)};
}
// Free-form panes: (content height, viewport height, pixel offset).
constexpr Model pixels(int contentH, int viewportH, int scrollPx) noexcept {
    return {std::max(0, contentH), std::max(0, viewportH), std::max(0, scrollPx)};
}

struct Layout {
    bool visible = false;   // false when the content fits: nothing drawn, nothing captured
    int x = 0, y = 0, w = 0, h = 0;   // track rectangle (screen pixels)
    int thumbY = 0, thumbH = 0;       // thumb rectangle, absolute
    int maxScroll = 0;
    Model model{};                    // the extents this geometry was published with

    [[nodiscard]] constexpr int travel() const noexcept { return std::max(0, h - thumbH); }
    [[nodiscard]] constexpr int thumbBottom() const noexcept { return thumbY + thumbH; }
};

// Track + thumb geometry. `x` is the track's left edge, [y, y + h] the viewport it scrolls.
constexpr Layout layout(int x, int y, int w, int h, const Model& m) noexcept {
    Layout out{};
    out.x = x; out.y = y; out.w = std::max(1, w); out.h = std::max(0, h);
    out.maxScroll = m.maxScroll();
    out.model = m;
    if (!m.overflows() || h <= 0) return out;
    out.visible = true;
    int length = static_cast<int>((static_cast<long long>(h) * m.viewport) / m.content);
    length = std::clamp(length, std::min(kMinThumb, h), h);
    out.thumbH = length;
    const int travel = h - length;
    const int scroll = m.clamped();
    // Rounded share of the travel, so first/last map to the exact track ends.
    out.thumbY = y + (out.maxScroll == 0 ? 0 :
        static_cast<int>((static_cast<long long>(travel) * scroll + out.maxScroll / 2) / out.maxScroll));
    return out;
}

// Inverse of layout(): the scroll offset whose thumb top is at `thumbTop` (absolute px).
constexpr int scrollForThumbTop(const Layout& l, int thumbTop) noexcept {
    const int travel = l.travel();
    if (!l.visible || travel <= 0 || l.maxScroll <= 0) return 0;
    const int rel = std::clamp(thumbTop - l.y, 0, travel);
    return static_cast<int>((static_cast<long long>(rel) * l.maxScroll + travel / 2) / travel);
}

constexpr bool hitTrack(const Layout& l, int px, int py) noexcept {
    return l.visible &&
           px >= l.x - kTouchSlop && px < l.x + l.w + kTouchSlop &&
           py >= l.y && py < l.y + l.h;
}

constexpr bool hitThumb(const Layout& l, int px, int py) noexcept {
    return hitTrack(l, px, py) && py >= l.thumbY && py < l.thumbBottom();
}

// Platform-neutral touch sample for one frame.
struct Touch {
    bool down = false;
    bool justDown = false;
    bool justReleased = false;
    int x = 0, y = 0;
    int startX = 0, startY = 0;
};

template <class TouchInputLike>
inline Touch sample(const TouchInputLike& t) noexcept {
    return {t.isDown(), t.justTouchedDown(), t.justReleased(),
            t.x(), t.y(), t.x() - t.deltaX(), t.y() - t.deltaY()};
}

struct State {
    Layout track;              // published by draw; hit-tested by the next frame's update
    bool dragging = false;     // finger holds the thumb
    bool armedTrack = false;   // finger went down on the track beside the thumb
    int grab = 0;              // finger y minus thumb top at capture
    int pressY = 0;

    void reset() noexcept { dragging = false; armedTrack = false; grab = 0; pressY = 0; }
    [[nodiscard]] bool capturing() const noexcept { return dragging || armedTrack; }
};

struct TouchResult {
    bool consumed = false;  // the frame's touch belongs to the scrollbar: skip list hit-testing
    bool changed = false;   // `scroll` differs from the model the owner passed in
    int scroll = 0;         // the offset the owner should adopt (always clamped)
};

// Run once per frame BEFORE the owner's own touch handling, with the owner's current model.
inline TouchResult handleTouch(State& s, const Touch& t, const Model& m) noexcept {
    TouchResult r{false, false, m.clamped()};
    const Layout& l = s.track;
    if (!l.visible || !m.overflows()) { s.reset(); return r; }
    const auto adopt = [&](int target) {
        target = std::clamp(target, 0, m.maxScroll());
        if (target != r.scroll) { r.scroll = target; r.changed = true; }
    };

    if (t.justDown) {
        s.reset();
        if (hitThumb(l, t.x, t.y)) {
            s.dragging = true;
            s.grab = t.y - l.thumbY;
            r.consumed = true;
        } else if (hitTrack(l, t.x, t.y)) {
            s.armedTrack = true;
            s.pressY = t.y;
            r.consumed = true;
        }
        return r;
    }

    if (s.dragging) {
        r.consumed = true;
        if (t.down) adopt(scrollForThumbTop(l, t.y - s.grab));
        if (!t.down) s.dragging = false;   // release: position already applied, nothing selected
        return r;
    }

    if (s.armedTrack) {
        r.consumed = true;
        if (t.down) {
            if (std::abs(t.y - s.pressY) > kTapSlop) {
                // A press beside the thumb that turns into a drag: jump the thumb under the finger
                // and keep dragging from there.
                s.armedTrack = false;
                s.dragging = true;
                s.grab = l.thumbH / 2;
                adopt(scrollForThumbTop(l, t.y - s.grab));
            }
            return r;
        }
        s.armedTrack = false;
        if (t.justReleased && hitTrack(l, t.x, t.y)) {
            // Tap on the track pages one viewport towards the finger.
            const int page = std::max(1, m.viewport);
            if (t.y < l.thumbY) adopt(r.scroll - page);
            else if (t.y >= l.thumbBottom()) adopt(r.scroll + page);
        }
        return r;
    }
    return r;
}

// Owners that only keep a scroll offset: the extents come from the geometry the last draw published.
inline TouchResult handleTouch(State& s, const Touch& t, int scroll) noexcept {
    Model m = s.track.model;
    m.scroll = scroll;
    return handleTouch(s, t, m);
}

// Convenience: owners that want the touch to be ignored for the rest of the frame while the
// scrollbar is captured (down frame through release) ask this instead of re-deriving it.
[[nodiscard]] inline bool owns(const State& s, const Touch& t) noexcept {
    return s.capturing() && (t.down || t.justReleased);
}

template <class Framebuffer>
inline void paint(Framebuffer& fb, const Layout& l, bool active = false) {
    if (!l.visible) return;
    const int radius = std::max(1, l.w / 2);
    fb.drawFilledRoundedRect(l.x, l.y, l.w, l.h, radius, Colors::Divider);
    fb.drawFilledRoundedRect(l.x, l.thumbY, l.w, l.thumbH, radius,
                             active ? Colors::Accent : Colors::TextDim);
}

// Layout + publish + draw in one call: the owner stores the geometry for next frame's update.
template <class Framebuffer>
inline void drawAndPublish(Framebuffer& fb, State& s, int x, int y, int w, int h, const Model& m) {
    s.track = layout(x, y, w, h, m);
    paint(fb, s.track, s.capturing());
}

} // namespace UI::SharedScrollbar
