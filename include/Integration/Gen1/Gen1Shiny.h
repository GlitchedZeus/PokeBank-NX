#ifndef POKEBANK_GEN1_SHINY_H
#define POKEBANK_GEN1_SHINY_H

#include <array>
#include <cstdint>

namespace PokeVault::Integration::Gen1 {

// Generation I has no stored shiny flag. A R/B/Y Pokemon is compatible with
// Generation II shininess exactly when its four stored DVs match this rule.
// HP DV remains derived from the low bits of these four values and is never
// independently stored or edited.
class ShinyDVs final {
public:
    static constexpr std::array<uint8_t, 8> kValidAttackDVs{2, 3, 6, 7, 10, 11, 14, 15};

    static constexpr bool attackIsShinyCompatible(uint8_t attack) noexcept {
        for (const auto value : kValidAttackDVs)
            if (attack == value) return true;
        return false;
    }

    static constexpr bool isShiny(const std::array<uint8_t,4>& dvs) noexcept {
        return attackIsShinyCompatible(dvs[0]) && dvs[1] == 10 && dvs[2] == 10 && dvs[3] == 10;
    }

    // Choose the closest valid shiny Attack DV. Ties resolve downward so the
    // transformation is deterministic and never depends on randomness.
    static constexpr uint8_t nearestShinyAttack(uint8_t attack) noexcept {
        uint8_t best = kValidAttackDVs[0];
        unsigned bestDistance = distance(attack, best);
        for (const auto value : kValidAttackDVs) {
            const unsigned candidateDistance = distance(attack, value);
            if (candidateDistance < bestDistance ||
                (candidateDistance == bestDistance && value < best)) {
                best = value;
                bestDistance = candidateDistance;
            }
        }
        return best;
    }

    static constexpr std::array<uint8_t,4> makeShiny(std::array<uint8_t,4> dvs) noexcept {
        dvs[0] = attackIsShinyCompatible(dvs[0]) ? dvs[0] : nearestShinyAttack(dvs[0]);
        dvs[1] = 10;
        dvs[2] = 10;
        dvs[3] = 10;
        return dvs;
    }

    // UI sessions should restore their remembered pre-shiny DV snapshot when
    // available. This deterministic fallback exists for an already-shiny source
    // opened without such a snapshot: changing Defense to 9 makes it normal with
    // the smallest obvious source-truth change and still stores only real Gen I DVs.
    static constexpr std::array<uint8_t,4> makeNormalFallback(std::array<uint8_t,4> dvs) noexcept {
        if (isShiny(dvs)) dvs[1] = 9;
        return dvs;
    }

private:
    static constexpr unsigned distance(uint8_t a, uint8_t b) noexcept {
        return a > b ? static_cast<unsigned>(a - b) : static_cast<unsigned>(b - a);
    }
};

} // namespace PokeVault::Integration::Gen1

#endif
