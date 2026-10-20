// NEW-12 / R2-CHECK-011 (Round 3): Gen IV / V Met Location context. The generated encounter table
// (tools/gen_gen45_encounters.py, PKHeX 6501f0ab) lists where each species is met; the picker shows
// those spots first with method / level and keeps every other native location as an egg / trade /
// event origin. Host-only.
#include "Integration/Encounter/Gen45Encounters.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/LocationNames.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
namespace E = PokeVault::Integration::Gen45Encounters;

constexpr uint8_t kVersionOf[10] = {0, 10, 11, 12, 7, 8, 21, 20, 23, 22};   // table game -> GameVersion

bool nativeLocation(uint8_t game, uint16_t location) {
    if (game <= 5) return Names::isGen4NativeMetLocation(kVersionOf[game], location);
    const auto& list = PokeVault::Integration::Gen5::NativeMetLocations5;
    return std::find(list.begin(), list.end(), location) != list.end();
}

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void testTableIsNative() {
    int perGame[10]{};
    for (const auto& e : E::kGen45Encounters) {
        assert(e.game >= 1 && e.game <= 9);
        assert(e.species >= 1 && e.species <= (e.game <= 5 ? 493 : 649));
        assert(e.minLevel >= 1 && e.minLevel <= e.maxLevel && e.maxLevel <= 100);
        // Every spot is a location the picker offers, except PKHeX's HG/SS water slots at 234, which
        // lies outside Names::isGen4NativeMetLocation's 126..233; orderBySpots skips a spot the
        // native list does not offer, so such a row is simply not promoted (never invented).
        assert(nativeLocation(e.game, e.location) || ((e.game == 4 || e.game == 5) && e.location == 234));
        assert(E::gameForVersion(kVersionOf[e.game]) == e.game);
        ++perGame[e.game];
    }
    for (int g = 1; g <= 9; ++g) assert(perGame[g] > 100);   // every Gen IV / V game has data
    assert(E::gameForVersion(3) == 0 && E::gameForVersion(0) == 0);   // Emerald / none: no table
    std::cout << "table is native ok\n";
}

void testKnownEncounters() {
    // Owner screenshot: SoulSilver Walrein. Walrein is not wild in HG/SS; its line is (Spheal).
    const auto walrein = E::spotsFor(E::gameForVersion(8), 365);
    assert(!walrein.empty());
    for (const auto& s : walrein) assert(s.metAs == 363 || s.metAs == 364 || s.metAs == 365);
    assert(std::any_of(walrein.begin(), walrein.end(), [](const E::Spot& s) { return s.metAs == 363; }));
    // Diamond Pikachu: Trophy Garden (68), Lv. 18 (PKHeX encounter_d).
    const auto pikachuD = E::spotsFor(E::gameForVersion(10), 25);
    assert(std::any_of(pikachuD.begin(), pikachuD.end(), [](const E::Spot& s) {
        return s.location == 68 && s.metAs == 25 && s.minLevel == 18; }));
    // R2-CHECK-011: Black 2 has no wild / gift Pikachu, so no location is claimed as its encounter.
    assert(E::spotsFor(E::gameForVersion(23), 25).empty());
    // Static gifts with a fixed location: Platinum Turtwig at Route 201 (16), Lv. 5.
    const auto turtwig = E::spotsFor(E::gameForVersion(12), 387);
    assert(std::any_of(turtwig.begin(), turtwig.end(), [](const E::Spot& s) {
        return s.location == 16 && s.method == 40 && s.minLevel == 5; }));
    // Pre-evolutions: Raichu -> Pikachu -> Pichu.
    assert(E::preEvolution(26) == 25 && E::preEvolution(25) == 172 && E::preEvolution(172) == 0);
    std::cout << "known encounters ok\n";
}

void testOrderingNeverRejects() {
    std::vector<uint16_t> values{1, 2, 3, 4, 5, 6};
    const std::vector<E::Spot> spots{{5, 25, 3, 5, 0}, {2, 172, 1, 1, 40}, {5, 25, 9, 9, 1}, {77, 25, 1, 2, 0}};
    const int leading = E::orderBySpots(values, spots);
    assert(leading == 2);                                               // 5 and 2 (77 is not offered)
    assert((values == std::vector<uint16_t>{5, 2, 1, 3, 4, 6}));       // every native value is kept
    const auto name = [](uint16_t s) { return s == 172 ? std::string("Pichu") : std::string("?"); };
    assert(E::detailFor(spots, 5, 25, name) == "Grass Lv. 3\xE2\x80\x93" "5");
    assert(E::detailFor(spots, 2, 25, name) == "as Pichu: Gift / static Lv. 1");
    assert(E::detailFor(spots, 4, 25, name) == E::kOtherOriginDetail);   // other origin, not rejected
    std::vector<uint16_t> none{1, 2, 3};
    assert(E::orderBySpots(none, {}) == 0 && (none == std::vector<uint16_t>{1, 2, 3}));
    std::cout << "ordering keeps every native location ok\n";
}

void testWiring() {
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    assert(shared.find("state.metSpotCount = E::orderBySpots(values, state.metSpots);") != std::string::npos);
    assert(gen4.find("Surface::prepareMetLocations(state, state.pickerValues, record.originVersion(), record.species());") != std::string::npos);
    assert(gen4.find("Surface::metLocationDetail(state, value, preview->species())") != std::string::npos);
    assert(gen5.find("EditorSurface::prepareMetLocations(state, values, p.originVersion(), p.species());") != std::string::npos);
    assert(gen5.find("EditorSurface::metLocationDetail(state, value, p->species())") != std::string::npos);
    // The stored Met Lv. is no longer repeated on every row.
    assert(gen4.find("label += \"  |  Met Lv. \" + std::to_string(preview->metLevel());") == std::string::npos);
    assert(gen5.find("if (p) label += \"  |  Met Lv. \" + std::to_string(p->metLevel());") == std::string::npos);
    std::cout << "Gen IV / V met location wiring ok\n";
}
} // namespace

int main() {
    testTableIsNative();
    testKnownEncounters();
    testOrderingNeverRejects();
    testWiring();
    std::cout << "test_gen45_encounters: all passed\n";
    return 0;
}
