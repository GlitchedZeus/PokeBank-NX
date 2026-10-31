#include "fixtures/main_workspace_move_focus.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <array>
#include <cassert>
#include <iostream>

using namespace PokeBank::UIModel::Gen3SharedEditor;
namespace Gen3 = PokeVault::Integration::Gen3;

int main() {
    assert((nativeAbilitySlots(25) == std::vector<uint16_t>{1})); // Pikachu: Static only in Gen III.
    assert((nativeAbilitySlots(280) == std::vector<uint16_t>{1, 2})); // Ralts: two native slots.
    for (uint16_t species = 1; species <= 386; ++species) {
        const auto slots = nativeAbilitySlots(species);
        assert(slots.front() == 1 && slots.size() <= 2);
        assert(slots.size() == 1 || slots[1] == 2);
    }
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

        // EXP is a first-class progression edit in both Create and Edit. The Switch UI
        // opens promptNumber for this row; the session maps entered EXP back to Level.
        const uint32_t exp42 =
            Pokemon::getExpForLevel(42, Pokemon::getGrowthRate(identity.working.species));
        assert(identity.setExperience(exp42));
        assert(identity.working.experience == exp42);
        assert(identity.working.level == 42);
        if (mode == Mode::Create) {
            const auto request = identity.createRequest();
            assert(request.experience && *request.experience == exp42);
            assert(request.level == 42);
        } else {
            const auto request = identity.editRequest();
            assert(request.experience && *request.experience == exp42);
            assert(!request.level);
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

    Session encounterFloor{};
    encounterFloor.begin(source, Mode::Create);
    assert(encounterFloor.setLevel(5));
    assert(encounterFloor.setEncounterMetLevel(25));
    assert(encounterFloor.working.metLevel == 25);
    assert(encounterFloor.working.level == 25);
    assert(encounterFloor.working.experience ==
           Pokemon::getExpForLevel(25, Pokemon::getGrowthRate(25)));
    assert(encounterFloor.setLevel(50));
    const auto level50Exp = encounterFloor.working.experience;
    assert(encounterFloor.setEncounterMetLevel(25));
    assert(encounterFloor.working.level == 50);
    assert(encounterFloor.working.experience == level50Exp);
    assert(!encounterFloor.setEncounterMetLevel(101));

    Session genderCycle{};
    genderCycle.begin(source, Mode::Edit);
    const auto firstGender = genderCycle.working.gender;
    assert(genderCycle.cycleGender());
    assert(genderCycle.working.gender != firstGender);
    assert(genderCycle.cycleGender());
    assert(genderCycle.working.gender == firstGender);

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
        uint8_t originGame;
    };
    constexpr std::array exactGames{
        ExactGameCase{"ruby_gba", 261, 2, 2},
        ExactGameCase{"sapphire_gba", 261, 2, 1},
        ExactGameCase{"emerald_gba", 261, 2, 3},
        ExactGameCase{"firered_gba", 16, 2, 4},
        ExactGameCase{"leafgreen_gba", 16, 2, 5},
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
        create.beginCreate(trainer, game.originGame);
        assert(create.mode == Mode::Create);
        assert(create.working.originGame == game.originGame);
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
        create.working.metLocation = 57;
        create.working.metLevel = 25;
        create.working.ivs = {31, 30, 29, 28, 27, 26};
        create.working.evs = {100, 90, 80, 70, 60, 50};
        create.working.pokerus = 0x21;
        assert(create.setAbilityNumber(1)); // native slot 1 is explicit even for one-ability species.
        const auto add = create.createRequest();
        assert(add.species == game.replacementSpecies);
        assert(add.level == expected);
        assert(add.experience && *add.experience == create.working.experience);
        assert(add.language == 2);
        assert(add.friendship == 70);
        assert(add.metLocation == 57 && add.metLevel == 25);
        assert(add.ivs == create.working.ivs && add.evs == create.working.evs);
        assert(add.pokerus == 0x21);
        assert(add.abilityNumber && *add.abilityNumber == 1);
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

    // --- Shared-editor contract points 3/7/9 (Gen III PID-linked rows) ---
    // Confirming an unchanged Nature/Gender/Shiny/Ability in Edit must not request a PID reroll.
    {
        Session same{};
        same.begin(source, Mode::Edit);
        assert(same.setNature(source.nature) && same.setGender(source.gender) && same.setShiny(source.shiny));
        auto r = same.editRequest();
        assert(!r.nature && !r.gender && !r.shiny && !r.abilityNumber && !same.dirty());
        // A real change is requested; returning to the record's own value drops it again.
        assert(same.setNature(static_cast<uint8_t>((source.nature + 1) % 25)) && same.editRequest().nature);
        assert(same.setNature(source.nature) && !same.editRequest().nature);
        Session rs{};
        rs.begin(ralts, Mode::Edit);
        assert(rs.setAbilityNumber(1) && !rs.editRequest().abilityNumber);
        // Create keeps every explicit choice (its starting PID is not the user's).
        Session c{};
        c.beginCreate(trainer);
        assert(c.setNature(c.baseline.nature) && c.createRequest().nature == c.baseline.nature);
    }
    // A Trainer ID change that would flip shininess requests the current shininess explicitly;
    // one that would not flip it requests nothing; returning to the original TID drops it.
    {
        static_assert(shinyFor(0, 0, 0x00000007) && !shinyFor(0, 0, 0x00000008));
        Session tid{};
        tid.begin(source, Mode::Edit);
        assert(!shinyFor(source.tid, source.sid, source.pid) && !source.shiny);
        const uint16_t flipping = static_cast<uint16_t>(source.sid ^ (source.pid >> 16) ^ (source.pid & 0xFFFF));
        assert(shinyFor(flipping, source.sid, source.pid));
        assert(tid.setTid(flipping));
        auto r = tid.editRequest();
        assert(r.tid == flipping && r.shiny && *r.shiny == false);
        assert(tid.setTid(static_cast<uint16_t>(flipping ^ 0x0100)));
        assert(!shinyFor(tid.working.tid, source.sid, source.pid));
        assert(!tid.editRequest().shiny); // would not flip: the PID is kept
        assert(tid.setTid(source.tid) && !tid.editRequest().shiny && !tid.editRequest().tid);
        // An explicit Shiny choice is never overridden by a later TID edit.
        assert(tid.setShiny(true) && tid.setTid(flipping) && tid.editRequest().shiny == true);
        Gen3::StagedPokemonRecord shinySource = source;
        shinySource.tid = flipping;
        shinySource.shiny = true;
        Session keepShiny{};
        keepShiny.begin(shinySource, Mode::Edit);
        assert(keepShiny.setTid(source.tid) && keepShiny.editRequest().shiny == true);
    }
    // A species change drops gender/ability requests the new species cannot represent, so
    // Keep/Add is never stuck behind a row that is no longer editable.
    {
        Session g{};
        g.begin(ralts, Mode::Edit);
        assert(g.setGender(1) && g.editRequest().gender == 1);
        assert(g.setAbilityNumber(2) && g.editRequest().abilityNumber == 2);
        assert(Pokemon::getPersonalInfo(106, 0).genderRatio == 0); // Hitmonlee: male only
        assert(nativeAbilitySlots(106).size() == 1);
        assert(g.setSpecies(106, "ruby_gba"));
        auto r = g.editRequest();
        assert(!r.gender && !r.abilityNumber && r.species == 106);
        // Still-representable requests survive a species change.
        Session k{};
        k.begin(ralts, Mode::Edit);
        assert(k.setGender(1) && k.setSpecies(281, "ruby_gba") && k.editRequest().gender == 1);
        Session c{};
        c.beginCreate(trainer);
        assert(c.setGender(1) && c.setSpecies(106, "ruby_gba") && !c.createRequest().gender);
    }
    // The exact preview's unrequested PID-linked values become the draft's displayed values, so
    // Gender/Shiny toggles start from what the user sees; requested values are never overridden.
    {
        Session a{};
        a.begin(ralts, Mode::Edit);
        assert(a.setNature(3));
        auto preview = a.working;
        preview.gender = 1;
        preview.shiny = true;
        preview.nature = 9;
        preview.abilityNumber = 2;
        preview.ability = Pokemon::getPersonalInfoG3(280).ability2;
        a.adoptPreview(preview);
        assert(a.working.gender == 1 && a.working.shiny && a.working.abilityNumber == 2);
        assert(a.working.nature == 3); // requested
        assert(a.cycleGender() && a.working.gender == 0 && !a.editRequest().gender);
        preview.species = 281; // a stale preview for another species is ignored
        preview.gender = 0;
        a.adoptPreview(preview);
        assert(a.working.gender == 0);
    }

    std::cout << "Generation III shared editor draft session: PASS\n";
}
