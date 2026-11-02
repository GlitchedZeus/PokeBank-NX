// Round 14 (owner Round 13 R06 Gen V: "all pokemon seem the have the same met location : mystery zone,
// link trade, PKMN Breedeer, other location"; H05 "pikachu has werid met locations"; shared-UX audit
// P1-9: one met-location caption and order for Crystal and Gen III-V). Integration/Encounter/MetListScope.h.
//
//  1. Measurement: the Gen V wild data IS wired and matches PKHeX 6042efa2 encounter_{b,w,b2,w2}.pkl
//     (species counts below); what empties a new Lv. 5 Pokemon's list is the R12-005 level rule.
//     The verified-species counts per level are pinned so a data or wiring regression shows here.
//  2. The DEFAULT caption says WHY a list has no verified row: "No catch at Lv. 5; N need Lv. X+
//     (unverified list)" or "Not in this game's wild / gift data; markers only".
//  3. The UNVERIFIED list (Gen IV / V) puts the "level too low" places right after the stored row,
//     lowest level first (Gen III / Crystal already do), and loses or adds no row.
//  4. Pikachu across Gen III-V: every list is explainable from the data.
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Encounter/MetListScope.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "UI/Gen3EncounterPickerModel.h"

#include <algorithm>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

namespace E = PokeVault::Integration::EncounterContext45;
namespace S = PokeVault::Integration::MetListScope;
namespace Policy = PokeVault::Integration::MetLocationPolicy;
namespace G3 = PokeBank::UIModel::Gen3EncounterPicker;

namespace {
int failures = 0;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

const std::vector<uint16_t> kNative5(PokeVault::Integration::Gen5::NativeMetLocations5.begin(),
                                     PokeVault::Integration::Gen5::NativeMetLocations5.end());
E::Catalogue gen5(uint8_t origin, uint16_t species, uint8_t level, uint16_t stored = 0) {
    return E::Catalogue(origin, species, stored, kNative5, {}, level);
}
std::size_t verifiedSpecies(uint8_t origin, uint8_t level) {
    std::size_t n = 0;
    for (uint16_t s = 1; s <= 649; ++s) n += gen5(origin, s, level).verifiedCount() > 0;
    return n;
}
std::size_t wildSpecies(std::string_view game) {
    std::set<uint16_t> species;
    for (const auto& e : E::kWild45) if (e.sourceGameId == game) species.insert(e.species);
    return species.size();
}
} // namespace

int main() {
    // 1. Measurement (Gen V). PKHeX encounter_*.pkl species counts, independently parsed: 231 / 264 / 286 / 286.
    CHECK(wildSpecies("black_nds") == 231 && wildSpecies("white_nds") == 264);
    CHECK(wildSpecies("black2_nds") == 286 && wildSpecies("white2_nds") == 286);
    // Species (of 649) with at least one verified row in Black at Lv. 5 / 30 / 50 / 100 (wild + gift /
    // static / trade, species or a pre-evolution). Lv. 5 is what Create starts at: 23 of 649.
    const std::size_t black[] = {verifiedSpecies(21, 5), verifiedSpecies(21, 30), verifiedSpecies(21, 50), verifiedSpecies(21, 100)};
    std::printf("  Black verified species at Lv. 5/30/50/100: %zu / %zu / %zu / %zu of 649\n", black[0], black[1], black[2], black[3]);
    CHECK(black[0] == 23 && black[1] == 193 && black[2] == 310 && black[3] == 321);
    const std::size_t b2[] = {verifiedSpecies(23, 5), verifiedSpecies(23, 30), verifiedSpecies(23, 50), verifiedSpecies(23, 100)};
    std::printf("  Black 2 verified species at Lv. 5/30/50/100: %zu / %zu / %zu / %zu of 649\n", b2[0], b2[1], b2[2], b2[3]);
    CHECK(b2[0] == 47 && b2[1] == 220 && b2[2] == 358 && b2[3] == 417);

    // 2 / 3. Black Pidove (519) Lv. 5: no verified row, two places need a higher level (Route 3 Lv. 8+
    // first). Before Round 14 the caption only said "No verified encounter here".
    {
        const auto c = gen5(21, 519, 5);
        CHECK(c.verifiedCount() == 0);
        const auto gap = S::levelGap(c);
        CHECK(gap.count() == 2 && gap.lowest == 8 && gap.level == 5);
        CHECK(S::defaultScope(c) == "No catch at Lv. 5; 2 need Lv. 8+ (unverified list)");
        const auto list = S::unverifiedChoices(c);
        CHECK(list.size() == c.choices(true).size());
        CHECK(std::is_permutation(list.begin(), list.end(), c.choices(true).begin()));
        CHECK(list[0] == c.stored() && list[1] == 16 && list[2] == 33);   // level too low, lowest first
        CHECK(c.detail(list[1], [](uint16_t) { return std::string("Pidove"); }).find(Policy::kLevelBelowEncounter) != std::string::npos);
        // At Lv. 30 those rows are verified again and move to the DEFAULT list.
        const auto high = gen5(21, 519, 30);
        CHECK(high.verifiedCount() >= 2 && S::levelGap(high).count() == 0 && S::defaultScope(high) == Policy::kDefaultListScope);
    }
    // Black Lillipup (506) Lv. 5: Route 1 / 2 verified, one more place needs Lv. 9+.
    {
        const auto c = gen5(21, 506, 5);
        CHECK(c.verifiedCount() == 2 && S::defaultScope(c) == "Verified + markers; 1 more needs Lv. 9+");
    }
    // The stored placeholder 0 (Mystery Zone) of a new Pokemon is "no place recorded", not a gift.
    CHECK(gen5(21, 25, 5).detail(0, [](uint16_t) { return std::string("Pikachu"); }) == "Stored: no place recorded; unverified");

    // 4. Pikachu (H05) across Gen IV / V. Black / White / Black 2 / White 2: PKHeX has no wild, gift,
    //    static or trade Pikachu or Pichu, so at ANY level the list is the stored row, the trade
    //    markers and "Other location (unverified)..." -- and the caption now says so.
    for (const uint8_t origin : {20, 21, 22, 23})
        for (const uint8_t level : {5, 100}) {
            const auto c = gen5(origin, 25, level);
            CHECK(c.verifiedCount() == 0 && S::levelGap(c).count() == 0 && S::defaultScope(c) == S::kNotInGameData);
        }
    // Diamond / Platinum: Trophy Garden only, above Lv. 5 (Lv. 18+ / 22+).
    CHECK(S::defaultScope(E::Catalogue(10, 25, 0, {}, {}, 5)) == "No catch at Lv. 5; 1 needs Lv. 18+ (unverified list)");
    CHECK(S::defaultScope(E::Catalogue(12, 25, 0, {}, {}, 5)) == "No catch at Lv. 5; 1 needs Lv. 22+ (unverified list)");
    CHECK(E::Catalogue(10, 25, 0, {}, {}, 18).verifiedEncounter(68));
    // HeartGold: Viridian Forest Lv. 4 is verified at Lv. 5; the Lv. 30 Pichu row waits for Lv. 30.
    {
        const E::Catalogue c(7, 25, 0, {}, {}, 5);
        CHECK(c.verifiedEncounter(224) && S::defaultScope(c) == "Verified + markers; 1 more needs Lv. 30+");
    }
    // Gen III Ruby Pikachu (Safari Zone): the Gen III picker's own rows, the shared caption.
    {
        const auto choices = G3::templatesForOrigin(2, 25);
        CHECK(!choices.empty());
        for (const uint8_t level : {5, 25}) {
            const auto rows = G3::defaultRows(choices, 0, 2, level);
            S::LevelGap gap;
            gap.level = level;
            for (const auto& choice : choices)
                if (!G3::levelReached(choice, level)) gap.add(G3::encounterOf(choice).location, G3::encounterOf(choice).minLevel);
            const auto scope = S::defaultScope(G3::verifiedRowCount(choices, rows), gap);
            std::printf("  Ruby Pikachu Lv. %u: %zu verified rows; \"%s\"\n", level, G3::verifiedRowCount(choices, rows), scope.c_str());
            if (level == 5) CHECK(G3::verifiedRowCount(choices, rows) == 0 && scope.rfind("No catch at Lv. 5; 1 needs Lv. ", 0) == 0);
            else CHECK(G3::verifiedRowCount(choices, rows) > 0 && scope == Policy::kDefaultListScope);
        }
    }
    // The caption for a list with no data and no level gap never claims a catch.
    CHECK(S::defaultScope(0, S::LevelGap{}) == S::kNotInGameData);

    if (failures) {
        std::fprintf(stderr, "%d met-list scope check(s) failed\n", failures);
        return 1;
    }
    std::puts("Met Location list caption and order (Round 14 R06 / H05 / P1-9: Gen V data measured, "
              "level-too-low explained and listed first, Pikachu Gen III-V): PASS");
    return 0;
}
