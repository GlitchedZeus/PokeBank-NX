#ifndef PARTY_POKEMON_PANEL_H
#define PARTY_POKEMON_PANEL_H

#include <vector>
#include <memory>
#include <cstdint>

// Forward declarations
class PKSEFramebuffer;
class PKM;

namespace Panels {
    void drawPartyPokemon(PKSEFramebuffer& fb, const std::vector<std::unique_ptr<PKM>>& party, uint32_t trainerID32, int x, int y, int width, int height, int selectedIndex = -1);
}

#endif // PARTY_POKEMON_PANEL_H
