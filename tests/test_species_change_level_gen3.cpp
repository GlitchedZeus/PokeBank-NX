// Round 13 owner decision 9: an EXISTING Pokemon keeps its current level on a species change and
// its EXP is reconciled to the new species' growth rate (Pokemon/SpeciesChangeExperience.h).
// Generation III (Ruby / Sapphire / Emerald / FireRed / LeafGreen) through the real
// Session::setSpecies (the Gen III species picker's only apply) and the real StagedPokemonEditor,
// box and Party, all six growth rates (Medium Fast, Erratic, Fluctuating, Medium Slow, Fast, Slow),
// Lv 1 / mid / 100: Keep stages exactly that level and EXP (the Party level byte and stats follow
// EXP), Review lists the EXP change and no Level change, export -> strict reparse keeps both, and
// the existing Pokemon's Met Location / Met Level / Pokerus / origin stay unchanged (owner H25). A
// species-only edit staged straight into the editor obeys the same rule. A Create draft keeps its
// default-level rule.
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "UI/Gen3SharedPokemonSession.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen3;
using PokeBank::UIModel::Gen3SharedEditor::Session;
using Mode = PokeBank::UIModel::Gen3SharedEditor::Mode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Save fixture: the same layout tests/test_gen3_staged_pokemon_editor.cpp builds (Pikachu Lv20 in
// box 1 slot 1, both save slots valid).
enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
uint8_t origin(SourceGame game) {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}
const char* sourceId(SourceGame game) {
    switch (game) {
        case SourceGame::RubyGBA: return "ruby_gba";
        case SourceGame::SapphireGBA: return "sapphire_gba";
        case SourceGame::EmeraldGBA: return "emerald_gba";
        case SourceGame::FireRedGBA: return "firered_gba";
        case SourceGame::LeafGreenGBA: return "leafgreen_gba";
    }
    return "";
}
std::array<uint8_t, 80> samplePokemon(SourceGame game) {
    std::array<uint8_t, 80> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(25);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"PIKACHU");
    p.setHeldItem(13);
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setPokerus(0x21);   // Round 11 follow-up: an existing Pokemon's Pokerus must survive an Edit species change
    p.setLevel(20);
    p.setMove(0, 84);  // Thunder Shock with PP Ups, the move that must not survive
    p.setMovePPUps(0, 2);
    p.setMovePP(0, 42);
    p.setMove(1, 45);
    p.setMovePP(1, 40);
    for (int i = 0; i < 6; ++i) {
        p.setIV(i, static_cast<uint8_t>(10 + i));
        p.setEV(i, static_cast<uint8_t>(20 + i));
    }
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::array<uint8_t, 80> out{};
    std::memcpy(out.data(), encrypted, out.size());
    delete[] encrypted;
    return out;
}
LogicalSectors makeLogical(Family family, SourceGame game) {
    LogicalSectors logical{};
    if (family == Family::FRLG) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, key);
        write32(logical[1], 0x0290, key);
    } else if (family == Family::Emerald) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        write32(logical[1], 0x0490, key);
    } else {
        write32(logical[0], 0xAC, 0);
        write32(logical[1], 0x0490, 0);
    }
    write16(logical[0], 0x0A, 12345);
    write16(logical[0], 0x0C, 54321);
    const auto mon = samplePokemon(game);
    for (std::size_t i = 0; i < mon.size(); ++i) {
        const std::size_t position = 4 + i;
        logical[5 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] = mon[i];
    }
    return logical;
}
void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family,
               SourceGame game) {
    auto logical = makeLogical(family, game);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}
std::vector<uint8_t> fixture(Family family, SourceGame game) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game);
    writeSlot(save, 1, 9, 5, family, game);
    return save;
}

// Adds the fixture Pikachu as Party 1 too (the layout tests/test_gen3_staged_pokemon_editor.cpp uses).
std::vector<uint8_t> withParty(std::vector<uint8_t> source, SourceGame game) {
    const auto selected = Detail::validateSlot(source, 1);
    require(selected.valid, "slot 1 valid");
    const auto sector = selected.logicalSectorOffsets[1];
    const auto body = sector + Detail::partyDataOffset(game);
    source[sector + Detail::partyCountOffset(game)] = 1;
    const auto boxed = samplePokemon(game);
    std::copy(boxed.begin(), boxed.end(), source.begin() + static_cast<std::ptrdiff_t>(body));
    Pokemon::Pokemon3FRLG native(std::span<const std::byte>(reinterpret_cast<const std::byte*>(source.data() + body), 100));
    native.recalculateStats();
    auto* encoded = Encryption::encryptArray3FRLG(native.getData());
    std::memcpy(source.data() + body, encoded, 100);
    delete[] encoded;
    write16(source, sector + 0xFF6,
            Detail::sectorChecksum(std::span<const uint8_t>(source).subspan(sector, Detail::kChunkLengths[1])));
    return source;
}

// The owner rule, written out independently of the shared helper.
uint32_t expected(uint8_t level, uint32_t exp, uint8_t g) {
    const uint32_t low = Pokemon::getExpForLevel(level, g);
    const uint32_t high = level == 100 ? low : Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), g) - 1;
    return exp >= low && exp <= high ? exp : low;
}

std::optional<StagedPokemonRecord> read(const StagedPokemonEditor& editor, bool party) {
    std::string error;
    return party ? editor.partyPokemon(0, error) : editor.boxedPokemon(0, 0, error);
}

void scenario(SourceGame game, Family family, bool party, uint16_t from, uint16_t to, uint8_t level, bool mid) {
    const std::string id = sourceId(game);
    const auto source = withParty(fixture(family, game), game);
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    require(editor && error.empty(), id + " editor: " + error);
    const uint8_t gFrom = Pokemon::getGrowthRate(from), gTo = Pokemon::getGrowthRate(to);
    uint32_t exp = Pokemon::getExpForLevel(level, gFrom);
    if (mid && level < 100) exp += (Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), gFrom) - exp) / 2;
    const std::string what = id + (party ? " Party " : " Box ") + std::to_string(from) + " Lv" +
                             std::to_string(level) + (mid ? "+" : "") + " -> " + std::to_string(to);
    BoxPokemonEdit seed;
    seed.species = from;
    seed.experience = exp;
    seed.moves = std::array<uint16_t, 4>{33, 0, 0, 0};   // Tackle: every scenario species can use it
    seed.pp = std::array<uint8_t, 4>{35, 0, 0, 0};
    seed.ppUps = std::array<uint8_t, 4>{};
    const bool seededOk = party ? editor->stagePartyPokemonEdit(0, seed, error) : editor->stageBoxPokemonEdit(0, 0, seed, error);
    require(seededOk, what + ": seed " + error);
    const auto seeded = *read(*editor, party);
    require(seeded.species == from && seeded.level == level && seeded.experience == exp, what + ": seeded");

    Session s;
    s.begin(seeded, Mode::Edit);
    require(s.setSpecies(to, id), what + ": setSpecies");
    const uint32_t want = expected(level, exp, gTo);
    require(s.working.level == level, what + ": session keeps the level (got " + std::to_string(s.working.level) + ")");
    require(s.working.experience == want, what + ": session EXP (got " + std::to_string(s.working.experience) +
                                              ", want " + std::to_string(want) + ")");
    require(s.speciesChangeExperienceNote().empty() == (want == exp), what + ": EXP note only when EXP changed");
    const bool kept = s.keep(*editor, 0, 0, error, party);
    require(kept, what + ": keep " + error);
    const auto after = *read(*editor, party);
    require(after.species == to && after.level == level && after.experience == want, what + ": staged level / EXP (Lv " +
                                                                                         std::to_string(after.level) + ")");
    require(after.metLocation == seeded.metLocation && after.metLevel == seeded.metLevel &&
                after.pokerus == seeded.pokerus && after.originGame == seeded.originGame && after.ball == seeded.ball,
            what + ": history kept");
    bool levelOk = true, expRow = false;
    for (const auto& c : editor->pendingChanges()) {
        if (c.label.ends_with(" Level") && c.afterValue != std::to_string(level)) levelOk = false;
        if (c.label.ends_with(" Experience") && c.afterValue == std::to_string(want)) expRow = true;
    }
    require(levelOk, what + ": Review keeps the level");
    require(expRow || want == Pokemon::getExpForLevel(20, Pokemon::getGrowthRate(25)), what + ": Review lists the EXP");

    const auto out = editor->finalizedBytes(error);
    require(!out.empty(), what + ": export " + error);
    auto reparsed = StagedPokemonEditor::create(out, game, error);
    {
        const bool ok_ = reparsed && error.empty();
        require(ok_, what + ": strict reparse " + error);
    }
    const auto back = *read(*reparsed, party);
    require(back.species == to && back.level == level && back.experience == want && back.metLocation == seeded.metLocation &&
                back.metLevel == seeded.metLevel && back.pokerus == seeded.pokerus,
            what + ": reparsed level / EXP / history");
}


// Round 15 (owner Round 14): "Show every actual change in Review, including species and EXP." The
// source Pikachu (Lv 20, 8000 EXP) -> Magikarp keeps Lv 20 at Slow's Lv 20 minimum (10000): Review
// lists Species and Experience, and no Level row. Box and Party.
void reviewRows(SourceGame game, Family family) {
    const std::string id = sourceId(game);
    for (const bool party : {false, true}) {
        std::string error;
        auto editor = StagedPokemonEditor::create(withParty(fixture(family, game), game), game, error);
        Session s;
        s.begin(*read(*editor, party), Mode::Edit);
        require(s.working.level == 20 && s.working.experience == 8000, "fixture Lv 20 / 8000");
        s.working.moves = {33, 0, 0, 0};   // Tackle, which Magikarp can use
        s.working.pp = {35, 0, 0, 0};
        s.working.ppUps = {};
        require(s.setSpecies(129, id), "Pikachu -> Magikarp");
        const bool kept = s.keep(*editor, 0, 0, error, party);
        require(kept, id + " keep: " + error);
        bool species = false, exp = false, level = false;
        for (const auto& c : editor->pendingChanges()) {
            species = species || (c.label.ends_with(" Species") && c.afterValue == "Magikarp");
            exp = exp || (c.label.ends_with(" Experience") && c.beforeValue == "8000" && c.afterValue == "10000");
            level = level || c.label.ends_with(" Level");
        }
        require(species && exp && !level, id + (party ? " Party" : " Box") +
                                              ": Review shows Species and Experience, no Level row");
    }
}

void run(SourceGame game, Family family) {
    reviewRows(game, family);
    // Rattata Medium Fast, Volbeat Erratic, Makuhita Fluctuating, Bulbasaur Medium Slow, Snubbull
    // Fast, Magikarp Slow.
    const uint16_t species[] = {19, 313, 296, 1, 209, 129};
    for (const bool party : {false, true})
        for (const auto from : species)
            for (const auto to : species) {
                if (from == to) continue;
                for (const uint8_t level : {uint8_t(1), uint8_t(37), uint8_t(100)})
                    for (const bool mid : {false, true}) scenario(game, family, party, from, to, level, mid);
            }
    const std::string id = sourceId(game);
    // The owner's example: Lv 50 Medium Fast -> Slow stays Lv 50 at Slow's Lv 50 minimum, with the note.
    {
        std::string error;
        auto editor = StagedPokemonEditor::create(fixture(family, game), game, error);
        Session s;
        s.begin(*editor->boxedPokemon(0, 0, error), Mode::Edit);
        require(s.setLevel(50) && s.keep(*editor, 0, 0, error), "Lv 50 Pikachu");
        s.begin(*editor->boxedPokemon(0, 0, error), Mode::Edit);
        require(s.setSpecies(129, id) && s.working.level == 50 && s.working.experience == 156250, "Lv 50 Magikarp");
        require(s.speciesChangeExperienceNote() == "Kept Lv. 50; EXP 125000 -> 156250 for the new species' growth rate",
                "note: " + s.speciesChangeExperienceNote());
        // A species-only edit staged straight into the editor obeys the same rule.
        auto direct = StagedPokemonEditor::create(fixture(family, game), game, error);
        BoxPokemonEdit seed; seed.level = 50;
        require(direct->stageBoxPokemonEdit(0, 0, seed, error), "direct Lv 50");
        BoxPokemonEdit change; change.species = 129;
        const bool ok = direct->stageBoxPokemonEdit(0, 0, change, error);
        require(ok, "direct species-only edit: " + error);
        const auto now = *direct->boxedPokemon(0, 0, error);
        require(now.level == 50 && now.experience == 156250, "direct edit keeps Lv 50 (got Lv " + std::to_string(now.level) + ")");
    }
    // A Create draft keeps the default-level rule.
    {
        Session create;
        TrainerRecord trainer;
        create.beginCreate(trainer, origin(game));
        require(create.setLevel(50) && create.setSpecies(129, id), "Create species change");
        require(create.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(id, 129),
                "Create keeps the default-level rule");
        require(create.speciesChangeExperienceNote().empty(), "no EXP note for Create");
    }
}

} // namespace

int main() {
    require(Pokemon::getGrowthRate(19) == 0 && Pokemon::getGrowthRate(313) == 1 && Pokemon::getGrowthRate(296) == 2 &&
                Pokemon::getGrowthRate(1) == 3 && Pokemon::getGrowthRate(209) == 4 && Pokemon::getGrowthRate(129) == 5,
            "fixture growth rates");
    run(SourceGame::RubyGBA, Family::RS);
    run(SourceGame::SapphireGBA, Family::RS);
    run(SourceGame::EmeraldGBA, Family::Emerald);
    run(SourceGame::FireRedGBA, Family::FRLG);
    run(SourceGame::LeafGreenGBA, Family::FRLG);
    std::cout << "test_species_change_level_gen3: OK\n";
    return 0;
}
