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
    assert(shared.find("drawSharedProvenanceSurface") != std::string::npos);
    assert(shared.find("Encounter legality\", \"Not checked") != std::string::npos);
    assert(shared.find("case SharedEditor::Action::Close") != std::string::npos);

    // Main Trainer page owns row navigation/editing. The detached full-screen Trainer editor title
    // is gone; Name/Money are editable and Trainer ID/Gender remain focusable/read-only.
    assert(trainerPage.find("Trainer — Generation II") != std::string::npos);
    assert(trainerPage.find("Edit Trainer — STAGED") == std::string::npos);
    assert(trainerPage.find("state.row = (state.row + 3) % 4") != std::string::npos);
    assert(trainerPage.find("state.row = (state.row + 1) % 4") != std::string::npos);
    assert(trainerPage.find("down & (HidNpadButton_A | HidNpadButton_X)") != std::string::npos);
    assert(trainerPage.find("{\"Name\", name}") != std::string::npos);
    assert(trainerPage.find("{\"Money\", \"$\" + std::to_string(money)}") != std::string::npos);
    assert(trainerPage.find("{\"Trainer ID\", std::to_string(editor->trainerId())}") != std::string::npos);
    assert(trainerPage.find("{\"Gender\", gender}") != std::string::npos);
    assert(trainerPage.find("i < 2") != std::string::npos);
    assert(trainerPage.find("Trainer ID is read-only") != std::string::npos);
    assert(trainerPage.find("Gender is save-derived/read-only") != std::string::npos);
    assert(trainerPage.find("Keep staged Trainer edits?") != std::string::npos);
    assert(trainerPage.find("Discard this Edit") != std::string::npos);
    assert(trainerPage.find("Continue") != std::string::npos);
    assert(trainerPage.find("original source immutable") != std::string::npos);

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
    assert(unified.find("scrollWindow(rows.size(), 5") != std::string::npos);

    const auto overlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(overlay.find("publishVerifiedStagedEditorExport(editor, request)") != std::string::npos);
    assert(overlay.find("Verified staged export: ") != std::string::npos);
    assert(overlay.find("source remains unchanged") != std::string::npos);
    assert(overlay.find("std::fopen") == std::string::npos);
    assert(overlay.find("std::fwrite") == std::string::npos);
    assert(overlay.find("writeBytes(") == std::string::npos);
    assert(overlay.find("writeText(") == std::string::npos);

    std::cout << "GSC shared editor Trainer/field-layout/session/export production contract: PASS\n";
    return 0;
}