#include "UI/ClassicPokemonActionMenu.h"
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

    drawUnifiedDetails(screen, fb, visual, unified, leftX, contentY, leftW, contentH, true);
    drawUnifiedValues(fb, visual, unified, midX, contentY, midW, contentH, true);
    drawUnifiedMoves(fb, screen, visual, unified, rightX, contentY, rightW, contentH, true);

    screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                   16, fb.getHeight() - 68, 190, 58});
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
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
    const auto actions = SharedEditor::actionsForSlot(
        static_cast<bool>(pokemon), gen2ActionCapabilities(screen),
        !editor->pendingChanges().empty());
    legacy.pokemonRow = std::clamp(
        legacy.pokemonRow, 0, std::max(0, static_cast<int>(actions.count) - 1));

    // Keep the live box as background; the narrow modal owns every foreground
    // hitbox. All active rows retain the exact Gen II staged command indices.
    screen.drawLegacyBase(fb);
    const auto model = PokeBank::UIModel::ClassicPokemonActionMenu::fromShared(actions);
    const std::string title = pokemon
        ? std::string(Names::getSpeciesName(pokemon->species)) : "Empty Slot";
    drawNarrowPokemonActions(fb, screen.touchButtons, model, legacy.pokemonRow,
                            title,
                            screen.legacyProviderLabel() + " GBC SOURCE / READ ONLY", 31600);
    UI::SharedEditorChrome::drawPickerFooter(fb, {true, false}); // Browse, A Select, B Cancel
}

void drawParityGen2Review(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    drawSharedReviewSurface(screen,fb);
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
    if (legacy.active && legacy.review) { drawParityGen2Review(screen, fb); return true; }
    if (legacy.active && legacy.pokemonActions) { drawParityGen2Actions(screen, fb); return true; }
    return false;
}

bool finalGen2SurfaceOwnsFrame(const TrainerViewScreen& screen) noexcept {
    if (parityPassiveDetailsTarget(screen) != nullptr || unifiedGen2SurfaceOwnsFrame(screen))
        return true;

    // Pokemon Actions / Empty Slot must bypass drawGSCOverlay() at the composite
    // wrapper level as well. Otherwise that wrapper paints the legacy full Accent
    // staged-editor frame before drawParityGen2Actions() gets a chance to replace it.
    const auto& legacy = editorOverlayState(screen);
    return legacy.active && legacy.pokemonActions && !legacy.review;
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
    // A box-cell tap selects that exact slot and activates the same staged
    // Actions surface as A. Do not let the inherited two-tap box handler open
    // its generic, mostly disabled source-save sheet.
    const int tappedSlot = touch.justTapped() ? screen.touchedButtonId(touch) : -1;
    const bool boxTap = surface == Access::Surface::Boxes && screen.detailViewActive &&
        tappedSlot >= 0 && tappedSlot < static_cast<int>(screen.trainer.getSlotsPerBox());
    if (boxTap) screen.selectedItemIndex = tappedSlot;
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
    // Gen II Boxes use the same primary interaction language as every Gen I-V box grid:
    // A opens Empty Slot/Pokemon Actions; X exports an occupied slot / imports into an empty one
    // (Round 11 X-IMPEXP; Add Pokemon stays on the A sheet).
    if (caps.canEditStagedCopy && surface == Access::Surface::Boxes && screen.detailViewActive && !blocked && validSlot) {
        std::string error;
        const bool occupied = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error).has_value();
        const auto activation = SharedEditor::boxActivation(occupied,
            (down & HidNpadButton_A) != 0 || boxTap, (down & HidNpadButton_X) != 0,
            SharedEditor::fileCapabilities(true));
        if (activation == SharedEditor::BoxActivation::ImportFile) {
            screen.controllerNavigation.reset();
            PokemonFileMenus::beginImport(screen, 2, box, slot);
            return true;
        }
        if (activation == SharedEditor::BoxActivation::ExportFile) {
            screen.controllerNavigation.reset();
            PokemonFileMenus::exportPokemon(screen, 2, false, box, slot);
            return true;
        }
        if (activation == SharedEditor::BoxActivation::Actions) {
            auto& legacy = editorOverlayState(screen);
            legacy.active = true;
            legacy.review = false;
            legacy.pokemonActions = true;
            legacy.pokemonParty = false;
            legacy.pokemonFields = false;
            legacy.pokemonBox = box;
            legacy.pokemonSlot = slot;
            legacy.pokemonRow = 0;
            legacy.reviewRow = 0;
            screen.controllerNavigation.reset();
            return true;
        }
        if (down & HidNpadButton_X) return true; // X is only Import / Export here, never a hidden edit / add.
    }

    // Save-side Storage retains the same contextual action menu; it has no direct-X shortcut contract.
    if (Access::entry(caps, surface, screen.detailViewActive, blocked, validSlot,
                      down & HidNpadButton_A) == Access::Entry::PokemonActions) {
        auto& legacy = editorOverlayState(screen);
        legacy.active = true;
        legacy.review = false;
        legacy.pokemonActions = true;
        legacy.pokemonParty = false;
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
