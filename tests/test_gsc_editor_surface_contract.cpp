#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "Gen II editor foundation source must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    const auto foundation = readFile("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto unified = readFile("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto parity = readFile("src/UI/Gen2SharedSurfaceParity.inc");
    const auto shared = readFile("src/UI/Gen2SharedPokemonSurface.inc");
    const auto composite = readFile("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto trainerPage = readFile("src/UI/Gen2TrainerSessionOverlay.inc");
    const auto pickerFix = readFile("src/UI/Gen2HardwarePickerFix.inc");
    const auto workspaceFix = readFile("src/UI/Gen2HardwareWorkspaceFix.inc");
    const auto finalFix = readFile("src/UI/Gen2HardwareFinalFix.inc");
    const auto picker = readFile("src/UI/Gen2PokemonPickerOverlay.inc");
    const auto pickerModel = readFile("include/UI/Gen2PokemonPickerModel.h");

    assert(foundation.find("Gen II Level") != std::string::npos);
    assert(foundation.find("Gen II Experience") != std::string::npos);

    assert(foundation.find("state.baseline = *pokemon") != std::string::npos);
    assert(foundation.find("state.working = *pokemon") != std::string::npos);
    assert(foundation.find("Keep staged Pokemon edits?") != std::string::npos);
    assert(foundation.find("This Edit session discarded; earlier staged work preserved") != std::string::npos);
    assert(foundation.find("{\"A\", \"Keep\"}") != std::string::npos);
    assert(foundation.find("{\"X\", \"Discard\"}") != std::string::npos);
    assert(foundation.find("{\"B\", \"Continue\"}") != std::string::npos);

    assert(foundation.find("Add blocked: correct red incompatible move rows first") != std::string::npos);
    assert(foundation.find("Create draft cancelled; staged save unchanged") != std::string::npos);
    assert(unified.find("{\"X\", \"Add\"}") != std::string::npos);
    assert(unified.find("Stage Add") == std::string::npos);
    assert(foundation.find("Pokemon added to staged copy only; source .srm unchanged") == std::string::npos);

    assert(foundation.find("refreshStagedBoxPresentation(error)") != std::string::npos);
    assert(foundation.find("closeToBoxAfterAdd(screen, destinationBox, static_cast<int>(slot))") != std::string::npos);
    assert(foundation.find("screen.selectedBoxIndex = target.box") != std::string::npos);
    assert(foundation.find("screen.selectedItemIndex = target.slot") != std::string::npos);
    assert(foundation.find("legacy.pokemonActions = false") != std::string::npos);
    assert(foundation.find("added to Box") != std::string::npos);

    assert(parity.find("SharedEditor::boxActivation") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Add") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Actions") != std::string::npos);
    assert(parity.find("if (down & HidNpadButton_X) return true") != std::string::npos);
    assert(composite.find("{\"A\", \"Actions\"}") != std::string::npos);
    assert(composite.find("{\"X\", \"Add\"}") != std::string::npos);
    assert(composite.find("{\"L/R\", \"Box\"}") != std::string::npos);
    assert(composite.find("drawGen2ClassicBoxFooter") != std::string::npos);

    assert(shared.find("caps.hasLegalityProvenance = true") != std::string::npos);
    assert(shared.find("SharedEditor::actionLabel(action)") != std::string::npos);
    assert(parity.find("SharedEditor::actionMenuGeometry()") != std::string::npos);
    assert(parity.find("screen.drawGSCOverlay(fb)") != std::string::npos);
    assert(parity.find("Colors::FocusBorder, 2") != std::string::npos);
    assert(shared.find("drawSharedProvenanceSurface") != std::string::npos);
    assert(shared.find("Encounter legality\", \"Not checked") != std::string::npos);
    assert(shared.find("case SharedEditor::Action::Close") != std::string::npos);

    // The established inline page owns rendering and exactly two cursor rows.
    assert(trainerPage.find("drawTrainerSession") == std::string::npos);
    assert(trainerPage.find("drawFilledRect") == std::string::npos);
    assert(trainerPage.find("screen.trainerSelectedRow = (screen.trainerSelectedRow + 1) % 2") != std::string::npos);
    assert(trainerPage.find("editor->stageTrainerEdit(name, money, error)") != std::string::npos);
    assert(parity.find("trainerSessionFor") == std::string::npos);
    const auto base = readFile("src/UI/TrainerViewScreenBase.inc");
    assert(base.find("constexpr int kEditRows = 2") != std::string::npos);
    assert(unified.find("Unified::statsHeading()") != std::string::npos);
    assert(unified.find("Unified::cellFocus(unified.focus)") != std::string::npos);
    assert(unified.find("passive ? 0 : unified.detailsFocus") != std::string::npos);
    const auto summary = readFile("src/UI/Panels/BoxPokemonPanel.cpp");
    assert(summary.find("StatsRadar::drawGen2Labeled") != std::string::npos);
    assert(summary.find("Gen2Workspace::battleStats(record)") != std::string::npos);

    // Exact-format capability hygiene and Gen II Values remain truthful: descriptive native fields
    // are now in DETAILS, while VALUES keeps stats plus derived Shiny/Gender.
    assert(foundation.find("HP DV is derived") != std::string::npos);
    assert(unified.find("\"DV\"") != std::string::npos);
    assert(unified.find("\"Stat Exp\"") != std::string::npos);
    assert(unified.find("CALCULATED SPECIAL STATS") != std::string::npos);
    assert(unified.find("\"SpA\"") != std::string::npos);
    assert(unified.find("\"SpD\"") != std::string::npos);
    assert(unified.find("{\"Held Item\", itemText(p.heldItem)}") != std::string::npos);
    assert(unified.find("{\"Friendship\", std::to_string(p.friendship)}") != std::string::npos);
    assert(unified.find("{\"Pokerus\", Gen2Native::pokerusText(p.pokerus)}") != std::string::npos);
    assert(unified.find("std::array<std::pair<std::string, std::string>, 2> capabilityRows") != std::string::npos);
    assert(unified.find("Encounter legality: Not checked") != std::string::npos);
    assert(unified.find("scrollWindow(rows.size(), 8") != std::string::npos);
    assert(finalFix.find("scrollWindow(rows.size(), 8") != std::string::npos);
    assert(finalFix.find("viewportY = y + 216") != std::string::npos);
    assert(finalFix.find("fb.clearClip()") != std::string::npos);
    assert(unified.find("\"OT Name\", p.originalTrainer") != std::string::npos);
    assert(unified.find("Met Level for selected Crystal encounter") != std::string::npos);
    assert(unified.find("Encounter::forGameSpecies(screen.sourceGameId, p.species)") != std::string::npos);

    const auto overlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(overlay.find("publishVerifiedStagedEditorExport(editor, request)") != std::string::npos);
    assert(overlay.find("Verified staged export: ") != std::string::npos);
    assert(overlay.find("source remains unchanged") != std::string::npos);
    assert(overlay.find("std::fopen") == std::string::npos);
    assert(overlay.find("std::fwrite") == std::string::npos);
    assert(overlay.find("writeBytes(") == std::string::npos);
    assert(overlay.find("writeText(") == std::string::npos);

    // Hardware-retest corrections remain narrow wrappers around the accepted shared editor.
    assert(composite.find("#define handlePickerInput handlePickerInputBase") != std::string::npos);
    assert(composite.find("#define drawPickerOverlay drawPickerOverlayBase") != std::string::npos);
    assert(composite.find("#define handleUnifiedGen2SurfaceInput handleUnifiedGen2SurfaceInputBase") != std::string::npos);
    assert(composite.find("#define drawUnifiedGen2Surface drawUnifiedGen2SurfaceBase") != std::string::npos);
    assert(composite.find("#define drawFinalGen2Surface drawFinalGen2SurfaceBase") != std::string::npos);

    // Crystal Met Location is species/exact-game aware; accepting it writes the native
    // location and encounter-minimum caught level rather than an arbitrary landmark.
    assert(pickerModel.find("Encounter::forGameSpecies(sourceGameId, species)") != std::string::npos);
    assert(picker.find("Species-valid Crystal encounters") != std::string::npos);
    assert(picker.find("encounter->minLevel") != std::string::npos);
    assert(picker.find("encounter->timeMask") != std::string::npos);
    assert(picker.find("state.working.caughtData = 0") != std::string::npos);

    // Move slots must enter the accepted Gen I-style Move N contextual editor first.
    assert(unified.find("beginUnifiedMoveEditor") != std::string::npos);
    assert(unified.find("drawUnifiedMoveEditor") != std::string::npos);
    assert(unified.find("Contextual editor — B always cancels this dialog") != std::string::npos);
    assert(unified.find("Keep these move details") != std::string::npos);
    assert(unified.find("Discard these move details") != std::string::npos);
    assert(unified.find("openPickerForFocusedField") != std::string::npos);
    assert(unified.find("beginUnifiedMoveEditor(screen, state, static_cast<int>(focus.row))") != std::string::npos);
    // Nested move picker input must not fall through to the underlying Box A action.
    assert(parity.find("if (pickerStateFor(screen).model.active()) return false;") != std::string::npos);

    // Normal Gen II Move picker: Empty + exact-game-compatible choices only.
    assert(pickerFix.find("hardwareMoveAllowed") != std::string::npos);
    assert(pickerFix.find("MoveCompatibility::canLearnMove") != std::string::npos);
    assert(pickerFix.find("choices.push_back(0)") != std::string::npos);
    assert(pickerFix.find("Empty + compatible moves only") != std::string::npos);
    assert(pickerFix.find("Needs correction") == std::string::npos);

    // Empty move rows cannot focus PP/Ups and their meaningless numeric cells are hidden.
    assert(workspaceFix.find("normalizeHardwareEmptyMoveFocus") != std::string::npos);
    assert(workspaceFix.find("state.working.moves[row] == 0 && unified.focus.column > 0") != std::string::npos);
    assert(workspaceFix.find("unified.focus.column = 0") != std::string::npos);
    assert(workspaceFix.find("state.working.moves[static_cast<std::size_t>(slot)] != 0") != std::string::npos);
    assert(workspaceFix.find("rightX + 184") != std::string::npos);

    // Panel titles are neutral; only the actual field/cell carries focus. The stray scroll glyph is erased.
    assert(workspaceFix.find("clearHardwarePanelHeading") != std::string::npos);
    assert(workspaceFix.find("\"DETAILS\", TextStyle::Caption") != std::string::npos);
    assert(workspaceFix.find("\"STATS\", TextStyle::Heading") != std::string::npos);
    assert(workspaceFix.find("\"MOVES\", TextStyle::Heading") != std::string::npos);
    assert(workspaceFix.find("stray bottom glyph") != std::string::npos);
    assert(workspaceFix.find("contentY + contentH - 34") != std::string::npos);

    // Gold/Silver receive the same informational Gender row that Crystal already renders;
    // the established Name/Money two-row cursor contract above is unchanged.
    assert(finalFix.find("gold_gbc") != std::string::npos);
    assert(finalFix.find("silver_gbc") != std::string::npos);
    assert(finalFix.find("hasTrainerGender()") != std::string::npos);
    assert(finalFix.find("\"Gender\"") != std::string::npos);
    assert(finalFix.find("trainerGender == 0 ? \"Male\"") != std::string::npos);

    std::cout << "GSC shared editor Trainer/field-layout/session/export + hardware regression contract: PASS\n";
    return 0;
}
