#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"
#include "Legality/Gen3GameCubeStarterCorrelation.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3GameCubeFixedEncounterEvidence {

enum class Kind : uint8_t {
    None,
    ColosseumEspeon,
    ColosseumUmbreon,
    ColosseumPlusle,
    XdEevee,
    XdChikorita,
    XdCyndaquil,
    XdTotodile,
    XdElekidTrade,
    XdMedititeTrade,
    XdShuckleTrade,
    XdLarvitarTrade,
    ColosseumBonusPikachu,
    ColosseumAgetoCelebi,
    ColosseumMtBattleHoOh,
};

enum class Policy : uint8_t {
    ColosseumStarter,
    XdStarter,
    CxdTrainerAware,
    CxdStandard,
};

enum class TrainerNameSet : uint8_t {
    None,
    Duking,
    Hordel,
    BonusPikachu,
    AgetoCelebi,
    Mattle,
};

enum class NicknameSet : uint8_t {
    None,
    Zaprong,
};

struct Entry {
    Kind kind;
    uint16_t species;
    uint8_t level;
    uint8_t location;
    uint8_t originGame;
    uint16_t fixedTid;
    int32_t fixedSid;
    uint8_t otGender;
    uint8_t pokemonGender;
    bool fateful;
    bool neverShiny;
    bool japaneseOnly;
    TrainerNameSet trainerNames;
    NicknameSet nicknames;
    Policy policy;
};

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t otGender = 0;
    uint8_t pokemonGender = 0xFF;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint8_t metLevel = 0;
    uint8_t metLocation = 0;
    uint8_t ball = 0;
    bool egg = false;
    bool fateful = false;
    bool shiny = false;
    uint32_t pid = 0;
    std::array<uint8_t, 6> ivs{};
    std::u16string_view otName{};
    std::u16string_view nickname{};
};

struct Evidence {
    bool identityMatched = false;
    bool rngMatched = false;
    bool searchLimited = false;
    Kind kind = Kind::None;
    Gen3CxdPidIv::Variant cxdVariant = Gen3CxdPidIv::Variant::None;
    Gen3GameCubeStarterCorrelation::Variant starterVariant =
        Gen3GameCubeStarterCorrelation::Variant::None;
};

inline constexpr uint8_t kPokeBall = 4;
inline constexpr uint8_t kAnyPokemonGender = 0xFF;

// Direct Gen III fixed encounters sourced from pinned PKHeX:
// - Encounters3Colo.Starters + Gifts
// - Encounters3XD.Gifts + Trades
// - Encounters3RSE.ColoGiftsR + ColoGiftsS
// Starting moves are deliberately not stored here: these Pokémon may legally
// change their moves after acquisition. Evolved descendants are also a later
// provenance layer; this table proves direct-template identity only.
inline constexpr std::array<Entry, 14> kEntries{{
    {Kind::ColosseumEspeon, 196, 25, 254, 15, 0, -1, 0, 0, false, true, false,
     TrainerNameSet::None, NicknameSet::None, Policy::ColosseumStarter},
    {Kind::ColosseumUmbreon, 197, 26, 254, 15, 0, -1, 0, 0, false, true, false,
     TrainerNameSet::None, NicknameSet::None, Policy::ColosseumStarter},
    {Kind::ColosseumPlusle, 311, 13, 254, 15, 37149, 0, 0, kAnyPokemonGender,
     false, true, false, TrainerNameSet::Duking, NicknameSet::None,
     Policy::CxdTrainerAware},

    {Kind::XdEevee, 133, 10, 0, 15, 0, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::None, NicknameSet::None,
     Policy::XdStarter},
    {Kind::XdChikorita, 152, 5, 16, 15, 0, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::None, NicknameSet::None,
     Policy::CxdTrainerAware},
    {Kind::XdCyndaquil, 155, 5, 16, 15, 0, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::None, NicknameSet::None,
     Policy::CxdTrainerAware},
    {Kind::XdTotodile, 158, 5, 16, 15, 0, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::None, NicknameSet::None,
     Policy::CxdTrainerAware},

    {Kind::XdElekidTrade, 239, 20, 164, 15, 41400, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::Hordel, NicknameSet::Zaprong,
     Policy::CxdStandard},
    {Kind::XdMedititeTrade, 307, 20, 116, 15, 37149, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::Duking, NicknameSet::None,
     Policy::CxdStandard},
    {Kind::XdShuckleTrade, 213, 20, 116, 15, 37149, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::Duking, NicknameSet::None,
     Policy::CxdStandard},
    {Kind::XdLarvitarTrade, 246, 20, 116, 15, 37149, -1, 0, kAnyPokemonGender,
     true, false, false, TrainerNameSet::Duking, NicknameSet::None,
     Policy::CxdStandard},

    {Kind::ColosseumBonusPikachu, 25, 10, 255, 2, 31121, 0, 0,
     kAnyPokemonGender, false, true, true, TrainerNameSet::BonusPikachu,
     NicknameSet::None, Policy::CxdTrainerAware},
    {Kind::ColosseumAgetoCelebi, 251, 10, 255, 2, 31121, 0, 1,
     kAnyPokemonGender, false, true, true, TrainerNameSet::AgetoCelebi,
     NicknameSet::None, Policy::CxdTrainerAware},
    {Kind::ColosseumMtBattleHoOh, 250, 70, 255, 1, 10048, 0, 0,
     kAnyPokemonGender, false, true, false, TrainerNameSet::Mattle,
     NicknameSet::None, Policy::CxdTrainerAware},
}};

constexpr bool isGen3Language(uint8_t language) noexcept {
    return language >= 1 && language <= 7 && language != 6;
}

constexpr std::u16string_view trainerName(TrainerNameSet set,
                                          uint8_t language) noexcept {
    switch (set) {
        case TrainerNameSet::None:
            return {};
        case TrainerNameSet::Duking:
            switch (language) {
                case 1: return u"ギンザル";
                case 2: return u"DUKING";
                case 3: return u"DOKING";
                case 4: return u"RODRIGO";
                case 5: return u"GRAND";
                case 7: return u"GERMÁN";
                default: return {};
            }
        case TrainerNameSet::Hordel:
            switch (language) {
                case 1: return u"ダニー";
                case 2: return u"HORDEL";
                case 3: return u"VOLKER";
                case 4: return u"ODINO";
                case 5: return u"HORAZ";
                case 7: return u"HORDEL";
                default: return {};
            }
        case TrainerNameSet::BonusPikachu:
            switch (language) {
                case 1: return u"コロシアム";
                case 2: return u"COLOS";
                case 3: return u"COLOSSEUM";
                case 4: return u"ARENA";
                case 5: return u"COLOSSEUM";
                case 7: return u"CLAUDIO";
                default: return {};
            }
        case TrainerNameSet::AgetoCelebi:
            switch (language) {
                case 1: return u"アゲト";
                case 2: return u"AGATE";
                case 3: return u"SAMARAGD";
                case 4: return u"SOFO";
                case 5: return u"EMERITAE";
                case 7: return u"ÁGATA";
                default: return {};
            }
        case TrainerNameSet::Mattle:
            switch (language) {
                case 1: return u"バトルやま";
                case 2: return u"MATTLE";
                case 3: return u"MT BATAILL";
                case 4: return u"MONTE LOTT";
                case 5: return u"DUELLBERG";
                case 7: return u"ERNESTO";
                default: return {};
            }
    }
    return {};
}

constexpr std::u16string_view nickname(NicknameSet set,
                                       uint8_t language) noexcept {
    switch (set) {
        case NicknameSet::None:
            return {};
        case NicknameSet::Zaprong:
            switch (language) {
                case 1: return u"コンセント";
                case 2:
                case 3:
                case 4:
                case 5:
                case 7: return u"ZAPRONG";
                default: return {};
            }
    }
    return {};
}

constexpr bool identityMatches(const Entry& entry,
                               const Candidate& candidate) noexcept {
    if (candidate.species != entry.species ||
        candidate.originGame != entry.originGame ||
        candidate.metLevel != entry.level ||
        candidate.metLocation != entry.location ||
        candidate.ball != kPokeBall || candidate.egg ||
        candidate.fateful != entry.fateful)
        return false;

    if (!isGen3Language(candidate.language))
        return false;
    if (entry.japaneseOnly && candidate.language != 1)
        return false;
    if (candidate.otGender != entry.otGender)
        return false;
    if (entry.pokemonGender != kAnyPokemonGender &&
        candidate.pokemonGender != entry.pokemonGender)
        return false;
    if (entry.neverShiny && candidate.shiny)
        return false;

    if (entry.fixedTid != 0 && candidate.tid != entry.fixedTid)
        return false;
    if (entry.fixedSid >= 0 && candidate.sid != static_cast<uint16_t>(entry.fixedSid))
        return false;

    if (entry.trainerNames != TrainerNameSet::None &&
        candidate.otName != trainerName(entry.trainerNames, candidate.language))
        return false;
    if (entry.nicknames != NicknameSet::None &&
        candidate.nickname != nickname(entry.nicknames, candidate.language))
        return false;
    return true;
}

constexpr Evidence analyze(const Candidate& candidate) noexcept {
    for (const auto& entry : kEntries) {
        if (!identityMatches(entry, candidate))
            continue;

        Evidence evidence{};
        evidence.identityMatched = true;
        evidence.kind = entry.kind;

        switch (entry.policy) {
            case Policy::ColosseumStarter: {
                const auto result = Gen3GameCubeStarterCorrelation::analyzeColosseum(
                    candidate.species, candidate.pid, candidate.ivs,
                    candidate.tid, candidate.sid);
                evidence.rngMatched = result.matched;
                evidence.searchLimited = result.searchLimited;
                evidence.starterVariant = result.variant;
                return evidence;
            }
            case Policy::XdStarter: {
                const auto result = Gen3GameCubeStarterCorrelation::analyzeXdEevee(
                    candidate.species, candidate.pid, candidate.ivs,
                    candidate.tid, candidate.sid);
                evidence.rngMatched = result.matched;
                evidence.searchLimited = result.searchLimited;
                evidence.starterVariant = result.variant;
                return evidence;
            }
            case Policy::CxdTrainerAware: {
                const auto result = Gen3CxdPidIv::analyzeWithTrainer(
                    candidate.pid, candidate.ivs, candidate.tid, candidate.sid);
                evidence.rngMatched = result.matched;
                evidence.searchLimited = result.searchLimited;
                evidence.cxdVariant = result.variant;
                return evidence;
            }
            case Policy::CxdStandard: {
                const auto result = Gen3CxdPidIv::analyze(candidate.pid, candidate.ivs);
                evidence.rngMatched = result.matched;
                evidence.searchLimited = result.searchLimited;
                evidence.cxdVariant = result.variant;
                return evidence;
            }
        }
    }
    return {};
}

inline constexpr std::size_t kTemplateCount = kEntries.size();

} // namespace Legality::Gen3GameCubeFixedEncounterEvidence
