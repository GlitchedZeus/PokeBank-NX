#include "UI/AppShellScreen.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "Globals.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/TouchInput.h"
#include "Utils/Settings.h"

namespace UI {
namespace {
    constexpr int kCardMarginX = 28;
    constexpr int kCardTop = 112;
    constexpr int kCardGapX = 18;
    constexpr int kCardGapY = 12;
    constexpr int kCardH = 126;

    template <typename Rect>
    bool contains(const Rect& rect, int x, int y) {
        return rect.index >= 0 &&
               x >= rect.x && x < rect.x + rect.w &&
               y >= rect.y && y < rect.y + rect.h;
    }

    Color badgeColor(PokeBank::UIModel::AppShellAvailability availability) {
        using PokeBank::UIModel::AppShellAvailability;
        switch (availability) {
            case AppShellAvailability::Ready:             return Colors::Info;
            case AppShellAvailability::WorkspaceRequired: return Colors::AccentPrimary;
            case AppShellAvailability::FutureBackend:     return Colors::TextMuted;
            case AppShellAvailability::ReadOnly:          return Colors::Info;
        }
        return Colors::TextMuted;
    }

    const std::vector<std::string>& sectionInfoLines(PokeBank::UIModel::AppShellSection section) {
        using PokeBank::UIModel::AppShellSection;
        static const std::vector<std::string> storage{
            "Legacy Storage is app-owned and persistent, but it is not the Master Vault.",
            "Open Games & Sources, then enter a save workspace to use Legacy Storage.",
            "A global Storage browser will arrive only when its backend can be represented honestly."
        };
        static const std::vector<std::string> banks{
            "Named Banks are an organization layer planned for the future Master Vault.",
            "Master Vault persistence is not implemented in this UI lane.",
            "No Pokemon or Bank records are fabricated by this screen."
        };
        static const std::vector<std::string> backups{
            "PokeBank backups are currently scoped to the selected game and profile.",
            "Open Games & Sources, select a game, then use its Save Backups screen.",
            "Inject / Restore is not available here; Issue #89 owns that future workflow."
        };
        static const std::vector<std::string> search{
            "Global Search needs the future Vault/search index before it can return real results.",
            "This lane will build filtering and no-results UI without reordering external save boxes.",
            "No source save is mutated by search, filter, sort, Favorites, or Recent presentation."
        };
        static const std::vector<std::string> collections{
            "Living Dex, Shiny Living Dex, Favorites and Recent are planned collection views.",
            "Collection persistence depends on the future Master Vault / Bank model.",
            "This shell exposes the product destination without pretending the data exists today."
        };
        static const std::vector<std::string> fallback{
            "This destination is not available from the root shell yet."
        };

        switch (section) {
            case AppShellSection::Storage:     return storage;
            case AppShellSection::Banks:       return banks;
            case AppShellSection::Backups:     return backups;
            case AppShellSection::Search:      return search;
            case AppShellSection::Collections: return collections;
            default:                           return fallback;
        }
    }
}

AppShellScreen::Action AppShellScreen::consumeAction() {
    const Action result = pendingAction;
    pendingAction = Action::None;
    return result;
}

void AppShellScreen::setStatus(std::string message, int frames) {
    statusMessage = std::move(message);
    statusFrames = frames;
}

void AppShellScreen::activateSelected() {
    using PokeBank::UIModel::AppShellSection;
    const auto section = PokeBank::UIModel::appShellEntry(selectedIndex).section;
    if (section == AppShellSection::Games) {
        pendingAction = Action::Games;
    } else if (section == AppShellSection::Settings) {
        settingsIndex = 0;
        statusMessage.clear();
        statusFrames = 0;
        overlay = Overlay::Settings;
    } else if (section == AppShellSection::Diagnostics) {
        overlay = Overlay::Diagnostics;
    } else {
        infoSection = section;
        overlay = Overlay::SectionInfo;
    }
}

void AppShellScreen::activateSetting() {
    bool changed = true;
    switch (settingsIndex) {
        case 0:
            g_autoBackupEnabled = !g_autoBackupEnabled;
            setStatus(g_autoBackupEnabled
                ? "Auto-backup enabled for supported installed-title workflows."
                : "Auto-backup disabled; supported workflows reuse their working copy.");
            break;
        case 1:
            applyTheme(nextThemeMode(g_themeMode));
            setStatus("Theme changed to " + std::string(themeModeName(g_themeMode)) + ".");
            break;
        case 2:
            g_allowIllegalEdits = !g_allowIllegalEdits;
            setStatus(g_allowIllegalEdits
                ? "Illegal-value editing enabled for fields that explicitly support it."
                : "Legal value caps restored.");
            break;
        case 3:
            g_moveWarn = !g_moveWarn;
            setStatus(g_moveWarn ? "Move compatibility warnings enabled."
                                 : "Move compatibility warnings disabled.");
            break;
        case 4:
            changed = false;
            setStatus("Locked: ordinary editing never writes emulator or installed-game sources.");
            break;
        case 5:
            g_debugLogging = !g_debugLogging;
            setStatus(g_debugLogging
                ? "Debug logging enabled under sdmc:/switch/PokeBank-NX/logs/."
                : "Debug logging disabled; no new log files will be written.");
            break;
        default:
            changed = false;
            break;
    }
    if (changed) Utils::saveSettings();
}

void AppShellScreen::update(const PadState& pad, const TouchInput& touch) {
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);
    u64 kDown = controllerNavigation.apply(
        padGetButtonsDown(&pad), padGetButtons(&pad), stick.x, stick.y,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right)
        | navTouchButton(touch);

    if (statusFrames > 0) --statusFrames;

    if (overlay == Overlay::Help || overlay == Overlay::Diagnostics ||
        overlay == Overlay::SectionInfo) {
        if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) overlay = Overlay::None;
        return;
    }

    if (overlay == Overlay::Settings) {
        if (touch.justPressed()) {
            for (const HitRect& rect : settingsRects) {
                if (contains(rect, touch.x(), touch.y())) {
                    settingsIndex = rect.index;
                    kDown |= HidNpadButton_A;
                    break;
                }
            }
        }
        if (kDown & HidNpadButton_B) {
            overlay = Overlay::None;
            statusMessage.clear();
            statusFrames = 0;
            return;
        }
        if (kDown & HidNpadButton_Up)
            settingsIndex = (settingsIndex + 5) % 6;
        if (kDown & HidNpadButton_Down)
            settingsIndex = (settingsIndex + 1) % 6;
        if (kDown & HidNpadButton_A) activateSetting();
        return;
    }

    if (kDown & HidNpadButton_Plus) {
        settingsIndex = 0;
        overlay = Overlay::Settings;
        return;
    }
    if (kDown & HidNpadButton_Minus) {
        overlay = Overlay::Help;
        return;
    }
    if (kDown & HidNpadButton_B) {
        exitRequested = true;
        return;
    }

    if (touch.justPressed()) {
        for (const HitRect& rect : cardRects) {
            if (contains(rect, touch.x(), touch.y())) {
                selectedIndex = rect.index;
                kDown |= HidNpadButton_A;
                break;
            }
        }
    }

    if (kDown & HidNpadButton_Left)
        selectedIndex = PokeBank::UIModel::appShellMoveSelection(selectedIndex, -1, 0);
    if (kDown & HidNpadButton_Right)
        selectedIndex = PokeBank::UIModel::appShellMoveSelection(selectedIndex, 1, 0);
    if (kDown & HidNpadButton_Up)
        selectedIndex = PokeBank::UIModel::appShellMoveSelection(selectedIndex, 0, -1);
    if (kDown & HidNpadButton_Down)
        selectedIndex = PokeBank::UIModel::appShellMoveSelection(selectedIndex, 0, 1);
    if (kDown & HidNpadButton_A) activateSelected();
}

void AppShellScreen::drawHome(PKSEFramebuffer& fb) {
    const int contentW = fb.getWidth() - kCardMarginX * 2;
    const int cardW = (contentW - kCardGapX) / 2;

    fb.drawText(kCardMarginX, 76, "PokeBank NX Home", Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(kCardMarginX, 98,
                "Sources, app-owned storage and future organization tools",
                Colors::TextMuted, TextStyle::Caption);

    for (int index = 0; index < PokeBank::UIModel::appShellEntryCount(); ++index) {
        const int row = index / PokeBank::UIModel::APP_SHELL_COLUMNS;
        const int col = index % PokeBank::UIModel::APP_SHELL_COLUMNS;
        const int x = kCardMarginX + col * (cardW + kCardGapX);
        const int y = kCardTop + row * (kCardH + kCardGapY);
        const bool focused = index == selectedIndex;
        const auto& entry = PokeBank::UIModel::appShellEntry(index);

        drawFocusedCard(fb, x, y, cardW, kCardH, focused, 16);
        cardRects[static_cast<std::size_t>(index)] = {x, y, cardW, kCardH, index};

        fb.drawText(x + 22, y + 18, std::string(entry.title),
                    focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Heading);
        fb.drawText(x + 22, y + 58, std::string(entry.subtitle),
                    Colors::TextSecondary, TextStyle::Caption);

        const Color badge = badgeColor(entry.availability);
        int bwText = 0, bhText = 0;
        fb.measureText(std::string(entry.badge), bwText, bhText, TextStyle::Caption);
        const int badgeW = bwText + 24;
        const int badgeH = 28;
        const int badgeX = x + cardW - badgeW - 18;
        const int badgeY = y + 16;
        fb.drawPill(badgeX, badgeY, badgeW, badgeH, withAlpha(badge, 42));
        fb.drawPillBorder(badgeX, badgeY, badgeW, badgeH, badge, 1);
        fb.drawText(badgeX + (badgeW - bwText) / 2,
                    badgeY + (badgeH - bhText) / 2,
                    std::string(entry.badge), badge, TextStyle::Caption);

        if (focused) {
            const std::string action = entry.rootActionable ? "Open" : "View availability";
            fb.drawText(x + 22, y + 91, action, Colors::FocusBorder, TextStyle::Caption);
        }
    }

    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Open"},
                    {"+", "Settings"}, {"-", "Help"}, {"B", "Exit"}});
}

void AppShellScreen::drawSettings(PKSEFramebuffer& fb) {
    constexpr int w = 820, h = 574;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;

    drawModalSurface(fb, x, y, w, h);
    fb.drawText(x + 28, y + 18, "POKEBANK NX  /  SETTINGS",
                Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(x + 28, y + 44, "Application Settings",
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 76,
                "These are the same persisted preferences used inside a loaded workspace.",
                Colors::TextSecondary, TextStyle::Caption);

    constexpr int rowH = 58, rowGap = 7;
    const char* labels[6] = {
        "Auto-Backup on Load",
        "Theme",
        "Allow Illegal Values",
        "Bank Storage LGPE Move Warning",
        "Live Game Writes",
        "Enable Debug Logging",
    };
    std::string values[6] = {
        g_autoBackupEnabled ? "On" : "Off",
        std::string(themeModeName(g_themeMode)),
        g_allowIllegalEdits ? "On" : "Off",
        g_moveWarn ? "On" : "Off",
        "Locked",
        g_debugLogging ? "On" : "Off",
    };

    int rowY = y + 108;
    for (int index = 0; index < 6; ++index) {
        const bool focused = settingsIndex == index;
        drawFocusedCard(fb, x + 26, rowY, w - 52, rowH, focused, 12);
        fb.drawText(x + 48, rowY + 18, labels[index],
                    focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Body);

        int valueW = 0, valueH = 0;
        fb.measureText(values[index], valueW, valueH, TextStyle::Caption);
        const int pillW = valueW + 28;
        const int pillH = 30;
        const int pillX = x + w - 48 - pillW;
        const int pillY = rowY + (rowH - pillH) / 2;

        Color pillColor = Colors::TextMuted;
        if (index == 4) pillColor = Colors::Info;
        else if ((index == 0 && g_autoBackupEnabled) ||
                 (index == 3 && g_moveWarn) ||
                 (index == 5 && g_debugLogging)) pillColor = Colors::AccentPrimary;
        else if (index == 2 && g_allowIllegalEdits) pillColor = Colors::Warning;
        else if (index == 1) pillColor = Colors::FocusBorder;

        fb.drawPill(pillX, pillY, pillW, pillH, withAlpha(pillColor, 40));
        fb.drawPillBorder(pillX, pillY, pillW, pillH, pillColor, 1);
        fb.drawText(pillX + (pillW - valueW) / 2, pillY + (pillH - valueH) / 2,
                    values[index], pillColor, TextStyle::Caption);

        settingsRects[static_cast<std::size_t>(index)] =
            {x + 26, rowY, w - 52, rowH, index};
        rowY += rowH + rowGap;
    }

    if (statusFrames > 0 && !statusMessage.empty())
        fb.drawText(x + 30, y + h - 54, statusMessage.substr(0, 100),
                    Colors::TextMuted, TextStyle::Caption);

    drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Change"}, {"B", "Close"}});
}

void AppShellScreen::drawDiagnostics(PKSEFramebuffer& fb) {
    constexpr int w = 820, h = 520;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;

    drawModalSurface(fb, x, y, w, h);
    fb.drawText(x + 28, y + 18, "POKEBANK NX  /  DIAGNOSTICS  /  READ ONLY",
                Colors::Info, TextStyle::Caption);
    fb.drawText(x + 28, y + 44, "Build & Safety",
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 76,
                "No personal save payloads are shown on this screen.",
                Colors::TextSecondary, TextStyle::Caption);

    int lineY = y + 120;
    auto line = [&](const std::string& label, const std::string& value, Color valueColor) {
        fb.drawText(x + 38, lineY, label, Colors::TextMuted, TextStyle::Caption);
        fb.drawText(x + 300, lineY, value, valueColor, TextStyle::Caption);
        lineY += 36;
    };

    line("Version", VERSION_STRING, Colors::TextPrimary);
    line("Build", BUILD_COMMIT, Colors::TextPrimary);
    line("Theme", std::string(themeModeName(g_themeMode)), Colors::TextPrimary);
    line("Auto-backup", g_autoBackupEnabled ? "On" : "Off", Colors::TextPrimary);
    line("Debug logging", g_debugLogging ? "On" : "Off", Colors::TextPrimary);
    line("External source writes", "LOCKED", Colors::Info);
    line("Cross-game True Move", "LOCKED", Colors::Info);
    line("Master Vault backend", "Not implemented", Colors::TextSecondary);
    line("Global search index", "Not implemented", Colors::TextSecondary);
    line("Legacy Storage", "App-owned / not Master Vault", Colors::TextSecondary);

    drawNavBar(fb, {{"B", "Close"}});
}

void AppShellScreen::drawSectionInfo(PKSEFramebuffer& fb) {
    const auto& entry = PokeBank::UIModel::APP_SHELL_ENTRIES[
        static_cast<std::size_t>(infoSection)];
    const auto& lines = sectionInfoLines(infoSection);

    constexpr int w = 820;
    const int h = 180 + static_cast<int>(lines.size()) * 44;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;
    drawModalSurface(fb, x, y, w, h);

    fb.drawText(x + 28, y + 18,
                "POKEBANK NX  /  " + std::string(entry.badge),
                badgeColor(entry.availability), TextStyle::Caption);
    fb.drawText(x + 28, y + 44, std::string(entry.title),
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 76, std::string(entry.subtitle),
                Colors::TextSecondary, TextStyle::Caption);
    fb.drawFilledRoundedRect(x + 28, y + 104, w - 56, 3, 2,
                             badgeColor(entry.availability));

    int lineY = y + 132;
    for (const std::string& line : lines) {
        fb.drawText(x + 34, lineY, line, Colors::TextSecondary, TextStyle::Body);
        lineY += 44;
    }

    drawNavBar(fb, {{"B", "Back"}});
}

void AppShellScreen::draw(PKSEFramebuffer& fb) {
    drawAppBackdrop(fb);
    drawTitleBar(fb, "Home  /  v" + VERSION_STRING);

    drawHome(fb);

    if (overlay == Overlay::Settings) {
        drawSettings(fb);
    } else if (overlay == Overlay::Diagnostics) {
        drawDiagnostics(fb);
    } else if (overlay == Overlay::SectionInfo) {
        drawSectionInfo(fb);
    } else if (overlay == Overlay::Help) {
        drawInfoOverlay(fb, "Home & Product Areas", {
            "D-pad / Left Stick   Move between product areas",
            "A   Open an available area or review its current availability",
            "+   Open application Settings from anywhere on Home",
            "-   Show this help",
            "B   Exit PokeBank NX from Home",
            "WORKSPACE / COMING SOON cards never fabricate backend support"
        });
    }
}

} // namespace UI
