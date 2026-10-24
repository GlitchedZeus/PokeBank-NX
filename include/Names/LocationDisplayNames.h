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
#include <optional>

namespace Names {

// Pinned PKHeX 6042efa241a9925ce57e645329ea7f976deabdd3:
// text_hgss_03000_en.txt entries 61..71 and text_bw2_40000_en.txt 74..84.
// These are native year markers, never proof of a particular distribution.
inline std::optional<unsigned> metLocationEventYear(uint8_t origin,uint16_t location) {
    const bool gen4=origin==7 || origin==8 || origin==10 || origin==11 || origin==12;
    if(gen4 && location>=3061 && location<=3071)return 2006u+location-3061u;
    if(origin>=20 && origin<=23 && location>=40074 && location<=40084)return 2010u+location-40074u;
    return std::nullopt;
}
inline std::string metLocationEventEvidence(uint8_t origin,uint16_t location) {
    if(const auto year=metLocationEventYear(origin,location))
        return std::to_string(*year)+" native event marker; distribution and exact dates unverified";
    const auto* raw=getMetLocationName(origin,location);
    if(raw && std::string_view(raw).find("Event")!=std::string_view::npos)
        return "Event-named native location; distribution and period unknown";
    return {};
}

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
    if (locationId == 0) return "None";   // every generation's "no location" (PKHeX dash sentinel)
    const char* raw = getMetLocationName(originVersion, locationId);
    if (!raw || !*raw) return "Unknown location";
    if(const auto year=metLocationEventYear(originVersion,locationId))
        return "Pokémon Event "+std::to_string(*year);
    // Gen V's table carries placeholder glyphs (U+EFxx) for some special entries, and PKHeX keeps
    // the game's text-variable escapes ("\xf000...'s ...", a name the game builds from save data)
    // verbatim. Neither is a place name the player would recognise, so neither is shown.
    const std::string_view text(raw);
    if (text.find("\\x") != std::string_view::npos) return "Unknown location";
    for (const char* p = raw; *p; ++p)
        if (static_cast<unsigned char>(*p) == 0xEF) return "Unknown location";
    return gen3 ? stripLocationSourceTags(raw) : std::string(raw);
}

} // namespace Names
