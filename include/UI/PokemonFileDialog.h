#pragma once
// The ONE Import from file dialog of every Gen I-V session (owner Oct 28, Feature A UI).
//
// A modal SD-card browser (UI/PokemonFileDialogModel.h for the controls) that owns the frame while
// it is open: TrainerViewScreen::update hands it the input before any generation surface, and
// TrainerViewScreen::draw paints it last (under the status line). It reads folders and the one file
// picked; it never writes to the SD card except the favourite / remembered folders in settings.cfg.
// Picking a file reads it (refused from its size alone when it is larger than a Pokemon record),
// decodes it and hands it to the generation's `stage` callback; on success the dialog closes,
// remembers the folder and posts the callback's status; on refusal it stays open and shows why.

#include "PokemonFile/NativePokemonFile.h"
#include "UI/TouchInput.h"

#include <cstdint>
#include <functional>
#include <string>

namespace UI {
class TrainerViewScreen;
class PKSEFramebuffer;
namespace PokemonFileDialog {

// Returns true when the record was staged; `status` is then the success line. On refusal `error`
// holds the editor's reason and nothing may have been staged.
using StageFn = std::function<bool(const PokeBank::PokemonFile::Decoded& file, std::string& status,
                                   std::string& error)>;

void openImport(TrainerViewScreen& screen, int generation, StageFn stage);
[[nodiscard]] bool active(const TrainerViewScreen& screen) noexcept;
void close(TrainerViewScreen& screen) noexcept;

// Frame input. Returns true (input consumed) while the dialog is open.
bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held, int stickX, int stickY,
                 const TouchInput& touch);
// Paints the dialog over the frame when it is open (TrainerViewScreen::draw calls it last).
void drawDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb);

}  // namespace PokemonFileDialog
}  // namespace UI
