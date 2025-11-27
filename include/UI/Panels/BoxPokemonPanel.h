#ifndef BOX_POKEMON_PANEL_H
#define BOX_POKEMON_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class TrainerViewScreen;

namespace Panels {
    void drawBoxPokemon(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height);
}

#endif // BOX_POKEMON_PANEL_H
