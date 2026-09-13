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
new_compare = r'''uint8_t semanticGender(const PokemonRecord& pokemon) noexcept {
    return static_cast<uint8_t>(genderFromAttackDV(pokemon.species, pokemon.dvs[1]));
}

bool samePokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return a.species == b.species && a.heldItem == b.heldItem && a.moves == b.moves &&
           a.trainerId == b.trainerId && a.experience == b.experience &&
           a.statExperience == b.statExperience && a.dvs == b.dvs && a.pp == b.pp &&
           a.ppUps == b.ppUps && a.friendship == b.friendship && a.pokerus == b.pokerus &&
           a.caughtData == b.caughtData && a.level == b.level && a.originalTrainer == b.originalTrainer &&
           a.nickname == b.nickname && a.isEgg == b.isEgg && a.shiny == b.shiny &&
           semanticGender(a) == semanticGender(b);
}

std::string pokemonMismatchDetail(const PokemonRecord& reparsed, const PokemonRecord& expected) {
    std::ostringstream os;
    auto add = [&](std::string_view field, auto actual, auto wanted) {
        if (actual == wanted) return;
        if (os.tellp() > 0) os << "; ";
        os << field << " expected=" << +wanted << " reparsed=" << +actual;
    };
    add("species", reparsed.species, expected.species);
    add("heldItem", reparsed.heldItem, expected.heldItem);
    add("trainerId", reparsed.trainerId, expected.trainerId);
    add("experience", reparsed.experience, expected.experience);
    for (std::size_t i = 0; i < 4; ++i) {
        add("move" + std::to_string(i + 1), reparsed.moves[i], expected.moves[i]);
        add("pp" + std::to_string(i + 1), reparsed.pp[i], expected.pp[i]);
        add("ppUps" + std::to_string(i + 1), reparsed.ppUps[i], expected.ppUps[i]);
    }
    for (std::size_t i = 0; i < 5; ++i) {
        add("statExperience" + std::to_string(i), reparsed.statExperience[i], expected.statExperience[i]);
        add("dv" + std::to_string(i), reparsed.dvs[i], expected.dvs[i]);
    }
    add("friendship", reparsed.friendship, expected.friendship);
    add("pokerus", reparsed.pokerus, expected.pokerus);
    add("caughtData", reparsed.caughtData, expected.caughtData);
    add("level", reparsed.level, expected.level);
    add("isEgg", reparsed.isEgg, expected.isEgg);
    add("shiny", reparsed.shiny, expected.shiny);
    add("gender", semanticGender(reparsed), semanticGender(expected));
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
'''
if old_compare not in source:
    raise SystemExit("samePokemon anchor not found")
source = source.replace(old_compare, new_compare, 1)

old_boxed = r'''    return *parsed.save->boxes()[box].slots[slot];
}

bool StagedEditor::syncCurrentBoxCopy'''
new_boxed = r'''    PokemonRecord normalized = *parsed.save->boxes()[box].slots[slot];
    // Gender is not stored independently in Gen II PK2 data. The read-only parser deliberately
    // leaves PokemonRecord::gender unresolved; staged-editor semantics resolve it from species
    // and Attack DV so a serialize/reparse cycle compares the actual retail meaning.
    normalized.gender = semanticGender(normalized);
    return normalized;
}

bool StagedEditor::syncCurrentBoxCopy'''
if old_boxed not in source:
    raise SystemExit("boxedPokemon return anchor not found")
source = source.replace(old_boxed, new_boxed, 1)

old_roundtrip = r'''    auto verified = boxedPokemon(box, slot, error);
    if (!verified || !samePokemon(*verified, after)) {
        if (error.empty()) error = "staged Generation II Pokemon edit failed semantic round-trip";
'''
new_roundtrip = r'''    auto verified = boxedPokemon(box, slot, error);
    if (!verified || !samePokemon(*verified, after)) {
        if (error.empty()) {
            const std::string mismatch = verified ? pokemonMismatchDetail(*verified, after)
                                                  : std::string("reparse unavailable");
            error = "staged Generation II Pokemon edit failed semantic round-trip";
            if (!mismatch.empty()) error += ": " + mismatch;
        }
'''
if old_roundtrip not in source:
    raise SystemExit("round-trip anchor not found")
source = source.replace(old_roundtrip, new_roundtrip, 1)
source_path.write_text(source)

test_path = Path("tests/test_gsc_pokemon_editor.cpp")
test = test_path.read_text()

old_parser = r'''    auto parsed=parse(raw,game);assert(parsed);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor&&error.empty());
'''
new_parser = r'''    auto parsed=parse(raw,game);assert(parsed);
    // The accepted read parser intentionally keeps the raw PokemonRecord gender unresolved.
    // The staged semantic layer must derive gender from species + Attack DV without changing it.
    assert(parsed.save->boxes()[0].slots[0]);
    assert(parsed.save->boxes()[0].slots[0]->gender==2);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor&&error.empty());
'''
if old_parser not in test:
    raise SystemExit("parser regression anchor not found")
test = test.replace(old_parser, new_parser, 1)

old_p = r'''    auto p=editor->boxedPokemon(0,0,error);assert(p&&p->species==25&&p->nickname=="PIKA");
    assert(p->shiny);assert(StagedEditor::isShinyDVs({7,10,10,10}));
'''
new_p = r'''    auto p=editor->boxedPokemon(0,0,error);assert(p&&p->species==25&&p->nickname=="PIKA");
    assert(p->gender==static_cast<uint8_t>(genderFromAttackDV(p->species,p->dvs[1])));
    assert(p->shiny);assert(StagedEditor::isShinyDVs({7,10,10,10}));
'''
if old_p not in test:
    raise SystemExit("staged gender regression anchor not found")
test = test.replace(old_p, new_p, 1)

old_changed = r'''    assert(changed->dvs[0]==StagedEditor::derivedHPDV({9,8,7,6}));
    assert(changed->originalTrainer=="RED"&&changed->trainerId==4321&&changed->friendship==200);
'''
new_changed = r'''    assert(changed->dvs[0]==StagedEditor::derivedHPDV({9,8,7,6}));
    assert(changed->gender==static_cast<uint8_t>(genderFromAttackDV(changed->species,changed->dvs[1])));
    assert(changed->originalTrainer=="RED"&&changed->trainerId==4321&&changed->friendship==200);
'''
if old_changed not in test:
    raise SystemExit("compound gender regression anchor not found")
test = test.replace(old_changed, new_changed, 1)

test_path.write_text(test)
