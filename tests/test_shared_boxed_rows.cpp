// R2-BUG-012: one boxed-row style for every interactive Gen I-V modal (reference: the Pokemon
// Actions popup, ScreenChrome drawNarrowPokemonActions).
//   * SharedBoxedRows' style numbers are the reference popup's (radius, borders, yellow focus,
//     pressed tint, label inset), read from the real ScreenChrome source;
//   * every interactive list/grid in the Gen III-V editor surfaces draws focus through
//     SharedBoxedRows::drawBox (no bare selection highlight left), on the same rect it publishes
//     as the touch target.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

int failures = 0;
void require(bool ok, const std::string& what) {
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

std::size_t count(const std::string& text, const std::string& needle) {
    std::size_t n = 0;
    for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) ++n;
    return n;
}

} // namespace

int main() {
    const std::string boxed = read("include/UI/SharedBoxedRows.h");
    const std::string chrome = read("include/UI/ScreenChrome.h");

    // The reference popup's numbers.
    const auto narrow = chrome.substr(chrome.find("inline void drawNarrowPokemonActions("));
    require(narrow.find("g.rowHeight - 6, 10,") != std::string::npos, "reference row radius 10");
    require(narrow.find("Colors::PartyNumber") != std::string::npos, "reference focus colour is the yellow PartyNumber");
    require(narrow.find("selected || pressed ? 2 : 1") != std::string::npos, "reference 2 px focus / 1 px border");
    require(narrow.find("withAlpha(narrowSelectionBorder, 112)") != std::string::npos, "reference pressed tint 112");
    require(narrow.find("fb.drawText(xx + 18,") != std::string::npos, "reference label inset 18");
    require(boxed.find("kRadius = 10;") != std::string::npos && boxed.find("kBorder = 1;") != std::string::npos &&
            boxed.find("kFocusBorder = 2;") != std::string::npos && boxed.find("kLabelInset = 18;") != std::string::npos &&
            boxed.find("kPressedAlpha = 112;") != std::string::npos,
            "SharedBoxedRows uses the reference popup's style numbers");
    require(boxed.find("outline ? Colors::PartyNumber : Colors::Divider") != std::string::npos &&
            boxed.find(": Colors::Surface);") != std::string::npos,
            "boxed rows: Surface fill, Divider border, yellow focus");
    require(boxed.find("touchButtons.push_back({touchId, cell.x, cell.y, cell.w, cell.h});") != std::string::npos,
            "the published touch rect IS the box");

    // Every interactive list / grid in the Gen III-V editors is boxed.
    struct Site { const char* path; std::size_t boxes; };
    for (const Site site : {Site{"src/UI/SharedPokemonEditorSurface.inc", 3}, Site{"src/UI/Gen3SharedPokemonSurface.inc", 3},
                            Site{"src/UI/Gen5SharedReadOnlySurface.inc", 1},
                            Site{"include/UI/SharedHeldItemPickerPresentation.h", 1}}) {
        const std::string text = read(site.path);
        require(count(text, "SharedBoxedRows::drawBox(") >= site.boxes,
                std::string("R2-BUG-012 boxed rows in ") + site.path);
        require(text.find("drawSelectionHighlight") == std::string::npos,
                std::string("R2-BUG-012 no bare selection highlight left in ") + site.path);
    }
    // Box rect == touch rect at each converted site (same x, y, w, h expression).
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    require(shared.find("screen.touchButtons.push_back({41400 + i, x + 22, yy, w - 44, 42});") != std::string::npos &&
            shared.find("SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy, w - 44, 42), {selected});") !=
                std::string::npos,
            "Move contextual editor: each row boxed on its own touch rect");

    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "R2-BUG-012 shared boxed rows (reference style, Gen III-V editor lists and grids): PASS\n";
    return EXIT_SUCCESS;
}
