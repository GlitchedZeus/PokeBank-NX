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

    enum class ThemeMode { OLEDBlack, Dark, Light };

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
        Color(0, 0, 0),       Color(14, 15, 20),    Color(27, 29, 38),    Color(42, 39, 66),
        Color(244, 244, 249), Color(194, 194, 207), Color(129, 130, 147), Color(151, 132, 255),
        Color(255, 184, 77),  Color(187, 172, 255), Color(49, 51, 64),    Color(79, 205, 137),
        Color(255, 199, 66),  Color(255, 94, 94),   Color(87, 169, 255)
    };

    inline constexpr ThemePalette DARK_PALETTE{
        Color(24, 27, 38),    Color(35, 39, 53),    Color(47, 52, 69),    Color(62, 61, 92),
        Color(239, 240, 247), Color(185, 188, 204), Color(126, 130, 151), Color(139, 122, 255),
        Color(255, 184, 77),  Color(174, 159, 255), Color(67, 72, 91),    Color(72, 190, 126),
        Color(255, 195, 61),  Color(243, 86, 89),   Color(82, 158, 240)
    };

    // Light is intentionally designed as a warm/pastel palette, not as an inverted dark palette.
    inline constexpr ThemePalette LIGHT_PALETTE{
        Color(246, 247, 252), Color(255, 255, 255), Color(235, 239, 249), Color(225, 221, 249),
        Color(29, 32, 45),    Color(75, 80, 101),   Color(113, 119, 140), Color(91, 73, 210),
        Color(229, 142, 44),  Color(93, 72, 226),   Color(205, 210, 226), Color(35, 137, 83),
        Color(166, 91, 0),    Color(190, 48, 52),   Color(28, 105, 184)
    };

    constexpr const ThemePalette& themePalette(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack: return OLED_BLACK_PALETTE;
            case ThemeMode::Light:     return LIGHT_PALETTE;
            case ThemeMode::Dark:      return DARK_PALETTE;
        }
        return DARK_PALETTE;
    }

    constexpr std::string_view themeModeName(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack: return "OLED Black";
            case ThemeMode::Dark:      return "Dark";
            case ThemeMode::Light:     return "Light";
        }
        return "Dark";
    }

    constexpr std::string_view themeModeKey(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack: return "oled-black";
            case ThemeMode::Dark:      return "dark";
            case ThemeMode::Light:     return "light";
        }
        return "dark";
    }

    constexpr ThemeMode themeModeFromKey(std::string_view key) {
        if (key == "oled-black" || key == "oled" || key == "black") return ThemeMode::OLEDBlack;
        if (key == "light") return ThemeMode::Light;
        return ThemeMode::Dark;
    }

    constexpr ThemeMode nextThemeMode(ThemeMode mode) {
        switch (mode) {
            case ThemeMode::OLEDBlack: return ThemeMode::Dark;
            case ThemeMode::Dark:      return ThemeMode::Light;
            case ThemeMode::Light:     return ThemeMode::OLEDBlack;
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
        inline Color& Panel     = Surface;
        inline Color& PanelAlt  = SurfaceRaised;
        inline Color& Selected  = SurfaceSelected;
        inline Color& Border    = Divider;
        inline Color& Text      = TextPrimary;
        inline Color& TextDim   = TextMuted;
        inline Color& Accent    = AccentPrimary;
        inline Color& AccentDim = AccentSecondary;
        // Warm secondary (HOME's "warm-on-cool" pop): selected pill / primary action / Save.
        // Indigo stays the hero; amber is used sparingly for the single primary/selected element.
        inline Color Primary      = Color(255, 184, 77);  // amber
        inline Color PrimaryText  = Color(43, 32, 10);    // dark text drawn on top of amber
        // Attention accent for warning dialog titles ("Unsaved Changes", "Delete Backup?"). Theme-aware
        // because a bright amber that reads on the dark UI is nearly invisible on light-mode white.
        // Shiny marker (star / "Yes"), HOME-style red rather than yellow. Theme-aware: yellow washed
        // out on light-mode white exactly like Warning did, and red reads on both sprites and panels.
        inline Color ShinyStar    = Color(255, 96, 86);
        // Storage cursor-mode colors, one per CursorMode — a red/blue/green scheme across the three
        // pointer arrows (Menu / Move / Multi). Theme-aware: the light variants are deepened so
        // the arrow, the selection wash and the carried-block backing still read against a white panel.
        inline Color CursorMenu   = Color(232, 92, 92);
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
        Primary         = palette.accentSecondary;
        PrimaryText     = Color(43, 32, 10);
        ShinyStar       = (mode == ThemeMode::Light) ? Color(190, 48, 52) : Color(255, 96, 86);
        CursorMenu      = (mode == ThemeMode::Light) ? Color(196, 52, 57) : Color(232, 92, 92);
        CursorMove      = (mode == ThemeMode::Light) ? Color(37, 99, 201) : Color(86, 148, 244);
        CursorMulti     = (mode == ThemeMode::Light) ? Color(35, 143, 81) : Color(96, 205, 128);
    }
}

#endif
