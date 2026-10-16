#pragma once

// Generation-correct move facts for the shared Moves panel and Move chooser.
//
// Every number comes from a pinned, generated table already in the repo:
//   Gen I-IV power / accuracy / base PP  Names/MoveBattleStats.h (retail-decomp tables)
//   Gen III type, Gen IV type + category Names/MoveTypes34.h      (tools/gen_move_types34.py)
//   Gen V type / category / power / acc  Names/MoveBattleStats5.h (tools/gen_move_battle_stats5.py)
//
// Gen I-III have NO per-move damage category: physical/special follows the move's type.
// This header therefore reports category only for Gen IV/V (CategoryRule::PerMove); a Gen III
// move shows its type, never a Gen IV-style per-move category.

#include "Enums/GameVersion.h"
#include "Names/MoveBattleStats.h"
#include "Names/MoveBattleStats5.h"
#include "Names/MoveTypes34.h"

#include <cstdint>
#include <string>

namespace PokeBank::UIModel::SharedMoveFacts {

enum class Era : uint8_t { Unknown, Gen1, Gen2, Gen3, Gen4, Gen5 };
enum class CategoryRule : uint8_t { None, PerMove };

inline Era eraFor(Enums::GameVersion game) noexcept {
    using G = Enums::GameVersion;
    const G group = Enums::getGameGroup(game);
    const G g = group == G::Invalid ? game : group;
    switch (g) {
        case G::RBY: return Era::Gen1;
        case G::GSC: return Era::Gen2;
        case G::FRLG: return Era::Gen3; // RSE share the FRLG move group in this repo
        case G::DP: case G::PT: case G::HGSS: return Era::Gen4;
        case G::BW: case G::B2W2: return Era::Gen5;
        default: return Era::Unknown;
    }
}

// Highest move id that exists in the era (PKHeX MoveInfo1..5 table lengths - 1).
constexpr uint16_t maxMoveId(Era era) noexcept {
    switch (era) {
        case Era::Gen1: return 165;
        case Era::Gen2: return 251;
        case Era::Gen3: return 354;
        case Era::Gen4: return 467;
        case Era::Gen5: return 559;
        default: return 0;
    }
}

constexpr CategoryRule categoryRule(Era era) noexcept {
    return era == Era::Gen4 || era == Era::Gen5 ? CategoryRule::PerMove : CategoryRule::None;
}

inline constexpr uint8_t kNoType = 255;
inline constexpr uint16_t kHiddenPower = 237;

struct MoveFacts {
    bool exists = false;
    uint8_t type = kNoType;    // PKHeX type order; kNoType = "???" / unknown for this era
    bool typeVaries = false;   // Hidden Power: type is derived from IVs
    int8_t category = -1;      // 0 Physical, 1 Special, 2 Status; -1 = not a per-move field
    int power = -1;            // -1 = variable / no fixed base power
    int accuracy = -1;         // -1 = never misses / not applicable
    int basePP = 0;
};

inline MoveFacts facts(uint16_t move, Enums::GameVersion game) noexcept {
    MoveFacts f;
    const Era era = eraFor(game);
    if (move == 0 || move > maxMoveId(era)) return f;
    f.exists = true;
    f.typeVaries = move == kHiddenPower && era != Era::Gen1;
    if (era == Era::Gen5) {
        if (const auto* s = Names::getMoveBattleStats5(move)) {
            f.type = s->type;
            f.category = static_cast<int8_t>(s->category);
            f.power = s->power == Names::kMoveValueUndefined5 ? -1 : s->power;
            f.accuracy = s->accuracy == Names::kMoveValueUndefined5 ? -1 : s->accuracy;
            f.basePP = s->pp;
        }
        if (f.typeVaries) f.power = -1; // Gen V Hidden Power: 30-70 from IVs
        return f;
    }
    // Gen I-IV power/accuracy: the decomp tables store 0 for "none" and 1 for a damaging move
    // whose damage is not a base-power value (Seismic Toss, Night Shade, Sonic Boom...).
    // Gen II-IV Hidden Power is 1 as well (its power is IV-derived).
    if (const auto* s = Names::getMoveBattleStats(move,
            era == Era::Gen3 ? Enums::GameVersion::FRLG : game)) {
        f.power = s->power <= 1 ? -1 : s->power;
        f.accuracy = s->accuracy == 0 ? -1 : s->accuracy;
        f.basePP = s->pp;
    }
    if (era == Era::Gen3 && move < Names::kMoveType3.size()) {
        f.type = Names::kMoveType3[move];
    } else if (era == Era::Gen4 && move < Names::kMoveTypeCategory4.size()) {
        f.type = Names::kMoveTypeCategory4[move].type;
        f.category = static_cast<int8_t>(Names::kMoveTypeCategory4[move].category);
    }
    return f;
}

inline bool moveExists(uint16_t move, Enums::GameVersion game) noexcept {
    return move != 0 && move <= maxMoveId(eraFor(game));
}

// PKHeX type order names (Names/TypeNames.cpp uses the same order; kept inline here so the
// presentation header does not need a link dependency).
constexpr const char* typeName(uint8_t type) noexcept {
    constexpr const char* names[] = {"Normal", "Fighting", "Flying", "Poison", "Ground", "Rock",
        "Bug", "Ghost", "Steel", "Fire", "Water", "Grass", "Electric", "Psychic", "Ice",
        "Dragon", "Dark"};
    return type < 17 ? names[type] : "???";
}

inline std::string typeLabel(const MoveFacts& f) {
    if (!f.exists) return {};
    if (f.typeVaries) return "Varies";
    return typeName(f.type);
}

constexpr const char* categoryLabel(const MoveFacts& f) noexcept {
    switch (f.category) {
        case 0: return "Physical";
        case 1: return "Special";
        case 2: return "Status";
        default: return "";
    }
}

inline std::string powerLabel(const MoveFacts& f) {
    return f.exists && f.power > 0 ? std::to_string(f.power) : std::string("\xE2\x80\x94");
}

inline std::string accuracyLabel(const MoveFacts& f) {
    return f.exists && f.accuracy > 0 ? std::to_string(f.accuracy) : std::string("\xE2\x80\x94");
}

// "Electric · Special · Pwr 90 · Acc 100" (Gen IV/V) or "Electric · Pwr 95 · Acc 100" (Gen III).
inline std::string detailLine(const MoveFacts& f) {
    if (!f.exists) return {};
    std::string line = typeLabel(f);
    if (const char* category = categoryLabel(f); *category) {
        line += " \xE2\x80\xA2 ";
        line += category;
    }
    line += " \xE2\x80\xA2 Pwr " + powerLabel(f) + " \xE2\x80\xA2 Acc " + accuracyLabel(f);
    return line;
}

// Max PP with PP Ups, from the same base PP shown in the chooser: base * (5 + ups) / 5
// (PKHeX PKM.GetMovePP; pokeemerald CalculatePPWithBonus).
constexpr int maxPP(int basePP, uint8_t ppUps) noexcept {
    return basePP * (5 + (ppUps > 3 ? 3 : ppUps)) / 5;
}

} // namespace PokeBank::UIModel::SharedMoveFacts
