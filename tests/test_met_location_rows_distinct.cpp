// Round 6 P5 (R2-CHECK-011, owner: "several rows read POKÉPARK, and Festa rows differ only by
// number"): in every Gen IV / V Met Location list the picker can show, no two visible rows are
// identical. A row is what the draw shows: the picker name (Names::getMetLocationChoiceName) and
// the EncounterContext45 detail. Every origin x species x stored location x list (the X key's two
// lists) is checked. Evidence: PKHeX 6501f0ab text_bw2_40000_{ja,de,fr}.txt number the POKÉPARK
// entries 10..20; same-name place ids show their native id with an "unverified" detail.
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/LocationDisplayNames.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace Names { const char* getSpeciesName(uint16_t species); }

namespace {
namespace C = PokeVault::Integration::EncounterContext45;

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::vector<uint16_t> nativeIds(uint8_t origin) {
    std::vector<uint16_t> native;
    if (C::generation(origin) == 4) {
        const auto t = Names::getLocationTable(origin);
        for (size_t i = 0; i < t.count; ++i)
            if (t.names && t.names[i] && t.names[i][0] && Names::isGen4NativeMetLocation(origin, static_cast<uint16_t>(i)))
                native.push_back(static_cast<uint16_t>(i));
    } else {
        native.assign(PokeVault::Integration::Gen5::NativeMetLocations5.begin(),
                      PokeVault::Integration::Gen5::NativeMetLocations5.end());
    }
    return native;
}

void testPokeParkAndFesta() {
    for (uint8_t origin : {20, 21, 22, 23}) {
        assert(Names::getMetLocationDisplayName(origin, 40097) == "POK\xC3\x89PARK");
        for (uint16_t id = 40098; id <= 40108; ++id) {
            const std::string number = std::to_string(10 + id - 40098);
            assert(Names::getMetLocationDisplayName(origin, id) == "POK\xC3\x89PARK " + number);
            assert(Names::metLocationEventEvidence(origin, id) ==
                   "Native name number " + number + "; meaning, distribution and period unverified");
        }
        for (uint16_t id = 40086; id <= 40096; ++id) {
            const std::string number = std::to_string(10 + id - 40086);
            assert(Names::getMetLocationDisplayName(origin, id) == "Pok\xC3\xA9mon Festa " + number);
            assert(Names::metLocationEventEvidence(origin, id).find("meaning, distribution and period unverified") !=
                   std::string::npos);
            assert(!Names::metLocationEventYear(origin, id));   // no year is claimed for Festa
        }
    }
    // Gen IV origins are untouched by the Gen V bank.
    assert(!Names::metLocationNativeNumber(12, 40098) && !Names::metLocationNativeNumber(7, 40086));
    std::cout << "POKÉPARK 10..20 from native ja/de/fr text, Festa numbers without a claimed year ok\n";
}

void testTwinsShowTheirId() {
    // Same place name under two native ids: the later id shows itself; field values stay plain.
    assert(Names::getMetLocationChoiceName(22, 9) == "Nimbasa City");
    assert(Names::getMetLocationChoiceName(22, 76) == "Nimbasa City #76");
    assert(Names::getMetLocationDisplayName(22, 76) == "Nimbasa City");
    assert(Names::getMetLocationChoiceName(20, 30001) == "Unknown location");
    assert(Names::getMetLocationChoiceName(20, 30012) == "Unknown location #30012");
    assert(Names::getMetLocationChoiceName(21, 30014) == "Entralink #30014");
    assert(Names::getMetLocationChoiceName(23, 40002) == "Faraway place #40002");
    for (const uint8_t origin : {7, 8, 10, 11, 12, 20, 21, 22, 23})
        for (const uint16_t id : nativeIds(origin))
            assert(Names::getMetLocationDisplayName(origin, id).find(" #") == std::string::npos);
    // Unknown-history twins say so in the detail column.
    const C::Catalogue c(22, 25, 4, nativeIds(22), {});
    const auto name = [](uint16_t s) { return Names::getSpeciesName(s); };
    assert(c.detail(76, name).find("unverified") != std::string::npos);
    assert(c.detail(30012, name).find("unverified") != std::string::npos);
    std::cout << "same-name ids show their native id in the picker only ok\n";
}

void testNoIdenticalVisibleRows() {
    long lists = 0, rows = 0;
    for (const uint8_t origin : {7, 8, 10, 11, 12, 20, 21, 22, 23}) {
        const auto native = nativeIds(origin);
        std::set<std::string> names;
        for (const uint16_t id : native) {   // every name the picker can show is unique on its own
            const std::string choice = Names::getMetLocationChoiceName(origin, id);
            if (!names.insert(choice).second) std::cerr << "repeated name " << int(origin) << " " << id << " " << choice << '\n';
        }
        assert(names.size() == native.size());
        const int maxSpecies = C::generation(origin) == 4 ? 493 : 649;
        const auto speciesName = [](uint16_t s) { return Names::getSpeciesName(s); };
        for (int species = 1; species <= maxSpecies; ++species)
            for (const uint16_t stored : {native.front(), uint16_t{76}, uint16_t{40098}, native.back()}) {
                const C::Catalogue catalogue(origin, static_cast<uint16_t>(species), stored, native, {});
                for (const bool other : {false, true}) {
                    std::set<std::string> seen;
                    for (const auto location : catalogue.choices(other)) {
                        const std::string row = Names::getMetLocationChoiceName(origin, location) + "  |  " +
                                                catalogue.detail(location, speciesName);
                        if (!seen.insert(row).second) {
                            std::cerr << "identical rows: origin " << int(origin) << " species " << species
                                      << " list " << other << ": " << row << '\n';
                            assert(false);
                        }
                        ++rows;
                    }
                    ++lists;
                }
            }
    }
    std::cout << "no two identical rows in " << lists << " Gen IV/V lists (" << rows << " rows) ok\n";
}

void testWiring() {
    {
        const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
        const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
        assert(gen4.find("? Names::getMetLocationChoiceName(preview->originVersion(), value)") != std::string::npos);
        assert(gen5.find("std::string label = Names::getMetLocationChoiceName(p ? p->originVersion() : 20, value);") !=
               std::string::npos);
        // The compact popup and the 1821c921 stored-row guard stay (tests/test_met_location_fit.cpp).
        const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
        assert(shared.find("kMetLocationPickerWidth = 620;") != std::string::npos);
        assert(shared.find("storedRow ? Colors::Accent : Colors::Text") != std::string::npos);
    }
    std::cout << "Gen IV / V picker labels use the choice name ok\n";
}
} // namespace

int main() {
    testPokeParkAndFesta();
    testTwinsShowTheirId();
    testNoIdenticalVisibleRows();
    testWiring();
    std::cout << "test_met_location_rows_distinct: all passed\n";
    return 0;
}
