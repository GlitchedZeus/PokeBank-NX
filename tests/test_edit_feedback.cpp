// Round 11 H22 (owner: "when i switch unowns i get a pop in middle bottom about stat changes and i
// dont need it ... i also get the pop up for rerolling ... can we fix this for all gens"): the one
// Gen I-V routine-edit toast rule (include/UI/EditFeedback.h) and its wiring into every
// generation's reroll / Unown letter / Shiny success path. Refusals, no-change outcomes and
// warnings keep their message. The runtime proof through the real screen is in
// tests/test_ui_unown_letter_host.cpp (Gen II) and tests/test_ui_editor_input_host.cpp (Gen III / V).
#include "UI/EditFeedback.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace F = PokeBank::UIModel::EditFeedback;

struct FakeScreen {
    std::vector<std::string> posted;
    void postStatus(const std::string& text, int) { posted.push_back(text); }
};

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
// The text from `from` up to (not including) `to`, both required.
std::string between(const std::string& text, const std::string& from, const std::string& to) {
    const auto a = text.find(from);
    const auto b = a == std::string::npos ? a : text.find(to, a + from.size());
    if (b == std::string::npos) std::cerr << "anchor missing (update this test with the code): " << from << " .. " << to << '\n';
    assert(a != std::string::npos && b != std::string::npos);
    return text.substr(a, b - a);
}
bool has(const std::string& text, const std::string& needle) { return text.find(needle) != std::string::npos; }
} // namespace

int main() {
    // The rule.
    for (const auto kind : {F::Kind::Reroll, F::Kind::UnownLetter, F::Kind::Shiny}) {
        assert(F::routine(kind));
        assert(!F::postsStatus(kind, F::Outcome::Applied));
        for (const auto kept : {F::Outcome::Unchanged, F::Outcome::Warning, F::Outcome::Refused})
            assert(F::postsStatus(kind, kept));
    }
    assert(!F::routine(F::Kind::Field));
    for (const auto any : {F::Outcome::Applied, F::Outcome::Unchanged, F::Outcome::Warning, F::Outcome::Refused})
        assert(F::postsStatus(F::Kind::Field, any));
    FakeScreen screen;
    assert(!F::report(screen, F::Kind::Reroll, F::Outcome::Applied, "re-rolled", 240) && screen.posted.empty());
    F::applied(screen, F::Kind::Shiny);
    assert(screen.posted.empty());
    assert(F::report(screen, F::Kind::Reroll, F::Outcome::Unchanged, "DVs unchanged", 300));
    assert(F::report(screen, F::Kind::Field, F::Outcome::Applied, "Nickname change staged", 240));
    assert(screen.posted == (std::vector<std::string>{"DVs unchanged", "Nickname change staged"}));

    // Gen I: Y reroll (Create and Edit) and the Shiny row (Edit commits through ux2CommitEdit).
    const auto g1 = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    const auto g1Reroll = between(g1, "const auto rolled = PokeBank::UIModel::Gen12DVReroll::reroll(", "void foundationEditIdentity(");
    assert(has(g1Reroll, "No other DVs keep this Pokemon shiny; DVs unchanged"));       // no-change still says why
    assert(!has(g1Reroll, "re-rolled in this draft") && !has(g1Reroll, "Randomized DVs set"));
    assert(has(g1Reroll, "EditFeedback::Kind::Reroll"));
    const auto g1Commit = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    const auto commit = between(g1Commit, "bool ux2CommitEdit(", "bool ux2StageAdd(");
    assert(has(commit, "EditFeedback::report(screen, kind, EditFeedback::Outcome::Applied"));
    assert(has(commit, "Generation I edit was rejected"));                               // refusals still post
    const auto g1Shiny = between(read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"), "bool ux3SetExistingShiny(",
                                 "\n}\n");
    assert(has(g1Shiny, "EditFeedback::Kind::Shiny") && !has(g1Shiny, "appearance staged"));

    // Gen II: Y reroll, an applied Unown letter, and the Shiny toggle.
    const auto g2 = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto apply = between(g2, "void applyUnownDVs(", "bool handleUnownQuestion(");
    assert(!has(apply, "postStatus") && has(apply, "EditFeedback::applied(screen, EditFeedback::Kind::UnownLetter)"));
    assert(has(between(g2, "bool handleUnownQuestion(", "bool handleUnownPicker("), "\"Unown unchanged\""));
    const auto y = between(g2, "if (down & HidNpadButton_Y) {\n        if (randomizeDVs(state))", "return true;\n    }\n");
    const auto ySuccess = between(y, "if (randomizeDVs(state))", "else if (rerollUnown)");
    assert(!has(ySuccess, "postStatus") && has(ySuccess, "EditFeedback::applied(screen, EditFeedback::Kind::Reroll)"));
    assert(has(y, "Gen2UnownLetter::rerollStatus(rerollBefore, rerollBefore, false)"));  // no-change explained
    assert(has(y, "DVs unchanged"));
    const auto g2f = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const auto toggle = between(g2f, "void toggleUnownShiny(", "bool randomizeDVs(");
    assert(!has(toggle, "\"Shiny off: ") && !has(toggle, "\"Shiny on: "));
    assert(has(toggle, "changed gender for this species ratio"));                       // warning kept
    assert(has(toggle, "can be shiny: letter "));                                       // letter change is a warning

    // Gen III and Gen IV / V: Y re-rolls IVs / EVs silently; the no-longer-shown text is gone.
    for (const char* path : {"src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc"}) {
        const auto s = read(path);
        const auto y35 = between(s, "namespace Reroll = PokeBank::UIModel::SharedStatReroll;", "return true;");
        assert(!has(y35, "postStatus") && has(y35, "EditFeedback::applied(screen, EditFeedback::Kind::Reroll)"));
        assert(!has(s, "kRerollStatus"));
    }
    assert(!has(read("include/UI/SharedStatReroll.h"), "kRerollStatus"));

    // Gen V Shiny (H29): an inline toggle like Gen I-IV, no "Change PID?" dialog, the same strict
    // PID re-verification as inline Gender (H08), silent on success; Review lists the PID change.
    const auto g5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto shinyRow = between(g5, "} else if (row == Fields::ShinyRow) {", "} else if (row == Fields::AbilityRow) {");
    assert(has(shinyRow, "Fields::toggleShiny(probe, error)") && !has(shinyRow, "offer("));
    assert(has(shinyRow, "verifiedPidProbe(screen, probe)") && has(shinyRow, "d = std::move(probe);"));
    assert(has(shinyRow, "EditFeedback::applied(screen, EditFeedback::Kind::Shiny)"));
    const auto genderRow = between(g5, "} else if (row == Fields::GenderRow) {", "} else if (row == Fields::ShinyRow) {");
    assert(has(genderRow, "verifiedPidProbe(screen, probe)") &&
           has(genderRow, "\" (nature, shininess and ability kept)\""));                  // H08 PASS text kept
    const auto verify = between(g5, "static std::optional<Record> verifiedPidProbe(", "\n    }\n");
    assert(has(verify, "pidLinksHold(*after, b2w2(screen))") &&
           has(verify, "The new PID failed strict re-verification; nothing was changed"));

    std::cout << "Gen I-V routine edits (reroll, Unown letter, Shiny) post no success toast; refusals, "
                 "no-change and warnings still do: PASS\n";
    return 0;
}
