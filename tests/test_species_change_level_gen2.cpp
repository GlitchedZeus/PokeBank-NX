// Round 13 owner decision 9: an EXISTING Pokemon keeps its current level on a species change and
// its EXP is reconciled to the new species' growth rate (Pokemon/SpeciesChangeExperience.h).
// Generation II (Gold / Silver / Crystal) through the real Session::setSpecies (both Gen II species
// pickers call it) and the real StagedEditor, box and Party, every Gen II growth-rate pair (Medium
// Fast, Fast, Medium Slow, Slow), Lv 1 / mid / 100: the stored level byte and Party stats agree
// with EXP, the Review "Level / Experience" row names the kept level and the reconciled EXP,
// export -> strict reparse keeps both, and the existing Pokemon's Pokerus and Crystal caught data
// stay byte for byte (owner H25). A species-only edit staged straight into the editor (the older
// GSC overlay path) obeys the same rule. A Create draft keeps its default-level rule.
#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include "Pokemon/Experience.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <cassert>
#include <iostream>
#include <string>

namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
using Rules::Session;
using Rules::SessionMode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

uint8_t growth(uint16_t species) { return personalRecord(species)->experienceGrowth; }

// The owner rule, written out independently of the shared helper.
uint32_t expected(uint8_t level, uint32_t exp, uint8_t g) {
    const uint32_t low = Pokemon::getExpForLevel(level, g);
    const uint32_t high = level == 100 ? low : Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), g) - 1;
    return exp >= low && exp <= high ? exp : low;
}

std::optional<PokemonRecord> read(const StagedEditor& editor, bool party) {
    std::string error;
    return party ? editor.partyPokemon(0, error) : editor.boxedPokemon(1, 0, error);
}

void scenario(const L& layout, SourceGame game, std::string_view sourceId, bool party, uint16_t from, uint16_t to,
              uint8_t level, bool mid) {
    const bool crystal = game == SourceGame::Crystal;
    auto parsed = parse(fixture(layout, true), game);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    require(static_cast<bool>(editor), "editor");
    uint32_t exp = Pokemon::getExpForLevel(level, growth(from));
    if (mid && level < 100) exp += (Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), growth(from)) - exp) / 2;
    const std::string what = std::string(sourceId) + (party ? " Party " : " Box ") + std::to_string(from) + " Lv" +
                             std::to_string(level) + (mid ? "+" : "") + " -> " + std::to_string(to);

    // Seed the existing Pokemon (species, exact EXP, Pokerus, Crystal caught data) as an earlier
    // session would have.
    namespace Native = PokeBank::UIModel::Gen2Native;
    Native::CrystalCaughtData caught;
    caught.present = true; caught.timeOfDay = 2; caught.levelCode = 16; caught.location = 50;
    const uint16_t caughtRaw = crystal ? Native::encodeCrystalCaughtData(caught) : 0;
    {
        Session seed;
        seed.begin(*read(*editor, party), SessionMode::Edit);
        seed.working.species = from;
        seed.working.moves = {33, 0, 0, 0};   // Tackle: every scenario species can use it
        seed.working.pp = {35, 0, 0, 0};
        seed.working.ppUps = {};
        require(seed.setExperience(exp), what + ": seed EXP");
        seed.working.pokerus = 0x21;
        seed.working.caughtData = caughtRaw;
        const bool kept = seed.keep(*editor, 1, 0, error, party);
        require(kept, what + ": seed keep " + error);
    }
    const auto seeded = *read(*editor, party);
    require(seeded.species == from && seeded.level == level && seeded.experience == exp, what + ": seeded");

    Session s;
    s.begin(seeded, SessionMode::Edit);
    require(s.setSpecies(to, sourceId), what + ": setSpecies");
    const uint32_t want = expected(level, exp, growth(to));
    require(s.working.level == level, what + ": session keeps the level (got " + std::to_string(s.working.level) + ")");
    require(s.working.experience == want, what + ": session EXP (got " + std::to_string(s.working.experience) +
                                              ", want " + std::to_string(want) + ")");
    require(s.working.pokerus == 0x21 && s.working.caughtData == caughtRaw, what + ": history kept in the draft");
    const std::string note = s.speciesChangeExperienceNote();
    require(note.empty() == (want == exp), what + ": EXP note only when EXP changed: " + note);
    if (want != exp)
        require(note == "Kept Lv. " + std::to_string(level) + "; EXP " + std::to_string(exp) + " -> " +
                            std::to_string(want) + " for the new species' growth rate", what + ": note text " + note);
    const bool kept = s.keep(*editor, 1, 0, error, party);
    require(kept, what + ": keep " + error);
    const auto after = *read(*editor, party);
    require(after.species == to && after.level == level && after.experience == want, what + ": staged level / EXP");
    require(after.rawBody[31] == level, what + ": stored level byte");
    require(after.pokerus == 0x21 && after.caughtData == caughtRaw, what + ": Pokerus / caught data byte for byte");
    if (party) {
        const auto stats = calculateBattleStats(to, level, Rules::storedDVs(after), after.statExperience).asArray();
        require(after.maxHP == stats[0] && after.attack == stats[1] && after.defense == stats[2] &&
                    after.speed == stats[3],
                what + ": Party stats recomputed for the kept level");
    }
    // Review compares with the source save (Pikachu Lv 15 / 3375 EXP).
    bool row = false;
    for (const auto& c : editor->pendingChanges())
        if (c.label.ends_with("Level / Experience"))
            row = c.afterValue == std::to_string(level) + " / " + std::to_string(want);
    require(row || (level == 15 && want == 3375), what + ": Review Level / Experience row");

    const auto exported = editor->finalizedBytes(error);
    require(!exported.empty(), what + ": export " + error);
    auto reread = parse(exported, game);
    require(static_cast<bool>(reread), what + ": strict reparse");
    auto rereadEditor = StagedEditor::create(*reread.save, error);
    const auto back = *read(*rereadEditor, party);
    require(back.species == to && back.level == level && back.experience == want && back.pokerus == 0x21 &&
                back.caughtData == caughtRaw,
            what + ": reparsed level / EXP / history");
}

void run(const L& layout, SourceGame game, std::string_view sourceId) {
    // Rattata Medium Fast (0), Snubbull Fast (4), Bulbasaur Medium Slow (3), Magikarp Slow (5).
    const uint16_t species[] = {19, 209, 1, 129};
    for (const bool party : {false, true})
        for (const auto from : species)
            for (const auto to : species) {
                if (from == to) continue;
                for (const uint8_t level : {uint8_t(1), uint8_t(37), uint8_t(100)})
                    for (const bool mid : {false, true}) scenario(layout, game, sourceId, party, from, to, level, mid);
            }

    // The owner's example and the older GSC overlay path: a species-only edit staged straight into
    // the editor keeps Lv 50; Medium Fast 125,000 -> Slow 156,250.
    {
        auto parsed = parse(fixture(layout, true), game);
        std::string error;
        auto editor = StagedEditor::create(*parsed.save, error);
        BoxPokemonEdit seed; seed.level = 50;
        require(editor->stageBoxPokemonEdit(1, 0, seed, error), "Lv 50 Pikachu");
        BoxPokemonEdit change; change.species = 129;
        {
            const bool ok_ = editor->stageBoxPokemonEdit(1, 0, change, error);
            require(ok_, "species-only edit: " + error);
        }
        const auto now = *editor->boxedPokemon(1, 0, error);
        require(now.level == 50 && now.experience == 156250, "species-only edit keeps Lv 50 at 156,250 EXP (got Lv " +
                                                                 std::to_string(now.level) + ")");
    }

    // A Create draft is not an existing Pokemon: the default-level policy is unchanged.
    {
        auto parsed = parse(fixture(layout, true), game);
        std::string error;
        auto editor = StagedEditor::create(*parsed.save, error);
        Session create;
        create.begin(*editor->boxedPokemon(1, 0, error), SessionMode::Create);
        require(create.setLevel(50) && create.setSpecies(129, sourceId), "Create species change");
        require(create.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, 129),
                "Create keeps the default-level rule");
        require(create.speciesChangeExperienceNote().empty(), "no EXP note for a Create draft");
    }
}

} // namespace

int main() {
    require(growth(19) == 0 && growth(209) == 4 && growth(1) == 3 && growth(129) == 5, "fixture growth rates");
    run(GS, SourceGame::Gold, "gold_gbc");
    run(GS, SourceGame::Silver, "silver_gbc");
    run(C, SourceGame::Crystal, "crystal_gbc");
    std::cout << "test_species_change_level_gen2: OK\n";
    return 0;
}
