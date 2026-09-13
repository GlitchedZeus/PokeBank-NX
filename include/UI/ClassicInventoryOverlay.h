#ifndef POKEBANK_UI_CLASSIC_INVENTORY_OVERLAY_H
#define POKEBANK_UI_CLASSIC_INVENTORY_OVERLAY_H

#include <cstdint>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;

namespace ClassicInventory {

[[nodiscard]] bool isClassicSource(const TrainerViewScreen& screen) noexcept;
// True only when PokeBank owns a staged classic editor for this read-only source.
[[nodiscard]] bool stagedEditingAvailable(TrainerViewScreen& screen);
// Returns true when the classic inventory layer consumed this frame's input.
[[nodiscard]] bool handleInput(TrainerViewScreen& screen, uint64_t down);
void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb);
// Rebuilds only the presentation lists from staged semantic inventory. It never writes a source save.
bool refreshPresentation(TrainerViewScreen& screen);

} // namespace ClassicInventory
} // namespace UI

#endif
