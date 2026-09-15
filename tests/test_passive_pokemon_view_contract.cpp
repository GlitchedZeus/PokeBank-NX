#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readFile(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "passive-view contract source file must exist");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void requireCleanPassiveSurface(const std::string& source) {
    assert(source.find("PKSE three-panel workspace") == std::string::npos);
}
} // namespace

int main() {
    // Party/Storage Gen I use the shared passive presenter; occupied action-sheet View is routed
    // through Gen1PokemonEditorPassiveView.inc into that same presenter. Gen II Party/Storage View
    // use Gen2PokemonDetailsModal directly, while the preserved GSC action sheet resolves there too.
    const auto gen1Modal = readFile("src/UI/Modals/Gen1PokemonDetailsModal.cpp");
    const auto gen1ActionView = readFile("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto gen2Modal = readFile("src/UI/Modals/Gen2PokemonDetailsModal.cpp");
    const auto composite = readFile("src/UI/TrainerViewScreenCompositeOverlay.cpp");

    const auto gen2Actions = readFile("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto gen2Routes = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(gen2Actions.find("Rules::slotActionAt(") != std::string::npos);
    assert(gen2Actions.find("Modals::drawGen2PokemonDetailsModal(screen, fb, pokemon)") != std::string::npos);
    assert(gen2Routes.find("validatedGSC && details.active") != std::string::npos);
    assert(gen2Routes.find("passiveViewAction(") != std::string::npos);
    requireCleanPassiveSurface(gen1Modal);
    requireCleanPassiveSurface(gen1ActionView);
    requireCleanPassiveSurface(gen2Modal);

    // Gen I action-sheet View must continue to route through the passive presenter rather than the
    // editable foundation workspace.
    assert(composite.find("foundationPassiveViewActive") != std::string::npos);
    assert(composite.find("drawFoundationPassiveView") != std::string::npos);

    // Gen II View is informational only: B Back, no correction-required red state, and encounter
    // legality remains explicitly deferred even though live move compatibility is calculated.
    assert(gen2Modal.find("Move compatibility") != std::string::npos);
    assert(gen2Modal.find("Unusual preserved") != std::string::npos);
    assert(gen2Modal.find("Encounter legality") != std::string::npos);
    assert(gen2Modal.find("Not checked") != std::string::npos);
    assert(gen2Modal.find("Needs correction") == std::string::npos);
    assert(gen2Modal.find("{\"B\", \"Back\"}") != std::string::npos);
    // Check executable rendering calls, not wording of a removed developer comment.
    assert(gen2Modal.find("drawSelectionHighlight(") == std::string::npos);
    assert(gen2Modal.find("selectedRowBackground(") == std::string::npos);
    assert(gen2Modal.find("stageBoxPokemonEdit(") == std::string::npos);
    assert(gen2Modal.find("promptNumber(") == std::string::npos);

    std::cout << "Passive Pokemon View surface contract: PASS\n";
    return 0;
}
