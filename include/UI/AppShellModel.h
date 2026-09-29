#ifndef UI_APP_SHELL_MODEL_H
#define UI_APP_SHELL_MODEL_H

#include <array>
#include <cstddef>
#include <string_view>

namespace PokeBank::UIModel {

    enum class AppShellSection {
        MasterVault,
        Pokedex,
        Storage,
        Banks,
        Backups,
        Search,
        Settings,
        Games,
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

    inline constexpr int APP_SHELL_PRIMARY_COUNT = 2;
    inline constexpr int APP_SHELL_DOCK_COUNT = 6;

    inline constexpr std::array<AppShellEntry, 8> APP_SHELL_ENTRIES{{
        {AppShellSection::MasterVault, "Master Vault",
         "Your central Pokémon library", "FOUNDATION",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Pokedex, "Pokédex",
         "Species, forms, cries and collection progress", "PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Storage, "Storage",
         "Legacy app-owned storage", "WORKSPACE",
         AppShellAvailability::WorkspaceRequired, false},
        {AppShellSection::Banks, "Banks",
         "Named Banks and Boxes", "PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Backups, "Backups",
         "Per-game backup history", "PER GAME",
         AppShellAvailability::WorkspaceRequired, false},
        {AppShellSection::Search, "Search",
         "Search, filters and collections", "PREVIEW",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Settings, "Settings",
         "Themes, safety and app preferences", "READY",
         AppShellAvailability::Ready, true},
        {AppShellSection::Games, "Games",
         "Profiles, saves, party, edit and launch", "READY",
         AppShellAvailability::Ready, true},
    }};

    constexpr int appShellEntryCount() {
        return static_cast<int>(APP_SHELL_ENTRIES.size());
    }

    constexpr const AppShellEntry& appShellEntry(int index) {
        return APP_SHELL_ENTRIES[static_cast<std::size_t>(index)];
    }

    // Professional HOME-style layout:
    //   0 Master Vault
    //   1 Pokédex
    //   2..7 circular dock (Storage, Banks, Backups, Search, Settings, Games)
    constexpr int appShellMoveSelection(int current, int dx, int dy) {
        if (current < 0 || current >= appShellEntryCount()) current = 0;

        if (current < APP_SHELL_PRIMARY_COUNT) {
            if (dy < 0) return current == 0 ? 0 : 0;
            if (dy > 0) return current == 0 ? 1 : APP_SHELL_PRIMARY_COUNT;
            return current;
        }

        const int dockFirst = APP_SHELL_PRIMARY_COUNT;
        const int dockLast = appShellEntryCount() - 1;
        if (dy < 0) return 1;
        if (dx < 0) return current == dockFirst ? dockLast : current - 1;
        if (dx > 0) return current == dockLast ? dockFirst : current + 1;
        return current;
    }

    constexpr bool appShellPreviewable(AppShellSection section) {
        return section == AppShellSection::Pokedex ||
               section == AppShellSection::Banks ||
               section == AppShellSection::Search;
    }

    constexpr bool appShellRootActionable(AppShellSection section) {
        return section == AppShellSection::Settings ||
               section == AppShellSection::Games;
    }

} // namespace PokeBank::UIModel

#endif
