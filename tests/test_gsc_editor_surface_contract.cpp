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

    // Hardware swkbd safety: Level and EXP are separate logical actions, so one A press never
    // deliberately chains the two library applets in the same update.
    assert(foundation.find("Gen II Level") != std::string::npos);
    assert(foundation.find("Gen II Experience") != std::string::npos);

    // Transactional Edit snapshots the current staged entry, edits a local working copy, and
    // exposes the accepted Keep / discard-this-session / Continue interaction.
    assert(foundation.find("state.baseline = *pokemon") != std::string::npos);
    assert(foundation.find("state.working = *pokemon") != std::string::npos);
    assert(foundation.find("Keep staged Pokemon edits?") != std::string::npos);
    assert(foundation.find("This Edit session discarded; earlier staged work preserved") != std::string::npos);
    assert(foundation.find("{\"A\", \"Keep\"}") != std::string::npos);
    assert(foundation.find("{\"X\", \"Discard\"}") != std::string::npos);
    assert(foundation.find("{\"B\", \"Continue\"}") != std::string::npos);

    // Create remains a staged-copy transaction internally, but hardware-facing language is Add.
    assert(foundation.find("Add blocked: correct red incompatible move rows first") != std::string::npos);
    assert(foundation.find("Create draft cancelled; staged save unchanged") != std::string::npos);
    assert(unified.find("{\"X\", \"Add\"}") != std::string::npos);
    assert(unified.find("Stage Add") == std::string::npos);
    assert(foundation.find("Pokemon added to staged copy only; source .srm unchanged") == std::string::npos);

    // Successful Add returns to the box, refreshes from staged bytes, selects the truthful native
    // packed append slot, and deliberately leaves Pokemon Actions closed.
    assert(foundation.find("refreshStagedBoxPresentation(error)") != std::string::npos);
    assert(foundation.find("closeToBoxAfterAdd(screen, destinationBox, static_cast<int>(slot))") != std::string::npos);
    assert(foundation.find("screen.selectedBoxIndex = target.box") != std::string::npos);
    assert(foundation.find("screen.selectedItemIndex = target.slot") != std::string::npos);
    assert(foundation.find("legacy.pokemonActions = false") != std::string::npos);
    assert(foundation.find("added to Box") != std::string::npos);

    // Gen II box interaction is the same classic contract as Gen I: A opens contextual Actions,
    // X is a direct Add shortcut only on an empty cell, and occupied X cannot hide an edit action.
    assert(parity.find("SharedEditor::boxActivation") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Add") != std::string::npos);
    assert(parity.find("SharedEditor::BoxActivation::Actions") != std::string::npos);
    assert(parity.find("if (down & HidNpadButton_X) return true") != std::string::npos);
    assert(composite.find("{\"A\", \"Actions\"}") != std::string::npos);
    assert(composite.find("{\"X\", \"Add\"}") != std::string::npos);
    assert(composite.find("{\"L/R\", \"Box\"}") != std::string::npos);
    assert(composite.find("drawGen2ClassicBoxFooter") != std::string::npos);

    // Empty/occupied actions share the common action contract and accepted classic geometry.
    assert(shared.find("caps.hasLegalityProvenance = true") != std::string::npos);
    assert(shared.find("SharedEditor::actionLabel(action)") != std::string::npos);
    assert(parity.find("SharedEditor::actionMenuGeometry()") != std::string::npos);
    assert(shared.find("drawSharedProvenanceSurface") != std::string::npos);
    assert(shared.find("Encounter legality\", \"Not checked") != std::string::npos);
    assert(shared.find("case SharedEditor::Action::Close") != std::string::npos);

    // Exact-format capability hygiene and authentic Gen II Values remain visible in the shared shell.
    assert(foundation.find("HP DV is derived") != std::string::npos);
    assert(unified.find("\"DV\"") != std::string::npos);
    assert(unified.find("\"Stat Exp\"") != std::string::npos);
    assert(unified.find("CALCULATED SPECIAL STATS") != std::string::npos);
    assert(unified.find("\"SpA\"") != std::string::npos);
    assert(unified.find("\"SpD\"") != std::string::npos);
    assert(unified.find("Held Item") != std::string::npos);
    assert(unified.find("Friendship") != std::string::npos);
    assert(unified.find("Pokerus") != std::string::npos);
    assert(unified.find("Encounter legality: Not checked") != std::string::npos);
    assert(unified.find("scrollWindow") != std::string::npos);

    // The real GSC Review -> Export UI is only a metadata/path wrapper around the verified staged
    // transaction. No second fopen/fwrite implementation may creep back into the overlay.
    const auto overlay = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(overlay.find("publishVerifiedStagedEditorExport(editor, request)") != std::string::npos);
    assert(overlay.find("Verified staged export: ") != std::string::npos);
    assert(overlay.find("source remains unchanged") != std::string::npos);
    assert(overlay.find("std::fopen") == std::string::npos);
    assert(overlay.find("std::fwrite") == std::string::npos);
    assert(overlay.find("writeBytes(") == std::string::npos);
    assert(overlay.find("writeText(") == std::string::npos);

    std::cout << "GSC shared editor UX parity/session/export production contract: PASS\n";
    return 0;
}
