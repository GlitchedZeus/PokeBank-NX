#ifndef POKEBANK_UI_CLASSIC_INVENTORY_OVERLAY_H
#define POKEBANK_UI_CLASSIC_INVENTORY_OVERLAY_H

#include <cstdint>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;
class TouchInput;

namespace ClassicInventory {

[[nodiscard]] bool isClassicSource(const TrainerViewScreen& screen) noexcept;
// True only when PokeBank owns a staged classic editor for this read-only source.
[[nodiscard]] bool stagedEditingAvailable(TrainerViewScreen& screen);
// Top Up / Add Missing (owner Oct 28, Feature B) is offered: a staged Bag editor exists and the save
// is not a Japanese Gen I / II save kept read-only by Integration/Common/JapaneseEditingGate.h.
[[nodiscard]] bool topUpAvailable(TrainerViewScreen& screen);
// Returns true when the classic inventory layer consumed this frame's input.
[[nodiscard]] bool handleInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch);
void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb);
// Rebuilds only the presentation lists from staged semantic inventory. It never writes a source save.
bool refreshPresentation(TrainerViewScreen& screen);

} // namespace ClassicInventory
} // namespace UI

#endif
