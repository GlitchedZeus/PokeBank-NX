// Round 5 NEW-10 (owner, Oct 24: Gen III-V Met Location popups "too wide"; "closer to Gen II's
// dimensions", no clipping): the window is 620 px, and every real row still shows its place name
// (Body) and its encounter context (Caption) in full on one line of a scrolling list
// (rowW = 620 - 44 - scrollbar gutter 28). The draw code fits the detail beside the measured name,
// so a row fits exactly when name + gap + detail <= rowW - 32. Every Gen IV/V origin x species x
// location the picker can list is measured with the real EncounterContext45 text, and every Gen
// III encounter / Keep-stored row with the same text Gen3SharedPokemonSurface.inc builds.
// Host-only (pinned Nunito metrics).
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
constexpr int kRowW = kWidth - 44 - 28;          // scrolling list: SharedScrollbar gutter
constexpr int kTextRoom = kRowW - 32 - 20;       // 16 px padding each side, 20 px name/detail gap

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int rowWidth(const std::string& name, const std::string& detail) {
    return G::textWidth(name, G::Text::Body) + G::textWidth(detail, G::Text::Caption);
}

void testCompactAndFittedBesideTheName() {
    static_assert(kWidth == 620, "Codex's Oct 24 compact Met Location window");
    static_assert(kWidth <= 640, "no wider than Crystal's approved 640 px window");
    for (const char* path : {"src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc"}) {
        const std::string src = read(path);
        assert(src.find("kMetLocationPickerWidth = 620;") != std::string::npos);
        // The detail column takes what the measured name leaves, never a fixed half / remainder.
        assert(src.find("std::max(0, rowW - 32 - nw - 20)") != std::string::npos);
        assert(src.find("std::max(0, rowW - 260)") == std::string::npos);
        assert(src.find("(rowW - 32) / 2") == std::string::npos);
    }
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    assert(shared.find("if (storedRow && example.rfind(\"Stored: \", 0) == 0 && ew > rowW - 32 - nw - 20)") != std::string::npos);
    assert(shared.find("storedRow ? Colors::Accent : Colors::Text") != std::string::npos);
    // The header line (x + 165, fitted to w - 195) shows either scope in full.
    const C::Catalogue any(20, 25, 4, {4}, {});
    for (const bool other : {false, true})
        assert(G::textWidth(any.scope(other), G::Text::Caption) <= kWidth - 195);
    std::cout << "620 px window, detail fitted beside the name ok\n";
}

void testGen45RowsFit() {
    int widest = 0, rows = 0;
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
        for (int species = 1; species <= maxSpecies; ++species)
            // Each species is measured with the stored location at every encounter spot it has (the
            // "Stored: " row) and with no stored match, over both lists the X key shows.
            for (const uint16_t stored : {native.front(), native.back()}) {
                const C::Catalogue catalogue(origin, static_cast<uint16_t>(species), stored, native, {});
                for (const bool other : {false, true})
                    for (const auto location : catalogue.choices(other)) {
                        const std::string name = Names::getMetLocationChoiceName(origin, location);   // P5 row name
                        const auto speciesName = [](uint16_t s) { return Names::getSpeciesName(s); };
                        std::string detail = catalogue.detail(location, speciesName);
                        if (catalogue.wildRow(location)) {
                            // The same spot as the record's stored location reads "Stored: <example>";
                            // the draw drops that prefix only when the row would not fit (the name is
                            // then accent-coloured and the header says "row 1 = stored").
                            const C::Catalogue storedHere(origin, static_cast<uint16_t>(species), location, native, {});
                            std::string asStored = storedHere.detail(location, speciesName);
                            if (asStored.rfind("Stored: ", 0) == 0 && rowWidth(name, asStored) > kTextRoom)
                                asStored.erase(0, 8);
                            if (rowWidth(name, asStored) > rowWidth(name, detail)) detail = asStored;
                        }
                        ++rows;
                        const int w = rowWidth(name, detail);
                        if (w > widest) { widest = w; worst = name + " | " + detail; }
                    }
            }
    }
    if (widest > kTextRoom) std::cerr << "clipped: " << worst << " (" << widest << " > " << kTextRoom << ")\n";
    assert(widest <= kTextRoom);
    std::cout << "every Gen IV/V Met Location row fits (" << rows << " rows, widest " << widest << " / "
              << kTextRoom << " px) ok\n";
}

void testGen3RowsFit() {
    // The exact text Gen3SharedPokemonSurface.inc metLocationChoiceText() builds.
    const std::string src = read("src/UI/Gen3SharedPokemonSurface.inc");
    assert(src.find("\" Lv. \" + std::to_string(encounter.minLevel);") != std::string::npos);
    assert(src.find("detail += \", met as \" + std::string(Names::getSpeciesName(selected->originalEncounterSpecies));") != std::string::npos);
    assert(src.find("detail += \" (evo. \" + std::to_string(selected->minimumEvolutionLevel) + \")\";") != std::string::npos);
    assert(src.find("\"Met Lv. \" + std::to_string(stored.metLevel)};") != std::string::npos);
    int widest = 0, rows = 0;
    std::string worst;
    const std::pair<const char*, uint8_t> games[] = {
        {"ruby_gba", 2}, {"sapphire_gba", 1}, {"emerald_gba", 3}, {"firered_gba", 4}, {"leafgreen_gba", 5}};
    for (const auto& [id, origin] : games)
        for (uint16_t species = 1; species <= 386; ++species)
            for (const auto& choice : E::forGameSpeciesWithGen3Provenance(id, species)) {
                const auto& e = choice.encounter;
                std::string detail = std::string(E::methodName(e.method)) + " Lv. " + std::to_string(e.minLevel);
                if (e.maxLevel != e.minLevel) detail += "\xE2\x80\x93" + std::to_string(e.maxLevel);
                if (choice.evolved) {
                    detail += ", met as " + std::string(Names::getSpeciesName(choice.originalEncounterSpecies));
                    if (choice.minimumEvolutionLevel != 0)
                        detail += " (evo. " + std::to_string(choice.minimumEvolutionLevel) + ")";
                }
                ++rows;
                const std::string name = Names::getMetLocationDisplayName(origin, e.location);
                const int w = rowWidth(name, detail);
                if (w > widest) { widest = w; worst = name + " | " + detail; }
            }
    for (uint8_t origin = 1; origin <= 5; ++origin)
        for (uint16_t location = 0; location < 400; ++location) {
            const std::string name = "Keep stored: " + Names::getMetLocationDisplayName(origin, location);
            ++rows;
            const int w = rowWidth(name, "Met Lv. 100");
            if (w > widest) { widest = w; worst = name; }
        }
    if (widest > kTextRoom) std::cerr << "clipped: " << worst << " (" << widest << " > " << kTextRoom << ")\n";
    assert(widest <= kTextRoom);
    std::cout << "every Gen III Met Location row fits (" << rows << " rows, widest " << widest << " / "
              << kTextRoom << " px) ok\n";
}
} // namespace

int main() {
    testCompactAndFittedBesideTheName();
    testGen45RowsFit();
    testGen3RowsFit();
    std::cout << "test_met_location_fit: all passed\n";
    return 0;
}
