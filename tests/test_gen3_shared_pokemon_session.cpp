#include "UI/Gen3SharedPokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

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

    Session edit{};
    edit.begin(source, Mode::Edit);
    assert(!edit.dirty());
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

    // Browsing / draft edits never mutate the source snapshot.
    assert(source.level == 20);
    assert(source.ivs[0] == 10);
    assert(source.pid == 0x12345678);

    // Species replacement is a new-species initialization in Edit too: it never
    // inherits the old Pokemon's level. Sapphire Poochyena has a real Lv2 minimum.
    Session speciesEdit{};
    speciesEdit.begin(source, Mode::Edit);
    assert(speciesEdit.setLevel(80));
    assert(speciesEdit.setSpecies(261, "sapphire_gba"));
    assert(speciesEdit.working.level == 2);
    assert(speciesEdit.working.experience ==
           Pokemon::getExpForLevel(2, Pokemon::getGrowthRate(261)));
    for (uint8_t manual : {uint8_t(1), uint8_t(50), uint8_t(100)}) {
        assert(speciesEdit.setLevel(manual));
        assert(speciesEdit.working.level == manual);
        assert(speciesEdit.working.experience ==
               Pokemon::getExpForLevel(manual, Pokemon::getGrowthRate(261)));
    }
    assert(source.species == 25 && source.level == 20 && source.experience ==
           Pokemon::getExpForLevel(20, Pokemon::getGrowthRate(25)));

    edit.discardDraft();
    assert(edit.mode == Mode::None);

    Gen3::TrainerRecord trainer{};
    trainer.name = "RED";
    trainer.tid16 = 1111;
    trainer.sid16 = 2222;

    Session create{};
    create.beginCreate(trainer);
    assert(create.mode == Mode::Create);
    assert(create.working.species == 25);
    assert(create.working.level == 5);
    assert(create.working.language == 2);
    assert(create.working.friendship == 70);
    assert(create.working.ball == 4);
    assert(create.working.tid == 1111);
    assert(create.working.sid == 2222);

    assert(create.setLevel(50));
    assert(create.setSpecies(261, "sapphire_gba"));
    assert(create.working.level == 2);
    assert(create.working.experience ==
           Pokemon::getExpForLevel(2, Pokemon::getGrowthRate(261)));
    assert(create.setSpecies(133, "sapphire_gba")); // no template -> shared fallback, never old level
    assert(create.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::fallbackLevel);
    assert(create.setLevel(10));
    const auto add = create.createRequest();
    assert(add.species == 133);
    assert(add.level == 10);
    assert(add.language == 2);
    assert(add.friendship == 70);

    // Unsupported source-range choices fail in the draft before reaching the staged save.
    assert(!create.setSpecies(0));
    assert(!create.setSpecies(387));
    assert(!create.setLevel(0));
    assert(!create.setLevel(101));

    std::cout << "Generation III shared editor draft session: PASS\n";
}
