#pragma once
// Round 7 B05 (R2-CHECK-011, owner Oct 26: "the names now differ, but many rows still say
// Event/special location; unverified. I want to understand what each choice means"): a short,
// source-backed secondary line for every Gen IV / V special met location (event, transfer, trade
// and egg markers), shown beside the name in the Met Location picker.
//
// Every claim is traced to a pinned table (PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722):
//  * Event range: Locations.IsEventLocation4 (3000..3076) / IsEventLocation5 (40001..49999).
//  * Markers: Locations.cs Transfer3 = 55 (Pal Park), Transfer4 = 30001 (Poké Transporter),
//    Transfer4_CelebiUnused / _CelebiUsed = 30010 / 30011, Transfer4_CrownUnused / _CrownUsed =
//    30012 / 30013 ("Event not activated / activated in Gen 5"), LinkTrade4 = 2002,
//    LinkTrade4NPC = 2001, Daycare4 = 2000, LinkTrade5 = 30003, LinkTrade5NPC = 30002 ("used by
//    Spin Trade player traded eggs"), Breeder5 = 60003; Locations5.Met6 = {60001, 60003} (the
//    60000 set); EncounterStatic5Radar.Location = 30015.
//  * Event kinds come from the native names (text_hgss_03000 / text_bw2_40000, all languages):
//    "WCS" is "Campeonato Mundial" in Spanish, "Worlds" is "Campeonato", "VGE" is "Evento de VJ" /
//    "Evento di Gioco" (Spanish / Italian), "PC <city>" is "CP <city>" (Centre / Centro Pokémon) in
//    French / Spanish / Italian, "Pokémon Cartoon" is ポケモンアニメ, "Pokémon DW" is "Pokémon Dream
//    World" (Spanish). A year is stated only where the native name carries it (WCS / Worlds / VGE
//    2010..2020, the Pokémon Event year markers of Names::metLocationEventYear); other numbers are
//    "native number N", meaning not documented.
// A name never proves an origin: no line names a distribution, recipient, date or legality.
// Each line ends with the native id ("#40026").
#include "Names/LocationDisplayNames.h"
#include "Names/LocationNames.h"

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Names {

namespace detail_notes {
inline bool startsWith(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}
// The number a native name ends with, written as the name writes it ("WCS 2012" -> "2012",
// "Pokémon Movie 06" -> "06"), or "".
inline std::string trailingNumber(std::string_view text) {
    std::size_t end = text.size(), begin = end;
    while (begin > 0 && std::isdigit(static_cast<unsigned char>(text[begin - 1]))) --begin;
    if (begin == end || begin == 0 || text[begin - 1] != ' ') return {};
    return std::string(text.substr(begin));
}
inline std::string withId(std::string text, uint16_t id) { return text + " \xC2\xB7 #" + std::to_string(id); }
} // namespace detail_notes

inline bool isGen4Origin(uint8_t origin) noexcept {
    return origin == 7 || origin == 8 || origin == 10 || origin == 11 || origin == 12;
}
inline bool isGen5Origin(uint8_t origin) noexcept { return origin >= 20 && origin <= 23; }

// `row` is the short kind the picker row shows beside the name (it never repeats the name's own
// number or year); `full` is the focused row's info line: kind, designation / year where the
// native name carries it, and the native id. Both are "" for an ordinary place.
struct SpecialMetLocation {
    std::string row;
    std::string full;
    bool empty() const noexcept { return row.empty(); }
};

inline SpecialMetLocation specialMetLocation(uint8_t origin, uint16_t id) {
    using detail_notes::startsWith;
    using detail_notes::trailingNumber;
    using detail_notes::withId;
    const bool gen4 = isGen4Origin(origin), gen5 = isGen5Origin(origin);
    if (!gen4 && !gen5) return {};
    const auto same = [&](const char* text) { return SpecialMetLocation{text, withId(text, id)}; };
    const auto two = [&](const char* row, const char* full) { return SpecialMetLocation{row, withId(full, id)}; };
    // Transfer, trade and egg markers (PKHeX Locations.cs / Locations5.cs).
    if (gen4) {
        switch (id) {
            case 55: return two("Transfer from Gen III", "Transfer from Gen III (Pal Park)");
            case 2000: return same("Day-Care marker (egg)");
            case 2001: return same("In-game (NPC) trade");
            case 2002: return same("Trade over link");
            default: break;
        }
    } else {
        switch (id) {
            case 30001: return two("Transfer from Gen IV", "Transfer from Gen IV (Poké Transporter)");
            case 30002: return two("NPC / egg trade", "NPC / egg trade: in-game or Spin Trade egg");
            case 30003: return same("Trade over link");
            case 30010: return two("Transfer: Crown Celebi", "Transfer: Crown Celebi, event not activated");
            case 30011: return two("Transfer: Crown Celebi", "Transfer: Crown Celebi, event activated");
            case 30012: return two("Transfer: Crown Beast", "Transfer: Crown Beast, event not activated");
            case 30013: return two("Transfer: Crown Beast", "Transfer: Crown Beast, event activated");
            case 30015: return same("Pokémon Dream Radar encounter");
            case 60002: return same("Day-Care marker (egg)");
            case 60003: return two("Egg: Pokémon Breeder", "Egg-location marker: Pokémon Breeder");
            default: break;
        }
        if (id == 30004 || id == 30005 || id == 30006 || id == 30007 || id == 30008 || id == 30014 ||
            id == 60001)
            return {"Not documented \xC2\xB7 #" + std::to_string(id), withId("Native marker; meaning not documented", id)};
    }
    const bool event = gen4 ? (id >= 3000 && id <= 3076) : (id > 40000 && id < 50000);
    if (!event) return {};

    const char* raw = getMetLocationName(origin, id);
    const std::string_view name = raw ? raw : "";
    const std::string number = trailingNumber(name);
    // Every event line starts "Event", then what the native name says the event was.
    const auto numbered = [&](const char* kind) {
        // POKÉPARK 10..20 (Gen V) carry their number in the ja / de / fr text only.
        std::string shown = number;
        if (shown.empty())
            if (const auto fromText = metLocationNativeNumber(origin, id)) shown = std::to_string(*fromText);
        return SpecialMetLocation{std::string("Event: ") + kind, withId(std::string("Event: ") + kind +
            (shown.empty() ? "" : ", native number " + shown), id)};
    };
    // WCS / Worlds / VGE names carry a four-digit year.
    const auto yearly = [&](const char* kind) {
        return SpecialMetLocation{std::string("Event: ") + kind,
            withId(std::string("Event: ") + kind + (number.size() == 4 ? ", " + number : ""), id)};
    };
    if (startsWith(name, "Pokémon Movie")) return numbered("Pokémon movie");
    if (startsWith(name, "Space World")) return numbered("Space World");
    if (startsWith(name, "Pokémon Festa")) return numbered("Pokémon Festa");
    if (startsWith(name, "POKÉPARK") || startsWith(name, "POKéPARK")) return numbered("POKÉPARK");
    if (startsWith(name, "Pokémon Event")) {
        const auto marker = metLocationEventYear(origin, id);
        return {"Event: Pokémon event", withId(marker ? "Event: Pokémon event, native year " +
                                                       std::to_string(*marker) : "Event: Pokémon event", id)};
    }
    if (startsWith(name, "WCS")) return yearly("World Championship (WCS)");
    if (startsWith(name, "Worlds")) return yearly("Championship (Worlds)");
    if (startsWith(name, "VGE")) return yearly("video game event (VGE)");
    if (name == "Pokémon Center") return same("Event: Pokémon Center store");
    if (startsWith(name, "PC ")) {
        const std::string city = std::string(name.substr(3));
        return {"Event: Pokémon Center store", withId("Event: Pokémon Center " + city + " store", id)};
    }
    if (name == "Pokémon Cartoon") return same("Event: Pokémon anime");
    if (name == "Pokémon DW") return same("Event: Pokémon Dream World");
    if (name == "PBR") return two("Event marker PBR", "Event marker PBR; not expanded");
    if (name == "Lovely place" || name == "Faraway place" || name.empty())
        return {"Event marker \xC2\xB7 #" + std::to_string(id), withId("Event marker; meaning not documented", id)};
    // Fan Club, Battle Tour, Wi-Fi Gift, Concert, Ranger ...: the name is all the tables say, so
    // the row shows the native id rather than one generic line repeated.
    return {"Event \xC2\xB7 #" + std::to_string(id), withId("Event: " + std::string(name), id)};
}

// The info line alone (focused row), "" for an ordinary place.
inline std::string specialMetLocationNote(uint8_t origin, uint16_t id) { return specialMetLocation(origin, id).full; }

} // namespace Names
