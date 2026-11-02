// Gen II Pokemon Actions -> Clone ownership (ClassicReleaseActionFix.inc, Gen2SharedPokemonSurface.inc).
//
// Round 11 H31 (owner worksheet, FAIL): "gen 2 when i clone a pokemon its creates it in the box and
// than revert back to the action menu ... it shouldnt do that, it should clone the pokemon and no
// menu popup like gen 1". This test previously pinned that reopen (the Clone dialog's backToActions()
// after staging); it now pins the owner's requested behaviour instead:
//  - BOTH Gen II Clone entry points (the live Actions handler and the shared surface) close the
//    Actions sheet and start the shared visual Clone carry (ClassicPackedMove::beginCloneFromAction),
//    so A places the copy, B cancels, and no Actions menu reopens after the clone;
//  - the separate Gen II Clone dialog (cloneActive / handleSharedClone / its two draws / its touch
//    path) and the legacy Gen II keypad clone are gone, so no second Clone UI can resurface;
//  - the Actions layer still refuses ownership under Legality & Provenance (the guard kept from the
//    original fix), and Actions is still routed ahead of the final Gen II surface.
// The end-to-end behaviour through the real TrainerViewScreen is tests/test_ui_clone_carry_host.cpp.
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
std::string between(const std::string& text, const char* begin, const char* end, size_t from = 0) {
    const auto a = text.find(begin, from);
    if (a == std::string::npos) std::cerr << "Missing route start: " << begin << '\n';
    assert(a != std::string::npos);
    const auto b = text.find(end, a + 1);
    if (b == std::string::npos) std::cerr << "Missing route end: " << end << '\n';
    assert(b != std::string::npos);
    return text.substr(a, b - a);
}
void has(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void lacks(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos) std::cerr << "Unexpected: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

// One Clone entry point: the sheet closes before the shared carry starts, and nothing reopens it.
void assertCloneEntry(const std::string& clone) {
    has(clone, "legacy.active = false;");
    has(clone, "legacy.pokemonActions = false;");
    has(clone, "ClassicPackedMove::beginCloneFromAction(screen, box, slot)");
    assert(clone.find("legacy.pokemonActions = false;") < clone.find("beginCloneFromAction"));
    lacks(clone, "cloneActive");
    lacks(clone, "pokemonActions = true");
    lacks(clone, "stageCloneBoxPokemon(");   // nothing is staged before A in the carry
}

} // namespace

int main() {
    const auto release = read("src/UI/ClassicReleaseActionFix.inc");
    const auto gen2 = release.substr(release.find("namespace UI::Gen2PokemonEditor {"));
    const auto surface = read("src/UI/Gen2SharedPokemonSurface.inc");

    // Both Clone entry points (live Actions and the shared surface).
    assertCloneEntry(between(gen2, "case Action::Clone: {", "case Action::Remove:"));
    assertCloneEntry(between(surface, "case SharedEditor::Action::Clone: {",
                             "case SharedEditor::Action::LegalityProvenance: {"));

    // The old Clone dialog and its "return to Actions" are gone everywhere.
    lacks(surface, "cloneActive");
    lacks(surface, "handleSharedClone");
    lacks(surface, "drawSharedCloneSurface");
    lacks(surface, "backToActions");
    lacks(read("src/UI/Gen2SharedSurfaceParity.inc"), "drawParityGen2Clone");
    lacks(read("src/UI/Gen2HardwareWorkspaceFix.inc"), "cloneActive");
    lacks(gen2, "cloneActive");
    // The legacy Gen II action handler routes Clone to the same carry (no keypad).
    const auto legacy = read("src/UI/TrainerViewScreenGSCOverlay.inc");
    lacks(legacy, "Clone to Gen II Box");
    has(between(legacy, "case Actions::SlotAction::Clone: {", "case Actions::SlotAction::Add:"),
        "ClassicPackedMove::beginCloneFromAction(screen, box, slot)");

    // Input and draw guards: the Actions layer never owns a frame under Legality & Provenance, and the
    // guard runs before the action sheet consumes A/B.
    const auto input = between(gen2, "bool handleReleaseActionInput(", "bool drawReleaseActionSurface(");
    const auto inputGuard = input.find("if (sharedSurfaceStateFor(screen).provenanceActive) return false;");
    assert(inputGuard != std::string::npos);
    assert(inputGuard < input.find("if (down & HidNpadButton_B)"));
    assert(inputGuard < input.find("if (!(down & HidNpadButton_A)) return true;"));
    const auto draw = between(gen2, "bool drawReleaseActionSurface(", "} // namespace UI::Gen2PokemonEditor");
    const auto drawGuard = draw.find("if (sharedSurfaceStateFor(screen).provenanceActive) return false;");
    assert(drawGuard != std::string::npos && drawGuard < draw.find("drawNarrowPokemonActions("));

    // The shared carry places only on A over an empty slot, and never reopens Actions after the clone.
    const auto carry = read("src/UI/ClassicPackedMoveOverlay.inc");
    const auto placeClone = between(carry, "bool placeClone(", "\nbool place(");
    has(placeClone, "Carry::decideCloneDrop(");
    has(placeClone, "editor->stageCloneBoxPokemon(sourceBox, sourceSlot, toBox, landed, why)");
    lacks(placeClone, "pokemonActions");

    // Why: the Actions handler is routed ahead of the Gen II final surface, and the carry layer only
    // runs once the Actions overlay is closed (classicPackedMoveLayerAvailable).
    const auto composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp");
    const auto actionsRoute = composite.find("Gen2PokemonEditor::handleReleaseActionInput(*this");
    const auto finalRoute = composite.find("Gen2PokemonEditor::handleFinalGen2SurfaceInput(*this");
    assert(actionsRoute != std::string::npos && finalRoute != std::string::npos && actionsRoute < finalRoute);
    has(composite, "return state.mode == decltype(state.mode)::None && !editorOverlayState(screen).active;");

    std::cout << "Gen II Actions -> Clone: shared carry, no dialog, no Actions reopen: PASS\n";
}
