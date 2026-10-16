// libFuzzer harness for Legality::analyzePk5 (Gen V, read-only).
//   clang++ -std=c++20 -g -O1 -fsanitize=fuzzer,address,undefined -Iinclude \
//     tests/fuzz/fuzz_legality_gen5.cpp src/Legality/Gen5Legality.cpp \
//     src/Legality/Gen5LegalityData.cpp src/Legality/Gen5Text.cpp src/Names/SpeciesNames.cpp \
//     src/Names/MoveNames.cpp src/Pokemon/Experience.cpp -o fuzz_gen5
// The first input byte picks the decoding path; with the decrypted path the
// checksum is fixed up so the fuzzer reaches the analyzer past the checksum gate.
// Aborts when the analyzer writes to its input or its verdict contradicts its
// issues (Invalid without an Invalid issue, or "No problems found" with a Warning
// or an area that is not Complete).
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include "Legality/Gen5Legality.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;
    const bool encrypted = data[0] & 1;
    std::vector<uint8_t> pk(data + 1, data + size);
    if (!encrypted && (pk.size() == 136 || pk.size() == 220)) {
        const uint16_t sum = Legality::Gen5::checksum(pk);
        pk[6] = static_cast<uint8_t>(sum);
        pk[7] = static_cast<uint8_t>(sum >> 8);
    }
    const auto copy = pk;
    const auto report = Legality::analyzePk5(pk, {.encrypted = encrypted});
    if (pk != copy) std::abort();  // the analyzer must never write to its input
    auto any = [&](Legality::Severity s) {
        return std::any_of(report.issues.begin(), report.issues.end(), [s](const auto& i) { return i.severity == s; });
    };
    const auto verdict = report.verdict();
    if ((verdict == Legality::Verdict::Invalid) != any(Legality::Severity::Invalid)) std::abort();
    if (verdict == Legality::Verdict::NoProblemsFound) {
        const auto& c = report.coverage;
        for (auto level : {c.structure, c.sourceGame, c.internal, c.moves, c.encounter, c.eventGift, c.pidRng, c.eggBreeding, c.transfer})
            if (level != Legality::CoverageLevel::Complete) std::abort();
        if (any(Legality::Severity::Warning)) std::abort();
    }
    return 0;
}
