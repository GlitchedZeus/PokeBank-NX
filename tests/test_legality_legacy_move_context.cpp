#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon3FRLG.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>

namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
bool hasText(const Legality::Report& report, const std::string& needle) {
    for (const auto& issue : report.issues)
        if (issue.text.find(needle) != std::string::npos) return true;
    return false;
}

Pokemon::Pokemon3FRLG baseMon() {
    std::array<uint8_t, 80> raw{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setLanguage(2);
    p.setSpecies(1); // Bulbasaur
    p.setOTName(u"RED");
    p.setNickname(u"BULBASAUR");
    p.setOriginGame(2);
    p.setBall(4);
    p.setMetLevel(5);
    p.setLevel(10);
    return p;
}
}

int main() {
    {
        auto p = baseMon();
        p.setMove(0, 57); // Surf: not in Bulbasaur's audited R/B/Y compatibility pool.
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, "red_gb");
        assert(hasText(report, "exact Gen I game"));
        assert(report.coverage.sourceGame == Legality::CoverageLevel::Complete);
        assert(report.coverage.encounter == Legality::CoverageLevel::Partial);
        assert(report.verdict() == Legality::Verdict::Incomplete);
    }

    {
        auto p = baseMon();
        p.setMove(0, 57); // Still outside Bulbasaur's G/S/C + Time Capsule compatibility pool.
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, "gold_gbc");
        assert(hasText(report, "exact Gen II game"));
        assert(report.coverage.sourceGame == Legality::CoverageLevel::Complete);
        assert(report.verdict() == Legality::Verdict::Incomplete);
    }

    {
        auto p = baseMon();
        p.setMove(0, 467); // Gen IV format-valid id, but not a Bulbasaur direct/transfer move.
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, "platinum_nds");
        assert(hasText(report, "exact Gen IV game"));
        assert(report.coverage.sourceGame == Legality::CoverageLevel::Complete);
        assert(report.coverage.encounter == Legality::CoverageLevel::Partial);
        // This focused test deliberately reuses a PK3 wrapper to exercise the exact-game
        // move tables. The newer transfer verifier can therefore flag the synthetic entity
        // as an invalid Gen III -> IV transfer; do not use the global verdict here.
        assert(!hasText(report, "Move id 467 cannot exist in a Generation 4 save"));
    }

    {
        auto p = baseMon();
        p.setMove(0, 468); // Gen V move id: cannot exist in a Gen IV entity.
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, "platinum_nds");
        assert(hasText(report, "cannot exist in a Generation 4 save"));
        assert(report.hasInvalid());
        assert(report.verdict() == Legality::Verdict::Invalid);
    }

    {
        // Container/save identity and encounter origin are different concepts.
        // The pinned Diamond gift Eevee is Lv.5 at location 10, while Platinum's
        // gift Eevee at the same location is Lv.20. Eevee is Gen-III-representable,
        // so this focused compatibility wrapper can prove stored-origin routing
        // without fabricating a post-Gen-III species inside a PK3.
        auto p = baseMon();
        p.setSpecies(133);
        p.setOriginGame(10); // Diamond
        p.setMetLocation(10);
        p.setMetLevel(5);
        p.setLevel(5);
        p.setMove(0, 0);
        const auto report = Legality::analyze(p, Enums::GameVersion::FRLG, "platinum_nds");
        assert(hasText(report, "static/gift encounter for the stored origin game"));
        assert(!hasText(report, "No matching Gen IV wild/static/gift/trade evidence for the stored origin game"));
    }

    std::cout << "Gen I/II/IV exact-source legality context: PASS\n";
}
