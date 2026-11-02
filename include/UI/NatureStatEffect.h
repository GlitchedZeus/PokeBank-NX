#pragma once

namespace PokeBank::UIModel::NatureStatEffect {
// Native Gen III-V order: Attack, Defense, Speed, Special Attack, Special Defense.
// PKHeX 6501f0ab Editing/NatureAmp.cs GetNatureModification; pret nature stat tables.
// UI stat rows prepend HP, which no nature changes. A modifier is +10% or -10%.
constexpr bool neutral(unsigned nature) noexcept { return nature < 25 && nature / 5 == nature % 5; }
constexpr int modifier(unsigned nature, unsigned statRow) noexcept {
    if (nature >= 25 || statRow == 0 || statRow > 5 || neutral(nature)) return 0;
    return statRow == nature / 5 + 1 ? 1 : statRow == nature % 5 + 1 ? -1 : 0;
}
constexpr const char* statName(unsigned row) noexcept {
    constexpr const char* names[] = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};
    return row < 6 ? names[row] : "?";
}
} // namespace PokeBank::UIModel::NatureStatEffect
