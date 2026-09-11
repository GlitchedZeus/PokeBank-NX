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

    const auto* red = findGame("red_gb");
    const auto* blue = findGame("blue_gb");
    const auto* yellow = findGame("yellow_gb");
    const auto* gold = findGame("gold_gbc");
    const auto* silver = findGame("silver_gbc");
    const auto* crystal = findGame("crystal_gbc");
    const auto* ruby = findGame("ruby_gba");
    const auto* sapphire = findGame("sapphire_gba");
    const auto* emerald = findGame("emerald_gba");
    const auto* fireRedGba = findGame("firered_gba");
    const auto* fireRedSwitch = findGame("firered_switch");
    const auto* leafGreenGba = findGame("leafgreen_gba");
    const auto* leafGreenSwitch = findGame("leafgreen_switch");
    assert(red && blue && yellow && gold && silver && crystal && ruby && sapphire && emerald &&
           fireRedGba && fireRedSwitch && leafGreenGba && leafGreenSwitch);
    assert(red->support == SourceSupport::ReadOnly);
    assert(blue->support == SourceSupport::ReadOnly);
    assert(yellow->support == SourceSupport::ReadOnly);
    assert(gold->support == SourceSupport::ReadOnly);
    assert(silver->support == SourceSupport::ReadOnly);
    assert(crystal->support == SourceSupport::ReadOnly);
    assert(red->platform == Platform::GameBoy);
    assert(blue->platform == Platform::GameBoy);
    assert(yellow->platform == Platform::GameBoy);
    assert(gold->platform == Platform::GameBoyColor);
    assert(silver->platform == Platform::GameBoyColor);
    assert(crystal->platform == Platform::GameBoyColor);
    assert(ruby->support == SourceSupport::ReadOnly);
    assert(sapphire->support == SourceSupport::ReadOnly);
    assert(emerald->support == SourceSupport::ReadOnly);
    assert(fireRedGba->id != fireRedSwitch->id);
    assert(leafGreenGba->id != leafGreenSwitch->id);
    assert(fireRedGba->platform == Platform::GameBoyAdvance);
    assert(fireRedSwitch->platform == Platform::NintendoSwitch);
    assert(platformName(Platform::GameBoy) == "Game Boy");
    assert(platformName(Platform::GameBoyColor) == "Game Boy Color");
    assert(platformName(Platform::GameBoyAdvance) == "Game Boy Advance");
    assert(platformName(Platform::NintendoSwitch) == "Nintendo Switch");
    assert(legacyPlatformAbbreviation("red_gb") == "GB");
    assert(legacyPlatformAbbreviation("blue_gb") == "GB");
    assert(legacyPlatformAbbreviation("yellow_gb") == "GB");
    assert(legacyPlatformAbbreviation("gold_gbc") == "GBC");
    assert(legacyPlatformAbbreviation("silver_gbc") == "GBC");
    assert(legacyPlatformAbbreviation("crystal_gbc") == "GBC");
    assert(legacyPlatformAbbreviation("firered_gba") == "GBA");
    assert(legacyPlatformAbbreviation("leafgreen_gba") == "GBA");
    assert(legacyPlatformAbbreviation("ruby_gba") == "GBA");
    assert(legacyPlatformAbbreviation("sapphire_gba") == "GBA");
    assert(legacyPlatformAbbreviation("emerald_gba") == "GBA");
    assert(legacyPlatformAbbreviation("firered_switch").empty());
    assert(gameCardArtworkPath("red_gb") == "romfs:/game_cards/red_gb.png");
    assert(gameCardArtworkPath("blue_gb") == "romfs:/game_cards/blue_gb.png");
    assert(gameCardArtworkPath("yellow_gb") == "romfs:/game_cards/yellow_gb.png");
    assert(gameCardArtworkPath("firered_gba") == "romfs:/game_cards/firered_gba.png");
    assert(gameCardArtworkPath("leafgreen_gba") == "romfs:/game_cards/leafgreen_gba.png");
    assert(gameCardArtworkPath("ruby_gba") == "romfs:/game_cards/ruby_gba.png");
    assert(gameCardArtworkPath("sapphire_gba") == "romfs:/game_cards/sapphire_gba.png");
    assert(gameCardArtworkPath("emerald_gba") == "romfs:/game_cards/emerald_gba.png");
    assert(gameCardArtworkPath("firered_switch").empty());
    assert(findGame("missing") == nullptr);
    assert(findSwitchGame(0) == nullptr);
}
