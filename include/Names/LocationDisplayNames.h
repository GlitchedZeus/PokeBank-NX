#pragma once

// Player-facing met-location names for the Gen III-V editors.
//
// Names/LocationNames.cpp is generated verbatim from PKHeX text_*_en.txt
// (tools/gen_locations.py) and keeps PKHeX's internal source tags ("(RSE)", "(FRLG)", "(E)",
// "(unused)", "Battle Tower (RS) / Battle Frontier (E)"). Those tags are developer data, not
// place names. This layer turns an exact (origin game, location id) into the name the game
// shows, and adds a natural region suffix ONLY where two different places really share a name:
//   Safari Zone 57 (Hoenn) / 136 (Kanto), Victory Road 70 (Hoenn) / 132 (Kanto),
//   Altering Cave 183 (Sevii Islands, FRLG Six Island) / 210 (Hoenn, Emerald Route 103).
// Navel Rock 174/211 and Birth Island 187/200 are the same event islands, so no suffix.
// Gen III origin ids: 1 Sapphire, 2 Ruby, 3 Emerald, 4 FireRed, 5 LeafGreen (PK3 origins).
// Never shows a raw id: an unknown id reads "Unknown location".

#include "Names/LocationNames.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Names {

inline std::string stripLocationSourceTags(std::string_view raw) {
    std::string name(raw);
    for (const std::string_view tag : {" (RSE)", " (FRLG)", " (RS)", " (E)", " (unused)"}) {
        for (std::size_t at = name.find(tag); at != std::string::npos; at = name.find(tag))
            name.erase(at, tag.size());
    }
    return name;
}

inline std::string getMetLocationDisplayName(uint8_t originVersion, uint16_t locationId) {
    const bool gen3 = originVersion >= 1 && originVersion <= 5;
    if (gen3) {
        const bool emerald = originVersion == 3;
        const bool frlg = originVersion == 4 || originVersion == 5;
        switch (locationId) {
            case 0: return "None";
            case 57: return "Safari Zone (Hoenn)";
            case 136: return "Safari Zone (Kanto)";
            case 70: return "Victory Road (Hoenn)";
            case 132: return "Victory Road (Kanto)";
            case 183: return "Altering Cave (Sevii Islands)";
            case 210: return "Altering Cave (Hoenn)";
            case 58: return emerald ? "Battle Frontier" : "Battle Tower";
            case 196: return frlg ? "Celadon Dept." : emerald ? "Special Area" : "Celadon Dept.";
            case 253: return "Gift egg";
            case 254: return "In-game trade";
            case 255: return "Fateful encounter";
            default: break;
        }
    }
    const char* raw = getMetLocationName(originVersion, locationId);
    if (!raw || !*raw) return locationId == 0 ? "None" : "Unknown location";
    // Gen V's table carries placeholder glyphs (U+EFxx) for some special entries.
    for (const char* p = raw; *p; ++p)
        if (static_cast<unsigned char>(*p) == 0xEF) return "Unknown location";
    return gen3 ? stripLocationSourceTags(raw) : std::string(raw);
}

} // namespace Names
