#include "Conversion/Gen3PidSearch.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string readText(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    using Conversion::Gen3PidSearch::Traits;
    using Conversion::Gen3PidSearch::find;

    // A candidate that already satisfies every Gen III PID-derived trait is accepted unchanged.
    Traits exact{};
    exact.nature = 0;
    exact.gender = 0;
    exact.genderRatio = 0;
    exact.abilityBit = 0;
    exact.tid32 = 0;
    exact.shiny = true;
    const auto same = find(0, exact, 1);
    assert(same && *same == 0);

    // Impossible trait tuple: a genderless species can never produce a male PID-derived gender.
    // Exhaustion must be explicit; there is no fallback candidate.
    Traits impossible = exact;
    impossible.gender = 0;
    impossible.genderRatio = 255;
    const auto exhausted = find(0x12345678u, impossible, 128);
    assert(!exhausted);

    // Product control-flow contract: the PK8->PK3 remap propagates exhaustion and convert() exposes
    // a typed failure instead of building a destination candidate from the original PID.
    const std::string source = readText("src/Conversion/Convert.cpp");
    const std::string api = readText("include/Conversion/Convert.h");
    assert(source.find("if (!outPid) return std::nullopt;") != std::string::npos);
    assert(source.find("Result::TraitPreservationFailed") != std::string::npos);
    assert(source.find("if (!pk3) { result = Result::TraitPreservationFailed; return nullptr; }") != std::string::npos);
    assert(source.find("Falls back to the original PID") == std::string::npos);
    assert(api.find("TraitPreservationFailed") != std::string::npos);

    std::cout << "F07 Gen III PID search exhaustion fail-closed: PASS\n";
}
