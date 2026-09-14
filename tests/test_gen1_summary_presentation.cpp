#include "UI/Gen1PokemonPresentation.h"
#include "UI/BattleStatRadarModel.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string_view>

using namespace PokeVault::Integration::Gen1;
using namespace PokeBank::UIModel;

int main() {
    PokemonRecord record{};
    record.species = 1;
    record.level = 5;
    record.dvs = {0, 8, 8, 8, 8};
    auto normal = presentGen1Pokemon(record);
    assert(!normal.shiny);
    assert((normal.battleStats == std::array<uint16_t,5>{19,10,10,10,12}));
    assert(!normal.capabilities.supportsNature && !normal.capabilities.supportsAbility);
    assert(!normal.capabilities.supportsHeldItem && !normal.capabilities.supportsFriendship);
    assert(!normal.capabilities.supportsEgg && !normal.capabilities.supportsMetLevel);
    assert(!normal.capabilities.supportsRibbons && !normal.capabilities.supportsMarks);
    assert(!normal.capabilities.hasSplitSpecial);
    assert(normal.battleStats.size() == 5);
    constexpr std::array<std::string_view,5> expected{"HP", "Atk", "Def", "Spe", "Spc"};
    for (size_t i = 0; i < 5; ++i) assert(gen1RadarLabels[i] == expected[i]);

    // Actual polymorphic sprite getter used by box, summary, party and carried previews.
    // It must ignore the modern identity arguments and use only this record's DVs.
    for (uint8_t atk = 0; atk < 16; ++atk) {
        record.dvs = {0, atk, 10, 10, 10};
        Pokemon::Pokemon1ReadOnly wrapper(record);
        const Pokemon::Pokemon& base = wrapper;
        const auto summary = presentGen1Pokemon(wrapper.strictRecord());
        const auto editor = presentGen1Pokemon(1, 5, {atk,10,10,10}, {});
        assert(summary.shiny == ShinyDVs::isShiny({atk,10,10,10}));
        assert(base.isShiny(0, "Bulbasaur") == summary.shiny);
        assert(base.isShiny(0xFFFFFFFF, "Anything") == summary.shiny);
        assert(base.clone()->isShiny(0, "") == summary.shiny);
        assert(summary.battleStats == editor.battleStats);
        assert(gen1RadarModel(summary.battleStats).normalized == gen1RadarModel(editor.battleStats).normalized);
    }
    // Real stat changes, including low-level integer rounding, feed the shared graph.
    const auto baseline = presentGen1Pokemon(1, 5, {8,8,8,8}, {});
    const auto dvs = presentGen1Pokemon(1, 5, {15,15,15,15}, {});
    const auto effort = presentGen1Pokemon(1, 5, {8,8,8,8}, {65535,65535,65535,65535,65535});
    const auto level = presentGen1Pokemon(1, 6, {8,8,8,8}, {});
    for (const auto& changed : {dvs, effort, level}) {
        assert(changed.battleStats != baseline.battleStats);
        assert(gen1RadarModel(changed.battleStats).normalized != gen1RadarModel(baseline.battleStats).normalized);
    }
    // One denominator preserves relative shape, all-zero input is safe, and full
    // uint16 range (including bucket boundaries) is finite and bounded.
    for (unsigned n = 0; n <= 65535; ++n) {
        std::array<uint16_t,5> stats{static_cast<uint16_t>(n), static_cast<uint16_t>(n/2), 0, 1, 5};
        const auto model = gen1RadarModel(stats);
        assert(std::isfinite(model.scale) && model.scale > 0);
        for (size_t i = 0; i < 5; ++i) {
            assert(std::isfinite(model.normalized[i]) && model.normalized[i] >= 0 && model.normalized[i] <= 1);
            assert(std::abs(model.normalized[i] * model.scale - stats[i]) < 0.01f);
        }
    }
    assert((gen1RadarModel({}).normalized == std::array<float,5>{}));
    assert(gen1RadarModel({19,10,10,10,12}).scale == gen1RadarModel({20,11,10,10,12}).scale);
    // Gen I Clefairy and Magnemite must not acquire Fairy/Steel typing.
    assert((presentGen1Pokemon(35, 5, {8,8,8,8}, {}).nativeTypes == std::array<uint8_t,2>{0,0}));
    assert((presentGen1Pokemon(81, 5, {8,8,8,8}, {}).nativeTypes == std::array<uint8_t,2>{23,23}));
    assert(gen1TypeSprite(22) == 11 && gen1TypeSprite(3) == 3);
    std::cout << "Gen I summary/editor stats, capabilities, sprite shininess and radar scaling: PASS\n";
}
