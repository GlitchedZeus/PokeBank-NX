#include "UI/TrainerViewScreen.h"
#include "UI/Gen1PokemonEditorOverlay.h"
#include "UI/ClassicDefaultNickname.h"

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

namespace UI::Gen1PokemonEditor {
namespace {

// Cleanup3 remains the accepted Gen I product implementation. This narrow wrapper changes only the
// post-success presentation around its already-proven packed append: the native box/slot chosen by
// UX2 is preserved, the box cursor follows that actual packed slot, and success copy stays user-facing.
bool ux2StageAddWithClassicSelection(TrainerViewScreen& screen) {
    auto& state = ux2StateFor(screen);
    const int destinationBox = state.box;
    const int destinationSlot = state.slot;
    const std::string displayName = state.draft.nickname.empty()
        ? std::string(Names::getSpeciesName(state.draft.species)) : state.draft.nickname;
    if (!ux2StageAdd(screen)) return false;
    screen.selectedBoxIndex = destinationBox;
    screen.selectedItemIndex = destinationSlot;
    screen.detailViewActive = true;
    screen.postStatus(displayName + " added to Box " + std::to_string(destinationBox + 1), 260);
    return true;
}

void drawFooterWithClassicAddLabel(PKSEFramebuffer& fb, std::string text) {
    constexpr const char* oldLabel = "Stage Add";
    if (const auto pos = text.find(oldLabel); pos != std::string::npos)
        text.replace(pos, std::char_traits<char>::length(oldLabel), "Add");
    drawFooter(fb, text);
}

} // namespace
} // namespace UI::Gen1PokemonEditor

// Keep the accepted Cleanup3 source untouched while routing only its staged-Add call and footer
// presentation through the hardware-retest parity adapters above.
#define ux2StageAdd ux2StageAddWithClassicSelection
#define drawFooter drawFooterWithClassicAddLabel
#define isGen1SourceUX isGen1SourceUXCleanup3
#define handleInputUX handleInputUXCleanup3
#define drawOverlayUX drawOverlayUXCleanup3
#include "Gen1PokemonEditorOverlayUXCleanup3.inc"
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX
#undef drawFooter
#undef ux2StageAdd

#include "Gen1PokemonEditorOverlayFoundation.inc"
#include "Gen1PokemonEditorFoundationHardwareFix.inc"
#include "Gen1PokemonEditorPassiveView.inc"
#include "Gen2PokemonEditorFoundation.inc"

// Keep the accepted Gen II implementations available as exact bases, then place only
// the physical-hardware regression corrections around their public entry points.
#define handlePickerInput handlePickerInputBase
#define drawPickerOverlay drawPickerOverlayBase
#include "Gen2PokemonPickerOverlay.inc"
#undef drawPickerOverlay
#undef handlePickerInput
#include "Gen2HardwarePickerFix.inc"

#include "Gen2SharedPokemonSurface.inc"

#define handleUnifiedGen2SurfaceInput handleUnifiedGen2SurfaceInputBase
#define drawUnifiedGen2Surface drawUnifiedGen2SurfaceBase
#include "Gen2UnifiedPokemonWorkspace.inc"
#undef drawUnifiedGen2Surface
#undef handleUnifiedGen2SurfaceInput
#include "Gen2HardwareWorkspaceFix.inc"

#define drawFinalGen2Surface drawFinalGen2SurfaceBase
#include "Gen2SharedSurfaceParity.inc"
#undef drawFinalGen2Surface
#include "Gen2HardwareFinalFix.inc"

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
    if (!Gen2PokemonEditor::finalGen2SurfaceOwnsFrame(*this)) {
        drawGSCOverlay(fb);
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
