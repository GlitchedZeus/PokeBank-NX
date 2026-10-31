#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen1 {
namespace {
#include "Gen1PersonalData.inc"
}

std::array<uint8_t,2> StagedPokemonEditor::personalTypes(uint16_t species) noexcept {
    if (species < 1 || species > 151) return {0, 0};
    return {kPersonal[species][5], kPersonal[species][6]};
}


} // namespace PokeVault::Integration::Gen1