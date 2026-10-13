/**
 * GBLegality.h - Game Boy (Gen I/II) origin search.
 *
 * Answers the question every legality checker has to answer: is there at least
 * one legitimate way this Pokemon could exist? The search walks the evolution
 * chain, tries every encounter that could have produced the earliest form
 * (wild slots, gifts and statics, in-game trades, event distributions), and
 * accepts the first one under which every stored move has a source and the
 * Gen I catch-rate byte, type bytes and trade OT/nickname are consistent.
 *
 * The rules follow PKHeX.Core (EncounterGenerator1/12, LearnGroup1/2,
 * GBRestrictions, MiscG1Verifier), with GB cartridge-era settings: Gen I <-> II
 * Time Capsule trades allowed, Stadium/Stadium 2 gifts and tutors allowed.
 * Data comes from GBLegalityData (generated from PKHeX, cross-checked with pret).
 */

#ifndef LEGALITY_GB_LEGALITY_H
#define LEGALITY_GB_LEGALITY_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Legality::GB {

    enum class Language : uint8_t { Unknown, Japanese, International };

    /// The fields of a native PK1 that the origin search reads.
    struct Entity1 {
        uint8_t species = 0;                // National Dex number
        uint8_t level = 0;
        std::array<uint8_t, 4> moves{};
        std::array<uint8_t, 4> pp{};        // current PP per move slot
        std::array<uint8_t, 4> ppUps{};
        uint8_t catchRate = 0;              // byte 0x07; a Gen II held item after a Time Capsule trip
        uint8_t type1 = 0, type2 = 0;       // stored type bytes (Gen I type ids)
        std::array<uint8_t, 4> dvs{};       // ATK DEF SPE SPC
        uint16_t tid = 0;
        bool tradeTrainer = false;          // OT is the in-game-trade marker
        std::string otName;                 // decoded; compared against event OTs
        std::string nickname;               // decoded; compared against trade nicknames
        Language language = Language::Unknown;
    };

    /// The fields of a native PK2 that the origin search reads.
    struct Entity2 {
        uint8_t species = 0;                // National Dex number
        uint8_t level = 0;
        std::array<uint8_t, 4> moves{};
        std::array<uint8_t, 4> pp{};        // current PP per move slot
        std::array<uint8_t, 4> ppUps{};
        uint8_t heldItem = 0;
        std::array<uint8_t, 4> dvs{};       // ATK DEF SPE SPC
        uint16_t tid = 0;
        uint32_t exp = 0;
        uint8_t friendship = 0;             // egg hatch counter while unhatched
        bool isEgg = false;
        uint16_t caughtData = 0;            // Crystal met data; 0 when absent or wiped
        bool tradeTrainer = false;          // OT is the Gen I in-game-trade marker
        std::string otName;
        std::string nickname;
        Language language = Language::Unknown;
    };

    enum class EncounterKind : uint8_t { None, Wild, Static, Trade, Event, Egg };

    struct Origin {
        EncounterKind kind = EncounterKind::None;
        uint8_t generation = 0;             // game generation the encounter happened in
        uint8_t species = 0;
        uint8_t level = 0;                  // encounter level (minimum for wild slots)
        uint8_t versions = 0;               // GBData::Version bits
        uint8_t slotType = 0;               // GBData::SlotType1/2 for wild encounters
    };

    enum class Status : uint8_t { Legal, Illegal };

    enum class Area : uint8_t { Encounter, Moves, CatchRate, Types, Trainer, HeldItem, Egg, MetData };

    struct Problem {
        Area area;
        std::string text;
    };

    struct Result {
        Status status = Status::Illegal;
        Origin origin;                      // the explanation, or the closest candidate when Illegal
        bool timeCapsule = false;           // origin needs a Gen I <-> II Time Capsule trip
        std::vector<Problem> problems;      // empty when Legal
    };

    Result checkGen1(const Entity1& pk);
    Result checkGen2(const Entity2& pk);

    /// "wild Rattata in Red/Blue (level 2+)" style description for reports.
    std::string describe(const Origin& origin);
}

#endif
