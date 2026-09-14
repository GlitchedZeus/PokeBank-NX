#ifndef POKEBANK_BATTLE_STAT_RADAR_MODEL_H
#define POKEBANK_BATTLE_STAT_RADAR_MODEL_H
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace PokeBank::UIModel {
inline constexpr std::array<const char*,5> gen1RadarLabels{"HP", "Atk", "Def", "Spe", "Spc"};
struct BattleStatRadarModel {
    float scale = 1.0f;
    std::array<float,5> normalized{};
};
inline BattleStatRadarModel gen1RadarModel(const std::array<uint16_t,5>& stats) noexcept {
    const float peak = static_cast<float>(*std::max_element(stats.begin(), stats.end()));
    // Common linear scale preserves shape. Rounded local buckets leave headroom
    // and let increases grow within a bucket instead of dividing by the exact
    // current maximum every frame. The renderer labels the scale and raw stats.
    const float step = peak <= 50.0f ? 5.0f : peak <= 100.0f ? 10.0f : 50.0f;
    BattleStatRadarModel model;
    model.scale = std::max(20.0f, std::ceil(peak * 1.1f / step) * step);
    for (size_t i = 0; i < stats.size(); ++i)
        model.normalized[i] = std::clamp(static_cast<float>(stats[i]) / model.scale, 0.0f, 1.0f);
    return model;
}
} // namespace PokeBank::UIModel
#endif
