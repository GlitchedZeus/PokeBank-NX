// Round 14 (owner R09, Round 13 hardware on 9c301c48, verbatim: "i didnt need to add a met location
// to ADD it to the box, which is wrong, met location should always be a part od the stuff before a
// pokemon is added, maybe this is cause its gen 2 but pokemon crystal").
//
// ONE shared Create gate (UI/CreateRequiredFields.h) for every Gen I-V game:
//  - every game that stores met / caught data (Crystal, all Gen III, IV, V games) refuses Create /
//    Add until a Met Location was CHOSEN;
//  - Red / Blue / Yellow and Gold / Silver store no met data, so they never ask (stated, not invented);
//  - "no caught data" stays native for an EXISTING (e.g. traded) Crystal Pokemon: Edit never asks.
// Then the real Gen II session (UI/Gen2PokemonSession.h, the shared Gen II editor's model) on
// synthetic Gold / Silver / Crystal saves: Crystal Add is refused until the Met Location picker
// reports a choice, a species change that clears the chosen location asks again, and Gold / Silver
// add without one. Source contracts pin every Gen II Add entry to the exact-game rule.
#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/CreateRequiredFields.h"
#include "UI/Gen2PokemonSession.h"

#include <fstream>
#include <sstream>
#include <string_view>

namespace Required = PokeBank::UIModel::CreateRequiredFields;
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Native = PokeBank::UIModel::Gen2Native;
using Rules::Session;
using Rules::SessionMode;

namespace {
int failures = 0, checks = 0;
void require(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
std::string read(const char* path) {
    std::ifstream in(path);
    std::stringstream s;
    s << in.rdbuf();
    return s.str();
}
std::string functionBody(const std::string& text, const std::string& signature) {
    const auto start = text.find(signature);
    if (start == std::string::npos) return {};
    const auto open = text.find('{', start);
    int depth = 0;
    for (size_t i = open; i < text.size(); ++i) {
        if (text[i] == '{') ++depth;
        else if (text[i] == '}' && --depth == 0) return text.substr(start, i - start + 1);
    }
    return {};
}

struct GameRow { const char* id; int generation; bool storesMetData; };
// Every Gen I-V game id the app knows (Games/GameIdentity.cpp). storesMetData is the native format
// fact: Gen I has no met field, Gold/Silver have none, Crystal stores caught data, Gen III-V store
// Met Location in every game.
constexpr GameRow kGames[] = {
    {"red_gb", 1, false}, {"blue_gb", 1, false}, {"yellow_gb", 1, false},
    {"gold_gbc", 2, false}, {"silver_gbc", 2, false}, {"crystal_gbc", 2, true},
    {"ruby_gba", 3, true}, {"sapphire_gba", 3, true}, {"emerald_gba", 3, true},
    {"firered_gba", 3, true}, {"leafgreen_gba", 3, true}, {"firered_switch", 3, true},
    {"leafgreen_switch", 3, true},
    {"diamond_nds", 4, true}, {"pearl_nds", 4, true}, {"platinum_nds", 4, true},
    {"heartgold_nds", 4, true}, {"soulsilver_nds", 4, true},
    {"black_nds", 5, true}, {"white_nds", 5, true}, {"black2_nds", 5, true}, {"white2_nds", 5, true},
};

void sharedRuleEveryGame() {
    const std::array<uint16_t, 4> none{};
    const std::array<uint16_t, 4> one{33, 0, 0, 0};
    for (const auto& game : kGames) {
        const std::string id = game.id;
        require(Required::metLocationRequiredForGame(game.generation, id) == game.storesMetData,
                id + ": Met Location required exactly when the game stores met data");
        const auto blank = Required::forCreateInGame(game.generation, id, false, one);
        require(blank.metLocation == game.storesMetData,
                id + ": Create with a move but no chosen Met Location " +
                    (game.storesMetData ? "is refused" : "is allowed (no met data in this game)"));
        require(!Required::forCreateInGame(game.generation, id, true, one).any(),
                id + ": chosen Met Location and one move: nothing missing");
        require(Required::forCreateInGame(game.generation, id, true, none).move,
                id + ": a move is still required");
        if (game.storesMetData)
            require(Required::message(blank, "Add") == "Choose a Met Location before Add",
                    id + ": one message asks for the choice");
    }
}

void gen2Session(const L& layout, SourceGame game) {
    const std::string_view sourceId = game == SourceGame::Gold ? "gold_gbc"
                                    : game == SourceGame::Silver ? "silver_gbc" : "crystal_gbc";
    const bool crystal = game == SourceGame::Crystal;
    const std::string name(sourceId);
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    require(static_cast<bool>(parsed), name + ": synthetic save parses");
    if (!parsed) return;
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    require(static_cast<bool>(editor), name + ": staged editor");
    if (!editor) return;
    auto entry = *editor->boxedPokemon(0, 0, error);

    // A fresh Create draft: one usable Pikachu move (ThunderShock), no caught data.
    PokemonRecord fresh = entry;
    fresh.moves = {84, 0, 0, 0};
    fresh.pp = {30, 0, 0, 0};
    fresh.ppUps = {};
    fresh.caughtData = 0;
    {
        Session s;
        s.begin(fresh, SessionMode::Create);
        const auto missing = s.missingRequiredFields(game);
        require(missing == (crystal ? std::string("Choose a Met Location before Add") : std::string()),
                name + ": Create without a Met Location -> \"" + missing + "\"");
        size_t slot = 0;
        error.clear();
        const bool added = s.add(*editor, 1, slot, error);
        require(added == !crystal, name + ": Add " + (crystal ? "refused" : "allowed") +
                                      " without a chosen Met Location (" + error + ")");
        if (crystal)
            require(error == "Choose a Met Location before Add", name + ": the refusal says why: " + error);
    }
    if (crystal) {
        // The picker reports the choice; Add then stages, and the chosen caught data is what is staged.
        Session s;
        s.begin(fresh, SessionMode::Create);
        Native::CrystalCaughtData caught;
        caught.present = true; caught.timeOfDay = 2; caught.levelCode = 5; caught.location = 50;
        s.working.caughtData = Native::encodeCrystalCaughtData(caught);
        require(!s.missingRequiredFields(game).empty(),
                "Crystal: a value alone is not a choice (only the Met Location picker marks it chosen)");
        s.noteMetLocationChosen();
        require(s.missingRequiredFields(game).empty(), "Crystal: chosen Met Location -> nothing missing");
        size_t slot = 0;
        error.clear();
        require(s.add(*editor, 1, slot, error), "Crystal: Add after choosing a Met Location (" + error + ")");
        const auto staged = editor->boxedPokemon(1, slot, error);
        require(staged && staged->caughtData == Native::encodeCrystalCaughtData(caught),
                "Crystal: staged caught data is the chosen one");

        // A species change that clears the chosen location asks for a new choice again.
        Session change;
        change.begin(fresh, SessionMode::Create);
        change.working.caughtData = Native::encodeCrystalCaughtData(caught);
        change.noteMetLocationChosen();
        require(change.setSpecies(24, sourceId), "Crystal: Pikachu -> Arbok");
        if (change.working.caughtData == 0) {
            require(!change.metLocationChosen, "Crystal: a cleared location is no longer chosen");
            require(change.speciesChangeResetStatus(sourceId).find("Met Location needs a new choice") != std::string::npos,
                    "Crystal: the reset status asks for a new choice: " + change.speciesChangeResetStatus(sourceId));
            change.setMove(0, 40);  // Poison Sting: a move Arbok can use
            require(change.missingRequiredFields(game) == "Choose a Met Location before Add",
                    "Crystal: Add refused again after the reset");
        }

        // An EXISTING traded Crystal Pokemon with no caught data: Edit never asks for one.
        Session edit;
        PokemonRecord traded = entry;
        traded.moves = {84, 0, 0, 0};
        traded.pp = {30, 0, 0, 0};
        traded.caughtData = 0;
        edit.begin(traded, SessionMode::Edit);
        edit.working.heldItem = 1;
        require(edit.missingRequiredFields(game).empty(),
                "Crystal Edit: existing Pokemon without caught data is never refused for it");
    } else {
        Session s;
        s.begin(fresh, SessionMode::Create);
        require(s.setSpecies(24, sourceId), name + ": species change");
        require(s.speciesChangeResetStatus(sourceId).find("needs a new choice") == std::string::npos,
                name + ": Gold / Silver never ask for a Met Location");
    }
}

void sourceContracts() {
    const auto picker = read("src/UI/Gen2PokemonPickerOverlay.inc");
    const auto locationBranch = picker.substr(picker.find("picker.model.kind == Gen2Picker::Kind::Location"));
    size_t notes = 0;
    for (size_t at = locationBranch.find("noteMetLocationChosen()"); at != std::string::npos;
         at = locationBranch.find("noteMetLocationChosen()", at + 1)) ++notes;
    require(notes >= 2, "Gen II picker: both the verified-row and the other-landmark choice mark the Met Location chosen");

    const auto foundation = functionBody(read("src/UI/Gen2PokemonEditorFoundation.inc"), "bool stageCreate(TrainerViewScreen& screen)");
    require(foundation.find("state.missingRequiredFields(Rules::exactGameFromSourceId(screen.sourceGameId))") != std::string::npos,
            "Gen II shared editor Add asks the exact-game rule");
    const auto session = read("include/UI/Gen2PokemonSession.h");
    const auto add = functionBody(session, "bool add(Gen2::StagedEditor& editor,size_t box,size_t& slot,std::string& error)");
    require(add.find("missingRequiredFields(editor.metadata().sourceGame)") < add.find("stageAddBoxPokemon"),
            "Gen II session add(): the exact-game rule runs before staging");
    const auto legacy = functionBody(read("src/UI/TrainerViewScreenGSCOverlay.inc"),
                                     "bool stageAddPokemon(TrainerViewScreen& screen, int box)");
    require(legacy.find("CreateRequiredFields::forCreateInGame(2, screen.sourceGameId") != std::string::npos &&
                legacy.find("CreateRequiredFields::forCreateInGame(2, screen.sourceGameId") < legacy.find("stageAddBoxPokemon("),
            "Gen II legacy prompt Add asks the exact-game rule before staging");
}
} // namespace

int main() {
    sharedRuleEveryGame();
    gen2Session(GS, SourceGame::Gold);
    gen2Session(GS, SourceGame::Silver);
    gen2Session(C, SourceGame::Crystal);
    sourceContracts();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "R09 Create Met Location gate, every Gen I-V game + Gen II session (" << checks
              << " checks): PASS\n";
    return EXIT_SUCCESS;
}
