// Gen IV terrain must retain its actual byte value when checking encounter flags.
// Inputs are unchanged synthetic records from the existing PK4 fixtures and
// retained PK5 oracle corpus, verified clean before constructing corrupt copies.
// A native HGSS capture obtains its terrain from BattleSystem_GetTerrainId(),
// clamped to TERRAIN_MAX=24, then writes MON_DATA_MET_TERRAIN. The field survives
// Poke Transfer. Values 25..255 are not native terrain codes; in particular, adding
// 32 must never turn a valid Grass=2 into another accepted representation.
#include "Legality/Gen4Legality.h"
#include "Legality/Gen5Legality.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
Legality::Report analyze(int format, const std::string& game, const std::vector<uint8_t>& raw) {
    return format == 4 ? Legality::analyzePk4(raw, game)
                       : Legality::analyzePk5(raw, {.encrypted=false});
}

bool hasTerrainFailure(const Legality::Report& report) {
    for (const auto& issue : report.issues)
        if (issue.severity == Legality::Severity::Invalid &&
            issue.text.find("ground tile type does not match") != std::string::npos)
            return true;
    return false;
}
} // namespace

int main() {
    std::ifstream input("tests/fixtures/gen15_session2/ground_tile.txt");
    assert(input);
    std::set<std::string> games;
    std::set<uint8_t> transferOrigins;
    bool maximumPreserved = false;
    size_t fixtures = 0, negatives = 0;
    int format;
    std::string game, id, hex;
    while (input >> format >> game >> id >> hex) {
        assert((format == 4 || format == 5) && hex.size() == 272);
        std::vector<uint8_t> original;
        for (size_t i = 0; i < hex.size(); i += 2)
            original.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        assert(original[0x85] <= 24);
        if (format == 4) games.insert(game);
        else transferOrigins.insert(original[0x5F]);
        maximumPreserved |= original[0x85] == 24;
        const auto untouched = original;
        const auto positive = analyze(format, game, original);
        assert(positive.verdict() == Legality::Verdict::NoProblemsFound);
        assert(original == untouched);

        // Every out-of-domain byte, including every possible high-bit alias,
        // goes through the real native/transfer analyzer with a valid checksum.
        for (unsigned terrain = 25; terrain <= 255; ++terrain) {
            auto corrupt = original;
            corrupt[0x85] = static_cast<uint8_t>(terrain);
            const auto checksum = Legality::Gen4::checksum(corrupt);
            corrupt[6] = static_cast<uint8_t>(checksum);
            corrupt[7] = static_cast<uint8_t>(checksum >> 8);
            const auto before = corrupt;
            const auto report = analyze(format, game, corrupt);
            assert(report.verdict() == Legality::Verdict::Invalid && hasTerrainFailure(report));
            assert(corrupt == before);
            ++negatives;
        }
        // A corrupt-copy analysis cannot leak terrain state into a later record.
        assert(analyze(format, game, original).verdict() == Legality::Verdict::NoProblemsFound);
        ++fixtures;
    }
    assert(input.eof() && fixtures == 10 && games.size() == 5 && transferOrigins.size() == 5);
    assert(maximumPreserved && negatives == 2310);
    std::cout << "Ground terrain:10 original positives across five PK4 games/five PK5 origins; "
                 "2310 corrupt bytes rejected, terrain24 preserved, immutable/repeated analysis PASS\n";
}
