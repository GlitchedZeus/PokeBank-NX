#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <cstdint>
#include <string_view>

namespace UI {
    struct Color {
        uint8_t r, g, b, a;

        constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}

        uint32_t toRGBA8() const {
            return (r << 0) | (g << 8) | (b << 16) | (a << 24);
        }

        constexpr bool operator==(const Color&) const = default;
    };

    enum class ThemeMode { OLEDBlack, Dark, Light, Red, Blue, Green, Gray, PokeClassic, Purple, Orange };

    struct ThemePalette {
        Color background;
        Color surface;
        Color surfaceRaised;
        Color surfaceSelected;
        Color textPrimary;
        Color textSecondary;
        Color textMuted;
        Color accentPrimary;
        Color accentSecondary;
        Color focusBorder;
        Color divider;
        Color success;
        Color warning;
        Color error;
        Color info;
    };

    inline constexpr ThemePalette OLED_BLACK_PALETTE{
        Color(0, 0, 0),       Color(14, 15, 20),    Color(30, 32, 42),    Color(30, 32, 42),
        Color(250, 250, 252), Color(218, 220, 230), Color(166, 170, 186), Color(236, 62, 72),
        Color(35, 86, 94),    Color(90, 210, 222), Color(65, 68, 82),    Color(85, 220, 150),
        Color(255, 205, 80),  Color(255, 110, 110), Color(100, 185, 255)
    };

    inline constexpr ThemePalette DARK_PALETTE{
        Color(20, 23, 32),    Color(31, 35, 47),    Color(44, 49, 65),    Color(44, 49, 65),
        Color(248, 249, 252), Color(218, 220, 230), Color(166, 170, 188), Color(232, 60, 70),
        Color(42, 94, 103),   Color(87, 204, 216), Color(73, 78, 98),    Color(82, 205, 137),
        Color(255, 202, 76),  Color(250, 103, 108), Color(96, 176, 255)
    };

    // Light is intentionally designed as a bright high-contrast palette, not an inverted dark palette.
    inline constexpr ThemePalette LIGHT_PALETTE{
        Color(246, 247, 252), Color(255, 255, 255), Color(232, 236, 245), Color(232, 236, 245),
        Color(18, 20, 28),    Color(48, 53, 69),    Color(78, 84, 103),  Color(184, 36, 46),
        Color(218, 224, 232), Color(16, 18, 24),    Color(188, 195, 214), Color(24, 116, 66),
        Color(148, 78, 0),    Color(166, 38, 44),   Color(21, 88, 160)
    };

    inline constexpr ThemePalette RED_PALETTE{
        Color(28, 14, 16),    Color(43, 21, 24),    Color(61, 29, 33),    Color(61, 29, 33),
        Color(252, 244, 245), Color(231, 207, 210), Color(190, 155, 160), Color(241, 70, 80),
        Color(87, 38, 44),    Color(255, 140, 145), Color(90, 48, 52),    Color(88, 214, 145),
        Color(255, 207, 86),  Color(255, 120, 126), Color(120, 190, 255)
    };

    inline constexpr ThemePalette BLUE_PALETTE{
        Color(12, 20, 34),    Color(20, 31, 50),    Color(29, 44, 68),    Color(29, 44, 68),
        Color(245, 249, 255), Color(206, 220, 240), Color(155, 177, 207), Color(74, 140, 255),
        Color(31, 68, 111),   Color(110, 180, 255), Color(52, 70, 98),    Color(82, 210, 142),
        Color(255, 205, 82),  Color(255, 112, 120), Color(120, 200, 255)
    };

    inline constexpr ThemePalette GREEN_PALETTE{
        Color(11, 25, 19),    Color(18, 39, 29),    Color(27, 55, 41),    Color(27, 55, 41),
        Color(244, 252, 248), Color(207, 232, 219), Color(155, 190, 172), Color(67, 196, 119),
        Color(28, 76, 53),    Color(112, 232, 158), Color(47, 83, 64),    Color(96, 224, 150),
        Color(255, 207, 82),  Color(255, 112, 120), Color(112, 192, 255)
    };

    inline constexpr ThemePalette GRAY_PALETTE{
        Color(24, 25, 27),    Color(38, 40, 44),    Color(54, 57, 62),    Color(54, 57, 62),
        Color(249, 249, 250), Color(219, 221, 225), Color(170, 174, 181), Color(195, 198, 204),
        Color(74, 78, 86),    Color(235, 238, 243), Color(80, 84, 91),    Color(91, 210, 145),
        Color(255, 207, 82),  Color(255, 112, 120), Color(120, 190, 255)
    };

    // Original PokeBank-NX take on the familiar red/blue/yellow monster-RPG palette.
    inline constexpr ThemePalette POKE_CLASSIC_PALETTE{
        Color(18, 24, 39),    Color(29, 38, 61),    Color(42, 54, 84),    Color(42, 54, 84),
        Color(250, 249, 245), Color(221, 225, 236), Color(171, 180, 205), Color(226, 58, 68),
        Color(31, 72, 126),   Color(255, 207, 64),  Color(63, 77, 108),   Color(80, 205, 130),
        Color(255, 207, 64),  Color(255, 108, 112), Color(87, 169, 255)
    };

    inline constexpr ThemePalette PURPLE_PALETTE{
        Color(24, 16, 32),    Color(38, 25, 48),    Color(55, 36, 68),    Color(55, 36, 68),
        Color(250, 246, 253), Color(224, 210, 234), Color(177, 156, 193), Color(188, 100, 255),
        Color(77, 45, 96),    Color(215, 145, 255), Color(83, 58, 98),    Color(88, 214, 145),
        Color(255, 207, 82),  Color(255, 112, 120), Color(122, 190, 255)
    };

    inline constexpr ThemePalette ORANGE_PALETTE{
        Color(30, 20, 10),    Color(46, 30, 15),    Color(64, 42, 20),    Color(64, 42, 20),
        Color(255, 248, 238), Color(236, 216, 193), Color(191, 163, 130), Color(255, 138, 48),
        Color(100, 57, 23),   Color(255, 181, 92), Color(94, 65, 36),    Color(90, 215, 145),
        Color(255, 211, 94),  Color(255, 116, 116), Color(122, 190, 255)
    };

    constexpr const ThemePalette& themePalette(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack:   return OLED_BLACK_PALETTE;
            case ThemeMode::Dark:        return DARK_PALETTE;
            case ThemeMode::Light:       return LIGHT_PALETTE;
            case ThemeMode::Red:         return RED_PALETTE;
            case ThemeMode::Blue:        return BLUE_PALETTE;
            case ThemeMode::Green:       return GREEN_PALETTE;
            case ThemeMode::Gray:        return GRAY_PALETTE;
            case ThemeMode::PokeClassic: return POKE_CLASSIC_PALETTE;
            case ThemeMode::Purple:      return PURPLE_PALETTE;
            case ThemeMode::Orange:      return ORANGE_PALETTE;
        }
        return DARK_PALETTE;
    }

    constexpr std::string_view themeModeName(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack:   return "OLED Black";
            case ThemeMode::Dark:        return "Dark";
            case ThemeMode::Light:       return "Light";
            case ThemeMode::Red:         return "Red";
            case ThemeMode::Blue:        return "Blue";
            case ThemeMode::Green:       return "Green";
            case ThemeMode::Gray:        return "Grey";
            case ThemeMode::PokeClassic: return "Poke Classic";
            case ThemeMode::Purple:      return "Purple";
            case ThemeMode::Orange:      return "Orange";
        }
        return "Dark";
    }

    constexpr std::string_view themeModeKey(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack:   return "oled-black";
            case ThemeMode::Dark:        return "dark";
            case ThemeMode::Light:       return "light";
            case ThemeMode::Red:         return "red";
            case ThemeMode::Blue:        return "blue";
            case ThemeMode::Green:       return "green";
            case ThemeMode::Gray:        return "gray";
            case ThemeMode::PokeClassic: return "poke-classic";
            case ThemeMode::Purple:      return "purple";
            case ThemeMode::Orange:      return "orange";
        }
        return "dark";
    }

    constexpr ThemeMode themeModeFromKey(std::string_view key) {
        if (key == "oled-black" || key == "oled" || key == "black") return ThemeMode::OLEDBlack;
        if (key == "light") return ThemeMode::Light;
        if (key == "red") return ThemeMode::Red;
        if (key == "blue") return ThemeMode::Blue;
        if (key == "green") return ThemeMode::Green;
        if (key == "gray" || key == "grey") return ThemeMode::Gray;
        if (key == "poke-classic" || key == "pokemon" || key == "poke") return ThemeMode::PokeClassic;
        if (key == "purple") return ThemeMode::Purple;
        if (key == "orange") return ThemeMode::Orange;
        return ThemeMode::Dark;
    }

    constexpr ThemeMode nextThemeMode(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack:   return ThemeMode::Dark;
            case ThemeMode::Dark:        return ThemeMode::Light;
            case ThemeMode::Light:       return ThemeMode::Red;
            case ThemeMode::Red:         return ThemeMode::Blue;
            case ThemeMode::Blue:        return ThemeMode::Green;
            case ThemeMode::Green:       return ThemeMode::Gray;
            case ThemeMode::Gray:        return ThemeMode::PokeClassic;
            case ThemeMode::PokeClassic: return ThemeMode::Purple;
            case ThemeMode::Purple:      return ThemeMode::Orange;
            case ThemeMode::Orange:      return ThemeMode::OLEDBlack;
        }
        return ThemeMode::Dark;
    }

    namespace Colors {
        // --- Fixed literal colors (theme-independent, always constant) ---
        constexpr Color Black(0, 0, 0);
        constexpr Color White(255, 255, 255);
        constexpr Color Gray(128, 128, 128);
        constexpr Color LightGray(192, 192, 192);
        constexpr Color DarkGray(64, 64, 64);
        constexpr Color Red(255, 0, 0);
        constexpr Color Green(0, 255, 0);
        constexpr Color Blue(0, 0, 255);
        constexpr Color Yellow(255, 255, 0);
        constexpr Color Cyan(0, 255, 255);
        constexpr Color Magenta(255, 0, 255);
        constexpr Color Orange(255, 165, 0);

        // --- Semantic theme colors (runtime-swappable by applyTheme()) ---
        // These are mutable so a dark/light toggle can restyle the whole UI without
        // changing the 15+ screens that reference Colors::Text etc. Defaults = dark theme.
        inline Color Background      = DARK_PALETTE.background;
        inline Color Surface         = DARK_PALETTE.surface;
        inline Color SurfaceRaised   = DARK_PALETTE.surfaceRaised;
        inline Color SurfaceSelected = DARK_PALETTE.surfaceSelected;
        inline Color TextPrimary     = DARK_PALETTE.textPrimary;
        inline Color TextSecondary   = DARK_PALETTE.textSecondary;
        inline Color TextMuted       = DARK_PALETTE.textMuted;
        inline Color AccentPrimary   = DARK_PALETTE.accentPrimary;
        inline Color AccentSecondary = DARK_PALETTE.accentSecondary;
        inline Color FocusBorder     = DARK_PALETTE.focusBorder;
        inline Color Divider         = DARK_PALETTE.divider;
        inline Color Success         = DARK_PALETTE.success;
        inline Color Warning         = DARK_PALETTE.warning;
        inline Color Error           = DARK_PALETTE.error;
        inline Color Info            = DARK_PALETTE.info;

        // Transitional aliases keep mature screens working while all new primitives and refreshed
        // screens use the semantic names above. They are references, so there is still one palette.
        inline Color& Panel        = Surface;
        inline Color& PanelAlt     = SurfaceRaised;
        inline Color& Selected     = SurfaceSelected;
        inline Color& Border       = Divider;
        inline Color& Text         = TextPrimary;
        inline Color& TextDim      = TextMuted;
        // Branding and interaction are deliberately separate. BrandAccent remains the original
        // PokeBank red; Accent/FocusAccent are the teal interaction language used by focus/selection.
        inline Color& BrandAccent  = AccentPrimary;
        inline Color& FocusAccent  = FocusBorder;
        inline Color& Accent       = FocusBorder;
        inline Color& AccentDim    = AccentSecondary;
        inline Color& SelectedText = TextPrimary;
        // Primary action fill follows the interaction accent. Selection rows themselves never use
        // this fill: focused rows keep their normal dark surface and use a teal outline + white text.
        inline Color Primary      = DARK_PALETTE.focusBorder;
        inline Color PrimaryText  = Color(43, 32, 36);
        // Attention accent for warning dialog titles ("Unsaved Changes", "Delete Backup?"). Theme-aware
        // because a bright amber that reads on the dark UI is nearly invisible on light-mode white.
        // Shiny marker is gold rather than red so red stays reserved for brand/danger semantics.
        // It remains theme-aware for readable contrast on dark and light panels.
        inline Color ShinyStar    = Color(255, 208, 92);
        // Storage cursor-mode colors: ordinary Menu navigation follows the global teal focus accent;
        // Move and Multi retain distinct blue/green mode semantics.
        inline Color CursorMenu   = DARK_PALETTE.focusBorder;
        inline Color CursorMove   = Color(86, 148, 244);
        inline Color CursorMulti  = Color(96, 205, 128);

        // Let's Go specific colors (matching in-game UI) — theme-independent
        constexpr Color PartnerHeart(255, 105, 180);    // Hot pink heart for Partner Pokemon
        constexpr Color PartyNumber(255, 200, 50);      // Yellow/amber for party position numbers
        // Party-position badge: a gold disc with a dark digit, drawn behind the number so it stays
        // legible on any sprite in either theme (a bare amber digit washed out on light-mode tiles).
        constexpr Color PartyBadge(255, 193, 68);
        constexpr Color PartyBadgeText(38, 28, 8);
    }

    // Height of the arrowhead on the storage/box grid cursor, in framebuffer pixels. Absolute
    // rather than a fraction of the disc, because the two grids size their discs differently (the
    // Boxes view has more room per cell than the bank's paired panes) -- scaling off the disc gave
    // a visibly bigger cursor in one view than the other.
    constexpr int kGridCursorH = 32;

    // Minimum comfortable touch-target size in framebuffer pixels. The UI renders at 1280x720 on
    // the ~6" handheld screen, so tappable controls (menu rows, dialog buttons, tab/nav hit areas)
    // should be at least this tall/wide for a fingertip. Grid slots are already larger than this.
    constexpr int TouchTargetMin = 56;

    inline ThemeMode g_themeMode = ThemeMode::Dark;

    // Swap the semantic palette. Screens keep using Colors::Text/Panel/... and pick up
    // the change automatically on the next frame (everything redraws each frame).
    inline void applyTheme(ThemeMode mode) {
        using namespace Colors;
        g_themeMode = mode;
        const ThemePalette& palette = themePalette(mode);
        Background      = palette.background;
        Surface         = palette.surface;
        SurfaceRaised   = palette.surfaceRaised;
        SurfaceSelected = palette.surfaceSelected;
        TextPrimary     = palette.textPrimary;
        TextSecondary   = palette.textSecondary;
        TextMuted       = palette.textMuted;
        AccentPrimary   = palette.accentPrimary;
        AccentSecondary = palette.accentSecondary;
        FocusBorder     = palette.focusBorder;
        Divider         = palette.divider;
        Success         = palette.success;
        Warning         = palette.warning;
        Error           = palette.error;
        Info            = palette.info;
        Primary         = palette.focusBorder;
        PrimaryText     = (mode == ThemeMode::Light) ? Colors::White : Color(32, 28, 24);
        ShinyStar       = (mode == ThemeMode::Light) ? Color(132, 91, 0) : Color(255, 218, 90);
        // Light mode deliberately uses a near-black cursor/focus treatment. The dark and
        // colored themes use their brighter theme focus color.
        CursorMenu      = (mode == ThemeMode::Light) ? Colors::Black : palette.focusBorder;
        CursorMove      = (mode == ThemeMode::Light) ? Color(20, 56, 130) : Color(86, 148, 244);
        CursorMulti     = (mode == ThemeMode::Light) ? Color(24, 112, 61) : Color(96, 205, 128);
    }
}

#endif
