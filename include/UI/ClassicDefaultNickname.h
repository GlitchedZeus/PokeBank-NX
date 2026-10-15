#pragma once
#include "Names/SpeciesNames.h"
#include <string>

namespace PokeBank::UIModel {
// Display-case species name with ASCII stand-ins for the glyphs the classic editors cannot type.
inline std::string classicSpeciesName(uint16_t species) {
    if (species == 29) return "Nidoran F";
    if (species == 32) return "Nidoran M";
    if (species == 83) return "Farfetchd";
    return Names::getSpeciesName(species);
}
inline std::string classicUpper(std::string s) {
    for (auto& c : s) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    return s;
}
// The name a Gen I-III Pokemon carries when it has no nickname. The retail games store species
// names in upper case (pret/pokered and pokecrystal data/pokemon/names.asm, pokeemerald
// gSpeciesNames), and evolution renames only a Pokemon whose name still equals its old species
// name, so a blank nickname must default to the upper-case retail spelling.
inline std::string classicDefaultNickname(uint16_t species) {
    return classicUpper(classicSpeciesName(species));
}
// A species change carries a still-default nickname to the new species, preserving the case the
// user sees (a mixed-case legacy draft stays mixed case); a real nickname is kept.
inline std::string nicknameAfterSpeciesChange(const std::string& nickname, uint16_t oldSpecies,
                                               uint16_t newSpecies) {
    if (nickname.empty()) return nickname;
    const auto oldName = classicSpeciesName(oldSpecies);
    if (nickname == oldName) return classicSpeciesName(newSpecies);
    if (nickname == classicUpper(oldName)) return classicDefaultNickname(newSpecies);
    return nickname;
}
}
