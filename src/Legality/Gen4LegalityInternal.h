/**
 * Gen4LegalityInternal.h - shared internals of the Gen IV analyzer (not a public API).
 *
 * The analyzer is split by topic: Gen4LegalityRng.cpp (PID/IV correlation and wild
 * lead/frame checks), Gen4LegalityMoves.cpp (evolution chains and move sources),
 * Gen4LegalityEncounters.cpp (encounter search), Gen4LegalityPalPark.cpp (the Generation III
 * encounter search for Pal Park records), Gen4LegalityText.cpp (names and the word filter) and
 * Gen4Legality.cpp (field verifiers and the report).
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Legality/Gen4Legality.h"
#include "Legality/Gen4LegalityData.h"

namespace Legality::Gen4 {

namespace D = ::Legality::Gen4Data;

// ---------------------------------------------------------------- species and versions
enum : uint16_t {
    kBulbasaur = 1, kCharmander = 4, kPikachu = 25, kNidoranF = 29, kNidoranM = 32, kClefairy = 35,
    kMarill = 183, kAzumarill = 184, kPichu = 172, kUnown = 201, kWobbuffet = 202, kWurmple = 265,
    kSilcoon = 266, kCascoon = 268, kNincada = 290, kNinjask = 291, kShedinja = 292,
    kAzurill = 298, kCastform = 351, kDeoxys = 386, kTurtwig = 387, kBurmy = 412, kWormadam = 413,
    kMothim = 414, kCombee = 415, kVespiquen = 416, kCherrim = 421, kShellos = 422, kGastrodon = 423,
    kKirlia = 281, kGallade = 475, kSnorunt = 361, kFroslass = 478, kRotom = 479, kGiratina = 487,
    kManaphy = 490, kShaymin = 492, kArceus = 493, kSmeargle = 235, kEevee = 133, kSpinda = 327,
    kMaxSpecies4 = 493,
};
inline bool isGen4Version(uint8_t v) noexcept { return v == D::kHG || v == D::kSS || v == D::kD || v == D::kP || v == D::kPt; }
inline bool isGen3Version(uint8_t v) noexcept { return (v >= 1 && v <= 5) || v == 15; }
inline bool isHGSS(uint8_t v) noexcept { return v == D::kHG || v == D::kSS; }

// ---------------------------------------------------------------- personal tables
enum class Table : uint8_t { DP, Pt, HGSS };
struct Personal {
    const uint8_t* p = nullptr;
    uint8_t baseStat(int i) const noexcept { return p[i]; }
    uint8_t type1() const noexcept { return p[6]; }
    uint8_t type2() const noexcept { return p[7]; }
    uint16_t evYield() const noexcept { return static_cast<uint16_t>(p[0xA] | (p[0xB] << 8)); }
    uint16_t item1() const noexcept { return static_cast<uint16_t>(p[0xC] | (p[0xD] << 8)); }
    uint16_t item2() const noexcept { return static_cast<uint16_t>(p[0xE] | (p[0xF] << 8)); }
    uint8_t gender() const noexcept { return p[0x10]; }
    uint8_t hatchCycles() const noexcept { return p[0x11]; }
    uint8_t baseFriendship() const noexcept { return p[0x12]; }
    uint8_t expGrowth() const noexcept { return p[0x13]; }
    uint8_t eggGroup1() const noexcept { return p[0x14]; }
    uint8_t eggGroup2() const noexcept { return p[0x15]; }
    uint8_t ability1() const noexcept { return p[0x16]; }
    uint8_t ability2() const noexcept { return p[0x17]; }
    uint8_t color() const noexcept { return p[0x19]; }
    bool tmhm(int bit) const noexcept { return bit >= 0 && bit < 100 && ((p[0x1C + (bit >> 3)] >> (bit & 7)) & 1) != 0; }
    uint8_t formCount() const noexcept { return p[0x29]; }
    uint16_t formStatsIndex() const noexcept { return static_cast<uint16_t>(p[0x2A] | (p[0x2B] << 8)); }
    bool hasForm(uint8_t form) const noexcept { return form != 0 && formStatsIndex() > 0 && form < formCount(); }
    uint16_t formIndex(uint16_t species, uint8_t form) const noexcept {
        return hasForm(form) ? static_cast<uint16_t>(formStatsIndex() + form - 1) : species;
    }
};
std::size_t tableCount(Table t) noexcept;
Personal personalAt(Table t, std::size_t index) noexcept;              // raw index (form index)
uint16_t formIndex(Table t, uint16_t species, uint8_t form) noexcept; // PersonalTable4.GetFormIndex
Personal personal(Table t, uint16_t species, uint8_t form) noexcept;  // GetFormEntry
bool isPresentInGame(Table t, uint16_t species, uint8_t form) noexcept;
inline Table tableForVersion(uint8_t v) noexcept {
    return isHGSS(v) ? Table::HGSS : (v == D::kPt ? Table::Pt : Table::DP);
}
/// GameData.GetLearnSource(version) for Gen IV versions, including the grouped values.
inline Table learnTable(uint8_t v) noexcept {
    if (v == D::kD || v == D::kP || v == D::kVerDP) return Table::DP;
    if (v == D::kPt || v == D::kVerDPPt) return Table::Pt;
    return Table::HGSS;
}
/// EntityGender.GetFromPIDAndRatio
inline uint8_t genderFromPid(uint32_t pid, uint8_t ratio) noexcept {
    if (ratio == 255) return 2;
    if (ratio == 254) return 1;
    if (ratio == 0) return 0;
    return (pid & 0xFF) < ratio ? 1 : 0;
}
uint8_t levelFromExp(uint32_t exp, uint8_t growth) noexcept;
uint32_t expForLevel(uint8_t level, uint8_t growth) noexcept;

// ---------------------------------------------------------------- RNG
namespace lcrng {
constexpr uint32_t next(uint32_t s) noexcept { return s * 0x41C64E6Du + 0x6073u; }
constexpr uint32_t prev(uint32_t s) noexcept { return s * 0xEEB9EB65u + 0x0A3561A1u; }
constexpr uint32_t advance(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = next(s); return s; }
constexpr uint32_t reverse(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = prev(s); return s; }
inline uint32_t next16(uint32_t& s) noexcept { s = next(s); return s >> 16; }
inline uint32_t next15(uint32_t& s) noexcept { s = next(s); return (s >> 16) & 0x7FFF; }
inline uint32_t prev16(uint32_t& s) noexcept { s = prev(s); return s >> 16; }
int getSeeds(uint32_t* out, uint32_t first, uint32_t second) noexcept;      // LCRNGReversal.GetSeeds
int getSeedsIVs(uint32_t* out, uint32_t first, uint32_t second) noexcept;   // LCRNGReversal.GetSeedsIVs
int getSeedsSkip(uint32_t* out, uint32_t first, uint32_t third) noexcept;   // LCRNGReversalSkip.GetSeeds
int getSeedsIVs(uint32_t* out, const std::array<uint8_t, 6>& ivs) noexcept; // hp atk def spe spa spd order input
} // namespace lcrng
inline constexpr int kSeedBuffer = 32;
/// ClassicEraRNG.GetSequentialPID
inline uint32_t sequentialPid(uint32_t seed) noexcept {
    const uint32_t a = lcrng::next16(seed);
    const uint32_t b = lcrng::next16(seed);
    return (b << 16) | a;
}

enum class PidType : uint8_t {
    None, Method_1, Method_2, Method_3, Method_4,
    Method_1_Unown, Method_2_Unown, Method_3_Unown, Method_4_Unown, Method_1_Roamer,
    CXD, CXDAnti, G4MGAntiShiny,
    BACD, BACD_A, BACD_AX, BACD_ES, BACD_EA, BACD_EAX, BACD_S,
    ChainShiny, CuteCharm, G5MGShiny, Pokewalker,
    BACD_R, Channel, // only ever produced as a PIDIV.Mutated revision (BACD_R) or by GetChannelMatch (R/S)
};
struct PidIv { PidType type = PidType::None; uint32_t seed = 0; };
/// MethodFinder.Analyze for a PK4 (Colosseum starters need Colosseum/XD origin and never apply here).
PidIv analyzePidIv(const Pk4& pk) noexcept;
/// MethodJ.GetReversalWindow (shared by Method H).
int methodJReversalWindow(uint32_t seed, uint8_t nature) noexcept;
namespace xdrng {
constexpr uint32_t next(uint32_t s) noexcept { return s * 0x343FDu + 0x269EC3u; }
constexpr uint32_t prev(uint32_t s) noexcept { return s * 0xB9B33155u + 0xA170F641u; }
inline uint32_t prev16(uint32_t& s) noexcept { s = prev(s); return s >> 16; }
} // namespace xdrng
/// MethodFinder.GetLCRNGMethod1Match
bool method1Seed(const Pk4& pk, uint32_t& seed) noexcept;
/// ShinyUtil.GetIsShiny3 on (TID ^ SID, PID)
inline bool isShinyXor(uint32_t idXor, uint32_t pid) noexcept {
    const uint32_t x = pid ^ idXor;
    return ((x ^ (x >> 16)) & 0xFFFF) < 8;
}
/// CuteCharm4.IsCuteCharm(pk, pid) and CuteCharm4.IsValid(enc, pk)
bool isCuteCharm(const Pk4& pk, uint32_t pid) noexcept;
bool isCuteCharmValid(uint16_t encSpecies, const Pk4& pk) noexcept;
/// PokewalkerRNG.GetPID
uint32_t pokewalkerPid(uint16_t tid, uint16_t sid, uint32_t nature, uint8_t gender, uint8_t ratio) noexcept;
/// PGT.IsG4ManaphyPIDValid-style helpers live with the gift code.

// ---------------------------------------------------------------- wild slots
struct SlotRef {
    const D::Area4* area = nullptr;
    const D::Slot4* slot = nullptr;
    uint8_t type() const noexcept { return area->type; }
    uint8_t location() const noexcept { return area->location; }
    uint8_t version() const noexcept { return area->version; }
    uint8_t levelMin() const noexcept { return slot->levelMin; }
    uint8_t levelMax() const noexcept { return slot->levelMax; }
    bool isFixedLevel() const noexcept { return slot->levelMin == slot->levelMax; }
    uint8_t pressureLevel() const noexcept;
    bool canUseRadar() const noexcept;
    bool isCoronetFeebasArea() const noexcept { return area->rate == 255; }
    bool isRerollMinimum31() const noexcept { return area->type == kBugContest || area->location == 202; }
    enum : uint8_t {
        kGrass = 0, kSurf = 1, kOldRod = 2, kGoodRod = 3, kSuperRod = 4, kRockSmash = 5, kHeadbutt = 6,
        kHeadbuttSpecial = 7, kBugContest = 8, kHoneyTree = 9, kSafariGrass = 10, kSafariSurf = 11,
        kSafariOldRod = 12, kSafariGoodRod = 13, kSafariSuperRod = 14,
    };
};
enum class Lead : uint8_t {
    Invalid = 0, StaticMagnetFail, CuteCharmFail, SynchronizeFail, PressureHustleSpiritFail,
    IntimidateKeenEyeFail, Illuminate, SuctionCups, PressureHustleSpirit, MagnetPull, Static,
    CuteCharm, Synchronize, CuteCharmRadar, SynchronizeRadar, Radar, None = 255,
};
struct LeadSeed { uint32_t seed = 0; Lead lead = Lead::Invalid; bool valid() const noexcept { return lead != Lead::Invalid; } };
/// LeadFinder.GetLeadInfo4 with the Gen IV level constraint (met level).
LeadSeed leadInfo4(const Pk4& pk, const SlotRef& slot, const PidIv& pidiv, uint8_t levelMin, uint8_t levelMax) noexcept;
bool isMunchlaxTree(uint32_t id32, uint8_t location) noexcept;
bool isUnownFormValid(const Pk4& pk, uint8_t form, bool ruinsOfAlph) noexcept;
bool isWurmpleEvoValid(uint32_t pid, uint16_t species) noexcept;


// ---------------------------------------------------------------- evolutions and moves
/// PKHeX EvoCriteria; method kSentinel marks SentinelNotReached.
struct Evo {
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t levelUpRequired = 0;
    uint8_t levelMax = 0;
    uint8_t levelMin = 0;
    uint8_t method = 0;
    bool insideLevelRange(uint8_t level) const noexcept { return levelMin <= level && level <= levelMax; }
};
inline constexpr uint8_t kSentinel = 255;

enum class EncKind : uint8_t {
    Invalid, Slot, Static, Walker, Trade, Ranch, Pcd, RangerManaphy, Egg,
    // Generation III templates (Pal Park origin): EncounterSlot3(+Swarm), EncounterStatic3, EncounterTrade3,
    // EncounterGift3, EncounterGift3NY, EncounterGift3JPN, EncounterGift3Colo, EncounterEgg3.
    Slot3, Static3, Trade3, Gift3, GiftNY3, GiftJP3, GiftColo3, Egg3,
    // A Colosseum/XD (version 15) encounter as the Gen III GameCube judge described it
    // (Gen4Legality.h CxdOriginJudgement): shadow, static, trade, starter, gift or Poke Spot.
    Cxd3,
};
inline bool isGen3Kind(EncKind k) noexcept { return k >= EncKind::Slot3; }
/// The facts of a matched encounter template that the evolution and move code reads.
struct EncInfo {
    EncKind kind = EncKind::Invalid;
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t levelMin = 0;
    uint8_t levelMax = 0;
    uint8_t version = 0;                 // template version (egg/learn source)
    std::array<uint16_t, 4> moves{};     // IMoveset moves (all zero when none)
    bool hasMoves() const noexcept { return moves[0] != 0; }
    bool isEggTemplate = false;          // IEncounterable.IsEgg
};

/// EvolutionChain.GetEvolutionChainsAllGens for a format-4 entity: the Gen IV history.
std::vector<Evo> evolutionHistory(const Pk4& pk, const EncInfo& enc, uint8_t currentLevel);
bool hasVisited(const std::vector<Evo>& history, uint16_t species) noexcept;
/// EvolutionHistory.Gen3 / Gen4 of a format-4 entity whose encounter is a Generation III template.
struct History34 { std::vector<Evo> gen3, gen4; };
History34 evolutionHistory3(const Pk4& pk, const EncInfo& enc, uint8_t currentLevel);
/// EncounterOrigin.GetOriginChain(pk, 3, Gen3) for a PK4.
std::vector<Evo> originChain3(const Pk4& pk, uint8_t currentLevel);
/// EvolutionTree.Evolves3.GetBaseSpeciesForm
std::pair<uint16_t, uint8_t> baseSpeciesForm3(uint16_t species, uint8_t form) noexcept;
/// PersonalTable.E.IsPresentInGame
bool isPresentInGameE(uint16_t species, uint8_t form) noexcept;

enum class LearnMethod : uint8_t {
    None, Duplicate, EmptyInvalid, Unobtainable, UnobtainableExpect, Empty, Initial, LevelUp, TMHM,
    Tutor, Sketch, HOME, Encounter, Evolution, Shared, ShedinjaEvo, Relearn, EggMove, InheritLevelUp, SpecialEgg,
};
struct MoveResult {
    LearnMethod method = LearnMethod::None;
    uint16_t expect = 0;
    bool valid() const noexcept { return method >= LearnMethod::Empty; }
    bool parsed() const noexcept { return method != LearnMethod::None; }
};
using MoveResults = std::array<MoveResult, 4>;
/// LearnVerifier.Verify for a PK4 (Gen IV context).
MoveResults verifyMoves(const Pk4& pk, const EncInfo& enc, const std::vector<Evo>& history);
/// EvolutionRestrictions.IsValidEvolutionWithMove
bool isValidEvolutionWithMove(const Pk4& pk, const EncInfo& enc, const std::vector<Evo>& history, const MoveResults& moves);
/// LearnVerifier.Verify / IsValidEvolutionWithMove for a Generation III encounter (LearnGroup4 then LearnGroup3).
MoveResults verifyMoves3(const Pk4& pk, const EncInfo& enc, const History34& history);
bool isValidEvolutionWithMove3(const Pk4& pk, const EncInfo& enc, const History34& history, const MoveResults& moves);
/// MoveBreed4.Validate (exposed for tests).
bool moveBreed4Validate(uint16_t species, uint8_t version, std::span<const uint16_t> moves, std::span<uint8_t> origins);
/// Level-up learnset of a species/form for a game table.
struct Learnset { const D::LevelMove* begin = nullptr; const D::LevelMove* end = nullptr; };
Learnset learnset(Table t, uint16_t species, uint8_t form) noexcept;
/// Breeding.GetCanInheritMoves etc. from kBreed flags.
bool canInheritMoves(uint16_t species) noexcept;


// ---------------------------------------------------------------- encounters
/// One yielded encounter template (EncounterGenerator4 output).
struct Candidate {
    EncKind kind = EncKind::Invalid;
    int index = -1;          // kStatics / kWalkers / kTrades / kRanch / kPcd / kSlots index
    int area = -1;           // kAreas index (slots)
    uint16_t species = 0;    // egg species (eggs), template species otherwise
    uint8_t version = 0;     // egg version
    Lead lead = Lead::Invalid;
};
/// A decoded PCD gift (PGT header plus its PK4).
struct Gift {
    uint16_t cardType = 0;
    Pk4 pk;
    bool isManaphyEgg() const noexcept { return cardType == 7; }
    bool isEgg() const noexcept { return cardType == 2 || cardType == 7; }
    bool isEntity() const noexcept { return cardType == 1 || cardType == 2 || cardType == 7 || cardType == 13; }
    uint16_t species() const noexcept { return isManaphyEgg() ? static_cast<uint16_t>(kManaphy) : pk.species; }
    uint8_t level() const noexcept { return isManaphyEgg() ? 1 : isEntity() ? pk.metLevel : 0; }
    uint8_t ball() const noexcept { return isManaphyEgg() ? 4 : isEntity() ? pk.ball() : 0; }
    uint8_t version() const noexcept { return isManaphyEgg() ? D::kVerGen4 : pk.version; }
    bool fateful() const noexcept { return isManaphyEgg() || pk.fateful; }
    uint16_t location() const noexcept { return static_cast<uint16_t>(isEgg() ? 0 : pk.eggLocation() + 3000); }
    uint16_t eggLocation() const noexcept { return static_cast<uint16_t>(isEgg() ? pk.eggLocation() + 3000 : 0); }
};
Gift pcdGift(int index) noexcept;
Gift rangerManaphy() noexcept;
/// Template view used by the verifiers.
struct Template {
    EncKind kind = EncKind::Invalid;
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t levelMin = 0, levelMax = 0;
    uint8_t version = 0;
    uint16_t location = 0, eggLocation = 0;
    bool isEgg = false;           // IEncounterable.IsEgg
    uint8_t fixedBall = 0;        // Ball (0 none)
    int8_t ability = 0;           // AbilityPermission
    uint8_t shiny = 0;            // Shiny: 0 Random, 1 Never, 2 Always, 5 FixedValue
    bool fateful = false;
    uint8_t gender = 0xFF;        // fixed gender or 0xFF
    uint8_t nature = 25;          // fixed nature or 25
    uint16_t heldItem = 0;
    std::array<uint16_t, 4> moves{};
    const D::Area4* area = nullptr;
    const D::Slot4* slot = nullptr;
    const D::Area3* area3 = nullptr;
    const D::Slot3* slot3 = nullptr;
    int index = -1;
    Gift gift{};                  // PCD / Ranger Manaphy
    bool fixedTrainer = false;    // Cxd3: IFixedTrainer (the judge checked the OT)
    bool fixedNickname = false;   // Cxd3: IFixedNickname (the judge checked the nickname)
    bool shadow = false;          // Cxd3: IShadow3 (National Ribbon required once purified)
    const char* name = nullptr;   // Cxd3: template name from the judge
};
Template makeTemplate(const Candidate& c, const Pk4& pk);
EncInfo encInfo(const Template& t);
inline constexpr uint8_t kShinyRandom = 0, kShinyNever = 1, kShinyAlways = 2, kShinyAlwaysStar = 3, kShinyAlwaysSquare = 4,
                         kShinyFixed = 5; // PKHeX Shiny enum values

struct Search {
    PidIv pidiv;
    std::vector<Candidate> list;
    uint8_t manualFlag = 0; // 1 InvalidPIDIV, 2 InvalidFrame: set when the last element is the deferred last resort
};
/// EncounterOrigin.GetOriginChain(pk, 4, Gen4)
std::vector<Evo> originChain(const Pk4& pk, uint8_t currentLevel);
/// EncounterGenerator4.GetEncounters
Search searchEncounters(const Pk4& pk, uint8_t currentLevel);
/// Rating of a template that implements IEncounterMatch (-1 when it does not).
int matchRating(const Template& t, const Pk4& pk);
/// EncounterGenerator3.GetEncounters for a PK4 of Generation III origin (versions 1-5).
Search searchEncounters3(const Pk4& pk, uint8_t currentLevel);
Template makeTemplate3(const Candidate& c, const Pk4& pk);
int matchRating3(const Template& t, const Pk4& pk);
/// IFixedTrainer.IsTrainerMatch / IFixedNickname.IsNicknameMatch of a Gen III template (Trade3, GiftColo3).
bool isTrainerMatch3(const Template& t, const Pk4& pk, std::u16string_view trainer, int language);
bool isNicknameMatch3(const Template& t, const Pk4& pk, std::u16string_view nickname, int language);
/// IFixedTrainer.IsFixedTrainer of a Gen III template.
bool isFixedTrainer3(const Template& t) noexcept;
/// EntityPID.GetUnownForm3
inline uint8_t unownForm3(uint32_t pid) noexcept {
    const uint32_t v = ((pid & 0x3000000u) >> 18) | ((pid & 0x30000u) >> 12) | ((pid & 0x300u) >> 6) | (pid & 3u);
    return static_cast<uint8_t>(v % 28u);
}
bool isFormChangeable(uint16_t species) noexcept;
/// (species, form) of EvolutionTree.Evolves4.GetBaseSpeciesForm
std::pair<uint16_t, uint8_t> baseSpeciesForm(uint16_t species, uint8_t form) noexcept;
bool decodeEqualOT(const Pk4& a, const Pk4& b) noexcept;

// ---------------------------------------------------------------- text (Gen4LegalityText.cpp)
/// StringConverter4.LoadString: code units up to the 0xFFFF terminator; unmapped units decode to U+FFFF.
std::u16string decodeText(std::span<const uint16_t> raw);
std::u16string nicknameText(const Pk4& pk);
std::u16string otText(const Pk4& pk);
/// SpeciesName.GetSpeciesNameGeneration(species, language, 4) (species 0: the Gen IV egg name).
std::u16string_view speciesName4(uint16_t species, uint8_t language) noexcept;
/// SpeciesName.IsNicknamedAnyLanguage(species, nickname, Gen4).
bool isNicknamedAnyLanguage4(uint16_t species, std::u16string_view nickname) noexcept;
/// SpeciesName.TryGetSpeciesAnyLanguage(name, out species, Gen4) (case-sensitive).
bool tryGetSpeciesAnyLanguage4(std::u16string_view name, uint16_t& species) noexcept;
/// StringConverter.HasEastAsianScriptCharacters.
bool hasEastAsianScript(std::u16string_view s) noexcept;
/// Count of chars for which char.IsNumber is true.
int numberCount(std::u16string_view s) noexcept;
/// WordFilter.IsFiltered(message, Gen4, Gen4) with default settings.
bool isWordFiltered4(std::u16string_view message);
/// WordFilter.IsFiltered(message, Gen4, original) for an original context of Gen III or Gen IV.
bool isWordFiltered(std::u16string_view message, uint8_t originalGeneration);

// ---------------------------------------------------------------- results
/// One PKHeX CheckResult. severity: 0 Valid, 1 Fishy (our Warning), 2 Invalid, 3 a PKHeX
/// Fishy result that the games are shown to produce in normal play (reported as Info).
struct Check {
    uint8_t severity = 0;
    CheckIdentifier id = CheckIdentifier::Misc;
    std::string message;
};
inline constexpr uint8_t kValid = 0, kFishy = 1, kInvalid = 2, kNote = 3;

// ---------------------------------------------------------------- format 5 (a PK5 of Gen III/IV origin)
// PKHeX analyzes a PK5 whose origin is Generation III or IV with that generation's encounter
// generator, the Gen V evolution tree and personal table, and the Gen V/IV/III learn groups.
// The Gen V data lives with the Gen V analyzer (Gen5Legality.cpp), which passes it in through
// Format5Data so that the Gen IV analyzer does not link against it.

/// PersonalTable.B2W2.GetFormEntry fields the format-5 verifiers read.
struct Personal5 {
    std::array<uint8_t, 3> abilities{}; // Ability1, Ability2, AbilityH
    uint8_t gender = 0;
    uint8_t formCount = 1;
    uint8_t growth = 0;
    bool valid = false;
    int indexOfAbility(uint8_t ability) const noexcept {
        for (int i = 0; i < 3; ++i) if (abilities[static_cast<std::size_t>(i)] == ability) return i;
        return -1;
    }
    bool ability12Same() const noexcept { return abilities[0] == abilities[1]; }
};

struct Format5Data {
    /// EvolutionTree.Evolves5 forward methods of a species (levelUp set from EvolutionType.IsLevelUpRequired).
    std::span<const D::Evolution> (*evolutions)(uint16_t species) = nullptr;
    /// PersonalTable.BW form count (the Evolves5 reverse lineage is built over BW forms).
    uint8_t (*formCountBW)(uint16_t species) = nullptr;
    /// PersonalTable.B2W2.IsPresentInGame.
    bool (*presentB2W2)(uint16_t species, uint8_t form) = nullptr;
    /// PersonalTable.B2W2.GetFormEntry.
    Personal5 (*personalB2W2)(uint16_t species, uint8_t form) = nullptr;
    /// LearnGroup5.CheckInternal for one evolution stage and form: LearnSource5B2W2 (level-up up to
    /// evo.levelMax, TM/HM, type tutors, B2W2 tutors, the enhanced tutors) then LearnSource5BW level-up.
    LearnMethod (*learn)(const Pk4& pk, const Evo& evo, uint16_t move, bool past) = nullptr;
    bool ready() const noexcept { return evolutions && formCountBW && presentB2W2 && personalB2W2 && learn; }
};

/// EvolutionHistory.Gen3 / Gen4 / Gen5 of a PK5.
struct History345 { std::vector<Evo> gen3, gen4, gen5; };
/// EncounterOrigin.GetOriginChain(pk, 4, Gen4) / (pk, 3, Gen3) for a PK5 (gen3: Gen III origin).
std::vector<Evo> originChain5(const Pk4& pk, uint8_t currentLevel, bool gen3, const Format5Data& f5);
/// EvolutionChain.GetEvolutionChainsAllGens for a PK5 and a Generation III/IV encounter.
History345 evolutionHistory5(const Pk4& pk, const EncInfo& enc, uint8_t currentLevel, const Format5Data& f5);
/// LearnVerifier.Verify for a PK5 (LearnGroup5, then LearnGroup4, then LearnGroup3 for a Gen III encounter).
MoveResults verifyMoves5(const Pk4& pk, const EncInfo& enc, const History345& history, const Format5Data& f5);
bool isValidEvolutionWithMove5(const Pk4& pk, const EncInfo& enc, const History345& history, const MoveResults& moves,
                               const Format5Data& f5);
/// EncounterGenerator4 / EncounterGenerator3 over a given origin chain (format 5 passes originChain5).
Search searchEncounters(const Pk4& pk, const std::vector<Evo>& chain);
Search searchEncounters3(const Pk4& pk, const std::vector<Evo>& chain);

/// What the format-5 origin analysis found (see analyzeOrigin5).
struct Origin5 {
    bool analyzed = false;          // the analysis ran (a Gen III/IV origin it handles)
    bool encounterMatched = false;  // an encounter template matched (not EncounterInvalid)
    bool allMovesVerified = false;  // every move slot has a source
    std::vector<Check> checks;      // PKHeX results the Gen V analyzer does not produce itself
    // Facts the Gen V name rules need.
    EncKind kind = EncKind::Invalid;
    uint16_t encSpecies = 0;
    uint8_t encGeneration = 0;      // 3 or 4
    uint8_t encContextGeneration = 0; // IEncounterTemplate.Context as a generation (3, 4; 5 for EncounterInvalid)
    bool encIsEgg = false;          // IEncounterTemplate.IsEgg
    bool playerOT = true;           // TrainerNameVerifier.IsPlayerOriginalTrainer
    bool fixedNickname = false;     // IFixedNickname { IsFixedNickname: true }
    bool gtsSanitized = false;      // LanguageVerifier GTSTrainerSanitized tag
    bool wasTradedEgg = false;      // PKM.WasTradedEgg (egg location 2002 for Gen IV, 30002 for Gen III)
    std::array<const char*, 4> moveSource{}; // how each move slot was verified (nullptr: not verified)
    bool clean() const noexcept {
        for (const Check& c : checks) if (c.severity == kInvalid || c.severity == kFishy) return false;
        return analyzed && encounterMatched && allMovesVerified;
    }
};
/// Runs PKHeX's encounter search, move/evolution history and the encounter-dependent verifiers
/// for a PK5 of Generation III/IV origin, as LegalityAnalysis does for a format-5 entity.
/// pk must be a format-5 view (pk.format == 5); currentLevel is the PK5's level.
Origin5 analyzeOrigin5(const Pk4& pk, uint8_t currentLevel, const Format5Data& f5);

} // namespace Legality::Gen4
