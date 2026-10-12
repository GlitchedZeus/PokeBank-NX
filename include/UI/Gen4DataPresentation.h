#ifndef POKEBANK_UI_GEN4_DATA_PRESENTATION_H
#define POKEBANK_UI_GEN4_DATA_PRESENTATION_H

#include <array>
#include <cstdint>
#include <string>

namespace PokeBank::UIModel::Gen4DataPresentation {

// PK4's genuine six-bit PC markings field. A raw decimal mask (e.g. "0")
// is not a useful display to a player. Keep unrecognized high bits visible
// rather than silently pretending the record contains no other flags.
[[nodiscard]] inline std::string markings(uint8_t mask) {
    constexpr std::array<const char*, 6> kLabels{{
        "Cir", "Tri", "Sq", "Hrt", "Star", "Dia"
    }};
    if (mask == 0) return "None";
    std::string out;
    for (unsigned bit = 0; bit < kLabels.size(); ++bit) {
        if ((mask & (1u << bit)) == 0) continue;
        if (!out.empty()) out += " ";
        out += kLabels[bit];
    }
    if ((mask & 0xC0u) != 0) {
        if (!out.empty()) out += " ";
        out += "Other bits";
    }
    return out;
}

// PK4 Pokérus byte: low four bits hold contagious days, upper four
// hold strain. A non-zero strain with zero days is a cured record.
[[nodiscard]] inline const char* pokerus(uint8_t value) noexcept {
    if (value == 0) return "None";
    return (value & 0x0Fu) ? "Infected" : "Cured";
}

} // namespace PokeBank::UIModel::Gen4DataPresentation
#endif
