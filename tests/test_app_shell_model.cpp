#include <cassert>
#include <string_view>

#include "UI/AppShellModel.h"
#include "UI/OrganizationPreviewModel.h"

int main() {
    using namespace PokeBank::UIModel;

    static_assert(APP_SHELL_PRIMARY_COUNT == 2);
    static_assert(APP_SHELL_DOCK_COUNT == 4);
    static_assert(APP_SHELL_ENTRIES.size() == 6);

    assert(appShellEntryCount() == 6);
    assert(appShellEntry(0).section == AppShellSection::MasterVault);
    assert(appShellEntry(0).title == std::string_view("Master Vault"));
    assert(appShellEntry(1).section == AppShellSection::Pokedex);
    assert(appShellEntry(1).title == std::string_view("Pokédex"));
    assert(appShellEntry(2).section == AppShellSection::Banks);
    assert(appShellEntry(3).section == AppShellSection::Search);
    assert(appShellEntry(4).section == AppShellSection::Settings);
    assert(appShellEntry(5).section == AppShellSection::Games);

    assert(!appShellRootActionable(AppShellSection::MasterVault));
    assert(!appShellRootActionable(AppShellSection::Pokedex));
    assert(!appShellRootActionable(AppShellSection::Banks));
    assert(!appShellRootActionable(AppShellSection::Search));
    assert(appShellRootActionable(AppShellSection::Settings));
    assert(appShellRootActionable(AppShellSection::Games));
    assert(appShellPreviewable(AppShellSection::Pokedex));
    assert(appShellPreviewable(AppShellSection::Banks));
    assert(appShellPreviewable(AppShellSection::Search));

    assert(appShellMoveSelection(0, 0, 1) == 1);
    assert(appShellMoveSelection(1, 0, 1) == 2);
    assert(appShellMoveSelection(1, 0, -1) == 0);
    assert(appShellMoveSelection(2, -1, 0) == 5);
    assert(appShellMoveSelection(5, 1, 0) == 2);
    assert(appShellMoveSelection(3, 0, -1) == 1);

    assert(previewCount(OrganizationPreviewKind::Banks) == 6);
    assert(previewCount(OrganizationPreviewKind::Search) == 7);
    assert(previewCount(OrganizationPreviewKind::Collections) == 4);
    assert(BANK_BOX_PREVIEW[0].count == 0);
    assert(BANK_BOX_PREVIEW[0].capacity == 30);

    assert(appShellMoveSelection(-1, 0, 0) == 0);
    assert(appShellMoveSelection(99, 0, 0) == 0);
    return 0;
}
