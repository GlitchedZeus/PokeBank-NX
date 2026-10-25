// Round 5 4D (owner, Oct 24): Gen I-V Help pages name only controls that perform the stated action
// in that generation, and never a vague "Tools". Each claim is tied to the code that handles it.
#include "UI/ClassicHelpPresentation.h"
#include "UI/ClassicStagedExit.h"
#include "UI/GlyphInstruction.h"

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
                // Gen V browse surfaces open Review on +; Gen I-III Bags open Inventory Options.
                const bool ok = gen == 5 || (gen <= 3 && page == Help::Page::Items &&
                                             line.find("Inventory Options: Review") != std::string::npos);
                if (!ok) std::cerr << "Gen " << gen << ": " << line << '\n';
                assert(ok);
            }
    // Gen I-IV say where Review really is.
    for (int gen = 1; gen <= 4; ++gen) {
        const auto pokemon = Help::lines(Help::Page::Pokemon, gen);
        assert(std::find(pokemon.begin(), pokemon.end(), std::string(Help::kReviewRoute)) != pokemon.end());
    }
    const auto gen4Items = Help::lines(Help::Page::Items, 4);
    assert(std::find(gen4Items.begin(), gen4Items.end(), std::string(Help::kReviewRoute)) != gen4Items.end());
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
    // Gen I-III Bag: + opens Inventory Options, whose first row is Review.
    const std::string bag = read("src/UI/ClassicInventoryOverlay.cpp");
    assert(bag.find("ClassicInventoryInput::Plus") != std::string::npos);
    assert(bag.find("\"Review Pending Changes (\"") != std::string::npos);
    // Gen I-IV: + in the game browser is not Review (it points to Product Home).
    assert(read("src/UI/TrainerViewScreenBase.inc").find("Current Game tools are on Product Home") != std::string::npos);
    // Trainer page: Y or the Review button opens Review (Gen I-IV shared session input).
    const std::string trainer = read("include/UI/ClassicTrainerSessionInput.h");
    assert(trainer.find("((down & HidNpadButton_Y) ||") != std::string::npos && trainer.find("review();") != std::string::npos);
    // Moving a Pokemon: Y places, B cancels.
    const std::string carry = read("src/UI/ClassicPackedMoveOverlay.inc");
    assert(carry.find("if (down & HidNpadButton_Y) return place(screen, state);") != std::string::npos);
    assert(carry.find("if (down & HidNpadButton_B) return cancel(screen, state);") != std::string::npos);
    std::cout << "Help claims match the input code ok\n";
}

// Round 6 E05 (owner, 1ca2faec): Game Browser Controls rows lacked coloured glyphs and Help repeated
// stale Review lines. Every row after the banner now starts with real keys (drawn as badges), each
// page names one Review route, and the new rows are tied to their handlers.
void testEveryRowDrawsGlyphsAndReviewIsNamedOnce() {
    namespace G = PokeBank::UIModel::GlyphInstruction;
    for (int gen = 1; gen <= 5; ++gen)
        for (const auto page : kPages) {
            const auto lines = Help::lines(page, gen);
            int reviewRoutes = 0;
            for (std::size_t i = 1; i < lines.size(); ++i) {
                const auto segments = G::helpLine(lines[i]);
                const bool glyph = !segments.empty() && segments.front().kind == G::Kind::Glyph;
                if (!glyph) std::cerr << "Gen " << gen << " row without a key badge: " << lines[i] << '\n';
                assert(glyph);
                assert(lines[i].find("Review: X Export") == std::string::npos);      // another screen's keys
                assert(lines[i].find("Available from the staged exit menu") == std::string::npos);
                const bool names = lines[i] == Help::kReviewRoute ||
                                   lines[i].rfind("+   Review", 0) == 0 ||
                                   lines[i].rfind("+   Inventory Options: Review", 0) == 0 ||
                                   lines[i].rfind("Y   Review", 0) == 0;
                reviewRoutes += names;
            }
            if (page != Help::Page::Hub) assert(reviewRoutes == 1);
            assert(lines.back() == Help::kCloseHelp);
        }
    // The Hub's exit-menu row matches ClassicStagedExit::choose; B (also a tap on 0) continues.
    namespace Exit = PokeBank::UIModel::ClassicStagedExit;
    static_assert(Exit::choose(true, false, false, false) == Exit::Action::Review);
    static_assert(Exit::choose(false, false, true, false) == Exit::Action::ExportCopy);
    static_assert(Exit::choose(false, false, false, true) == Exit::Action::DiscardExit);
    static_assert(Exit::choose(false, true, false, false) == Exit::Action::ContinueEditing);
    for (int gen = 1; gen <= 5; ++gen) {
        const auto hub = Help::lines(Help::Page::Hub, gen);
        assert(std::find(hub.begin(), hub.end(), std::string(Help::kExitMenu)) != hub.end());
    }
    const std::string base = read("src/UI/TrainerViewScreenBase.inc");
    assert(base.find("if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) helpOverlayActive = false;") != std::string::npos);
    assert(base.find("const auto action = Exit::choose(kDown & HidNpadButton_A, kDown & HidNpadButton_B,") != std::string::npos);
    assert(read("src/UI/Gen5SharedReadOnlySurface.inc").find("const auto action=Exit::choose(keys&HidNpadButton_A,keys&HidNpadButton_B,") != std::string::npos);
    std::cout << "every Help row draws key badges; one Review route per page; exit menu and close keys match ok\n";
}
} // namespace

int main() {
    testPlusOnlyWhereHandled();
    testEveryLineIsConcrete();
    testClaimsMatchTheCode();
    testEveryRowDrawsGlyphsAndReviewIsNamedOnce();
    std::cout << "test_classic_help_accuracy: all passed\n";
    return 0;
}
