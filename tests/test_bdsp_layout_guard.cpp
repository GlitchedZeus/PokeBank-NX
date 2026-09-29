#include "Save/BDSPReadValidation.h"
#include "Utils/MD5.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    namespace V = PokeBank::SaveValidation::BDSP;

    static_assert(V::minimumLayoutBytes > V::partyCountOffset);
    static_assert(V::minimumLayoutBytes > V::romCodeOffset);
    static_assert(V::minimumLayoutBytes > V::hashOffset);

    assert(!V::hasMinimumLayout(0));
    assert(!V::hasMinimumLayout(V::partyCountOffset));
    assert(!V::hasMinimumLayout(V::minimumLayoutBytes - 1));
    assert(V::hasMinimumLayout(V::minimumLayoutBytes));
    assert(V::hasMinimumLayout(V::minimumLayoutBytes + 4096));

    // AUDIT-022: validate the stored whole-file MD5 without mutating the candidate.
    std::vector<uint8_t> valid(V::minimumLayoutBytes + 4096, 0);
    for (std::size_t i = 0; i < valid.size(); ++i)
        valid[i] = static_cast<uint8_t>((i * 37u + 11u) & 0xFFu);
    std::fill_n(valid.begin() + static_cast<std::ptrdiff_t>(V::hashOffset),
                V::hashBytes, uint8_t{0});
    std::array<uint8_t, V::hashBytes> digest{};
    Utils::md5(valid.data(), valid.size(), digest.data());
    std::copy(digest.begin(), digest.end(),
              valid.begin() + static_cast<std::ptrdiff_t>(V::hashOffset));
    const auto beforeValidation = valid;
    assert(V::wholeFileHashValid(valid));
    assert(valid == beforeValidation);

    auto corrupt = valid;
    corrupt[0x1234] ^= 0x80;
    assert(!V::wholeFileHashValid(corrupt));
    assert(corrupt[0x1234] != valid[0x1234]);

    const auto trainer = read("src/Trainer/Trainer8BDSP.cpp");
    const auto ctor = trainer.find("Trainer8BDSP::Trainer8BDSP");
    const auto guard = trainer.find("BDSP::hasMinimumLayout(saveData.size())", ctor);
    const auto hashGuard = trainer.find("BDSP::wholeFileHashValid(saveData)", ctor);
    const auto parse = trainer.find("parseMyStatus();", ctor);
    assert(ctor != std::string::npos);
    assert(guard != std::string::npos);
    assert(hashGuard != std::string::npos);
    assert(parse != std::string::npos);
    assert(guard < hashGuard && hashGuard < parse);

    // Refuse the malformed workspace before the Trainer screen is opened as well.
    const auto save = read("src/Save/GetSaveFileContents.cpp");
    const auto openGuard = save.find("if (group == GameVersion::BDSP)");
    assert(openGuard != std::string::npos);
    const auto boundary = save.find("SaveValidation::BDSP::hasMinimumLayout", openGuard);
    const auto hashBoundary = save.find("BDSP::wholeFileHashValid", openGuard);
    const auto refusal = save.find("BDSP save whole-file MD5 does not match", openGuard);
    assert(boundary != std::string::npos);
    assert(hashBoundary != std::string::npos);
    assert(refusal != std::string::npos);
    assert(boundary < hashBoundary && hashBoundary < refusal);

    std::cout << "BDSP layout + whole-file MD5 guard: PASS\\n";
}
