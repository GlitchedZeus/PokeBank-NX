// 6A (owner hardware, Oct 24): touching the Gen IV Pokerus field must do what A does.
//   * Pokerus is Details row 14 of 16; the Details window shows 8 rows, so the field is often only
//     partly visible below the window and its clipped touch target is still published;
//   * a completed tap on that row focuses it AND activates the inline toggle (it used to only
//     scroll and ask for "press A again");
//   * a controller A on a field scrolled out of sight still reveals it first without editing.
#include "UI/SharedPokemonEditorWorkspace.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "missing " << path << '\n';
        std::exit(EXIT_FAILURE);
    }
    return {std::istreambuf_iterator<char>(in), {}};
}

std::string between(const std::string& text, const std::string& from, const std::string& to) {
    const auto a = text.find(from);
    if (a == std::string::npos) return {};
    const auto b = text.find(to, a + from.size());
    return text.substr(a, b == std::string::npos ? std::string::npos : b - a);
}

} // namespace

int main() {
    // The geometry that puts Pokerus outside the first Details window.
    constexpr int kPokerusRow = 14;
    static_assert(Workspace::kVisibleDetailRows == 8);
    static_assert(kPokerusRow >= Workspace::kVisibleDetailRows);
    Shared::Focus focus{};
    check(Workspace::focusForTouch(Workspace::kDetailTouchBase + kPokerusRow, 16, focus) &&
              focus.panel == Shared::Panel::Details && focus.row == kPokerusRow,
          "a tap on the Pokerus row id focuses Details row 14");

    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    check(gen4.find("\"Met Level\", \"Met Location\", \"Pokerus\", \"Form\"") != std::string::npos,
          "Gen IV Details: Pokerus is row 14");
    check(between(gen4, "bool detailEditable(", "\n}\n").find("case 14: // Pokerus") != std::string::npos,
          "Gen IV Pokerus is an editable Details field");
    check(between(gen4, "    if (row == 14) {", "    if (row == 15) {").find("nextPokerusByte(") != std::string::npos,
          "A on Pokerus runs the inline None / Infected / Cured toggle");

    const auto shared = read("src/UI/SharedPokemonEditorSurface.inc");
    const auto handler = between(shared, "bool handleWorkspace(TrainerViewScreen& screen, EditorState& state, u64 down, const TouchInput& touch) {",
                                 "\ntemplate <class A>");
    check(handler.find("appendClippedTouchButton") == std::string::npos, "handler body found");
    const auto tap = between(handler, "bool tappedDetail = false;", "if (touch.justReleased() && touch.dragged())");
    check(tap.find("tappedDetail = true;") != std::string::npos && tap.find("if (editable) down |= HidNpadButton_A;") != std::string::npos,
          "a completed Details tap is remembered and pressed as A");
    const auto guard = between(handler, "!state.detailsViewport.containsSelection(state.focus.row, rows, visibleDetailRows)) {",
                               "const auto activation = Workspace::activationFor(state.focus);");
    check(guard.find("state.detailsViewport.reveal(state.focus.row, rows, visibleDetailRows);") != std::string::npos,
          "the partly visible row is revealed");
    check(guard.find("if (!tappedDetail) {") != std::string::npos &&
              guard.find("\"Focused field brought into view; press A again\"") != std::string::npos,
          "only a controller A stops at the reveal; a tap continues to the same activation as A");
    const auto draw = between(shared, "appendClippedTouchButton(screen.touchButtons, 41000 + row,", ";");
    check(!draw.empty(), "Details rows publish clipped touch targets");
    check(shared.find("firstDetailRow + static_cast<int>(detailVisibleRows) + 1); ++row) {") != std::string::npos,
          "the partly visible row below the window keeps its touch target");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "6A Gen IV Pokerus touch = A (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
