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

#define update updateGSCOverlay
#define draw drawGSCOverlay
#include "TrainerViewScreenGSCOverlay.inc"
#undef draw
#undef update

namespace UI::Gen1PokemonEditor {
using SourceGame = PokeVault::Integration::Gen1::SourceGame;
using PokeVault::Integration::Gen1::parse;

// Preserve the hardware-proven pass-1 implementation under recovery/reference symbols.
[[nodiscard]] bool isGen1SourceUXBase(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXBase(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXBase(TrainerViewScreen& screen, PKSEFramebuffer& fb);

// Cleanup #2 remains compiled under explicit recovery symbols. Cleanup #3 owns the
// live entry points while reusing Cleanup #2's accepted staged/export helpers.
[[nodiscard]] bool isGen1SourceUXCleanup2(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXCleanup2(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXCleanup2(TrainerViewScreen& screen, PKSEFramebuffer& fb);

[[nodiscard]] bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down);
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY);
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

// Hardware cleanup pass #2 stays byte-for-byte available as the immediate recovery/reference layer.
#define isGen1SourceUX isGen1SourceUXCleanup2
#define handleInputUX handleInputUXCleanup2
#define drawOverlayUX drawOverlayUXCleanup2
#include "Gen1PokemonEditorOverlayUXCleanup2.inc"
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX

// Cleanup pass #3: PKSE-style Gen I Create/Edit structure, five-stat radar, DV-derived shiny UX,
// numbered normal/shiny Species preview, shared stick navigation, move compatibility, clone sprites,
// mature read-only View reuse, and content-aware Pending Changes.
#include "Gen1PokemonEditorOverlayUXCleanup3.inc"

namespace UI {

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    const u64 held = padGetButtons(&pad);
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);
    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y)) return;
    updateGSCOverlay(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawGSCOverlay(fb);
    Gen1PokemonEditor::drawOverlayUX(*this, fb);
}

} // namespace UI
