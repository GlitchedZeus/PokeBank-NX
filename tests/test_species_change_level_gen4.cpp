// Round 13 owner decision 9: an EXISTING Pokemon keeps its current level on a species change and
// its EXP is reconciled to the new species' growth rate (Pokemon/SpeciesChangeExperience.h, used by
// Pokemon4Mutable::setSpecies). Generation IV (Diamond / Pearl / Platinum / HeartGold / SoulSilver)
// through the real Gen4SharedEditor::Session::changeSpecies (applyGen4Picker's Species branch) and
// Gen4StagedPokemonEditor, box and Party, all six growth rates, Lv 1 / mid / 100: EXP is kept when it
// already fits the level for the new growth rate (it was always reset to the level's minimum
// before), the Party level byte follows EXP, Review describes the slot, export -> strict reparse
// keeps level / EXP, and Met Location / Met Level / Pokerus / origin are kept (owner H25).
#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Pokemon/Experience.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <vector>

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;
using PokeBank::UIModel::Gen4SharedEditor::Session;
using PokeBank::UIModel::Gen4SharedEditor::Mode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Save fixture: the same layout tests/test_gen4_staged_pokemon_editor.cpp builds (Pikachu in box 1
// slot 1, a party Pikachu, both partitions valid, partition 0 newest).
constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC = 0x20060623;

struct Spec {
    size_t generalSize;
    size_t storageStart;
    size_t storageSize;
    size_t footerSize;
    size_t trainer;
    size_t party;
    size_t boxData;
    size_t boxStride;
};

Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
}

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
    b[o + 2] = static_cast<uint8_t>(v >> 16);
    b[o + 3] = static_cast<uint8_t>(v >> 24);
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
    b[o + 2] = static_cast<std::byte>(v >> 16);
    b[o + 3] = static_cast<std::byte>(v >> 24);
}
void copy(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i)
        dst[off + i] = static_cast<uint8_t>(src[i]);
}
void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(),
              dst.begin() + static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> occupiedPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, 0x12345678u);
    wb16(d, 0x08, species);
    wb16(d, 0x0A, 1);
    wb16(d, 0x0C, 12345);
    wb16(d, 0x0E, 54321);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x15] = std::byte{9};
    d[0x17] = std::byte{2};
    auto nickname = Utils::encodeGen4Field(u"PIKA", 11, 10, 2);
    copy(d, 0x48, nickname);
    auto ot = Utils::encodeGen4Field(u"ASH", 8, 7, 2);
    copy(d, 0x68, ot);
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{25};
    d[0x86] = std::byte{17};
    return Encryption::encryptArray4(d);
}

std::vector<std::byte> occupiedPartyPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    const auto stored = occupiedPk4(species);
    const auto decoded = Encryption::decryptArray4(stored);
    std::copy(decoded.begin(), decoded.end(), d.begin());
    d[0x8C] = std::byte{20};
    wb16(d, 0x8E, 35);
    wb16(d, 0x90, 35);
    wb16(d, 0x92, 25);
    wb16(d, 0x94, 20);
    wb16(d, 0x96, 30);
    wb16(d, 0x98, 25);
    wb16(d, 0x9A, 20);
    return Encryption::encryptArray4(d);
}

void stamp(std::vector<uint8_t>& save, size_t off, size_t len,
           size_t footer, uint32_t major) {
    const size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, MAGIC);
    w16(save, end - 2,
        Utils::crc16ccitt(save.data() + off, len - footer));
}

std::vector<uint8_t> makeSave(Layout layout, uint8_t romCode = 7,
                                  int newerGeneral = 0, int newerStorage = 0) {
    const auto sp = spec(layout);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = occupiedPk4();
    const auto party = occupiedPartyPk4();

    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(storage),
                  save.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);

        auto trainerName = Utils::encodeGen4Field(u"Kylie", 8, 7, 2);
        copy(save, base + sp.trainer, trainerName);
        w16(save, base + sp.trainer + 0x10, 12150);
        w16(save, base + sp.trainer + 0x12, 22558);
        save[base + sp.trainer + 0x18] = 1;
        save[base + sp.trainer + 0x19] = 2;
        save[base + sp.trainer + 0x1C] = romCode;
        save[base + sp.party - 4] = 1;
        copy(save, base + sp.party, party);

        for (size_t box = 0; box < 18; ++box) {
            const size_t boxBase = storage + sp.boxData + box * sp.boxStride;
            for (size_t slot = 0; slot < 30; ++slot)
                copy(save, boxBase + slot * Encryption::SIZE_STORED4, blank);
            if (layout == Layout::HeartGoldSoulSilver) {
                std::fill(
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0xFF0),
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0x1000),
                    static_cast<uint8_t>(0xC0 + box));
            }
        }
        copy(save, storage + sp.boxData, occupied);

        if (layout == Layout::HeartGoldSoulSilver)
            save[storage + 0x12000] = 0;
        else
            save[storage] = 0;

        stamp(save, base, sp.generalSize, sp.footerSize,
              partition == newerGeneral ? 20 : 10);
        stamp(save, storage, sp.storageSize, sp.footerSize,
              partition == newerStorage ? 30 : 15);
    }
    return save;
}

// The owner rule, written out independently of the shared helper.
uint32_t expected(uint8_t level, uint32_t exp, uint8_t g) {
    const uint32_t low = Pokemon::getExpForLevel(level, g);
    const uint32_t high = level == 100 ? low : Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), g) - 1;
    return exp >= low && exp <= high ? exp : low;
}

std::optional<Pokemon::Pokemon4Mutable> read(const Gen4StagedPokemonEditor& editor, bool party) {
    std::string error;
    return party ? editor.editablePartyPokemon(0, &error) : editor.editableBoxPokemon(0, 0, &error);
}

void scenario(Layout layout, uint8_t romCode, const char* id, bool party, uint16_t from, uint16_t to, uint8_t level,
              bool mid) {
    auto source = makeSave(layout, romCode);
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
    require(editor && error.empty(), std::string(id) + " editor: " + error);
    const auto group = editor->sourceGroup();
    const uint8_t gFrom = Pokemon::getGrowthRate(from), gTo = Pokemon::getGrowthRate(to);
    uint32_t exp = Pokemon::getExpForLevel(level, gFrom);
    if (mid && level < 100) exp += (Pokemon::getExpForLevel(static_cast<uint8_t>(level + 1), gFrom) - exp) / 2;
    const std::string what = std::string(id) + (party ? " Party " : " Box ") + std::to_string(from) + " Lv" +
                             std::to_string(level) + (mid ? "+" : "") + " -> " + std::to_string(to);
    // Seed: species, exact EXP, Tackle (every scenario species can use it), Pokerus.
    auto mon = *read(*editor, party);
    require(mon.setSpecies(from) && mon.setExperience(exp), what + ": seed species / EXP");
    require(mon.setMove(0, 33) && mon.setMove(1, 0) && mon.setMove(2, 0) && mon.setMove(3, 0), what + ": Tackle");
    require(mon.setPokerusMode(Pokemon::PokerusMode::Infected), what + ": Pokerus");
    const bool seededOk = party ? editor->commitPartyPokemon(0, mon, &error) : editor->commitBoxPokemon(0, 0, mon, &error);
    require(seededOk, what + ": seed commit " + error);
    const auto seeded = *read(*editor, party);
    require(seeded.species() == from && seeded.level() == level && seeded.experience() == exp, what + ": seeded");
    const auto metBefore = PokeBank::UIModel::Gen4SharedEditor::metLocationOf(seeded, group);

    Session s;
    std::string beginError;
    require(s.begin(Pokemon::Pokemon4ReadOnly(seeded.encryptedBytes(), group), group, Mode::Edit, beginError),
            what + ": begin " + beginError);
    s.baselineMoves = seeded.moves();
    bool reset = false;
    require(s.changeSpecies(to, seeded.shiny(), id, group, reset), what + ": changeSpecies");
    const uint32_t want = expected(level, exp, gTo);
    require(s.working->level() == level, what + ": session keeps the level");
    require(s.working->experience() == want, what + ": session EXP (got " + std::to_string(s.working->experience()) +
                                                 ", want " + std::to_string(want) + ")");
    require(s.speciesChangeExperienceNote().empty() == (want == exp), what + ": EXP note only when EXP changed");
    const bool kept = party ? s.keepParty(*editor, 0, error) : s.keep(*editor, 0, 0, error);
    require(kept, what + ": keep " + error);
    const auto after = *read(*editor, party);
    require(after.species() == to && after.level() == level && after.experience() == want, what + ": staged");
    const auto metAfter = PokeBank::UIModel::Gen4SharedEditor::metLocationOf(after, group);
    require(metBefore && metAfter && metAfter->location == metBefore->location && metAfter->origin == metBefore->origin &&
                after.metLevel() == seeded.metLevel() && after.pokerus() == seeded.pokerus(),
            what + ": history kept");
    if (party)
        require(Pokemon::Pokemon4ReadOnly(after.encryptedBytes(), group).partyLevel() == level, what + ": Party level byte");
    require(!PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor).empty(), what + ": Review describes the slot");

    const auto out = editor->finalizedBytes(&error);
    require(!out.empty() && error.empty(), what + ": export " + error);
    auto parsed = Gen4ReadOnlySave::parse(out, layout, id, &error);
    {
        const bool ok_ = parsed && error.empty();
        require(ok_, what + ": strict reparse " + error);
    }
    const auto& back = party ? parsed->party()[0] : parsed->box(0, 0);
    require(back.valid() && back.species() == to && back.experience() == want && back.metLevel() == seeded.metLevel() &&
                back.pokerusState() == seeded.pokerus(),
            what + ": reparsed EXP / history");
    if (party) require(back.partyLevel() == level, what + ": reparsed Party level byte");
}


// Round 15 (owner Round 14): "Show every actual change in Review, including species and EXP." The
// fixture Pikachu (10000 EXP, Lv 21) -> Magikarp keeps Lv 21 at Slow's Lv 21 minimum (11576):
// Review lists Species and Experience, and no Level row. Box and Party.
void reviewRows(Layout layout, uint8_t romCode, const char* id) {
    for (const bool party : {false, true}) {
        auto source = makeSave(layout, romCode);
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
        const auto group = editor->sourceGroup();
        auto mon = *read(*editor, party);
        require(mon.setMove(0, 33), "Tackle");   // a move Magikarp can use
        Session s;
        std::string beginError;
        require(s.begin(Pokemon::Pokemon4ReadOnly(mon.encryptedBytes(), group), group, Mode::Edit, beginError), "begin");
        s.baselineMoves = mon.moves();
        require(s.working->level() == 21 && s.working->experience() == 10000, "fixture Lv 21 / 10000");
        bool reset = false;
        require(s.changeSpecies(129, mon.shiny(), id, group, reset), "Pikachu -> Magikarp");
        const bool kept = party ? s.keepParty(*editor, 0, error) : s.keep(*editor, 0, 0, error);
        require(kept, std::string(id) + " keep: " + error);
        bool species = false, exp = false, level = false;
        for (const auto& [label, value] : PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor)) {
            species = species || (label.ends_with("/ Species") && value == "Pikachu -> Magikarp");
            exp = exp || (label.ends_with("/ Experience") && value == "10000 -> 11576");
            // The Party fixture's level cache (20) disagrees with its EXP (Lv 21); Gen IV shows the
            // EXP level, and the edited record's cache is rewritten to it -- a real change, shown.
            level = level || (label.ends_with("/ Level") && !(party && value == "20 -> 21"));
        }
        require(species && exp && !level, std::string(id) + (party ? " Party" : " Box") +
                                              ": Review shows Species and Experience, no Level row");
    }
}

void run(Layout layout, uint8_t romCode, const char* id) {
    reviewRows(layout, romCode, id);
    // Rattata Medium Fast, Volbeat Erratic, Makuhita Fluctuating, Bulbasaur Medium Slow, Snubbull
    // Fast, Magikarp Slow.
    const uint16_t species[] = {19, 313, 296, 1, 209, 129};
    for (const bool party : {false, true})
        for (const auto from : species)
            for (const auto to : species) {
                if (from == to) continue;
                for (const uint8_t level : {uint8_t(1), uint8_t(37), uint8_t(100)})
                    for (const bool mid : {false, true}) scenario(layout, romCode, id, party, from, to, level, mid);
            }
    // The owner's example: Lv 50 Medium Fast -> Slow keeps Lv 50 at 156,250 EXP, with the note; a
    // species change between equal growth rates keeps EXP exactly (Rattata -> Pikachu, both MF).
    auto source = makeSave(layout, romCode);
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
    const auto group = editor->sourceGroup();
    auto mon = *editor->editableBoxPokemon(0, 0, &error);
    require(mon.setExperience(130000), "Pikachu Lv 50 + 5,000");
    Session s;
    std::string beginError;
    require(s.begin(Pokemon::Pokemon4ReadOnly(mon.encryptedBytes(), group), group, Mode::Edit, beginError), "begin");
    bool reset = false;
    require(s.changeSpecies(19, mon.shiny(), id, group, reset) && s.working->experience() == 130000 &&
                s.speciesChangeExperienceNote().empty(),
            "same growth rate keeps EXP");
    require(s.changeSpecies(129, mon.shiny(), id, group, reset) && s.working->level() == 50 &&
                s.working->experience() == 156250,
            "Lv 50 Magikarp at 156,250");
    require(s.speciesChangeExperienceNote() == "Kept Lv. 50; EXP 130000 -> 156250 for the new species' growth rate",
            "note: " + s.speciesChangeExperienceNote());
}

} // namespace

int main() {
    require(Pokemon::getGrowthRate(19) == 0 && Pokemon::getGrowthRate(313) == 1 && Pokemon::getGrowthRate(296) == 2 &&
                Pokemon::getGrowthRate(1) == 3 && Pokemon::getGrowthRate(209) == 4 && Pokemon::getGrowthRate(129) == 5,
            "fixture growth rates");
    run(Layout::DiamondPearl, 10, "diamond_nds");
    run(Layout::DiamondPearl, 11, "pearl_nds");
    run(Layout::Platinum, 12, "platinum_nds");
    run(Layout::HeartGoldSoulSilver, 7, "heartgold_nds");
    run(Layout::HeartGoldSoulSilver, 8, "soulsilver_nds");
    std::cout << "test_species_change_level_gen4: OK\n";
    return 0;
}
