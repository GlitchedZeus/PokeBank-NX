#pragma once
#include "Names/SpeciesNames.h"
#include <string>

namespace PokeBank::UIModel {
inline std::string classicDefaultNickname(uint16_t species) {
    if (species == 29) return "Nidoran F";
    if (species == 32) return "Nidoran M";
    if (species == 83) return "Farfetchd";
    return Names::getSpeciesName(species);
}
inline std::string nicknameAfterSpeciesChange(const std::string& nickname, uint16_t oldSpecies,
                                               uint16_t newSpecies) {
    if (nickname.empty()) return nickname;
    auto oldName = classicDefaultNickname(oldSpecies);
    auto upper = [](std::string s) {
        for (auto& c : s) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        return s;
    };
    if (nickname == oldName) return classicDefaultNickname(newSpecies);
    if (nickname == upper(oldName)) return upper(classicDefaultNickname(newSpecies));
    return nickname;
}
}
