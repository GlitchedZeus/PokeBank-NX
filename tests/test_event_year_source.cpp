// Round 5 6B (R2-CHECK-011 follow-up): Gen IV / V event-year labels appear ONLY where the native
// location name itself carries the year marker. The names come from PKHeX text_hgss_03000_en.txt /
// text_bw2_40000_en.txt (src/Names/LocationNames.cpp, tools/gen_locations.py), verified at PKHeX
// 6042efa2: "Pokémon Event 06".."16" (Gen IV 3061..3071) and "Pokémon Event 10".."20" (Gen V
// 40074..40084). Every other event-named location keeps an unknown period. Host-only.
#include "Names/LocationDisplayNames.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string>

namespace {

std::string twoDigits(unsigned year) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02u", year % 100);
    return buf;
}

void testYearsComeFromNativeNames() {
    int labelled = 0;
    for (const uint8_t origin : {7, 8, 10, 11, 12, 20, 21, 22, 23})
        for (uint32_t location = 0; location <= 0xFFFF; ++location) {
            const auto id = static_cast<uint16_t>(location);
            const auto year = Names::metLocationEventYear(origin, id);
            const char* raw = Names::getMetLocationName(origin, id);
            const std::string native = raw ? raw : "";
            if (year) {
                // The label is the native name's own two-digit year marker, written out in full.
                assert(native == "Pok\xC3\xA9mon Event " + twoDigits(*year));
                assert(Names::getMetLocationDisplayName(origin, id) == "Pok\xC3\xA9mon Event " + std::to_string(*year));
                assert(Names::metLocationEventEvidence(origin, id).find("distribution and exact dates unverified") !=
                       std::string::npos);
                ++labelled;
            } else if (native.rfind("Pok\xC3\xA9mon Event ", 0) == 0) {
                std::cerr << "unlabelled year marker: " << int(origin) << " " << id << " " << native << '\n';
                assert(false);   // every native year marker is shown as its year
            }
        }
    assert(labelled == 5 * 11 + 4 * 11);   // Gen IV 06..16 for five origins, Gen V 10..20 for four
    // Event names without a year never gain one.
    assert(!Names::metLocationEventYear(22, 40061));   // "Game Event"
    assert(!Names::metLocationEventYear(12, 3060));    // "Pokémon Event"
    assert(Names::metLocationEventEvidence(22, 40061).find("period unknown") != std::string::npos);
    std::cout << "event years only from native year markers (" << labelled << " labels) ok\n";
}
} // namespace

int main() {
    testYearsComeFromNativeNames();
    std::cout << "test_event_year_source: all passed\n";
    return 0;
}
