// M1 D2/D3: localized default species names (PKHeX SpeciesName.GetSpeciesNameGeneration 8e3eb3d4).
// Every Gen III species in every Gen III language yields a name the native Gen III table can encode
// and that fits the record language's nickname limit, so a blank-nickname Create never fails.
#include "Names/DefaultSpeciesName.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <cassert>
#include <iostream>
#include <string>

namespace {
std::string u8(uint16_t species, uint8_t language, uint8_t generation) {
    return Utils::utf16ToUtf8(Names::defaultSpeciesName(species, language, generation));
}
}  // namespace

int main() {
    // Spot values from PKHeX's rule.
    assert(u8(1, 1, 3) == "フシギダネ");
    assert(u8(1, 2, 3) == "BULBASAUR" && u8(1, 3, 3) == "BULBIZARRE" && u8(1, 5, 3) == "BISASAM");
    assert(u8(4, 3, 3) == "SALAMECHE");                  // French E accent stripped (Gen III/IV)
    assert(u8(83, 2, 3) == "FARFETCH'D");                // straight apostrophe
    assert(u8(83, 2, 5) == "Farfetch'd");                // Gen V: listed case, straight apostrophe
    assert(u8(4, 3, 5) == "Salamèche");                  // Gen V keeps the accent
    assert(u8(25, 1, 5) == "ピカチュウ" && u8(25, 5, 5) == "Pikachu");
    assert(u8(25, 2, 4) == "PIKACHU");                   // Gen IV unchanged by the refactor

    int checked = 0;
    for (uint16_t species = 1; species <= 386; ++species) {
        for (const uint8_t language : {1, 2, 3, 4, 5, 7}) {
            const auto name = Names::defaultSpeciesName(species, language, 3);
            assert(!name.empty());
            const std::size_t limit = language == 1 ? 5 : 10;
            if (name.size() > limit) {
                std::cerr << "species " << species << " lang " << int(language) << " too long\n";
                assert(false);
            }
            for (const char16_t ch : name) {
                if (Utils::charToGen3(ch, language) == Utils::GEN3_TERMINATOR) {
                    std::cerr << "species " << species << " lang " << int(language)
                              << " has no Gen III glyph for U+" << std::hex << int(ch) << std::dec << "\n";
                    assert(false);
                }
            }
            ++checked;
        }
    }
    assert(checked == 386 * 6);
    std::cout << "Default species names: PKHeX rule spot values; 2316 Gen III names encodable and in limits: PASS\n";
}
