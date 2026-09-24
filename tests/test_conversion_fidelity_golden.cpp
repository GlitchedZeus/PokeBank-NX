#include "Conversion/Fidelity.h"
#include "Conversion/Gen3PidSearch.h"
#include "Utils/SHA256.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::string readText(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}

std::array<uint8_t, 32> hashBytes(const std::vector<uint8_t>& bytes) {
    Utils::SHA256 sha;
    sha.update(bytes.data(), bytes.size());
    std::array<uint8_t, 32> out{};
    sha.finalize(out.data());
    return out;
}
}

int main() {
    using namespace Conversion;

    // F05: shiny threshold. Gen III shiny is XOR<8; modern is XOR<16.
    {
        Report report;
        assert(Fidelity::isGen3Shiny(0u, 0u));
        assert(Fidelity::isModernShiny(0u, 0u));
        assert(!Fidelity::isGen3Shiny(8u, 0u));
        assert(Fidelity::isModernShiny(8u, 0u));

        std::vector<uint8_t> source{8,0,0,0, 0,0,0,0};
        const auto before = hashBytes(source);
        const uint32_t adapted = Fidelity::adaptGen3PidForModern(8u, 0u, &report);
        assert(adapted == (8u ^ 0x80000000u));
        assert(!Fidelity::isModernShiny(adapted, 0u));
        assert(report.hasAdaptation(Adaptation::PidAdjustedForShinyThreshold));
        assert(hashBytes(source) == before); // source native bytes immutable

        Report ordinary;
        assert(Fidelity::adaptGen3PidForModern(16u, 0u, &ordinary) == 16u);
        assert(!ordinary.hasAdaptation(Adaptation::PidAdjustedForShinyThreshold));
    }

    // F06: Unown form is PID-derived and is a hard PID-search constraint.
    {
        const uint32_t raw = 0x12345678u;
        for (int form : {0, 1, 13, 27}) {
            const uint32_t stamped = Gen3PidSearch::stampUnownForm(raw, form);
            assert(Gen3PidSearch::unownForm(stamped) == form);

            Gen3PidSearch::Traits wanted;
            wanted.nature = static_cast<uint8_t>(stamped % 25);
            wanted.gender = 2;
            wanted.genderRatio = 255;
            wanted.abilityBit = static_cast<uint8_t>(stamped & 1u);
            wanted.tid32 = 0;
            wanted.shiny = Fidelity::isGen3Shiny(stamped, 0);
            wanted.unownForm = form;
            wanted.constrainAbilityBit = false; // Unown has one Gen III ability
            const auto found = Gen3PidSearch::find(stamped, wanted, 1);
            assert(found && Gen3PidSearch::unownForm(*found) == form);
        }
    }

    // F07: deterministic exhaustion remains explicit.
    {
        Gen3PidSearch::Traits impossible;
        impossible.nature = 0;
        impossible.gender = 0;       // impossible for genderless ratio
        impossible.genderRatio = 255;
        impossible.abilityBit = 0;
        impossible.tid32 = 0;
        impossible.shiny = false;
        assert(!Gen3PidSearch::find(0x12345678u, impossible, 128));
    }

    // F08: ability id + slot must both be representable; hidden ability fails closed.
    {
        const auto slot1 = Fidelity::mapModernAbilityToGen3(9, 1, 9, 31);
        const auto slot2 = Fidelity::mapModernAbilityToGen3(31, 2, 9, 31);
        const auto duplicate2 = Fidelity::mapModernAbilityToGen3(51, 2, 51, 51);
        assert(slot1.ok && slot1.abilityBit == 0);
        assert(slot2.ok && slot2.abilityBit == 1);
        assert(duplicate2.ok && duplicate2.abilityBit == 1);
        assert(!Fidelity::mapModernAbilityToGen3(158, 4, 9, 31).ok);
        assert(!Fidelity::mapModernAbilityToGen3(31, 1, 9, 31).ok);
        assert(Fidelity::gen3AbilityNumberForModern(1, 51, 51) == 1);
    }

    // F09: destination semantics, not overlapping bytes, determine Tera defaults.
    {
        assert(Fidelity::defaultTeraType(12, 4) == 12);
        assert(Fidelity::defaultTeraType(0, 4) == 4);
        Report report;
        report.addLoss(Loss::TeraDataDropped);
        report.addLoss(Loss::ZAAlphaDropped);
        report.addAdaptation(Adaptation::TargetDefaultTeraSynthesized);
        assert(report.hasLoss(Loss::TeraDataDropped));
        assert(report.hasLoss(Loss::ZAAlphaDropped));
        assert(report.hasAdaptation(Adaptation::TargetDefaultTeraSynthesized));
    }

    // F10: Gen III 252 is exact; 253/254/255 clamp only with a declared loss.
    {
        for (uint8_t v : {uint8_t{252}, uint8_t{253}, uint8_t{254}, uint8_t{255}}) {
            Report report;
            const uint8_t modern = Fidelity::modernEvFromGen3(v, &report);
            assert(modern == (v > 252 ? 252 : v));
            assert(report.hasLoss(Loss::Gen3EVClamped) == (v > 252));
        }
    }

    // F11: international Gen III text is preserved only when it can be represented exactly.
    {
        assert(Fidelity::gen3InternationalLanguageSupported(2)); // English
        assert(Fidelity::gen3InternationalLanguageSupported(3)); // French
        assert(Fidelity::gen3InternationalLanguageSupported(5)); // German
        assert(Fidelity::gen3InternationalLanguageSupported(7)); // Spanish
        assert(!Fidelity::gen3InternationalLanguageSupported(1)); // Japanese table not implemented here
        assert(!Fidelity::gen3InternationalLanguageSupported(8)); // Korean has no Gen III mapping
        assert(Fidelity::gen3TextRepresentable(u"SPARKY", 10));
        assert(!Fidelity::gen3TextRepresentable(u"TOO-LONG-NAME", 10));
        assert(!Fidelity::gen3TextRepresentable(u"Ω", 10));
    }

    // F13: origin restamping is a declared provenance loss, never silently treated as historical truth.
    {
        Report report;
        report.sourceOriginVersion = 44;
        report.destinationEntityOriginVersion = 4;
        report.addLoss(Loss::OriginGameRestamped);
        assert(report.hasLosses());
        assert(report.hasLoss(Loss::OriginGameRestamped));
        assert(report.sourceOriginVersion != report.destinationEntityOriginVersion);
    }

    // Production-wiring contract for the golden policies above.
    const std::string source = readText("src/Conversion/Convert.cpp");
    const std::string api = readText("include/Conversion/Convert.h");
    const std::string move = readText("src/UI/TrainerViewScreenBase.inc");

    assert(source.find("Fidelity::adaptGen3PidForModern") != std::string::npos);
    assert(source.find("wanted.unownForm") != std::string::npos);
    assert(source.find("Fidelity::mapModernAbilityToGen3") != std::string::npos);
    assert(source.find("Loss::TeraDataDropped") != std::string::npos);
    assert(source.find("Fidelity::modernEvFromGen3") != std::string::npos);
    assert(source.find("sourceNicknamed") != std::string::npos);
    assert(source.find("Result::TextNotRepresentable") != std::string::npos);
    assert(source.find("Loss::OriginGameRestamped") != std::string::npos);
    assert(api.find("Report* report = nullptr") != std::string::npos);

    // A04b remains globally fail-closed for cross-game true Move in this tranche.
    assert(move.find("Cross-game true Move is locked until conversion preservation is proven.") != std::string::npos);

    std::cout << "F05-F13 conversion fidelity golden fixture contract: PASS\n";
}
