/**
 * GBLegality.cpp - Gen I/II origin search. See GBLegality.h.
 *
 * Structure mirrors PKHeX.Core so behaviour can be compared rule by rule:
 *   origin chain   EncounterOrigin.GetOriginChain12 + EvolutionGroup1/2.Devolve
 *   encounters     EncounterEnumerator1/2, Encounter{Slot,Static,Trade,Gift,Egg}{1,2}.IsMatchExact
 *   move history   EvolutionChain.GetEvolutionChainsAllGens (forward Evolve ranges)
 *   move sources   LearnGroup1/LearnGroup2, LearnSource1RB/1YW/2GS/2C/2Stadium
 *   catch rate     GBRestrictions + MiscG1Verifier
 */

#include "Legality/GBLegality.h"
#include "Legality/GBLegalityData.h"

#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"

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

constexpr uint8_t kSpeciesPikachu = 25, kSpeciesRaichu = 26, kSpeciesKadabra = 64;
constexpr uint8_t kSpeciesMagnemite = 81, kSpeciesMagneton = 82, kSpeciesDitto = 132;
constexpr uint8_t kSpeciesPorygon = 137, kSpeciesDragonite = 149, kSpeciesPsyduck = 54;
constexpr uint8_t kMoveSurf = 57, kMoveDizzyPunch = 146;
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
Chain originChain(uint8_t species, uint8_t level, bool gen2) {
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
        if (levelUp) {  // EvolutionMethod.Check with levelMin = 2
            if (curMax < link->level || (link->level == 0 && curMax < 2) || curMax < kMinWildLevel + 1)
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
    uint8_t levelMin = kMinWildLevel;
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
    bool starterPikachu = false;
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
    bool anyGen2 = false;
    bool allValid() const {
        for (const MoveResult& r : result)
            if (!r.valid())
                return false;
        return true;
    }
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

/// LearnGroup1.Check (+ LearnGroup2.Check when the Pokemon can have visited Gen II).
MoveCheck checkMoves(const Entity1& pk, const Candidate& enc, const Chain& hist1, const Chain& hist2,
                     bool gen2Visit) {
    MoveCheck mc;
    const std::array<uint8_t, 4>& current = pk.moves;
    for (int i = 0; i < 4; ++i)
        if (current[i] == 0)
            mc.result[i] = {Source::Initial, 0, 0, 0};  // empty slots are judged separately

    // --- Gen I: encounter moves, then each history stage (Yellow first, then R/B).
    std::array<uint8_t, 4> initial1{};
    if (enc.generation == 1) {
        // LearnGroup1.GetEncounterMoves: Yellow data for Yellow and all-version encounters.
        initial1 = enc.hasFixedMoves ? enc.fixedMoves
                                     : gen1EncounterMoves(enc.species, enc.level, hasYellow(enc.versions));
        markInitial(mc, current, initial1, 1);
    }
    for (int stage = 0; stage < hist1.count; ++stage) {
        const Stage& s = hist1.stage[stage];
        if (s.species > kMaxSpecies1)
            continue;
        for (int i = 3; i >= 0; --i) {
            MoveResult& entry = mc.result[i];
            if (current[i] == 0 || (entry.valid() && entry.generation != 1 && entry.generation != 2))
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

    // --- Gen II, when it is reachable (encounter there, or Time Capsule trip possible).
    if (gen2Visit) {
        bool all = true;
        for (const MoveResult& r : mc.result)
            all &= r.valid();
        if (!all) {
            std::array<uint8_t, 4> initial2{};
            if (enc.generation == 2) {
                const bool crystal = (enc.versions & Crystal) != 0;
                initial2 = enc.hasFixedMoves ? enc.fixedMoves
                                             : (enc.egg ? std::array<uint8_t, 4>{}
                                                        : gen2EncounterMoves(enc.species, enc.level, crystal));
                if (enc.egg && !enc.hasFixedMoves) {
                    // Learnset.GetBaseEggMoves: last four moves known at the hatch level.
                    initial2 = gen2EncounterMoves(enc.species, kEggLevel, crystal);
                }
                markInitial(mc, current, initial2, 2);
            }
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
            if (enc.bred) {  // LearnGroup2: only EncounterEgg2, never gift or event eggs
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
    }
    for (const MoveResult& r : mc.result)
        mc.anyGen2 |= r.valid() && r.generation == 2;

    if (enc.generation == 1 && !gen2Visit) {
        // LearnGroup1.FlagFishyMoveSlots: without a Gen II move deleter, a move slot can
        // only be empty if the Pokemon never knew that many moves.
        int empty = 0;
        for (uint8_t m : current)
            empty += m == 0;
        if (empty != 0) {
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
                if (j == i || current[j] == 0 || !mc.result[j].valid())
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
std::string catchRateProblem(const Entity1& pk, const Candidate& enc, bool anyGen2Move) {
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
bool gen2Possible(const Entity1& pk, const Chain& chain1) {
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

std::string typeProblem(const Entity1& pk) {
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

void addIf(std::vector<Candidate>& out, const Candidate& c) { out.push_back(c); }

std::vector<Candidate> gen1Candidates(const Entity1& pk, const Chain& chain) {
    std::vector<Candidate> out;
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
        addIf(out, c);
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
        c.starterPikachu = s.version == Yellow && s.species == kSpeciesPikachu && s.level == 5;
        const uint8_t rate = pk.catchRate;
        const bool rateOk = isHeldItemRate(rate) ||
                            (c.starterPikachu ? rate == kRateLightBall
                                              : rateMatchesEncounter(s.species, s.version, rate));
        if (!rateOk) {
            c.partial = true;
            c.partialReason = "Catch rate does not match the " + speciesName(s.species) + " encounter";
        }
        addIf(out, c);
    }

    // Wild slots: lowest level per (species, version) is all the search needs.
    for (std::size_t i = 0; i < kWildSlotCount; ++i) {
        const WildSlot& w = kWildSlots[i];
        if (!isGen1Version(w.version))
            continue;
        const int idx = chain.indexOf(w.species);
        if (idx < 0 || w.minLevel > chain.stage[idx].max)
            continue;
        if (w.version == BlueJP && !maybeJapanese)
            continue;
        const uint8_t expect = personal1(w.species, w.version == Yellow).catchRate;
        if (pk.catchRate != expect && !isHeldItemRate(pk.catchRate))
            continue;
        bool seen = false;
        for (const Candidate& o : out)
            seen |= o.kind == EncounterKind::Wild && o.species == w.species && o.versions == w.version &&
                    o.level <= w.minLevel;
        if (seen)
            continue;
        Candidate c;
        c.kind = EncounterKind::Wild;
        c.generation = 1;
        c.species = w.species;
        c.level = w.minLevel;
        c.versions = w.version;
        c.slotType = w.type;
        addIf(out, c);
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
        addIf(out, c);
    }
    return out;
}

std::vector<Candidate> gen2Candidates(const Entity1& pk, const Chain& chain) {
    std::vector<Candidate> out;
    const bool maybeJapanese = pk.language != Language::International;

    for (std::size_t i = 0; i < kTrades2Count; ++i) {
        const Trade2& t = kTrades2[i];
        const int idx = chain.indexOf(t.species);
        if (idx < 0 || chain.stage[idx].max < t.level)
            continue;
        if (t.dvs[0] != 255 && (!fixedDVsMatch(t.dvs, pk.dvs) || pk.tid != t.tid))
            continue;
        Candidate c;
        c.kind = EncounterKind::Trade;
        c.generation = 2;
        c.species = t.species;
        c.level = t.level;
        c.versions = Gold | Silver | Crystal;
        const bool nickOk = nameMatches(kTradeNames2, kTradeNames2Count, t.nameIndex, pk.nickname, pk.language);
        const bool otOk = nameMatches(kTradeNames2, kTradeNames2Count, static_cast<uint8_t>(t.nameIndex + 11),
                                      pk.otName, pk.language);
        if (!nickOk || !otOk) {
            c.partial = true;
            c.partialReason = "In-game trade " + speciesName(t.species) +
                              " must keep its fixed trainer name and nickname";
        }
        addIf(out, c);
    }

    // Eggs: the earliest form must be able to hatch, at level 5.
    {
        const Stage& lowest = chain.stage[chain.count - 1];
        const uint8_t baby = lowest.species;
        const bool hatchable = baby != kSpeciesDitto && baby != 144 && baby != 145 && baby != 146 &&
                               baby != 150 && baby != 151 && baby != 201 && !(baby >= 243 && baby <= 251);
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
                addIf(out, c);
            }
        }
    }

    for (std::size_t i = 0; i < kStatics2Count; ++i) {
        const Static2& s = kStatics2[i];
        const int idx = chain.indexOf(s.species);
        if (idx < 0 || chain.stage[idx].max < s.level)
            continue;
        if (s.shinyAlways && !isShinyDVs(pk.dvs))
            continue;
        if (s.shinyNever && isShinyDVs(pk.dvs))
            continue;
        if (!fixedDVsMatch(s.dvs, pk.dvs))
            continue;
        const bool oddEgg = s.egg && s.moves[0] != 0;
        if (oddEgg && std::find(pk.moves.begin(), pk.moves.end(), kMoveDizzyPunch) == pk.moves.end())
            continue;
        Candidate c;
        c.kind = s.egg ? EncounterKind::Egg : EncounterKind::Static;
        c.generation = 2;
        c.species = s.species;
        c.level = s.level;
        c.versions = s.version;
        if (s.moves[0] != 0) {
            c.hasFixedMoves = true;
            c.fixedMoves = {s.moves[0], s.moves[1], s.moves[2], s.moves[3]};
        }
        addIf(out, c);
    }

    for (std::size_t i = 0; i < kWildSlotCount; ++i) {
        const WildSlot& w = kWildSlots[i];
        if (isGen1Version(w.version))
            continue;
        const int idx = chain.indexOf(w.species);
        if (idx < 0 || w.minLevel > chain.stage[idx].max)
            continue;
        bool seen = false;
        for (const Candidate& o : out)
            seen |= o.kind == EncounterKind::Wild && o.species == w.species && o.versions == w.version &&
                    o.level <= w.minLevel;
        if (seen)
            continue;
        Candidate c;
        c.kind = EncounterKind::Wild;
        c.generation = 2;
        c.species = w.species;
        c.level = w.minLevel;
        c.versions = w.version;
        c.slotType = w.type;
        addIf(out, c);
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
        if (!g.egg) {
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
        c.versions = Gold | Silver | Crystal;
        c.egg = g.egg != 0;
        c.hasFixedMoves = true;
        c.fixedMoves = {g.moves[0], g.moves[1], g.moves[2], g.moves[3]};
        addIf(out, c);
    }
    return out;
}

// ---------------------------------------------------------------- evaluation

struct Evaluation {
    Candidate enc;
    std::vector<Problem> problems;
    bool timeCapsule = false;
};

Evaluation evaluate(const Entity1& pk, const Candidate& enc, const Chain& chain1, const Chain& chain2,
                    bool tradebackPossible) {
    Evaluation ev;
    ev.enc = enc;
    const Chain& origin = enc.generation == 1 ? chain1 : chain2;
    const int idx = origin.indexOf(enc.species);

    Chain hist;
    if (idx < 0 || !historyFrom(origin, idx, enc.level, pk.level, hist)) {
        ev.problems.push_back({Area::Encounter, speciesName(enc.species) + " met at level " +
                                                    std::to_string(enc.level) + " cannot become " +
                                                    speciesName(pk.species) + " by level " +
                                                    std::to_string(pk.level)});
        return ev;
    }

    // Gen I history keeps only the forms that exist in Gen I (SetHistory with RB).
    Chain hist1;
    for (int i = 0; i < hist.count; ++i) {
        if (hist.stage[i].species > kMaxSpecies1)
            break;
        hist1.stage[hist1.count++] = hist.stage[i];
    }

    const bool gen2Visit = enc.generation == 2 || (tradebackPossible && isHeldItemRate(pk.catchRate));
    const MoveCheck mc = checkMoves(pk, enc, hist1, hist, gen2Visit);
    ev.timeCapsule = enc.generation == 2 || mc.anyGen2;

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
    if (mc.anyGen2 && !isHeldItemRate(pk.catchRate))
        ev.problems.push_back({Area::CatchRate, "A move only learnable in Gold/Silver/Crystal needs a Time "
                                                "Capsule trip, but the catch rate byte is not a Gen II item"});
    const std::string rate = catchRateProblem(pk, enc, mc.anyGen2);
    if (!rate.empty())
        ev.problems.push_back({Area::CatchRate, rate});
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

Result checkGen1(const Entity1& pk) {
    Result res;
    if (pk.species == 0 || pk.species > kMaxSpecies1) {
        res.problems.push_back({Area::Encounter, "Species does not exist in Gen I"});
        return res;
    }
    if (pk.level < 1 || pk.level > 100) {
        res.problems.push_back({Area::Encounter, "Level is outside 1-100"});
        return res;
    }
    for (uint8_t m : pk.moves) {
        if (m > kMaxMove1) {
            res.problems.push_back({Area::Moves, "Move id " + std::to_string(m) + " does not exist in Gen I"});
            return res;
        }
    }
    if (pk.moves[0] == 0) {
        res.problems.push_back({Area::Moves, "A Pokemon always knows at least one move"});
        return res;
    }
    for (int i = 1; i < 4; ++i) {
        if (pk.moves[i] != 0 && pk.moves[i - 1] == 0) {
            res.problems.push_back({Area::Moves, "Moves are not packed: an empty slot comes before a move"});
            return res;
        }
    }

    const Chain chain1 = originChain(pk.species, pk.level, false);
    const Chain chain2 = originChain(pk.species, pk.level, true);
    const bool tradeback = gen2Possible(pk, chain1);

    std::vector<Candidate> candidates = gen1Candidates(pk, chain1);
    if (tradeback) {
        std::vector<Candidate> more = gen2Candidates(pk, chain2);
        candidates.insert(candidates.end(), more.begin(), more.end());
    }

    const std::string types = typeProblem(pk);

    bool haveBest = false;
    Evaluation best;
    for (const Candidate& c : candidates) {
        Evaluation ev = evaluate(pk, c, chain1, chain2, tradeback);
        if (ev.problems.empty()) {
            res.status = types.empty() ? Status::Legal : Status::Illegal;
            res.origin = {c.kind, c.generation, c.species, c.level, c.versions, c.slotType};
            res.timeCapsule = ev.timeCapsule;
            if (!types.empty())
                res.problems.push_back({Area::Types, types});
            return res;
        }
        if (!haveBest || ev.problems.size() < best.problems.size()) {
            best = std::move(ev);
            haveBest = true;
        }
    }

    res.status = Status::Illegal;
    if (!haveBest) {
        res.problems.push_back({Area::Encounter,
                                "No " + std::string(tradeback ? "Red/Blue/Yellow or Gold/Silver/Crystal"
                                                              : "Red/Blue/Yellow") +
                                    " encounter can produce " + speciesName(pk.species) + " at level " +
                                    std::to_string(pk.level)});
    } else {
        const Candidate& c = best.enc;
        res.origin = {c.kind, c.generation, c.species, c.level, c.versions, c.slotType};
        res.timeCapsule = best.timeCapsule;
        res.problems = std::move(best.problems);
    }
    if (!types.empty())
        res.problems.push_back({Area::Types, types});
    return res;
}

} // namespace Legality::GB
