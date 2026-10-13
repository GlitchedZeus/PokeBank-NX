/**
 * GBLegality.cpp - Gen I/II origin search. See GBLegality.h.
 *
 * Structure mirrors PKHeX.Core so behaviour can be compared rule by rule:
 *   origin chain   EncounterOrigin.GetOriginChain12 + EvolutionGroup1/2.Devolve
 *   encounters     EncounterEnumerator1/2, Encounter{Slot,Static,Trade,Gift,Egg}{1,2}.IsMatchExact
 *   move history   EvolutionChain.GetEvolutionChainsAllGens (forward Evolve ranges)
 *   move sources   LearnGroup1/LearnGroup2, LearnSource1RB/1YW/2GS/2C/2Stadium
 *   catch rate     GBRestrictions + MiscG1Verifier
 *   Gen II extras  ItemVerifier, EggStateLegality, LevelVerifier.VerifyG1, MiscG1Verifier (met time)
 *
 * PK1 and PK2 share one search: a Gen I Pokemon may have come from Gen II through the
 * Time Capsule and a Gen II Pokemon from Gen I, so both generations' encounters and move
 * sources are tried, in the order PKHeX's EncounterGenerator12 and LearnGroup1/2 use.
 */

#include "Legality/GBLegality.h"
#include "Legality/GBLegalityData.h"

#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"

#include <algorithm>
#include <cstddef>

namespace Legality::GB {

namespace {

using namespace GBData;

constexpr uint8_t kMaxSpecies1 = 151;
constexpr uint8_t kMaxMove1 = 165;
constexpr uint8_t kMinWildLevel = 2;   // lowest level any GB encounter yields
constexpr uint8_t kEggLevel = 5;       // Gen II eggs hatch at level 5
constexpr uint8_t kMaxStages = 4;      // longest GB chain is baby -> 3 stages

constexpr uint8_t kMaxSpecies2 = 251;
constexpr uint8_t kMaxMove2 = 251;

constexpr uint8_t kSpeciesPikachu = 25, kSpeciesRaichu = 26;
constexpr uint8_t kSpeciesMagnemite = 81, kSpeciesMagneton = 82, kSpeciesDitto = 132;
constexpr uint8_t kSpeciesPorygon = 137, kSpeciesDragonite = 149, kSpeciesPsyduck = 54;
constexpr uint8_t kSpeciesCelebi = 251;
constexpr uint8_t kMoveSurf = 57, kMoveDizzyPunch = 146;
constexpr uint8_t kLocationLinkTrade2 = 126;  // Gen II met location for NPC trades
constexpr uint32_t kOddEggExp = 125;
constexpr uint8_t kSpeciesSmeargle = 235;
constexpr uint8_t kMoveSketch = 166;
constexpr uint8_t kRateLightBall = 163, kRateNormalBox = 167, kRateGorgeousBox = 168;
constexpr uint8_t kTypeSteel = 9;

// ---------------------------------------------------------------- small helpers

bool bit(const uint8_t* bits, int index) { return (bits[index >> 3] >> (index & 7)) & 1; }

// PKHeX ItemConverter.IsCatchRateHeldItem: catch-rate bytes that are also Gen II items.
constexpr uint8_t kCatchRateIsHeldItem[32] = {
    0x3F, 0xFF, 0xFF, 0xFD, 0xFF, 0xDF, 0x3B, 0xD2,
    0x03, 0xFF, 0xFF, 0xFB, 0xEF, 0xFF, 0xE7, 0x7E,
    0x18, 0x9C, 0xC5, 0xF1, 0xFB, 0x77, 0xF0, 0xBF,
    0xF7, 0xFF, 0xFF, 0xEF, 0xFF, 0xFF, 0x07, 0x00,
};
bool isHeldItemRate(uint8_t rate) { return bit(kCatchRateIsHeldItem, rate); }

bool isHoldableGen2(uint8_t item) { return item == 0 || bit(kHeldItems2, item); }

// GBRestrictions.CanVisitGen1: Gen I species, or Gen II evolutions of them.
bool canVisitGen1(uint8_t species) {
    switch (species) {
        case 169: case 182: case 186: case 196: case 197: case 199: case 208: case 212: case 230:
        case 233: case 242:
            return true;
        default:
            return species <= kMaxSpecies1;
    }
}

std::string speciesName(uint8_t species) { return Names::getSpeciesName(species); }
std::string moveName(uint8_t move) { return Names::getMoveName(move); }

bool isGen1Version(uint8_t v) { return (v & AnyGen1) != 0; }
bool hasYellow(uint8_t v) { return (v & Yellow) != 0; }

const Personal1& personal1(uint8_t species, bool yellow) {
    return yellow ? kPersonalY[species] : kPersonalRB[species];
}

// GBRestrictions.RateMatchesEncounter
bool rateMatchesEncounter(uint8_t species, uint8_t versions, uint8_t rate) {
    if (species > kMaxSpecies1)
        return false;  // PKHeX's Gen I table has no entry above Mew
    if (hasYellow(versions)) {
        if (rate == kPersonalY[species].catchRate)
            return true;
        if (versions == Yellow)
            return false;
    }
    return rate == kPersonalRB[species].catchRate;
}

bool rateMatchesEither(uint8_t rate, uint8_t species) {
    return species <= kMaxSpecies1 &&
           (rate == kPersonalRB[species].catchRate || rate == kPersonalY[species].catchRate);
}

// Species whose catch rate differs from their pre-evolution and that cannot be met
// directly, so a matching rate cannot have come from this species (GBRestrictions).
bool isSpeciesNotAvailableCatchRate(uint8_t species) {
    switch (species) {
        case 12: case 18: case 31: case 34: case 38: case 45: case 53: case 59: case 62:
        case 65: case 68: case 71: case 78: case 91: case 103: case 121: case 149:
            return true;
        default:
            return false;
    }
}

// EncounterGift1.IsTourOT / IsJapanOT: trainer names the Mew tour events used.
bool isEuropeTourOT(const std::string& ot) {
    for (const char* name : {"YOSHIRA", "YOSHIRB", "YOSHIBA", "YOSHIBB", "LINKE", "LINKW", "LUIGE", "LUIGW",
                             "LUIGIC", "YOSHIC", "EUROPE", "SWEDEN", "NORWAY", "FINLAND", "DENMARK", "AUSTRIA",
                             "UK", "NAL", "MARIO"})
        if (ot == name)
            return true;
    return false;
}

bool isJapanTourOT(const std::string& ot) {
    for (const char* name : {"マクハリ", "フクオカ", "トウキョー", "オーサカ", "サッポロ", "ナゴヤ", "マリオ", "クッパ",
                             "ルイージ", "ピーチ", "ヨッシー", "ドンキー"})
        if (ot == name)
            return true;
    return false;
}

bool isStadiumGiftSpecies(uint8_t species) {
    switch (species) {
        case 1: case 4: case 7: case 54: case 106: case 107: case 133: case 138: case 140:
            return true;
        default:
            return false;
    }
}

bool isLevelUpMethod(uint8_t method) {
    switch (method) {
        case EvoLevelUpFriendship: case EvoLevelUpFriendshipMorning: case EvoLevelUpFriendshipNight:
        case EvoLevelUp: case EvoLevelUpATK: case EvoLevelUpAeqD: case EvoLevelUpDEF:
            return true;
        default:
            return false;
    }
}

const Evolution* preEvolution(bool gen2, uint8_t species) {
    const Evolution* table = gen2 ? kEvolutions2 : kEvolutions1;
    const std::size_t count = gen2 ? kEvolutions2Count : kEvolutions1Count;
    for (std::size_t i = 0; i < count; ++i)
        if (table[i].to == species)
            return &table[i];
    return nullptr;
}

// Gen II DVs: shiny = DEF/SPE/SPC 10 and ATK in {2,3,6,7,10,11,14,15}.
bool isShinyDVs(const std::array<uint8_t, 4>& dv) {
    return dv[1] == 10 && dv[2] == 10 && dv[3] == 10 && (dv[0] & 2) != 0;
}

uint8_t hpDV(const std::array<uint8_t, 4>& dv) {
    return static_cast<uint8_t>(((dv[0] & 1) << 3) | ((dv[1] & 1) << 2) | ((dv[2] & 1) << 1) | (dv[3] & 1));
}

// Legal.GetIsFixedIVSequenceValidNoRand for GB formats: IVs are DVs, HP is derived,
// and both special stats read the single Special DV.
bool fixedDVsMatch(const uint8_t fixed[6], const std::array<uint8_t, 4>& dv) {
    if (fixed[0] == 255)
        return true;  // not fixed
    return fixed[0] == hpDV(dv) && fixed[1] == dv[0] && fixed[2] == dv[1] &&
           fixed[3] == dv[2] && fixed[4] == dv[3] && fixed[5] == dv[3];
}

// ---------------------------------------------------------------- learnsets

struct LevelSpan {
    const LevelMove* begin;
    const LevelMove* end;
};

enum class Env : uint8_t { RB, Y, GS, C };

LevelSpan levelUp(Env env, uint8_t species) {
    switch (env) {
        case Env::RB: return {kLevelUpRB + kLevelUpRBIndex[species], kLevelUpRB + kLevelUpRBIndex[species + 1]};
        case Env::Y: return {kLevelUpY + kLevelUpYIndex[species], kLevelUpY + kLevelUpYIndex[species + 1]};
        case Env::GS: return {kLevelUpGS + kLevelUpGSIndex[species], kLevelUpGS + kLevelUpGSIndex[species + 1]};
        case Env::C: return {kLevelUpC + kLevelUpCIndex[species], kLevelUpC + kLevelUpCIndex[species + 1]};
    }
    return {nullptr, nullptr};
}

/// Learnset.TryGetLevelLearnMove: level of the first listing of `move`, or 0.
uint8_t learnLevel(Env env, uint8_t species, uint8_t move) {
    const LevelSpan s = levelUp(env, species);
    for (const LevelMove* p = s.begin; p != s.end; ++p)
        if (p->move == move)
            return p->level;
    return 0;
}

bool tm1(uint8_t species, bool yellow, uint8_t move) {
    for (int i = 0; i < 55; ++i)
        if (kMachineMoves1[i] == move)
            return bit(personal1(species, yellow).tmhm, i);
    return false;
}

bool tm2(uint8_t species, bool crystal, uint8_t move) {
    const Personal2& p = crystal ? kPersonalC[species] : kPersonalGS[species];
    for (int i = 0; i < 57; ++i)
        if (kMachineMoves2[i] == move)
            return bit(p.tmhm, i);
    return false;
}

bool tutorC(uint8_t species, uint8_t move) {
    for (int i = 0; i < 3; ++i)
        if (kTutorMoves2[i] == move)
            return bit(kPersonalC[species].tmhm, 57 + i);
    return false;
}

bool eggMove(bool crystal, uint8_t species, uint8_t move) {
    const uint16_t* idx = crystal ? kEggMovesCIndex : kEggMovesGSIndex;
    const uint8_t* data = crystal ? kEggMovesC : kEggMovesGS;
    for (uint16_t i = idx[species]; i < idx[species + 1]; ++i)
        if (data[i] == move)
            return true;
    return false;
}

/// LearnsetStadium.CanRelearn: Stadium 2's reminder offers level-up moves up to `level`.
uint8_t stadiumRelearnLevel(uint8_t species, uint8_t move, uint8_t level) {
    for (uint16_t i = kStadium2RelearnIndex[species]; i < kStadium2RelearnIndex[species + 1]; ++i) {
        const Stadium2Move& m = kStadium2Relearn[i];
        if (level < m.level)
            return 0;  // sorted by level
        if (m.move == move && (m.sources & 0x0F) != 0)
            return m.level;
    }
    return 0;
}

/// Learnset.SetEncounterMoves: the last four distinct moves learned up to `level`,
/// on top of `moves` (Gen I level-1 moves live in the personal record).
void addEncounterMoves(Env env, uint8_t species, uint8_t level, std::array<uint8_t, 4>& moves) {
    int count = 0;
    while (count < 4 && moves[count] != 0)
        ++count;
    count &= 3;  // matches PKHeX's start index (LastIndexOfAnyExcept + 1) & 3
    int ctr = count;
    const LevelSpan s = levelUp(env, species);
    for (const LevelMove* p = s.begin; p != s.end; ++p) {
        if (p->level < 1)
            continue;
        if (p->level > level)
            break;
        if (std::find(moves.begin(), moves.end(), p->move) == moves.end())
            moves[(ctr++) & 3] = p->move;
    }
    if (ctr > 4) {
        const int rotation = ctr & 3;
        for (int r = 0; r < rotation; ++r)
            std::rotate(moves.begin(), moves.begin() + 1, moves.end());
    }
}

std::array<uint8_t, 4> gen1EncounterMoves(uint8_t species, uint8_t level, bool yellow) {
    const Personal1& p = personal1(species, yellow);
    std::array<uint8_t, 4> moves{p.baseMoves[0], p.baseMoves[1], p.baseMoves[2], p.baseMoves[3]};
    addEncounterMoves(yellow ? Env::Y : Env::RB, species, level, moves);
    return moves;
}

std::array<uint8_t, 4> gen2EncounterMoves(uint8_t species, uint8_t level, bool crystal) {
    std::array<uint8_t, 4> moves{};
    addEncounterMoves(crystal ? Env::C : Env::GS, species, level, moves);
    return moves;
}

// ---------------------------------------------------------------- evolution chains

struct Stage {
    uint8_t species = 0;
    uint8_t min = 0, max = 0;
    uint8_t method = 0;    // method that evolves this stage into the one above it
    uint8_t evoLevel = 0;  // level that method needs
};

struct Chain {
    std::array<Stage, kMaxStages> stage{};
    uint8_t count = 0;     // stage[0] is the current species, stage[count-1] the earliest
    int indexOf(uint8_t species) const {
        for (int i = 0; i < count; ++i)
            if (stage[i].species == species)
                return i;
        return -1;
    }
};

/// EncounterOrigin.GetOriginChain12: devolve from the current species, keeping the
/// highest level each earlier form can have held and the lowest it must have reached.
/// `levelMin` is the lowest level the Pokemon can have been met at.
Chain originChain(uint8_t species, uint8_t level, bool gen2, uint8_t levelMin = kMinWildLevel) {
    Chain c;
    c.stage[0].species = species;
    c.stage[0].max = level;
    c.count = 1;
    while (c.count < kMaxStages) {
        const Evolution* link = preEvolution(gen2, c.stage[c.count - 1].species);
        if (!link)
            break;
        const uint8_t curMax = c.stage[c.count - 1].max;
        const bool levelUp = isLevelUpMethod(link->method);
        if (levelUp) {  // EvolutionMethod.Check
            if (curMax < link->level || (link->level == 0 && curMax < 2) || curMax < levelMin + 1)
                break;
        }
        Stage pre;
        pre.species = link->from;
        pre.max = static_cast<uint8_t>(curMax - (levelUp ? 1 : 0));
        pre.method = link->method;
        pre.evoLevel = link->level;
        c.stage[c.count++] = pre;
    }
    // EvolutionUtil.CleanDevolve: minimum levels trickle up from the earliest form.
    for (int i = c.count - 1; i >= 0; --i) {
        Stage& s = c.stage[i];
        const uint8_t evolveAt = s.evoLevel;
        s.min = std::min(s.max, levelMin);
        levelMin = std::max(evolveAt, levelMin);
    }
    return c;
}

/// Forward Evolve from the encounter species: each later stage needs another level
/// up after the previous one and the evolution level itself. Returns false when the
/// encounter cannot grow into the current species by the current level.
bool historyFrom(const Chain& origin, int encIndex, uint8_t encLevel, uint8_t currentLevel, Chain& out) {
    out = Chain{};
    out.count = static_cast<uint8_t>(encIndex + 1);
    for (int i = 0; i <= encIndex; ++i)
        out.stage[i] = origin.stage[i];
    out.stage[encIndex].min = encLevel;
    out.stage[encIndex].max = currentLevel;
    for (int j = encIndex - 1; j >= 0; --j) {
        const Stage& below = out.stage[j + 1];
        const bool levelUp = isLevelUpMethod(below.method);
        const uint8_t needed = std::max<uint8_t>(static_cast<uint8_t>(below.min + (levelUp ? 1 : 0)),
                                                 levelUp ? below.evoLevel : 0);
        if (levelUp && (currentLevel < below.evoLevel || currentLevel < below.min + 1))
            return false;
        out.stage[j].min = needed;
        out.stage[j].max = currentLevel;
    }
    return true;
}


// ---------------------------------------------------------------- the Pokemon being checked

/// PK1 and PK2 fields in one place; `format` says which apply.
struct Subject {
    uint8_t format = 1;
    uint8_t species = 0, level = 0;
    std::array<uint8_t, 4> moves{};
    std::array<uint8_t, 4> dvs{};           // ATK DEF SPE SPC
    uint16_t tid = 0;
    bool tradeTrainer = false;
    std::string otName, nickname;
    Language language = Language::Unknown;
    // PK1 only
    uint8_t catchRate = 0, type1 = 0, type2 = 0;
    // PK2 only
    uint8_t heldItem = 0, friendship = 0;
    uint32_t exp = 0;
    bool egg = false;
    bool caught = false;                    // Crystal met data present
    uint8_t metLevel = 0, metLocation = 0, metTime = 0, otGender = 0;
};

// ---------------------------------------------------------------- encounters

struct Candidate {
    EncounterKind kind = EncounterKind::None;
    uint8_t generation = 1;
    uint8_t species = 0, level = 0, versions = 0, slotType = 0;
    std::array<uint8_t, 4> fixedMoves{};  // non-empty when the encounter dictates its moves
    bool hasFixedMoves = false;
    bool egg = false;                     // hatched from an egg (bred, gift or event)
    bool bred = false;                    // daycare egg: egg moves and inherited moves apply
    bool stadiumGift = false;             // Gen I Stadium gift (catch rate is a gift box)
    bool trade1 = false;
    bool gift2 = false;                   // Gen II event distribution
    bool oddEgg = false;
    uint8_t eggCycles = 0;                // fixed hatch cycles of a gift egg (0 = species')
    bool partial = false;                 // matched, but trainer/nickname/catch rate disagree
    std::string partialReason;
};

bool nameMatches(const TradeName* table, std::size_t count, uint8_t index, const std::string& name,
                 Language lang) {
    for (std::size_t i = 0; i < count; ++i) {
        const TradeName& t = table[i];
        if (t.index != index || name != t.name)
            continue;
        const bool jp = t.language == 1;
        if (lang == Language::Unknown || (lang == Language::Japanese) == jp)
            return true;
    }
    return false;
}

/// Languages (bit per PKHeX LanguageID) in which `name` is entry `index` of the table.
uint32_t nameLanguages(const TradeName* table, std::size_t count, uint8_t index, const std::string& name,
                       Language lang) {
    uint32_t mask = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const TradeName& t = table[i];
        if (t.index != index || name != t.name)
            continue;
        const bool jp = t.language == 1;
        if (lang == Language::Unknown || (lang == Language::Japanese) == jp)
            mask |= 1u << t.language;
    }
    return mask;
}

// EncounterSlot2.IsTreeAvailable: the headbutt trees a trainer ID can shake, per area.
bool isTreeAvailable(uint8_t location, uint8_t slotType, uint16_t tid) {
    static constexpr uint8_t kTreeLocations[] = {2,  4,  5,  8,  11, 12, 14, 15, 18, 20,
                                                 21, 25, 26, 34, 37, 38, 39, 91, 92};
    static constexpr uint32_t kTrees[] = {
        0x3FF3FF, 0x0FF3FF, 0x3FE3FF, 0x3EE3FF, 0x2403FF, 0x37F3FF, 0x3FF3FF, 0x0013FE, 0x2613FF, 0x3FF3FF,
        0x2B93FF, 0x3FF3FF, 0x1843FF, 0x3FF3FF, 0x3FF3FF, 0x3FF3FF, 0x2FF3FF, 0x2001FF, 0x2BB3FF,
    };
    static_assert(std::size(kTreeLocations) == std::size(kTrees));
    for (std::size_t i = 0; i < std::size(kTreeLocations); ++i) {
        if (kTreeLocations[i] != location)
            continue;
        const int pivot = tid % 10;
        const int bit = slotType == static_cast<uint8_t>(SlotType2::Headbutt) ? pivot : pivot + 12;
        return ((kTrees[i] >> bit) & 1) != 0;
    }
    return false;
}

bool isHeadbutt(uint8_t slotType) {
    return slotType == static_cast<uint8_t>(SlotType2::Headbutt) ||
           slotType == static_cast<uint8_t>(SlotType2::HeadbuttSpecial);
}

/// EncounterStatic2.DetectOddEggLanguage: the fixed trainer names of an unhatched Odd Egg.
bool isOddEggTrainer(const std::string& ot, Language lang) {
    if (lang == Language::Japanese)
        return ot == "なぞナゾ" || ot == "なぞ";
    return ot == "ODD" || ot == "BIZAR" || ot == "Strano" || ot == "Kurios" || ot == "Raro";
}

// ---------------------------------------------------------------- move verification

enum class Source : uint8_t {
    None, Initial, LevelUp, Machine, Tutor, Relearn, Egg, Inherit, Evolution, EmptySlotInvalid,
    EvolutionOrderInvalid
};

struct MoveResult {
    Source source = Source::None;
    uint8_t generation = 0;  // environment that supplied the move
    uint8_t stage = 0;       // history index of the species that learned it
    uint8_t level = 0;
    bool valid() const {
        return source != Source::None && source != Source::EmptySlotInvalid &&
               source != Source::EvolutionOrderInvalid;
    }
};

struct Learn {
    Source source = Source::None;
    uint8_t level = 0;
};

// LearnGroup1/2.GetIsPreferable: which of two valid explanations to keep.
bool preferable(const MoveResult& entry, const Learn& chk, int stage) {
    if (entry.source == Source::None)
        return true;
    if (entry.source == Source::LevelUp) {
        if (chk.source != Source::LevelUp)
            return true;
        if (entry.stage == stage)
            return entry.level < chk.level;
    } else if (entry.source == Source::Egg || entry.source == Source::Inherit) {
        return true;
    } else if (chk.source == Source::LevelUp) {
        return false;
    }
    return entry.stage < stage;
}

Learn canLearn1(uint8_t species, uint8_t move, const Stage& s, bool yellow) {
    if (move > kMaxMove1)
        return {};
    if (tm1(species, yellow, move))
        return {Source::Machine, 0};
    // Stadium's Surfing Pikachu minigame reward (GB cartridge era only).
    if (move == kMoveSurf && (species == kSpeciesPikachu || species == kSpeciesRaichu))
        return {Source::Tutor, 0};
    const uint8_t lv = learnLevel(yellow ? Env::Y : Env::RB, species, move);
    if (lv != 0 && lv >= s.min && lv <= s.max)
        return {Source::LevelUp, lv};
    return {};
}

Learn canLearn2(uint8_t species, uint8_t move, const Stage& s, bool crystal, bool allowEvolutionMove) {
    if (tm2(species, crystal, move))
        return {Source::Machine, 0};
    if (crystal && tutorC(species, move))
        return {Source::Tutor, 0};
    const uint8_t lv = learnLevel(crystal ? Env::C : Env::GS, species, move);
    if (lv != 0) {
        if (lv >= s.min && lv <= s.max)
            return {Source::LevelUp, lv};
        if (!crystal && lv == 1 && allowEvolutionMove)
            return {Source::Evolution, lv};
    }
    return {};
}

struct MoveCheck {
    std::array<MoveResult, 4> result{};
    bool anyGen1 = false, anyGen2 = false;
};

void markInitial(MoveCheck& mc, const std::array<uint8_t, 4>& current, const std::array<uint8_t, 4>& initial,
                 uint8_t generation) {
    for (uint8_t move : initial) {
        if (move == 0)
            break;
        for (int i = 0; i < 4; ++i)
            if (current[i] == move)
                mc.result[i] = {Source::Initial, generation, 0, 0};
    }
}

bool allValid(const MoveCheck& mc) {
    for (const MoveResult& r : mc.result)
        if (!r.valid())
            return false;
    return true;
}

/// Moves the encounter itself gives (LearnGroup1/2.GetEncounterMoves).
std::array<uint8_t, 4> initialMoves(const Candidate& enc) {
    if (enc.hasFixedMoves)
        return enc.fixedMoves;
    if (enc.generation == 1)  // Yellow data for Yellow and all-version encounters
        return gen1EncounterMoves(enc.species, enc.level, hasYellow(enc.versions));
    const bool crystal = (enc.versions & Crystal) != 0;
    // Eggs: Learnset.GetBaseEggMoves -- the last four moves known at the hatch level.
    return gen2EncounterMoves(enc.species, enc.egg ? kEggLevel : enc.level, crystal);
}

/// LearnGroup1.Check: Gen I encounter moves, then each Gen I form (Yellow, then R/B).
void checkGen1Sources(MoveCheck& mc, const std::array<uint8_t, 4>& current, const Candidate& enc,
                      const Chain& hist1) {
    if (enc.generation == 1)
        markInitial(mc, current, initialMoves(enc), 1);
    for (int stage = 0; stage < hist1.count; ++stage) {
        const Stage& s = hist1.stage[stage];
        for (int i = 3; i >= 0; --i) {
            MoveResult& entry = mc.result[i];
            if (current[i] == 0)
                continue;
            for (bool yellow : {true, false}) {
                const Learn chk = canLearn1(s.species, current[i], s, yellow);
                if (chk.source != Source::None && preferable(entry, chk, stage)) {
                    entry = {chk.source, 1, static_cast<uint8_t>(stage), chk.level};
                    break;
                }
            }
        }
    }
}

/// LearnGroup2.Check: Gen II encounter moves, each Gen II form (G/S, Crystal, Stadium 2's
/// reminder), and egg / inherited moves for daycare eggs.
void checkGen2Sources(MoveCheck& mc, const std::array<uint8_t, 4>& current, const Candidate& enc,
                      const Chain& hist2) {
    if (enc.generation == 2)
        markInitial(mc, current, initialMoves(enc), 2);
    for (int stage = 0; stage < hist2.count; ++stage) {
        const Stage& s = hist2.stage[stage];
        const bool encounterStage = stage == hist2.count - 1;
        for (int i = 3; i >= 0; --i) {
            MoveResult& entry = mc.result[i];
            if (current[i] == 0 || entry.valid())
                continue;
            Learn chk = canLearn2(s.species, current[i], s, false, !encounterStage);
            if (chk.source == Source::None)
                chk = canLearn2(s.species, current[i], s, true, !encounterStage);
            if (chk.source == Source::None) {
                const uint8_t lv = stadiumRelearnLevel(s.species, current[i], s.max);
                if (lv != 0)
                    chk = {Source::Relearn, lv};
            }
            if (chk.source != Source::None && preferable(entry, chk, stage))
                entry = {chk.source, 2, static_cast<uint8_t>(stage), chk.level};
        }
    }
    if (enc.bred) {  // only EncounterEgg2, never gift or event eggs
        const bool crystal = (enc.versions & Crystal) != 0;
        const uint8_t baby = enc.species;
        const uint8_t ratio = kPersonalC[baby].genderRatio;
        const bool inherit = (ratio != 0 && ratio != 255) || baby == 29 || baby == 32;
        for (int i = 3; i >= 0; --i) {
            MoveResult& entry = mc.result[i];
            if (current[i] == 0 || entry.valid())
                continue;
            if (eggMove(crystal, baby, current[i]))
                entry = {Source::Egg, 2, 0, 0};
            else if (inherit && learnLevel(crystal ? Env::C : Env::GS, baby, current[i]) != 0)
                entry = {Source::Inherit, 2, 0, 0};
        }
    }
}

/// Gen I's move bookkeeping rules: no move deleter, no relearner.
void flagGen1Bookkeeping(MoveCheck& mc, const std::array<uint8_t, 4>& current, const Candidate& enc,
                         const Chain& hist1, bool emptySlots) {
    const std::array<uint8_t, 4> initial1 = enc.generation == 1 ? initialMoves(enc) : std::array<uint8_t, 4>{};
    if (emptySlots && enc.generation == 1) {
        // LearnGroup1.FlagFishyMoveSlots: without a Gen II move deleter, a move slot can
        // only be empty if the Pokemon never knew that many moves.
        int missing = 0;
        for (uint8_t m : initial1) {
            if (m == 0)
                break;
            missing += std::find(current.begin(), current.end(), m) == current.end();
        }
        for (int i = 0; i < 4 && missing > 0; ++i) {
            if (current[i] == 0) {
                mc.result[i].source = Source::EmptySlotInvalid;
                --missing;
            }
        }
    }
    // A pre-evolution cannot learn a move at a higher level than a move its evolved
    // form learned by level-up: it evolved after that level, and Gen I has no move
    // relearner. (PKHeX's LearnGroup1.FlagEvolutionSlots states this rule, but its
    // GetPrevious fall-through means it never runs for PK1 files.)
    // Only a contradiction under EVERY way of learning both moves counts: each move's
    // options are its encounter/TM/tutor sources (no timing) and every in-range
    // level-up listing at every form, in Red/Blue and Yellow.
    struct Option { bool timed; int stage; uint8_t level; };
    auto options = [&](uint8_t move) {
        std::vector<Option> out;
        if (std::find(initial1.begin(), initial1.end(), move) != initial1.end())
            out.push_back({false, 0, 0});
        for (int stage = 0; stage < hist1.count; ++stage) {
            const Stage& s = hist1.stage[stage];
            for (bool yellow : {true, false}) {
                if (tm1(s.species, yellow, move) ||
                    (move == kMoveSurf && (s.species == kSpeciesPikachu || s.species == kSpeciesRaichu)))
                    out.push_back({false, stage, 0});
                const LevelSpan span = levelUp(yellow ? Env::Y : Env::RB, s.species);
                for (const LevelMove* p = span.begin; p != span.end; ++p)
                    if (p->move == move && p->level >= s.min && p->level <= s.max)
                        out.push_back({true, stage, p->level});
            }
        }
        return out;
    };
    auto conflict = [](const Option& a, const Option& b) {
        // b learned by an earlier form (higher history index) after a's level.
        return a.timed && b.timed && b.stage > a.stage && b.level > a.level;
    };
    for (int i = 0; i < 4; ++i) {
        if (current[i] == 0 || !mc.result[i].valid() || mc.result[i].generation != 1)
            continue;
        const std::vector<Option> optA = options(current[i]);
        for (int j = 0; j < 4; ++j) {
            if (j == i || current[j] == 0 || !mc.result[j].valid() || mc.result[j].generation != 1)
                continue;
            const std::vector<Option> optB = options(current[j]);
            bool always = !optA.empty() && !optB.empty();
            for (const Option& a : optA)
                for (const Option& b : optB)
                    always &= conflict(a, b);
            if (always) {
                mc.result[i].source = Source::EvolutionOrderInvalid;
                break;
            }
        }
    }
}

/// LearnVerifier over the groups a Pokemon of this format passes through: the current
/// generation first, then the other one when the history allows a Time Capsule trip.
MoveCheck checkMoves(const Subject& pk, const Candidate& enc, const Chain& hist1, const Chain& hist2,
                     bool otherGenerationReachable) {
    MoveCheck mc;
    const std::array<uint8_t, 4>& current = pk.moves;
    for (int i = 0; i < 4; ++i)
        if (current[i] == 0)
            mc.result[i] = {Source::Initial, 0, 0, 0};  // empty slots are judged separately

    if (pk.format == 1) {
        checkGen1Sources(mc, current, enc, hist1);
        if (otherGenerationReachable && !allValid(mc))
            checkGen2Sources(mc, current, enc, hist2);
        if (!otherGenerationReachable)
            flagGen1Bookkeeping(mc, current, enc, hist1, true);
    } else {
        checkGen2Sources(mc, current, enc, hist2);
        if (otherGenerationReachable && !allValid(mc)) {
            // LearnGroup2: a trip to Gen I needs every move to exist there; a Gen II-only
            // move must be re-learnable afterwards, so encounter and egg moves above Gen I's
            // range cannot survive it.
            for (int i = 0; i < 4; ++i) {
                MoveResult& r = mc.result[i];
                if (current[i] > kMaxMove1 && r.valid() &&
                    (r.source == Source::Initial || r.source == Source::Egg || r.source == Source::Inherit))
                    r = {};
            }
            checkGen1Sources(mc, current, enc, hist1);
            // LearnGroup1.GetPrevious is null for PK2, so the evolution-order rule applies.
            flagGen1Bookkeeping(mc, current, enc, hist1, false);
        }
    }
    for (const MoveResult& r : mc.result) {
        mc.anyGen1 |= r.valid() && r.generation == 1;
        mc.anyGen2 |= r.valid() && r.generation == 2;
    }
    return mc;
}

// ---------------------------------------------------------------- catch rate (Gen I)

enum class TimeCapsule : uint8_t { Indeterminate, Transferred21, Transferred12, NotTransferred, BadCatchRate };

bool catchRateMatchesEncounter(const Candidate& enc, uint8_t rate) {
    if (enc.kind == EncounterKind::Trade && enc.generation == 1)
        return true;
    if ((enc.kind == EncounterKind::Static || enc.kind == EncounterKind::Event) && enc.generation == 1)
        return !enc.partial;
    return rateMatchesEncounter(enc.species, enc.versions, rate);
}

/// MiscG1Verifier.VerifyMiscG1CatchRate; returns an empty string when valid.
std::string catchRateProblem(const Subject& pk, const Candidate& enc, bool anyGen2Move) {
    const uint8_t rate = pk.catchRate;
    TimeCapsule eval;
    if (enc.generation == 2) {
        eval = TimeCapsule::Transferred21;
    } else if (rate == 0) {
        eval = TimeCapsule::Transferred12;
    } else if (anyGen2Move) {
        eval = isHeldItemRate(rate) ? TimeCapsule::Transferred12 : TimeCapsule::BadCatchRate;
    } else {
        const bool item = isHeldItemRate(rate);
        if (catchRateMatchesEncounter(enc, rate))
            eval = item ? TimeCapsule::Indeterminate : TimeCapsule::NotTransferred;
        else
            eval = item ? TimeCapsule::Transferred12 : TimeCapsule::BadCatchRate;
    }

    const bool tradeback = eval != TimeCapsule::NotTransferred && eval != TimeCapsule::BadCatchRate;
    if (tradeback && isHeldItemRate(rate))
        return {};

    if (anyGen2Move)
        return "Catch rate byte must be a Gen II held item: a move was learned in Gold/Silver/Crystal";
    if ((enc.kind == EncounterKind::Event && enc.stadiumGift) || enc.trade1)
        return {};
    if (isSpeciesNotAvailableCatchRate(pk.species) && rate == kPersonalRB[pk.species].catchRate) {
        const bool dragoniteException = pk.species == kSpeciesDragonite && rate == 45 &&
                                        (enc.versions == BlueJP || hasYellow(enc.versions));
        if (!dragoniteException)
            return "Catch rate " + std::to_string(rate) + " belongs to " + speciesName(pk.species) +
                   ", which cannot be caught or received directly";
    }
    if (!rateMatchesEncounter(enc.species, enc.versions, rate))
        return "Catch rate " + std::to_string(rate) + " does not match " + speciesName(enc.species) +
               (eval == TimeCapsule::Transferred12 ? " or its evolution line" : "") +
               " and is not a Gen II held item";
    return {};
}

/// GBRestrictions.GetTradebackStatusRBY: can this PK1 have come from / visited Gen II?
bool gen2Possible(const Subject& pk, const Chain& chain1) {
    const uint8_t rate = pk.catchRate;
    if (rate == 0)
        return true;
    bool matchAny = false;
    for (int i = 0; i < chain1.count; ++i) {
        const uint8_t s = chain1.stage[i].species;
        if (!isSpeciesNotAvailableCatchRate(s) && rateMatchesEither(rate, s))
            matchAny = true;
    }
    if (!matchAny && (rate == kRateNormalBox || rate == kRateGorgeousBox) &&
        isStadiumGiftSpecies(chain1.stage[chain1.count - 1].species))
        matchAny = true;
    if (!matchAny)
        return true;
    return isHeldItemRate(rate);
}

// ---------------------------------------------------------------- types (Gen I)

bool gen1TypeExists(uint8_t t) { return t <= 8 || (t >= 0x14 && t <= 0x1A); }

std::string typeProblem(const Subject& pk) {
    const Personal1& p = kPersonalRB[pk.species];
    if (pk.species == kSpeciesPorygon) {
        // Conversion can give Porygon any type combination a Gen I species has.
        if (!gen1TypeExists(pk.type1) || !gen1TypeExists(pk.type2))
            return "Porygon has a type that does not exist in Gen I";
        for (int s = 1; s <= kMaxSpecies1; ++s)
            if (kPersonalRB[s].types[0] == pk.type1 && kPersonalRB[s].types[1] == pk.type2)
                return {};
        return "Porygon's type combination is not one any Gen I Pokemon has";
    }
    std::string out;
    if (pk.type1 != p.types[0])
        out = "First type byte does not match " + speciesName(pk.species);
    const bool steelMagnet = (pk.species == kSpeciesMagnemite || pk.species == kSpeciesMagneton) &&
                             pk.type2 == kTypeSteel;  // Stadium 2 writes Gen II Steel back
    if (pk.type2 != p.types[1] && !steelMagnet)
        out += std::string(out.empty() ? "" : "; ") + "Second type byte does not match " +
               speciesName(pk.species);
    return out;
}

// ---------------------------------------------------------------- candidate generation

/// History restricted to one generation's species (EvolutionUtil.SetHistory): skip leading
/// forms the game does not have, then keep the contiguous run it does.
Chain presentIn(const Chain& hist, uint8_t maxSpecies) {
    Chain out;
    int i = 0;
    while (i < hist.count && hist.stage[i].species > maxSpecies)
        ++i;
    for (; i < hist.count && hist.stage[i].species <= maxSpecies; ++i)
        out.stage[out.count++] = hist.stage[i];
    return out;
}

void addWild(std::vector<Candidate>& out, const WildSlot& w, uint8_t generation, uint8_t level) {
    for (const Candidate& o : out)
        if (o.kind == EncounterKind::Wild && o.species == w.species && o.versions == w.version &&
            o.level <= level)
            return;
    Candidate c;
    c.kind = EncounterKind::Wild;
    c.generation = generation;
    c.species = w.species;
    c.level = level;
    c.versions = w.version;
    c.slotType = w.type;
    out.push_back(c);
}

std::vector<Candidate> gen1Candidates(const Subject& pk, const Chain& chain) {
    std::vector<Candidate> out;
    const bool pk1 = pk.format == 1;
    const bool maybeJapanese = pk.language != Language::International;
    const bool maybeInternational = pk.language != Language::Japanese;

    // In-game trades (fixed OT, fixed nickname per language).
    for (std::size_t i = 0; i < kTrades1Count; ++i) {
        const Trade1& t = kTrades1[i];
        const int idx = chain.indexOf(t.species);
        if (idx < 0)
            continue;
        if (t.version == BlueJP && !maybeJapanese)
            continue;
        if (chain.stage[idx].max < t.levelMinGSC)  // tradeback-era minimum
            continue;
        Candidate c;
        c.kind = EncounterKind::Trade;
        c.generation = 1;
        c.species = t.species;
        c.level = t.levelMinGSC;
        c.versions = t.version;
        c.trade1 = true;
        if (!pk.tradeTrainer) {
            c.partial = true;
            c.partialReason = "In-game trade " + speciesName(t.species) +
                              " must keep the trade's original trainer";
        } else if (!nameMatches(kTradeNames1, kTradeNames1Count, t.nameIndex, pk.nickname, pk.language)) {
            c.partial = true;
            c.partialReason = "In-game trade " + speciesName(t.species) +
                              " must keep its fixed nickname";
        }
        out.push_back(c);
    }

    // Static encounters, gifts and Game Corner prizes.
    for (std::size_t i = 0; i < kStatics1Count; ++i) {
        const Static1& s = kStatics1[i];
        const int idx = chain.indexOf(s.species);
        if (idx < 0 || s.level > chain.stage[idx].max)
            continue;
        if (s.version == BlueJP && !maybeJapanese)
            continue;
        Candidate c;
        c.kind = EncounterKind::Static;
        c.generation = 1;
        c.species = s.species;
        c.level = s.level;
        c.versions = s.version;
        if (pk1) {
            const bool starterPikachu = s.version == Yellow && s.species == kSpeciesPikachu && s.level == 5;
            const uint8_t rate = pk.catchRate;
            const bool rateOk = isHeldItemRate(rate) ||
                                (starterPikachu ? rate == kRateLightBall
                                                : rateMatchesEncounter(s.species, s.version, rate));
            if (!rateOk) {
                c.partial = true;
                c.partialReason = "Catch rate does not match the " + speciesName(s.species) + " encounter";
            }
        }
        out.push_back(c);
    }

    // Wild slots: the lowest level per (species, version) is all the search needs.
    for (std::size_t i = 0; i < kWildSlotCount; ++i) {
        const WildSlot& w = kWildSlots[i];
        if (!isGen1Version(w.version))
            continue;
        const int idx = chain.indexOf(w.species);
        if (idx < 0 || w.minLevel > chain.stage[idx].max)
            continue;
        if (w.version == BlueJP && !maybeJapanese)
            continue;
        if (pk1) {
            const uint8_t expect = personal1(w.species, w.version == Yellow).catchRate;
            if (pk.catchRate != expect && !isHeldItemRate(pk.catchRate))
                continue;
        }
        addWild(out, w, 1, w.minLevel);
    }

    // GB-era event gifts (Stadium gifts, Mew tours).
    for (std::size_t i = 0; i < kGifts1Count; ++i) {
        const Gift1& g = kGifts1[i];
        const int idx = chain.indexOf(g.species);
        if (idx < 0 || g.level > chain.stage[idx].max)
            continue;
        if (g.language == 1 && !maybeJapanese)
            continue;
        if (g.language == 2 && !maybeInternational)
            continue;
        if (g.trainer == 3 || g.trainer == 4) {  // tour Mews: fixed DVs 10/1/12/5 and tour OTs
            if (pk.dvs != std::array<uint8_t, 4>{10, 1, 12, 5})
                continue;
            if (!(g.trainer == 3 ? isEuropeTourOT(pk.otName) : isJapanTourOT(pk.otName)))
                continue;
        }
        if (g.trainer == 2) {  // Stadium: OT STADIUM (or localized), TID 2000 / 1999
            const bool otOk = pk.otName == "STADIUM" || pk.otName == "STADE" || pk.otName == "STADIO" ||
                              pk.otName == "ESTADIO" || pk.otName == "スタジアム";
            const bool tidOk = pk.tid == 2000 || pk.tid == 1999;
            if (!otOk || !tidOk)
                continue;
        }
        Candidate c;
        c.kind = EncounterKind::Event;
        c.generation = 1;
        c.species = g.species;
        c.level = g.level;
        c.versions = Red | Green | BlueJP;
        c.stadiumGift = g.trainer == 2;
        c.hasFixedMoves = true;
        c.fixedMoves = {g.moves[0], g.moves[1], g.moves[2], g.moves[3]};
        if (pk1) {
            const uint8_t rate = pk.catchRate;
            bool rateOk = isHeldItemRate(rate);
            if (!rateOk) {
                if (c.stadiumGift)
                    rateOk = g.species == kSpeciesPsyduck ? rate == kRateGorgeousBox
                                                          : (rate == kRateNormalBox || rate == kRateGorgeousBox);
                else
                    rateOk = rate == kPersonalRB[g.species].catchRate;
            }
            if (!rateOk) {
                c.partial = true;
                c.partialReason = "Catch rate does not match the " + speciesName(g.species) + " event gift";
            }
        }
        out.push_back(c);
    }
    return out;
}

/// Breeding.IsAbleToHatchFromEgg for Gen I/II species.
bool canHatch(uint8_t species) {
    switch (species) {
        case kSpeciesDitto: case 144: case 145: case 146: case 150: case 151:  // Ditto, birds, Mewtwo, Mew
        case 201: case 243: case 244: case 245: case 249: case 250: case 251:  // Unown, beasts, Lugia, Ho-Oh, Celebi
            return false;
        default:
            return true;
    }
}

// Roaming beasts can be met on Johto routes 29-46 except 40 and 41 (EncounterStatic2).
bool isRoamLocation(uint8_t location) {
    constexpr uint64_t kRoam = 0b10'1000'1010'0100'0000'0110'0011'0100'1000'1001'0011'0100ULL;
    return location <= 45 && ((kRoam >> location) & 1) != 0;
}

/// EncounterStatic2 / EncounterGift2.IsMatchEggLocationInternal for a PK2.
bool matchEggLocation(const Subject& pk, bool encounterIsEgg) {
    if (pk.format != 2)
        return true;
    if (pk.egg) {
        if (!encounterIsEgg)
            return false;
        return !(pk.metLocation != 0 && pk.metLevel != 0);
    }
    if (pk.metLevel == 0 && pk.metLocation != 0)
        return false;
    if (pk.metLevel == 1)
        return true;
    return !(pk.metLocation == 0 && pk.metLevel != 0);
}

std::vector<Candidate> gen2Candidates(const Subject& pk, const Chain& chain) {
    std::vector<Candidate> out;
    const bool pk2 = pk.format == 2;
    const bool maybeJapanese = pk.language != Language::International;
    const bool hasMet = pk2 && pk.caught;
    // EncounterEnumerator2: a Crystal origin keeps its met data unless a Time Capsule trip
    // wiped it, which only species that can visit Gen I can have taken.
    const bool crystalOk = !pk2 || pk.caught || canVisitGen1(chain.stage[0].species);
    const bool gsOk = !(hasMet || (pk2 && pk.caught && pk.otGender == 1));

    for (std::size_t i = 0; i < kTrades2Count; ++i) {
        const Trade2& t = kTrades2[i];
        const int idx = chain.indexOf(t.species);
        if (idx < 0 || chain.stage[idx].max < t.level)
            continue;
        if (t.dvs[0] != 255 && (!fixedDVsMatch(t.dvs, pk.dvs) || pk.tid != t.tid))
            continue;
        if (hasMet && (pk.metLocation != kLocationLinkTrade2 || pk.metLevel != 0 ||
                       (t.dvs[0] != 255 && pk.otGender != t.otGender)))
            continue;
        Candidate c;
        c.kind = EncounterKind::Trade;
        c.generation = 2;
        c.species = t.species;
        c.level = t.level;
        c.versions = Gold | Silver | Crystal;
        // EncounterTrade2.DetectLanguage: nickname and trainer must agree on one language.
        const uint32_t nickLangs = nameLanguages(kTradeNames2, kTradeNames2Count, t.nameIndex, pk.nickname,
                                                 pk.language);
        const uint32_t otLangs = nameLanguages(kTradeNames2, kTradeNames2Count,
                                               static_cast<uint8_t>(t.nameIndex + 11), pk.otName, pk.language);
        if ((nickLangs & otLangs) == 0) {
            c.partial = true;
            c.partialReason = "In-game trade " + speciesName(t.species) +
                              " must keep its fixed trainer name and nickname";
        }
        out.push_back(c);
    }

    // Daycare eggs: the earliest form must be able to hatch, at level 5.
    {
        const Stage& lowest = chain.stage[chain.count - 1];
        const uint8_t baby = lowest.species;
        const bool hatchable = canHatch(baby);
        const bool base = preEvolution(true, baby) == nullptr;
        if (hatchable && base && lowest.min <= kEggLevel && kEggLevel <= lowest.max) {
            for (uint8_t v : {static_cast<uint8_t>(Gold | Silver), static_cast<uint8_t>(Crystal)}) {
                Candidate c;
                c.kind = EncounterKind::Egg;
                c.generation = 2;
                c.species = baby;
                c.level = kEggLevel;
                c.versions = v;
                c.egg = true;
                c.bred = true;
                out.push_back(c);
            }
        }
    }

    for (std::size_t i = 0; i < kStatics2Count; ++i) {
        const Static2& s = kStatics2[i];
        const bool oddEgg = s.egg && s.moves[0] != 0;
        // EncounterEnumerator2 offers the Odd Eggs before (and regardless of) its Crystal gate.
        if (s.version == Crystal && !crystalOk && !oddEgg)
            continue;
        if ((s.version == Gold || s.version == Silver || s.version == (Gold | Silver)) && !gsOk)
            continue;
        const int idx = chain.indexOf(s.species);
        if (idx < 0 || chain.stage[idx].max < s.level)
            continue;
        if (s.shinyAlways && !isShinyDVs(pk.dvs))
            continue;
        if (s.dvs[0] != 255 && s.shinyNever && isShinyDVs(pk.dvs))
            continue;
        if (!fixedDVsMatch(s.dvs, pk.dvs))
            continue;
        if (oddEgg) {
            if (std::find(pk.moves.begin(), pk.moves.end(), kMoveDizzyPunch) == pk.moves.end())
                continue;
            // An unhatched Odd Egg keeps its fixed EXP and trainer.
            if (pk2 && pk.egg && (pk.exp != kOddEggExp || pk.otGender != 0 || !isOddEggTrainer(pk.otName, pk.language)))
                continue;
        }
        if (!matchEggLocation(pk, s.egg))
            continue;
        if (hasMet) {
            const bool roamer = s.species == 243 || s.species == 244 || (s.species == 245 && s.location != 23);
            if (!s.egg) {
                if (roamer ? !isRoamLocation(pk.metLocation) : s.location != pk.metLocation)
                    continue;
            }
            if (pk.metLevel != (s.egg ? 1 : s.level))
                continue;
        } else if (pk2 && !s.egg && s.species == kSpeciesCelebi && (s.version & Crystal) != 0) {
            continue;  // Crystal met data cannot be wiped from Celebi
        }
        Candidate c;
        c.kind = s.egg ? EncounterKind::Egg : EncounterKind::Static;
        c.generation = 2;
        c.species = s.species;
        c.level = s.level;
        c.versions = s.version;
        c.egg = s.egg;
        c.oddEgg = oddEgg;
        c.eggCycles = oddEgg ? 20 : 0;
        if (s.moves[0] != 0) {
            c.hasFixedMoves = true;
            c.fixedMoves = {s.moves[0], s.moves[1], s.moves[2], s.moves[3]};
        }
        out.push_back(c);
    }

    for (std::size_t i = 0; i < kWildSlotCount; ++i) {
        const WildSlot& w = kWildSlots[i];
        if (isGen1Version(w.version))
            continue;
        if (w.version == Crystal ? !crystalOk : !gsOk)
            continue;
        const int idx = chain.indexOf(w.species);
        if (idx < 0)
            continue;
        uint8_t level = w.minLevel;
        if (hasMet) {
            // Met data pins the area, the level within the slot and the time of day.
            const bool timeOk = w.time == 0 || ((w.time >> pk.metTime) & 1) != 0;
            if (w.location != pk.metLocation || pk.metLevel < w.minLevel || pk.metLevel > w.maxLevel || !timeOk)
                continue;
            level = pk.metLevel;
        }
        if (level > chain.stage[idx].max)
            continue;
        if (isHeadbutt(w.type) && !isTreeAvailable(w.location, w.type, pk.tid))
            continue;  // PKHeX defers these and then flags them; no tree means no encounter
        addWild(out, w, 2, level);
    }

    for (std::size_t i = 0; i < kGifts2Count; ++i) {
        const Gift2& g = kGifts2[i];
        const int idx = chain.indexOf(g.species);
        if (idx < 0 || chain.stage[idx].max < g.level)
            continue;
        if (g.shiny && !isShinyDVs(pk.dvs))
            continue;
        if (pk.language == Language::Japanese && g.language != 1)
            continue;
        if (pk.language == Language::International && g.language == 1)
            continue;
        if (g.language == 1 && !maybeJapanese)
            continue;
        if (g.currentLevel != 0 && g.currentLevel > pk.level)
            continue;
        if (!matchEggLocation(pk, g.egg != 0))
            continue;
        if (pk2 && pk.egg && pk.friendship > 10)  // event eggs were sent with 10 hatch cycles
            continue;
        if (pk2 && !(g.egg && !pk.egg)) {
            // EncounterGift2.IsMatchLocation (Crystal-format gifts carry a location).
            if (g.location != 0 && hasMet && g.location != pk.metLocation)
                continue;
            if (g.location != 0 && !hasMet && pk.species == kSpeciesCelebi)
                continue;
        }
        if (hasMet && pk.metLevel != (g.egg ? 1 : g.level))  // EncounterGift2.IsMatchLevel
            continue;
        if (!g.egg || pk.egg) {
            if (g.trainer == 4) {  // Pokemon Center New York
                if (pk.otName != "PCNYa" && pk.otName != "PCNYb" && pk.otName != "PCNYc" && pk.otName != "PCNYd")
                    continue;
            } else if (g.trainer >= 1 && g.trainer <= 3) {
                const bool otOk = g.trainer == 1 ? pk.otName == "スタジアム"
                                : g.trainer == 2 ? pk.otName == "Stadium"
                                                 : (pk.otName == "Stadion" || pk.otName == "Stade" ||
                                                    pk.otName == "Stadio" || pk.otName == "Estadio");
                const bool tidOk = g.trainer == 3 ? pk.tid == 2001 : pk.tid == 2000;
                if (!otOk || !tidOk)
                    continue;
            }
        }
        Candidate c;
        c.kind = EncounterKind::Event;
        c.generation = 2;
        c.species = g.species;
        c.level = g.level;
        c.versions = static_cast<uint8_t>(g.location != 0 ? Crystal : (Gold | Silver));
        c.egg = g.egg != 0;
        c.gift2 = true;
        c.eggCycles = g.egg ? 10 : 0;
        c.hasFixedMoves = true;
        c.fixedMoves = {g.moves[0], g.moves[1], g.moves[2], g.moves[3]};
        out.push_back(c);
    }
    return out;
}

// ---------------------------------------------------------------- unhatched eggs and Smeargle (Gen II)

/// Learnset.GetBaseEggMoves: the last four level-up entries at or below the hatch level.
int baseEggMoves(uint8_t species, bool crystal, std::array<uint8_t, 4>& out) {
    const LevelSpan span = levelUp(crystal ? Env::C : Env::GS, species);
    int count = 0;
    for (const LevelMove* p = span.begin; p != span.end && p->level <= kEggLevel; ++p)
        ++count;
    const int start = count > 4 ? count - 4 : 0;
    count -= start;
    for (int i = 0; i < count; ++i)
        out[i] = span.begin[start + i].move;
    return count;
}

enum EggSource : uint8_t { EggNone, EggBase, EggFatherEgg, EggFatherTM, EggParentLevelUp, EggTutor, EggSourceMax };

struct BreedInfo {
    std::array<uint8_t, 4> moves{};
    std::array<uint8_t, 4> actual{};    // chosen source per slot
    std::array<uint8_t, 4> possible{};  // bitmask of EggSource per slot
    std::array<uint8_t, 4> base{};
    int baseCount = 0;
};

// MoveBreed2.VerifyBaseMoves: the leading base moves are the tail of the base list, in order.
bool verifyBaseMoves(const BreedInfo& b) {
    int count = 0;
    while (count < 4 && b.actual[count] == EggBase)
        ++count;
    if (b.baseCount < count)
        return false;
    if (b.moves[3] == 0 && count != b.baseCount)
        return false;
    for (int i = count - 1, j = b.baseCount - 1; i >= 0; --i, --j)
        if (b.moves[i] != b.base[j])
            return false;
    if (b.baseCount == count)
        return true;
    // A pushed-out base move can only come back after enough non-base moves.
    for (int i = count; i < 4; ++i) {
        if ((b.possible[i] & (1 << EggBase)) == 0)
            continue;
        int baseIndex = -1;
        for (int j = 0; j < b.baseCount; ++j)
            if (b.base[j] == b.moves[i]) {
                baseIndex = j;
                break;
            }
        const int min = 4 - b.baseCount + baseIndex;
        if (i < min + count)
            return false;
    }
    return true;
}

// MoveBreed2.RecurseMovesForOrigin
bool recurseOrigins(BreedInfo& b, int start, uint8_t type) {
    int i = start;
    do {
        if (type != EggBase && recurseOrigins(b, i, EggBase))
            return true;
        int flag = 1 << EggBase;
        if (type != EggBase)
            flag = ~flag;
        const uint8_t permit = b.possible[i];
        if ((permit & flag) == 0)
            return false;
        if (type == EggBase) {
            b.actual[i] = EggBase;
        } else {
            uint8_t first = EggFatherEgg;
            while (first < EggSourceMax && (permit & (1 << first)) == 0)
                ++first;
            b.actual[i] = first;
        }
    } while (--i >= 0);
    return verifyBaseMoves(b);
}

/// MoveBreed2.Validate: can a daycare egg hold exactly these moves, in this order?
bool validateBreed2(uint8_t species, bool crystal, const std::array<uint8_t, 4>& moves) {
    int count = 0;
    while (count < 4 && moves[count] != 0)
        ++count;
    if (count == 0)
        return false;
    BreedInfo b;
    b.moves = moves;
    b.baseCount = baseEggMoves(species, crystal, b.base);
    const uint8_t ratio = kPersonalC[species].genderRatio;
    const bool inherit = (ratio != 0 && ratio != 255) || species == 29 || species == 32;
    const Env env = crystal ? Env::C : Env::GS;
    for (int i = 0; i < count; ++i) {
        const uint8_t move = moves[i];
        if (std::find(b.base.begin(), b.base.begin() + b.baseCount, move) != b.base.begin() + b.baseCount)
            b.possible[i] |= 1 << EggBase;
        if (inherit && learnLevel(env, species, move) != 0)
            b.possible[i] |= 1 << EggParentLevelUp;
        if (eggMove(crystal, species, move))
            b.possible[i] |= 1 << EggFatherEgg;
        if (tm2(species, crystal, move))
            b.possible[i] |= 1 << EggFatherTM;
        if (crystal && tutorC(species, move))
            b.possible[i] |= 1 << EggTutor;
    }
    return recurseOrigins(b, count - 1, EggSourceMax);
}

/// LearnVerifierEgg: an unhatched egg knows exactly what it was created with.
std::string eggMovesProblem(const std::array<uint8_t, 4>& moves, uint8_t species, const std::array<uint8_t, 4>& fixed,
                            bool hasFixed, bool bred, bool crystal) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < i; ++j)
            if (moves[i] != 0 && moves[i] == moves[j])
                return "An egg cannot know the same move twice";
    if (bred)
        return validateBreed2(species, crystal, moves)
                   ? std::string()
                   : "No daycare pairing gives an egg these moves in this order";
    std::array<uint8_t, 4> expect{};
    if (hasFixed && fixed[0] != 0)
        expect = fixed;
    else
        baseEggMoves(species, crystal, expect);
    return moves == expect ? std::string() : "A gift egg knows only the moves it was given with";
}

/// MoveInfo.IsSketchValid for Gen II.
bool isSketchable2(uint8_t move) {
    switch (move) {
        case 0: case 102: case 118: case 119: case 120: case 144: case 153: case 165: case 214:
            return false;  // Mimic, Metronome, Mirror Move, Self-Destruct, Transform, Explosion, Struggle, Sleep Talk
        default:
            return move <= kMaxMove2;
    }
}

// ---------------------------------------------------------------- PP

uint8_t basePP(uint8_t format, uint8_t move) {
    if (format == 1)
        return move <= kMaxMove1 ? kMovePP1[move] : 0;
    return move <= kMaxMove2 ? kMovePP2[move] : 0;
}

uint8_t maxPP(uint8_t format, uint8_t move, uint8_t ppUps) {  // GBPKM.GetMovePP
    const uint8_t pp = basePP(format, move);
    return static_cast<uint8_t>(pp + ppUps * std::min(7, pp / 5));
}

/// MovePPVerifier for GB formats.
std::string ppProblem(uint8_t format, const std::array<uint8_t, 4>& moves, const std::array<uint8_t, 4>& pp,
                      const std::array<uint8_t, 4>& ups, bool egg) {
    for (int i = 0; i < 4; ++i) {
        const std::string slot = "Move slot " + std::to_string(i + 1);
        if (egg) {
            if (ups[i] != 0)
                return "An egg cannot have PP Ups";
            if (pp[i] != basePP(format, moves[i]))
                return slot + ": an egg's moves have their full base PP";
            continue;
        }
        if (ups[i] != 0 && (moves[i] == 0 || moves[i] == kMoveSketch))
            return slot + " has PP Ups that cannot be applied";
        if (pp[i] > maxPP(format, moves[i], ups[i]))
            return slot + " has more PP than the move allows";
    }
    return {};
}

// ---------------------------------------------------------------- evaluation

struct Evaluation {
    Candidate enc;
    std::vector<Problem> problems;
    bool timeCapsule = false;
};

Evaluation evaluate(const Subject& pk, const Candidate& enc, const Chain& chain, bool otherGeneration) {
    Evaluation ev;
    ev.enc = enc;
    const int idx = chain.indexOf(enc.species);

    Chain hist;
    if (idx < 0 || !historyFrom(chain, idx, enc.level, pk.level, hist)) {
        ev.problems.push_back({Area::Encounter, speciesName(enc.species) + " met at level " +
                                                    std::to_string(enc.level) + " cannot become " +
                                                    speciesName(pk.species) + " by level " +
                                                    std::to_string(pk.level)});
        return ev;
    }
    const Chain hist1 = presentIn(hist, kMaxSpecies1);
    const Chain& hist2 = hist;

    // Can the moves also come from the other generation? PK1: through Gen II when the
    // encounter was there or the catch-rate byte is a Gen II item. PK2: through Gen I when
    // the encounter was there or a form of it existed in Gen I.
    bool reachable;
    if (pk.format == 1)
        reachable = enc.generation == 2 || (otherGeneration && isHeldItemRate(pk.catchRate));
    else
        reachable = enc.generation == 1 || hist1.count != 0;
    MoveCheck mc;
    if (pk.format == 2 && pk.egg) {
        // LearnVerifierEgg: the moveset is judged as a whole, in order.
        const std::string eggMoves = eggMovesProblem(pk.moves, enc.species, enc.fixedMoves, enc.hasFixedMoves,
                                                     enc.bred, (enc.versions & Crystal) != 0);
        if (!eggMoves.empty())
            ev.problems.push_back({Area::Moves, eggMoves});
        for (MoveResult& r : mc.result)
            r = {Source::Initial, 2, 0, 0};
    } else if (enc.species == kSpeciesSmeargle) {
        // LearnVerifierHistory.VerifySmeargle: Sketch copies nearly anything.
        for (int i = 0; i < 4; ++i)
            mc.result[i] = pk.moves[i] == 0 || isSketchable2(pk.moves[i]) ? MoveResult{Source::Initial, 2, 0, 0}
                                                                         : MoveResult{};
    } else {
        mc = checkMoves(pk, enc, hist1, hist2, reachable);
    }
    ev.timeCapsule = enc.generation != pk.format || (pk.format == 1 ? mc.anyGen2 : mc.anyGen1);

    for (int i = 0; i < 4; ++i) {
        const uint8_t move = pk.moves[i];
        const MoveResult& r = mc.result[i];
        if (r.source == Source::EmptySlotInvalid) {
            ev.problems.push_back({Area::Moves, "Move slot " + std::to_string(i + 1) +
                                                    " is empty, but a Gen I Pokemon cannot forget the moves it "
                                                    "started with"});
        } else if (r.source == Source::EvolutionOrderInvalid) {
            ev.problems.push_back({Area::Moves, moveName(move) + " was learned at a level its pre-evolution "
                                                                 "could not still have been at"});
        } else if (!r.valid()) {
            ev.problems.push_back({Area::Moves, speciesName(pk.species) + " cannot learn " + moveName(move) +
                                                    " from this origin"});
        }
    }
    if (enc.partial)
        ev.problems.push_back({enc.kind == EncounterKind::Trade ? Area::Trainer : Area::CatchRate,
                               enc.partialReason});
    if (pk.tradeTrainer && !(enc.kind == EncounterKind::Trade && enc.generation == 1))
        ev.problems.push_back({Area::Trainer, "The in-game-trade trainer marker only appears on Pokemon "
                                              "received in a Red/Blue/Yellow in-game trade"});

    if (pk.format == 1) {
        if (mc.anyGen2 && !isHeldItemRate(pk.catchRate))
            ev.problems.push_back({Area::CatchRate, "A move only learnable in Gold/Silver/Crystal needs a "
                                                    "Time Capsule trip, but the catch rate byte is not a Gen II "
                                                    "item"});
        const std::string rate = catchRateProblem(pk, enc, mc.anyGen2);
        if (!rate.empty())
            ev.problems.push_back({Area::CatchRate, rate});
    } else {
        // EncounterVerifier.VerifyEncounterEgg2 / VerifyUnhatchedEgg2: Crystal records eggs at
        // met level 1, with no location until they hatch.
        if (enc.egg && pk.caught) {
            if (pk.metLevel != 1)
                ev.problems.push_back({Area::MetData, "Crystal records a hatched egg at met level 1"});
            else if (pk.egg ? pk.metLocation != 0 : pk.metLocation > 95)
                ev.problems.push_back({Area::MetData, "Crystal egg met location is not possible"});
        }
        // TrainerNameVerifier.VerifyOTGB: Gen I and non-egg event Pokemon have a male OT.
        if (pk.caught && pk.otGender == 1 && (enc.generation == 1 || (enc.gift2 && !enc.egg)))
            ev.problems.push_back({Area::Trainer, "This origin cannot have a female original trainer"});
        // MiscG1Verifier: Crystal records the time of day, except for gifts and trades.
        if (pk.caught) {
            const bool fixedTime = (enc.gift2 && (!enc.egg || pk.egg)) ||
                                   (enc.kind == EncounterKind::Trade && enc.generation == 2);
            const bool timeOk = fixedTime ? pk.metTime == 0 : (pk.metTime >= 1 && pk.metTime <= 3);
            if (!timeOk)
                ev.problems.push_back({Area::MetData, "Crystal met time of day does not fit this origin"});
        }
        // EggStateLegality.GetIsEggHatchCyclesValid: 1 .. the encounter's hatch cycles.
        if (pk.egg) {
            const uint8_t maxCycles = enc.eggCycles != 0 ? enc.eggCycles : kPersonalC[pk.species].hatchCycles;
            if (pk.friendship < 1 || pk.friendship > maxCycles)
                ev.problems.push_back({Area::Egg, "Egg hatch counter is outside 1-" + std::to_string(maxCycles)});
            const uint32_t expect = enc.oddEgg ? kOddEggExp
                                               : Pokemon::getExpForLevel(kEggLevel, Pokemon::getGrowthRate(pk.species));
            if (pk.exp != expect)
                ev.problems.push_back({Area::Egg, "An unhatched egg has exactly " + std::to_string(expect) + " EXP"});
        }
    }
    return ev;
}

std::string versionsText(uint8_t v) {
    std::string out;
    auto add = [&](uint8_t bitMask, const char* name) {
        if (v & bitMask) {
            if (!out.empty())
                out += "/";
            out += name;
        }
    };
    if ((v & (Red | Green | BlueJP)) == (Red | Green | BlueJP)) {
        out = "Red/Blue";
    } else {
        add(Red, "Red");
        add(Green, "Blue");
        add(BlueJP, "Blue (Japan)");
    }
    add(Yellow, "Yellow");
    if ((v & (Gold | Silver)) == (Gold | Silver)) {
        if (!out.empty())
            out += "/";
        out += "Gold/Silver";
    } else {
        add(Gold, "Gold");
        add(Silver, "Silver");
    }
    add(Crystal, "Crystal");
    return out;
}

Result search(const Subject& pk, const Chain& chainOwn, const Chain& chainOther, bool otherGeneration,
              const std::vector<Problem>& extra) {
    Result res;
    std::vector<Candidate> candidates;
    if (pk.format == 1) {
        candidates = gen1Candidates(pk, chainOwn);
        if (otherGeneration) {
            std::vector<Candidate> more = gen2Candidates(pk, chainOther);
            candidates.insert(candidates.end(), more.begin(), more.end());
        }
    } else {
        // EncounterGenerator12: Gen I origins for PK2 come through the same chain.
        candidates = gen2Candidates(pk, chainOwn);
        if (otherGeneration) {
            std::vector<Candidate> more = gen1Candidates(pk, chainOwn);
            candidates.insert(candidates.end(), more.begin(), more.end());
        }
    }

    bool haveBest = false;
    Evaluation best;
    for (const Candidate& c : candidates) {
        const Chain& chain = pk.format == 1 ? (c.generation == 1 ? chainOwn : chainOther) : chainOwn;
        Evaluation ev = evaluate(pk, c, chain, otherGeneration);
        if (ev.problems.empty()) {
            res.status = extra.empty() ? Status::Legal : Status::Illegal;
            res.origin = {c.kind, c.generation, c.species, c.level, c.versions, c.slotType};
            res.timeCapsule = ev.timeCapsule;
            res.problems = extra;
            return res;
        }
        if (!haveBest || ev.problems.size() < best.problems.size()) {
            best = std::move(ev);
            haveBest = true;
        }
    }

    res.status = Status::Illegal;
    if (!haveBest) {
        const char* games = pk.format == 1
            ? (otherGeneration ? "Red/Blue/Yellow or Gold/Silver/Crystal" : "Red/Blue/Yellow")
            : (otherGeneration ? "Gold/Silver/Crystal or Red/Blue/Yellow" : "Gold/Silver/Crystal");
        res.problems.push_back({Area::Encounter, std::string("No ") + games + " encounter can produce " +
                                                     speciesName(pk.species) + " at level " +
                                                     std::to_string(pk.level) +
                                                     (pk.caught ? " with this Crystal met data" : "")});
    } else {
        const Candidate& c = best.enc;
        res.origin = {c.kind, c.generation, c.species, c.level, c.versions, c.slotType};
        res.timeCapsule = best.timeCapsule;
        res.problems = std::move(best.problems);
    }
    res.problems.insert(res.problems.end(), extra.begin(), extra.end());
    return res;
}

/// Shared format checks; returns a problem when the record cannot be searched at all.
bool movesWellFormed(const std::array<uint8_t, 4>& moves, uint8_t maxMove, const char* gen, Result& res) {
    for (uint8_t m : moves) {
        if (m > maxMove) {
            res.problems.push_back({Area::Moves, "Move id " + std::to_string(m) + " does not exist in " + gen});
            return false;
        }
    }
    if (moves[0] == 0) {
        res.problems.push_back({Area::Moves, "A Pokemon always knows at least one move"});
        return false;
    }
    for (int i = 1; i < 4; ++i) {
        if (moves[i] != 0 && moves[i - 1] == 0) {
            res.problems.push_back({Area::Moves, "Moves are not packed: an empty slot comes before a move"});
            return false;
        }
    }
    return true;
}

} // namespace

std::string describe(const Origin& o) {
    if (o.kind == EncounterKind::None)
        return "no known origin";
    std::string kind;
    switch (o.kind) {
        case EncounterKind::Wild: kind = "wild"; break;
        case EncounterKind::Static: kind = "gift/static"; break;
        case EncounterKind::Trade: kind = "in-game trade"; break;
        case EncounterKind::Event: kind = "event"; break;
        case EncounterKind::Egg: kind = "egg"; break;
        case EncounterKind::None: break;
    }
    std::string out = kind + " " + speciesName(o.species) + " in " + versionsText(o.versions);
    out += o.kind == EncounterKind::Egg ? " (hatched at level 5)"
                                        : " (level " + std::to_string(o.level) +
                                              (o.kind == EncounterKind::Wild || o.kind == EncounterKind::Trade
                                                   ? "+)"
                                                   : ")");
    return out;
}

Result checkGen1(const Entity1& e) {
    Result res;
    if (e.species == 0 || e.species > kMaxSpecies1) {
        res.problems.push_back({Area::Encounter, "Species does not exist in Gen I"});
        return res;
    }
    if (e.level < 1 || e.level > 100) {
        res.problems.push_back({Area::Encounter, "Level is outside 1-100"});
        return res;
    }
    if (!movesWellFormed(e.moves, kMaxMove1, "Gen I", res))
        return res;

    Subject pk;
    pk.format = 1;
    pk.species = e.species;
    pk.level = e.level;
    pk.moves = e.moves;
    pk.dvs = e.dvs;
    pk.tid = e.tid;
    pk.tradeTrainer = e.tradeTrainer;
    pk.otName = e.otName;
    pk.nickname = e.nickname;
    pk.language = e.language;
    pk.catchRate = e.catchRate;
    pk.type1 = e.type1;
    pk.type2 = e.type2;

    // Problems that do not depend on the origin.
    std::vector<Problem> extra;
    if (const std::string types = typeProblem(pk); !types.empty())
        extra.push_back({Area::Types, types});
    if (const std::string pp = ppProblem(1, e.moves, e.pp, e.ppUps, false); !pp.empty())
        extra.push_back({Area::Moves, pp});

    const Chain chain1 = originChain(pk.species, pk.level, false);
    const Chain chain2 = originChain(pk.species, pk.level, true);
    return search(pk, chain1, chain2, gen2Possible(pk, chain1), extra);
}

Result checkGen2(const Entity2& e) {
    Result res;
    if (e.species == 0 || e.species > kMaxSpecies2) {
        res.problems.push_back({Area::Encounter, "Species does not exist in Gen II"});
        return res;
    }
    if (e.level < 1 || e.level > 100) {
        res.problems.push_back({Area::Encounter, "Level is outside 1-100"});
        return res;
    }
    if (!movesWellFormed(e.moves, kMaxMove2, "Gen II", res))
        return res;

    Subject pk;
    pk.format = 2;
    pk.species = e.species;
    pk.level = e.level;
    pk.moves = e.moves;
    pk.dvs = e.dvs;
    pk.tid = e.tid;
    pk.tradeTrainer = e.tradeTrainer;
    pk.otName = e.otName;
    pk.nickname = e.nickname;
    pk.language = e.language;
    pk.heldItem = e.heldItem;
    pk.friendship = e.friendship;
    pk.exp = e.exp;
    pk.egg = e.isEgg;
    pk.caught = e.caughtData != 0;
    pk.metTime = static_cast<uint8_t>((e.caughtData >> 14) & 3);
    pk.metLevel = static_cast<uint8_t>((e.caughtData >> 8) & 0x3F);
    pk.otGender = static_cast<uint8_t>((e.caughtData >> 7) & 1);
    pk.metLocation = static_cast<uint8_t>(e.caughtData & 0x7F);

    // Problems that do not depend on the origin.
    std::vector<Problem> extra;
    if (pk.egg && pk.heldItem != 0)
        extra.push_back({Area::HeldItem, "An egg cannot hold an item"});
    else if (!isHoldableGen2(pk.heldItem))
        extra.push_back({Area::HeldItem, "Held item " + std::to_string(pk.heldItem) +
                                             " cannot be held in Gold/Silver/Crystal"});
    if (pk.egg && pk.level != kEggLevel)
        extra.push_back({Area::Egg, "An unhatched egg is always level 5"});
    if (pk.metLocation != 0 && !pk.egg && pk.level < pk.metLevel)
        extra.push_back({Area::MetData, "Level is below the Crystal met level"});
    if (const std::string pp = ppProblem(2, e.moves, e.pp, e.ppUps, pk.egg); !pp.empty())
        extra.push_back({Area::Moves, pp});

    // EncounterOrigin.GetMinMaxGB: eggs are level 5, Crystal-met Pokemon start at their met level.
    const uint8_t levelMin = pk.egg ? kEggLevel : (pk.caught ? pk.metLevel : kMinWildLevel);
    Chain chain;
    if (pk.egg) {
        chain.stage[0] = {pk.species, pk.level, pk.level, 0, 0};
        chain.count = 1;
    } else {
        chain = originChain(pk.species, pk.level, true, levelMin);
    }
    // GBRestrictions.CanOriginateGen1: no Korean games, no eggs, no Crystal met data, and a
    // species that could have been in Gen I.
    const bool fromGen1 = !pk.egg && !pk.caught && canVisitGen1(pk.species);
    return search(pk, chain, chain, fromGen1, extra);
}

} // namespace Legality::GB
