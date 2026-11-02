// Round 13 owner decisions 5 / 6 ("Add proper egg-group and breeding compatibility data. No generic
// egg histories presented as verified." / "Support Gen IV Day-Care/trade location markers where
// native semantics and history are verified; distinguish unverified choices."). For Diamond, Pearl,
// Platinum, HeartGold and SoulSilver:
//  * the Met Location catalogue (EncounterContext45::Catalogue, the Gen IV picker's model) offers
//    "Hatched from a Day-Care egg" and the 2001 / 2002 trade markers as VERIFIED rows only when the
//    breeding data / in-game trade data and the level verify them, and otherwise only in the
//    unverified list with the reason;
//  * what the picker writes (Pokemon4Mutable::setHatchedEggHistory / setMetLocation(2001 / 2002),
//    the calls applyGen4Picker makes) goes through Create Keep, export and a strict reparse with the
//    PKHeX G4PKM two-field encoding (D/P field 0x80 / 0x7E, Pt/HGSS extended field 0x46 / 0x44);
//  * a Create species change to a species that never hatches clears the hatched fields.
#include "Encryption/Encryption4.h"
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Names/LocationNames.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
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
namespace Context = PokeVault::Integration::EncounterContext45;
namespace Policy = PokeVault::Integration::MetLocationPolicy;
using PokeBank::UIModel::Gen4SharedEditor::Session;

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


std::vector<uint16_t> nativeList(uint8_t origin) {
    std::vector<uint16_t> native;
    const auto table = Names::getLocationTable(origin);
    for (size_t id = 1; id < table.count && id <= 0xFFFFu; ++id)
        if (table.names && table.names[id] && table.names[id][0] != '\0' &&
            Names::isGen4NativeMetLocation(origin, static_cast<uint16_t>(id)))
            native.push_back(static_cast<uint16_t>(id));
    return native;
}

bool has(const std::vector<uint16_t>& rows, uint16_t value) {
    return std::find(rows.begin(), rows.end(), value) != rows.end();
}

std::string speciesName(uint16_t species) { return std::to_string(species); }

// The picker model: where a row lands (default = verified, unverified = with a reason).
void catalogue(uint8_t origin) {
    const bool hgss = origin == 7 || origin == 8;
    const auto native = nativeList(origin);
    const auto open = [&](uint16_t species, uint8_t level, Context::History history = {}) {
        return Context::Catalogue(origin, species, 0, native, history, level);
    };
    const std::string tag = "origin " + std::to_string(origin);
    // Hatched from a Day-Care egg (Gen IV eggs hatch at Lv. 1).
    {
        const auto pichu = open(172, 1);
        require(has(pichu.choices(), Policy::kHatchedEggRow) && !has(pichu.choices(true), Policy::kHatchedEggRow) &&
                    pichu.detail(Policy::kHatchedEggRow, speciesName) == "Hatched Day-Care egg",
                tag + ": Pichu Lv. 1 hatched row verified");
        const auto raichu = open(26, 40);
        require(has(raichu.choices(), Policy::kHatchedEggRow) &&
                    raichu.detail(Policy::kHatchedEggRow, speciesName) == "Hatched Day-Care egg via 172",
                tag + ": Raichu hatched via Pichu");
        for (const auto& [species, reason] : {std::pair<uint16_t, const char*>{150, "Undiscovered"}, {132, "Ditto"},
                                              {490, "Phione"}, {201, "Undiscovered"}, {487, "Undiscovered"}}) {
            const auto c = open(species, 50);
            require(!has(c.choices(), Policy::kHatchedEggRow) && has(c.choices(true), Policy::kHatchedEggRow) &&
                        c.detail(Policy::kHatchedEggRow, speciesName).find(reason) != std::string::npos,
                    tag + ": species " + std::to_string(species) + " egg row only unverified: " +
                        c.detail(Policy::kHatchedEggRow, speciesName));
        }
        const auto phione = open(489, 1);
        require(has(phione.choices(), Policy::kHatchedEggRow), tag + ": Phione hatches (from Manaphy / Phione)");
        const auto nidoqueen = open(31, 40);
        require(has(nidoqueen.choices(), Policy::kHatchedEggRow) &&
                    nidoqueen.detail(Policy::kHatchedEggRow, speciesName).find("via 29") != std::string::npos,
                tag + ": Nidoqueen via Nidoran F");
        const auto chansey = open(113, 20);
        require(has(chansey.choices(), Policy::kHatchedEggRow) &&
                    chansey.detail(Policy::kHatchedEggRow, speciesName) == "Hatched Day-Care egg",
                tag + ": Chansey hatches itself without Luck Incense");
        // 2000 is an Egg Location value: it is never offered as a Met Location.
        require(!has(pichu.choices(), 2000) && !has(pichu.choices(true), 2000), tag + ": 2000 never a Met Location row");
    }
    // 2001: this exact game's in-game trades (and their evolutions) at or above the trade level.
    {
        const uint16_t trade = hgss ? 95 : 93;         // HG/SS Onix Lv. 1, D/P/Pt Haunter Lv. 35
        const uint16_t evolved = hgss ? 208 : 94;      // Steelix (also its own HG/SS trade) / Gengar
        const uint8_t tradeLevel = hgss ? 1 : 35;
        const auto here = open(trade, tradeLevel);
        require(has(here.choices(), 2001) && here.verifiedEncounter(2001) &&
                    here.detail(2001, speciesName).find("Lv. " + std::to_string(tradeLevel)) != std::string::npos,
                tag + ": in-game trade verified: " + here.detail(2001, speciesName));
        const auto evo = open(evolved, 50);
        require(has(evo.choices(), 2001), tag + ": an evolved in-game trade keeps 2001 verified");
        if (!hgss) {
            const auto low = open(93, 30);
            require(!has(low.choices(), 2001) && has(low.choices(true), 2001) &&
                        low.detail(2001, speciesName).find(Policy::kLevelBelowEncounter) != std::string::npos,
                    tag + ": Haunter Lv. 30 below the Lv. 35 trade");
        }
        const uint16_t otherGamesTrade = hgss ? 93 : 95;   // Haunter is not an HG/SS trade; Onix not D/P/Pt
        const auto other = open(otherGamesTrade, 40);
        require(!has(other.choices(), 2001) && has(other.choices(true), 2001) &&
                    other.detail(2001, speciesName) == "NPC trade; not this species here",
                tag + ": another game's trade stays unverified: " + other.detail(2001, speciesName));
        const auto mewtwo = open(150, 70);
        require(!has(mewtwo.choices(), 2001) && has(mewtwo.choices(true), 2001), tag + ": Mewtwo 2001 unverified");
    }
    // 2002: the Met Location of an egg traded before hatching.
    {
        const auto pikachu = open(25, 20);
        require(!has(pikachu.choices(), 2002) && has(pikachu.choices(true), 2002) &&
                    pikachu.detail(2002, speciesName) == "Link trade; only an unhatched egg",
                tag + ": a hatched / caught Pokemon never has 2002 verified");
        const auto egg = open(172, 1, Context::History{2000, true, false});
        require(has(egg.choices(), 2002) && egg.detail(2002, speciesName) == "Egg traded before hatching",
                tag + ": a Pichu egg traded before hatching");
        require(!has(egg.choices(), Policy::kHatchedEggRow) && !has(egg.choices(true), Policy::kHatchedEggRow),
                tag + ": an unhatched egg gets no hatched-egg row");
        const auto pikachuEgg = open(25, 1, Context::History{2000, true, false});
        require(!has(pikachuEgg.choices(), 2002), tag + ": no Pikachu egg exists (eggs hatch as Pichu)");
    }
    // Lists stay disjoint and complete: every row once, stored row first in both.
    for (const uint16_t species : {1, 25, 93, 150, 172, 490}) {
        const auto c = open(species, 40);
        std::vector<uint16_t> all(c.choices().begin(), c.choices().end());
        all.insert(all.end(), c.choices(true).begin() + 1, c.choices(true).end());
        auto sorted = all;
        std::sort(sorted.begin(), sorted.end());
        require(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end(), tag + ": no row in both lists");
        require(has(all, Policy::kHatchedEggRow) && has(all, 2001) && has(all, 2002), tag + ": every marker reachable");
    }
    // The editor / Review "Met evidence" note for a hatched record.
    const uint16_t hatch = Policy::dayCareHatchLocation(origin);
    require(Context::unverifiedMetNote45(origin, 172, hatch, 1, 2000, 0).empty(), tag + ": hatched Pichu, no note");
    require(Context::unverifiedMetNote45(origin, 26, hatch, 30, 2002, 0).empty(), tag + ": traded-egg Raichu, no note");
    require(Context::unverifiedMetNote45(origin, 150, hatch, 70, 2000, 0).find("Undiscovered") != std::string::npos,
            tag + ": hatched Mewtwo is labelled");
    // (a verified data row carries encdata's "single source (PKHeX)" evidence note, never "unverified")
    require(Context::unverifiedMetNote45(origin, hgss ? 95 : 93, 2001, hgss ? 1 : 35).find("unverified") == std::string::npos,
            tag + ": trade, no unverified note");
    require(Context::unverifiedMetNote45(origin, 25, 2002, 20).find("unverified") != std::string::npos,
            tag + ": a non-egg 2002 stays unverified");
}

uint16_t extendedMet(const Pokemon::Pokemon4ReadOnly& r) { return r.metLocationExtended(); }

void writeExportReparse(Layout layout, uint8_t romCode, const char* id) {
    auto source = makeSave(layout, romCode);
    const auto original = source;
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, id, &error);
    require(editor && error.empty(), std::string(id) + " editor: " + error);
    const auto group = editor->sourceGroup();
    const bool hgss = layout == Layout::HeartGoldSoulSilver;
    const bool dp = layout == Layout::DiamondPearl;

    struct Pick { uint16_t species; int kind; };   // 0 hatched, 1 = 2001, 2 = 2002
    const Pick picks[] = {{172, 0}, {static_cast<uint16_t>(hgss ? 208 : 93), 1}, {25, 2}};
    size_t slot = 1;
    std::vector<std::pair<size_t, Pick>> staged;
    for (const auto& pick : picks) {
        auto draft = editor->createBoxDraft(0, slot, pick.species, &error);
        require(draft.has_value(), std::string(id) + " Create draft: " + error);
        Session c;
        require(c.beginCreate(std::move(*draft), error), "beginCreate: " + error);
        const uint8_t origin = c.working->encryptedBytes().empty() ? 0 :
            Pokemon::Pokemon4ReadOnly(c.working->encryptedBytes(), group).originVersion();
        if (pick.kind == 0) {
            require(c.working->setHatchedEggHistory(), std::string(id) + ": write hatched-egg history");
            require(c.hatchedEggHistory(group), "the draft reads as a hatched egg");
        } else {
            if (pick.kind == 1 && !hgss) require(c.working->setLevel(35), "Haunter Lv. 35");
            require(c.working->setMetLocation(pick.kind == 1 ? 2001 : 2002), std::string(id) + ": write trade marker");
        }
        c.noteMetLocationChosen();
        require(c.working->setMove(0, 33), "a move");
        // What the picker showed for this record is the catalogue's verdict.
        const Context::Catalogue model(origin, pick.species, 0, nativeList(origin), {}, c.working->level());
        const uint16_t row = pick.kind == 0 ? Policy::kHatchedEggRow : pick.kind == 1 ? 2001 : 2002;
        require(has(pick.kind == 2 ? model.choices(true) : model.choices(), row),
                std::string(id) + ": the row is in the expected list");
        require(c.keepCreate(*editor, 0, slot, error), std::string(id) + " keepCreate: " + error);
        staged.push_back({slot, pick});
        ++slot;
    }
    // Review names the Egg Location and keeps the honest note for the unverified 2002 Pikachu.
    const auto rows = PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor);
    bool eggRow = false, linkNote = false;
    for (const auto& [label, value] : rows) {
        eggRow |= label.find("Egg Location") != std::string::npos && value.find("Day-Care") != std::string::npos;
        linkNote |= label.find("Slot 4 / Met evidence") != std::string::npos && value.find("unverified") != std::string::npos;
    }
    require(eggRow, std::string(id) + ": Review shows Egg Location Day-Care Couple");
    require(linkNote, std::string(id) + ": Review labels the unverified 2002 choice");
    const auto out = editor->finalizedBytes(&error);
    require(!out.empty() && error.empty(), std::string(id) + " export: " + error);
    auto parsed = Gen4ReadOnlySave::parse(out, layout, id, &error);
    require(parsed && error.empty(), std::string(id) + " strict reparse: " + error);
    for (const auto& [at, pick] : staged) {
        const auto& back = parsed->box(0, at);
        const Pokemon::Pokemon4ReadOnlyView view(back);
        require(back.valid() && back.species() == pick.species, std::string(id) + ": reparsed species");
        if (pick.kind == 0) {
            const uint16_t hatch = hgss ? 182 : 4;
            require(view.metLocation() == hatch && back.metLevel() == 0 && view.eggLocation() == 2000,
                    std::string(id) + ": reparsed hatched egg (Met Location, Met Level 0, Egg Location 2000)");
            require(back.metLocationDP() == (hgss ? 3002 : 4) && extendedMet(back) == (dp ? 0 : hatch),
                    std::string(id) + ": PKHeX Met Location fields");
            require(back.eggLocationDP() == 2000 && back.eggLocationExtended() == (dp ? 0 : 2000),
                    std::string(id) + ": PKHeX Egg Location fields");
            require(Context::unverifiedMetNote45(back.originVersion(), back.species(), view.metLocation(),
                                                 static_cast<uint8_t>(view.level()), view.eggLocation(),
                                                 back.metLevel()).empty(),
                    std::string(id) + ": the reparsed hatched Pichu reads verified");
        } else {
            const uint16_t marker = pick.kind == 1 ? 2001 : 2002;
            require(view.metLocation() == marker && back.metLocationDP() == marker &&
                        extendedMet(back) == (dp ? 0 : marker),
                    std::string(id) + ": reparsed trade marker " + std::to_string(marker));
        }
    }
    require(source == original, "source bytes never change");
    editor->discard();

    // A Create species change away from a hatchable species clears the hatched fields (H25 reset).
    auto draft = editor->createBoxDraft(0, 10, 172, &error);
    require(draft.has_value(), "Create Pichu");
    Session c;
    require(c.beginCreate(std::move(*draft), error), "beginCreate");
    require(c.working->setHatchedEggHistory(), "hatched Pichu");
    c.noteMetLocationChosen();
    bool reset = false;
    require(c.changeSpecies(26, false, id, group, reset) && !reset && c.hatchedEggHistory(group),
            std::string(id) + ": Raichu keeps the hatched history (via Pichu)");
    require(c.changeSpecies(150, false, id, group, reset) && reset && !c.hatchedEggHistory(group),
            std::string(id) + ": Mewtwo clears it");
    const Pokemon::Pokemon4ReadOnly cleared(c.working->encryptedBytes(), group);
    require(Pokemon::Pokemon4ReadOnlyView(cleared).eggLocation() == 0, std::string(id) + ": Egg Location cleared");
    // Values outside the native markers are refused, byte-identical.
    const auto before = c.working->encryptedBytes();
    require(!c.working->setEggLocation(2001) && !c.working->setEggLocation(3002) && !c.working->setMetLocation(2000) &&
                !c.working->setMetLocation(2003) && c.working->encryptedBytes() == before,
            std::string(id) + ": 2000 is never a Met Location; other ids refused");
}

} // namespace

int main() {
    for (const uint8_t origin : {10, 11, 12, 7, 8}) catalogue(origin);
    writeExportReparse(Layout::DiamondPearl, 10, "diamond_nds");
    writeExportReparse(Layout::DiamondPearl, 11, "pearl_nds");
    writeExportReparse(Layout::Platinum, 12, "platinum_nds");
    writeExportReparse(Layout::HeartGoldSoulSilver, 7, "heartgold_nds");
    writeExportReparse(Layout::HeartGoldSoulSilver, 8, "soulsilver_nds");
    std::cout << "Gen IV Day-Care / trade markers (Diamond, Pearl, Platinum, HeartGold, SoulSilver): verified only "
                 "with breeding / trade data; write, Review, export, strict reparse: PASS\n";
}
