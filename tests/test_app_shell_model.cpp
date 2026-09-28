#include <cassert>
#include <string_view>

#include "UI/AppShellModel.h"

int main() {
    using namespace PokeBank::UIModel;

    static_assert(APP_SHELL_COLUMNS == 2);
    static_assert(APP_SHELL_ROWS == 4);
    static_assert(APP_SHELL_ENTRIES.size() == 8);

    assert(appShellEntryCount() == 8);
    assert(appShellEntry(0).section == AppShellSection::Games);
    assert(appShellEntry(0).title == std::string_view("Games & Sources"));
    assert(appShellEntry(1).section == AppShellSection::Storage);
    assert(appShellEntry(2).section == AppShellSection::Banks);
    assert(appShellEntry(6).section == AppShellSection::Settings);
    assert(appShellEntry(7).section == AppShellSection::Diagnostics);

    assert(appShellRootActionable(AppShellSection::Games));
    assert(appShellRootActionable(AppShellSection::Settings));
    assert(appShellRootActionable(AppShellSection::Diagnostics));
    assert(!appShellRootActionable(AppShellSection::Storage));
    assert(!appShellRootActionable(AppShellSection::Banks));
    assert(!appShellRootActionable(AppShellSection::Backups));
    assert(!appShellRootActionable(AppShellSection::Search));
    assert(!appShellRootActionable(AppShellSection::Collections));

    assert(appShellEntry(1).availability == AppShellAvailability::WorkspaceRequired);
    assert(appShellEntry(2).availability == AppShellAvailability::FutureBackend);
    assert(appShellEntry(7).availability == AppShellAvailability::ReadOnly);

    // 2 x 4 controller grid: both axes wrap and preserve the other axis.
    assert(appShellMoveSelection(0, 1, 0) == 1);
    assert(appShellMoveSelection(1, 1, 0) == 0);
    assert(appShellMoveSelection(0, -1, 0) == 1);
    assert(appShellMoveSelection(0, 0, 1) == 2);
    assert(appShellMoveSelection(6, 0, 1) == 0);
    assert(appShellMoveSelection(1, 0, -1) == 7);
    assert(appShellMoveSelection(5, -1, 0) == 4);

    // Invalid selection fails predictably to the first card before movement is applied.
    assert(appShellMoveSelection(-1, 0, 0) == 0);
    assert(appShellMoveSelection(99, 0, 0) == 0);

    return 0;
}
