// Generation II species change and Create moves / history: Gold, Silver, Crystal through the real
// Session::setSpecies (the one function both Gen II species-apply paths call) and the real
// StagedEditor, ending in a strict export -> reparse. Round 11 (owner H28 / Extra 5 / Extra 6 / H25)
// replaces the round 3 R2-BUG-004 refill and the Oct 26 seeded Create moves (C14): a new Create
// record starts with no moves; a species change keeps only the moves the new species can use in
// this exact game, with their PP and PP Ups, and never adds one; a Create draft's Pokerus and a
// no-longer-valid Crystal caught location reset, while an existing Pokemon keeps both; Add refuses
// a moveless Pokemon. The same species changes nothing; Discard restores; only the edited box's
// bytes change in the export.
#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
using Rules::Session;
using Rules::SessionMode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Every kept move came from `before` with its own PP / PP Ups, is usable by the new species, and
// the empty slots are at the end with PP 0 / PP Ups 0.
void checkKeptMoves(const PokemonRecord& before, const PokemonRecord& after, SourceGame game,
                    const std::string& what) {
    bool empty = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto move = after.moves[i];
        if (move == 0) {
            empty = true;
            require(after.pp[i] == 0 && after.ppUps[i] == 0, what + ": empty slot PP 0 / PP Ups 0");
            continue;
        }
        require(!empty, what + ": kept moves move up");
        const auto at = std::find(before.moves.begin(), before.moves.end(), move);
        require(at != before.moves.end(), what + ": no move is ever added");
        const auto from = static_cast<std::size_t>(at - before.moves.begin());
        require(after.pp[i] == before.pp[from] && after.ppUps[i] == before.ppUps[from],
                what + ": PP and PP Ups travel with the move");
        require(MoveCompatibility::canLearnMove(game, after.species, move), what + ": kept move is compatible");
    }
    for (const auto move : before.moves)
        if (move && MoveCompatibility::canLearnMove(game, after.species, move))
            require(std::find(after.moves.begin(), after.moves.end(), move) != after.moves.end(),
                    what + ": a compatible move is kept");
}

void run(const L& layout, SourceGame game, std::string_view sourceId) {
    const bool crystal = game == SourceGame::Crystal;
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;

    // Box 1 slot 0 is a non-current box, so its export bytes are isolated.
    auto baseEditor = StagedEditor::create(*parsed.save, error);
    require(static_cast<bool>(baseEditor), "editor");
    const auto normalized = baseEditor->finalizedBytes(error);
    auto entry = *baseEditor->boxedPokemon(1, 0, error);
    require(entry.species == 25, "fixture Pikachu");

    // Transitions with different learnsets, Edit and Create sessions.
    const uint16_t chain[] = {26, 22, 1, 152, 133, 22, 249, 25};
    for (const auto mode : {SessionMode::Edit, SessionMode::Create}) {
        Session s;
        s.begin(entry, mode);
        s.working.moves = {84, 45, 98, 0};  // Thunder Shock, Growl, Quick Attack
        s.working.pp = {30, 40, 30, 0};
        s.working.ppUps = {3, 2, 1, 0};
        for (const uint16_t species : chain) {
            const auto before = s.working;
            require(s.setSpecies(species, sourceId), "setSpecies " + std::to_string(species));
            require(s.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, species),
                    "level policy unchanged");
            checkKeptMoves(before, s.working, game, std::string(sourceId) + " -> " + std::to_string(species));
            for (const auto move : s.working.moves)
                require(move != 84 || species == 25 || species == 26, "Thunder Shock never survives a change away from Pikachu");
            // Same species: nothing changes, even after hand-edited moves.
            require(s.setMove(3, 33), "hand-edit slot 4");
            const auto again = s.working;
            require(s.setSpecies(species, sourceId), "reselect");
            require(Rules::sameEditableRecord(again, s.working), "same species changes nothing");
        }
        // Unknown game: nothing is provably compatible, so the slots are cleared -- never kept blindly.
        Session unknown;
        unknown.begin(entry, mode);
        unknown.working.moves = {84, 45, 0, 0};
        require(unknown.setSpecies(22, "not_a_gen2_game"), "unknown-game species change");
        require(unknown.working.moves == (std::array<uint8_t, 4>{}) && unknown.working.pp == (std::array<uint8_t, 4>{}),
                "no data: empty slots");
    }

    // Round 11 (owner H28 / Extra 5 / Extra 6; C14 reversed by the owner): a new Create record
    // starts with NO moves and a species change does not fill any in. Session::add refuses until a
    // move is chosen, staging nothing; then the staged record and Create Review carry the choice.
    {
        auto createEditor = StagedEditor::create(*parsed.save, error);
        require(static_cast<bool>(createEditor), "create editor");
        const auto beforeBytes = createEditor->finalizedBytes(error);
        PokemonRecord fresh = entry;
        fresh.level = 5;
        fresh.moves = {};
        fresh.pp = {};
        fresh.ppUps = {};
        fresh.pokerus = 0;
        fresh.caughtData = 0;
        Session create;
        create.begin(fresh, SessionMode::Create);
        require(create.setSpecies(24, sourceId) && create.working.moves == (std::array<uint8_t, 4>{}),
                "blank Create stays blank after a species change");
        require(create.setSpecies(25, sourceId) && create.working.moves == (std::array<uint8_t, 4>{}) &&
                create.working.pp == (std::array<uint8_t, 4>{}), "Create starts with no moves (C14 reversed)");
        require(create.setLevel(5), "Create level 5");
        std::size_t slot = 99;
        require(!create.add(*createEditor, 2, slot, error) && error == "Choose at least one move before Add",
                "Add refuses a moveless Pokemon with the shared message: " + error);
        require(createEditor->finalizedBytes(error) == beforeBytes && !createEditor->hasPendingChanges(),
                "a refused Add stages nothing");
        require(create.mode == SessionMode::Create, "the draft stays open");
        require(create.setMove(0, 84) && create.setMove(1, 45), "user picks Thunder Shock, Growl");
        require(create.add(*createEditor, 2, slot, error), "Create add: " + error);
        const auto added = createEditor->boxedPokemon(2, slot, error);
        require(added && added->species == 25 && added->level == 5 &&
                added->moves == (std::array<uint8_t, 4>{84, 45, 0, 0}) &&
                added->pp == (std::array<uint8_t, 4>{30, 40, 0, 0}), "staged Create carries the chosen moves");
        bool movesRow = false;
        for (const auto& change : createEditor->pendingChanges())
            movesRow |= change.label == "Box 3 Slot " + std::to_string(slot + 1) + " / Moves" &&
                        change.afterValue == "Thunder Shock, Growl";
        require(movesRow, "Create Review shows a Moves row");
    }

    // H25: a Create draft's Pokerus and a Crystal caught location that is no encounter of the new
    // species reset; a still-valid one stays; an existing Pokemon keeps both.
    {
        namespace Native = PokeBank::UIModel::Gen2Native;
        Native::CrystalCaughtData caught;
        caught.present = true; caught.timeOfDay = 2; caught.levelCode = 16; caught.location = 50;
        const uint16_t caughtRaw = Native::encodeCrystalCaughtData(caught);  // a Crystal Pikachu location
        for (const auto mode : {SessionMode::Create, SessionMode::Edit}) {
            PokemonRecord p = entry;
            p.moves = {84, 45, 0, 0};
            p.pp = {30, 40, 0, 0};
            p.pokerus = 0x21;
            p.caughtData = caughtRaw;
            Session s;
            s.begin(p, mode);
            require(s.setSpecies(24, sourceId), "Pikachu -> Arbok");
            const bool create = mode == SessionMode::Create;
            require(s.working.pokerus == (create ? 0 : 0x21), "Pokerus resets only for a Create draft");
            const bool locationValidForArbok = false;  // no Crystal template puts Arbok at location 50
            require(s.working.caughtData == (create && !locationValidForArbok ? 0 : caughtRaw),
                    "Create clears a location Arbok was never met at; Edit keeps its recorded history");
        }
        if (crystal) {
            // Ekans -> Arbok at location 52, where Crystal has both: the caught data stays.
            Native::CrystalCaughtData both = caught;
            both.location = 52;
            PokemonRecord p = entry;
            p.species = 23;
            p.moves = {84, 45, 0, 0};
            p.pokerus = 0x21;
            p.caughtData = Native::encodeCrystalCaughtData(both);
            Session s;
            s.begin(p, SessionMode::Create);
            require(s.setSpecies(24, sourceId) && s.working.caughtData == p.caughtData && s.working.pokerus == 0,
                    "Create: a location still valid for Arbok is kept; Pokerus still resets");
        }
    }

    // An existing Pokemon whose species change leaves no usable move cannot be kept moveless.
    {
        auto editEditor = StagedEditor::create(*parsed.save, error);
        PokemonRecord p = entry;
        p.moves = {84, 0, 0, 0};
        p.pp = {30, 0, 0, 0};
        Session s;
        s.begin(p, SessionMode::Edit);
        require(s.setSpecies(129, sourceId) && s.working.moves == (std::array<uint8_t, 4>{}),
                "Magikarp cannot use Thunder Shock");
        require(!s.keep(*editEditor, 1, 0, error) && error == "Choose at least one move before Keep",
                "Keep refused with the shared message: " + error);
        require(!editEditor->hasPendingChanges(), "nothing staged");
    }

    // Pikachu -> Fearow, Keep, Review, export -> strict reparse; only box 1 bytes move.
    auto editor = StagedEditor::create(*parsed.save, error);
    Session s;
    PokemonRecord taught = entry;
    taught.moves = {84, 45, 98, 0};
    taught.pp = {30, 40, 30, 0};
    taught.ppUps = {1, 2, 3, 0};
    s.begin(taught, SessionMode::Edit);
    require(s.setSpecies(22, sourceId), "Pikachu -> Fearow");
    require(std::find(s.working.moves.begin(), s.working.moves.end(), 84) == s.working.moves.end() &&
            s.working.moves[0] == 45, "Fearow: Thunder Shock cleared, Growl kept");
    const auto fearow = s.working;
    require(s.keep(*editor, 1, 0, error), "keep: " + error);
    require(!editor->pendingChanges().empty(), "Review lists the change");
    const auto staged = editor->boxedPokemon(1, 0, error);
    require(staged && staged->species == 22 && staged->moves == fearow.moves && staged->pp == fearow.pp &&
            staged->ppUps == fearow.ppUps, "staged record carries the new moves");
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), "export");
    require(differsOnlyInRange(normalized, out, boxStart(1), boxStart(1) + listLen(20, 32, 11)),
            "no byte outside the edited box changes");
    auto reloaded = parse(out, game);
    require(static_cast<bool>(reloaded), "strict reparse");
    auto check = StagedEditor::create(*reloaded.save, error);
    const auto back = check->boxedPokemon(1, 0, error);
    require(back && back->species == 22 && back->moves == fearow.moves && back->pp == fearow.pp &&
            back->ppUps == fearow.ppUps, "reparsed Fearow has its kept moves");
    checkKeptMoves(taught, *back, game, "reparsed");
    const auto other = check->boxedPokemon(0, 0, error);
    require(other && other->species == 25, "the other box is untouched");
    require(raw == fixture(layout, true), "source bytes never change");

    // Discard restores the original record exactly.
    editor->discard();
    require(!editor->hasPendingChanges(), "Discard clears Review");
    require(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), editor->originalBytes().begin()),
            "Discard restores every byte");
}

// The UI's Create entry is a TrainerViewScreen member path (no host harness): pin that it no longer
// seeds moves (Round 11) and that its Add asks the session's shared required-fields check.
void createEntryStartsBlank() {
    std::ifstream in("src/UI/Gen2PokemonEditorFoundation.inc", std::ios::binary);
    const std::string src{std::istreambuf_iterator<char>(in), {}};
    const auto begin = src.find("bool beginCreate(TrainerViewScreen& screen, int box) {");
    require(begin != std::string::npos, "beginCreate found");
    const auto body = src.substr(begin, src.find("\n}\n", begin) - begin);
    require(body.find("state.working = defaultCreateRecord(*editor);") != std::string::npos &&
            body.find("seedNativeMoves") == std::string::npos, "beginCreate starts with the moveless default");
    const auto stage = src.find("bool stageCreate(TrainerViewScreen& screen) {");
    const auto stageBody = src.substr(stage, src.find("\n}\n", stage) - stage);
    require(stageBody.find("state.missingRequiredFields()") < stageBody.find("state.add("),
            "stageCreate checks the required fields before Add");
}

} // namespace

int main() {
    createEntryStartsBlank();
    run(GS, SourceGame::Gold, "gold_gbc");
    run(GS, SourceGame::Silver, "silver_gbc");
    run(C, SourceGame::Crystal, "crystal_gbc");
    std::cout << "Gen II blank Create, species change keeps compatible moves, Create history reset (Gold, "
                 "Silver, Crystal; keep, Review, export, reparse, Discard): PASS\n";
    return 0;
}
