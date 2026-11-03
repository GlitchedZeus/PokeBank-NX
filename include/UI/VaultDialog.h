#pragma once
// The ONE Master Vault browser of every Gen I-V session (Master Vault v1, build flag OFF by default:
// Vault/VaultFeature.h). Opened by "Withdraw from Vault" on an empty slot: it lists the Vault's
// records of the open save's generation, Y searches (live, as typed) and highlights the matching
// part of each row, A withdraws a COPY of the focused record through the generation's own staged
// import (the same StageFn Import from file uses, so occupied / capacity refusals are unchanged).
// The Vault record stays in the Vault; the source save on the SD card is never written.
// Modal like UI/PokemonFileDialog.h: it owns the frame's input and paints last.

#include "PokemonFile/NativePokemonFile.h"
#include "UI/PokemonFileDialog.h"
#include "UI/TouchInput.h"
#include "Vault/VaultStore.h"

#include <cstdint>
#include <string>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;
namespace VaultDialog {

// The app's one Vault (sdmc:/switch/PokeBank-NX/vault), opened on first use and kept open. Null with
// `error` set when it cannot be opened (the caller refuses the action with that reason).
PokeBank::Vault::Store* sharedStore(std::string& error);

void openWithdraw(TrainerViewScreen& screen, int generation, PokemonFileDialog::StageFn stage);
[[nodiscard]] bool active(const TrainerViewScreen& screen) noexcept;
void close(TrainerViewScreen& screen) noexcept;

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                 const TouchInput& touch);
void drawDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb);

}  // namespace VaultDialog
}  // namespace UI
