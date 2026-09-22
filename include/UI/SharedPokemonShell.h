#pragma once
#include "UI/ClassicGameContext.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Common.h"
#include "UI/SpriteManager.h"
#include "UI/StatsRadar.h"
#include "Names/SpeciesNames.h"

namespace UI::SharedPokemonShell {
namespace ClassicContext = PokeBank::UIModel::ClassicGameContext;
struct Geometry {
    int y = 96, h;
    int leftX = 24, leftW = 300, midX = 338, midW = 398, rightX = 750, rightW;
    constexpr Geometry(int width, int height, int navBarHeight)
        : h(height - 96 - navBarHeight - 8), rightW(width - 24 - 750) {}
};
inline std::string dexLabel(uint16_t species) {
    auto value = std::to_string(species);
    while (value.size() < 3) value = "0" + value;
    return value;
}
inline void drawHeaderGender(PKSEFramebuffer& fb, int x, int y, uint8_t gender) {
    if (gender == 0) fb.drawSymbol(x, y, "♂", g_themeMode == ThemeMode::Light ? Color(24,112,184) : Color(92,205,255));
    else if (gender == 1) fb.drawSymbol(x, y, "♀", Colors::Magenta);
}
inline void drawChrome(PKSEFramebuffer& fb, uint16_t species, const std::string& nickname,
    uint8_t level, uint8_t gender, bool shiny, ClassicContext::SurfaceMode surfaceMode,
    const std::string& recordContext, const std::string& sourceGameId) {
    fb.drawVerticalGradient(0, 0, fb.getWidth(), fb.getHeight(),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    const std::string name = nickname.empty() ? std::string(Names::getSpeciesName(species)) : nickname;
    fb.drawText(28, 16, name, Colors::Text, TextStyle::Heading);
    int nw = 0, nh = 0;
    fb.measureText(name, nw, nh, TextStyle::Heading);
    drawHeaderGender(fb, 40 + nw, 22, gender);
    if (shiny) fb.drawShinyMark(66 + nw, 20, 18, Colors::ShinyStar);

    const std::string rightHeader = "Lv. " + std::to_string(level) + "     No. " + dexLabel(species);
    int rw = 0, rh = 0;
    fb.measureText(rightHeader, rw, rh);
    fb.drawText(fb.getWidth() - 28 - rw, 24, rightHeader, Colors::TextDim);

    const std::string mode = std::string(ClassicContext::modeLabel(surfaceMode));
    int mw = 0, mh = 0;
    fb.measureText(mode, mw, mh, TextStyle::Caption);
    fb.drawText((fb.getWidth() - mw) / 2, 23, mode, Colors::Accent, TextStyle::Caption);
    fb.drawFilledRect(0, 60, fb.getWidth(), 2, Colors::Accent);
    fb.drawText(28, 66, ClassicContext::contextLine(recordContext, sourceGameId, surfaceMode),
                Colors::TextDim, TextStyle::Caption);

}
inline void drawPortrait(PKSEFramebuffer& fb, int x, int y, uint16_t species, bool shiny) {
    if (auto* sprite = SpriteManager::getSprite(species, shiny); sprite && sprite->data)
        fb.drawSpriteStaticContained(x + 63, y + 34, 174, 146,
            sprite->width, sprite->height, sprite->data, sprite->channels);
    else fb.drawText(x + 82, y + 104, "Sprite unavailable", Colors::TextDim, TextStyle::Caption);
}
inline void drawVerticalScrollIndicator(PKSEFramebuffer& fb, int x, int y, int height,
    std::size_t totalRows, std::size_t visibleCapacity, std::size_t firstVisible) {
    constexpr int width = 4;
    const auto thumb = PokeBank::UIModel::SharedPokemonEditor::scrollThumb(
        totalRows, visibleCapacity, firstVisible, height);
    if (!thumb.visible) return;
    fb.drawFilledRoundedRect(x, y, width, height, 2, Colors::Divider);
    fb.drawFilledRoundedRect(x, y + thumb.offset, width, thumb.length, 2, Colors::TextDim);
}

// Scroll only the Details rows below the fixed portrait/type header.
// Two baselines keep full native values visible without clipping the narrow panel.
template <class Label, class Value>
inline void drawScrollableDetails(PKSEFramebuffer& fb, int x, int y, int w, int h,
    std::size_t total, std::size_t focus, bool focused, Label label, Value value) {
    const auto window = PokeBank::UIModel::SharedPokemonEditor::scrollWindow(total, 6, focus);
    // Scrolling owns one clipped row viewport. Clear it before painting the next window so
    // an older set of rows can never ghost underneath the current focus position.
    const int viewportY = y + 216;
    const int viewportBottom = y + h - 40;
    const int viewportH = std::max(0, viewportBottom - viewportY);
    fb.drawFilledRect(x + 8, viewportY, w - 16, viewportH, Colors::Panel);
    fb.setClipRect(x + 8, viewportY, w - 16, viewportH);
    for (std::size_t i = 0; i < window.count; ++i) {
        const auto row = window.first + i;
        const int yy = y + 224 + static_cast<int>(i) * 48;
        const bool selected = focused && row == focus;
        if (selected)
            fb.drawRoundedRect(x + 8, yy - 4, w - 16, 45, 6, Colors::FocusBorder, 2);
        fb.drawText(x + 16, yy, label(row),
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 16, yy + 20, value(row), Colors::Text, TextStyle::Caption);
    }
    fb.clearClip();
    fb.drawText(x + 16, y + h - 30, "Rows " + std::to_string(window.first + 1) + "-" +
        std::to_string(window.first + window.count) + " / " + std::to_string(total),
        Colors::TextDim, TextStyle::Caption);
}
// The native-data and six-stat radar composition is shared by exact-format providers.
template <class Rows>
inline void drawDataAndGraph(PKSEFramebuffer& fb, int x, int y, int w, int h,
    const char* title, const Rows& rows, const std::array<uint16_t,6>& stats) {
    constexpr int inset = 12, gap = 10, dataW = 198;
    const int dataX = x + inset, graphX = dataX + dataW + gap;
    const int graphW = w - 2 * inset - gap - dataW;
    for (const auto& pane : {std::pair{dataX, dataW}, std::pair{graphX, graphW}}) {
        fb.drawFilledRoundedRect(pane.first, y, pane.second, h, 12, Colors::Surface);
        fb.drawRoundedRect(pane.first, y, pane.second, h, 12, Colors::Divider, 1);
    }
    fb.drawText(dataX + 10, y + 10, title, Colors::Accent, TextStyle::Caption);
    int rowY = y + 34;
    for (const auto& row : rows) {
        const auto combined = row.label + ": " + row.value;
        int tw = 0, th = 0;
        fb.measureText(combined, tw, th, TextStyle::Caption);
        if (tw <= dataW - 20) {
            fb.drawText(dataX + 10, rowY, combined, Colors::Text, TextStyle::Caption);
            rowY += 20;
        } else {
            fb.drawText(dataX + 10, rowY, row.label, Colors::TextDim, TextStyle::Caption);
            fb.drawText(dataX + 10, rowY + 20, row.value, Colors::Text, TextStyle::Caption);
            rowY += 40;
        }
    }
    fb.drawText(graphX + 10, y + 10, "BATTLE STATS", Colors::Accent, TextStyle::Caption);
    StatsRadar::drawGen2Labeled(fb, graphX + 8, y + 32, graphW - 16, h - 40, stats);
}
} // namespace UI::SharedPokemonShell
