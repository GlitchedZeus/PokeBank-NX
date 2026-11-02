// Round 13 recheck regressions (docs/qa/RECHECK_REGRESSIONS.md): Hardware Accepted behaviour whose
// code changed after its accepted candidate, re-asserted on the CHANGED code paths. Display rules only.
//
// PBX-MET-004 (owner PASS R3 62a2f8c3 L03: Gen III Met Location names readable, no (RSE)/(FRLG)/ID
//   numbers). Changed after 62a2f8c3: 01951ab7 (R11 exact-origin native list) and c78a8328 (R12-004 /
//   R12-005 default list = verified rows + markers + "Other location (unverified)...", and the
//   unverified list with templates above the level). The existing tests pin the name text only for
//   template rows; this walks EVERY row of BOTH lists the R12 model builds (defaultRows /
//   unverifiedRows -> rowText) for every origin x species x level.
// PBX-MET-003 (owner PASS R1 7fea1797 / R3 62a2f8c3 L02: Crystal chooser names only, no raw IDs).
//   Changed after 62a2f8c3: 03387c48 (R11 open native list) and c78a8328 (R12 default / unverified
//   lists, level-blocked and landmark rows). test_gen2_location_picker checks names-only for catch
//   templates (locationRowText); this checks every row locationRowTextAt draws in both lists.
// PBX-LNG-009 (Gen I/II store no language and show no Language row; Gen III-V show the stored
//   language). Shared field contract re-asserted after the R11 (d1954551) / R12 (7ecf5c99) language
//   changes. The Gen I/II "no Language row drawn" check is in test_ui_recheck_regressions_host.
#include "Names/LocationDisplayNames.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/Gen3EncounterPickerModel.h"
#include "UI/SharedPokemonEditorContract.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        if (++failures <= 20) std::cerr << "FAILED: " << what << '\n';
    }
}

// A developer-facing token in a place name: a raw id ("123", "0x7E", "#12", "ID 5") or a source tag
// in parentheses made of capitals only ("(RSE)", "(FRLG)", "(E)", "(GSC)").
std::string rawToken(const std::string& place) {
    if (place.empty()) return "empty name";
    if (place.find("0x") != std::string::npos || place.find("0X") != std::string::npos) return "0x id";
    if (place.find('#') != std::string::npos) return "# id";
    if (place.find("ID ") != std::string::npos || place.find("Id ") != std::string::npos) return "ID label";
    bool allDigits = true;
    for (const char c : place) allDigits = allDigits && std::isdigit(static_cast<unsigned char>(c));
    if (allDigits) return "bare number";
    for (std::size_t open = place.find('('); open != std::string::npos; open = place.find('(', open + 1)) {
        const auto close = place.find(')', open);
        if (close == std::string::npos) break;
        const std::string inner = place.substr(open + 1, close - open - 1);
        bool tag = !inner.empty();
        for (const char c : inner) tag = tag && (std::isupper(static_cast<unsigned char>(c)) || c == '/');
        if (tag) return "source tag (" + inner + ")";
        bool number = !inner.empty();
        for (const char c : inner) number = number && std::isdigit(static_cast<unsigned char>(c));
        if (number) return "number in parentheses";
    }
    for (const char* bad : {"Unknown location", "Unused", "invalid", "Location "})
        if (place.find(bad) != std::string::npos) return std::string("placeholder \"") + bad + "\"";
    return {};
}

void gen3MetRowsNamesOnly() {
    namespace Picker = PokeBank::UIModel::Gen3EncounterPicker;
    namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
    int rows = 0;
    for (uint8_t origin = 1; origin <= 5; ++origin) {           // Sapphire, Ruby, Emerald, FireRed, LeafGreen
        const auto offered = Rows::gen3OfferedLocations(origin);
        const std::vector<uint16_t> storedValues{0, offered.front(), offered.back()};
        for (uint16_t species = 1; species <= 386; ++species) {
            const auto choices = Picker::templatesForOrigin(origin, species);
            for (const uint16_t stored : storedValues)
                for (const uint8_t level : {uint8_t{1}, uint8_t{50}, uint8_t{100}}) {
                    for (const bool unverified : {false, true}) {
                        const auto values = unverified ? Picker::unverifiedRows(choices, stored, origin, level)
                                                       : Picker::defaultRows(choices, stored, origin, level);
                        if (!unverified) check(!values.empty(), "Gen III default list is never empty");
                        for (const auto value : values) {
                            const auto text = Picker::rowText(choices, value, origin, stored, 5, level);
                            ++rows;
                            if (value == Picker::kUnverifiedListRow) {
                                check(text.place == PokeVault::Integration::MetLocationPolicy::kUnverifiedListRowPlace,
                                      "Gen III unverified-list entry row keeps its own place text");
                                continue;
                            }
                            std::string place = text.place;
                            if (value == Picker::kKeepStoredRow) {
                                check(place.rfind("Keep stored: ", 0) == 0, "Gen III keep-stored row is labelled");
                                place.erase(0, 13);
                            }
                            const auto bad = rawToken(place);
                            check(bad.empty(), "Gen III origin " + std::to_string(origin) + " species " +
                                                   std::to_string(species) + " row \"" + text.place + "\": " + bad);
                            // R12 detail text keeps the accepted honesty: a row never says "legal".
                            check(text.detail.find("legal") == std::string::npos &&
                                      text.detail.find("Legal") == std::string::npos,
                                  "Gen III row detail never claims legality: " + text.detail);
                        }
                    }
                }
        }
    }
    check(rows > 100000, "Gen III rows walked: " + std::to_string(rows));
    std::cout << "Gen III Met Location rows (R12 default + unverified lists): names only, " << rows << " rows\n";
}

void crystalMetRowsNamesOnly() {
    namespace P = PokeBank::UIModel::Gen2PokemonPicker;
    int rows = 0;
    for (uint16_t species = 1; species <= 251; ++species)
        for (const uint8_t level : {uint8_t{0}, uint8_t{1}, uint8_t{100}})
            for (const bool unverified : {false, true}) {
                P::Model m;
                m.openLocation("crystal_gbc", species, 0, 0, level, unverified);
                if (!unverified) check(m.locationCount() > 0, "Crystal default list is never empty");
                std::set<std::string> seen;
                for (int i = 0; i < m.locationCount(); ++i) {
                    const auto text = P::locationRowTextAt(m, i);
                    ++rows;
                    if (m.unverifiedEntryAt(i)) continue;   // "Other location (unverified)..." opens a list
                    const auto bad = rawToken(text.place);
                    check(bad.empty(), "Crystal species " + std::to_string(species) + " row \"" + text.place +
                                           "\": " + bad);
                    check(text.detail.find("legal") == std::string::npos && text.detail.find("Legal") == std::string::npos,
                          "Crystal row detail never claims legality: " + text.detail);
                }
            }
    check(rows > 10000, "Crystal rows walked: " + std::to_string(rows));
    std::cout << "Crystal Met Location rows (R12 default + unverified lists): names only, " << rows << " rows\n";
}

void languageOnlyWhereStored() {
    namespace SE = PokeBank::UIModel::SharedPokemonEditor;
    using G = SE::Generation;
    using F = SE::FieldIdentity;
    using A = SE::FieldAccess;
    // Gen I / II records have no language byte: never shown (Crystal caught data does not change that).
    for (const bool crystal : {false, true}) {
        check(SE::fieldAccessForGeneration(G::Gen1, F::Language, crystal) == A::Hidden, "Gen I has no Language row");
        check(SE::fieldAccessForGeneration(G::Gen2, F::Language, crystal) == A::Hidden, "Gen II has no Language row");
    }
    // PK3 0x12 / PK4 0x17 / PK5 0x17 store it: shown and edited as the stored byte.
    for (const auto g : {G::Gen3, G::Gen4, G::Gen5})
        check(SE::fieldAccessForGeneration(g, F::Language) == A::Editable,
              "Gen " + std::to_string(static_cast<int>(g)) + " shows the stored Language");
    std::cout << "Language only where the format stores it\n";
}
} // namespace

int main() {
    // The detector itself: the R3-era labels it must catch, and real names it must accept.
    for (const char* bad : {"Route 101 (RSE)", "Viridian Forest (FRLG)", "123", "0x7E", "Place #12", "Location 40"})
        check(!rawToken(bad).empty(), std::string("detector flags \"") + bad + "\"");
    for (const char* good : {"Route 101", "Mt. Moon", "S.S. Anne", "Pok\xC3\xA9mon Tower", "Ruins of Alph"})
        check(rawToken(good).empty(), std::string("detector accepts \"") + good + "\"");
    gen3MetRowsNamesOnly();
    crystalMetRowsNamesOnly();
    languageOnlyWhereStored();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "test_recheck_met_language: " << checks << " checks PASS\n";
    return EXIT_SUCCESS;
}
