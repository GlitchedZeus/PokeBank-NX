#pragma once
// Host UI harness (never shipped): drives the REAL TrainerViewScreen::update / draw on the host with
// scripted controller and touch input. Build: Makefile.host.host-ui (the app's own sources, linked
// against tests/host-ui/switch.h and HostUiStubs.cpp). Each frame mirrors the device loop: input ->
// update(pad, touch) -> draw(fb), so a tap hit-tests the touch targets the previous draw published.
#include <switch.h>

#include <algorithm>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "Utils/Keyboard.h"

namespace HostUi {

inline std::vector<std::string> texts;                 // every string the last draw() rendered

// Layout recorder (Round 6 E05): the text each draw leaves VISIBLE, in paint order. A drawText
// records the ink band of its capitals and digits, cut to the active clip; a later opaque fill
// (rect, rounded rect, pill, card, gradient) hides any recorded text it touches, as on the device.
// A full-width fill on the footer band counts as a footer bar.
//
// Metrics follow the device font (PKSEFramebuffer kFontSizes 17 / 20 / 28 / 32 px for Caption /
// Body / Heading / Title; Nunito ascender 1.011 em, cap height 0.705 em, drawn NVG_ALIGN_TOP), so
// the band is [y + 0.306 em, y + 1.011 em]. Width is the stub's per-style advance (charWidth),
// the same value measureText reports, so measured and fixed layouts are judged alike.
struct TextRect {
    int x = 0, y = 0, w = 0, h = 0;
    std::string text;
    int style = -1;          // TextStyle as drawn (Round 7 typography checks)
    uint32_t rgba = 0;       // the drawn Color, packed as Color::toRGBA8
};
// Round 7 (lane C): every opaque rounded fill / pill of the frame, in paint order, with its colour
// and corner radius (-1 for a pill), so style tokens (row cards, backgrounds) can be compared.
struct Fill {
    int x = 0, y = 0, w = 0, h = 0, radius = 0;
    uint32_t rgba = 0;
};
inline std::vector<Fill> fills;
inline constexpr int kFontPx[4] = {17, 20, 28, 32};
inline constexpr int charWidth(int style) noexcept { return style == 0 ? 9 : style == 1 ? 10 : style == 2 ? 14 : 16; }
inline std::vector<TextRect> visibleText;
inline int footerBars = 0;
inline bool clipOn = false;
inline int clipX = 0, clipY = 0, clipW = 0, clipH = 0;
inline void resetLayout() {
    visibleText.clear();
    fills.clear();
    footerBars = 0;
    clipOn = false;
}
inline void recordText(int x, int y, const std::string& text, int style, uint32_t rgba = 0) {
    if (text.empty() || style < 0 || style > 3) return;
    const int px = kFontPx[style];
    int x0 = x, y0 = y + (px * 306 + 500) / 1000, x1 = x + static_cast<int>(text.size()) * charWidth(style);
    int y1 = y + (px * 1011 + 500) / 1000;
    if (clipOn) {
        x0 = std::max(x0, clipX); y0 = std::max(y0, clipY);
        x1 = std::min(x1, clipX + clipW); y1 = std::min(y1, clipY + clipH);
    }
    if (x1 <= x0 || y1 <= y0) return;
    visibleText.push_back({x0, y0, x1 - x0, y1 - y0, text, style, rgba});
}
inline void occlude(int x, int y, int w, int h, unsigned alpha) {
    if (alpha < 240 || w <= 0 || h <= 0) return;   // translucent scrims leave text readable
    std::erase_if(visibleText, [&](const TextRect& t) {
        return t.x < x + w && x < t.x + t.w && t.y < y + h && y < t.y + t.h;
    });
}
inline std::deque<Utils::NumberResult> numbers;         // answers for promptNumber, in order
inline std::deque<Utils::KeyboardResult> textAnswers;  // answers for promptText / promptSystemText
inline int promptCount = 0;                            // prompts shown so far
inline int japanesePromptCount = 0;                    // of which kana name prompts (promptJapaneseName)
inline int lastJapaneseMaxChars = 0;                   // the cap the last kana name prompt was opened with

inline Utils::NumberResult nextNumber() {
    ++promptCount;
    if (numbers.empty()) return {};
    auto next = numbers.front();
    numbers.pop_front();
    return next;
}
inline Utils::KeyboardResult nextText() {
    ++promptCount;
    if (textAnswers.empty()) return {};
    auto next = textAnswers.front();
    textAnswers.pop_front();
    return next;
}

} // namespace HostUi
