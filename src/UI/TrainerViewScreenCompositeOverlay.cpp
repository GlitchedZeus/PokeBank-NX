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

#define update updateGSCOverlay
#define draw drawGSCOverlay
#include "TrainerViewScreenGSCOverlay.inc"
#undef draw
#undef update

namespace UI::Gen1PokemonEditor {
// Preserve the hardware-proven pass-1 implementation under recovery/reference symbols. Cleanup #2
// reuses its staged/export helpers in the same translation unit while owning the active UI state.
[[nodiscard]] bool isGen1SourceUXBase(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXBase(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXBase(TrainerViewScreen& screen, PKSEFramebuffer& fb);

[[nodiscard]] bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUX(TrainerViewScreen& screen, PKSEFramebuffer& fb);
} // namespace UI::Gen1PokemonEditor

#define isGen1Source isGen1SourceUXBase
#define handleInput handleInputUXBase
#define drawOverlay drawOverlayUXBase
#define SelectedText Text
#include "Gen1PokemonEditorOverlayUX.inc"
#undef SelectedText
#undef drawOverlay
#undef handleInput
#undef isGen1Source

// Hardware cleanup pass #2: compact geometry, clipped logical editor, non-mutating hover preview,
// grouped move/stat editors, true five-stat radar, truthful legality wording, and visual clone browse.
#include "Gen1PokemonEditorOverlayUXCleanup2.inc"

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
