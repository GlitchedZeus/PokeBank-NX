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
#include "Integration/Gen5/Gen5EncounterLocations.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <array>
#include <vector>

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
// Round 6 P5 (R2-CHECK-011): Gen V's bank 40098..40108 is "POKÉPARK" eleven times in PKHeX's
// English text, but the same entries of the game's Japanese, German and French text carry a
// number: ポケパーク１０..２０ / "POKÉPARK 10".."POKÉPARK 20" (PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
// Resources/text/locations/gen5/text_bw2_40000_{ja,de,fr}.txt, entries 98..108). The number is
// shown as the game writes it; like "Pokémon Festa 10".."20" (40086..40096), what it denotes
// (a year, an edition) is not documented, so no year is claimed.
inline std::optional<unsigned> metLocationNativeNumber(uint8_t origin,uint16_t location) {
    if(origin<20 || origin>23)return std::nullopt;
    if(location>=40086 && location<=40096)return 10u+location-40086u;   // Pokémon Festa NN
    if(location>=40098 && location<=40108)return 10u+location-40098u;   // POKÉPARK NN
    return std::nullopt;
}
inline std::string metLocationEventEvidence(uint8_t origin,uint16_t location) {
    if(const auto year=metLocationEventYear(origin,location))
        return std::to_string(*year)+" native event marker; distribution and exact dates unverified";
    if(const auto number=metLocationNativeNumber(origin,location))
        return "Native name number "+std::to_string(*number)+"; meaning, distribution and period unverified";
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
    if(locationId>=40098 && locationId<=40108)
        if(const auto number=metLocationNativeNumber(originVersion,locationId))
            return "POKÉPARK "+std::to_string(*number);
    // Gen V's table carries placeholder glyphs (U+EFxx) for some special entries, and PKHeX keeps
    // the game's text-variable escapes ("\xf000...'s ...", a name the game builds from save data)
    // verbatim. Neither is a place name the player would recognise, so neither is shown.
    const std::string_view text(raw);
    if (text.find("\\x") != std::string_view::npos) return "Unknown location";
    for (const char* p = raw; *p; ++p)
        if (static_cast<unsigned char>(*p) == 0xEF) return "Unknown location";
    return gen3 ? stripLocationSourceTags(raw) : std::string(raw);
}

// Round 6 P5: a met-location picker row's name. Black / White / Black 2 / White 2 reuse place names
// under different native ids (9 and 76 are both "Nimbasa City" in every language PKHeX ships; 76..105
// have no wild table in either game), and the transfer markers 30001 / 30010..30013 all read
// "Unknown location". No source says what tells them apart, so the picker shows the native id on
// every id after the first one of a name: "Nimbasa City #76". The row's detail column says the
// history is unverified (EncounterContext45::detail). Field values elsewhere keep the plain name.
inline const std::vector<uint16_t>& metLocationTwinIds(uint8_t originVersion) {
    static std::array<std::vector<uint16_t>, 24> cache;
    static std::array<bool, 24> built{};
    if (originVersion >= cache.size()) { static const std::vector<uint16_t> none; return none; }
    auto& twins = cache[originVersion];
    if (built[originVersion]) return twins;
    built[originVersion] = true;
    std::vector<uint16_t> native;
    if (originVersion >= 20 && originVersion <= 23) {
        native.assign(PokeVault::Integration::Gen5::NativeMetLocations5.begin(),
                      PokeVault::Integration::Gen5::NativeMetLocations5.end());
    } else if (originVersion == 7 || originVersion == 8 || originVersion == 10 ||
               originVersion == 11 || originVersion == 12) {
        const auto table = getLocationTable(originVersion);
        for (std::size_t i = 1; i < table.count && i <= 0xFFFF; ++i)
            if (isGen4NativeMetLocation(originVersion, static_cast<uint16_t>(i)))
                native.push_back(static_cast<uint16_t>(i));
    }
    std::vector<std::string> seen;
    for (const uint16_t id : native) {   // native lists are in ascending id order
        if (id == 0) continue;
        std::string name = getMetLocationDisplayName(originVersion, id);
        bool repeat = false;
        for (const auto& earlier : seen) if (earlier == name) { repeat = true; break; }
        if (repeat) twins.push_back(id);
        else seen.push_back(std::move(name));
    }
    return twins;
}
inline std::string getMetLocationChoiceName(uint8_t originVersion, uint16_t locationId) {
    std::string name = getMetLocationDisplayName(originVersion, locationId);
    for (const uint16_t twin : metLocationTwinIds(originVersion))
        if (twin == locationId) return name + " #" + std::to_string(locationId);
    return name;
}

} // namespace Names
