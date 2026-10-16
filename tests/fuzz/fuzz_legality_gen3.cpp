// libFuzzer harness for Legality::analyzePk3 (Gen III, read-only).
//   clang++ -std=c++20 -g -O1 -fsanitize=fuzzer,address,undefined -Wno-unused-parameter \
//     -Iinclude tests/fuzz/fuzz_legality_gen3.cpp src/Legality/Gen3Legality.cpp \
//     src/Legality/Gen3LegalityData.cpp src/Names/SpeciesNames.cpp \
//     src/Names/MoveNames.cpp -o fuzz_gen3
// The first input byte picks the exact source game and whether the checksum is fixed
// up, so the fuzzer reaches the rules past the checksum gate.
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <vector>

#include "Legality/Gen3Legality.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    static constexpr std::string_view kGames[] = {"ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba",
                                                  "leafgreen_gba", "", "colosseum_gc", "diamond_nds"};
    const std::string_view game = kGames[data[0] & 7];
    std::vector<uint8_t> pk(data + 1, data + size);
    if ((data[0] & 8) == 0 && (pk.size() == 80 || pk.size() == 100)) {
        const uint16_t sum = Legality::Gen3Pk3::checksum(pk);
        pk[0x1C] = static_cast<uint8_t>(sum);
        pk[0x1D] = static_cast<uint8_t>(sum >> 8);
    }
    const auto copy = pk;
    const auto report = Legality::analyzePk3(pk, game);
    if (pk != copy) std::abort();  // the analyzer must never write to its input
    // Only an exact GBA cartridge source can ever certify a record.
    if (report.verdict() == Legality::Verdict::NoProblemsFound && (data[0] & 7) >= 5) std::abort();
    return 0;
}
