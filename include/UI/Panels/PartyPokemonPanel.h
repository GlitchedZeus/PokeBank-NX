#ifndef UI_PANELS_PARTY_POKEMON_PANEL_H
#define UI_PANELS_PARTY_POKEMON_PANEL_H

#include <vector>
#include <memory>
#include <cstdint>

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
}
namespace Pokemon {
    class Pokemon;
}

namespace UI {
namespace Panels {
    void drawPartyPokemon(UI::PKSEFramebuffer& fb, const std::vector<std::unique_ptr<Pokemon::Pokemon>>& party, uint32_t trainerID32, int x, int y, int width, int height, int selectedIndex = -1);
}
}

#endif
