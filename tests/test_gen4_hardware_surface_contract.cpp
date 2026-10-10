#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen4BoxTouchActions.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos)
        std::cerr << "Missing Gen IV surface contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
}

int main() {
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    static_assert(Shared::genderUsesInlineToggle(Shared::Generation::Gen4));
    static_assert(!Shared::genderOpensDedicatedPicker(Shared::Generation::Gen4));

    // First tap on a different slot still selects it via the legacy box cursor.
    // Only a tap on the focused slot or its summary enters Gen IV actions.
    namespace BoxTouch = PokeBank::UIModel::Gen4BoxTouchActions;
    static_assert(BoxTouch::opensSelectedSlotActions(0, 0, 30));
    static_assert(BoxTouch::opensSelectedSlotActions(29, 29, 30));
    static_assert(BoxTouch::opensSelectedSlotActions(BoxTouch::SummaryPanelTouchId, 12, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(3, 2, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(1000, 2, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(1001, 2, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(1002, 2, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(BoxTouch::SummaryPanelTouchId, -1, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(BoxTouch::SummaryPanelTouchId, 30, 30));
    static_assert(!BoxTouch::opensSelectedSlotActions(0, 0, 0));

    // Immutable external sources still need the staged dirty-session exit confirmation.
    static_assert(Shared::immutableSourceBlocksSaveDialog(true, false));
    static_assert(!Shared::immutableSourceBlocksSaveDialog(true, true));
    static_assert(!Shared::immutableSourceBlocksSaveDialog(false, false));

    // Gen IV has six native IV/EV rows. The sixth row must stay a column-selectable
    // IV/EV cell, not collapse into the older-generation full-width fallback row.
    constexpr auto gen4SixthIv = Shared::cellFocusFor(
        Shared::Generation::Gen4, {Shared::Panel::Values, 5, 1});
    static_assert(gen4SixthIv.x == 180 && gen4SixthIv.width == 90);
    constexpr auto gen2SixthFallback = Shared::cellFocusFor(
        Shared::Generation::Gen2, {Shared::Panel::Values, 5, 1});
    static_assert(gen2SixthFallback.x == 130 && gen2SixthFallback.width == 244);

    // Gen IV has six editable IV/EV rows; Sp. Def must retain distinct IV vs EV focus rectangles.
    constexpr Shared::Focus spDefIv{Shared::Panel::Values, 5, 0};
    constexpr Shared::Focus spDefEv{Shared::Panel::Values, 5, 1};
    constexpr auto spDefIvFocus = Shared::cellFocusFor(Shared::Generation::Gen4, spDefIv);
    constexpr auto spDefEvFocus = Shared::cellFocusFor(Shared::Generation::Gen4, spDefEv);
    static_assert(spDefIvFocus.x == 110 && spDefIvFocus.width == 64);
    static_assert(spDefEvFocus.x == 180 && spDefEvFocus.width == 90);
    static_assert(spDefIvFocus.x != spDefEvFocus.x || spDefIvFocus.width != spDefEvFocus.width);

    // Simulate the two-frame immutable-source dirty-exit path that the first source-only contract
    // missed: B opens the exit-only prompt, a neutral frame must not clear it, then A may discard.
    bool saveConfirmActive = true;
    bool exitingWithUnsavedChanges = true;
    if (Shared::immutableSourceBlocksSaveDialog(saveConfirmActive, exitingWithUnsavedChanges))
        saveConfirmActive = false;
    assert(saveConfirmActive); // neutral frame preserved the exit-only confirmation
    bool goBack = false;
    if (saveConfirmActive && exitingWithUnsavedChanges) {
        saveConfirmActive = false;
        exitingWithUnsavedChanges = false;
        goBack = true;
    }
    assert(goBack && !saveConfirmActive && !exitingWithUnsavedChanges);

    const auto surface = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto moveCompatibility = read("include/Integration/Gen4/Gen4MoveCompatibility.h");
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto bridge = read("src/Legacy/Gen4ReadOnlyTrainer.cpp");
    const auto staged = read("src/Integration/Gen4/Gen4StagedPokemonEditor.cpp");
    const auto session = read("include/UI/Gen4SharedPokemonSession.h");
    const auto baseUi = read("src/UI/TrainerViewScreenBase.inc");
    const auto locations = read("src/Names/LocationNames.cpp");
    const auto locationGenerator = read("tools/gen_locations.py");
    const auto itemsPanel = read("src/UI/Panels/ItemsPanel.cpp");
    const auto boxPanel = read("src/UI/Panels/BoxPokemonPanel.cpp");
    const auto gen3Picker = read("src/UI/Gen3SharedPokemonSurface.inc");
    const auto gen2Held = read("include/UI/Gen2HeldItemPicker.h");
    const auto gen2Editor = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto sharedHeld = read("include/UI/SharedHeldItemPickerPresentation.h");
    const auto gen1Move = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    const auto gen2Move = read("src/UI/Gen2HardwarePickerFix.inc");
    const auto movePickerModel = read("include/UI/MovePickerPresentation.h");
    const auto itemArtwork = read("include/UI/ItemPickerArtwork.h");
    const auto spriteRuntime = read("src/UI/SpriteManager.cpp");
    const auto spriteGenerator = read("tools/gen_item_sprites.py");
    const auto recover = read("tools/recover_workspace.py");
    const auto assetGate = read("tools/check_device_assets.py");
    const auto trainerBase = read("include/Trainer/Trainer.h");
    const auto rbyBridge = read("include/Legacy/RBYReadOnlyTrainer.h");
    const auto gscBridge = read("include/Legacy/GSCReadOnlyTrainer.h");
    const auto gen3Bridge = read("include/Legacy/FRLGReadOnlyTrainer.h");
    const auto gen4Bridge = read("include/Legacy/Gen4ReadOnlyTrainer.h");

    contains(surface, "SharedPokemonShell::drawChrome");
    contains(surface, "SharedPokemonShell::Geometry");
    contains(surface, "SharedPokemonShell::drawScrollableDetails");
    contains(surface, "SharedPokemonShell::drawDataAndGraph");
    contains(surface, "Shared::normalizeMoveRowFocus(Shared::Generation::Gen4");
    contains(surface, "Shared::moveRowColumn(Shared::Generation::Gen4");
    contains(surface, "Shared::passiveViewMoveColumn(Shared::Generation::Gen4");
    contains(surface, "Shared::moveRowFocus(g.rightW)");

    // Gen IV inherits the accepted D-pad / left-stick repeat navigation path.
    contains(surface, "screen.controllerNavigation.apply(");
    contains(surface, "down, held, stickX, stickY");

    // Gender is a one-press inline toggle, never a nested Male/Female picker.
    contains(surface, "state.session.cycleGender()");
    assert(surface.find("PickerTarget::Gender") == std::string::npos);
    assert(surface.find("Choose Gender") == std::string::npos);

    // Nature and Ability use bounded pickers; Shiny is another direct field action.
    contains(surface, "PickerTarget::Nature");
    contains(surface, "PickerTarget::Ability");
    contains(surface, "state.session.cycleShiny()");
    contains(surface, "Could not reconcile Species/Shiny while preserving PID-linked traits");

    // Outer move focus is exactly one row. PP / PP Ups live in the contextual dialog.
    contains(surface, "openMoveEditor(screen, state, state.focus.row)");
    contains(surface, "A: choose compatible Gen IV move");
    contains(surface, "OK = native   Transfer/Preserved = warning");
    contains(surface, "PP Ups");
    contains(surface, "state.moveEditorRow = 0; // Shared move editor opens on the Move row");
    contains(surface, "state.moveEditorRow == 1");
    contains(surface, "state.moveEditorRow == 2");
    contains(surface, "state.session.working->setMove(slot, value)");
    contains(surface, "uint16_t moveBaseline = 0;");
    contains(surface, "state.moveBaseline = state.session.working->moves()");
    contains(surface, "state.session.working->setMove(slot, state.moveBaseline)");
    contains(surface, "PickerTarget::Move");
    contains(surface, "#include \"Integration/Gen4/Gen4MoveCompatibility.h\"");
    contains(surface, "PokeVault::Integration::Gen4MoveCompatibility::selectableMoves(");
    contains(surface, "screen.sourceGameId, state.session.working->species()");
    contains(surface, "state.session.working->form(), state.session.working->moves()");
    // Gen IV native Inventory removal is APP-MEMORY staged only and always
    // uses an explicit Y confirmation; A must never remove an item.
    contains(surface, "state.itemRemoveConfirmActive");
    contains(surface, "stageBagRemove(");
    contains(surface, "Gen IV item removed from staged workspace ONLY");
    contains(surface, "state.itemAddPickerActive");
    contains(surface, "Gen4::gen4BagChoices(staged->layout()");
    contains(surface, "staged->stageBagAdd(");
    contains(surface, "ItemPickerArtwork::draw(fb,x+w-31,yy+3,32,name)");
    contains(surface, "Gen IV item added to staged workspace ONLY (x1)");
    contains(surface, "ItemPickerArtwork::draw(fb,x+w-38,y+65,36,itemName)");
    const auto removeGuardAt=surface.find("if(state.itemRemoveConfirmActive) {");
    const auto quantityEntryAt=surface.find("if(!state.itemQuantityActive) {",removeGuardAt);
    assert(removeGuardAt!=std::string::npos && quantityEntryAt>removeGuardAt);
    const auto confirmControls=surface.substr(removeGuardAt,quantityEntryAt-removeGuardAt);
    assert(confirmControls.find("down&HidNpadButton_B")!=std::string::npos);
    assert(confirmControls.find("down&HidNpadButton_Y")!=std::string::npos);
    assert(confirmControls.find("down&HidNpadButton_A")==std::string::npos);
    contains(surface, "state.itemRemoveConfirmActive=false;");
    const auto stagedBag=read("src/Integration/Gen4/Gen4StagedPokemonEditor.cpp");
    contains(stagedBag, "bool Gen4StagedPokemonEditor::stageBagRemove(");
    contains(stagedBag, "bool Gen4StagedPokemonEditor::stageBagAdd(");
    contains(stagedBag, "gen4BagItemAllowed(layout_,pocket,itemId)");
    contains(stagedBag, "Gen IV Add changed unrelated original save bytes");
    contains(stagedBag, "Gen IV item removal modified unrelated save bytes");
    contains(stagedBag, "Gen IV item removal changed another pocket");
    contains(surface, "MoveUI::rowLabel(value, bridge(screen).sourceSave().rawFamily())");
    contains(surface, "Empty + compatible moves only • exact Gen IV Acc / Pwr / PP");
    contains(moveCompatibility, "inline constexpr uint16_t MaxMove = 467;");
    contains(moveCompatibility, "result.push_back(0);");
    contains(moveCompatibility, "const auto availability = classify(exactGameId, species, form, move, false);");
    contains(moveCompatibility, "availability == Availability::Direct || availability == Availability::Transfer");
    contains(moveCompatibility, "if (existingSourceMove) return Availability::Preserved;");
    contains(surface, "MoveResult::Compatible");
    contains(surface, "status = \"OK\"");
    contains(surface, "statusColor = Colors::Success");
    contains(surface, "MoveResult::PreserveExisting");
    contains(surface, "status = \"Preserved\"");
    contains(surface, "MoveResult::Unsupported");
    contains(surface, "status = \"Transfer\"");
    contains(surface, "Gen IV move compatibility • event/encounter legality is not fully checked");
    assert(surface.find("Names::isMovePresent") == std::string::npos);
    assert(surface.find("Native Gen IV move catalog") == std::string::npos);
    assert(surface.find("move choice read-only in G4-03") == std::string::npos);

    // G4-04 keeps accepted View/Edit and adds native Create for empty PC slots.
    // The current action builder also preserves Gen I-III presentation parity while
    // keeping intentionally locked Party actions visible but disabled.
    contains(surface, "MenuActionSet gen4Actions(");
    contains(surface, "add(MenuAction::View, \"View\", true);");
    contains(surface, "add(MenuAction::Edit, \"Edit\", stagedAvailable);");
    contains(surface, "add(MenuAction::Add, \"Add Pokemon\", stagedAvailable);");
    contains(surface, "case MenuAction::Add:");
    contains(surface, "beginCreate(screen)");
    contains(surface, "Gen4BoxTouchActions::opensSelectedSlotActions(");
    contains(surface, "screen.touchedButtonId(touch)");
    contains(surface, "bridge(screen).stagedPokemonUnavailableReason()");
    contains(baseUi, "const bool gen4Staged = group == Enums::GameVersion::DP");
    contains(baseUi, "A: Actions  |  X: Add  |  B: Back");
    contains(boxPanel, "? \"A / tap: Actions\" : \"A / tap: Edit\"");
    contains(surface, "add(MenuAction::Clone, \"Clone\", stagedAvailable);");
    contains(surface, "add(MenuAction::Release, \"Release\", stagedAvailable);");
    contains(surface, "add(MenuAction::AddMasterVault, \"Add to Master Vault\", false);");
    contains(surface, "add(MenuAction::AddBank, \"Add to Bank...\", false);");
    contains(surface, "add(MenuAction::TransferGame, \"Transfer to Game...\", false);");
    contains(surface, "add(MenuAction::Clone, \"Clone\", false);");
    contains(surface, "add(MenuAction::MakeShiny, \"Make Shiny\", false);");
    contains(surface, "PickerTarget::Species");
    assert(surface.find("createSpeciesInitialized") == std::string::npos);
    contains(surface, "state.session.working->setSpecies(value)");
    contains(surface, "state.speciesPreviewShiny = !state.speciesPreviewShiny");
    contains(surface, "state.session.working->setShiny(state.speciesPreviewShiny)");
    contains(surface, "\"Y\", \"Normal/Shiny\"");
    contains(surface, "return \"#\" + number + \"  \" + Names::getSpeciesName(value);");
    contains(surface, "PickerTarget::HeldItem");
    contains(surface, "PickerTarget::Language");
    contains(surface, "PickerTarget::Ball");
    contains(surface, "PickerTarget::MetLocation");
    contains(surface, "PickerTarget::Pokerus");
    contains(surface, "setPokerusMode");
    contains(surface, "PickerTarget::Form");
    contains(surface, "openFormPicker");
    contains(surface, "auto probe = *state.session.working");
    contains(surface, "probe.setForm");
    contains(surface, "gen4FormLabel");
    contains(surface, "state.session.working->setForm");
    contains(surface, "Names::isGen4HeldItemPresent");
    contains(surface, "Names::getLocationTable");
    contains(surface, "Enums::getBallList");
    contains(surface, "createBoxDraft(");
    contains(surface, "keepCreate(");
    contains(session, "Mode::Create");
    contains(session, "Guard::Create");
    contains(staged, "stageCreateBoxPokemon");
    assert(surface.find("Generation IV Create follows after Edit hardware acceptance") == std::string::npos);
    assert(surface.find("Create is not enabled in the Edit milestone") == std::string::npos);
    contains(surface, "\"Clone\"");
    contains(surface, "\"Release\"");
    contains(surface, "\"Legality & Provenance\"");
    contains(surface, "\"Add to Master Vault\"");
    contains(surface, "\"Add to Bank...\"");
    contains(surface, "\"Transfer to Game...\"");
    contains(surface, "\"Make Shiny\"");
    contains(staged, "stageCloneBoxPokemon");
    contains(staged, "stageReleaseBoxPokemon");
    contains(surface, "partyEntrySurface");
    contains(surface, "TargetKind::Party");
    contains(surface, "keepParty");
    contains(surface, "beginPassiveView");
    contains(surface, "screen.closeDetailsModal()");

    // Create opens the editor itself, like Gen I-III; Species is an explicit field action.
    const auto beginCreateAt = surface.find("bool beginCreate(TrainerViewScreen& screen)");
    const auto heldItemAt = surface.find("void openHeldItemPicker", beginCreateAt);
    assert(beginCreateAt != std::string::npos && heldItemAt > beginCreateAt);
    const auto beginCreateBody = surface.substr(beginCreateAt, heldItemAt - beginCreateAt);
    assert(beginCreateBody.find("openSpeciesPicker(screen, state)") == std::string::npos);

    // Create/Edit focus must only land on actual controls. OT/TID are true editable Pokémon
    // metadata, SID/Origin remain inspectable read-only, and fixed-only rows are skipped.
    contains(surface, "bool detailEditable(const State& state");
    contains(surface, "hasAlternateValidForm");
    contains(surface, "if (detailEditable(state, state.focus.row, p)) return;");
    contains(surface, "if (pidLinkedRowEditable(state, state.focus.row, p)) return;");
    contains(surface, "\"OT\", \"Trainer ID\"");
    assert(surface.find("OT (read-only)") == std::string::npos);
    assert(surface.find("TID (read-only)") == std::string::npos);
    contains(surface, "SID (read-only)");
    contains(surface, "setOriginalTrainerName");
    contains(surface, "setTID");
    contains(surface, "Origin (read-only)");
    contains(surface, "state.session.mode == SessionModel::Mode::View || editable");

    // A nested move choice must render above the contextual Move editor, never behind it.
    const auto moveEditorDraw = surface.find("if (state.moveEditor) drawMoveEditor(screen, fb, state);");
    const auto valuePickerDraw = surface.find("if (state.valuePicker) drawValuePicker(screen, fb, state);");
    assert(moveEditorDraw != std::string::npos && valuePickerDraw > moveEditorDraw);

    // Gen IV picker/action chrome follows the accepted shared layout instead of the cramped G4-04 prototype.
    contains(surface, "HeldItemGrid::move");
    contains(surface, "const int w = heldItems ? HeldItemGrid::modalWidth");
    contains(surface, "constexpr auto moveLayout = MoveUI::compactPickerLayout()");
    contains(surface, "movePicker ? moveLayout.width");
    contains(surface, "movePicker ? moveLayout.height");
    contains(surface, "constexpr int visible = moveLayout.visibleRows");
    contains(surface, "moveLayout.rowStep");
    contains(surface, "Held Item — Generation IV");
    contains(surface, "Names::machineDisplayLabel(bridge(screen).sourceSave().rawFamily()");
    contains(surface, "constexpr int w = 560;");
    assert(surface.find("const int w = occupied ? 650 : 560;") == std::string::npos);
    contains(surface, "static_cast<int>(actions.count) * geometry.rowStep + 62");
    contains(surface, "state.target == TargetKind::Party");
    contains(surface, "Master Vault is intentionally not started");
    contains(surface, "const Color disabledColor(");
    contains(surface, "Colors::TextSecondary");
    contains(surface, "Colors::TextDim.b, 105");
    contains(surface, "Live source writes remain hard disabled");
    contains(surface, "drawLegality");
    contains(surface, "drawReleaseConfirm");
    assert(surface.find("const int h = occupied ? 500 : 350;") == std::string::npos);
    contains(surface, "  |  Met Lv. ");
    assert(surface.find(" (current)") == std::string::npos);
    contains(surface, "const bool compactChoices=");
    contains(surface, "visibleChoices");
    contains(surface, "Colors::Text, TextStyle::Body");
    // Item pictures must be actual per-ball/item PNGs and part of reproducible
    // ROMFS recovery, not a generic placeholder drawn for every choice.
    // All Held Item generations now use one exact Gen III grid renderer.
    contains(sharedHeld, "ItemPickerArtwork::draw(fb, cellX + cellWidth - 24");
    contains(sharedHeld, "fb.drawSelectionHighlight(cellX, cellY - 3");
    contains(sharedHeld, "fb.drawText(cellX + 10, cellY + 7");
    contains(gen2Held, "SharedHeldItemPickerPresentation::drawGrid(");
    contains(gen2Editor, "SharedHeldItemPickerPresentation::drawHeading(");
    contains(gen2Editor, "Gen2HeldItemPickerPresentation::drawList(fb, x, y");
    contains(gen2Editor, "const int y = (H - kNavBarH - h) / 2");
    contains(gen3Picker, "SharedHeldItemPickerPresentation::drawGrid(");
    contains(gen3Picker, "SharedHeldItemPickerPresentation::drawHeading(");
    contains(surface, "SharedHeldItemPickerPresentation::drawGrid(");
    contains(surface, "SharedHeldItemPickerPresentation::drawHeading(");
    contains(surface, "ItemPickerArtwork::draw(fb, x + w - 35");
    contains(gen3Picker, "ItemPickerArtwork::draw(fb, x + w - 35");
    contains(gen2Editor, "\"A\", \"Choose\"");
    contains(sharedHeld, "if (value != 0)");
    contains(itemArtwork, "SpriteManager::getItemSprite(exactName)");
    contains(spriteRuntime, "sprites/items/");
    contains(spriteGenerator, "REQUIRED_BALLS");
    contains(spriteGenerator, "PINNED_REF");
    contains(recover, "tools\" / \"gen_item_sprites.py");
    contains(assetGate, "BALL_ICON_NAMES");

    // Gen I is included in the shared move text formatting requirements:
    // all four native picker paths consume the identical separator formatter.
    const auto fixedMoveRows = read("include/UI/MovePickerRowUI.h");
    contains(gen1Move, "MovePickerRowUI::draw(fb");
    contains(gen2Move, "MovePickerRowUI::draw(fb");
    contains(gen3Picker, "MovePickerRowUI::draw(fb");
    contains(surface, "MovePickerRowUI::draw(fb");
    contains(fixedMoveRows, "Presentation::numberedName(move)");
    contains(fixedMoveRows, "x+308");
    contains(fixedMoveRows, "x+423");
    contains(fixedMoveRows, "x+538");
    contains(movePickerModel, "  |  Pwr ");
    contains(movePickerModel, "  |  PP ");

    assert(surface.find("+ \" (#\" + std::to_string(value)") == std::string::npos);

    // Generated location strings must never retain a source UTF-8 BOM as a visible glyph.
    assert(locations.find("\xEF\xBB\xBF") == std::string::npos);
    contains(locationGenerator, "encoding=\"utf-8-sig\"");

    // Read-only Gen IV inventory is now decoded from the selected CRC-valid
    // General partition and rendered in the existing shared Items surface.
    assert(itemsPanel.find("G4-02") == std::string::npos);
    assert(itemsPanel.find("Generation IV inventory support is not implemented yet.") ==
           std::string::npos);
    contains(itemsPanel, "Native Generation IV bag data failed validation.");
    contains(itemsPanel, "bagPocketName");
    contains(bridge, "decodeReadOnlyBag(save_)");
    contains(bridge, "items.assign(bag->begin(),bag->end())");
    // Native Item quantity changes are staged only, routed in the Gen IV
    // overlay BEFORE the inherited generic source mutation/dialog handlers.
    const auto stagedBackend = read("src/Integration/Gen4/Gen4StagedPokemonEditor.cpp");
    contains(stagedBackend, "stageBagQuantity(");
    contains(stagedBackend, "refreshGeneralCrc(*before,error)");
    contains(stagedBackend, "unexpectedly changed unrelated save bytes");
    contains(surface, "if(handleItemQuantity(screen,state,down,held,stickX,stickY))return true;");
    contains(surface, "state.itemQuantityDraft");
    contains(surface, "staged->stageBagQuantity(");
    contains(surface, "auto saved=*staged;");
    contains(surface, "*staged=std::move(saved);");
    contains(surface, "Gen IV item quantity staged; emulator source unchanged");
    contains(surface, "else if (state.itemQuantityActive)drawItemQuantity");
    contains(surface, "if (state.itemAddPickerActive)drawItemAddPicker");
    contains(surface, "else if (state.itemRemoveConfirmActive)drawItemRemoveConfirm");


    // A committed staged edit must not claim UI success when the refreshed presentation failed.
    contains(surface, "if (!refreshPresentation(screen))");
    contains(surface, "Generation IV change is staged, but presentation refresh failed");

    // Dirty Back uses the same shared exit guard and explicit A/Y/B confirmation.
    contains(session, "PokemonEditorExitGuard::requiresConfirmation");
    contains(surface, "if (state.session.confirmExit)");
    contains(surface, "HidNpadButton_Y");
    contains(surface, "Dialogs::drawDialogFrame");
    contains(surface, "\"Unsaved changes\"");
    contains(surface, "\"Y\", \"Discard\"");

    // Staged trainer bridge exists, but the immutable source stays owned separately.
    contains(bridge, "Gen4StagedPokemonEditor::create(");
    contains(bridge, "save.sourceBytes()");
    contains(bridge, "refreshStagedPokemonPresentation");

    // Invalid unrelated Gen IV records remain quarantined instead of making a valid
    // staged edit to another Pokémon fail presentation refresh.
    contains(bridge, "displayParty.push_back(nullptr);");
    contains(bridge, "if (!pokemon.valid() || pokemon.empty()) continue;");
    contains(bridge, "displayParty.push_back(nullptr)");
    contains(bridge, "if (!pokemon.valid() || pokemon.empty()) continue;");
    contains(surface, "if (!refreshPresentation(screen))");
    contains(staged, "original_");
    contains(staged, "staged_");
    contains(staged, "refreshStorageCrc");
    contains(staged, "refreshGeneralCrc");
    contains(staged, "commitPartyPokemon");
    contains(staged, "Gen4ReadOnlySave::parse(");
    assert(staged.find("std::fopen") == std::string::npos);
    assert(staged.find("std::fwrite") == std::string::npos);
    assert(staged.find("rename(") == std::string::npos);

    // Save-level B/+ exit guards protect staged work even when the external source is immutable.
    contains(trainerBase, "virtual bool hasStagedChanges() const noexcept { return false; }");
    contains(rbyBridge, "hasStagedChanges() const noexcept override");
    contains(gscBridge, "hasStagedChanges() const noexcept override");
    contains(gen3Bridge, "hasStagedChanges() const noexcept override");
    contains(gen4Bridge, "hasStagedChanges() const noexcept override");
    contains(baseUi, "hasUnsavedChanges || trainer.hasStagedChanges()");
    contains(baseUi, "PokeVault::Integration::Gen4::BagPocketCount");
    contains(baseUi, "sourceGameId==\"platinum_nds\"");
    contains(baseUi, "A: Edit Quantity  |  Y: Remove...");
    contains(baseUi, "X: Add Item");

    contains(baseUi, "immutableSourceBlocksSaveDialog(");
    contains(baseUi, "exitOnlySaveConfirm");
    assert(baseUi.find("!sourceReadOnly() && hasUnsavedChanges") == std::string::npos);
    assert(baseUi.find("!sourceReadOnly() && bank && bank->hasChanged()") == std::string::npos);

    contains(baseUi, "immutableSourceBlocksSaveDialog(");
    contains(baseUi, "const bool exitOnlySaveConfirm = saveConfirmActive && exitingWithUnsavedChanges;");
    assert(baseUi.find("statEdit.dialogActive = saveConfirmActive") == std::string::npos);

    // Gen IV Move Pokémon uses exact staged native 0x88-byte transactions.
    // Action visibility and A/Touch move-confirm paths must not depend on
    // the legacy source-mutating box machinery.
    contains(surface, "MenuAction::MovePokemon");
    contains(surface, "state.moveSourceBox = state.box;");
    contains(surface, "state.moveSourceSlot = state.slot;");
    contains(surface, "bool handleBoxMove(");
    contains(surface, "stageMoveBoxPokemon(");
    contains(surface, "if (down & HidNpadButton_B)");
    contains(surface, "state.moveActive = false;");
    contains(surface, "Gen IV move/swap staged; original source unchanged");
    // The outer border is always neutral; focus belongs to the selected row.
    const auto addItemStart = surface.find("void drawItemAddPicker(");
    const auto addItemEnd = surface.find("void drawItemRemoveConfirm(", addItemStart);
    assert(addItemStart != std::string::npos && addItemEnd > addItemStart);
    const auto addItemBody = surface.substr(addItemStart, addItemEnd - addItemStart);
    assert(addItemBody.find("fb.drawRoundedRect(x,y,w,h,18,Colors::Divider,1)") != std::string::npos);
    assert(addItemBody.find("fb.drawRoundedRect(x,y,w,h,18,Colors::FocusBorder,1)") == std::string::npos);
    contains(addItemBody, "ItemPickerArtwork::draw");

    // Final composite routes Gen IV before generic/Gen III fallback.
    contains(composite, "#include \"Gen4SharedPokemonSurface.inc\"");
    const auto update = composite.find("void TrainerViewScreen::update");
    const auto g4input = composite.find("Gen4SharedEditorSurface::handleInput", update);
    const auto g3input = composite.find("Gen3SharedEditorSurface::handleInput", update);
    assert(update != std::string::npos && g4input > update && g3input > g4input);
    const auto draw = composite.find("void TrainerViewScreen::draw(PKSEFramebuffer& fb)");
    const auto g4draw = composite.find("Gen4SharedEditorSurface::draw", draw);
    const auto g3draw = composite.find("Gen3SharedEditorSurface::draw", draw);
    assert(draw != std::string::npos && g4draw > draw && g3draw > g4draw);

    std::cout << "Gen IV shared editor hardware surface contract PASS\n";
    return 0;
}
