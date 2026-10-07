#ifndef POKEBANK_GEN1_POKEMON_PRESENTATION_H
#define POKEBANK_GEN1_POKEMON_PRESENTATION_H
#include "Integration/Gen1/Gen1Shiny.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "UI/PokemonEditorFoundationContract.h"

namespace PokeBank::UIModel {
// Current PK1 format, never species debut or origin generation. Drafts and parsed
// records converge here; mechanics remain owned by the existing helpers.
struct Gen1PokemonPresentation {
    PokemonEditorFoundation::Capabilities capabilities =
        PokemonEditorFoundation::capabilitiesForGeneration(PokemonEditorFoundation::Generation::Gen1);
    bool shiny = false;
    std::array<uint16_t,5> battleStats{};
    std::array<uint8_t,2> nativeTypes{};
};
inline Gen1PokemonPresentation presentGen1Pokemon(uint16_t species, uint8_t level,
        const std::array<uint8_t,4>& dvs, const std::array<uint16_t,5>& statExperience) noexcept {
    using namespace PokeVault::Integration::Gen1;
    Gen1PokemonPresentation result;
    result.shiny = ShinyDVs::isShiny(dvs);
    result.battleStats = StagedPokemonEditor::calculateBattleStats(species, level, dvs, statExperience).asArray();
    result.nativeTypes = StagedPokemonEditor::personalTypes(species);
    return result;
}
inline Gen1PokemonPresentation presentGen1Pokemon(const PokeVault::Integration::Gen1::PokemonRecord& p) noexcept {
    return presentGen1Pokemon(p.species, p.level, {p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]}, p.statExperience);
}
// Convert genuine Gen I type numbering to sprite atlas numbering.
constexpr uint8_t gen1TypeSprite(uint8_t type) noexcept {
    if (type <= 5) return type;
    if (type == 7 || type == 8) return type - 1;
    if (type >= 20 && type <= 26) return type - 11;
    return 255;
}
} // namespace PokeBank::UIModel
#endif
