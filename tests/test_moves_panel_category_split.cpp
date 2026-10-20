// M04 / M05 (Round 3 hardware): in the Gen IV / V editor Moves panel the per-move category crowded
// the Power / Accuracy section of each move's second line. Gen IV / V now keep the category with the
// type on the left and give "Pwr  Acc" their own right-aligned section; Gen I-III (no per-move
// category, M01-M03 owner PASS) keep the accepted single detail line byte for byte.
#include "UI/SharedMovesPanel.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace Panel = UI::SharedMovesPanel;
namespace Facts = PokeBank::UIModel::SharedMoveFacts;
namespace Geo = PokeBank::UIModel::SharedMenuGeometry;

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
} // namespace

int main() {
    using G = Enums::GameVersion;
    // The real right-side panel (SharedPokemonShell::Geometry(1280, 720, 56): rightX 750, rightW 506).
    constexpr auto layout = Panel::layout(750, 96, 1280 - 24 - 750);
    const int lineW = layout.statusRight() - layout.nameX();

    // Gen I-III: never split; the accepted detail line is unchanged.
    for (const auto game : {G::RBY, G::GSC, G::FRLG}) {
        for (uint16_t move = 1; move <= 354; ++move) {
            const auto f = Facts::facts(move, game);
            if (!f.exists) continue;
            check(!Panel::detailSplit(f).split, "Gen I-III move " + std::to_string(move) + " keeps the single line");
            check(Facts::detailLine(f).find("Pwr ") != std::string::npos, "Gen I-III detail line still has Pwr");
        }
    }
    // Gen IV / V: split, and both sections fit side by side without truncation.
    int splitMoves = 0;
    for (const auto game : {G::DP, G::PT, G::HGSS, G::BW, G::B2W2}) {
        for (uint16_t move = 1; move <= 559; ++move) {
            const auto f = Facts::facts(move, game);
            if (!f.exists) continue;
            const auto d = Panel::detailSplit(f);
            check(d.split, "Gen IV/V move " + std::to_string(move) + " splits its second line");
            if (!d.split) continue;
            ++splitMoves;
            check(d.left.find("Pwr") == std::string::npos && d.left.find("Acc") == std::string::npos,
                  "category stays with the type; Pwr / Acc leave the left part");
            check(d.right.rfind("Pwr ", 0) == 0 && d.right.find("   Acc ") != std::string::npos,
                  "Pwr / Acc are their own section");
            check(Geo::textWidth(d.left, Geo::Text::Caption) + Panel::kDetailSectionGap + d.rightW <= lineW,
                  "Gen IV/V move " + std::to_string(move) + ": type/category and Pwr/Acc fit apart in the panel");
        }
    }
    check(splitMoves > 900, "every Gen IV / V move checked");
    const auto thunderbolt = Panel::detailSplit(Facts::facts(85, G::HGSS));
    check(thunderbolt.left == "Electric \xE2\x80\xA2 Special" && thunderbolt.right == "Pwr 95   Acc 100",
          "HGSS Thunderbolt: \"Electric • Special\" | \"Pwr 95   Acc 100\"");

    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "M04/M05 Gen IV/V Moves panel category split (Gen I-III unchanged, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
