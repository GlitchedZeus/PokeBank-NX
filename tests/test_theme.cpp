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
    assert(OLED_BLACK_PALETTE.accentSecondary == Color(35, 86, 94));
    assert(OLED_BLACK_PALETTE.focusBorder == Color(90, 210, 222));
    assert(DARK_PALETTE.surfaceSelected == DARK_PALETTE.surfaceRaised);
    assert(DARK_PALETTE.accentPrimary == Color(232, 60, 70));
    assert(DARK_PALETTE.accentSecondary == Color(42, 94, 103));
    assert(DARK_PALETTE.focusBorder == Color(87, 204, 216));
    assert(LIGHT_PALETTE.surfaceSelected == LIGHT_PALETTE.surfaceRaised);
    assert(LIGHT_PALETTE.accentPrimary == Color(184, 36, 46));
    assert(LIGHT_PALETTE.accentSecondary == Color(218, 224, 232));
    assert(LIGHT_PALETTE.focusBorder == Color(16, 18, 24));
    assert(RED_PALETTE.accentPrimary == Color(241, 70, 80));
    assert(BLUE_PALETTE.accentPrimary == Color(74, 140, 255));
    assert(GREEN_PALETTE.accentPrimary == Color(67, 196, 119));
    assert(GRAY_PALETTE.accentPrimary == Color(195, 198, 204));
    assert(POKE_CLASSIC_PALETTE.focusBorder == Color(255, 207, 64));
    assert(PURPLE_PALETTE.accentPrimary == Color(188, 100, 255));
    assert(ORANGE_PALETTE.accentPrimary == Color(255, 138, 48));
    assert(PS5_PALETTE.focusBorder == Color(150, 184, 255));
    assert(XBOX_PALETTE.focusBorder == Color(116, 218, 132));
    assert(POKEMON_HOME_PALETTE.accentPrimary == Color(239, 104, 163));
    assert(SWITCH_PALETTE.focusBorder == Color(226, 231, 240));

    assert(DARK_PALETTE.success == Color(82, 205, 137));
    assert(DARK_PALETTE.warning == Color(255, 202, 76));
    assert(DARK_PALETTE.error == Color(250, 103, 108));
    assert(DARK_PALETTE.info == Color(96, 176, 255));

    for (ThemeMode mode : {ThemeMode::OLEDBlack, ThemeMode::Dark, ThemeMode::Light,
                           ThemeMode::Red, ThemeMode::Blue, ThemeMode::Green,
                           ThemeMode::Gray, ThemeMode::PokeClassic,
                           ThemeMode::Purple, ThemeMode::Orange,
                           ThemeMode::PS5, ThemeMode::Xbox, ThemeMode::PokemonHome,
                           ThemeMode::Switch, ThemeMode::Midnight, ThemeMode::Graphite,
                           ThemeMode::Arctic, ThemeMode::Ivory, ThemeMode::Aurora,
                           ThemeMode::Sunset}) {
        const ThemePalette& palette = themePalette(mode);
        assert(contrast(palette.textPrimary, palette.background) >= 7.0);
        assert(contrast(palette.textPrimary, palette.surface) >= 7.0);
        assert(contrast(palette.textSecondary, palette.background) >= 4.5);
        assert(contrast(palette.textSecondary, palette.surface) >= 4.5);
        // Comfort/readability floor is stronger than AA for ordinary dim copy.
        assert(contrast(palette.textMuted, palette.background) >= 6.0);
        assert(contrast(palette.textMuted, palette.surface) >= 6.0);
        assert(contrast(palette.textMuted, palette.surfaceRaised) >= 5.5);
        assert(contrast(palette.textSecondary, palette.surfaceRaised) >= 7.0);
        assert(contrast(palette.textPrimary, palette.accentSecondary) >= 4.5);
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
        assert(Colors::CursorMenu == (mode == ThemeMode::Light ? Colors::Black : palette.focusBorder));
        assert(Colors::ShinyStar != palette.error);
        assert(Colors::ShinyStar != palette.accentPrimary);
        assert(Colors::Warning == palette.warning);
    }

    assert(themeModeFromKey("old-or-corrupt-value") == ThemeMode::Dark);
    assert(themeModeFromKey("grey") == ThemeMode::Gray);
    assert(themeModeFromKey("pokemon") == ThemeMode::PokeClassic);
    assert(themeModeName(ThemeMode::OLEDBlack) == std::string_view("OLED Black"));
    assert(themeModeName(ThemeMode::Gray) == std::string_view("Grey"));
    assert(themeModeName(ThemeMode::PokeClassic) == std::string_view("Poke Classic"));
    assert(themeModeName(ThemeMode::PS5) == std::string_view("PS5"));
    assert(themeModeName(ThemeMode::PokemonHome) == std::string_view("Pokemon HOME"));
    assert(themeModeFromKey("playstation") == ThemeMode::PS5);
    assert(themeModeFromKey("home") == ThemeMode::PokemonHome);
    assert(themeModeFromKey("cream") == ThemeMode::Ivory);
    assert(isLightTheme(ThemeMode::Light));
    assert(isLightTheme(ThemeMode::Arctic));
    assert(isLightTheme(ThemeMode::Ivory));
    assert(!isLightTheme(ThemeMode::PokeClassic));
    assert(nextThemeMode(ThemeMode::OLEDBlack) == ThemeMode::Dark);
    assert(nextThemeMode(ThemeMode::Dark) == ThemeMode::Light);
    assert(nextThemeMode(ThemeMode::Light) == ThemeMode::Red);
    assert(nextThemeMode(ThemeMode::Red) == ThemeMode::Blue);
    assert(nextThemeMode(ThemeMode::Blue) == ThemeMode::Green);
    assert(nextThemeMode(ThemeMode::Green) == ThemeMode::Gray);
    assert(nextThemeMode(ThemeMode::Gray) == ThemeMode::PokeClassic);
    assert(nextThemeMode(ThemeMode::PokeClassic) == ThemeMode::Purple);
    assert(nextThemeMode(ThemeMode::Purple) == ThemeMode::Orange);
    assert(nextThemeMode(ThemeMode::Orange) == ThemeMode::PS5);
    assert(nextThemeMode(ThemeMode::PS5) == ThemeMode::Xbox);
    assert(nextThemeMode(ThemeMode::Xbox) == ThemeMode::PokemonHome);
    assert(nextThemeMode(ThemeMode::PokemonHome) == ThemeMode::Switch);
    assert(nextThemeMode(ThemeMode::Switch) == ThemeMode::Midnight);
    assert(nextThemeMode(ThemeMode::Midnight) == ThemeMode::Graphite);
    assert(nextThemeMode(ThemeMode::Graphite) == ThemeMode::Arctic);
    assert(nextThemeMode(ThemeMode::Arctic) == ThemeMode::Ivory);
    assert(nextThemeMode(ThemeMode::Ivory) == ThemeMode::Aurora);
    assert(nextThemeMode(ThemeMode::Aurora) == ThemeMode::Sunset);
    assert(nextThemeMode(ThemeMode::Sunset) == ThemeMode::OLEDBlack);
    applyTheme(ThemeMode::Light);
    assert(Colors::FocusBorder == Color(16, 18, 24));
    assert(Colors::CursorMenu == Colors::Black);
    assert(Colors::PrimaryText == Colors::White);
    applyTheme(ThemeMode::Arctic);
    assert(Colors::PrimaryText == Colors::White);
    assert(Colors::CursorMenu == ARCTIC_PALETTE.focusBorder);
    applyTheme(ThemeMode::PokeClassic);
    assert(themeModeKey(g_themeMode) == std::string_view("poke-classic"));

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
    assert(gen1.find("levelSelected") == std::string::npos);
    assert(gen2.find("Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen3.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(trainer.find("sel ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

    const std::string shell = readFile("include/UI/SharedPokemonShell.h");
    assert(shell.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

    const std::string gen2Shared = readFile("src/UI/Gen2SharedPokemonSurface.inc");
    assert(gen2Shared.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);

    // The final hardware repaint must preserve focus readability; checking only the
    // underlying shared surface misses labels that are immediately painted over.
    const std::string gen2Hardware = readFile("src/UI/Gen2HardwareFinalFix.inc");
    const auto repaintStart = gen2Hardware.find("void redrawGen2DetailsSemantics");
    const auto repaintEnd = gen2Hardware.find("void redrawGen2MovesSemantics");
    assert(repaintStart != std::string::npos && repaintEnd > repaintStart);
    const auto repaint = gen2Hardware.substr(repaintStart, repaintEnd - repaintStart);
    assert(repaint.find("rows[row].first, selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(repaint.find("rowSelected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(repaint.find("shinySelected ? Colors::SelectedText : (p.shiny") != std::string::npos);
    assert(repaint.find("genderSelected ? Colors::SelectedText : gen2GenderColor") != std::string::npos);
    assert(repaint.find("Colors::Accent") == std::string::npos);
    assert(repaint.find("stat, Colors::TextSecondary") != std::string::npos);
    assert(repaint.find("r == 0 ? Colors::TextDim : Colors::Text") != std::string::npos);
    const std::string unified = readFile("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    assert(unified.find("rows[row].first, selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(unified.find("rowSelected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen1.find("rowSelected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    assert(gen1.find("shinySelected ? Colors::SelectedText : (presentation.shiny") != std::string::npos);

    const std::string radar = readFile("include/UI/StatsRadar.h");
    assert(radar.find("fillPolygon(fb, data") == std::string::npos);
    assert(radar.find("Color(232, 60, 70, 58)") == std::string::npos);
    assert(radar.find("Colors::FocusBorder") != std::string::npos);
}
