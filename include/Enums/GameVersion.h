#ifndef ENUMS_GAME_VERSION_H
#define ENUMS_GAME_VERSION_H

#include <cstdint>
#include <string>

namespace Enums {
    enum class GameVersion
    {
        Any = 0,
        Invalid = 255,
        FR = 4,
        LG = 5,
        GP = 42,
        GE = 43,
        SW = 44,
        SH = 45,
        PLA = 47,
        BD = 48,
        SP = 49,
        SL = 50,
        VL = 51,
        ZA = 52,

        // Presentation/read-model groups that are not stored PKM origin bytes.
        GSC = 70,  // Pokemon Gold/Silver/Crystal legacy read-only group
        RBY = 71,  // Pokemon Red/Blue/Yellow legacy read-only group
        FRLG = 72,
        GG = 73,
        SWSH = 74,
        BDSP = 75,
        SV = 76,

        Gen7B = 84,
        Gen8 = 85,
        Gen9 = 86
    };

    inline GameVersion getGameVersion(uint64_t titleId) {
        switch (titleId) {
            case 0x0100554023408000: return GameVersion::FR;
            case 0x010034D02340E000: return GameVersion::LG;
            case 0x010003F003A34000: return GameVersion::GP;
            case 0x0100187003A36000: return GameVersion::GE;
            case 0x0100ABF008968000: return GameVersion::SW;
            case 0x01008DB008C2C000: return GameVersion::SH;
            case 0x0100000011D90000: return GameVersion::BD;
            case 0x010018E011D92000: return GameVersion::SP;
            case 0x01001F5010DFA000: return GameVersion::PLA;
            case 0x0100A3D008C5C000: return GameVersion::SL;
            case 0x01008F6008C5E000: return GameVersion::VL;
            case 0x0100F43008C44000: return GameVersion::ZA;
            default: return GameVersion::Invalid;
        }
    }

    inline GameVersion getGameGroup(GameVersion version) {
        switch (version) {
            case GameVersion::FR:
            case GameVersion::LG: return GameVersion::FRLG;
            case GameVersion::GP:
            case GameVersion::GE: return GameVersion::GG;
            case GameVersion::SW:
            case GameVersion::SH: return GameVersion::SWSH;
            case GameVersion::BD:
            case GameVersion::SP: return GameVersion::BDSP;
            case GameVersion::SL:
            case GameVersion::VL: return GameVersion::SV;
            case GameVersion::PLA: return GameVersion::PLA;
            case GameVersion::ZA: return GameVersion::ZA;
            case GameVersion::GSC: return GameVersion::GSC;
            case GameVersion::RBY: return GameVersion::RBY;
            default: return GameVersion::Invalid;
        }
    }

    inline std::string getGameVersionName(GameVersion version) {
        switch (version) {
            case GameVersion::FR: return "FireRed";
            case GameVersion::LG: return "LeafGreen";
            case GameVersion::GSC: return "Gold/Silver/Crystal";
            case GameVersion::RBY: return "Red/Blue/Yellow";
            case GameVersion::FRLG: return "FireRed/LeafGreen";
            case GameVersion::GP: return "Let's Go Pikachu";
            case GameVersion::GE: return "Let's Go Eevee";
            case GameVersion::SW: return "Sword";
            case GameVersion::SH: return "Shield";
            case GameVersion::BD: return "Brilliant Diamond";
            case GameVersion::SP: return "Shining Pearl";
            case GameVersion::PLA: return "Legends Arceus";
            case GameVersion::SL: return "Scarlet";
            case GameVersion::VL: return "Violet";
            case GameVersion::ZA: return "Legends Z-A";
            case GameVersion::GG: return "Let's Go Pikachu/Eevee";
            case GameVersion::SWSH: return "Sword/Shield";
            case GameVersion::BDSP: return "Brilliant Diamond/Shining Pearl";
            case GameVersion::SV: return "Scarlet/Violet";
            default: return "Unknown";
        }
    }

    inline const char* getSaveFileName(GameVersion group) {
        switch (group) {
            case GameVersion::GG: return "savedata.bin";
            case GameVersion::SWSH: return "main";
            case GameVersion::BDSP: return "SaveData.bin";
            case GameVersion::PLA: return "main";
            case GameVersion::SV: return "main";
            default: return "main";
        }
    }

    inline bool usesSixDigitTrainerID(uint8_t version) {
        return (version >= 30 && version <= 33) || version >= 42;
    }

    inline std::string getOriginGameName(uint8_t version) {
        switch (version) {
            case 1: return "Sapphire"; case 2: return "Ruby"; case 3: return "Emerald";
            case 4: return "FireRed"; case 5: return "LeafGreen"; case 15: return "Colosseum/XD";
            case 7: return "HeartGold"; case 8: return "SoulSilver"; case 10: return "Diamond";
            case 11: return "Pearl"; case 12: return "Platinum"; case 16: return "Battle Revolution";
            case 20: return "White"; case 21: return "Black"; case 22: return "White 2"; case 23: return "Black 2";
            case 24: return "X"; case 25: return "Y"; case 26: return "Alpha Sapphire"; case 27: return "Omega Ruby";
            case 30: return "Sun"; case 31: return "Moon"; case 32: return "Ultra Sun"; case 33: return "Ultra Moon"; case 34: return "Pokemon GO";
            case 35: return "Red"; case 36: return "Blue"; case 37: return "Blue (JP)"; case 38: return "Yellow";
            case 39: return "Gold"; case 40: return "Silver"; case 41: return "Crystal";
            case 42: return "Let's Go Pikachu"; case 43: return "Let's Go Eevee";
            case 44: return "Sword"; case 45: return "Shield"; case 47: return "Legends Arceus";
            case 48: return "Brilliant Diamond"; case 49: return "Shining Pearl"; case 50: return "Scarlet";
            case 51: return "Violet"; case 52: return "Legends Z-A"; case 53: return "Champions";
            default: return "Unknown";
        }
    }

    inline int getVersionGeneration(uint8_t version) {
        if ((version >= 1 && version <= 5) || version == 15) return 3;
        if (version == 7 || version == 8 || (version >= 10 && version <= 12) || version == 16) return 4;
        if (version >= 20 && version <= 23) return 5;
        if (version >= 24 && version <= 27) return 6;
        if ((version >= 30 && version <= 34) || version == 42 || version == 43) return 7;
        if (version >= 44 && version <= 49) return 8;
        if (version >= 50 && version <= 52) return 9;
        return 0;
    }

    inline uint8_t getGroupRepVersion(GameVersion group) {
        switch (group) {
            case GameVersion::GSC: return 39;
            case GameVersion::RBY: return 35;
            case GameVersion::FRLG: return 4;
            case GameVersion::GG: return 42;
            case GameVersion::SWSH: return 44;
            case GameVersion::PLA: return 47;
            case GameVersion::BDSP: return 48;
            case GameVersion::SV: return 50;
            case GameVersion::ZA: return 52;
            default: return 50;
        }
    }

    inline uint8_t locationTableVersion(uint8_t originVersion, uint8_t formatVersion, bool isEggLocation) {
        if (originVersion == 34) return formatVersion;
        const int og = getVersionGeneration(originVersion);
        if (og >= 5) return originVersion;
        if (og == 4 && isEggLocation) return originVersion;
        return formatVersion;
    }

    inline bool isGameSupported(GameVersion version) {
        switch (version) {
            case GameVersion::FR: case GameVersion::LG: case GameVersion::GP: case GameVersion::GE:
            case GameVersion::SW: case GameVersion::SH: case GameVersion::ZA: case GameVersion::SL:
            case GameVersion::VL: case GameVersion::PLA: case GameVersion::BD: case GameVersion::SP:
                return true;
            default: return false;
        }
    }
}

#endif
