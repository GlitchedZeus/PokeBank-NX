#include "Legality/Gen3EggEventRibbonEvidence.h"
#include "Legality/Legality.h"
#include "Pokemon/Pokemon3FRLG.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {
namespace Ribbon = Legality::Gen3EggEventRibbon;
constexpr std::array<unsigned, 6> bits{25, 24, 23, 20, 21, 22};
Pokemon::Pokemon3FRLG specimen(bool egg, uint32_t ribbons) {
    std::array<std::byte, 80> raw{};
    Pokemon::Pokemon3FRLG p{std::span<const std::byte>(raw)};
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(1);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"BULBASAUR");
    p.setOriginGame(2); // Ruby-origin, irrespective of the supplied container context.
    p.setLevel(5);
    p.setMetLevel(0);
    p.setBall(4);
    p.setEgg(egg);
    p.wr32(0x4C, ribbons);
    p.refreshChecksum();
    return p;
}
std::array<uint8_t, 80> snapshot(const Pokemon::Pokemon3FRLG& p) {
    std::array<uint8_t, 80> result{};
    for (size_t i = 0; i < result.size(); ++i) result[i] = p.rd8(i);
    return result;
}
std::vector<std::string> ribbonIssues(const Legality::Report& report) {
    std::vector<std::string> result;
    for (const auto& issue : report.issues) {
        if (!issue.text.starts_with("Unhatched Gen III egg cannot have the ")) continue;
        assert(issue.severity == Legality::Severity::Invalid);
        assert(issue.identifier == Legality::CheckIdentifier::Egg);
        result.push_back(issue.text);
    }
    return result;
}
}
int main() {
    // Exhaust the helper's six-bit domain for both egg states.
    for (unsigned mask = 0; mask < 64; ++mask) {
        for (bool egg : {false, true}) {
            const auto result = Ribbon::evaluate({egg, bool(mask & 1), bool(mask & 2),
                bool(mask & 4), bool(mask & 8), bool(mask & 16), bool(mask & 32)});
            assert(result.status == (!egg ? Ribbon::Status::NotApplicable :
                mask ? Ribbon::Status::Invalid : Ribbon::Status::Consistent));
            for (size_t i = 0; i < bits.size(); ++i)
                assert(result.contradictions[i] == (egg && bool(mask & (1u << i))));
        }
    }
    constexpr std::array<const char*, 5> sources{
        "ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba"
    };
    for (const char* source : sources) {
        for (bool egg : {false, true}) {
            for (unsigned selection = 0; selection <= 7; ++selection) {
                const uint32_t word = selection == 0 ? 0u :
                    selection == 7 ? 0x03F00000u : uint32_t{1} << bits[selection - 1];
                auto p = specimen(egg, word);
                const auto before = snapshot(p);
                const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, source);
                assert(snapshot(p) == before); // Analysis does not mutate even invalid specimens.
                const auto issues = ribbonIssues(report);
                const size_t expected = !egg || selection == 0 ? 0 : selection == 7 ? 6 : 1;
                assert(issues.size() == expected);
                if (expected) {
                    assert(report.verdict() == Legality::Verdict::Invalid);
                    if (selection != 7)
                        assert(issues[0] == "Unhatched Gen III egg cannot have the " +
                            std::string(Ribbon::kNames[selection - 1]) + " Ribbon");
                }
            }
        }
    }
    // Routing probes intentionally reuse a PK3 object with all ribbon capabilities:
    // the exact container context must win over stored Ruby origin/group hints.
    // These are not asserted to be structurally valid later-generation entities.
    for (const char* source : {"", "unknown", "red_gb", "gold_gbc", "diamond_nds",
                               "platinum_nds", "heartgold_nds"}) {
        auto p = specimen(true, 0x03F00000u);
        const auto before = snapshot(p);
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, source);
        assert(ribbonIssues(report).empty());
        assert(snapshot(p) == before);
    }
    // Earth may be acquired after hatching: adding it must leave the whole report
    // unchanged, including pre-existing encounter/template diagnostics.
    auto p = specimen(false, 0);
    const auto beforeEarth = Legality::analyze(p, Enums::GameVersion::FRLG, "ruby_gba");
    p.wr32(0x4C, 0x02000000u);
    p.refreshChecksum();
    const auto withEarth = Legality::analyze(p, Enums::GameVersion::FRLG, "ruby_gba");
    assert(beforeEarth.verdict() == withEarth.verdict());
    assert(beforeEarth.issues.size() == withEarth.issues.size());
    for (size_t i = 0; i < beforeEarth.issues.size(); ++i) {
        assert(beforeEarth.issues[i].text == withEarth.issues[i].text);
        assert(beforeEarth.issues[i].severity == withEarth.issues[i].severity);
        assert(beforeEarth.issues[i].identifier == withEarth.issues[i].identifier);
    }
    std::cout << "Gen III unhatched Event3 ribbons, exact-source routing and read-only analysis: PASS\n";
}
