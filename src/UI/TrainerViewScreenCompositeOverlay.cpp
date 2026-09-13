#include "UI/TrainerViewScreen.h"
#include "UI/Gen1PokemonEditorOverlay.h"

// Pre-include every dependency used by the preserved GSC source before the narrow rename macros.
// Their include guards ensure the macros below rename only TrainerViewScreen's two method definitions,
// never unrelated update()/draw() declarations in headers.
#include "Enums/GameVersion.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "UI/Common.h"
#include "UI/ClassicInventoryOverlay.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

// Compile the already-accepted Gen II wrapper byte-for-byte under private method names. The public
// update()/draw() below then add the Gen I staged box editor in front/above it. Non-RBY behavior is
// therefore delegated straight to the exact accepted GSC/base implementation.
#define update updateGSCOverlay
#define draw drawGSCOverlay
#include "TrainerViewScreenGSCOverlay.inc"
#undef draw
#undef update

// Distinct entry points for the hardware-polish implementation. The accepted Gen I implementation
// still compiles normally from Gen1PokemonEditorOverlay.cpp; these names deliberately avoid replacing
// its symbols until the owner accepts the new UX on-device.
namespace UI::Gen1PokemonEditor {
[[nodiscard]] bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUX(TrainerViewScreen& screen, PKSEFramebuffer& fb);
} // namespace UI::Gen1PokemonEditor

// Include the new implementation under those isolated names. The dependency headers it includes are
// already guarded; the macros therefore rename only this implementation's Gen I overlay entry points.
#define isGen1Source isGen1SourceUX
#define handleInput handleInputUX
#define drawOverlay drawOverlayUX
#include "Gen1PokemonEditorOverlayUX.inc"
#undef drawOverlay
#undef handleInput
#undef isGen1Source

namespace UI {

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    if (Gen1PokemonEditor::handleInputUX(*this, down)) return;
    updateGSCOverlay(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawGSCOverlay(fb);
    Gen1PokemonEditor::drawOverlayUX(*this, fb);
}

} // namespace UI
