#ifndef POKEBANK_UI_GEN1_POKEMON_EDITOR_OVERLAY_H
#define POKEBANK_UI_GEN1_POKEMON_EDITOR_OVERLAY_H

#include <cstdint>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;

// TrainerViewScreenCompositeOverlay.cpp is the touch-aware composition layer around the preserved
// base TrainerViewScreen implementation. The base .inc keeps these layout constants private to its
// translation unit, so mirror the accepted Party surface geometry here for direct card hit-testing.
// Keep these values in lockstep with TrainerViewScreenBase.inc; touch contract tests guard the use.
inline constexpr int LEFT_PANEL_X = 12;
inline constexpr int CONTENT_PANEL_Y = 80;
inline constexpr int CONTENT_PANEL_HEIGHT = 560;

namespace Gen1PokemonEditor {

[[nodiscard]] bool isGen1Source(const TrainerViewScreen& screen) noexcept;
// Returns true when the Generation I boxed-Pokemon layer consumed this frame's input.
[[nodiscard]] bool handleInput(TrainerViewScreen& screen, uint64_t down);
void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb);

} // namespace Gen1PokemonEditor
} // namespace UI

#endif
