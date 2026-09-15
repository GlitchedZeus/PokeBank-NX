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
#include "UI/PokemonViewActions.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/TouchInput.h"
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

[[nodiscard]] bool isGen1SourceUXBase(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXBase(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXBase(TrainerViewScreen& screen, PKSEFramebuffer& fb);
[[nodiscard]] bool isGen1SourceUXCleanup2(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXCleanup2(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXCleanup2(TrainerViewScreen& screen, PKSEFramebuffer& fb);
[[nodiscard]] bool isGen1SourceUXCleanup3(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down);
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY);
void drawOverlayUXCleanup3(TrainerViewScreen& screen, PKSEFramebuffer& fb);
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

#define isGen1SourceUX isGen1SourceUXCleanup2
#define handleInputUX handleInputUXCleanup2
#define drawOverlayUX drawOverlayUXCleanup2
#include "Gen1PokemonEditorOverlayUXCleanup2.inc"
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX

#define isGen1SourceUX isGen1SourceUXCleanup3
#define handleInputUX handleInputUXCleanup3
#define drawOverlayUX drawOverlayUXCleanup3
#include "Gen1PokemonEditorOverlayUXCleanup3.inc"
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX

#include "Gen1PokemonEditorOverlayFoundation.inc"
#include "Gen1PokemonEditorFoundationHardwareFix.inc"
#include "Gen1PokemonEditorPassiveView.inc"
#include "Gen2PokemonEditorFoundation.inc"
#include "Gen2PokemonPickerOverlay.inc"
#include "Gen2SharedPokemonSurface.inc"
#include "Gen2UnifiedPokemonWorkspace.inc"
#include "Gen2SharedSurfaceParity.inc"

namespace UI {
namespace {

bool gen2ClassicBoxFooterActive(const TrainerViewScreen& screen) noexcept {
    const bool gsc = screen.trainer.getGameGroup() == Enums::GameVersion::GSC &&
        (screen.sourceGameId == "gold_gbc" || screen.sourceGameId == "silver_gbc" ||
         screen.sourceGameId == "crystal_gbc");
    return gsc && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes && screen.detailViewActive &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.carrying() && !screen.currentlySelecting;
}

void drawGen2ClassicBoxFooter(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!gen2ClassicBoxFooterActive(screen)) return;
    bool occupied = false;
    if (screen.selectedBoxIndex >= 0 && screen.selectedItemIndex >= 0 &&
        screen.selectedBoxIndex < static_cast<int>(screen.trainer.getBoxCount()) &&
        screen.selectedItemIndex < static_cast<int>(screen.trainer.getSlotsPerBox())) {
        occupied = static_cast<bool>(screen.trainer.boxes[static_cast<std::size_t>(screen.selectedBoxIndex)]
                                                [static_cast<std::size_t>(screen.selectedItemIndex)]);
    }
    if (occupied)
        drawNavBar(fb, {{"A", "Actions"}, {"L/R", "Box"}, {"B", "Back"}});
    else
        drawNavBar(fb, {{"A", "Actions"}, {"X", "Add"}, {"L/R", "Box"}, {"B", "Back"}});
}

} // namespace

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    const u64 held = padGetButtons(&pad);
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);

    // Gen II uses the same top-level Pokemon editor shell as Gen I. Generation-specific code below
    // supplies PK2 rules/data only; the preserved GSC trainer/inventory overlay is not a competing
    // Pokemon editor.
    if (Gen2PokemonEditor::handleFinalGen2SurfaceInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen2PokemonEditor::handlePickerInput(*this, down, held, stick.x, stick.y, touch)) return;

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        if (Gen1PokemonEditor::handleInputUXCleanup3(*this, down, held, stick.x, stick.y)) return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::handleFoundationPassiveViewInput(*this, down, touchedButtonId(touch));
        return;
    }

    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y)) return;
    updateGSCOverlay(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    // Exactly one top-level Pokemon surface owns the frame. GSC still owns its trainer/inventory
    // surfaces, but it never draws underneath Actions/View/Edit/Create/Review.
    if (!Gen2PokemonEditor::finalGen2SurfaceOwnsFrame(*this)) {
        drawGSCOverlay(fb);
        // The preserved base screen still draws its generic footer; overwrite it with the classic
        // Pokemon-box control language that the production Gen II input route actually implements.
        drawGen2ClassicBoxFooter(*this, fb);
    }

    if (Gen2PokemonEditor::drawFinalGen2Surface(*this, fb)) {
        Gen2PokemonEditor::drawPickerOverlay(*this, fb);
        return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        Gen1PokemonEditor::drawOverlayUXCleanup3(*this, fb);
        return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::drawFoundationPassiveView(*this, fb);
        return;
    }

    Gen1PokemonEditor::drawOverlayUX(*this, fb);
    Gen1PokemonEditor::drawFoundationBottomSplit(*this, fb);
}

} // namespace UI
