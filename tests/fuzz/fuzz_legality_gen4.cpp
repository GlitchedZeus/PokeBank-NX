// libFuzzer harness for Legality::analyzePk4 and the Gen IV hook in Legality::analyze
// (read-only). Build with Clang and libFuzzer (Makefile.host.gen4-legality):
//   make -f Makefile.host CXX=clang++ HOST_BUILD=build/fuzz build/fuzz/fuzz_legality_gen4
// The first input byte picks the path: bit 0 set sends the bytes as a stored (encrypted)
// record through the PK4 reader and Legality::analyze; otherwise they are a decrypted
// record whose checksum is fixed up so the fuzzer gets past the checksum gate. Bits 1-3
// pick the save's game.
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <span>
#include <vector>

#include "Legality/Gen4Legality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
constexpr std::array<const char*, 5> kGames = {"diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds"};
constexpr std::array<Enums::GameVersion, 5> kGroups = {Enums::GameVersion::DP, Enums::GameVersion::DP, Enums::GameVersion::PT,
                                                       Enums::GameVersion::HGSS, Enums::GameVersion::HGSS};
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    const bool stored = (data[0] & 1) != 0;
    const size_t game = (data[0] >> 1) % kGames.size();
    std::vector<uint8_t> pk(data + 1, data + size);
    if (stored) {
        std::vector<std::byte> raw(pk.size());
        std::transform(pk.begin(), pk.end(), raw.begin(), [](uint8_t b) { return static_cast<std::byte>(b); });
        const auto copy = raw;
        const Pokemon::Pokemon4ReadOnly source(raw, kGroups[game]);
        const Pokemon::Pokemon4ReadOnlyView view(source);
        const auto report = Legality::analyze(view, kGroups[game], kGames[game]);
        if (raw != copy) std::abort();  // nothing may write to the input
        (void)report.verdict();
        return 0;
    }
    if (pk.size() == 136 || pk.size() == 236) {
        const uint16_t sum = Legality::Gen4::checksum(pk);
        pk[6] = static_cast<uint8_t>(sum);
        pk[7] = static_cast<uint8_t>(sum >> 8);
    }
    const auto copy = pk;
    const auto report = Legality::analyzePk4(pk, kGames[game]);
    if (pk != copy) std::abort();  // the analyzer must never write to its input
    (void)report.verdict();
    return 0;
}
