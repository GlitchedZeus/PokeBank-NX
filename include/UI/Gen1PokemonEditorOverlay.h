#ifndef POKEBANK_UI_GEN1_POKEMON_EDITOR_OVERLAY_H
#define POKEBANK_UI_GEN1_POKEMON_EDITOR_OVERLAY_H

#include <cstdint>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;

namespace Gen1PokemonEditor {

[[nodiscard]] bool isGen1Source(const TrainerViewScreen& screen) noexcept;
// Returns true when the Generation I boxed-Pokemon layer consumed this frame's input.
[[nodiscard]] bool handleInput(TrainerViewScreen& screen, uint64_t down);
void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb);

} // namespace Gen1PokemonEditor
} // namespace UI

#endif
