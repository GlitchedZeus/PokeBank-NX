#ifndef UI_APP_SHELL_MODEL_H
#define UI_APP_SHELL_MODEL_H

#include <array>
#include <cstddef>
#include <string_view>

namespace PokeBank::UIModel {

    enum class AppShellSection {
        Games,
        Storage,
        Banks,
        Backups,
        Search,
        Collections,
        Settings,
        Diagnostics,
    };

    enum class AppShellAvailability {
        Ready,
        WorkspaceRequired,
        FutureBackend,
        ReadOnly,
    };

    struct AppShellEntry {
        AppShellSection section;
        std::string_view title;
        std::string_view subtitle;
        std::string_view badge;
        AppShellAvailability availability;
        bool rootActionable;
    };

    inline constexpr int APP_SHELL_COLUMNS = 2;
    inline constexpr int APP_SHELL_ROWS = 4;

    inline constexpr std::array<AppShellEntry, 8> APP_SHELL_ENTRIES{{
        {AppShellSection::Games, "Games & Sources",
         "Validated saves and source instances", "READY",
         AppShellAvailability::Ready, true},
        {AppShellSection::Storage, "Storage",
         "App-owned Legacy Storage inside a workspace", "WORKSPACE",
         AppShellAvailability::WorkspaceRequired, false},
        {AppShellSection::Banks, "Banks",
         "Named Banks and Box organization foundation", "UI PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Backups, "Backups",
         "PokeBank backups are currently game-scoped", "PER GAME",
         AppShellAvailability::WorkspaceRequired, false},
        {AppShellSection::Search, "Search",
         "Filter and sort presentation foundation", "UI PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Collections, "Collections",
         "Living Dex, Shiny Dex, Favorites and Recent", "UI PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Settings, "Settings",
         "Themes, backups and safety preferences", "READY",
         AppShellAvailability::Ready, true},
        {AppShellSection::Diagnostics, "Diagnostics",
         "Build identity and safety state", "READ ONLY",
         AppShellAvailability::ReadOnly, true},
    }};

    constexpr int appShellEntryCount() {
        return static_cast<int>(APP_SHELL_ENTRIES.size());
    }

    constexpr const AppShellEntry& appShellEntry(int index) {
        return APP_SHELL_ENTRIES[static_cast<std::size_t>(index)];
    }

    constexpr int appShellMoveSelection(int current, int dx, int dy) {
        if (current < 0 || current >= appShellEntryCount()) current = 0;
        int row = current / APP_SHELL_COLUMNS;
        int col = current % APP_SHELL_COLUMNS;

        col = (col + (dx % APP_SHELL_COLUMNS) + APP_SHELL_COLUMNS) % APP_SHELL_COLUMNS;
        row = (row + (dy % APP_SHELL_ROWS) + APP_SHELL_ROWS) % APP_SHELL_ROWS;
        return row * APP_SHELL_COLUMNS + col;
    }

    constexpr bool appShellPreviewable(AppShellSection section) {
        return section == AppShellSection::Banks ||
               section == AppShellSection::Search ||
               section == AppShellSection::Collections;
    }

    constexpr bool appShellRootActionable(AppShellSection section) {
        return section == AppShellSection::Games ||
               section == AppShellSection::Settings ||
               section == AppShellSection::Diagnostics;
    }

} // namespace PokeBank::UIModel

#endif
