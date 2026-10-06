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

// Species changes use the shared exact-game level/EXP initialization policy; overlays must not reintroduce inherited level semantics.
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
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY, const TouchInput& touch);
void drawOverlayUXCleanup3(TrainerViewScreen& screen, PKSEFramebuffer& fb);
[[nodiscard]] bool isGen1SourceUX(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down);
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY);
[[nodiscard]] bool handleInputUX(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch);
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

#define ux2StageAdd ux2StageAddWithClassicSelection
#define drawFooter drawFooterWithClassicAddLabel
#define isGen1SourceUX isGen1SourceUXCleanup3
#define handleInputUX handleInputUXCleanup3
#define drawOverlayUX drawOverlayUXCleanup3
// The Gen I touch overlays are preserved .inc layers; adapt their legacy free hit-test spelling
// to the owning TrainerViewScreen without changing the editor implementation itself.
#define touchedButtonDownId(touchArg) screen.touchedButtonDownId(touchArg)
#define touchedButtonId(touchArg) screen.touchedButtonId(touchArg)
#include "Gen1PokemonEditorOverlayUXCleanup3.inc"
#undef touchedButtonId
#undef touchedButtonDownId
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX
#undef drawFooter
#undef ux2StageAdd

#define touchedButtonDownId(touchArg) screen.touchedButtonDownId(touchArg)
#define touchedButtonId(touchArg) screen.touchedButtonId(touchArg)
#include "Gen1PokemonEditorOverlayFoundation.inc"
#undef touchedButtonId
#undef touchedButtonDownId
#include "Gen1PokemonEditorFoundationHardwareFix.inc"
#include "Gen1PokemonEditorPassiveView.inc"

// Keep the historical no-touch Gen II foundation overload compilable while the actual composite
// route below supplies the live TouchInput to every active picker/surface.
namespace UI::Gen2PokemonEditor {
namespace {
const TouchInput kLegacyNoTouch{};
} // namespace
} // namespace UI::Gen2PokemonEditor
#define touch kLegacyNoTouch
#include "Gen2PokemonEditorFoundation.inc"
#undef touch

// Older Gen II presentation layers call the two-argument Held Item renderer. Forward them through
// the session owner so the touch-aware renderer still publishes real cell hitboxes.
namespace UI::Gen2PokemonEditor {
namespace {
void drawHeldItemPicker(PKSEFramebuffer& fb, const State& state) {
    auto* owner = const_cast<TrainerViewScreen*>(state.owner);
    if (owner) drawHeldItemPicker(*owner, fb, state);
}
} // namespace
} // namespace UI::Gen2PokemonEditor

#define handlePickerInput handlePickerInputBase
#define drawPickerOverlay drawPickerOverlayBase
#include "Gen2PokemonPickerOverlay.inc"
#undef drawPickerOverlay
#undef handlePickerInput

// The hardware move-picker renderer already has this layout locally; expose the same exact compact
// layout to its touch-drag handler without altering the accepted picker source layer.
namespace UI::Gen2PokemonEditor {
namespace {
constexpr auto kTouchMovePickerLayout = PokeBank::UIModel::MovePickerPresentation::compactPickerLayout();
} // namespace
} // namespace UI::Gen2PokemonEditor
#define layout kTouchMovePickerLayout
#include "Gen2HardwarePickerFix.inc"
#undef layout

#include "Gen2SharedPokemonSurface.inc"

#define handleUnifiedGen2SurfaceInput handleUnifiedGen2SurfaceInputBase
#define drawUnifiedGen2Surface drawUnifiedGen2SurfaceBase
// The unified workspace already owns a live `touch` parameter; route it into the newly touch-aware
// Held Item picker while preserving the older six-argument source call.
#define handleItemPicker(screenArg, stateArg, downArg, heldArg, stickXArg, stickYArg) \
    handleItemPicker(screenArg, stateArg, downArg, heldArg, stickXArg, stickYArg, touch)
#include "Gen2UnifiedPokemonWorkspace.inc"
#undef handleItemPicker
#undef drawUnifiedGen2Surface
#undef handleUnifiedGen2SurfaceInput
#include "Gen2HardwareWorkspaceFix.inc"

#define drawFinalGen2Surface drawFinalGen2SurfaceBase
#include "Gen2SharedSurfaceParity.inc"
#undef drawFinalGen2Surface
#include "Gen2HardwareFinalFix.inc"

#include "ClassicPackedMoveOverlay.inc"
#include "ClassicReleaseActionFix.inc"
#include "Gen3SharedPokemonSurface.inc"
#include "Gen4SharedPokemonSurface.inc"

namespace UI {
namespace {

void clampSourceBoxSelection(TrainerViewScreen& screen) noexcept {
    if (screen.selectedMode != TrainerViewScreen::ViewMode::Boxes ||
        !screen.detailViewActive || screen.selectedItemIndex < 0) {
        return;
    }

    const int capacity = static_cast<int>(screen.trainer.getSlotsPerBox());
    if (capacity <= 0) {
        screen.selectedItemIndex = -1;
        return;
    }
    if (screen.selectedItemIndex >= capacity)
        screen.selectedItemIndex = capacity - 1;
}

bool classicPackedMoveLayerAvailable(const TrainerViewScreen& screen) noexcept {
    if (Gen3SharedEditorSurface::isGen3Gba(screen))
        return !Gen3SharedEditorSurface::ownsFrame(screen);
    if (Gen1PokemonEditor::isGen1SourceUX(screen)) {
        const auto& state = Gen1PokemonEditor::ux2StateFor(screen);
        return state.mode == decltype(state.mode)::Closed &&
               !Gen1PokemonEditor::foundationPickerActive(screen) &&
               !Gen1PokemonEditor::foundationPassiveViewActive(screen);
    }
    const bool gsc = screen.sourceGameId == "gold_gbc" ||
                     screen.sourceGameId == "silver_gbc" ||
                     screen.sourceGameId == "crystal_gbc";
    if (gsc) {
        const auto& state = Gen2PokemonEditor::stateFor(screen);
        return state.mode == decltype(state.mode)::None && !editorOverlayState(screen).active;
    }
    return false;
}

bool gen2ClassicBoxFooterActive(const TrainerViewScreen& screen) noexcept {
    const bool gsc = screen.trainer.getGameGroup() == Enums::GameVersion::GSC &&
        (screen.sourceGameId == "gold_gbc" || screen.sourceGameId == "silver_gbc" ||
         screen.sourceGameId == "crystal_gbc");
    return gsc && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes && screen.detailViewActive &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.carrying() && !screen.swapActive && !screen.currentlySelecting;
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

constexpr int kPartyTouchBase = 6200;

bool basePartyTouchActive(const TrainerViewScreen& screen) noexcept {
    return screen.detailViewActive && screen.selectedMode == TrainerViewScreen::ViewMode::Party &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.releaseConfirmActive && !screen.storageExitConfirmActive && !screen.groupMenuActive &&
        !screen.carrying() && !screen.swapActive && !screen.currentlySelecting;
}

void publishBasePartyTouchTargets(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!basePartyTouchActive(screen)) return;
    constexpr int x = LEFT_PANEL_X;
    constexpr int y = CONTENT_PANEL_Y;
    constexpr int height = CONTENT_PANEL_HEIGHT;
    constexpr int gutter = 16;
    constexpr int slotGap = 12;
    const int width = fb.getWidth() - x;
    const int gridTop = y + 58;
    const int colW = (width - 3 * gutter) / 2;
    const int colX[2] = {x + gutter, x + gutter + colW + gutter};
    const int slotH = (height - (gridTop - y) - 2 * slotGap - gutter) / 3;
    for (int i = 0; i < 6; ++i) {
        const int col = i >= 3 ? 1 : 0;
        const int row = i >= 3 ? i - 3 : i;
        screen.touchButtons.push_back({kPartyTouchBase + i, colX[col],
                                       gridTop + row * (slotH + slotGap), colW, slotH});
    }
}

bool handleBasePartyTouch(TrainerViewScreen& screen, const TouchInput& touch) {
    if (!basePartyTouchActive(screen)) return false;
    const int downId = screen.touchedButtonDownId(touch);
    if (downId >= kPartyTouchBase && downId < kPartyTouchBase + 6) {
        screen.selectedPartyIndex = downId - kPartyTouchBase;
        return true;
    }
    const int tapId = screen.touchedButtonId(touch);
    if (tapId >= kPartyTouchBase && tapId < kPartyTouchBase + 6) {
        const int slot = tapId - kPartyTouchBase;
        screen.selectedPartyIndex = slot;
        if (slot >= 0 && slot < static_cast<int>(screen.trainer.party.size())) {
            const auto* pokemon = screen.trainer.party[static_cast<std::size_t>(slot)].get();
            if (pokemon && pokemon->speciesID() != 0) {
                screen.openPokemonActionSheet({
                    PokeVault::UIModel::PokemonLocation::Party, 0, slot});
            }
        }
        return true;
    }
    return false;
}

} // namespace

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad) | navTouchButton(touch);
    const u64 held = padGetButtons(&pad);
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);

    // Base navigation still uses the shared modern grid geometry. Normalize the source cursor to
    // the adapter's native capacity before any source action/move layer sees it.
    clampSourceBoxSelection(*this);

    if (classicPackedMoveLayerAvailable(*this) &&
        ClassicPackedMove::handleInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen4SharedEditorSurface::handleInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen3SharedEditorSurface::handleInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen1PokemonEditor::handleReleaseActionInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen2PokemonEditor::handleReleaseActionInput(*this, down, held, stick.x, stick.y, touch)) return;

    if (Gen2PokemonEditor::handleFinalGen2SurfaceInput(*this, down, held, stick.x, stick.y, touch)) return;
    if (Gen2PokemonEditor::handlePickerInput(*this, down, held, stick.x, stick.y, touch)) return;

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        if (Gen1PokemonEditor::handleInputUXCleanup3(*this, down, held, stick.x, stick.y, touch)) return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::handleFoundationPassiveViewInput(*this, down, touchedButtonId(touch));
        return;
    }

    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y, touch)) return;
    if (handleBasePartyTouch(*this, touch)) return;
    updateGSCOverlay(pad, touch);
    clampSourceBoxSelection(*this);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    // Touch input is consumed before draw, so each rendered frame must publish fresh geometry.
    // Clear both screen-owned targets and visible controller-glyph buttons here because the
    // Gen I-IV editor frames do not all pass through drawAppBackdrop().
    touchButtons.clear();
    g_touchGlyphAccum.clear();
    g_touchGlyphHits.clear();

    if (Gen4SharedEditorSurface::draw(*this, fb)) return;
    if (Gen3SharedEditorSurface::draw(*this, fb)) return;

    // Prevent a base-navigation transition from ever presenting a non-existent source slot, even
    // for the single frame in which the shared 30-slot navigation math crosses the native edge.
    clampSourceBoxSelection(*this);

    if (!Gen2PokemonEditor::finalGen2SurfaceOwnsFrame(*this)) {
        drawGSCOverlay(fb);
        publishBasePartyTouchTargets(*this, fb);
        drawGen2ClassicBoxFooter(*this, fb);
    }

    if (Gen2PokemonEditor::drawReleaseActionSurface(*this, fb)) return;
    if (Gen1PokemonEditor::drawReleaseActionSurface(*this, fb)) return;

    if (Gen2PokemonEditor::drawFinalGen2Surface(*this, fb)) {
        Gen2PokemonEditor::drawPickerOverlay(*this, fb);
        if (classicPackedMoveLayerAvailable(*this)) ClassicPackedMove::draw(*this, fb);
        return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) &&
        (Gen1PokemonEditor::foundationPickerActive(*this) || Gen1PokemonEditor::foundationMoveEditorActive(*this))) {
        Gen1PokemonEditor::drawFoundationPicker(*this, fb);
        return;
    }

    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::drawFoundationPassiveView(*this, fb);
        return;
    }

    Gen1PokemonEditor::drawOverlayUX(*this, fb);
    Gen1PokemonEditor::drawFoundationBottomSplit(*this, fb);
    if (classicPackedMoveLayerAvailable(*this)) ClassicPackedMove::draw(*this, fb);
}

} // namespace UI