#include <cassert>
#include <cmath>
#include <string_view>
#include <fstream>
#include <sstream>
#include <string>

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

std::string readFile(const char* path) {
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}
}

int main() {
    using namespace UI;

    assert(OLED_BLACK_PALETTE.background == Color(0, 0, 0));
    assert(OLED_BLACK_PALETTE.surface != OLED_BLACK_PALETTE.background);
    assert(DARK_PALETTE.background != OLED_BLACK_PALETTE.background);
    assert(LIGHT_PALETTE.background != DARK_PALETTE.background);
    assert(LIGHT_PALETTE.surfaceRaised != DARK_PALETTE.surfaceRaised);

    // Brand red is intentionally unchanged. Interaction focus is teal and selected surfaces are
    // untinted: the selected token equals the ordinary raised surface in every theme.
    assert(OLED_BLACK_PALETTE.surfaceSelected == OLED_BLACK_PALETTE.surfaceRaised);
    assert(OLED_BLACK_PALETTE.accentPrimary == Color(236, 62, 72));
    assert(OLED_BLACK_PALETTE.accentSecondary == Color(58, 142, 153));
    assert(OLED_BLACK_PALETTE.focusBorder == Color(69, 184, 196));
    assert(DARK_PALETTE.surfaceSelected == DARK_PALETTE.surfaceRaised);
    assert(DARK_PALETTE.accentPrimary == Color(232, 60, 70));
    assert(DARK_PALETTE.accentSecondary == Color(62, 149, 160));
    assert(DARK_PALETTE.focusBorder == Color(69, 184, 196));
    assert(LIGHT_PALETTE.surfaceSelected == LIGHT_PALETTE.surfaceRaised);
    assert(LIGHT_PALETTE.accentPrimary == Color(200, 48, 58));
    assert(LIGHT_PALETTE.accentSecondary == Color(50, 137, 148));
    assert(LIGHT_PALETTE.focusBorder == Color(43, 127, 137));

    assert(DARK_PALETTE.success == Color(72, 190, 126));
    assert(DARK_PALETTE.warning == Color(255, 195, 61));
    assert(DARK_PALETTE.error == Color(243, 86, 89));
    assert(DARK_PALETTE.info == Color(82, 158, 240));

    for (ThemeMode mode : {ThemeMode::OLEDBlack, ThemeMode::Dark, ThemeMode::Light}) {
        const ThemePalette& palette = themePalette(mode);
        assert(contrast(palette.textPrimary, palette.background) >= 7.0);
        assert(contrast(palette.textPrimary, palette.surface) >= 7.0);
        assert(palette.focusBorder != palette.divider);
        assert(palette.focusBorder != palette.error);
        assert(palette.focusBorder != palette.accentPrimary);
        assert(palette.surfaceSelected == palette.surfaceRaised);
        assert(themeModeFromKey(themeModeKey(mode)) == mode);

        applyTheme(mode);
        assert(Colors::Background == palette.background);
        assert(Colors::Surface == palette.surface);
        assert(Colors::FocusBorder == palette.focusBorder);
        assert(Colors::Accent == palette.focusBorder);
        assert(Colors::FocusAccent == palette.focusBorder);
        assert(Colors::BrandAccent == palette.accentPrimary);
        assert(Colors::SelectedText == palette.textPrimary);
        assert(Colors::Primary == palette.focusBorder);
        assert(Colors::CursorMenu == palette.focusBorder);
        assert(Colors::ShinyStar != palette.error);
        assert(Colors::ShinyStar != palette.accentPrimary);
        assert(Colors::Warning == palette.warning);
    }

    assert(themeModeFromKey("old-or-corrupt-value") == ThemeMode::Dark);
    assert(themeModeName(ThemeMode::OLEDBlack) == std::string_view("OLED Black"));
    assert(nextThemeMode(ThemeMode::OLEDBlack) == ThemeMode::Dark);
    assert(nextThemeMode(ThemeMode::Dark) == ThemeMode::Light);
    assert(nextThemeMode(ThemeMode::Light) == ThemeMode::OLEDBlack);

    const std::string framebuffer = readFile("src/UI/PKSEFramebuffer.cpp");
    const auto hi = framebuffer.find("void PKSEFramebuffer::drawSelectionHighlight");
    assert(hi != std::string::npos);
    const auto hiEnd = framebuffer.find("\n    }", hi);
    const std::string highlight = framebuffer.substr(hi, hiEnd - hi);
    assert(highlight.find("drawFilledRoundedRect") == std::string::npos);
    assert(highlight.find("Colors::FocusBorder") != std::string::npos);

    const std::string chrome = readFile("include/UI/ScreenChrome.h");
    assert(chrome.find("Colors::BrandAccent") != std::string::npos);
    assert(chrome.find("selected ? Colors::Primary : Colors::PanelAlt") == std::string::npos);
    assert(chrome.find("selected  ? Colors::SelectedText") != std::string::npos);

    const std::string picker = readFile("src/UI/Dialogs/PickerDialog.cpp");
    assert(picker.find("fb.drawRoundedRect(px, py, pw, ph, 16, Colors::Divider, 1);") != std::string::npos);
    assert(picker.find("fb.drawRoundedRect(px, py, pw, ph, 16, Colors::Accent, 2);") == std::string::npos);

    const std::string dialog = readFile("include/UI/Dialogs/DialogFrame.h");
    assert(dialog.find("Colors::Border, 1") != std::string::npos);

    // Active/reachable screens that used to bypass the shared helper must also avoid focus fills.
    const std::string trainer = readFile("src/UI/TrainerViewScreenBase.inc");
    assert(trainer.find("sel ? Colors::Selected : Colors::PanelAlt") == std::string::npos);

    const std::string items = readFile("src/UI/Panels/ItemsPanel.cpp");
    assert(items.find("selected ? Colors::Primary : Colors::PanelAlt") == std::string::npos);

    const std::string party = readFile("src/UI/Panels/PartyPokemonPanel.cpp");
    assert(party.find("selected ? Colors::Selected : Colors::PanelAlt") == std::string::npos);

    const std::string details = readFile("src/UI/Modals/PokemonDetailsModal.cpp");
    assert(details.find("Colors::Selected") == std::string::npos);

    const std::string storage = readFile("src/UI/Panels/StoragePanel.cpp");
    assert(storage.find("Colors::AccentPrimary") == std::string::npos);

    const std::string gen1 = readFile("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc");
    assert(gen1.find("c == 2 ? Colors::TextSecondary : Colors::Text") != std::string::npos);

    const std::string gen2 = readFile("src/UI/Gen2PokemonEditorFoundation.inc");
    assert(gen2.find("drawFilledRoundedRect(x, y - 5, w, 28, 8, Colors::SurfaceSelected)") == std::string::npos);

    const std::string gen3 = readFile("src/UI/Gen3SharedPokemonSurface.inc");
    assert(gen3.find("18, Colors::FocusBorder, 2") == std::string::npos);
    // Focus readability: active labels/values stay bright without introducing a selection fill.
    assert(gen1.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen1.find("shinySelected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen1.find("levelSelected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen2.find("Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen3.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(trainer.find("sel ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

    const std::string shell = readFile("include/UI/SharedPokemonShell.h");
    assert(shell.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

    const std::string gen2Shared = readFile("src/UI/Gen2SharedPokemonSurface.inc");
    assert(gen2Shared.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

}
