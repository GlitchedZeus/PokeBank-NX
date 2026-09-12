from pathlib import Path

source_path = Path("src/Integration/Gen2/Gen2StagedEditor.cpp")
source = source_path.read_text()

old_compare = r'''bool samePokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return a.species == b.species && a.heldItem == b.heldItem && a.moves == b.moves &&
           a.trainerId == b.trainerId && a.experience == b.experience &&
           a.statExperience == b.statExperience && a.dvs == b.dvs && a.pp == b.pp &&
           a.ppUps == b.ppUps && a.friendship == b.friendship && a.pokerus == b.pokerus &&
           a.caughtData == b.caughtData && a.level == b.level && a.originalTrainer == b.originalTrainer &&
           a.nickname == b.nickname && a.isEgg == b.isEgg && a.shiny == b.shiny &&
           a.gender == b.gender;
}
'''

new_compare = r'''std::string pokemonMismatchDetail(const PokemonRecord& reparsed, const PokemonRecord& expected) {
    std::ostringstream os;
    auto add = [&](const char* field, const auto& actual, const auto& wanted) {
        if (actual == wanted) return;
        if (os.tellp() > 0) os << "; ";
        os << field << " expected=" << +wanted << " reparsed=" << +actual;
    };
    add("species", reparsed.species, expected.species);
    add("heldItem", reparsed.heldItem, expected.heldItem);
    add("trainerId", reparsed.trainerId, expected.trainerId);
    add("experience", reparsed.experience, expected.experience);
    for (std::size_t i = 0; i < 4; ++i) {
        const std::string moveField = "move" + std::to_string(i + 1);
        const std::string ppField = "pp" + std::to_string(i + 1);
        const std::string upField = "ppUps" + std::to_string(i + 1);
        add(moveField.c_str(), reparsed.moves[i], expected.moves[i]);
        add(ppField.c_str(), reparsed.pp[i], expected.pp[i]);
        add(upField.c_str(), reparsed.ppUps[i], expected.ppUps[i]);
    }
    for (std::size_t i = 0; i < 5; ++i) {
        const std::string statField = "statExp" + std::to_string(i);
        const std::string dvField = "dv" + std::to_string(i);
        add(statField.c_str(), reparsed.statExperience[i], expected.statExperience[i]);
        add(dvField.c_str(), reparsed.dvs[i], expected.dvs[i]);
    }
    add("friendship", reparsed.friendship, expected.friendship);
    add("pokerus", reparsed.pokerus, expected.pokerus);
    add("caughtData", reparsed.caughtData, expected.caughtData);
    add("level", reparsed.level, expected.level);
    add("isEgg", reparsed.isEgg, expected.isEgg);
    add("shiny", reparsed.shiny, expected.shiny);
    add("gender", reparsed.gender, expected.gender);
    if (reparsed.originalTrainer != expected.originalTrainer) {
        if (os.tellp() > 0) os << "; ";
        os << "originalTrainer expected='" << expected.originalTrainer
           << "' reparsed='" << reparsed.originalTrainer << "'";
    }
    if (reparsed.nickname != expected.nickname) {
        if (os.tellp() > 0) os << "; ";
        os << "nickname expected='" << expected.nickname
           << "' reparsed='" << reparsed.nickname << "'";
    }
    return os.str();
}

bool samePokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return pokemonMismatchDetail(a, b).empty();
}
'''

if old_compare not in source:
    raise SystemExit("samePokemon anchor not found")
source = source.replace(old_compare, new_compare, 1)

old_roundtrip = r'''    auto verified = boxedPokemon(box, slot, error);
    if (!verified || !samePokemon(*verified, after)) {
        if (error.empty()) error = "staged Generation II Pokemon edit failed semantic round-trip";
'''
new_roundtrip = r'''    auto verified = boxedPokemon(box, slot, error);
    if (!verified || !samePokemon(*verified, after)) {
        if (error.empty()) {
            const std::string mismatch = verified ? pokemonMismatchDetail(*verified, after)
                                                  : std::string("reparse unavailable");
            error = "staged Generation II Pokemon edit failed semantic round-trip: " + mismatch;
        }
'''
if old_roundtrip not in source:
    raise SystemExit("round-trip anchor not found")
source_path.write_text(source.replace(old_roundtrip, new_roundtrip, 1))

test_path = Path("tests/test_gsc_pokemon_editor.cpp")
test = test_path.read_text()
old_assert = "    assert(editor->stageBoxPokemonEdit(0,0,edit,error));\n"
new_assert = r'''    if (!editor->stageBoxPokemonEdit(0,0,edit,error)) {
        std::cerr << "GEN2_COMPOUND_ROUNDTRIP_FAILURE: " << error << "\n";
        return;
    }
'''
if old_assert not in test:
    raise SystemExit("compound test anchor not found")
test_path.write_text(test.replace(old_assert, new_assert, 1))
