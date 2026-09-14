#ifndef POKEBANK_BATTLE_STAT_RADAR_MODEL_H
#define POKEBANK_BATTLE_STAT_RADAR_MODEL_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace PokeBank::UIModel {
inline constexpr std::array<const char*,5> gen1RadarLabels{"HP", "Atk", "Def", "Spe", "Spc"};
// Canonical Gen II display order. Storage/calculation code keeps its historical
// HP/Atk/Def/Spe/SpA/SpD array layout; canonicalGen2RadarStats performs the UI-only remap.
inline constexpr std::array<const char*,6> gen2RadarLabels{"HP", "Atk", "Def", "SpA", "SpD", "Spe"};

template <std::size_t N>
struct BattleStatRadarModelN {
    float scale = 1.0f;
    std::array<float,N> normalized{};
};
using BattleStatRadarModel = BattleStatRadarModelN<5>;
using Gen2BattleStatRadarModel = BattleStatRadarModelN<6>;

template <std::size_t N>
inline BattleStatRadarModelN<N> radarModel(const std::array<uint16_t,N>& stats) noexcept {
    const float peak = static_cast<float>(*std::max_element(stats.begin(), stats.end()));
    // Common linear scale preserves shape. Rounded local buckets leave headroom
    // and let increases grow within a bucket instead of dividing by the exact
    // current maximum every frame. The renderer labels the scale and raw stats.
    const float step = peak <= 50.0f ? 5.0f : peak <= 100.0f ? 10.0f : 50.0f;
    BattleStatRadarModelN<N> model;
    model.scale = std::max(20.0f, std::ceil(peak * 1.1f / step) * step);
    for (size_t i = 0; i < stats.size(); ++i)
        model.normalized[i] = std::clamp(static_cast<float>(stats[i]) / model.scale, 0.0f, 1.0f);
    return model;
}

inline BattleStatRadarModel gen1RadarModel(const std::array<uint16_t,5>& stats) noexcept {
    return radarModel(stats);
}

inline constexpr std::array<uint16_t,6> canonicalGen2RadarStats(
    const std::array<uint16_t,6>& internalStats) noexcept {
    // Internal battle-stat order: HP, Atk, Def, Spe, SpA, SpD.
    // User-facing radar order:    HP, Atk, Def, SpA, SpD, Spe.
    return {internalStats[0], internalStats[1], internalStats[2],
            internalStats[4], internalStats[5], internalStats[3]};
}

inline Gen2BattleStatRadarModel gen2RadarModel(const std::array<uint16_t,6>& internalStats) noexcept {
    return radarModel(canonicalGen2RadarStats(internalStats));
}
} // namespace PokeBank::UIModel
#endif
