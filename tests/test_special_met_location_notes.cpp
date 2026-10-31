// Round 7 B05 (R2-CHECK-011, owner Oct 26: rows read "Event/special location; unverified"; "I want
// to understand what each choice means and how it differs"). Names/SpecialMetLocationNotes.h gives
// every Gen IV / V special met location a short row kind and a full info line with its native id.
//  * every claim is traced: the marker lines equal the PKHeX 6501f0ab Locations.cs constants pinned
//    below; a year only where the native name (or the Pokémon Event year marker) carries it; a
//    "native number" only where the name, or the ja / de / fr POKÉPARK text, carries it;
//  * no line names a distribution, recipient, date or legality;
//  * two rows share a kind line only when they are the same kind of thing (a numbered series, the
//    Pokémon Center stores, the two Crown events); generic lines carry their native id instead;
//  * rows fit the 620 px window beside the name (tests/test_met_location_fit.cpp measures the rows
//    the picker draws) and the focused row's info line fits its header slot (w - 195).
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/SpecialMetLocationNotes.h"
#include "UI/SharedMenuGeometry.h"

#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
namespace G = PokeBank::UIModel::SharedMenuGeometry;
const std::string kDot = " \xC2\xB7 #";

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

struct Origin { uint8_t id; std::vector<uint16_t> special; };
std::vector<Origin> origins() {
    std::vector<Origin> out;
    for (const uint8_t o : {7, 8, 10, 11, 12}) {
        // Gen IV pickers list wild places; event and marker ids reach the picker as a record's
        // stored row (EncounterContext45), so every one the name table holds is checked.
        Origin origin{o, {}};
        for (uint32_t i = 1; i <= 0xFFFF; ++i) {
            const char* raw = Names::getMetLocationName(o, static_cast<uint16_t>(i));
            if (raw && raw[0] && !Names::specialMetLocation(o, static_cast<uint16_t>(i)).empty())
                origin.special.push_back(static_cast<uint16_t>(i));
        }
        out.push_back(origin);
    }
    for (const uint8_t o : {20, 21, 22, 23}) {
        Origin origin{o, {}};
        for (const uint16_t id : PokeVault::Integration::Gen5::NativeMetLocations5)
            if (!Names::specialMetLocation(o, id).empty()) origin.special.push_back(id);
        out.push_back(origin);
    }
    return out;
}

// The native name without a trailing number: the series a row belongs to.
std::string family(uint8_t origin, uint16_t id) {
    const char* raw = Names::getMetLocationName(origin, id);
    std::string name = raw ? raw : "";
    while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back()))) name.pop_back();
    while (!name.empty() && name.back() == ' ') name.pop_back();
    if (name.rfind("PC ", 0) == 0 || name == "Pok\xC3\xA9mon Center") return "Pokemon Center stores";
    if (name == "POK\xC3\xA9PARK") return "POK\xC3\x89PARK";   // Gen IV spelling
    return name;
}

void testMarkersEqualPinnedConstants() {
    // PKHeX 6501f0ab Game/Locations/Locations.cs, Locations5.cs, EncounterStatic5Radar.cs.
    const std::map<std::pair<uint8_t, uint16_t>, std::string> pinned{
        {{12, 55}, "Transfer from Gen III (Pal Park)"},            // Transfer3 = 0x37
        {{12, 2000}, "Day-Care marker (egg)"},                     // Daycare4 = 2000
        {{12, 2001}, "In-game (NPC) trade"},                       // LinkTrade4NPC = 2001
        {{12, 2002}, "Trade over link"},                           // LinkTrade4 = 2002
        {{22, 30001}, "Transfer from Gen IV (Pok\xC3\xA9 Transporter)"},   // Transfer4
        {{22, 30002}, "NPC / egg trade: in-game or Spin Trade egg"},       // LinkTrade5NPC (Spin Trade eggs)
        {{22, 30003}, "Trade over link"},                          // LinkTrade5
        {{22, 30010}, "Transfer: Crown Celebi, event not activated"},      // Transfer4_CelebiUnused
        {{22, 30011}, "Transfer: Crown Celebi, event activated"},          // Transfer4_CelebiUsed
        {{22, 30012}, "Transfer: Crown Beast, event not activated"},       // Transfer4_CrownUnused
        {{22, 30013}, "Transfer: Crown Beast, event activated"},           // Transfer4_CrownUsed
        {{22, 30015}, "Pok\xC3\xA9mon Dream Radar encounter"},     // EncounterStatic5Radar.Location
        {{22, 60002}, "Day-Care marker (egg)"},                    // Daycare5 = 60002
        {{22, 60003}, "Egg-location marker: Pok\xC3\xA9mon Breeder"},      // Breeder5, Locations5.Met6
    };
    for (const auto& [key, text] : pinned) {
        const auto special = Names::specialMetLocation(key.first, key.second);
        assert(special.full == text + kDot + std::to_string(key.second));
    }
    // Undocumented markers say so, with their id; never a guessed meaning.
    for (const uint16_t id : {30004, 30005, 30006, 30007, 30008, 30014, 60001}) {
        const auto special = Names::specialMetLocation(21, id);
        assert(special.full == "Native marker; meaning not documented" + kDot + std::to_string(id));
        assert(special.row == "Not documented" + kDot + std::to_string(id));
    }
    // Ordinary places and Gen III origins have no note.
    assert(Names::specialMetLocation(22, 4).empty() && Names::specialMetLocation(12, 1).empty());
    assert(Names::specialMetLocation(3, 255).empty() && Names::specialMetLocation(0, 40026).empty());
    std::cout << "transfer / trade / egg markers equal the pinned PKHeX constants ok\n";
}

void testEventClaimsTraceToNames() {
    int events = 0;
    for (const auto& origin : origins())
        for (const uint16_t id : origin.special) {
            const auto special = Names::specialMetLocation(origin.id, id);
            const std::string idText = "#" + std::to_string(id);
            assert(special.full.size() > kDot.size() + 1);
            assert(special.full.compare(special.full.size() - kDot.size() - std::to_string(id).size(),
                                        std::string::npos, kDot + std::to_string(id)) == 0);
            for (const char* forbidden : {"distribut", "legal", "recipient", "valid", "date", "given to",
                                          "unverified"})
                assert(special.full.find(forbidden) == std::string::npos &&
                       special.row.find(forbidden) == std::string::npos);
            const bool event = Names::isGen4Origin(origin.id) ? id >= 3000 && id <= 3076 : id > 40000 && id < 50000;
            if (!event) continue;
            ++events;
            assert(special.row.rfind("Event", 0) == 0 && special.full.rfind("Event", 0) == 0);
            const char* raw = Names::getMetLocationName(origin.id, id);
            const std::string name = raw ? raw : "";
            // A four-digit year in the line is the native name's own, or the Pokémon Event marker.
            for (std::size_t i = 0; i + 4 <= special.full.size(); ++i) {
                const std::string four = special.full.substr(i, 4);
                if (four.rfind("20", 0) != 0 || !std::isdigit(static_cast<unsigned char>(four[2])) ||
                    !std::isdigit(static_cast<unsigned char>(four[3])) ||
                    (i > 0 && (special.full[i - 1] == '#' || std::isdigit(static_cast<unsigned char>(special.full[i - 1])))))
                    continue;
                const auto marker = Names::metLocationEventYear(origin.id, id);
                assert(name.find(four) != std::string::npos || (marker && std::to_string(*marker) == four));
            }
            // A "native number" is the name's own, or the ja / de / fr POKÉPARK number.
            if (const auto at = special.full.find("native number "); at != std::string::npos) {
                const std::string number = special.full.substr(at + 14, special.full.find(kDot) - at - 14);
                const auto text = Names::metLocationNativeNumber(origin.id, id);
                assert(name.size() > number.size() + 1 &&
                       (name.compare(name.size() - number.size(), std::string::npos, number) == 0 ||
                        (text && std::to_string(*text) == number)));
            }
            // Expansions only where another language's native text spells them out.
            if (name.rfind("WCS", 0) == 0) assert(special.row == "Event: World Championship (WCS)");
            if (name.rfind("Worlds", 0) == 0) assert(special.row == "Event: Championship (Worlds)");
            if (name.rfind("VGE", 0) == 0) assert(special.row == "Event: video game event (VGE)");
            if (name.rfind("PC ", 0) == 0)
                assert(special.full == "Event: Pok\xC3\xA9mon Center " + name.substr(3) + " store" + kDot +
                                       std::to_string(id));
        }
    // Every Gen V event id (40001..40109, four origins) and every Gen IV one each game offers.
    int gen5 = 0;
    for (const auto& origin : origins())
        if (Names::isGen5Origin(origin.id))
            for (const uint16_t id : origin.special) gen5 += id > 40000 && id < 50000;
    assert(gen5 == 4 * 109 && events == gen5 + 5 * 77);   // + Gen IV 3000..3076, five origins
    // The owner's Black 2 screenshot rows.
    assert(Names::specialMetLocation(22, 40026).full == "Event: World Championship (WCS), 2012" + kDot + "40026");
    assert(Names::specialMetLocation(22, 40017).full == "Event: Pok\xC3\xA9mon Center Tokyo store" + kDot + "40017");
    assert(Names::specialMetLocation(22, 40104).full == "Event: POK\xC3\x89PARK, native number 16" + kDot + "40104");
    assert(Names::specialMetLocation(22, 40011).full == "Event: Pok\xC3\xA9mon movie, native number 17" + kDot + "40011");
    assert(Names::specialMetLocation(22, 40092).full == "Event: Pok\xC3\xA9mon Festa, native number 16" + kDot + "40092");
    assert(Names::specialMetLocation(22, 40080).full == "Event: Pok\xC3\xA9mon event, native year 2016" + kDot + "40080");
    assert(Names::specialMetLocation(12, 3004).full == "Event: Pok\xC3\xA9mon movie, native number 06" + kDot + "3004");
    std::cout << events << " event locations: years, numbers and expansions traced to native names ok\n";
}

void testRowsDifferUnlessSameKind() {
    for (const auto& origin : origins()) {
        std::map<std::string, std::set<std::string>> families;   // row kind -> families using it
        for (const uint16_t id : origin.special)
            families[Names::specialMetLocation(origin.id, id).row].insert(family(origin.id, id));
        for (const auto& [row, used] : families) {
            if (used.size() > 1) {
                std::cerr << int(origin.id) << ": \"" << row << "\" shared by";
                for (const auto& f : used) std::cerr << " [" << f << "]";
                std::cerr << '\n';
            }
            // One row line per kind: a numbered series, the Pokémon Center stores, the Crown events
            // (whose names, "Unknown location #30010", already differ), or a pinned marker kind.
            assert(used.size() == 1 || row == "Trade over link" || row == "Day-Care marker (egg)" ||
                   row.rfind("Transfer: Crown", 0) == 0);
        }
    }
    std::cout << "rows share a kind line only within one kind ok\n";
}

void testInfoLineFitsAndIsWired() {
    constexpr int kInfoRoom = G::rule(G::MenuType::LocationPicker).width - 195;
    int widest = 0;
    std::string worst;
    for (const auto& origin : origins())
        for (const uint16_t id : origin.special) {
            const auto full = Names::specialMetLocationNote(origin.id, id);
            const int w = G::textWidth(full, G::Text::Caption);
            if (w > widest) { widest = w; worst = full; }
        }
    if (widest > kInfoRoom) std::cerr << "info line clipped: " << worst << '\n';
    assert(widest <= kInfoRoom);
    // The picker rows use the kind line (EncounterContext45::detail); the header shows the full line.
    const std::string context = read("include/Integration/Encounter/EncounterContext45.h");
    assert(context.find("if (auto special = Names::specialMetLocation(origin_, location); !special.empty())\n"
                        "            return tagged(std::move(special.row));") != std::string::npos);
    assert(context.find("Event / special location") == std::string::npos);
    const std::string surface = read("src/UI/SharedPokemonEditorSurface.inc");
    assert(surface.find("const std::string note = Names::specialMetLocationNote(state.pickerMetContext.origin(), focused);") !=
           std::string::npos);
    assert(surface.find("note.empty() ? state.pickerMetContext.scope(state.pickerOtherHistories) : note, w - 195") !=
           std::string::npos);
    std::cout << "focused info line fits (" << widest << " / " << kInfoRoom << " px) and is wired ok\n";
}
} // namespace

int main() {
    testMarkersEqualPinnedConstants();
    testEventClaimsTraceToNames();
    testRowsDifferUnlessSameKind();
    testInfoLineFitsAndIsWired();
    std::cout << "test_special_met_location_notes: all passed\n";
    return 0;
}
