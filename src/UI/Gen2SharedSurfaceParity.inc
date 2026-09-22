#include "Pokemon/Pokemon2ReadOnly.h"
#include "UI/Gen2StagedAccess.h"
#include "Gen2TrainerSessionOverlay.inc"

namespace UI::Gen2PokemonEditor {
namespace {

static_assert(SharedEditor::gen2ExternalPassiveViewUsesSharedSurface());
static_assert(!SharedEditor::gen2LegacyPathProductionReachable(SharedEditor::Gen2LegacyPath::StageAddPromptWizard));
static_assert(!SharedEditor::gen2LegacyPathProductionReachable(SharedEditor::Gen2LegacyPath::LegacyTwentyEightRowEditor));
static_assert(!SharedEditor::gen2LegacyPathProductionReachable(SharedEditor::Gen2LegacyPath::LegacyPokemonActionsModal));
static_assert(!SharedEditor::gen2LegacyPathProductionReachable(SharedEditor::Gen2LegacyPath::LegacyHandleInput));

const Pokemon::Pokemon2ReadOnly* parityPassiveDetailsTarget(const TrainerViewScreen& screen) noexcept {
    if (!screen.details.active) return nullptr;
    // Generation II runtime Party/Box records are constructed exclusively as Pokemon2ReadOnly by
    // GSCReadOnlyTrainer. Legacy Storage cannot serialize GSC records yet, so Bank is deliberately
    // excluded instead of making an unsafe cast in the no-RTTI Switch build.
    if (screen.details.source != TrainerViewScreen::EditSource::Party &&
        screen.details.source != TrainerViewScreen::EditSource::Box)
        return nullptr;
    auto& mutableScreen = const_cast<TrainerViewScreen&>(screen);
    Pokemon::Pokemon* target = mutableScreen.detailsTargetPokemon();
    if (!target || target->getGameGroup() != Enums::GameVersion::GSC) return nullptr;
    return static_cast<const Pokemon::Pokemon2ReadOnly*>(target);
}

std::string parityPassiveContext(const TrainerViewScreen& screen) {
    std::string context;
    if (screen.details.source == TrainerViewScreen::EditSource::Party) {
        context = "Party Slot " + std::to_string(screen.details.partyIndex + 1);
    } else if (screen.selectedMode == TrainerViewScreen::ViewMode::Storage) {
        context = "Storage Save Box " + std::to_string(screen.selectedBoxIndex + 1) +
                  " / Slot " + std::to_string(screen.selectedItemIndex + 1);
    } else {
        context = "Box " + std::to_string(screen.selectedBoxIndex + 1) +
                  " / Slot " + std::to_string(screen.selectedItemIndex + 1);
    }
    context += "  •  ";
    context += sharedGameName(screen);
    context += "  •  Source save immutable";
    return context;
}

void drawParityExternalPassiveView(TrainerViewScreen& screen, PKSEFramebuffer& fb,
                                   const Pokemon::Pokemon2ReadOnly& pokemon) {
    State visual{};
    visual.begin(pokemon.strictRecord(), Mode::View);
    refreshDerived(visual.working); // presentation only: native DVs determine gender/shiny
    auto& unified = unifiedStateFor(screen);
    unified.crystal = screen.sourceGameId == "crystal_gbc";
    const auto g = SharedEditor::geometry720p();

    screen.touchButtons.clear();
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, g.x, g.y, g.width, g.height, false, 18);
    fb.drawText(g.x + 24, g.y + 14, SharedEditor::titleFor(SharedEditor::Surface::View),
                Colors::Text, TextStyle::Heading);
    fb.drawText(g.x + 24, g.y + 46, parityPassiveContext(screen), Colors::TextDim, TextStyle::Caption);

    const int contentY = g.y + 74;
    const int contentH = g.height - 94;
    const int leftX = g.x + 18, leftW = 300;
    const int midX = leftX + leftW + 14, midW = 398;
    const int rightX = midX + midW + 14, rightW = g.x + g.width - 18 - rightX;

    drawUnifiedDetails(fb, visual, unified, leftX, contentY, leftW, contentH, true);
    drawUnifiedValues(fb, visual, unified, midX, contentY, midW, contentH, true);
    drawUnifiedMoves(fb, screen, visual, unified, rightX, contentY, rightW, contentH, true);

    screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                   16, fb.getHeight() - 68, 190, 58});
    drawNavBar(fb, {{"D-pad", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}, {"Touch", "Back"}});
}

bool handleParityExternalPassiveInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                      int stickX, int stickY, const TouchInput& touch) {
    if (!parityPassiveDetailsTarget(screen)) return false;
    if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, screen.touchedButtonId(touch)) ==
        PokeBank::UIModel::PassiveViewAction::Back) {
        screen.closeDetailsModal();
        resetUnifiedFocus(screen);
        return true;
    }
    auto& unified = unifiedStateFor(screen);
    unified.crystal = screen.sourceGameId == "crystal_gbc";
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (down & HidNpadButton_L)
        unified.focus = Unified::passiveViewSwitchPanel(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (down & HidNpadButton_R)
        unified.focus = Unified::passiveViewSwitchPanel(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Up)
        unified.focus = Unified::passiveViewMoveVertical(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Down)
        unified.focus = Unified::passiveViewMoveVertical(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    if (nav & HidNpadButton_Left)
        unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, unified.focus, -1, unified.crystal);
    if (nav & HidNpadButton_Right)
        unified.focus = Unified::passiveViewMoveColumn(Unified::Generation::Gen2, unified.focus, 1, unified.crystal);
    unified.detailsFocus = Unified::detailsScrollFocus(unified.focus, unified.detailsFocus);
    // A/Y/X are intentionally ignored: passive View remains strictly read-only.
    return true;
}

void drawParityGen2Actions(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                               static_cast<std::size_t>(legacy.pokemonSlot), error);
    const auto actions = SharedEditor::actionsForSlot(static_cast<bool>(pokemon), gen2ActionCapabilities());
    legacy.pokemonRow = std::clamp(legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));

    // Match the accepted Gen I action-sheet geometry exactly.
    const int w = pokemon ? 650 : 560;
    const int h = pokemon ? 500 : 350;
    const int x = (1280 - w) / 2;
    const int y = (672 - h) / 2;
    const auto geometry = SharedEditor::actionMenuGeometry();
    // Action sheets are overlays on the live box browser, never standalone black screens.
    // Gen I/Yellow is the canonical presentation for every classic generation.
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::FocusBorder, 2);
    fb.drawText(x + 24, y + 18, pokemon ? "Pokemon Actions" : "Empty Slot", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(legacy.pokemonBox + 1) + " / Slot " +
        std::to_string(legacy.pokemonSlot + 1) + "  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);

    int rowY = y + geometry.rowStart;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == legacy.pokemonRow;
        if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, geometry.rowHeight);
        fb.drawText(x + 38, rowY + 10, SharedEditor::actionLabel(actions[i]),
                    selected ? Colors::Text : Colors::TextDim);
        rowY += geometry.rowStep;
    }
    fb.drawText(x + 24, y + h - 38, "Original source is immutable", Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Back"}});
}

void drawParityGen2Review(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& legacy = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const auto& changes = editor->pendingChanges();
    constexpr int w = 980, h = 540;
    constexpr int x = (1280 - w) / 2;
    constexpr int y = (672 - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Review Pending Changes", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        std::to_string(changes.size()) + " staged change(s)  •  original source is immutable",
        Colors::TextDim, TextStyle::Caption);
    if (changes.empty()) {
        fb.drawText(x + 30, y + 100, "No pending changes", Colors::TextDim, TextStyle::Body);
    } else {
        const int first = std::clamp(legacy.reviewRow - 4, 0, std::max(0, static_cast<int>(changes.size()) - 9));
        const int last = std::min(static_cast<int>(changes.size()), first + 9);
        for (int i = first; i < last; ++i) {
            const int rowY = y + 88 + (i - first) * 45;
            const bool selected = i == legacy.reviewRow;
            if (selected) fb.drawSelectionHighlight(x + 22, rowY, w - 44, 42);
            const auto& change = changes[static_cast<std::size_t>(i)];
            fb.drawText(x + 38, rowY + 10, change.label, selected ? Colors::Text : Colors::TextDim);
            fb.drawText(x + 350, rowY + 10, change.beforeValue + "  ->  " + change.afterValue,
                        Colors::Text, TextStyle::Caption);
        }
    }
    if (!legacy.lastExportDirectory.empty())
        fb.drawText(x + 24, y + h - 44, "Last verified export: " + legacy.lastExportDirectory,
                    Colors::Success, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Export Copy"},
                    {"X", "Discard Staged"}, {"B", "Back"}});
}

void drawParityGen2Clone(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& shared = sharedSurfaceStateFor(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) return;
    const int destination = firstEmptyGen2Slot(*editor, shared.cloneBox);
    constexpr int w = 930, h = 540;
    constexpr int x = (1280 - w) / 2;
    constexpr int y = (672 - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Clone Pokemon — Destination", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
        "Box " + std::to_string(shared.cloneBox + 1) + "  •  first writable slot  •  staged copy only",
        Colors::TextDim, TextStyle::Caption);

    constexpr int cols = 5;
    constexpr int cellW = 164, cellH = 72;
    const int startX = x + 48, startY = y + 96;
    for (int slot = 0; slot < static_cast<int>(editor->metadata().boxCapacity); ++slot) {
        const int col = slot % cols, row = slot / cols;
        const int xx = startX + col * cellW, yy = startY + row * cellH;
        std::string error;
        const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(shared.cloneBox),
                                                   static_cast<std::size_t>(slot), error);
        const bool target = slot == destination;
        if (target) fb.drawSelectionHighlight(xx, yy, cellW - 10, cellH - 10);
        fb.drawRoundedRect(xx, yy, cellW - 10, cellH - 10, 8, target ? Colors::Accent : Colors::Divider, 1);
        fb.drawText(xx + 10, yy + 8, "Slot " + std::to_string(slot + 1), Colors::TextDim, TextStyle::Caption);
        fb.drawText(xx + 10, yy + 34,
            pokemon ? std::string(Names::getSpeciesName(pokemon->species)) : (target ? "Clone here" : "Empty"),
            target ? Colors::Accent : Colors::Text, TextStyle::Caption);
    }
    if (destination < 0)
        fb.drawText(x + 24, y + h - 44, "This box is full — choose another box with L/R", Colors::Error, TextStyle::Caption);
    drawNavBar(fb, {{"L/R", "Box"}, {"A", "Stage Clone"}, {"B", "Cancel"}});
}

} // namespace

bool drawFinalGen2Surface(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGSC(screen)) return false;
    if (const auto* passive = parityPassiveDetailsTarget(screen)) {
        drawParityExternalPassiveView(screen, fb, *passive);
        return true;
    }
    auto& legacy = editorOverlayState(screen);
    const auto& shared = sharedSurfaceStateFor(screen);
    if (stateFor(screen).mode != Mode::None) {
        drawUnifiedWorkspace(screen, fb);
        return true;
    }
    if (shared.provenanceActive) { drawSharedProvenanceSurface(screen, fb); return true; }
    if (shared.cloneActive) { drawParityGen2Clone(screen, fb); return true; }
    if (legacy.active && legacy.review) { drawParityGen2Review(screen, fb); return true; }
    if (legacy.active && legacy.pokemonActions) { drawParityGen2Actions(screen, fb); return true; }
    return false;
}

bool finalGen2SurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    return parityPassiveDetailsTarget(screen) != nullptr || unifiedGen2SurfaceOwnsFrame(screen);
}

bool handleFinalGen2SurfaceInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                                 int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    if (handleTrainerSession(screen, down, held, stickX, stickY, touch)) return true;
    if (handleParityExternalPassiveInput(screen, down, held, stickX, stickY, touch)) return true;
    if (stateFor(screen).mode != Mode::None) {
        if (!handleUnifiedGen2SurfaceInput(screen, down, held, stickX, stickY, touch))
            handlePickerInput(screen, down, held, stickX, stickY, touch);
        return true; // Editor Species activation must never fall through to Box Empty Slot.
    }
    if (handleUnifiedGen2SurfaceInput(screen, down, held, stickX, stickY, touch)) return true;

    // The named Species/Move/Pokerus picker is handled immediately after this
    // final-surface adapter in TrainerViewScreen::update. While it owns input,
    // do not let the underlying Box A activation see the same A press and open
    // Empty Slot / Pokemon Actions behind the editor.
    if (pickerStateFor(screen).model.active()) return false;

    namespace Access = PokeBank::UIModel::Gen2StagedAccess;
    auto* editor = stagedEditor(screen);
    const auto caps = Access::capabilities(
        screen.trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(screen), editor != nullptr);
    const bool storage = screen.selectedMode == TrainerViewScreen::ViewMode::Storage;
    const auto surface = screen.selectedMode == TrainerViewScreen::ViewMode::Boxes ? Access::Surface::Boxes
        : storage && screen.storageFocusPane == 0 ? Access::Surface::StorageSave
        : screen.selectedMode == TrainerViewScreen::ViewMode::Trainer ? Access::Surface::Trainer : Access::Surface::Other;
    const int box = storage ? screen.stSaveBox : screen.selectedBoxIndex;
    const int slot = storage ? screen.stSaveSlot : screen.selectedItemIndex;
    const bool validSlot = editor && box >= 0 && slot >= 0 &&
        box < static_cast<int>(editor->metadata().boxCount) &&
        slot < static_cast<int>(editor->metadata().boxCapacity);
    const bool blocked = editorOverlayState(screen).active || screen.helpOverlayActive ||
        screen.details.active || screen.actionSheet.isOpen() || screen.saveConfirmActive ||
        screen.pickerActive || screen.itemEditDialogActive || screen.carrying() || screen.currentlySelecting;
    if (caps.canEditStagedCopy && surface == Access::Surface::Trainer && screen.detailViewActive && !blocked &&
        (down & HidNpadButton_Y)) {
        auto& legacy = editorOverlayState(screen);
        legacy.active = legacy.review = true;
        legacy.pokemonActions = false;
        legacy.reviewRow = 0;
        return true;
    }
    // Gen II Boxes now use the same primary interaction language as accepted Gen I:
    // A opens Empty Slot/Pokemon Actions; X is a direct Add shortcut only on an empty cell.
    if (caps.canEditStagedCopy && surface == Access::Surface::Boxes && screen.detailViewActive && !blocked && validSlot) {
        std::string error;
        const bool occupied = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error).has_value();
        const auto activation = SharedEditor::boxActivation(occupied,
            (down & HidNpadButton_A) != 0, (down & HidNpadButton_X) != 0);
        if (activation == SharedEditor::BoxActivation::Add) {
            return beginCreate(screen, box);
        }
        if (activation == SharedEditor::BoxActivation::Actions) {
            auto& legacy = editorOverlayState(screen);
            legacy.active = true;
            legacy.review = false;
            legacy.pokemonActions = true;
            legacy.pokemonFields = false;
            legacy.pokemonBox = box;
            legacy.pokemonSlot = slot;
            legacy.pokemonRow = 0;
            legacy.reviewRow = 0;
            screen.controllerNavigation.reset();
            return true;
        }
        if (down & HidNpadButton_X) return true; // occupied X is intentionally not a hidden edit/add action.
    }

    // Save-side Storage retains the same contextual action menu; it has no direct-X shortcut contract.
    if (Access::entry(caps, surface, screen.detailViewActive, blocked, validSlot,
                      down & HidNpadButton_A) == Access::Entry::PokemonActions) {
        auto& legacy = editorOverlayState(screen);
        legacy.active = true;
        legacy.review = false;
        legacy.pokemonActions = true;
        legacy.pokemonFields = false;
        legacy.pokemonBox = box;
        legacy.pokemonSlot = slot;
        legacy.pokemonRow = 0;
        legacy.reviewRow = 0;
        screen.controllerNavigation.reset();
        return true; // consume before the legacy source-writable guard
    }
    return false;
}

} // namespace UI::Gen2PokemonEditor
