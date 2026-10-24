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
    // Gen I / II modals and the classic Add Item picker (one-round foreign-file pass): every
    // picker, clone grid, review list, action popup and selector row is boxed. Editor field rows
    // (not modals) keep their own focus treatment.
    for (const Site site : {Site{"src/UI/Gen1PokemonEditorOverlayUX.inc", 2},
                            Site{"src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", 2},
                            Site{"src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", 2},
                            Site{"src/UI/Gen2HardwarePickerFix.inc", 1}, Site{"src/UI/Gen2PokemonPickerOverlay.inc", 1},
                            Site{"src/UI/Gen2SharedPokemonSurface.inc", 1}, Site{"src/UI/Gen2SharedSurfaceParity.inc", 1},
                            Site{"src/UI/Gen2UnifiedPokemonWorkspace.inc", 3},
                            Site{"src/UI/TrainerViewScreenGSCOverlay.inc", 1},
                            Site{"src/UI/ClassicInventoryOverlay.cpp", 1}}) {
        require(count(read(site.path), "SharedBoxedRows::drawBox(") >= site.boxes,
                std::string("R2-BUG-012 boxed modal rows in ") + site.path);
    }
    // Gen II's noninteractive Review rows now use the accepted shared panel;
    // its interactive Clone grid still requires its own boxed cells above.
    require(read("src/UI/Gen2SharedSurfaceParity.inc").find("drawSharedReviewSurface(screen,fb);")!=std::string::npos &&
            read("src/UI/Gen2SharedPokemonSurface.inc").find("SharedEditorChrome::drawReviewPanel(fb,screen.touchButtons,rows,")!=std::string::npos &&
            read("include/UI/SharedEditorChrome.h").find("fb.drawFilledRoundedRect(x+18,yy,w-36,94,10,Colors::PanelAlt);")!=std::string::npos,
            "Gen II Review delegates to the accepted shared boxed change rows");
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen2HardwarePickerFix.inc",
                             "src/UI/Gen2PokemonPickerOverlay.inc", "src/UI/Gen2SharedSurfaceParity.inc",
                             "src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/TrainerViewScreenGSCOverlay.inc",
                             "src/UI/ClassicInventoryOverlay.cpp"}) {
        require(read(path).find("drawSelectionHighlight") == std::string::npos,
                std::string("R2-BUG-012 no bare selection highlight left in ") + path);
    }
    require(read("src/UI/Gen2PokemonPickerOverlay.inc")
                    .find("SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(g.listX, yy, g.listWidth, g.rowHeight), {selected});") !=
                std::string::npos,
            "Crystal Met Location picker: box == its clipped touch row");

    // Box rect == touch rect at each converted site (same x, y, w, h expression).
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    require(shared.find("screen.touchButtons.push_back({41400 + i, x + 22, yy, w - 44, 42});") != std::string::npos &&
            shared.find("SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy, w - 44, 42), {selected});") !=
                std::string::npos,
            "Move contextual editor: each row boxed on its own touch rect");

    // Confirmations: every choice is a boxed glyph row that presses exactly its button.
    require(boxed.find("g_touchGlyphHits.push_back(hit);") != std::string::npos &&
            boxed.find("buttonGlyph(fb, gx, cell.y + cell.h / 2, choice.glyph, false);") != std::string::npos,
            "boxed confirm choices use 2R's glyph renderer and the shared glyph-hit path");
    for (const auto& [path, sites] : {std::pair{"src/UI/Gen3SharedPokemonSurface.inc", std::size_t{2}},
                                      std::pair{"src/UI/Gen4SharedPokemonSurface.inc", std::size_t{2}},
                                      std::pair{"src/UI/Gen5SharedReadOnlySurface.inc", std::size_t{1}}}) {
        std::string text = read(path);
        if (std::string(path)=="src/UI/Gen4SharedPokemonSurface.inc" ||
            std::string(path)=="src/UI/Gen3SharedPokemonSurface.inc") {
            require(text.find("SharedEditorChrome::drawReviewDiscardConfirm(fb,screen.touchButtons)")!=std::string::npos,
                    "Gen III/IV Discard delegates to the actual shared confirmation");
            const auto sharedChrome=read("include/UI/SharedEditorChrome.h");
            const auto begin=sharedChrome.find("inline void drawReviewDiscardConfirm(");
            const auto end=sharedChrome.find("\n}\n",begin);
            require(begin!=std::string::npos && end!=std::string::npos && end>begin,"shared Discard confirmation exists");
            if(begin!=std::string::npos && end!=std::string::npos && end>begin) {
                const auto helper=sharedChrome.substr(begin,end-begin);
                require(helper.find("{\"B\",\"Keep Changes\"},{\"Y\",\"Discard All\"}")!=std::string::npos,
                        "shared Discard keeps B cancellation and Y confirmation");
                text+=helper;
            }
        }
        require(count(text, "SharedBoxedRows::drawGlyphChoices(") >= sites,
                std::string("R2-BUG-012 Release / Discard / decision choices boxed in ") + path);
    }
    require(read("src/UI/Gen4SharedPokemonSurface.inc").find("{{{\"B\", \"Cancel\"}, {\"Y\", \"Confirm Release\"}}}") !=
                std::string::npos, "Release still commits on Y only (B cancels)");

    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return EXIT_FAILURE;
    }
    std::cout << "R2-BUG-012 shared boxed rows (reference style, Gen III-V editor lists and grids): PASS\n";
    return EXIT_SUCCESS;
}
