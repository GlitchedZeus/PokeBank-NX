#include "Integration/Encounter/EncounterGuardrails.h"
#include "UI/SpeciesChangeLevelPolicy.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>

int main() {
    namespace E = PokeVault::Integration::EncounterGuardrails;

    const auto crystalPikachu = E::forGameSpecies("crystal_gbc", 25);
    assert(!crystalPikachu.empty());
    assert(std::all_of(crystalPikachu.begin(), crystalPikachu.end(), [](const auto& e) {
        return e.sourceGameId == "crystal_gbc" && e.species == 25 &&
               e.minLevel <= e.maxLevel && e.maxLevel <= 100;
    }));
    // PKHeX Gen II static encounter: Pikachu at the Crystal Celadon Game Corner, Lv.25.
    assert(std::any_of(crystalPikachu.begin(), crystalPikachu.end(), [](const auto& e) {
        return e.location == 71 && e.minLevel == 25 && e.maxLevel == 25;
    }));
    assert(E::locationAllowed("crystal_gbc", 25, 71));
    assert(!E::locationAllowed("crystal_gbc", 25, 1));
    const auto* gameCorner = E::find("crystal_gbc", 25, 71, 25);
    assert(gameCorner && gameCorner->fixedLevel());
    assert(E::find("crystal_gbc", 25, 71, 83) == nullptr);

    // Exact-game static examples pin real stored location IDs and fixed levels.
    const auto sapphireTorchic = E::forGameSpecies("sapphire_gba", 255);
    assert(std::any_of(sapphireTorchic.begin(), sapphireTorchic.end(), [](const auto& e) {
        return e.location == 16 && e.minLevel == 5 && e.maxLevel == 5;
    }));
    const auto fireRedBulbasaur = E::forGameSpecies("firered_gba", 1);
    assert(std::any_of(fireRedBulbasaur.begin(), fireRedBulbasaur.end(), [](const auto& e) {
        return e.location == 88 && e.minLevel == 5 && e.maxLevel == 5;
    }));

    // A PokeBank-owned generated draft can browse the same species across supported games.
    const auto allPikachu = E::forSpeciesAllSupported(25);
    bool sawCrystal = false, sawGen3 = false;
    for (const auto& e : allPikachu) {
        sawCrystal |= e.sourceGameId == "crystal_gbc";
        sawGen3 |= e.sourceGameId == "ruby_gba" || e.sourceGameId == "sapphire_gba" ||
                   e.sourceGameId == "emerald_gba" || e.sourceGameId == "firered_gba" ||
                   e.sourceGameId == "leafgreen_gba";
        assert(e.minLevel <= e.maxLevel);
    }
    assert(sawCrystal && sawGen3);

    // Shared species-change policy consumes the exact-game encounter minimum.
    namespace LevelPolicy = PokeBank::UIModel::SpeciesChangeLevelPolicy;
    assert(E::minimumLevel("crystal_gbc", 16) == 2); // Pidgey
    assert(E::minimumLevel("ruby_gba", 261) == 2); // Poochyena
    assert(E::minimumLevel("sapphire_gba", 261) == 2);
    assert(E::minimumLevel("emerald_gba", 261) == 2);
    assert(E::minimumLevel("firered_gba", 16) == 2);
    assert(E::minimumLevel("leafgreen_gba", 16) == 2);
    assert(LevelPolicy::defaultLevel("crystal_gbc", 16) == 2);
    assert(LevelPolicy::defaultLevel("sapphire_gba", 261) == 2);

    // Games not yet represented by the encounter provider use one stable fallback;
    // they never inherit the previous species' level.
    assert(!E::minimumLevel("red_gb", 16));
    assert(!E::minimumLevel("gold_gbc", 16));
    assert(LevelPolicy::defaultLevel("red_gb", 16) == LevelPolicy::fallbackLevel);
    assert(LevelPolicy::defaultLevel("gold_gbc", 16) == LevelPolicy::fallbackLevel);

    // Same numeric location in the wrong exact game is not treated as equivalent.
    assert(!E::locationAllowed("sapphire_gba", 1, 88));

    std::cout << "Gen II/III exact-game encounter editor guardrails: PASS\n";
}
