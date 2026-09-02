#include <cassert>
#include <cmath>
#include <string_view>

#include "UI/Common.h"

namespace {
double channel(unsigned char value) {
    const double s = value / 255.0;
    return s <= 0.04045 ? s / 12.92 : std::pow((s + 0.055) / 1.055, 2.4);
}

double contrast(UI::Color a, UI::Color b) {
    const double la = 0.2126 * channel(a.r) + 0.7152 * channel(a.g) + 0.0722 * channel(a.b);
    const double lb = 0.2126 * channel(b.r) + 0.7152 * channel(b.g) + 0.0722 * channel(b.b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}
}

int main() {
    using namespace UI;

    assert(OLED_BLACK_PALETTE.background == Color(0, 0, 0));
    assert(OLED_BLACK_PALETTE.surface != OLED_BLACK_PALETTE.background);
    assert(DARK_PALETTE.background != OLED_BLACK_PALETTE.background);
    assert(LIGHT_PALETTE.background != DARK_PALETTE.background);
    assert(LIGHT_PALETTE.surfaceRaised != DARK_PALETTE.surfaceRaised);

    for (ThemeMode mode : {ThemeMode::OLEDBlack, ThemeMode::Dark, ThemeMode::Light}) {
        const ThemePalette& palette = themePalette(mode);
        assert(contrast(palette.textPrimary, palette.background) >= 7.0);
        assert(contrast(palette.textPrimary, palette.surface) >= 7.0);
        assert(palette.focusBorder != palette.divider);
        assert(themeModeFromKey(themeModeKey(mode)) == mode);

        applyTheme(mode);
        assert(Colors::Background == palette.background);
        assert(Colors::Surface == palette.surface);
        assert(Colors::FocusBorder == palette.focusBorder);
        assert(Colors::Warning == palette.warning);
    }

    assert(themeModeFromKey("old-or-corrupt-value") == ThemeMode::Dark);
    assert(themeModeName(ThemeMode::OLEDBlack) == std::string_view("OLED Black"));
    assert(nextThemeMode(ThemeMode::OLEDBlack) == ThemeMode::Dark);
    assert(nextThemeMode(ThemeMode::Dark) == ThemeMode::Light);
    assert(nextThemeMode(ThemeMode::Light) == ThemeMode::OLEDBlack);
}
