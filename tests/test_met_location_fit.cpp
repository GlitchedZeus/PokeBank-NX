// Round 5 NEW-10 (owner, Oct 24: Gen III-V Met Location popups "too wide"): the window is 704 px,
// close to the approved 640 px Crystal one, and every real row still fits unclipped: the place name
// (Body) and its encounter context (Caption) side by side in a scrolling row (rowW = 704 - 72).
// Every Gen IV/V origin x species x native location is measured with the worst-case "Stored: "
// prefix, and every Gen III encounter / "Keep stored" row likewise. Host-only (NunitoMetrics).
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/LocationDisplayNames.h"
#include "UI/SharedMenuGeometry.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace Names { const char* getSpeciesName(uint16_t species); }

namespace {
namespace G = PokeBank::UIModel::SharedMenuGeometry;
namespace C = PokeVault::Integration::EncounterContext45;
namespace E = PokeVault::Integration::EncounterGuardrails;

constexpr int kWidth = G::rule(G::MenuType::LocationPicker).width;
constexpr int kRowW = kWidth - 72;              // rows of a scrolling list (value-list gutter)
constexpr int kTextRoom = kRowW - 32 - 20;      // 16 px row padding each side, 20 px name/detail gap

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int fitsBoth(const std::string& name, const std::string& detail) {
    return G::textWidth(name, G::Text::Body) + G::textWidth(detail, G::Text::Caption);
}

void testWidthIsCompact() {
    static_assert(kWidth == 704, "Round 5 NEW-10 exact pin");
    static_assert(kWidth <= 720 && kWidth < 900, "close to Crystal's approved 640 px, narrower than before");
    for (const char* path : {"src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc"}) {
        const std::string src = read(path);
        assert(src.find("kMetLocationPickerWidth = 704;") != std::string::npos);
        // The detail column is fitted beside the measured name, not to a fixed half / remainder.
        assert(src.find("std::max(0, rowW - 32 - nw - 20)") != std::string::npos);
    }
    std::cout << "704 px compact Met Location window ok\n";
}

void testGen45RowsFit() {
    int widest = 0;
    std::string worst;
    for (uint8_t origin : {7, 8, 10, 11, 12, 20, 21, 22, 23}) {
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
        const int maxSpecies = C::generation(origin) == 4 ? 493 : 649;
        for (int species = 1; species <= maxSpecies; ++species) {
            const C::Catalogue catalogue(origin, static_cast<uint16_t>(species), native.front(), native, {});
            for (const auto location : catalogue.choices(true)) {
                const std::string name = Names::getMetLocationDisplayName(origin, location);
                std::string detail = catalogue.detail(location, [](uint16_t s) { return Names::getSpeciesName(s); });
                if (detail.rfind("Stored: ", 0) != 0) detail = "Stored: " + detail;   // worst case
                const int w = fitsBoth(name, detail);
                if (w > widest) { widest = w; worst = name + " | " + detail; }
            }
        }
    }
    if (widest > kTextRoom) std::cerr << "too wide: " << worst << " (" << widest << " > " << kTextRoom << ")\n";
    assert(widest <= kTextRoom);
    std::cout << "every Gen IV/V Met Location row fits (" << widest << " / " << kTextRoom << " px) ok\n";
}

void testGen3RowsFit() {
    int widest = 0;
    std::string worst;
    const std::pair<const char*, uint8_t> games[] = {
        {"ruby_gba", 2}, {"sapphire_gba", 1}, {"emerald_gba", 3}, {"firered_gba", 4}, {"leafgreen_gba", 5}};
    for (const auto& [id, origin] : games)
        for (uint16_t species = 1; species <= 386; ++species)
            for (const auto& choice : E::forGameSpeciesWithGen3Provenance(id, species)) {
                const auto& e = choice.encounter;
                // Same text as Gen3SharedPokemonSurface.inc metLocationChoiceText().
                std::string detail = std::string(E::methodName(e.method)) + " \xE2\x80\xA2 Lv. " + std::to_string(e.minLevel);
                if (e.maxLevel != e.minLevel) detail += "\xE2\x80\x93" + std::to_string(e.maxLevel);
                if (choice.evolved) {
                    detail += " \xE2\x80\xA2 met as " + std::string(Names::getSpeciesName(choice.originalEncounterSpecies));
                    if (choice.minimumEvolutionLevel != 0)
                        detail += " (evolves Lv. " + std::to_string(choice.minimumEvolutionLevel) + ")";
                }
                const std::string name = Names::getMetLocationDisplayName(origin, e.location);
                const int w = fitsBoth(name, detail);
                if (w > widest) { widest = w; worst = name + " | " + detail; }
            }
    for (uint8_t origin = 1; origin <= 5; ++origin)
        for (uint16_t location = 0; location < 400; ++location) {
            const std::string name = "Keep stored: " + Names::getMetLocationDisplayName(origin, location);
            const int w = fitsBoth(name, "Met Lv. 100 \xE2\x80\xA2 as stored");
            if (w > widest) { widest = w; worst = name; }
        }
    if (widest > kTextRoom) std::cerr << "too wide: " << worst << " (" << widest << " > " << kTextRoom << ")\n";
    assert(widest <= kTextRoom);
    std::cout << "every Gen III Met Location row fits (" << widest << " / " << kTextRoom << " px) ok\n";
}
} // namespace

int main() {
    testWidthIsCompact();
    testGen45RowsFit();
    testGen3RowsFit();
    std::cout << "test_met_location_fit: all passed\n";
    return 0;
}
