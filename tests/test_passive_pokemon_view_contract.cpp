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
    // Party/Storage Gen I use the mature passive presenter; occupied action-sheet View is routed
    // through the accepted fullscreen correction layer. Gen II Party/Storage/action View receives
    // the final fullscreen GSC repaint.
    const auto gen1Modal = readFile("src/UI/Modals/Gen1PokemonDetailsModal.cpp");
    const auto gen1Presentation = readFile("src/UI/Gen1PokemonDetailsPresentation.cpp");
    const auto gen1ActionView = readFile("src/UI/Gen1PokemonEditorPassiveView.inc");
    const auto gen2Modal = readFile("src/UI/Modals/Gen2PokemonDetailsModal.cpp");
    const auto gen2Final = readFile("src/UI/Gen2HardwareFinalFix.inc");
    const auto composite = readFile("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto gen3Surface = readFile("src/UI/Gen3SharedPokemonSurface.inc");

    const auto gen2Actions = readFile("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto gen2Routes = readFile("src/UI/TrainerViewScreenGSCOverlay.inc");
    assert(gen2Actions.find("Rules::slotActionAt(") != std::string::npos);
    assert(gen2Actions.find("Modals::drawGen2PokemonDetailsModal(screen, fb, pokemon)") != std::string::npos);
    assert(gen2Routes.find("validatedGSC && details.active") != std::string::npos);
    assert(gen2Routes.find("passiveViewAction(") != std::string::npos);
    requireCleanPassiveSurface(gen1Modal);
    requireCleanPassiveSurface(gen1ActionView);
    requireCleanPassiveSurface(gen1Presentation);
    requireCleanPassiveSurface(gen2Modal);
    requireCleanPassiveSurface(gen2Final);

    // Pokémon-tab / Party Gen I View is no longer the old card renderer: it shares canonical
    // type sprites, neutral panel headings, exact-game context, accent calculated stats, and
    // a fixed far-right compatibility/status region.
    assert(gen1Presentation.find("ClassicTypeBadges::drawPairCentered") != std::string::npos);
    assert(gen1Presentation.find("passiveContextLine(p)") != std::string::npos);
    assert(gen1Presentation.find("\"STATS\", Colors::Text") != std::string::npos);
    assert(gen1Presentation.find("\"VALUES\"") == std::string::npos);
    assert(gen1Presentation.find("Colors::Accent, TextStyle::Caption") != std::string::npos);
    assert(gen1Presentation.find("moveStatusRightPad") != std::string::npos);
    assert(gen1Presentation.find("ppRightOffset") != std::string::npos);
    assert(gen1Presentation.find("drawPanelSurface(fb, workspaceX") == std::string::npos);
    assert(gen1Modal.find("view.sourceGameLabel = sourceGameLabel(screen.sourceGameId)") != std::string::npos);

    // Gen I action-sheet View remains routed through the accepted passive/fullscreen ownership
    // rather than becoming an editable source mutation route.
    assert(composite.find("foundationPassiveViewActive") != std::string::npos);
    assert(composite.find("drawFoundationPassiveView") != std::string::npos);

    // Gen II View is informational only and the final repaint owns passive Party/Box details.
    assert(gen2Modal.find("Move compatibility") != std::string::npos);
    assert(gen2Modal.find("Unusual preserved") != std::string::npos);
    assert(gen2Modal.find("Encounter legality") != std::string::npos);
    assert(gen2Modal.find("Not checked") != std::string::npos);
    assert(gen2Modal.find("Needs correction") == std::string::npos);
    assert(gen2Modal.find("{\"B\", \"Back\"}") != std::string::npos);
    assert(gen2Modal.find("drawSelectionHighlight(") == std::string::npos);
    assert(gen2Modal.find("selectedRowBackground(") == std::string::npos);
    assert(gen2Modal.find("stageBoxPokemonEdit(") == std::string::npos);
    assert(gen2Modal.find("promptNumber(") == std::string::npos);
    assert(gen2Final.find("drawFullscreenGen2Passive") != std::string::npos);
    assert(gen2Final.find("* HP DV derived / read-only • one stored Special DV / Stat Exp") == std::string::npos);
    assert(gen2Final.find("x + w - 18 - statusW") != std::string::npos);

    // Gen III read-only Party/Box View keeps navigation visible without enabling mutation.
    assert(gen3Surface.find("state.focus.panel == Shared::Panel::Details") != std::string::npos);
    assert(gen3Surface.find("state.focus.panel == Shared::Panel::Values") != std::string::npos);
    assert(gen3Surface.find("state.focus.panel == Shared::Panel::Moves") != std::string::npos);
    assert(gen3Surface.find("{\"D-pad\", \"Navigate\"}, {\"L/R\", \"Panel\"}, {\"B\", \"Back\"}") != std::string::npos);
    assert(gen3Surface.find("if (!(down & HidNpadButton_A) || !state.session.editable()) return true") != std::string::npos);

    std::cout << "Passive Pokemon View surface contract: PASS\n";
    return 0;
}
