#pragma once

// Player-facing met-location names for the Gen III-V editors.
//
// Names/LocationNames.cpp is generated from PKHeX text_*_en.txt (tools/gen_locations.py).
// Its Gen III table already carries the player-facing names (gen_locations.py GEN3_DISPLAY):
// no "(RSE)" / "(FRLG)" / "(E)" source tags, and a region suffix only where two different
// places share a name (Safari Zone, Victory Road, Altering Cave). This layer adds what needs
// the record's origin game: Battle Tower (R/S) vs Battle Frontier (E) at 58, Celadon Dept.
// (FRLG) vs Special Area (E) at 196, the 253-255 special ids, and PKHeX's " (unused)" marker.
// Gen III origin ids: 1 Sapphire, 2 Ruby, 3 Emerald, 4 FireRed, 5 LeafGreen (PK3 origins).
// Never shows a raw id: an unknown id reads "Unknown location".

#include "Names/LocationNames.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace Names {

inline std::string stripLocationSourceTags(std::string_view raw) {
    std::string name(raw);
    for (const std::string_view tag : {" (unused)"}) {
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
