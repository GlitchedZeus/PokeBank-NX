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

    /// The game language a record was made in, as its name bytes show it. Korean is
    /// Korean Gold/Silver (Hangul trainer name; there is no Korean Red/Blue/Yellow or
    /// Crystal); International is any other non-Japanese language.
    enum class Language : uint8_t { Unknown, Japanese, International, Korean };

    /// The fields of a native PK1 that the origin search reads.
    struct Entity1 {
        uint8_t species = 0;                // National Dex number
        uint8_t level = 0;
        std::array<uint8_t, 4> moves{};
        std::array<uint8_t, 4> pp{};        // current PP per move slot
        std::array<uint8_t, 4> ppUps{};
        uint32_t exp = 0;                   // total EXP (bytes 0x0E-0x10)
        uint8_t catchRate = 0;              // byte 0x07; a Gen II held item after a Time Capsule trip
        uint8_t type1 = 0, type2 = 0;       // stored type bytes (Gen I type ids)
        std::array<uint8_t, 4> dvs{};       // ATK DEF SPE SPC
        std::array<uint16_t, 5> statExp{};  // HP ATK DEF SPE SPC
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
        std::array<uint16_t, 5> statExp{};  // HP ATK DEF SPE SPC
        uint8_t pokerus = 0;                // strain << 4 | days
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
        bool gsBall = false;                // Japanese Crystal's GS Ball Celebi (not in PKHeX's GB-era set)
    };

    /// Legal: a legitimate origin exists and every check passed.
    /// Illegal: every possible origin is excluded by a definitive rule, or a
    /// definitive format rule fails.
    /// Uncertain: no origin was confirmed, but what rules them out is not
    /// definitive (a rule PKHeX does not apply, or an event catalog that is
    /// not known to be complete). Report as incomplete, never as Invalid.
    enum class Status : uint8_t { Legal, Illegal, Uncertain };

    enum class Area : uint8_t { Encounter, Moves, CatchRate, Types, Trainer, HeldItem, Egg, MetData, Stats, Misc };

    struct Problem {
        Area area;
        std::string text;
        bool definitive = true;             // false: not enough on its own to call the Pokemon illegal
    };

    struct Result {
        Status status = Status::Illegal;
        Origin origin;                      // the explanation, or the closest candidate when Illegal
        bool timeCapsule = false;           // origin needs a Gen I <-> II Time Capsule trip
        std::vector<Problem> problems;      // empty when Legal
    };

    Result checkGen1(const Entity1& pk);
    Result checkGen2(const Entity2& pk);

    /// The rules that hold whatever the origin and whatever else is wrong with the record
    /// (stat experience, Pokerus, level against EXP, an empty nickname). checkGen1/checkGen2
    /// already include them; this is for callers that cannot run the origin search on a
    /// malformed record.
    std::vector<Problem> formatProblems(const Entity1& pk);
    std::vector<Problem> formatProblems(const Entity2& pk);

    /// "wild Rattata in Red/Blue (level 2+)" style description for reports.
    std::string describe(const Origin& origin);
}

#endif
