// Round 13 owner decision 9: an EXISTING Pokemon keeps its current level on a species change and
// its EXP is reconciled to the new species' growth rate (Pokemon/SpeciesChangeExperience.h: EXP is
// kept when it already lies in that level's range, else it becomes the level's minimum; Lv 100 is
// the exact maximum). Generation I (Red / Blue / Yellow) through the real staged editor, box and
// Party, every Gen I growth-rate pair (Medium Fast, Fast, Medium Slow, Slow), Lv 1 / mid / 100:
// the box level byte, the Party level byte and battle stats agree with EXP, Review lists the EXP
// change and no Level change, and export -> strict reparse keeps level and EXP. Every Gen I species
// apply path stages edit.species with no level (the Edit branch of ux3ApplySpeciesPicker no longer
// forces the new species' encounter default level), so the editor's species-only rule is the one
// the UI uses. A Create (Add) draft keeps its own default-level rule.
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/Experience.h"
#include "Pokemon/SpeciesChangeExperience.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen1;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

uint8_t sum(const std::vector<uint8_t>& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += v[i];
    return (~s) & 255;
}
size_t bank(size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; }
void checksums(std::vector<uint8_t>& v) {
    v[0x3523] = sum(v, 0x2598, 0xF8B);
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        v[b + 0x1A4C] = sum(v, b, 0x1A4C);
        for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = sum(v, b + i * 0x462, 0x462);
    }
}
constexpr size_t kBox = 0x30C0, kBoxBody = kBox + 22, kParty = 0x2F2C, kPartyBody = kParty + 8;

// A Pikachu in box 3 (the current box) and the same Pikachu as Party 1 with its 44-byte cache.
std::vector<uint8_t> fixture(SourceGame game, uint8_t boxLevelByte = 5, uint32_t boxExp = 125) {
    std::vector<uint8_t> v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;  // RED
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = v[0x2F2D] = 0xFF;
    v[0x284C] = 0x82;
    v[0x29C3] = game == SourceGame::Yellow ? 0x54 : 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    v[kBox] = 1; v[kBox + 1] = 0x54; v[kBox + 2] = 0xFF;
    const size_t body = kBoxBody;
    v[body] = 0x54; v[body + 1] = 0; v[body + 2] = 20; v[body + 3] = boxLevelByte;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 45;  // Growl
    v[body + 12] = 0x12; v[body + 13] = 0x34;
    v[body + 14] = static_cast<uint8_t>(boxExp >> 16); v[body + 15] = static_cast<uint8_t>(boxExp >> 8);
    v[body + 16] = static_cast<uint8_t>(boxExp);
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 40;
    std::fill_n(v.begin() + kBox + 682, 11, 0x50); v[kBox + 682] = 0x91; v[kBox + 683] = 0x84; v[kBox + 684] = 0x83;
    std::fill_n(v.begin() + kBox + 902, 11, 0x50);
    v[kBox + 902] = 0x8F; v[kBox + 903] = 0x88; v[kBox + 904] = 0x8A; v[kBox + 905] = 0x80;  // PIKA
    // Party 1: the same body plus the level byte and battle-stat cache.
    v[kParty] = 1; v[kParty + 1] = 0x54; v[kParty + 2] = 0xFF;
    std::copy_n(v.begin() + kBoxBody, 33, v.begin() + kPartyBody);
    v[kPartyBody + 3] = 5;
    v[kPartyBody + 14] = 0; v[kPartyBody + 15] = 0; v[kPartyBody + 16] = 125;
    v[kPartyBody + 33] = 5;
    const auto stats = StagedPokemonEditor::calculateBattleStats(25, 5, {8, 8, 8, 8}, {}).asArray();
    for (size_t i = 0; i < stats.size(); ++i) {
        v[kPartyBody + 34 + i * 2] = static_cast<uint8_t>(stats[i] >> 8);
        v[kPartyBody + 35 + i * 2] = static_cast<uint8_t>(stats[i]);
    }
    v[kPartyBody + 1] = static_cast<uint8_t>(stats[0] >> 8); v[kPartyBody + 2] = static_cast<uint8_t>(stats[0]);
    std::copy_n(v.begin() + kBox + 682, 11, v.begin() + kPartyBody + 6 * 44);
    std::copy_n(v.begin() + kBox + 902, 11, v.begin() + kPartyBody + 6 * 44 + 6 * 11);
    std::copy_n(v.begin() + kBox, 0x462, v.begin() + bank(2));
    checksums(v);
    return v;
}

// The owner rule, written out independently of the shared helper.
uint32_t expected(uint8_t level, uint32_t exp, uint8_t growth) {
    const uint32_t low = Pokemon::getExpForLevel(level, growth);
    const uint32_t high = level == 100 ? low : Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), growth) - 1;
    return exp >= low && exp <= high ? exp : low;
}

std::optional<PokemonRecord> read(const StagedPokemonEditor& editor, bool party) {
    std::string error;
    return party ? editor.partyPokemon(0, error) : editor.boxedPokemon(2, 0, error);
}

// Seeds `from` at `level` (+ a within-level offset) with an explicit EXP, then stages a species-only
// change to `to` exactly as every Gen I species picker does, and checks the kept level / EXP.
void scenario(SourceGame game, bool party, uint16_t from, uint16_t to, uint8_t level, bool midLevel) {
    const auto raw = fixture(game);
    const auto source = parse(raw, game);
    require(static_cast<bool>(source), "fixture parses");
    std::string error;
    auto editor = StagedPokemonEditor::create(*source.save, error);
    require(static_cast<bool>(editor), "editor");
    const uint8_t gFrom = StagedPokemonEditor::growthRate(from), gTo = StagedPokemonEditor::growthRate(to);
    uint32_t exp = Pokemon::getExpForLevel(level, gFrom);
    if (midLevel && level < 100) exp += (Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), gFrom) - exp) / 2;
    const std::string what = std::string(party ? "Party " : "Box ") + std::to_string(from) + " Lv" +
                             std::to_string(level) + (midLevel ? "+" : "") + " -> " + std::to_string(to);
    BoxPokemonEdit seed;
    seed.species = from;
    seed.experience = exp;
    require(party ? editor->stagePartyPokemonEdit(0, seed, error) : editor->stageEdit(2, 0, seed, error),
            what + ": seed " + error);
    const auto seeded = *read(*editor, party);
    require(seeded.level == level && seeded.experience == exp, what + ": seeded level / EXP");

    BoxPokemonEdit change;
    change.species = to;
    require(party ? editor->stagePartyPokemonEdit(0, change, error) : editor->stageEdit(2, 0, change, error),
            what + ": species change " + error);
    const auto after = *read(*editor, party);
    const uint32_t want = expected(level, exp, gTo);
    require(after.species == to, what + ": species");
    require(after.level == level, what + ": level kept (got " + std::to_string(after.level) + ")");
    require(after.experience == want, what + ": EXP reconciled (got " + std::to_string(after.experience) +
                                          ", want " + std::to_string(want) + ")");
    require(Pokemon::getLevelFromExp(after.experience, gTo) == level, what + ": EXP reads back as the kept level");
    if (gFrom == gTo) require(after.experience == exp, what + ": same growth rate never touches EXP");
    require(after.rawBody[3] == level || (party && level == seeded.level), what + ": box level byte");
    if (party) {
        require(after.rawBody[33] == level, what + ": Party level byte");
        const auto stats = StagedPokemonEditor::calculateBattleStats(to, level,
            {after.dvs[1], after.dvs[2], after.dvs[3], after.dvs[4]}, after.statExperience).asArray();
        require((std::array<uint16_t, 5>{after.maxHP, after.attack, after.defense, after.speed, after.special}) == stats,
                what + ": Party battle stats recomputed for the kept level");
    }
    // Review (against the source save's Lv 5 / 125 EXP Pikachu): any Level row names the kept
    // level, and the reconciled EXP is listed whenever it differs from the source.
    bool levelOk = true, expRow = false;
    for (const auto& c : editor->pendingChanges()) {
        if (c.label.ends_with(": Level") && c.afterValue != std::to_string(level)) levelOk = false;
        if (c.label.ends_with(": Experience") && c.afterValue == std::to_string(want)) expRow = true;
    }
    require(levelOk, what + ": Review keeps the level");
    require(expRow || want == 125, what + ": Review lists the EXP");

    const auto exported = editor->finalizedBytes(error);
    require(!exported.empty(), what + ": export " + error);
    const auto reread = parse(exported, game);
    require(static_cast<bool>(reread), what + ": strict reparse");
    auto rereadEditor = StagedPokemonEditor::create(*reread.save, error);
    const auto back = *read(*rereadEditor, party);
    require(back.species == to && back.level == level && back.experience == want, what + ": reparsed level / EXP");
}

} // namespace

int main() {
    // Gen I growth rates: Pikachu Medium Fast (0), Clefairy Fast (4), Bulbasaur Medium Slow (3),
    // Magikarp Slow (5). Every ordered pair, Lv 1 / 37 (threshold and mid-level) / 100.
    const uint16_t species[] = {25, 35, 1, 129};
    require(StagedPokemonEditor::growthRate(25) == 0 && StagedPokemonEditor::growthRate(35) == 4 &&
            StagedPokemonEditor::growthRate(1) == 3 && StagedPokemonEditor::growthRate(129) == 5,
            "fixture growth rates");
    for (const auto game : {SourceGame::Red, SourceGame::Blue, SourceGame::Yellow})
        for (const bool party : {false, true})
            for (const auto from : species)
                for (const auto to : species) {
                    if (from == to) continue;
                    for (const uint8_t level : {uint8_t(1), uint8_t(37), uint8_t(100)})
                        for (const bool mid : {false, true}) scenario(game, party, from, to, level, mid);
                }

    // The owner's example: Lv 50 Medium Fast -> Slow stays Lv 50 with Slow's Lv 50 minimum EXP.
    {
        const auto source = parse(fixture(SourceGame::Red), SourceGame::Red);
        std::string error;
        auto editor = StagedPokemonEditor::create(*source.save, error);
        BoxPokemonEdit seed; seed.level = 50;
        require(editor->stageEdit(2, 0, seed, error), "Lv 50 Pikachu");
        BoxPokemonEdit change; change.species = 129;
        require(editor->stageEdit(2, 0, change, error), "Pikachu -> Magikarp");
        const auto now = *editor->boxedPokemon(2, 0, error);
        require(now.level == 50 && now.experience == 156250, "Lv 50 Magikarp at 156,250 EXP");
    }

    // A stored box level byte that disagrees with EXP (Lv 5 byte, Lv 6 EXP 216): the level the
    // Pokemon shows (5) is kept and EXP is reconciled to it for the new species.
    {
        const auto source = parse(fixture(SourceGame::Red, 5, 216), SourceGame::Red);
        require(static_cast<bool>(source), "disagreeing fixture parses");
        std::string error;
        auto editor = StagedPokemonEditor::create(*source.save, error);
        require(editor->boxedPokemon(2, 0, error)->level == 5, "shows Lv 5");
        BoxPokemonEdit change; change.species = 35;   // Clefairy, Fast: Lv 5 = 100, Lv 6 = 172
        require(editor->stageEdit(2, 0, change, error), "Pikachu -> Clefairy");
        const auto now = *editor->boxedPokemon(2, 0, error);
        require(now.level == 5 && now.experience == 100 && now.rawBody[3] == 5,
                "kept Lv 5, EXP 100, box level byte 5");
    }

    // A Create (Add) draft is not an existing Pokemon: its level is what the draft says.
    {
        const auto source = parse(fixture(SourceGame::Red), SourceGame::Red);
        std::string error;
        auto editor = StagedPokemonEditor::create(*source.save, error);
        BoxPokemonCreate draft;
        draft.species = 129; draft.level = 5; draft.moves = {150, 0, 0, 0}; draft.pp = {40, 0, 0, 0};
        const auto slot = editor->appendSlot(2, error);
        {
            const bool ok_ = slot && editor->stageAdd(2, *slot, draft, error);
            require(ok_, "Add: " + error);
        }
        const auto added = *editor->boxedPokemon(2, *slot, error);
        require(added.level == 5 && added.experience == Pokemon::getExpForLevel(5, 5), "Create level unchanged");
    }
    std::cout << "test_species_change_level_gen1: OK\n";
    return 0;
}
