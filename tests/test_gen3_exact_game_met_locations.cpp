// Round 11 FRLG (owner Extra 2: "leaf green and fire red on gba so gen 3 dont have a working met
// location for pokemon ... i dont think i can get a rayquaza in leaf green or fire red"). The Gen III
// Met Location picker was a closed list of the save's catch templates and refused to open for the
// 261 FireRed / 259 LeafGreen species with none (Rayquaza, Charizard, Raichu ...). It now offers,
// from the RECORD's origin game: the stored location (R2-BUG-008), the catch templates, then every
// other native location of that game labelled as unverified history. FireRed / LeafGreen get Kanto
// and Sevii ids only, R/S/E Hoenn ids only. The species list is never restricted (a traded Rayquaza
// is legitimately in an FRLG save).
#include "UI/Gen3EncounterPickerModel.h"
#include "Names/LocationDisplayNames.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {
namespace Picker = PokeBank::UIModel::Gen3EncounterPicker;
namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;

constexpr uint8_t kSapphire = 1, kRuby = 2, kEmerald = 3, kFireRed = 4, kLeafGreen = 5;

bool kanto(uint16_t id) { return (id >= 88 && id <= 196) || (id >= 253 && id <= 255); }
bool hoenn(uint16_t id) { return id <= 87 || (id >= 197 && id <= 212); }

std::vector<uint16_t> rowLocations(const std::vector<Encounter::EncounterProvenanceChoice>& choices,
                                   const std::vector<uint16_t>& rows, uint16_t stored) {
    std::vector<uint16_t> out;
    for (const auto row : rows) {
        if (row == Picker::kKeepStoredRow) out.push_back(stored);
        else if (Rows::isOtherNativeRow(row)) out.push_back(Rows::otherNativeRowLocation(row));
        else out.push_back(Picker::choiceAt(choices, row)->encounter.location);
    }
    return out;
}

void testNativeValidity() {
    for (const uint8_t frlg : {kFireRed, kLeafGreen}) {
        for (uint16_t id = 0; id <= 255; ++id)
            assert(Names::isGen3NativeMetLocation(frlg, id) == kanto(id));
        assert(!Names::isGen3NativeMetLocation(frlg, 0));     // Littleroot Town is Hoenn
        assert(!Names::isGen3NativeMetLocation(frlg, 85));    // Sky Pillar
        assert(Names::isGen3NativeMetLocation(frlg, 101));    // Route 1
        assert(Names::isGen3NativeMetLocation(frlg, 187));    // Birth Island (Kanto side)
    }
    for (const uint8_t rs : {kSapphire, kRuby})
        for (uint16_t id = 0; id <= 255; ++id)
            assert(Names::isGen3NativeMetLocation(rs, id) == (id <= 87 || id >= 253));
    for (uint16_t id = 0; id <= 255; ++id)
        assert(Names::isGen3NativeMetLocation(kEmerald, id) == (hoenn(id) || id >= 253));
    for (const uint8_t other : {uint8_t{0}, uint8_t{6}, uint8_t{15}, uint8_t{10}})
        for (uint16_t id = 0; id <= 255; ++id) assert(!Names::isGen3NativeMetLocation(other, id));
    std::cout << "Gen III native met locations per exact origin game ok\n";
}

void testOfferedListsAreExactGameAndNamed() {
    for (uint8_t origin = 1; origin <= 5; ++origin) {
        const auto offered = Rows::gen3OfferedLocations(origin);
        assert(!offered.empty());
        for (const auto id : offered) {
            assert(id != 0 && Names::isGen3NativeMetLocation(origin, id));
            const auto name = Names::getMetLocationDisplayName(origin, id);
            assert(!name.empty() && name != "Unknown location" && name != "None");
            if (origin >= 4) assert(kanto(id));
            else assert(hoenn(id) || id >= 253);
        }
        // Markers are offered (in-game trade, gift egg, fateful encounter) with their own labels.
        for (const uint16_t marker : {253, 254, 255})
            assert(std::find(offered.begin(), offered.end(), marker) != offered.end());
        assert(std::string(Rows::gen3OtherDetail(254)) == Rows::kTradeMarkerDetail);
        assert(std::string(Rows::gen3OtherDetail(255)) == Rows::kEventMarkerDetail);
        assert(std::string(Rows::gen3OtherDetail(253)) == Rows::kEggMarkerDetail);
        // Every row name is distinct within one game, so no two rows read the same.
        std::vector<std::string> names;
        for (const auto id : offered) {
            const auto name = Names::getMetLocationDisplayName(origin, id);
            if (std::find(names.begin(), names.end(), name) != names.end())
                std::cerr << "duplicate name " << name << " #" << id << " origin " << int(origin) << "\n";
            assert(std::find(names.begin(), names.end(), name) == names.end());
            names.push_back(name);
        }
        // PKHeX's "(unused)" duplicate sections are not offered (same displayed name as the real one).
        for (const uint16_t unused : {64, 75, 77})
            assert(std::find(offered.begin(), offered.end(), unused) == offered.end());
    }
    assert(Rows::gen3OfferedLocations(15).empty());   // Colosseum / XD: no GBA list
    std::cout << "offered lists are exact-game, named, markers labelled ok\n";
}

void testFrlgRayquazaHasKantoChoices() {
    for (const uint8_t origin : {kFireRed, kLeafGreen}) {
        // Create draft (nothing stored): the old picker refused ("No supported exact-game encounter
        // templates"). Now: no catch template, every row a Kanto / Sevii / marker row.
        const auto choices = Picker::templatesForOrigin(origin, 384);
        assert(choices.empty());
        const auto rows = Picker::rowValuesWithNative(choices, 0, origin);
        assert(!rows.empty() && rows.front() != Picker::kKeepStoredRow);
        for (const auto row : rows) {
            assert(Rows::isOtherNativeRow(row));
            assert(kanto(Rows::otherNativeRowLocation(row)));
        }
        const auto locations = rowLocations(choices, rows, 0);
        assert(std::find(locations.begin(), locations.end(), 85) == locations.end());   // no Sky Pillar
        assert(std::find(locations.begin(), locations.end(), 254) != locations.end());  // in-game trade
        assert(Picker::selectedRowWithNative(choices, rows, 0, 5) == 0);

        // Same for evolved species with no own wild slot (Charizard, Raichu, Dragonite).
        for (const uint16_t species : {6, 26, 149, 2, 65, 94})
            assert(!Picker::rowValuesWithNative(Picker::templatesForOrigin(origin, species), 0, origin).empty());
    }
    // Emerald Rayquaza: Sky Pillar is a real catch template there.
    const auto emerald = Picker::templatesForOrigin(kEmerald, 384);
    assert(std::any_of(emerald.begin(), emerald.end(), [](const auto& c) { return c.encounter.location == 85; }));
    const auto emeraldRows = Picker::rowValuesWithNative(emerald, 0, kEmerald);
    for (const auto location : rowLocations(emerald, emeraldRows, 0))
        assert(hoenn(location) || location >= 253);
    std::cout << "FireRed / LeafGreen Rayquaza and evolved species have Kanto choices ok\n";
}

void testTemplatesFirstThenOtherNativeWithoutDuplicates() {
    // FireRed Bulbasaur: the Pallet Town gift template first, then every other Kanto id once.
    const auto choices = Picker::templatesForOrigin(kFireRed, 1);
    assert(!choices.empty());
    const auto rows = Picker::rowValuesWithNative(choices, 0, kFireRed);
    assert(rows.size() == choices.size() + (Rows::gen3OfferedLocations(kFireRed).size() -
           [&] { std::vector<uint16_t> l; for (const auto& c : choices)
                     if (std::find(l.begin(), l.end(), c.encounter.location) == l.end()) l.push_back(c.encounter.location);
                 return l.size(); }()));
    for (std::size_t i = 0; i < choices.size(); ++i) assert(rows[i] == i);
    assert(choices.front().encounter.location == 88);
    for (std::size_t i = choices.size(); i < rows.size(); ++i) {
        assert(Rows::isOtherNativeRow(rows[i]));
        assert(Rows::otherNativeRowLocation(rows[i]) != 88);
        assert(Picker::choiceAt(choices, rows[i]) == nullptr);   // never mistaken for a template
    }
    // Focus on a stored template location stays on the template.
    assert(Picker::selectedRowWithNative(choices, rows, 88, 5) == 0);
    std::cout << "templates first, then other native rows, no duplicates ok\n";
}

void testStoredLocationKeptAndNotRepeated() {
    // FireRed Charizard stored at Route 1 (101): keep-stored row first, Route 1 not repeated.
    const auto choices = Picker::templatesForOrigin(kFireRed, 6);
    const auto rows = Picker::rowValuesWithNative(choices, 101, kFireRed);
    assert(rows.front() == Picker::kKeepStoredRow);
    assert(std::count(rows.begin(), rows.end(), Rows::otherNativeRow(101)) == 0);
    assert(Picker::selectedRowWithNative(choices, rows, 101, 30) == 0);
    // A stored Hoenn id on an FRLG record (written by the old Create) is kept, not re-offered.
    const auto legacy = Picker::rowValuesWithNative(choices, 1, kFireRed);
    assert(legacy.front() == Picker::kKeepStoredRow);
    for (std::size_t i = 1; i < legacy.size(); ++i) assert(kanto(Rows::otherNativeRowLocation(legacy[i])));
    // Colosseum / XD origin: only the stored value can be kept.
    const auto cxd = Picker::rowValuesWithNative(Picker::templatesForOrigin(15, 25), 255, 15);
    assert(cxd.size() == 1 && cxd.front() == Picker::kKeepStoredRow);
    assert(Picker::rowValuesWithNative(Picker::templatesForOrigin(15, 25), 0, 15).empty());
    std::cout << "stored location kept first and not repeated ok\n";
}

void testRecordOriginNotSaveGame() {
    // A Ruby-origin Pikachu (in any save): Ruby templates (Safari Zone 57) and Hoenn ids only.
    const auto ruby = Picker::templatesForOrigin(kRuby, 25);
    assert(!ruby.empty() && ruby.front().encounter.location == 57);
    const auto rows = Picker::rowValuesWithNative(ruby, 0, kRuby);
    for (const auto location : rowLocations(ruby, rows, 0)) assert(location <= 87 || location >= 253);
    // A FireRed-origin Pikachu: Viridian Forest / Power Plant, Kanto only.
    const auto fr = Picker::templatesForOrigin(kFireRed, 25);
    assert(std::any_of(fr.begin(), fr.end(), [](const auto& c) { return c.encounter.location == 126; }));
    for (const auto location : rowLocations(fr, Picker::rowValuesWithNative(fr, 0, kFireRed), 0))
        assert(kanto(location));
    // The provenance edge still reads "met as Horsea (evo. 32)" from the record's own origin game.
    const auto seadra = Picker::templatesForOrigin(kSapphire, 117);
    assert(!seadra.empty() && seadra.front().evolved && seadra.front().minimumEvolutionLevel == 32);
    std::cout << "templates follow the record's origin game ok\n";
}
} // namespace

int main() {
    testNativeValidity();
    testOfferedListsAreExactGameAndNamed();
    testFrlgRayquazaHasKantoChoices();
    testTemplatesFirstThenOtherNativeWithoutDuplicates();
    testStoredLocationKeptAndNotRepeated();
    testRecordOriginNotSaveGame();
    std::cout << "test_gen3_exact_game_met_locations: all passed\n";
    return 0;
}
