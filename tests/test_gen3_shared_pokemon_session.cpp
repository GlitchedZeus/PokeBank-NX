#include "fixtures/main_workspace_move_focus.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <array>
#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::Gen3SharedEditor;
namespace Gen3 = PokeVault::Integration::Gen3;

int main() {
    Gen3::StagedPokemonRecord source{};
    source.species = 25;
    source.nickname = "PIKACHU";
    source.level = 20;
    source.experience = Pokemon::getExpForLevel(20, Pokemon::getGrowthRate(25));
    source.heldItem = 13;
    source.ivs = {10,11,12,13,14,15};
    source.evs = {20,21,22,23,24,25};
    source.pid = 0x12345678;
    source.tid = 12345;
    source.sid = 54321;
    source.nature = static_cast<uint8_t>(source.pid % 25);
    source.gender = 0;
    source.shiny = false;
    source.ability = 9;
    source.abilityNumber = 1;

    for (const auto game : {"ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba"}) {
        for (const auto mode : {Mode::View, Mode::Edit, Mode::Create}) {
            Session moves; moves.begin(source, mode);
            moves.working.moves = {204, 3, 84, 0};
            moves.working.pp = {20, 10, 30, 0};
            moves.working.ppUps = {0, 1, 2, 0};
            const auto before = moves.working;
            checkMainMoveFocus(PokeBank::UIModel::SharedPokemonEditor::Generation::Gen3,
                moves.working, mode == Mode::View);
            assert(moves.working.moves == before.moves && moves.working.pp == before.pp &&
                   moves.working.ppUps == before.ppUps);
            if (mode != Mode::View) {
                assert(moves.setSpecies(25, game));
                // The contextual editor changes the draft fields; requests preserve all three.
                moves.working.moves[1] = 33;
                moves.working.pp[1] = 10;
                moves.working.ppUps[1] = 2;
                if (mode == Mode::Edit) {
                    const auto request = moves.editRequest();
                    assert(request.moves && (*request.moves)[1] == 33);
                    assert(request.pp && (*request.pp)[1] == 10);
                    assert(request.ppUps && (*request.ppUps)[1] == 2);
                } else {
                    const auto request = moves.createRequest();
                    assert(request.moves[1] == 33 && request.pp[1] == 10 && request.ppUps[1] == 2);
                }
            } else assert(!moves.editable());
            assert((source.moves == std::array<uint16_t,4>{}));
        }
    }
    for (const auto mode : {Mode::Create, Mode::Edit}) {
        Session identity; identity.begin(source, mode);
        identity.working.nickname = "TEST";
        identity.working.otName = "BLUE";
        identity.working.tid = 65535;
        assert(identity.setLevel(50) && identity.dirty());
        if (mode == Mode::Create) {
            const auto request = identity.createRequest();
            assert(request.otName == "BLUE" && request.tid == 65535 && request.level == 50);
        } else {
            const auto request = identity.editRequest();
            assert(request.otName == "BLUE" && request.tid == 65535 && request.level == 50);
        }
        identity.discardDraft();
        assert(identity.working.tid == source.tid && identity.working.otName == source.otName);
    }
    Session edit{};
    edit.begin(source, Mode::Edit);
    assert(!edit.dirty());
    assert(!edit.back() && edit.confirmExit && edit.mode == Mode::Edit);
    edit.continueEditing();
    assert(!edit.confirmExit && edit.mode == Mode::Edit);
    assert(edit.setLevel(25));
    assert(edit.working.level == 25);
    assert(edit.working.pid == source.pid);
    assert(edit.working.tid == source.tid);
    assert(edit.working.sid == source.sid);
    assert(edit.dirty());

    auto request = edit.editRequest();
    assert(request.level && *request.level == 25);
    assert(!request.experience);
    assert(!request.ivs);

    edit.working.ivs[0] = 31;
    request = edit.editRequest();
    assert(request.ivs && (*request.ivs)[0] == 31);

    assert(edit.setNature(7));
    assert(edit.setGender(1));
    assert(edit.setShiny(true));
    assert(!edit.setAbilityNumber(2)); // Pikachu has one Gen III ability.
    request = edit.editRequest();
    assert(request.nature && *request.nature == 7);
    assert(request.gender && *request.gender == 1);
    assert(request.shiny && *request.shiny);
    assert(!request.abilityNumber);

    Gen3::StagedPokemonRecord ralts = source;
    ralts.species = 280;
    ralts.ability = Pokemon::getPersonalInfoG3(280).ability1;
    ralts.abilityNumber = 1;
    Session correlated{};
    correlated.begin(ralts, Mode::Edit);
    assert(correlated.setAbilityNumber(2));
    assert(correlated.editRequest().abilityNumber == 2);

    // Browsing / draft edits never mutate the source snapshot.
    assert(source.level == 20);
    assert(source.ivs[0] == 10);
    assert(source.pid == 0x12345678);

    // Species replacement is one shared new-species initialization policy in both
    // Edit and Create. Exercise every exact Generation III game identity here so a
    // future per-game integration cannot silently restore inherited level semantics.
    struct ExactGameCase {
        const char* sourceGameId;
        uint16_t replacementSpecies;
        uint8_t expectedMinimum;
    };
    constexpr std::array exactGames{
        ExactGameCase{"ruby_gba", 261, 2},
        ExactGameCase{"sapphire_gba", 261, 2},
        ExactGameCase{"emerald_gba", 261, 2},
        ExactGameCase{"firered_gba", 16, 2},
        ExactGameCase{"leafgreen_gba", 16, 2},
    };

    Gen3::TrainerRecord trainer{};
    trainer.name = "RED";
    trainer.tid16 = 1111;
    trainer.sid16 = 2222;

    for (const auto& game : exactGames) {
        const uint8_t expected =
            PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(
                game.sourceGameId, game.replacementSpecies);
        assert(expected == game.expectedMinimum);

        Session speciesEdit{};
        speciesEdit.begin(source, Mode::Edit);
        assert(speciesEdit.setLevel(80));
        assert(speciesEdit.setSpecies(game.replacementSpecies, game.sourceGameId));
        assert(speciesEdit.working.species == game.replacementSpecies);
        assert(speciesEdit.working.level == expected);
        assert(speciesEdit.working.level != 80);
        assert(speciesEdit.working.experience ==
               Pokemon::getExpForLevel(
                   expected, Pokemon::getGrowthRate(game.replacementSpecies)));
        const auto speciesRequest = speciesEdit.editRequest();
        assert(speciesRequest.species &&
               *speciesRequest.species == game.replacementSpecies);
        assert(speciesRequest.level && *speciesRequest.level == expected);
        assert(!speciesRequest.experience);

        // Encounter legality is reporting context, not an editing restriction.
        // Manual progression remains freely stageable across the full 1..100 range.
        for (uint8_t manual : {uint8_t(1), uint8_t(50), uint8_t(100)}) {
            assert(speciesEdit.setLevel(manual));
            assert(speciesEdit.working.level == manual);
            assert(speciesEdit.working.experience ==
                   Pokemon::getExpForLevel(
                       manual, Pokemon::getGrowthRate(game.replacementSpecies)));
            const auto manualRequest = speciesEdit.editRequest();
            assert(manualRequest.level && *manualRequest.level == manual);
        }
        speciesEdit.discardDraft();
        assert(speciesEdit.mode == Mode::None);

        Session create{};
        create.beginCreate(trainer);
        assert(create.mode == Mode::Create);
        assert(!create.back() && create.confirmExit && create.mode == Mode::Create);
        create.continueEditing();
        assert(!create.confirmExit && create.mode == Mode::Create);
        assert(create.working.species == 25);
        assert(create.working.level == 5);
        assert(create.working.language == 2);
        assert(create.working.friendship == 70);
        assert(create.working.ball == 4);
        assert(create.working.tid == 1111);
        assert(create.working.sid == 2222);
        assert(create.setLevel(50));
        assert(create.setSpecies(game.replacementSpecies, game.sourceGameId));
        assert(create.working.species == game.replacementSpecies);
        assert(create.working.level == expected);
        assert(create.working.level != 50);
        assert(create.working.experience ==
               Pokemon::getExpForLevel(
                   expected, Pokemon::getGrowthRate(game.replacementSpecies)));
        const auto add = create.createRequest();
        assert(add.species == game.replacementSpecies);
        assert(add.level == expected);
        assert(add.language == 2);
        assert(add.friendship == 70);
    }

    // Missing encounter data uses one deterministic shared fallback and never a stale
    // prior level/EXP. The fallback is independent of generation-specific UI code.
    Session fallback{};
    fallback.beginCreate(trainer);
    assert(fallback.setLevel(50));
    assert(fallback.setSpecies(133, "mock_no_encounter_provider"));
    assert(fallback.working.level ==
           PokeBank::UIModel::SpeciesChangeLevelPolicy::fallbackLevel);
    assert(fallback.working.level != 50);
    assert(fallback.working.experience ==
           Pokemon::getExpForLevel(
               PokeBank::UIModel::SpeciesChangeLevelPolicy::fallbackLevel,
               Pokemon::getGrowthRate(133)));

    // The source snapshot remains immutable through all draft edits/discards above.
    assert(source.species == 25 && source.level == 20 && source.experience ==
           Pokemon::getExpForLevel(20, Pokemon::getGrowthRate(25)));
    assert(source.ivs[0] == 10 && source.pid == 0x12345678);

    edit.discardDraft();
    assert(edit.mode == Mode::None);

    // Unsupported source-range choices fail in the draft before reaching the staged save.
    assert(!fallback.setSpecies(0));
    assert(!fallback.setSpecies(387));
    assert(!fallback.setLevel(0));
    assert(!fallback.setLevel(101));

    std::cout << "Generation III shared editor draft session: PASS\n";
}
