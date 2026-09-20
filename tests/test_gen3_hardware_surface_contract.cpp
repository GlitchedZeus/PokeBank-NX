#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonShell.h"
#include "UI/ClassicPackedMultiSelect.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>

std::string read(const char* path) {
    std::ifstream in(path); assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
void contains(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing surface contract: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
int main() {
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    namespace Multi = PokeBank::UIModel::ClassicPackedMove;
    constexpr UI::SharedPokemonShell::Geometry geometry(1280, 720, 46);
    static_assert(geometry.leftX == 24 && geometry.leftW == 300 && geometry.midX == 338);
    static_assert(geometry.midW == 398 && geometry.rightX == 750 && geometry.rightW == 506);
    static_assert(geometry.y + geometry.h == 666);
    // Execute the actual shared navigation/window model through every Gen III Details row.
    Shared::Focus focus{Shared::Panel::Details, 0, 0};
    for (int row = 0; row < 15; ++row) {
        assert(focus.row == row);
        const auto window = Shared::scrollWindow(15, 6, focus.row);
        assert(window.scrolls && window.count == 6);
        assert(window.first <= focus.row && focus.row < window.first + window.count);
        assert(window.first + window.count <= 15);
        // Last value baseline fits the Details panel; sprite/type header remains above it.
        assert(224 + int(window.count - 1) * 48 + 20 + 20 < geometry.h - 30);
        if (row < 14) focus = Shared::moveVertical(Shared::Generation::Gen3, focus, 1);
    }
    const auto lastDetails = Shared::detailsScrollFocus(focus, 0);
    focus = Shared::switchPanel(Shared::Generation::Gen3, focus, 1);
    assert(Shared::detailsScrollFocus(focus, lastDetails) == 14);
    const auto rectangle = Multi::rectangleSlots(14, 0, 30);
    const int expected[] = {0,1,2,6,7,8,12,13,14};
    assert(rectangle.count == 9);
    for (unsigned i = 0; i < rectangle.count; ++i) assert(rectangle[i] == expected[i]);
    assert(Multi::moveCursor(29, Multi::Direction::Right, 30) == 29);
    assert(Multi::moveCursor(29, Multi::Direction::Down, 30) == 29);

    // Production routing and renderer wiring (Switch compilation is a separate native gate).
    const auto surface = read("src/UI/Gen3SharedPokemonSurface.inc");
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto move = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto shell = read("include/UI/SharedPokemonShell.h");
    const auto gen2 = read("src/UI/Gen2HardwareFinalFix.inc");
    contains(surface, "beginPassiveView(screen)");
    contains(surface, "screen.detailsTargetPokemon()"); // Box, Party and Bank exact target resolver
    contains(surface, "target->getGameGroup() != Enums::GameVersion::FRLG");
    contains(surface, "state.session.begin(*record, SessionModel::Mode::View)");
    contains(surface, "screen.closeDetailsModal()");
    contains(surface, "if (!(down & HidNpadButton_A) || !state.session.editable()) return true");
    contains(surface, "Nature, Gender, Shiny, Ability and PID are read-only");
    contains(surface, "TID (read-only)"); contains(surface, "SID (read-only)");
    contains(surface, "SharedPokemonShell::drawChrome");
    contains(gen2, "SharedPokemonShell::drawChrome");
    contains(surface, "SharedPokemonShell::Geometry"); contains(gen2, "SharedPokemonShell::Geometry");
    contains(surface, "SharedPokemonShell::drawPortrait"); contains(surface, "getTypeSprite");
    contains(surface, "SharedPokemonShell::drawDataAndGraph");
    contains(shell, "BATTLE STATS"); contains(shell, "StatsRadar::drawGen2Labeled");
    contains(surface, "GEN III DATA"); contains(surface, "Colors::Panel");
    contains(surface, "SharedPokemonShell::drawScrollableDetails");
    contains(shell, "scrollWindow(total, 6, focus)");
    contains(shell, "viewportY = y + 216");
    contains(shell, "fb.setClipRect");
    contains(shell, "fb.clearClip()");
    contains(surface, "previewCreate"); contains(surface, "previewEdit");
    contains(surface, "SessionModel::sameEditableRecord");
    contains(surface, "Encounter::forGameSpecies(screen.sourceGameId, state.session.working.species)");
    contains(surface, "Met Level for selected encounter");
    contains(surface, "normalizeEditableFocus(state");
    contains(surface, "beginMoveEditor");
    contains(surface, "drawMoveEditor");
    contains(surface, "Contextual editor — B always cancels this dialog");
    contains(surface, "Keep these move details");
    contains(surface, "Discard these move details");
    contains(surface, "000 - Empty move slot");
    contains(surface, "openValuePicker(screen, state, PickerTarget::Move, state.moveEditorSlot)");
    assert(surface.find("Add Pokemon") != std::string::npos); // Box action still exists.
    const auto moveEditorBegin = surface.find("void drawMoveEditor(");
    const auto workspaceBegin = surface.find("void drawWorkspace(", moveEditorBegin);
    assert(moveEditorBegin != std::string::npos && workspaceBegin > moveEditorBegin);
    contains(surface, "state.session.editable() &&");
    contains(surface, "screen.drawGSCOverlay(fb)");
    contains(surface, "Game/species/location/level constrained");
    const auto actionsBegin = surface.find("void drawActions(");
    const auto actionsEnd = surface.find("void drawReview(", actionsBegin);
    assert(actionsBegin != std::string::npos && actionsEnd != std::string::npos && actionsEnd > actionsBegin);
    const auto actionsBody = surface.substr(actionsBegin, actionsEnd - actionsBegin);
    assert(actionsBody.find("screen.drawGSCOverlay(fb)") != std::string::npos);
    assert(actionsBody.find("Colors::Background") == std::string::npos);
    assert(actionsBody.find("const int w = occupied ? 650 : 560") != std::string::npos);
    assert(actionsBody.find("const int h = occupied ? 500 : 350") != std::string::npos);
    assert(actionsBody.find("Shared::actionMenuGeometry()") != std::string::npos);
    assert(actionsBody.find("Colors::FocusBorder, 2") != std::string::npos);
    assert(actionsBody.find("constexpr int x = 250, y = 72, w = 780, h = 560") == std::string::npos);
    contains(composite, "return !Gen3SharedEditorSurface::ownsFrame(screen)");
    assert(composite.find("ClassicPackedMove::handleInput") < composite.find("Gen3SharedEditorSurface::handleInput"));
    contains(move, "Move / Hold Multi"); contains(move, "Hold Multi");
    contains(move, "kMultiHoldFrames"); contains(move, "rectangleSlots");
    contains(move, "beginSparseMove"); contains(move, "placeSparseMove"); contains(move, "cancelSparseMove");
    contains(move, "beginPackedMove"); contains(move, "placePackedGroupMove"); // accepted packed backend remains
    contains(move, "if (isGen3(screen)) return beginGen3");
    std::cout << "Gen III hardware routing, shared shell, Details scroll and Y contract: PASS\n";
}
