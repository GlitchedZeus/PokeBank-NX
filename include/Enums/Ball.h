#ifndef ENUMS_BALL_H
#define ENUMS_BALL_H

#include <cstdint>
#include <cstddef>
#include <vector>

#include "Enums/GameVersion.h"

namespace Enums {
    /// Ball IDs for the corresponding English ball name.
    enum class Ball
    {
        None,
        Master,
        Ultra,
        Great,
        Poke,
        Safari,
        Net,
        Dive,
        Nest,
        Repeat,
        Timer,
        Luxury,
        Premier,
        Dusk,
        Heal,
        Quick,
        Cherish,
        Fast,
        Level,
        Lure,
        Heavy,
        Love,
        Friend,
        Moon,
        Sport,
        Dream,
        Beast,
        // Legends: Arceus
        Strange,
        LAPoke,
        LAGreat,
        LAUltra,
        LAFeather,
        LAWing,
        LAJet,
        LAHeavy,
        LALeaden,
        LAGigaton,
        LAOrigin
    };

    /// English ball name for a stored ball id (indices match the Ball enum above).
    inline const char* getBallName(uint8_t id) {
        static const char* const names[] = {
            "(none)", "Master Ball", "Ultra Ball", "Great Ball", "Poké Ball", "Safari Ball",
            "Net Ball", "Dive Ball", "Nest Ball", "Repeat Ball", "Timer Ball", "Luxury Ball",
            "Premier Ball", "Dusk Ball", "Heal Ball", "Quick Ball", "Cherish Ball", "Fast Ball",
            "Level Ball", "Lure Ball", "Heavy Ball", "Love Ball", "Friend Ball", "Moon Ball",
            "Sport Ball", "Dream Ball", "Beast Ball", "Strange Ball", "Poké Ball", "Great Ball",
            "Ultra Ball", "Feather Ball", "Wing Ball", "Jet Ball", "Heavy Ball", "Leaden Ball",
            "Gigaton Ball", "Origin Ball"
        };
        return id < (sizeof(names) / sizeof(names[0])) ? names[id] : "(unknown)";
    }

    /// The ball ids a given game can actually use, for the per-game ball picker. Legends: Arceus is the
    /// odd one out -- it stores its own Hisui ball set (Strange + LA Poke..Origin, ids 27-37) and none of
    /// the standard balls; the standard games use ids 1-26 (Gen 3 / Let's Go carry a smaller subset).
    inline std::vector<uint8_t> getBallList(GameVersion group) {
        switch (group) {
            case GameVersion::PLA:  // Hisui: Strange (transfers) + Poke/Great/Ultra/Feather..Origin
                return {27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37};
            case GameVersion::FRLG: // Gen 3 balls (Master..Premier)
                return {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
            case GameVersion::GG:   // Let's Go: Master, Ultra, Great, Poke, Premier only
                return {1, 2, 3, 4, 12};
            default:                // SwSh / BDSP / SV / Z-A: the standard modern set
                return {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                        14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
        }
    }
}

#endif