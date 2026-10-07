#include "UI/AppShellScreen.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "Globals.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/OrganizationPreviewModel.h"
#include "UI/ScreenChrome.h"
#include "UI/ProductChrome.h"
#include "UI/TouchInput.h"
#include "Utils/Settings.h"

namespace UI {
namespace {
    constexpr int kHeroX = 28;
    constexpr int kHeroY = 112;
    constexpr int kHeroW = 472;
    constexpr int kHeroH = 438;
    constexpr int kPrimaryX = 532;
    constexpr int kPrimaryY = 132;
    constexpr int kPrimaryW = 714;
    constexpr int kPrimaryH = 98;
    constexpr int kPrimaryGap = 18;
    constexpr int kDockY = 398;
    constexpr int kDockSize = 88;
    constexpr int kDockGap = 26;

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

    void drawRootDockIcon(PKSEFramebuffer& fb,
                          PokeBank::UIModel::AppShellSection section,
                          int x, int y, int size, bool focused) {
        using PokeBank::UIModel::AppShellSection;
        const Color ink = focused ? Colors::SelectedText : Colors::TextPrimary;
        const int cx = x + size / 2;
        const int cy = y + size / 2;

        if (section == AppShellSection::Banks) {
            fb.drawRoundedRect(cx - 23, cy - 18, 46, 28, 7, ink, 2);
            fb.drawRoundedRect(cx - 17, cy - 8, 34, 28, 6, ink, 2);
        } else if (section == AppShellSection::Backups) {
            fb.drawRoundedRect(cx - 24, cy - 18, 48, 36, 7, ink, 2);
            fb.drawFilledRoundedRect(cx - 12, cy - 25, 24, 8, 3, ink);
            fb.drawFilledRoundedRect(cx - 14, cy + 2, 28, 4, 2, ink);
        } else if (section == AppShellSection::Search) {
            fb.drawCircle(cx - 5, cy - 5, 15, ink, 2);
            fb.drawFilledRoundedRect(cx + 7, cy + 8, 22, 5, 2, ink);
        } else if (section == AppShellSection::More) {
            for (int oy : {-10, 10})
                for (int ox : {-10, 10})
                    fb.drawFilledRoundedRect(cx + ox - 4, cy + oy - 4, 8, 8, 2, ink);
        } else if (section == AppShellSection::Settings) {
            fb.drawFilledCircle(cx, cy, 10, ink);
            fb.drawFilledRoundedRect(cx - 3, cy - 27, 6, 12, 3, ink);
            fb.drawFilledRoundedRect(cx - 3, cy + 15, 6, 12, 3, ink);
            fb.drawFilledRoundedRect(cx - 27, cy - 3, 12, 6, 3, ink);
            fb.drawFilledRoundedRect(cx + 15, cy - 3, 12, 6, 3, ink);
        } else if (section == AppShellSection::Games) {
            fb.drawRoundedRect(cx - 28, cy - 17, 56, 34, 15, ink, 2);
            fb.drawFilledRoundedRect(cx - 18, cy - 2, 15, 4, 2, ink);
            fb.drawFilledRoundedRect(cx - 12, cy - 8, 4, 16, 2, ink);
            fb.drawFilledCircle(cx + 12, cy - 4, 4, ink);
            fb.drawFilledCircle(cx + 20, cy + 5, 4, ink);
        }
    }

    const std::vector<std::string>& sectionInfoLines(PokeBank::UIModel::AppShellSection section) {
        using PokeBank::UIModel::AppShellSection;
        static const std::vector<std::string> vault{
            "Master Vault is coming in a future PokeBank NX update.",
            "Your stored Pokémon and collections will appear here when Vault storage is ready.",
            "Opening this page never changes any game or emulator save."
        };
        static const std::vector<std::string> fallback{
            "This destination is not available from the main menu yet."
        };

        switch (section) {
            case AppShellSection::MasterVault: return vault;
            default:                           return fallback;
        }
    }

    constexpr std::array<const char*, 6> kSettingsCategories{{
        "Look", "System", "Data", "Update", "Developer", "Info"
    }};

    int settingsOptionCount(int category) {
        switch (category) {
            case 0: return 1;
            case 1: return 3;
            case 2: return 2;
            case 3: return 2;
            case 4: return 2;
            case 5: return 2;
            default: return 1;
        }
    }

    struct SettingsRow {
        std::string label;
        std::string value;
        std::string detail;
        bool actionable = false;
    };

    std::vector<SettingsRow> settingsRows(int category) {
        switch (category) {
            case 0:
                return {{"Theme", std::string(themeModeName(g_themeMode)),
                         "Change the visual theme used throughout PokeBank NX.", true}};
            case 1:
                return {
                    {"Auto-Backup", g_autoBackupEnabled ? "On" : "Off",
                     "Create or reuse the established working backup before supported edits.", true},
                    {"Move Compatibility Warnings", g_moveWarn ? "On" : "Off",
                     "Warn when a move is unusual for the selected generation or species.", true},
                    {"Allow Illegal Values", g_allowIllegalEdits ? "On" : "Off",
                     "Permit explicitly supported out-of-range editor values.", true},
                };
            case 2:
                return {
                    {"Source Save Protection", "LOCKED",
                     "Original emulator and installed-title sources are never edited live.", false},
                    {"Backup / Staging", "App-owned",
                     "Edits continue through PokeBank-owned backup and staging workflows.", false},
                };
            case 3:
                return {
                    {"Installed Version", VERSION_STRING,
                     "Current PokeBank NX application version.", false},
                    {"Update Support", "Coming Soon",
                     "In-app update delivery is not enabled in this build.", false},
                };
            case 4:
                return {
                    {"Debug Logging", g_debugLogging ? "On" : "Off",
                     "Write diagnostic logs under sdmc:/switch/PokeBank-NX/logs/.", true},
                    {"Diagnostics / Build Info", "Open",
                     "Open read-only runtime and build diagnostics.", true},
                };
            case 5:
                return {
                    {"Version", VERSION_STRING,
                     "Installed application version.", false},
                    {"Build SHA", BUILD_COMMIT,
                     "Exact source revision embedded in this build.", false},
                };
            default:
                return {};
        }
    }
}

AppShellScreen::AppShellScreen(const NavigationState* resumeState) {
    if (!resumeState) return;
    selectedIndex = std::clamp(
        resumeState->selectedIndex, 0, PokeBank::UIModel::appShellEntryCount() - 1);
    settingsCategory = std::clamp(resumeState->settingsCategory, 0,
                                  static_cast<int>(kSettingsCategories.size()) - 1);
    settingsIndex = std::clamp(resumeState->settingsIndex, 0,
                               settingsOptionCount(settingsCategory) - 1);
    settingsCategoryFocused = resumeState->settingsCategoryFocused;
    previewIndex = std::max(0, resumeState->previewIndex);
    moreIndex = std::clamp(resumeState->moreIndex, 0, 5);
}

AppShellScreen::Action AppShellScreen::consumeAction() {
    const Action result = pendingAction;
    pendingAction = Action::None;
    return result;
}

void AppShellScreen::openSection(PokeBank::UIModel::AppShellSection section) {
    for (int i = 0; i < PokeBank::UIModel::appShellEntryCount(); ++i) {
        if (PokeBank::UIModel::appShellEntry(i).section == section) {
            selectedIndex = i;
            activateSelected();
            return;
        }
    }
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
    } else if (section == AppShellSection::More) {
        statusMessage.clear();
        statusFrames = 0;
        overlay = Overlay::More;
    } else if (section == AppShellSection::Settings) {
        statusMessage.clear();
        statusFrames = 0;
        overlay = Overlay::Settings;
    } else if (PokeBank::UIModel::appShellPreviewable(section)) {
        infoSection = section;
        overlay = Overlay::OrganizationPreview;
    } else {
        infoSection = section;
        overlay = Overlay::SectionInfo;
    }
}

void AppShellScreen::activateSetting() {
    bool changed = false;

    if (settingsCategory == 0 && settingsIndex == 0) {
        applyTheme(nextThemeMode(g_themeMode));
        setStatus("Theme changed to " + std::string(themeModeName(g_themeMode)) + ".");
        changed = true;
    } else if (settingsCategory == 1) {
        if (settingsIndex == 0) {
            g_autoBackupEnabled = !g_autoBackupEnabled;
            setStatus(g_autoBackupEnabled
                ? "Auto-backup enabled for supported installed-title workflows."
                : "Auto-backup disabled; supported workflows reuse their working copy.");
            changed = true;
        } else if (settingsIndex == 1) {
            g_moveWarn = !g_moveWarn;
            setStatus(g_moveWarn ? "Move compatibility warnings enabled."
                                 : "Move compatibility warnings disabled.");
            changed = true;
        } else if (settingsIndex == 2) {
            g_allowIllegalEdits = !g_allowIllegalEdits;
            setStatus(g_allowIllegalEdits
                ? "Illegal-value editing enabled for fields that explicitly support it."
                : "Legal value caps restored.");
            changed = true;
        }
    } else if (settingsCategory == 2) {
        setStatus(settingsIndex == 0
            ? "Locked: ordinary editing never writes emulator or installed-game sources."
            : "Backup and staging remain app-owned; source files stay protected.");
    } else if (settingsCategory == 3) {
        setStatus(settingsIndex == 0
            ? "This is the installed PokeBank NX version."
            : "Update support is Coming Soon.");
    } else if (settingsCategory == 4) {
        if (settingsIndex == 0) {
            g_debugLogging = !g_debugLogging;
            setStatus(g_debugLogging
                ? "Debug logging enabled under sdmc:/switch/PokeBank-NX/logs/."
                : "Debug logging disabled; no new log files will be written.");
            changed = true;
        } else {
            overlay = Overlay::Diagnostics;
        }
    } else if (settingsCategory == 5) {
        setStatus(settingsIndex == 0
            ? "Installed version information."
            : "Exact build revision embedded in this application.");
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

    if (overlay == Overlay::Diagnostics) {
        if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) overlay = Overlay::Settings;
        return;
    }
    if (overlay == Overlay::Help || overlay == Overlay::SectionInfo) {
        if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) overlay = Overlay::None;
        return;
    }

    if (overlay == Overlay::More) {
        constexpr int moreCount = 6;
        if (touch.justTouchedDown()) {
            for (const HitRect& rect : overlayRects)
                if (contains(rect, touch.x(), touch.y())) { moreIndex = rect.index; break; }
        }
        if (touch.justTapped()) {
            for (const HitRect& rect : overlayRects)
                if (contains(rect, touch.x(), touch.y())) { moreIndex = rect.index; kDown |= HidNpadButton_A; break; }
        } else if (touch.justReleased() && touch.dragged()) {
            const int dx = touch.deltaX(), dy = touch.deltaY();
            const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            if (ax >= 56 && ax >= ay) kDown |= dx < 0 ? HidNpadButton_Right : HidNpadButton_Left;
            else if (ay >= 56) kDown |= dy < 0 ? HidNpadButton_Down : HidNpadButton_Up;
        }
        if (kDown & HidNpadButton_B) {
            overlay = Overlay::None;
            statusMessage.clear();
            statusFrames = 0;
            return;
        }
        if (kDown & HidNpadButton_Left)
            moreIndex = (moreIndex + moreCount - 1) % moreCount;
        if (kDown & HidNpadButton_Right)
            moreIndex = (moreIndex + 1) % moreCount;
        if (kDown & HidNpadButton_Up)
            moreIndex = (moreIndex + moreCount - 2) % moreCount;
        if (kDown & HidNpadButton_Down)
            moreIndex = (moreIndex + 2) % moreCount;
        if (kDown & HidNpadButton_A)
            setStatus("Coming Soon — this module does not change save data.", 240);
        return;
    }

    if (overlay == Overlay::OrganizationPreview) {
        using PokeBank::UIModel::OrganizationPreviewKind;
        if (touch.justTouchedDown()) {
            for (const HitRect& rect : overlayRects)
                if (contains(rect, touch.x(), touch.y())) { previewIndex = rect.index; break; }
        }
        if (touch.justTapped()) {
            for (const HitRect& rect : overlayRects)
                if (contains(rect, touch.x(), touch.y())) { previewIndex = rect.index; kDown |= HidNpadButton_A; break; }
        } else if (touch.justReleased() && touch.dragged()) {
            const int dx = touch.deltaX(), dy = touch.deltaY();
            const int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            if (ax >= 56 && ax >= ay) kDown |= dx < 0 ? HidNpadButton_Right : HidNpadButton_Left;
            else if (ay >= 56) kDown |= dy < 0 ? HidNpadButton_Down : HidNpadButton_Up;
        }
        OrganizationPreviewKind kind = OrganizationPreviewKind::Banks;
        if (infoSection == PokeBank::UIModel::AppShellSection::Pokedex)
            kind = OrganizationPreviewKind::Collections;
        else if (infoSection == PokeBank::UIModel::AppShellSection::Search)
            kind = OrganizationPreviewKind::Search;

        const int count = PokeBank::UIModel::previewCount(kind);
        if (kDown & HidNpadButton_B) {
            overlay = Overlay::None;
            return;
        }
        if (kDown & HidNpadButton_Left)
            previewIndex = PokeBank::UIModel::previewMoveSelection(kind, previewIndex, -1, 0);
        if (kDown & HidNpadButton_Right)
            previewIndex = PokeBank::UIModel::previewMoveSelection(kind, previewIndex, 1, 0);
        if (kDown & HidNpadButton_Up)
            previewIndex = PokeBank::UIModel::previewMoveSelection(kind, previewIndex, 0, -1);
        if (kDown & HidNpadButton_Down)
            previewIndex = PokeBank::UIModel::previewMoveSelection(kind, previewIndex, 0, 1);
        if (kDown & HidNpadButton_A) {
            setStatus("Coming Soon — no save data was changed.", 240);
        }
        return;
    }

    if (overlay == Overlay::Settings) {
        if (touch.justTouchedDown()) {
            for (const HitRect& rect : settingsCategoryRects) {
                if (contains(rect, touch.x(), touch.y())) {
                    settingsCategory = rect.index;
                    settingsIndex = std::min(settingsIndex, settingsOptionCount(settingsCategory) - 1);
                    settingsCategoryFocused = true;
                    break;
                }
            }
            for (const HitRect& rect : settingsRects) {
                if (contains(rect, touch.x(), touch.y())) {
                    settingsIndex = rect.index; settingsCategoryFocused = false; break;
                }
            }
        }
        if (touch.justTapped()) {
            for (const HitRect& rect : settingsCategoryRects) {
                if (contains(rect, touch.x(), touch.y())) {
                    settingsCategory = rect.index;
                    settingsIndex = std::min(settingsIndex, settingsOptionCount(settingsCategory) - 1);
                    settingsCategoryFocused = true;
                    break;
                }
            }
            for (const HitRect& rect : settingsRects) {
                if (contains(rect, touch.x(), touch.y())) {
                    settingsIndex = rect.index; settingsCategoryFocused = false; kDown |= HidNpadButton_A; break;
                }
            }
        }
        if (kDown & HidNpadButton_B) {
            overlay = Overlay::None;
            statusMessage.clear();
            statusFrames = 0;
            return;
        }
        if (kDown & HidNpadButton_Left) {
            settingsCategoryFocused = true;
            return;
        }
        if (kDown & HidNpadButton_Right) {
            settingsCategoryFocused = false;
            settingsIndex = std::min(settingsIndex,
                                     settingsOptionCount(settingsCategory) - 1);
            return;
        }
        if (settingsCategoryFocused) {
            const int count = static_cast<int>(kSettingsCategories.size());
            if (kDown & HidNpadButton_Up)
                settingsCategory = (settingsCategory + count - 1) % count;
            if (kDown & HidNpadButton_Down)
                settingsCategory = (settingsCategory + 1) % count;
            settingsIndex = std::min(settingsIndex,
                                     settingsOptionCount(settingsCategory) - 1);
        } else {
            const int count = settingsOptionCount(settingsCategory);
            if (kDown & HidNpadButton_Up)
                settingsIndex = (settingsIndex + count - 1) % count;
            if (kDown & HidNpadButton_Down)
                settingsIndex = (settingsIndex + 1) % count;
            if (kDown & HidNpadButton_A) activateSetting();
        }
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

    if (touch.justTouchedDown()) {
        for (const HitRect& rect : cardRects) {
            if (contains(rect, touch.x(), touch.y())) { selectedIndex = rect.index; break; }
        }
    }
    if (touch.justTapped()) {
        for (const HitRect& rect : cardRects) {
            if (contains(rect, touch.x(), touch.y())) { selectedIndex = rect.index; kDown |= HidNpadButton_A; break; }
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
    using PokeBank::UIModel::appShellEntry;

    // Left hero panel: one visual identity surface instead of a developer dashboard.
    drawPanelSurface(fb, kHeroX, kHeroY, kHeroW, kHeroH, true, 22);
    fb.drawText(kHeroX + 28, kHeroY + 24, "POKEBANK NX", Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(kHeroX + 28, kHeroY + 56, "Your Pokémon.", Colors::TextPrimary, TextStyle::Title);
    fb.drawText(kHeroX + 28, kHeroY + 96, "One place.", Colors::TextPrimary, TextStyle::Title);

    const int markX = kHeroX + kHeroW / 2;
    const int markY = kHeroY + 226;
    fb.drawFilledCircle(markX, markY, 86, withAlpha(Colors::AccentPrimary, 30));
    fb.drawFilledCircle(markX, markY, 52, Colors::PanelAlt);
    fb.drawFilledRect(markX - 86, markY - 8, 172, 16, withAlpha(Colors::AccentPrimary, 46));
    fb.drawFilledCircle(markX, markY, 20, Colors::AccentPrimary);
    fb.drawFilledCircle(markX, markY, 10, Colors::Panel);

    fb.drawText(kHeroX + 28, kHeroY + 334,
                "Manage games, Banks, Pokédex and backups",
                Colors::TextSecondary, TextStyle::Body);
    fb.drawText(kHeroX + 28, kHeroY + 366,
                "from one console-style home.",
                Colors::TextSecondary, TextStyle::Body);
    fb.drawText(kHeroX + 28, kHeroY + 404,
                "Source saves stay protected.", Colors::Info, TextStyle::Caption);

    // Two large primary destinations, matching the HOME / Champions hierarchy.
    for (int index = 0; index < PokeBank::UIModel::APP_SHELL_PRIMARY_COUNT; ++index) {
        const int y = kPrimaryY + index * (kPrimaryH + kPrimaryGap);
        const auto& entry = appShellEntry(index);
        const bool focused = selectedIndex == index;
        drawFocusedCard(fb, kPrimaryX, y, kPrimaryW, kPrimaryH, focused, 26);
        cardRects[static_cast<std::size_t>(index)] =
            {kPrimaryX, y, kPrimaryW, kPrimaryH, index};

        fb.drawText(kPrimaryX + 30, y + 17, std::string(entry.title),
                    focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Heading);
        fb.drawText(kPrimaryX + 30, y + 52, std::string(entry.subtitle),
                    focused ? Colors::SelectedText : Colors::TextSecondary, TextStyle::Caption);

        int badgeW = 0, badgeH = 0;
        fb.measureText(std::string(entry.badge), badgeW, badgeH, TextStyle::Caption);
        const int px = kPrimaryX + kPrimaryW - badgeW - 48;
        const Color badge = badgeColor(entry.availability);
        fb.drawPill(px, y + 34, badgeW + 24, 30, withAlpha(badge, 36));
        fb.drawPillBorder(px, y + 34, badgeW + 24, 30, badge, 1);
        fb.drawText(px + 12, y + 42, std::string(entry.badge), badge, TextStyle::Caption);
    }

    fb.drawText(kPrimaryX, kDockY - 32, "NAVIGATION",
                Colors::TextMuted, TextStyle::Caption);

    // Compact secondary shell dock mirrors the non-settings Product Home destinations.
    // Settings is intentionally reachable only from Product Home's header gear.
    const int dockSpan = PokeBank::UIModel::APP_SHELL_DOCK_COUNT * kDockSize +
                         (PokeBank::UIModel::APP_SHELL_DOCK_COUNT - 1) * kDockGap;
    const int dockStartX = kPrimaryX + (kPrimaryW - dockSpan) / 2;
    for (int dock = 0; dock < PokeBank::UIModel::APP_SHELL_DOCK_COUNT; ++dock) {
        const int index = PokeBank::UIModel::APP_SHELL_PRIMARY_COUNT + dock;
        const auto& entry = appShellEntry(index);
        const int x = dockStartX + dock * (kDockSize + kDockGap);
        const bool focused = selectedIndex == index;

        drawFocusedCard(fb, x, kDockY, kDockSize, kDockSize, focused, kDockSize / 2);
        cardRects[static_cast<std::size_t>(index)] =
            {x, kDockY, kDockSize, kDockSize, index};
        drawRootDockIcon(fb, entry.section, x, kDockY, kDockSize, focused);

        if (focused) {
            int tw = 0, th = 0;
            fb.measureText(std::string(entry.title), tw, th, TextStyle::Caption);
            fb.drawText(x + (kDockSize - tw) / 2, kDockY + kDockSize + 10,
                        std::string(entry.title), Colors::FocusBorder, TextStyle::Caption);
        }
    }

    const auto& focusedEntry = appShellEntry(selectedIndex);
    fb.drawText(kPrimaryX, kDockY + 142,
                std::string(focusedEntry.subtitle),
                Colors::TextMuted, TextStyle::Caption);

    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Open"},
                    {"-", "Help"}, {"B", "Exit"}});
}

void AppShellScreen::drawSettings(PKSEFramebuffer& fb) {
    constexpr int x = 52, y = 82, w = 1176, h = 544;
    constexpr int leftW = 248;
    constexpr int gap = 18;
    constexpr int contentY = 108;
    constexpr int contentH = 374;
    constexpr int categoryRowH = 46;
    constexpr int optionRowH = 76;

    drawPanelSurface(fb, x, y, w, h, true, 18);
    fb.drawText(x + 28, y + 18, "SETTINGS",
                Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(x + 28, y + 44, "Application Settings",
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 76,
                "Categories on the left. Current options on the right.",
                Colors::TextSecondary, TextStyle::Caption);

    const int leftX = x + 24;
    const int rightX = leftX + leftW + gap;
    const int rightW = w - 48 - leftW - gap;

    drawPanelSurface(fb, leftX, y + contentY, leftW, contentH, false, 14);
    drawPanelSurface(fb, rightX, y + contentY, rightW, contentH, false, 14);

    int categoryY = y + contentY + 14;
    for (int index = 0; index < static_cast<int>(kSettingsCategories.size()); ++index) {
        const bool selected = settingsCategory == index;
        const bool focused = selected && settingsCategoryFocused;
        drawFocusedCard(fb, leftX + 12, categoryY, leftW - 24, categoryRowH,
                        focused, 10);
        if (selected && !focused)
            fb.drawRoundedRect(leftX + 12, categoryY, leftW - 24, categoryRowH,
                               10, Colors::AccentPrimary, 2);
        fb.drawText(leftX + 28, categoryY + 14, kSettingsCategories[index],
                    focused ? Colors::SelectedText
                            : selected ? Colors::TextPrimary : Colors::TextSecondary,
                    TextStyle::Body);
        settingsCategoryRects[static_cast<std::size_t>(index)] =
            {leftX + 12, categoryY, leftW - 24, categoryRowH, index};
        categoryY += categoryRowH + 2;
    }

    const auto rows = settingsRows(settingsCategory);
    fb.drawText(rightX + 22, y + contentY + 18,
                kSettingsCategories[static_cast<std::size_t>(settingsCategory)],
                Colors::AccentPrimary, TextStyle::Caption);

    for (auto& rect : settingsRects) rect = {};

    constexpr int optionStep = optionRowH + 10;
    const int optionViewportTop = y + contentY + 48;
    const int optionCount = static_cast<int>(rows.size());
    // Every Settings category currently fits in the pane (max 3 options). Keep these rows anchored:
    // a fake scroll surface adds gesture state without revealing any content and makes hitboxes harder
    // to reason about. Long/overflowing lists elsewhere keep true phone-style direct manipulation.
    for (int index = 0; index < optionCount; ++index) {
        const int rowY = optionViewportTop + index * optionStep;
        const auto& row = rows[static_cast<std::size_t>(index)];
        const bool focused = !settingsCategoryFocused && settingsIndex == index;
        drawFocusedCard(fb, rightX + 14, rowY, rightW - 28, optionRowH,
                        focused, 12);

        fb.drawText(rightX + 32, rowY + 12, row.label,
                    focused ? Colors::SelectedText : Colors::TextPrimary,
                    TextStyle::Body);
        fb.drawText(rightX + 32, rowY + 42, row.detail,
                    focused ? Colors::SelectedText : Colors::TextMuted,
                    TextStyle::Caption);

        int valueW = 0, valueH = 0;
        fb.measureText(row.value, valueW, valueH, TextStyle::Caption);
        const int pillW = std::min(230, valueW + 26);
        const int pillX = rightX + rightW - pillW - 28;
        Color pillColor = row.actionable ? Colors::AccentPrimary : Colors::TextMuted;
        if (row.value == "LOCKED") pillColor = Colors::Info;
        if (row.value == "Coming Soon") pillColor = Colors::TextMuted;
        if (row.label == "Allow Illegal Values" && g_allowIllegalEdits)
            pillColor = Colors::Warning;
        fb.drawPill(pillX, rowY + 10, pillW, 28, withAlpha(pillColor, 38));
        fb.drawPillBorder(pillX, rowY + 10, pillW, 28, pillColor, 1);
        fb.drawText(pillX + 12, rowY + 17,
                    row.value.size() > 28 ? row.value.substr(0, 25) + "..." : row.value,
                    pillColor, TextStyle::Caption);

        settingsRects[static_cast<std::size_t>(index)] =
            {rightX + 14, rowY, rightW - 28, optionRowH, index};
    }

    if (statusFrames > 0 && !statusMessage.empty())
        fb.drawText(x + 30, y + h - 42, statusMessage.substr(0, 110),
                    Colors::TextMuted, TextStyle::Caption);

    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"Left/Right", "Pane"},
                    {"A", "Change/Open"}, {"B", "Close"}});
}

void AppShellScreen::drawDiagnostics(PKSEFramebuffer& fb) {
    constexpr int x = 120, y = 94, w = 1040, h = 506;

    drawPanelSurface(fb, x, y, w, h, true, 18);
    fb.drawText(x + 28, y + 18, "DIAGNOSTICS  /  READ ONLY",
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
    line("Master Vault backend", "Coming Soon", Colors::TextSecondary);
    line("Global search index", "Coming Soon", Colors::TextSecondary);
    line("Legacy Storage", "App-owned / not Master Vault", Colors::TextSecondary);

    drawNavBar(fb, {{"B", "Close"}});
}


void AppShellScreen::drawOrganizationPreview(PKSEFramebuffer& fb) {
    using PokeBank::UIModel::AppShellSection;
    constexpr int x = 24, y = 78, w = 1232, h = 548;
    drawPanelSurface(fb, x, y, w, h, true, 18);

    const bool isBanks = infoSection == AppShellSection::Banks;
    const bool isSearch = infoSection == AppShellSection::Search;
    const bool isPokedex = infoSection == AppShellSection::Pokedex;
    const char* title = isBanks ? "Banks & Boxes"
                      : isSearch ? "Search"
                                 : "Pokédex & Collections";
    const char* subtitle = isBanks
        ? "No Banks Created — your future Banks will appear here."
        : isSearch
            ? "Search will become available when the Master Vault index is ready."
            : "Species, forms, cries, Living Dex and Shiny Dex.";

    const std::string sectionLabel = isBanks ? "POKEBANK NX  /  BANKS"
        : isSearch ? "POKEBANK NX  /  SEARCH"
                   : "POKEBANK NX  /  POKÉDEX";
    fb.drawText(x + 28, y + 18, sectionLabel,
                Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(x + 28, y + 46, title, Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 80, subtitle, Colors::TextSecondary, TextStyle::Caption);

    if (isBanks) {
        const int bankX = x + 28, bankY = y + 122, bankW = 330, bankH = 364;
        drawPanelSurface(fb, bankX, bankY, bankW, bankH, false, 14);
        fb.drawText(bankX + 20, bankY + 18, "Named Banks", Colors::TextPrimary, TextStyle::Heading);
        fb.drawText(bankX + 20, bankY + 56, "No Banks Created", Colors::TextSecondary, TextStyle::Body);
        fb.drawText(bankX + 20, bankY + 88, "Your Banks will appear here.",
                    Colors::TextMuted, TextStyle::Caption);
        fb.drawText(bankX + 20, bankY + 142, "MASTER VAULT", Colors::AccentPrimary, TextStyle::Caption);
        fb.drawText(bankX + 20, bankY + 170, "Coming Soon", Colors::TextPrimary, TextStyle::Heading);
        fb.drawText(bankX + 20, bankY + 212,
                    "Game saves stay protected while", Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(bankX + 20, bankY + 238,
                    "PokeBank storage is being completed.", Colors::TextSecondary, TextStyle::Caption);

        const int gridX = bankX + bankW + 24, gridY = bankY, gridW = w - 28 - 28 - bankW - 24;
        fb.drawText(gridX, gridY + 4, "Bank Boxes", Colors::TextPrimary, TextStyle::Heading);
        fb.drawText(gridX, gridY + 38, "Empty box placeholders — no Pokémon are stored here yet.",
                    Colors::TextMuted, TextStyle::Caption);
        constexpr int cols = 3;
        const int gap = 14;
        const int boxW = (gridW - gap * 2) / cols;
        const int boxH = 116;
        for (int i = 0; i < static_cast<int>(PokeBank::UIModel::BANK_BOX_PREVIEW.size()); ++i) {
            const int bx = gridX + (i % cols) * (boxW + gap);
            const int by = gridY + 72 + (i / cols) * (boxH + gap);
            drawFocusedCard(fb, bx, by, boxW, boxH, i == previewIndex, 12);
            overlayRects[static_cast<std::size_t>(i)] = {bx, by, boxW, boxH, i};
            const auto& box = PokeBank::UIModel::BANK_BOX_PREVIEW[static_cast<std::size_t>(i)];
            fb.drawText(bx + 16, by + 16, std::string(box.title),
                        i == previewIndex ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Body);
            fb.drawText(bx + 16, by + 52, "Empty", Colors::TextSecondary, TextStyle::Caption);
            fb.drawText(bx + 16, by + 78,
                        std::to_string(box.count) + " / " + std::to_string(box.capacity),
                        Colors::TextMuted, TextStyle::Caption);
        }
    } else if (isSearch) {
        const int qx = x + 28, qy = y + 120, qw = w - 56;
        drawPanelSurface(fb, qx, qy, qw, 66, false, 12);
        fb.drawText(qx + 18, qy + 14, "Search Pokémon", Colors::TextPrimary, TextStyle::Body);
        fb.drawText(qx + 190, qy + 14, "Search is not available yet", Colors::TextMuted, TextStyle::Body);
        fb.drawText(qx + 18, qy + 40, "Your search filters will appear here when Master Vault search is ready.",
                    Colors::TextMuted, TextStyle::Caption);

        fb.drawText(qx, qy + 92, "Filter / organization controls", Colors::TextPrimary, TextStyle::Heading);
        constexpr int cols = 2;
        const int gapX = 18, gapY = 10;
        const int filterW = (qw - gapX) / cols;
        const int filterH = 54;
        for (int i = 0; i < static_cast<int>(PokeBank::UIModel::SEARCH_FILTER_PREVIEW.size()); ++i) {
            const int fx = qx + (i % cols) * (filterW + gapX);
            const int fy = qy + 132 + (i / cols) * (filterH + gapY);
            drawFocusedCard(fb, fx, fy, filterW, filterH, i == previewIndex, 10);
            overlayRects[static_cast<std::size_t>(i)] = {fx, fy, filterW, filterH, i};
            const auto& f = PokeBank::UIModel::SEARCH_FILTER_PREVIEW[static_cast<std::size_t>(i)];
            fb.drawText(fx + 14, fy + 16, std::string(f.label),
                        i == previewIndex ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Caption);
            int vw = 0, vh = 0;
            fb.measureText(std::string(f.value), vw, vh, TextStyle::Caption);
            fb.drawText(fx + filterW - vw - 16, fy + 16, std::string(f.value),
                        Colors::TextMuted, TextStyle::Caption);
        }

        const int emptyY = qy + 132 + 4 * (filterH + gapY) + 2;
        fb.drawText(qx, emptyY, "No results yet.", Colors::TextMuted, TextStyle::Caption);
    } else {
        const int gx = x + 28, gy = y + 126;
        const int gap = 18;
        const int cardW = (w - 56 - gap) / 2;
        const int cardH = 150;
        for (int i = 0; i < static_cast<int>(PokeBank::UIModel::COLLECTION_PREVIEW.size()); ++i) {
            const int cx = gx + (i % 2) * (cardW + gap);
            const int cy = gy + (i / 2) * (cardH + gap);
            drawFocusedCard(fb, cx, cy, cardW, cardH, i == previewIndex, 14);
            overlayRects[static_cast<std::size_t>(i)] = {cx, cy, cardW, cardH, i};
            const auto& c = PokeBank::UIModel::COLLECTION_PREVIEW[static_cast<std::size_t>(i)];
            fb.drawText(cx + 20, cy + 18, std::string(c.title),
                        i == previewIndex ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(cx + 20, cy + 58, std::string(c.subtitle),
                        Colors::TextSecondary, TextStyle::Caption);
            fb.drawText(cx + 20, cy + 94, "Coming Soon", Colors::TextMuted, TextStyle::Body);
            fb.drawText(cx + 20, cy + 120,
                        isPokedex ? "Collection progress will appear here" : "Available with Master Vault",
                        Colors::TextMuted, TextStyle::Caption);
        }
    }

    if (statusFrames > 0 && !statusMessage.empty())
        fb.drawText(x + 30, y + h - 28, statusMessage, Colors::TextMuted, TextStyle::Caption);

    drawNavBar(fb, {{"D-pad/Stick", "Preview navigation"}, {"A", "Explain"}, {"B", "Back"}});
}


void AppShellScreen::drawMore(PKSEFramebuffer& fb) {
    constexpr int x = 24, y = 78, w = 1232, h = 548;
    drawPanelSurface(fb, x, y, w, h, true, 18);

    fb.drawText(x + 28, y + 18, "MORE", Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(x + 28, y + 46, "Future PokeBank NX Features",
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 80,
                "Post-v1.0 destinations live here without pretending unfinished backends are available.",
                Colors::TextSecondary, TextStyle::Caption);

    struct FutureModule { const char* title; const char* subtitle; };
    static constexpr FutureModule modules[6] = {
        {"Mystery Gifts", "Event distributions and gift library"},
        {"Legality", "Validation and encounter review"},
        {"Transfer History", "Where Pokémon moved and when"},
        {"Clone Lineage", "Original / Clone / Legit Clone relationships"},
        {"Collections", "Living Dex, Shiny Living Dex and Favorites"},
        {"Trade & Gifting", "Future exchange features"},
    };

    constexpr int gapX = 18, gapY = 14;
    const int gridX = x + 28;
    const int gridY = y + 120;
    const int cardW = (w - 56 - gapX) / 2;
    const int cardH = 112;
    for (int i = 0; i < 6; ++i) {
        const int cx = gridX + (i % 2) * (cardW + gapX);
        const int cy = gridY + (i / 2) * (cardH + gapY);
        const bool focused = i == moreIndex;
        drawFocusedCard(fb, cx, cy, cardW, cardH, focused, 14);
        overlayRects[static_cast<std::size_t>(i)] = {cx, cy, cardW, cardH, i};
        fb.drawText(cx + 20, cy + 16, modules[i].title,
                    focused ? Colors::SelectedText : Colors::TextPrimary,
                    TextStyle::Heading);
        fb.drawText(cx + 20, cy + 53, modules[i].subtitle,
                    Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(cx + 20, cy + 79, "Coming Soon",
                    Colors::Info, TextStyle::Caption);
    }

    if (statusFrames > 0 && !statusMessage.empty())
        fb.drawText(x + 30, y + h - 28, statusMessage,
                    Colors::TextMuted, TextStyle::Caption);

    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Details"}, {"B", "Back"}});
}

void AppShellScreen::drawSectionInfo(PKSEFramebuffer& fb) {
    const auto& entry = PokeBank::UIModel::APP_SHELL_ENTRIES[
        static_cast<std::size_t>(infoSection)];
    const auto& lines = sectionInfoLines(infoSection);

    constexpr int x = 120, y = 118, w = 1040;
    const int h = std::min(430, 180 + static_cast<int>(lines.size()) * 44);
    drawPanelSurface(fb, x, y, w, h, true, 18);

    fb.drawText(x + 28, y + 18,
                "POKEBANK NX  /  " + std::string(entry.title),
                Colors::AccentPrimary, TextStyle::Caption);
    fb.drawText(x + 28, y + 44, std::string(entry.title),
                Colors::TextPrimary, TextStyle::Heading);
    fb.drawText(x + 28, y + 76, std::string(entry.subtitle),
                Colors::TextSecondary, TextStyle::Caption);
    fb.drawFilledRoundedRect(x + 28, y + 104, w - 56, 3, 2,
                             Colors::AccentPrimary);

    int lineY = y + 132;
    for (const std::string& line : lines) {
        fb.drawText(x + 34, lineY, line, Colors::TextSecondary, TextStyle::Body);
        lineY += 44;
    }

    drawNavBar(fb, {{"B", "Back"}});
}

void AppShellScreen::draw(PKSEFramebuffer& fb) {
    for (auto& rect : overlayRects) rect = {};
    drawAppBackdrop(fb);
    drawProductTitleBar(fb);

    // The approved selected-game screen is the product root. Secondary destinations reuse these
    // overlays without flashing the retired dashboard behind them.
    if (overlay == Overlay::None) {
        drawHome(fb);
    } else if (overlay == Overlay::Settings) {
        drawSettings(fb);
    } else if (overlay == Overlay::Diagnostics) {
        drawDiagnostics(fb);
    } else if (overlay == Overlay::OrganizationPreview) {
        drawOrganizationPreview(fb);
    } else if (overlay == Overlay::More) {
        drawMore(fb);
    } else if (overlay == Overlay::SectionInfo) {
        drawSectionInfo(fb);
    } else if (overlay == Overlay::Help) {
        drawInfoOverlay(fb, "PokeBank NX", {
            "D-pad / Left Stick   Navigate",
            "A   Open the focused item",
            "B   Return to Games"
        });
    }
}

} // namespace UI
