#include "Save/BDSPReadValidation.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

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

    const auto src = read("src/Trainer/Trainer8BDSP.cpp");
    const auto ctor = src.find("Trainer8BDSP::Trainer8BDSP");
    const auto guard = src.find("BDSP::hasMinimumLayout(saveData.size())", ctor);
    const auto parse = src.find("parseMyStatus();", ctor);
    assert(ctor != std::string::npos);
    assert(guard != std::string::npos);
    assert(parse != std::string::npos);
    assert(guard < parse);

    std::cout << "BDSP minimum-layout guard: PASS\n";
}
