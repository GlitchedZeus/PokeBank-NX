#include "Games/GameIdentity.h"

#include <cassert>
#include <set>
#include <string>

using namespace PokeVault::Games;

int main() {
    const auto games = allGameDescriptors();
    assert(games.size() == 23);

    std::set<std::string> ids;
    std::set<uint64_t> switchIds;
    for (const auto& game : games) {
        assert(!game.id.empty());
        assert(!game.title.empty());
        assert(game.dataGeneration >= 1 && game.dataGeneration <= 9);
        assert(ids.insert(std::string(game.id)).second);
        if (game.switchTitleId != 0) {
            assert(game.isSwitchRelease());
            assert(switchIds.insert(game.switchTitleId).second);
            assert(findSwitchGame(game.switchTitleId) == &game);
        }
        assert(findGame(game.id) == &game);
    }

    const auto* fireRedGba = findGame("firered_gba");
    const auto* fireRedSwitch = findGame("firered_switch");
    const auto* leafGreenGba = findGame("leafgreen_gba");
    const auto* leafGreenSwitch = findGame("leafgreen_switch");
    assert(fireRedGba && fireRedSwitch && leafGreenGba && leafGreenSwitch);
    assert(fireRedGba->id != fireRedSwitch->id);
    assert(leafGreenGba->id != leafGreenSwitch->id);
    assert(fireRedGba->platform == Platform::GameBoyAdvance);
    assert(fireRedSwitch->platform == Platform::NintendoSwitch);
    assert(platformName(Platform::GameBoyAdvance) == "Game Boy Advance");
    assert(platformName(Platform::NintendoSwitch) == "Nintendo Switch");
    assert(findGame("missing") == nullptr);
    assert(findSwitchGame(0) == nullptr);
}
