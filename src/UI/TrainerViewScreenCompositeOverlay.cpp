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
#include "UI/ScreenChrome.h"
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

namespace UI {

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    if (Gen1PokemonEditor::handleInput(*this, down)) return;
    updateGSCOverlay(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawGSCOverlay(fb);
    Gen1PokemonEditor::drawOverlay(*this, fb);
}

} // namespace UI
