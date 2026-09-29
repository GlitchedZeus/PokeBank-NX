#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality {

enum class CoverageLevel : uint8_t {
    None,
    Partial,
    Complete,
};

struct SourceGameProfile {
    std::string_view id;
    uint8_t generation;
    uint16_t maxSpecies;
    uint16_t maxMove;
    CoverageLevel encounterCoverage;
};

inline constexpr std::array<SourceGameProfile, 16> kSourceGameProfiles{{
    {"red_gb",         1, 151, 165, CoverageLevel::Partial},
    {"blue_gb",        1, 151, 165, CoverageLevel::Partial},
    {"yellow_gb",      1, 151, 165, CoverageLevel::Partial},
    {"gold_gbc",       2, 251, 251, CoverageLevel::None},
    {"silver_gbc",     2, 251, 251, CoverageLevel::None},
    {"crystal_gbc",    2, 251, 251, CoverageLevel::Partial},
    {"ruby_gba",       3, 386, 354, CoverageLevel::Partial},
    {"sapphire_gba",   3, 386, 354, CoverageLevel::Partial},
    {"emerald_gba",    3, 386, 354, CoverageLevel::Partial},
    {"firered_gba",    3, 386, 354, CoverageLevel::Partial},
    {"leafgreen_gba",  3, 386, 354, CoverageLevel::Partial},
    {"diamond_nds",    4, 493, 467, CoverageLevel::Partial},
    {"pearl_nds",      4, 493, 467, CoverageLevel::Partial},
    {"platinum_nds",   4, 493, 467, CoverageLevel::Partial},
    {"heartgold_nds",  4, 493, 467, CoverageLevel::Partial},
    {"soulsilver_nds", 4, 493, 467, CoverageLevel::Partial},
}};

constexpr const SourceGameProfile* sourceGameProfile(std::string_view id) noexcept {
    for (const auto& profile : kSourceGameProfiles)
        if (profile.id == id) return &profile;
    return nullptr;
}

constexpr uint8_t sourceGeneration(std::string_view id) noexcept {
    const auto* profile = sourceGameProfile(id);
    return profile ? profile->generation : 0;
}

} // namespace Legality
