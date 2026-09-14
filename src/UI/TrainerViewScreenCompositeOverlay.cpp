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

// Cleanup #2 remains compiled under explicit recovery symbols.
[[nodiscard]] bool isGen1SourceUXCleanup2(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXCleanup2(TrainerViewScreen& screen, uint64_t down);
void drawOverlayUXCleanup2(TrainerViewScreen& screen, PKSEFramebuffer& fb);

// Cleanup #3 remains compiled intact as the accepted shiny/move/radar/staging recovery layer.
// The final foundation delegates every non-workspace surface back to these symbols.
[[nodiscard]] bool isGen1SourceUXCleanup3(const TrainerViewScreen& screen) noexcept;
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down);
[[nodiscard]] bool handleInputUXCleanup3(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                         int stickX, int stickY);
void drawOverlayUXCleanup3(TrainerViewScreen& screen, PKSEFramebuffer& fb);

// Final live entry points.
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

// Hardware-tested Cleanup #3 stays intact under explicit recovery symbols. Its accepted
// shiny mechanics, compatibility enforcement, species/move pickers, clone/review screens,
// staged-write helpers, and radar are reused by the final foundation rather than rewritten.
#define isGen1SourceUX isGen1SourceUXCleanup3
#define handleInputUX handleInputUXCleanup3
#define drawOverlayUX drawOverlayUXCleanup3
#include "Gen1PokemonEditorOverlayUXCleanup3.inc"
#undef drawOverlayUX
#undef handleInputUX
#undef isGen1SourceUX

// Final permanent editor shell: all three PKSE-style panels are interactive and capability-driven.
#include "Gen1PokemonEditorOverlayFoundation.inc"

// Physical-test correction layer. This stays small and additive: picker ownership is restored
// above the foundation workspace, and the lower-right display becomes two capability-aware panes.
#include "Gen1PokemonEditorFoundationHardwareFix.inc"

// The occupied-slot action-sheet View uses the same passive Gen I presenter as Party/Storage View.
// It owns all input while active, has no field cursor, and exposes only B Back.
#include "Gen1PokemonEditorPassiveView.inc"

namespace UI {

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    const u64 held = padGetButtons(&pad);
    const HidAnalogStickState stick = padGetStickPos(&pad, 0);

    // A picker is modal even though SpeciesPicker intentionally leaves the underlying mode as
    // AddDraft/Edit. Route it directly to the accepted Cleanup #3 picker handler so A on Species
    // cannot create an invisible picker that later appears over Level/EXP.
    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        if (Gen1PokemonEditor::handleInputUXCleanup3(*this, down, held, stick.x, stick.y)) return;
    }

    // View from the occupied-Pokemon action sheet is a passive read-only surface, just like the
    // Party and Storage detail routes. Consume all input here so no editor focus/cursor can appear.
    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::handleFoundationPassiveViewInput(*this, down);
        return;
    }

    if (Gen1PokemonEditor::handleInputUX(*this, down, held, stick.x, stick.y)) return;
    updateGSCOverlay(pad, touch);
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawGSCOverlay(fb);

    // Match the input ownership rule above: an active picker must be the visible top surface.
    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPickerActive(*this)) {
        Gen1PokemonEditor::drawOverlayUXCleanup3(*this, fb);
        return;
    }

    // Match Party/Storage View Pokemon exactly: shared passive presenter, no field cursor,
    // no editable focus, and no second foundation bottom pane drawn over it.
    if (Gen1PokemonEditor::isGen1SourceUX(*this) && Gen1PokemonEditor::foundationPassiveViewActive(*this)) {
        Gen1PokemonEditor::drawFoundationPassiveView(*this, fb);
        return;
    }

    Gen1PokemonEditor::drawOverlayUX(*this, fb);
    Gen1PokemonEditor::drawFoundationBottomSplit(*this, fb);
}

} // namespace UI
