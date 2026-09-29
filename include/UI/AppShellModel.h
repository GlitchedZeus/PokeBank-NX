#ifndef UI_APP_SHELL_MODEL_H
#define UI_APP_SHELL_MODEL_H

#include <array>
#include <cstddef>
#include <string_view>

namespace PokeBank::UIModel {

    enum class AppShellSection {
        MasterVault,
        Pokedex,
        Games,
        Banks,
        Backups,
        Search,
        More,
        Settings,
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
    inline constexpr int APP_SHELL_DOCK_COUNT = 5;

    inline constexpr std::array<AppShellEntry, 8> APP_SHELL_ENTRIES{{
        {AppShellSection::MasterVault, "Master Vault",
         "Your central Pokémon library", "COMING SOON",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Pokedex, "Pokédex",
         "Species, forms, cries and collection progress", "COMING SOON",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Games, "Games",
         "Selected-game workspace, party, boxes and editor", "",
         AppShellAvailability::Ready, true},
        {AppShellSection::Banks, "Banks",
         "Named Banks and Boxes", "NO BANKS YET",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::Backups, "Backups",
         "Backup history for supported selected games", "GAME WORKSPACE",
         AppShellAvailability::WorkspaceRequired, false},
        {AppShellSection::Search, "Search",
         "Find Pokémon across the future Vault index", "COMING SOON",
         AppShellAvailability::FutureBackend, false},
        {AppShellSection::More, "More",
         "Future PokeBank NX features", "COMING SOON",
         AppShellAvailability::FutureBackend, true},
        {AppShellSection::Settings, "Settings",
         "Themes, safety and app preferences", "",
         AppShellAvailability::Ready, true},
    }};

    constexpr int appShellEntryCount() {
        return static_cast<int>(APP_SHELL_ENTRIES.size());
    }

    constexpr const AppShellEntry& appShellEntry(int index) {
        return APP_SHELL_ENTRIES[static_cast<std::size_t>(index)];
    }

    // Product hierarchy:
    //   0 Master Vault
    //   1 Pokédex
    //   2..6 compact dock (Games, Banks, Backups, Search, More)
    //   7 Settings (header / + shortcut, never part of the bottom dock).
    // Backups remain selected-game context; future destinations stay truthful scaffolding.
    constexpr int appShellMoveSelection(int current, int dx, int dy) {
        if (current < 0 || current >= appShellEntryCount()) current = 0;

        if (current < APP_SHELL_PRIMARY_COUNT) {
            if (dy < 0) return current == 0 ? 0 : 0;
            if (dy > 0) return current == 0 ? 1 : APP_SHELL_PRIMARY_COUNT;
            return current;
        }

        const int dockFirst = APP_SHELL_PRIMARY_COUNT;
        const int dockLast = APP_SHELL_PRIMARY_COUNT + APP_SHELL_DOCK_COUNT - 1;
        if (current > dockLast) return 0;
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
        return section == AppShellSection::Games ||
               section == AppShellSection::More ||
               section == AppShellSection::Settings;
    }

} // namespace PokeBank::UIModel

#endif
