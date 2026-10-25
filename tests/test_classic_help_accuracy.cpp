// Round 5 4D (owner, Oct 24): Gen I-V Help pages name only controls that perform the stated action
// in that generation, and never a vague "Tools". Each claim is tied to the code that handles it.
#include "UI/ClassicHelpPresentation.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace Help = PokeBank::UIModel::ClassicHelp;

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
constexpr Help::Page kPages[] = {Help::Page::Hub, Help::Page::Pokemon, Help::Page::Party, Help::Page::Items,
                                 Help::Page::Trainer};

void testPlusOnlyWhereHandled() {
    for (int gen = 1; gen <= 5; ++gen)
        for (const auto page : kPages)
            for (const auto& line : Help::lines(page, gen)) {
                if (line.rfind("+   ", 0) != 0) continue;
                // Round 6: + opens Review on Boxes / Party / Bag in every generation and on Gen V's
                // hub; the Gen I-IV hub reaches it through the staged exit menu instead.
                const bool ok = (gen == 5 || page != Help::Page::Hub) &&
                                line.find("Review") != std::string::npos;
                if (!ok) std::cerr << "Gen " << gen << ": " << line << '\n';
                assert(ok);
            }
    // Every editor page says how to reach Review in every generation.
    for (int gen = 1; gen <= 5; ++gen)
        for (const auto page : {Help::Page::Pokemon, Help::Page::Party, Help::Page::Items, Help::Page::Trainer}) {
            const auto lines = Help::lines(page, gen);
            assert(std::any_of(lines.begin(), lines.end(), [](const std::string& line) {
                return line.find("Review staged changes") != std::string::npos ||
                       line.find("Review every staged change") != std::string::npos;
            }));
        }
    std::cout << "+ named only where it acts ok\n";
}

void testEveryLineIsConcrete() {
    for (int gen = 1; gen <= 5; ++gen)
        for (const auto page : kPages) {
            const auto lines = Help::lines(page, gen);
            assert(lines.size() >= 6 && lines.size() <= 10);
            for (const auto& line : lines) {
                assert(lower(line).find("tools") == std::string::npos);   // no vague "Tools"
                assert(line.find("on-screen Move controls") == std::string::npos);
                const auto gap = line.find("   ");
                if (gap != std::string::npos) assert(line.find_first_not_of(' ', gap) != std::string::npos);
            }
        }
    std::cout << "every Help line names a concrete action ok\n";
}

void testClaimsMatchTheCode() {
    // Gen V: + opens Review from the browse surfaces.
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    assert(gen5.find("if(keys&HidNpadButton_Plus) {\n        if(!state.openReviewFromBrowse(") != std::string::npos);
    // Gen I-III Bag: + opens the shared Review directly (Round 6, B08).
    const std::string bag = read("src/UI/ClassicInventoryOverlay.cpp");
    assert(bag.find("ClassicInventoryInput::Plus") != std::string::npos);
    assert(bag.find("if (screen.openStagedReview(false)) return true;") != std::string::npos);
    // Gen IV Bag: + opens Review; Gen I-IV Party (Y or +) and Boxes (+) open it from the grid.
    assert(read("src/UI/Gen4SharedPokemonSurface.inc").find("ReviewEntry::Screen::Bag,4") != std::string::npos);
    assert(read("src/UI/TrainerViewScreenCompositeOverlay.cpp").find("if (openClassicBrowseReview(*this, down)) return;") !=
           std::string::npos);
    // Gen I-IV: + in the game browser is not Review (it points to Product Home).
    assert(read("src/UI/TrainerViewScreenBase.inc").find("Current Game tools are on Product Home") != std::string::npos);
    // Trainer page: Y or the Review button opens Review (Gen I-IV shared session input).
    const std::string trainer = read("include/UI/ClassicTrainerSessionInput.h");
    assert(trainer.find("((down & (HidNpadButton_Y | HidNpadButton_Plus)) ||") != std::string::npos &&
           trainer.find("review();") != std::string::npos);
    // Moving a Pokemon: Y places, B cancels.
    const std::string carry = read("src/UI/ClassicPackedMoveOverlay.inc");
    assert(carry.find("if (down & HidNpadButton_Y) return place(screen, state);") != std::string::npos);
    assert(carry.find("if (down & HidNpadButton_B) return cancel(screen, state);") != std::string::npos);
    std::cout << "Help claims match the input code ok\n";
}
} // namespace

int main() {
    testPlusOnlyWhereHandled();
    testEveryLineIsConcrete();
    testClaimsMatchTheCode();
    std::cout << "test_classic_help_accuracy: all passed\n";
    return 0;
}
