// R2-BUG-004, Generation IV: Diamond, Pearl, Platinum, HeartGold, SoulSilver through the real
// Pokemon4Mutable draft and Gen4StagedPokemonEditor, ending in a strict export -> reparse. The
// draft goes through exactly what applyGen4Picker's species branch runs: setSpecies, the shiny
// preview, then SpeciesMoveDefaults::setMoves with the DP / Pt / HGSS table (test_species_change_
// moves_gen15 pins that the UI function calls it). New species' native moves (PKHeX lvlmove_*),
// base PP, no PP Ups; the same species changes nothing; only the edited slot record and the
// storage footer change; Review describes the slot; an uncommitted draft changes nothing.
#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Names/MoveInfo.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SpeciesChangeMoves.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;
namespace Moves = PokeBank::Moves;
namespace Reset = PokeBank::UIModel::SpeciesMoveDefaults;

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

// applyGen4Picker species branch, step for step.
bool applySpecies(Pokemon::Pokemon4Mutable& mon, Enums::GameVersion group, uint16_t value) {
    const uint16_t previousSpecies = mon.species();
    const bool shiny = mon.shiny();
    bool ok = mon.setSpecies(value);
    if (ok && mon.shiny() != shiny) ok = mon.setShiny(shiny);
    if (ok && value != previousSpecies)
        ok = Reset::setMoves(mon, Reset::forSpecies(Reset::gen4TableForGroup(group), value, mon.form(), mon.level(),
                                                    [group](uint16_t move) { return Names::getMoveBasePP(move, group); }));
    return ok;
}

void checkNative(const Pokemon::Pokemon4Mutable& mon, Enums::GameVersion group, const std::string& what) {
    const auto d = Moves::defaultMoves(Reset::gen4TableForGroup(group), mon.species(), mon.form(), mon.level());
    require(d.known && mon.moves() == d.moves, what + ": native default moves");
    for (std::size_t i = 0; i < 4; ++i) {
        require(mon.ppUps()[i] == 0, what + ": PP Ups 0");
        require(mon.pp()[i] == Names::getMoveBasePP(mon.moves()[i], group), what + ": base PP");
    }
}

void run(Layout layout, uint8_t romCode, const char* id) {
    auto source = makeSave(layout, romCode);
    const auto original = source;
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
    require(editor && error.empty(), std::string(id) + " editor: " + error);
    const auto group = editor->sourceGroup();
    require(Reset::gen4TableForGroup(group) != Moves::LevelUpTable15::Count, "a Gen IV table for the group");
    const auto untouched = editor->stagedBytes();

    auto mon = *editor->editableBoxPokemon(0, 0, &error);
    require(mon.species() == 25, "fixture Pikachu");
    require(mon.setMove(0, 84) && mon.setPPUps(0, 2), "give it Thunder Shock with PP Ups");
    const auto ivs = mon.ivs(), evs = mon.evs();
    const uint8_t nature = mon.nature();
    const bool shiny = mon.shiny();

    // Several transitions with different learnsets.
    for (const uint16_t species : {22, 1, 387, 133, 448, 25}) {
        require(applySpecies(mon, group, species), std::string(id) + " species " + std::to_string(species));
        require(mon.species() == species, "species applied");
        checkNative(mon, group, std::string(id) + " -> " + std::to_string(species));
        require(species == 25 || mon.moves()[0] != 84, "Thunder Shock never survives a change away from Pikachu");
        require(mon.ivs() == ivs && mon.evs() == evs && mon.nature() == nature && mon.shiny() == shiny,
                "IVs, EVs, nature and shininess kept");
        // Same species: not one byte changes, even after a hand edit.
        require(mon.setMove(3, 33), "hand edit");
        const auto before = mon.encryptedBytes();
        require(applySpecies(mon, group, species), "reselect");
        require(mon.encryptedBytes() == before, "same species changes nothing");
    }
    require(editor->stagedBytes() == untouched, "an uncommitted draft changes no staged byte");

    // Pikachu -> Fearow, commit, Review, export -> strict reparse, byte scope.
    auto fearow = *editor->editableBoxPokemon(0, 0, &error);
    require(applySpecies(fearow, group, 22), "Pikachu -> Fearow");
    require(editor->commitBoxPokemon(0, 0, fearow, &error), std::string(id) + " commit: " + error);
    const auto review = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
    require(!review.empty(), "Review describes the changed slot");
    const auto out = editor->finalizedBytes(&error);
    require(!out.empty() && error.empty(), std::string(id) + " export: " + error);
    const auto sp = spec(layout);
    const size_t record = sp.storageStart + sp.boxData;
    const size_t footer = sp.storageStart + sp.storageSize - sp.footerSize;
    for (size_t i = 0; i < out.size(); ++i) {
        if (out[i] == original[i]) continue;
        const bool inRecord = i >= record && i < record + Encryption::SIZE_STORED4;
        const bool inFooter = i >= footer && i < sp.storageStart + sp.storageSize;
        require(inRecord || inFooter, std::string(id) + ": byte " + std::to_string(i) + " outside the edited slot changed");
    }
    auto parsed = Gen4ReadOnlySave::parse(out, layout, id, &error);
    require(parsed && error.empty(), std::string(id) + " strict reparse: " + error);
    const auto& back = parsed->box(0, 0);
    require(back.valid() && back.species() == 22, "reparsed Fearow");
    require(back.moves() == fearow.moves() && back.pp() == fearow.pp() && back.ppUps() == fearow.ppUps(),
            "reparsed moves, PP, PP Ups");
    require(back.ivs() == ivs && back.evs() == evs, "reparsed IVs / EVs");
    require(source == original, "source bytes never change");
}

} // namespace

int main() {
    run(Layout::DiamondPearl, 10, "diamond_nds");
    run(Layout::DiamondPearl, 11, "pearl_nds");
    run(Layout::Platinum, 12, "platinum_nds");
    run(Layout::HeartGoldSoulSilver, 7, "heartgold_nds");
    run(Layout::HeartGoldSoulSilver, 8, "soulsilver_nds");
    std::cout << "Gen IV species change resets moves (Diamond, Pearl, Platinum, HeartGold, SoulSilver; commit, "
                 "Review, export, reparse, byte scope): PASS\n";
    return 0;
}
