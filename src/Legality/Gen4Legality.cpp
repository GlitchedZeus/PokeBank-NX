/**
 * Gen4Legality.cpp - Gen IV (D/P/Pt/HG/SS) legality for PK4 records: record parsing, the
 * verified-encounter search and the field verifiers PKHeX runs for a native PK4.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): EncounterFinder.FindVerifiedEncounter,
 * EncounterVerifier (Gen IV branches), EvolutionVerifier, and LegalityAnalysis.UpdateChecks
 * for format 4: PIDVerifier, LanguageVerifier, NicknameVerifier, TrainerNameVerifier,
 * TrainerIDVerifier, IndividualValueVerifier, EffortValueVerifier, LevelVerifier,
 * RibbonVerifier (Event3/Event4/Unique3/Common3/Unique4/Common4), AbilityVerifier,
 * BallVerifier, FormVerifier, MiscVerifier (eggs, MiscVerifierG4, fateful, Pokerus, dates),
 * MovePPVerifier, GenderVerifier, ItemVerifier, ContestStatVerifier, MarkingVerifier and
 * GroundTileVerifier. PKHeX runs with default settings and no active save; its Fishy results
 * become Warnings here, Invalid stays Invalid.
 *
 * A Gen III origin (Pal Park) record from R/S/E/FR/LG runs PKHeX's ParsePK3 for format 4: the
 * Generation III encounter search (Gen4LegalityPalPark.cpp, EncounterGenerator3), the Gen III
 * learn and evolution rules, the same field verifiers with their Generation III branches, and
 * TransferVerifier.VerifyTransferLegalityG3. A Colosseum/XD origin is reported as a Warning
 * (coverage stays partial) because the Orre encounter tables are not ported.
 */
#include "Gen4LegalityInternal.h"

#include <algorithm>
#include <cstring>

namespace Legality {
namespace Gen4 {

namespace {

uint16_t rd16(std::span<const uint8_t> d, std::size_t o) noexcept {
    return static_cast<uint16_t>(d[o] | (d[o + 1] << 8));
}
uint32_t rd32(std::span<const uint8_t> d, std::size_t o) noexcept {
    return static_cast<uint32_t>(d[o]) | (static_cast<uint32_t>(d[o + 1]) << 8) |
           (static_cast<uint32_t>(d[o + 2]) << 16) | (static_cast<uint32_t>(d[o + 3]) << 24);
}

} // namespace

uint16_t checksum(std::span<const uint8_t> decrypted) noexcept {
    uint16_t sum = 0;
    if (decrypted.size() < 136) return 0;
    for (std::size_t o = 8; o < 136; o += 2) sum = static_cast<uint16_t>(sum + rd16(decrypted, o));
    return sum;
}

bool read(std::span<const uint8_t> d, Pk4& p) noexcept {
    if (d.size() != 136 && d.size() != 236) return false;
    p = Pk4{};
    p.party = d.size() == 236;
    p.pid = rd32(d, 0x00);
    p.sanity = rd16(d, 0x04);
    p.checksum = rd16(d, 0x06);
    p.species = rd16(d, 0x08);
    p.heldItem = rd16(d, 0x0A);
    p.tid = rd16(d, 0x0C);
    p.sid = rd16(d, 0x0E);
    p.exp = rd32(d, 0x10);
    p.friendship = d[0x14];
    p.ability = d[0x15];
    p.markings = d[0x16];
    p.language = d[0x17];
    for (std::size_t i = 0; i < 6; ++i) p.evs[i] = d[0x18 + i];
    for (std::size_t i = 0; i < 6; ++i) p.contest[i] = d[0x1E + i];
    for (std::size_t i = 0; i < 4; ++i) p.ribbons[i] = d[0x24 + i];
    for (std::size_t i = 0; i < 4; ++i) p.moves[i] = rd16(d, 0x28 + i * 2);
    for (std::size_t i = 0; i < 4; ++i) p.pp[i] = d[0x30 + i];
    for (std::size_t i = 0; i < 4; ++i) p.ppUps[i] = d[0x34 + i];
    p.iv32 = rd32(d, 0x38);
    for (std::size_t i = 0; i < 6; ++i) p.ivs[i] = static_cast<uint8_t>((p.iv32 >> (5 * i)) & 0x1F);
    p.isEgg = ((p.iv32 >> 30) & 1) != 0;
    p.isNicknamed = ((p.iv32 >> 31) & 1) != 0;
    for (std::size_t i = 0; i < 4; ++i) p.ribbons[4 + i] = d[0x3C + i];
    p.fateful = (d[0x40] & 1) != 0;
    p.gender = static_cast<uint8_t>((d[0x40] >> 1) & 3);
    p.form = static_cast<uint8_t>(d[0x40] >> 3);
    p.shinyLeaf = d[0x41];
    p.eggLocationExtended = rd16(d, 0x44);
    p.metLocationExtended = rd16(d, 0x46);
    for (std::size_t i = 0; i < 11; ++i) p.nickname[i] = rd16(d, 0x48 + i * 2);
    p.version = d[0x5F];
    for (std::size_t i = 0; i < 4; ++i) p.ribbons[8 + i] = d[0x60 + i];
    for (std::size_t i = 0; i < 8; ++i) p.ot[i] = rd16(d, 0x68 + i * 2);
    for (std::size_t i = 0; i < 3; ++i) p.eggDate[i] = d[0x78 + i];
    for (std::size_t i = 0; i < 3; ++i) p.metDate[i] = d[0x7B + i];
    p.eggLocationDP = rd16(d, 0x7E);
    p.metLocationDP = rd16(d, 0x80);
    p.pokerus = d[0x82];
    p.ballDPPt = d[0x83];
    p.metLevel = static_cast<uint8_t>(d[0x84] & 0x7F);
    p.otGender = static_cast<uint8_t>(d[0x84] >> 7);
    p.groundTile = d[0x85];
    p.ballHGSS = d[0x86];
    p.walkingMood = static_cast<int8_t>(d[0x87]);
    return true;
}

bool isGen4SourceGameId(std::string_view id) noexcept {
    return id == "diamond_nds" || id == "pearl_nds" || id == "platinum_nds" || id == "heartgold_nds" ||
           id == "soulsilver_nds";
}

namespace {

enum : uint8_t {
    kBallMaster = 1, kBallPoke = 4, kBallSafari = 5, kBallCherish = 16, kBallSport = 24,
};
enum : uint16_t {
    kLocDaycare4 = 2000, kLocLinkTrade4 = 2002, kLocLinkTrade6 = 30002, kLocTransfer3 = 55, kItemGriseousOrb = 112,
    kMoveSketch = 166, kMoveDracoMeteor = 434, kSpeciesMew = 151,
};
enum : uint8_t { kVerS = 1, kVerR = 2, kVerE = 3, kVerFR = 4, kVerLG = 5, kVerCXD = 15 };
enum : uint8_t { kLangJapanese = 1, kLangEnglish = 2, kLangFrench = 3, kLangItalian = 4, kLangSpanish = 7, kLangKorean = 8 };

// Ribbon bits: index into Pk4::ribbons (0x24-0x27, 0x3C-0x3F, 0x60-0x63) and bit.
struct Rib { uint8_t byte; uint8_t bit; };
constexpr Rib kChampionSinnoh{0, 0}, kAbility{0, 1}, kAbilityGreat{0, 2}, kAbilityDouble{0, 3}, kAbilityMulti{0, 4},
    kAbilityPair{0, 5}, kAbilityWorld{0, 6}, kAlert{0, 7};
constexpr Rib kShock{1, 0}, kDowncast{1, 1}, kCareless{1, 2}, kRelax{1, 3}, kSnooze{1, 4}, kSmile{1, 5},
    kGorgeous{1, 6}, kRoyal{1, 7};
constexpr Rib kGorgeousRoyal{2, 0}, kFootprint{2, 1}, kRecord{2, 2}, kEvent{2, 3}, kLegend{2, 4},
    kChampionWorld{2, 5}, kBirthday{2, 6}, kSpecial{2, 7};
constexpr Rib kSouvenir{3, 0}, kWishing{3, 1}, kClassic{3, 2}, kPremier{3, 3};
constexpr Rib kChampionG3{6, 4}, kWinning{6, 5}, kVictory{6, 6}, kArtist{6, 7};
constexpr Rib kEffort{7, 0}, kChampionBattle{7, 1}, kChampionRegional{7, 2}, kChampionNational{7, 3},
    kCountry{7, 4}, kNational{7, 5}, kEarth{7, 6}, kWorld{7, 7};
constexpr Rib kEvent3Ribbons[] = {kNational, kCountry, kChampionBattle, kChampionRegional, kChampionNational};
constexpr Rib kEvent4Ribbons[] = {kClassic, kWishing, kPremier, kEvent, kBirthday, kSpecial, kWorld, kChampionWorld, kSouvenir};
constexpr Rib kAbilityRibbons[] = {kAbility, kAbilityGreat, kAbilityDouble, kAbilityMulti, kAbilityPair, kAbilityWorld};
constexpr Rib kDailyRibbons[] = {kAlert, kShock, kDowncast, kCareless, kRelax, kSnooze, kSmile};

bool rib(const Pk4& p, Rib r) noexcept { return ((p.ribbons[r.byte] >> r.bit) & 1) != 0; }

// NatureAmp.Table rows [Atk, Def, SpA, SpD, Spe] (PKHeX Editing/NatureAmp.cs).
constexpr int8_t kNatureAmp[25][5] = {
    {0, 0, 0, 0, 0}, {1, -1, 0, 0, 0}, {1, 0, 0, 0, -1}, {1, 0, -1, 0, 0}, {1, 0, 0, -1, 0},
    {-1, 1, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 0, 0, -1}, {0, 1, -1, 0, 0}, {0, 1, 0, -1, 0},
    {-1, 0, 0, 0, 1}, {0, -1, 0, 0, 1}, {0, 0, 0, 0, 0}, {0, 0, -1, 0, 1}, {0, 0, 0, -1, 1},
    {-1, 0, 1, 0, 0}, {0, -1, 1, 0, 0}, {0, 0, 1, 0, -1}, {0, 0, 0, 0, 0}, {0, 0, 1, -1, 0},
    {-1, 0, 0, 1, 0}, {0, -1, 0, 1, 0}, {0, 0, 0, 1, -1}, {0, 0, -1, 1, 0}, {0, 0, 0, 0, 0},
};

// EggHatchLocation4.LocationPermitted4: bit 0 D/P, bit 1 Pt, bit 2 HG/SS.
constexpr uint8_t kHatch4[] = {
    0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 7, 3, 3, 3, 3, 2, 0, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 6, 6, 6, 6, 6, 2, 6, 2,
    2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4,
};
bool hatchFlag(uint16_t location, uint8_t mask) noexcept {
    return location < sizeof(kHatch4) && (kHatch4[location] & mask) != 0;
}
bool isValidMet4(uint16_t location, uint8_t version) noexcept {
    if (version == D::kD || version == D::kP) return hatchFlag(location, 1);
    if (version == D::kPt) return hatchFlag(location, 2);
    if (isHGSS(version)) return hatchFlag(location, 4);
    return false;
}

constexpr uint16_t kBattleFrontierBanlist[] = {150, 151, 249, 250, 251, 382, 383, 384, 385, 386,
                                               483, 484, 487, 489, 490, 491, 492, 493};
bool isAllowedBattleFrontier(uint16_t species) noexcept {
    return std::find(std::begin(kBattleFrontierBanlist), std::end(kBattleFrontierBanlist), species) ==
           std::end(kBattleFrontierBanlist);
}

// FormInfo.BattleMegas restricted to species <= 493 (all have form count 1 in Gen IV).
constexpr uint16_t kMegaSpecies4[] = {3, 6, 9, 15, 18, 26, 36, 65, 71, 80, 94, 115, 121, 127, 130, 142, 149, 150,
                                      154, 160, 181, 208, 212, 214, 227, 229, 248, 254, 257, 260, 282, 302, 303, 306,
                                      308, 310, 319, 323, 334, 354, 359, 362, 373, 376, 380, 381, 384, 428, 445, 448,
                                      460, 475, 478};
bool isMegaSpecies4(uint16_t species) noexcept {
    return std::find(std::begin(kMegaSpecies4), std::end(kMegaSpecies4), species) != std::end(kMegaSpecies4);
}

constexpr uint16_t kArceusPlates[] = {303, 306, 304, 305, 309, 308, 310, 313, 298, 299, 301, 300, 307, 302, 311, 312, 644};
constexpr uint16_t kArceusZCrystals[] = {782, 785, 783, 784, 788, 787, 789, 792, 777, 778, 780, 779, 786, 781, 790, 791, 793};
uint8_t arceusForm4(uint16_t item) noexcept {
    if (item >= 777 && item <= 793) {
        const auto* it = std::find(std::begin(kArceusZCrystals), std::end(kArceusZCrystals), item);
        return static_cast<uint8_t>(it == std::end(kArceusZCrystals) ? 0 : (it - std::begin(kArceusZCrystals)) + 1);
    }
    if ((item >= 298 && item <= 313) || item == 644) {
        const auto* it = std::find(std::begin(kArceusPlates), std::end(kArceusPlates), item);
        const int form = it == std::end(kArceusPlates) ? 0 : static_cast<int>(it - std::begin(kArceusPlates)) + 1;
        return static_cast<uint8_t>(form < 9 ? form : form + 1);
    }
    return 0;
}

bool isLeapYear(int y) noexcept { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
bool isValidDate(const std::array<uint8_t, 3>& d) noexcept {
    const int year = 2000 + d[0];
    const int month = d[1];
    const int day = d[2];
    if (month < 1 || month > 12 || day < 1) return false;
    static constexpr int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max = kDays[month - 1];
    if (month == 2 && isLeapYear(year)) max = 29;
    return day <= max;
}

bool isZero(const std::array<uint8_t, 3>& d) noexcept { return d[0] == 0 && d[1] == 0 && d[2] == 0; }

const char* versionName(uint8_t v) noexcept {
    switch (v) {
    case 1: return "Sapphire"; case 2: return "Ruby"; case 3: return "Emerald"; case 4: return "FireRed";
    case 5: return "LeafGreen"; case 15: return "Colosseum/XD"; case 7: return "HeartGold"; case 8: return "SoulSilver";
    case 10: return "Diamond"; case 11: return "Pearl"; case 12: return "Platinum";
    default: return nullptr;
    }
}

bool allValid(const MoveResults& r) noexcept {
    for (const auto& m : r) if (!m.valid()) return false;
    return true;
}

std::string moveFailText(std::size_t slot, const MoveResult& m, uint16_t move, bool transferred = false) {
    std::string what;
    switch (m.method) {
    case LearnMethod::Duplicate: what = "is a duplicate of a later move"; break;
    case LearnMethod::EmptyInvalid: what = "is empty before a later move (moves must be packed)"; break;
    case LearnMethod::UnobtainableExpect: what = "is not the move this encounter must know"; break;
    default:
        what = transferred ? "cannot be learned by this Pokemon in any generation it lived in (III/IV before transfer, V after)"
                           : "cannot be learned by this Pokemon in Generation IV";
        break;
    }
    return "Move " + std::to_string(slot + 1) + " (#" + std::to_string(move) + ") " + what;
}

// How LearnVerifier verified a move slot, for MoveEvidence (nullptr: not verified).
const char* learnMethodText(LearnMethod m) noexcept {
    switch (m) {
    case LearnMethod::Empty: return "empty slot";
    case LearnMethod::Initial: return "a move the matched encounter starts with";
    case LearnMethod::LevelUp: return "level-up";
    case LearnMethod::TMHM: return "TM/HM";
    case LearnMethod::Tutor: return "move tutor";
    case LearnMethod::Sketch: return "Sketch";
    case LearnMethod::HOME: return "HOME";
    case LearnMethod::Encounter: return "the matched encounter";
    case LearnMethod::Evolution: return "evolution";
    case LearnMethod::Shared: return "shared egg move";
    case LearnMethod::ShedinjaEvo: return "Ninjask move kept by Shedinja";
    case LearnMethod::Relearn: return "relearn";
    case LearnMethod::EggMove: return "egg move";
    case LearnMethod::InheritLevelUp: return "inherited level-up move";
    case LearnMethod::SpecialEgg: return "special egg move";
    default: return nullptr;
    }
}

// ======================================================================= analyzer
class Analyzer {
public:
    /// cxd: the Gen III GameCube judge's answer for a Colosseum/XD origin (version 15); null means
    /// no judge (CxdOriginJudgement::Status::Unavailable). Ignored for every other origin.
    Analyzer(const Pk4& pk, Table cart, const CxdOriginJudgement* cxd = nullptr, bool cartKnown = true)
        : pk_(pk), cart_(cart), cartKnown_(cartKnown), cxd_(cxd) {}
    /// Format-5 mode: pk is the view of a PK5 of Generation III/IV origin (pk.format == 5).
    Analyzer(const Pk4& pk, const Format5Data& f5, const CxdOriginJudgement* cxd = nullptr) : pk_(pk), f5_(&f5), cxd_(cxd) {}
    Report run();
    /// The format-5 run: the encounter search, move/evolution history and the encounter-dependent
    /// verifiers LegalityAnalysis runs for a PK5 (see analyzeOrigin5).
    Origin5 run5(uint8_t currentLevel);
    MoveEvidence evidence() const noexcept;

private:
    const Pk4& pk_;
    const Format5Data* f5_ = nullptr; // format-5 mode (Gen V data hooks)
    History345 h5_{};                  // format 5: EvolutionHistory.Gen3 / Gen4 / Gen5
    Template enc_{};
    EncInfo info_{};
    std::vector<Evo> history_; // EvolutionHistory.Gen4
    History34 history3_{};     // Gen III origin only: EvolutionHistory.Gen3 (and Gen4, copied to history_)
    MoveResults moves_{};
    Search search_{};
    std::vector<Check> parse_;
    uint8_t level_ = 1;     // CurrentLevel (HGSS growth of the current species/form)
    Table cart_ = Table::HGSS; // personal table of the game whose save holds the record
    bool cartKnown_ = true;    // exactSourceGameId named one of the five Gen IV games (cart_ is not a default)
    Personal pi_{};         // LegalityAnalysis.PersonalInfo: cart table's GetFormEntry(species, form)
    Personal piEntity_{};   // PKM.PersonalInfo: PersonalTable.HGSS.GetFormEntry(species, form)
    bool gtsSanitized_ = false;
    bool encounterSearched_ = false;
    const CxdOriginJudgement* cxd_ = nullptr; // Colosseum/XD origin: the judge's answer (see findCxdEncounter)
    bool cxdUndecided_ = false;               // Colosseum/XD origin left unverified: encounter-dependent rules are skipped

    void add(uint8_t severity, CheckIdentifier id, std::string text) {
        parse_.push_back(Check{severity, id, std::move(text)});
    }
    void invalid(CheckIdentifier id, std::string text) { add(kInvalid, id, std::move(text)); }
    void fishy(CheckIdentifier id, std::string text) { add(kFishy, id, std::move(text)); }
    void note(CheckIdentifier id, std::string text) { add(kNote, id, std::move(text)); }

    bool isMysteryGift() const noexcept { return enc_.kind == EncKind::Pcd || enc_.kind == EncKind::RangerManaphy; }
    /// Format 5: the PKHeX Fishy heuristics the Gen V analyzer does not report for any record
    /// (PID 0, suspicious trainer IDs, equal IVs/EVs, EXP thresholds) are left out.
    bool format5() const noexcept { return f5_ != nullptr; }
    /// TrainerNameVerifier.IsPlayerOriginalTrainer
    bool isPlayerOT() const noexcept {
        if (enc_.kind == EncKind::Trade || enc_.kind == EncKind::Ranch) return false;
        if (isGen3Kind(enc_.kind)) return !isFixedTrainer3(enc_); // GiftColo3 (ITrainerID16ReadOnly) is fixed too
        if (isMysteryGift() && !enc_.isEgg) return false;
        return true;
    }
    bool gen3Origin() const noexcept { return isGen3Version(pk_.version); }
    bool hasVisitedGen3() const noexcept { return !history3_.gen3.empty(); }
    bool hasVisitedGen4() const noexcept { return !history_.empty(); }
    /// PKM.GetTradedEggLocation: Generation 4 uses the link-trade location, Generation 3 the default.
    uint16_t tradedEggLocation() const noexcept { return gen3Origin() ? kLocLinkTrade6 : kLocLinkTrade4; }
    bool wasTradedEgg() const noexcept { return pk_.eggLocation() == tradedEggLocation(); }
    bool isWithinEncounterRange() const noexcept;
    /// IEncounterTemplate.Generation of the matched encounter (EncounterInvalid takes the origin's).
    uint8_t encGeneration() const noexcept {
        if (isGen3Kind(enc_.kind)) return 3;
        if (enc_.kind != EncKind::Invalid) return 4;
        return isGen4Version(pk_.version) ? 4 : (gen3Origin() ? 3 : 0);
    }
    /// IEncounterTemplate.Context as a generation (EncounterInvalid takes the entity's: Gen IV, or V for a PK5).
    uint8_t encContextGeneration() const noexcept {
        if (isGen3Kind(enc_.kind)) return 3;
        return enc_.kind == EncKind::Invalid && format5() ? 5 : 4;
    }
    bool abilityCanMismatch3();

    // encounter search
    Check verifyEncounter(const Template& t) const;
    Check verifyEncounter5(const Template& t) const;
    Check verifyEgg4Hatched() const;
    void findVerifiedEncounter();
    void findCxdEncounter();
    void setCxdHistory();
    Check verifyEvolution(const Template& t, const std::vector<Evo>& history, const MoveResults& moves) const;

    // UpdateChecks
    void verifyPid();
    void verifyLanguage();
    void verifyNickname();
    void verifyTrade4Names(const std::u16string& nickname);
    void verifyNicknameEgg(const std::u16string& nickname);
    void verifyTrainerName();
    void verifyTrainerId();
    void verifyIvs();
    void verifyEvs();
    void verifyLevel();
    void verifyRibbons();
    void verifyAbility();
    void verifyBall();
    void verifyForm();
    void verifyMisc();
    void verifyFateful();
    void verifyAbility5();
    void verifyGender5();
    void verifyMovePP();
    void verifyGender();
    void verifyItem();
    void verifyContest();
    void verifyMarking();
    void verifyGroundTile();
    void verifyMoveResults();
};

bool Analyzer::isWithinEncounterRange() const noexcept {
    const uint8_t level = level_;
    // PKM.HasOriginalMetLocation is false for a Gen III origin (the met data was rewritten by Pal Park).
    if (gen3Origin()) return enc_.levelMin <= level && level <= enc_.levelMax;
    if (enc_.isEgg) return level == enc_.levelMin;
    if (enc_.kind == EncKind::Ranch) return enc_.levelMin <= level && level <= enc_.levelMax;
    if (pk_.metLevel != 0) return level == pk_.metLevel;
    return enc_.levelMin <= level && level <= enc_.levelMax;
}

// ----------------------------------------------------------------------- encounter verifier
Check Analyzer::verifyEgg4Hatched() const {
    if (pk_.metLevel != 0) return {kInvalid, CheckIdentifier::Egg, "Hatched egg: met level must be 0"};
    const uint16_t met = pk_.metLocation();
    if (pk_.language == kLangKorean && (met == 63 || met == 85))
        return {kInvalid, CheckIdentifier::Egg, "Hatched egg: Korean games cannot reach this hatch location"};
    if (isValidMet4(met, pk_.version)) return {};
    if (pk_.eggLocation() == kLocLinkTrade4 && hatchFlag(met, 7)) return {};
    return {kInvalid, CheckIdentifier::Egg, "Hatched egg: hatch location is not reachable in the origin game"};
}

Check Analyzer::verifyEncounter(const Template& t) const {
    if (t.kind == EncKind::Static3) {
        const auto& s = D::kStatics3[t.index];
        if (s.species == kSpeciesMew && pk_.language != kLangJapanese)
            return {kInvalid, CheckIdentifier::Encounter, "Mew at Faraway Island was only distributed to Japanese games"};
        if (s.species == kDeoxys && s.location == 200 && pk_.language == kLangJapanese)
            return {kInvalid, CheckIdentifier::Encounter, "Deoxys at Birth Island was never distributed to Japanese games"};
    }
    if (t.kind == EncKind::Egg3 && pk_.isEgg) { // VerifyUnhatchedEgg3
        if (pk_.metLevel != 0) return {kInvalid, CheckIdentifier::Egg, "Egg: met level must be 0"};
        const uint16_t hatch = (pk_.version == kVerFR || pk_.version == kVerLG) ? 146 : 32;
        if (pk_.metLocation() != hatch) return {kInvalid, CheckIdentifier::Egg, "Egg: met location is not the Generation III egg location"};
        return {};
    }
    if (isGen3Kind(t.kind)) {
        if (t.isEgg && !pk_.isEgg) { // VerifyEncounterEgg3Transfer (format 4)
            if (pk_.metLevel < 5)
                return {kInvalid, CheckIdentifier::Egg, "Pal Park transfer of a hatched egg: met level must be at least 5"};
            if (pk_.eggLocation() != 0)
                return {kInvalid, CheckIdentifier::Egg, "Pal Park transfer of a hatched egg: egg location must be empty"};
            if (pk_.metLocation() != kLocTransfer3)
                return {kInvalid, CheckIdentifier::Egg, "Pal Park transfer of a hatched egg: met location must be Pal Park"};
        }
        return {};
    }
    if (t.kind == EncKind::Static && t.species == kShaymin && pk_.language == kLangKorean)
        return {kInvalid, CheckIdentifier::Encounter, "Shaymin was never distributed to Korean games"};
    if (t.kind == EncKind::Static && D::kStatics[t.index].roaming && pk_.metLocation() == 193 && pk_.groundTile == 7)
        return {kInvalid, CheckIdentifier::Encounter, "Roaming Pokemon cannot be met surfing on Route 45"};
    if (t.kind == EncKind::Pcd || t.kind == EncKind::RangerManaphy) {
        if (!pk_.isEgg && t.isEgg) return verifyEgg4Hatched();
        return {};
    }
    if (t.kind == EncKind::Egg && pk_.isEgg) {
        if (pk_.metLevel != 0) return {kInvalid, CheckIdentifier::Egg, "Egg: met level must be 0"};
        if (pk_.eggLocation() == kLocLinkTrade4)
            return {kInvalid, CheckIdentifier::Egg, "Egg: egg location cannot be a link trade"};
        const uint16_t met = pk_.metLocation();
        if (met == kLocLinkTrade4 || met == 0) return {};
        return {kInvalid, CheckIdentifier::Egg, "Egg: unhatched egg has a met location"};
    }
    if (t.isEgg && !pk_.isEgg) return verifyEgg4Hatched();
    if (t.kind == EncKind::Invalid) return {kInvalid, CheckIdentifier::Encounter, ""};
    return {};
}

/// EncounterVerifier.VerifyEncounter for a PK5 (format 5): the Route 45 roamer rule needs a G4PKM, and a
/// hatched egg runs VerifyEncounterEgg3Transfer / VerifyEncounterEgg4's transferred branch (the PK5 keeps
/// only the Poke Transfer met location and level).
Check Analyzer::verifyEncounter5(const Template& t) const {
    if (t.kind == EncKind::Static3) {
        const auto& s = D::kStatics3[t.index];
        if (s.species == kSpeciesMew && pk_.language != kLangJapanese)
            return {kInvalid, CheckIdentifier::Encounter, "Mew at Faraway Island was only distributed to Japanese games"};
        if (s.species == kDeoxys && s.location == 200 && pk_.language == kLangJapanese)
            return {kInvalid, CheckIdentifier::Encounter, "Deoxys at Birth Island was never distributed to Japanese games"};
    }
    if (t.kind == EncKind::Static && t.species == kShaymin && pk_.language == kLangKorean)
        return {kInvalid, CheckIdentifier::Encounter, "Shaymin was never distributed to Korean games"};
    // EncounterInvalid (its IsEgg is PKM.WasEgg; VerifyEncounterEgg5 rejects a Poke Transfer met location too).
    if (t.kind == EncKind::Invalid) return {kInvalid, CheckIdentifier::Encounter, ""};
    if (pk_.isEgg && (t.isEgg || t.kind == EncKind::Egg || t.kind == EncKind::Egg3))
        return {kInvalid, CheckIdentifier::Egg, "An egg cannot be moved to Generation V"};
    if (t.isEgg && !pk_.isEgg) {
        const uint16_t kTransfer4 = 30001;
        if (isGen3Kind(t.kind)) { // VerifyEncounterEgg3Transfer
            if (pk_.metLevel < 5)
                return {kInvalid, CheckIdentifier::Egg, "Transferred Generation III egg Pokemon: met level must be at least 5"};
            if (pk_.eggLocation() != 0)
                return {kInvalid, CheckIdentifier::Egg, "Transferred Generation III egg Pokemon: egg location must be empty"};
        } else if (pk_.metLevel < 1) { // VerifyEncounterEgg4 (transferred)
            return {kInvalid, CheckIdentifier::Egg, "Transferred egg Pokemon: met level must be at least 1"};
        }
        if (pk_.metLocation() != kTransfer4)
            return {kInvalid, CheckIdentifier::Egg, "Transferred egg Pokemon: met location must be Poke Transfer"};
        return {};
    }
    return {};
}

Check Analyzer::verifyEvolution(const Template& t, const std::vector<Evo>& history, const MoveResults& moves) const {
    if (format5() && pk_.species != t.species) {
        // EvolutionVerifier for a PK5: the current species in the Gen V history, the encounter species
        // in the history of the encounter's context.
        const std::vector<Evo>& original = isGen3Kind(t.kind) ? h5_.gen3 : t.kind == EncKind::Invalid ? h5_.gen5 : h5_.gen4;
        if (!hasVisited(h5_.gen5, pk_.species) || !hasVisited(original, t.species))
            return {kInvalid, CheckIdentifier::Species, "Evolution from the encounter species is not possible"};
        if (!isValidEvolutionWithMove5(pk_, encInfo(t), h5_, moves, *f5_))
            return {kInvalid, CheckIdentifier::Species, "Evolution requires a move this Pokemon cannot have known"};
        return {};
    }
    if (pk_.species != t.species) {
        // The encounter species must appear in the history of the encounter's own context.
        const bool gen3 = isGen3Kind(t.kind);
        const std::vector<Evo>& original = gen3 ? history3_.gen3 : history;
        if (!hasVisited(history, pk_.species) || !hasVisited(original, t.species))
            return {kInvalid, CheckIdentifier::Species, "Evolution from the encounter species is not possible"};
        const bool withMove = gen3 ? isValidEvolutionWithMove3(pk_, encInfo(t), history3_, moves)
                                   : isValidEvolutionWithMove(pk_, encInfo(t), history, moves);
        if (!withMove)
            return {kInvalid, CheckIdentifier::Species, "Evolution requires a move this Pokemon cannot have known"};
    }
    return {};
}

std::string encounterHint(const Pk4& pk) {
    if (pk.format == 5) {
        if (pk.fateful) return std::string("No matching Generation ") + (isGen3Version(pk.version) ? "III" : "IV") + " event encounter for this transferred Pokemon";
        return std::string("No Generation ") + (isGen3Version(pk.version) ? "III" : "IV") +
               " encounter matches this transferred Pokemon (species, level, ball, PID/IV)";
    }
    if (isGen3Version(pk.version)) {
        if (pk.fateful) return "No matching Generation III event encounter";
        return "No matching Generation III encounter for this species, level, ball and PID/IV";
    }
    if (!pk.fateful && pk.eggLocation() - 2009u <= 5u) return "No matching gift egg";
    if (pk.fateful && pk.eggDate[2] != 0) return "No matching event egg (the only Gen IV event egg is Manaphy)";
    const uint16_t met = pk.metLocation();
    if (pk.fateful || (met >= 3000 && met <= 3076)) return "No matching Mystery Gift (event) encounter";
    return "No matching Gen IV encounter for this species, level, location, ball and PID/IV";
}

void Analyzer::findVerifiedEncounter() {
    // A Colosseum/XD origin (Pal Park too) is judged by the shared GameCube judge, not by the ported tables.
    if (pk_.version == kVerCXD) { findCxdEncounter(); return; }
    // A Gen III origin (Pal Park) record runs EncounterGenerator3 and the Gen III learn/evolution rules.
    const bool gen3 = gen3Origin();
    if (format5()) {
        // A PK5: the origin generation's generator over EncounterOrigin.GetOriginChain for format 5.
        const std::vector<Evo> chain = originChain5(pk_, level_, gen3, *f5_);
        search_ = gen3 ? searchEncounters3(pk_, chain) : searchEncounters(pk_, chain);
    } else {
        search_ = gen3 ? searchEncounters3(pk_, level_) : searchEncounters(pk_, level_);
    }
    encounterSearched_ = true;
    auto makeTemplateAny = [&](const Candidate& c) { return gen3 ? makeTemplate3(c, pk_) : makeTemplate(c, pk_); };
    auto setHistory = [&](const EncInfo& info) {
        if (format5()) {
            h5_ = evolutionHistory5(pk_, info, level_, *f5_);
            history3_.gen3 = h5_.gen3;
            history3_.gen4 = h5_.gen4;
            history_ = h5_.gen4;
        } else if (gen3) {
            history3_ = evolutionHistory3(pk_, info, level_);
            history_ = history3_.gen4;
        } else {
            history_ = evolutionHistory(pk_, info, level_);
        }
    };
    auto verifyMovesAny = [&](const EncInfo& info) {
        if (format5()) return verifyMoves5(pk_, info, h5_, *f5_);
        return gen3 ? verifyMoves3(pk_, info, history3_) : verifyMoves(pk_, info, history_);
    };
    auto verifyEncounterAny = [&](const Template& t) { return format5() ? verifyEncounter5(t) : verifyEncounter(t); };
    if (search_.list.empty()) {
        enc_ = makeTemplateAny(Candidate{});
        info_ = encInfo(enc_);
        setHistory(info_);
        parse_.clear();
        invalid(CheckIdentifier::Encounter, encounterHint(pk_));
        moves_ = verifyMovesAny(info_);
        return;
    }
    // PKHeX enumerates lazily (PeekEnumerator) and sets ManualFlag only when the generator is
    // advanced to its deferred last resort, so the flag counts only when that element was
    // requested: by MoveNext, or by a PeekIsNext made while the element before it was current.
    const std::size_t n = search_.list.size();
    std::size_t requested = 0; // highest list index the generator has produced
    auto peek = [&](std::size_t i) {
        if (i + 1 < n) requested = std::max(requested, i + 1);
        return i + 1 < n;
    };
    for (std::size_t i = 0; i < n; ++i) {
        requested = std::max(requested, i);
        const Template t = makeTemplateAny(search_.list[i]);
        Check e = verifyEncounterAny(t);
        if (e.severity != kValid && peek(i)) continue;
        enc_ = t;
        info_ = encInfo(t);
        parse_.clear();
        if (e.severity != kValid) {
            if (e.message.empty()) e.message = encounterHint(pk_);
            parse_.push_back(e);
        }
        setHistory(info_);
        moves_ = verifyMovesAny(info_);
        if (!allValid(moves_) && peek(i)) continue;
        bool parseValid = true;
        for (const auto& c : parse_) if (c.severity == kInvalid) parseValid = false;
        if (!parseValid && peek(i)) continue;
        const Check evo = verifyEvolution(t, history_, moves_);
        if (evo.severity != kValid && peek(i)) continue;
        if (evo.severity != kValid) parse_.push_back(evo);
        const int rating = gen3 ? matchRating3(t, pk_) : matchRating(t, pk_);
        if (rating < 0 || rating < 3) break; // not IEncounterMatch, or better than PartialMatch
        if (peek(i)) continue;
        invalid(CheckIdentifier::Encounter, "Encounter is only a partial match (wrong version, location or IVs)");
        break;
    }
    const bool lastResortReached = requested + 1 == n;
    // FramePatternSettings: RNGFrameNotFound3 is Fishy, RNGFrameNotFound4 Invalid.
    if (lastResortReached && search_.manualFlag == 2 && encGeneration() == 3)
        fishy(CheckIdentifier::PidRng, "The PID/IV does not match any reachable RNG frame for this Generation III wild encounter and lead");
    else if (lastResortReached && search_.manualFlag == 2)
        invalid(CheckIdentifier::PidRng, "The PID/IV does not match any reachable RNG frame for this wild encounter and lead");
    else if (lastResortReached && search_.manualFlag == 1)
        invalid(CheckIdentifier::PidRng, "The PID/IV correlation type does not match this encounter");
}

/// The evolution history of a Colosseum/XD origin: through Gen III and Gen IV for a Pal Park
/// record, and on to Gen V for a Poke Transfer record (format 5).
void Analyzer::setCxdHistory() {
    if (format5()) {
        h5_ = evolutionHistory5(pk_, info_, level_, *f5_);
        history3_.gen3 = h5_.gen3;
        history3_.gen4 = h5_.gen4;
        history_ = h5_.gen4;
    } else {
        history3_ = evolutionHistory3(pk_, info_, level_);
        history_ = history3_.gen4;
    }
}

/// Colosseum/XD origin (version 15). PKHeX runs EncounterGenerator3 over the Orre templates
/// (Encounters3Colo/ColoShadow/XD/XDShadow). Here that search is the Gen III GameCube judge's
/// (include/Legality/Gen3Cxd.h through Gen4::judgeCxdOrigin); this analyzer consumes its answer:
///  - Matched: the judge's template becomes the EncKind::Cxd3 encounter and every verifier runs
///    against it (moves through LearnGroup3 with the template's moves, evolution, level/EXP, EVs,
///    ribbons with the National Ribbon required for a purified shadow Pokemon, fateful flag, ball,
///    ability, gender, Pal Park met data); its Invalid reasons and Warnings are reported as such.
///  - NoMatch: Invalid with the judge's reasons (EncounterVerifier's EncInvalid).
///  - Undecided / Unavailable: a Warning that the Orre encounter is not verified. The rules that
///    need the encounter (ribbons, current moves, fateful flag, the EV/EXP comparison against the
///    encounter level) are skipped; the origin-independent ones (IVs, EVs, level and EXP, names,
///    language, PP, item, markings, form, gender from the PID, contest sheen, Pokerus, dates, the
///    Pal Park met data and met location 55) still run. The record is Incomplete, never certified.
void Analyzer::findCxdEncounter() {
    using Status = CxdOriginJudgement::Status;
    encounterSearched_ = true;
    search_ = Search{};
    parse_.clear();
    const CxdOriginJudgement* j = cxd_;
    const Status status = j != nullptr ? j->status : Status::Unavailable;
    if (status == Status::Matched) {
        Template t;
        t.kind = EncKind::Cxd3;
        t.species = j->species; t.form = j->form;
        t.levelMin = j->levelMin; t.levelMax = j->levelMax;
        t.version = kVerCXD; t.location = 0;
        t.fixedBall = j->fixedBall; t.fateful = j->fateful; t.shiny = j->shiny;
        t.gender = j->gender; t.ability = j->ability; t.moves = j->moves;
        t.fixedTrainer = j->fixedTrainer; t.fixedNickname = j->fixedNickname;
        t.shadow = j->shadow; t.name = j->templateName;
        search_.list.push_back(Candidate{EncKind::Cxd3, -1, -1, j->species, kVerCXD, Lead::Invalid});
        enc_ = t;
        info_ = encInfo(t);
        const Check e = format5() ? verifyEncounter5(t) : verifyEncounter(t);
        if (e.severity != kValid) parse_.push_back(e);
        setCxdHistory();
        moves_ = format5() ? verifyMoves5(pk_, info_, h5_, *f5_) : verifyMoves3(pk_, info_, history3_);
        for (const auto& alt : j->alternativeMoves) {
            if (allValid(moves_)) break;
            Template a = t;
            a.moves = alt;
            const EncInfo altInfo = encInfo(a);
            MoveResults m = format5() ? verifyMoves5(pk_, altInfo, h5_, *f5_) : verifyMoves3(pk_, altInfo, history3_);
            if (!allValid(m)) continue;
            t = a; enc_ = a; info_ = altInfo; moves_ = m;
        }
        const Check evo = verifyEvolution(t, history_, moves_);
        if (evo.severity != kValid) parse_.push_back(evo);
        for (const std::string& r : j->invalidReasons) invalid(CheckIdentifier::Encounter, r);
        for (const std::string& w : j->warnings) fishy(CheckIdentifier::Encounter, w);
        for (const std::string& n : j->notes) note(CheckIdentifier::Encounter, n);
        if (t.name != nullptr) note(CheckIdentifier::Encounter, std::string("Colosseum/XD origin: ") + t.name);
        return;
    }
    // EncounterInvalid view of the record (species, met level, current level).
    enc_ = makeTemplate3(Candidate{}, pk_);
    info_ = encInfo(enc_);
    setCxdHistory();
    moves_ = MoveResults{}; // nothing verified: the hook must not treat the moves as verified
    if (status == Status::NoMatch) {
        if (j->invalidReasons.empty()) invalid(CheckIdentifier::Encounter, "No Colosseum/XD encounter matches this Pokemon");
        for (const std::string& r : j->invalidReasons) invalid(CheckIdentifier::Encounter, r);
        for (const std::string& w : j->warnings) fishy(CheckIdentifier::Encounter, w);
        for (const std::string& n : j->notes) note(CheckIdentifier::Encounter, n);
        return;
    }
    cxdUndecided_ = true;
    const char* name = versionName(pk_.version);
    fishy(CheckIdentifier::Transfer, std::string(format5() ? "Poke Transfer from " : "Pal Park transfer from ") +
                                         (name != nullptr ? name : "Colosseum/XD") + ": the Colosseum/XD encounter is not verified");
    if (j != nullptr) {
        for (const std::string& w : j->warnings) fishy(CheckIdentifier::Encounter, w);
        for (const std::string& n : j->notes) note(CheckIdentifier::Encounter, n);
    }
}

// ----------------------------------------------------------------------- PID / EC
void Analyzer::verifyPid() {
    if (enc_.species == kWurmple && pk_.species != kWurmple && !isWurmpleEvoValid(pk_.pid, pk_.species))
        invalid(CheckIdentifier::PidRng, "Wurmple's evolution does not match its PID");
    if (pk_.pid == 0 && !format5()) fishy(CheckIdentifier::PidRng, "PID is zero");
    auto verifyEggGender8000 = [&]() {
        if (enc_.species > kMaxSpecies4 || (D::kBreed[enc_.species].flags & 8) == 0) return;
        const bool bit34 = pk_.gender != ((pk_.pid & 0x8000u) >> 15);
        if (bit34) return;
        bool inherited = false;
        for (const auto& m : moves_) if (m.method == LearnMethod::InheritLevelUp) inherited = true;
        if (pk_.gender == 1 || inherited)
            invalid(CheckIdentifier::PidRng, "PID bit 15 does not match the gender for this Nidoran/Volbeat/Illumise egg");
    };
    if (enc_.kind == EncKind::Egg) {
        if (pk_.pid == 0 && !format5()) fishy(CheckIdentifier::PidRng, "Egg PID is zero");
        verifyEggGender8000();
    } else if (enc_.kind == EncKind::Egg3) {
        // Daycare3.IsValidProcPID: R/S/FR/LG store only a non-zero lower half; Emerald any non-zero PID.
        const bool proc = enc_.version == kVerE ? pk_.pid != 0 : (pk_.pid & 0xFFFFu) != 0;
        if (!proc) invalid(CheckIdentifier::PidRng, "Generation III egg PID cannot be produced by the Day Care");
        verifyEggGender8000();
    }
    // Shiny.IsValid
    const bool shiny = pk_.isShiny();
    const uint32_t xorValue = (pk_.tid ^ pk_.sid ^ (pk_.pid >> 16) ^ (pk_.pid & 0xFFFFu)) & 0xFFFFu;
    bool shinyOk = true;
    switch (enc_.shiny) {
    case kShinyNever: shinyOk = !shiny; break;
    case kShinyAlways: shinyOk = shiny; break;
    case kShinyAlwaysStar: shinyOk = xorValue == 1; break;
    case kShinyAlwaysSquare: shinyOk = xorValue == 0; break;
    default: break;
    }
    if (!shinyOk) invalid(CheckIdentifier::PidRng, shiny ? "This encounter cannot be shiny" : "This encounter must be shiny");
    if (enc_.kind == EncKind::Pcd && enc_.gift.pk.pid != 1 && pk_.pid != enc_.gift.pk.pid)
        invalid(CheckIdentifier::EventGift, "PID does not match the Mystery Gift's fixed PID");
}

// ----------------------------------------------------------------------- language
void Analyzer::verifyLanguage() {
    const uint8_t lang = pk_.language;
    const uint8_t gen = encGeneration();
    if (format5()) {
        // The Gen V analyzer checks the language ID; only the GTS sanitization tag is needed here
        // (LanguageVerifier.CheckGTS: the format-4 GTS lockout does not apply to a PK5).
        if (gen == 4 && lang == kLangKorean && !pk_.isEgg) {
            const std::u16string ot = otText(pk_);
            gtsSanitized_ = ot == u"Ahn" || ot == u"Han" || ot == u"Jeong" || ot == u"Hwang";
        }
        return;
    }
    const int max = gen == 4 ? 8 : gen == 3 ? 7 : -1; // Legal.GetMaxLanguageID: Korean arrived in Generation IV
    if (lang == 6 || lang == 0 || static_cast<int>(lang) > max) {
        invalid(CheckIdentifier::Trainer, "Language ID " + std::to_string(lang) +
                                              (gen == 3 ? " is not valid for a Pokemon from a Generation III game"
                                                        : " is not valid for a Gen IV Pokemon"));
        return;
    }
    if (encGeneration() == 4 && lang == kLangKorean && !pk_.isEgg) {
        const std::u16string ot = otText(pk_);
        gtsSanitized_ = ot == u"Ahn" || ot == u"Han" || ot == u"Jeong" || ot == u"Hwang";
    }
}

// ----------------------------------------------------------------------- nickname
void Analyzer::verifyNicknameEgg(const std::u16string& nickname) {
    // EggStateLegality.IsNicknameFlagSet and SpeciesName.GetEggName(language, enc.Generation).
    if (encGeneration() == 3) {
        if (!pk_.isNicknamed) invalid(CheckIdentifier::Egg, "Generation III eggs have the nickname flag set");
        const std::u16string_view expect = pk_.language < 9 ? std::u16string_view(D::kEggName3[pk_.language]) : std::u16string_view();
        if (nickname != expect) invalid(CheckIdentifier::Egg, "Egg name does not match the egg name for its language");
        return;
    }
    if (pk_.isNicknamed) invalid(CheckIdentifier::Egg, "Gen IV eggs do not have the nickname flag set");
    if (nickname != speciesName4(0, pk_.language))
        invalid(CheckIdentifier::Egg, "Egg name does not match the egg name for its language");
}

void Analyzer::verifyTrade4Names(const std::u16string& nickname) {
    if (enc_.kind == EncKind::Trade) {
        const auto& t = D::kTrades[enc_.index];
        int lang = pk_.language;
        if (lang == kLangEnglish && (pk_.version == D::kD || pk_.version == D::kP))
            invalid(CheckIdentifier::Trainer, "D/P in-game trades into English games keep language ID 1");
        const std::u16string ot = otText(pk_);
        auto trainerName = [&](int l) -> std::u16string_view {
            return (l >= 0 && l < 9) ? std::u16string_view(D::kTradeStrings[t.otNames + l]) : std::u16string_view();
        };
        auto nickName = [&](int l) -> std::u16string_view {
            return (l >= 0 && l < 9) ? std::u16string_view(D::kTradeStrings[t.nicknames + l]) : std::u16string_view();
        };
        auto detect = [&](int current) {
            for (int l = 1; l < 9; ++l) if (ot == trainerName(l)) return l;
            return current;
        };
        if (t.species == kPikachu) {
            if (lang == kLangFrench) lang = kLangEnglish;
            else {
                lang = detect(lang);
                if (lang == kLangEnglish) lang = nickname == nickName(kLangItalian) ? kLangItalian : kLangSpanish;
            }
        } else if (t.species == 129) {
            if (lang == kLangEnglish) lang = 5;
            else {
                lang = detect(lang);
                if (lang == kLangEnglish) lang = nickname == nickName(kLangFrench) ? kLangFrench : kLangSpanish;
            }
        } else if (lang == kLangJapanese && (pk_.version == D::kD || pk_.version == D::kP)) {
            if (ot != trainerName(kLangJapanese)) lang = kLangEnglish;
        }
        if (ot != trainerName(lang) && !gtsSanitized_)
            invalid(CheckIdentifier::Trainer, "In-game trade OT name was changed");
        if (nickname != nickName(lang))
            invalid(CheckIdentifier::Trainer, "In-game trade nickname was changed");
        return;
    }
    // My Pokemon Ranch: fixed trainer, language as stored.
    static constexpr std::u16string_view kRanchOT[9] = {u"", u"ユカリ", u"Hayley", u"EULALIE", u"GIULIA", u"EUKALIA", u"", u"Eulalia", u""};
    const std::u16string ot = otText(pk_);
    const std::u16string_view expect = pk_.language < 9 ? kRanchOT[pk_.language] : std::u16string_view();
    if (ot != expect && !gtsSanitized_)
        invalid(CheckIdentifier::Trainer, "My Pokemon Ranch OT name was changed");
}

void Analyzer::verifyNickname() {
    const std::u16string nickname = nicknameText(pk_);
    if (format5()) {
        // NicknameVerifier for a PK5: only the parts that need the encounter (a Mystery Gift that cannot
        // be nicknamed, IFixedTrainer / IFixedNickname templates). The Gen V analyzer checks the rest
        // (species names in the Gen V and Gen IV forms, length, word filter) with the Origin5 facts.
        if (nickname.empty()) return; // reported by the Gen V analyzer
        if (pk_.isNicknamed && enc_.kind == EncKind::Pcd && !enc_.isEgg && !enc_.gift.pk.isNicknamed)
            invalid(CheckIdentifier::EventGift, "This Mystery Gift Pokemon cannot be nicknamed");
        if (enc_.kind == EncKind::Trade || enc_.kind == EncKind::Ranch) {
            verifyTrade4Names(nickname);
            return;
        }
        const bool fixedTrainer3 = enc_.kind == EncKind::Trade3 || enc_.kind == EncKind::Gift3 || enc_.kind == EncKind::GiftNY3 ||
                                   enc_.kind == EncKind::GiftJP3 || enc_.kind == EncKind::GiftColo3;
        if (fixedTrainer3) {
            if (isFixedTrainer3(enc_) && !isTrainerMatch3(enc_, pk_, otText(pk_), pk_.language) && !gtsSanitized_)
                invalid(CheckIdentifier::Trainer, "In-game trade or gift OT name was changed");
            if (enc_.kind == EncKind::Trade3 && !isNicknameMatch3(enc_, pk_, nickname, pk_.language))
                invalid(CheckIdentifier::Trainer, "In-game trade nickname was changed");
        }
        return;
    }
    if (nickname.empty()) { invalid(CheckIdentifier::Trainer, "Nickname is empty"); return; }
    if (nickname.find(u'￿') != std::u16string::npos) {
        invalid(CheckIdentifier::Trainer, "Nickname contains a character the Gen IV games cannot display");
        return;
    }
    if (pk_.isNicknamed && isMysteryGift() && !enc_.isEgg && enc_.kind == EncKind::Pcd && !enc_.gift.pk.isNicknamed)
        invalid(CheckIdentifier::EventGift, "This Mystery Gift Pokemon cannot be nicknamed");
    if (enc_.kind == EncKind::Trade || enc_.kind == EncKind::Ranch) {
        verifyTrade4Names(nickname);
        if (enc_.kind == EncKind::Trade) return;
    }
    // Generation III IFixedTrainer templates (trades, event gifts, PCNY/PCJP, Colosseum bonus discs): VerifyTrade.
    const bool fixedTrainer3 = enc_.kind == EncKind::Trade3 || enc_.kind == EncKind::Gift3 || enc_.kind == EncKind::GiftNY3 ||
                               enc_.kind == EncKind::GiftJP3 || enc_.kind == EncKind::GiftColo3;
    if (fixedTrainer3) {
        if (isFixedTrainer3(enc_) && !isTrainerMatch3(enc_, pk_, otText(pk_), pk_.language) && !gtsSanitized_)
            invalid(CheckIdentifier::Trainer, "In-game trade or gift OT name was changed");
        if (enc_.kind == EncKind::Trade3) { // IFixedNickname
            if (!isNicknameMatch3(enc_, pk_, nickname, pk_.language))
                invalid(CheckIdentifier::Trainer, "In-game trade nickname was changed");
            return;
        }
    }
    // A Colosseum/XD IFixedNickname trade (Hordel's Elekid): the GameCube judge checked the nickname.
    if (enc_.kind == EncKind::Cxd3 && enc_.fixedNickname) return;
    if (pk_.isEgg) { verifyNicknameEgg(nickname); return; }
    if (pk_.isNicknamed) {
        uint16_t species = 0;
        if (tryGetSpeciesAnyLanguage4(nickname, species)) {
            fishy(CheckIdentifier::Trainer, "Nickname is a species name");
            return;
        }
        if (hasEastAsianScript(nickname)) {
            invalid(CheckIdentifier::Trainer, "Nickname contains CJK ideographs the Gen IV games cannot enter");
            return;
        }
        const std::size_t max = (pk_.language == kLangJapanese || pk_.language == kLangKorean) ? 5 : 10;
        if (nickname.size() > max) {
            // GetForeignNicknameLength: only a traded egg from Generations 4-7.
            const std::size_t foreign = (encGeneration() == 4 && enc_.isEgg && wasTradedEgg()) ? 10 : 0;
            if (foreign != 0 && nickname.size() <= foreign) fishy(CheckIdentifier::Trainer, "Nickname is longer than its language allows");
            else invalid(CheckIdentifier::Trainer, "Nickname is longer than its language allows");
            return;
        }
    } else {
        bool valid = false;
        if (gtsSanitized_) {
            if (nickname == speciesName4(pk_.species, kLangEnglish)) valid = true;
            else if (history_.size() == 1) {
                invalid(CheckIdentifier::Trainer, "Nickname does not match the species name for its language");
                return;
            }
        }
        if (!valid && nickname == speciesName4(pk_.species, pk_.language)) valid = true;
        const bool anyLanguage = enc_.species != pk_.species || wasTradedEgg();
        if (!valid && anyLanguage && !isNicknamedAnyLanguage4(pk_.species, nickname)) valid = true;
        if (!valid) invalid(CheckIdentifier::Trainer, "Nickname does not match the species name for its language");
    }
    if (pk_.isNicknamed) {
        // WordFilter.IsFiltered(nickname, Gen4, enc.Context); ContainsTooManyNumbers has no limit for Generation III.
        if (isWordFiltered(nickname, encContextGeneration()))
            invalid(CheckIdentifier::Trainer, "Nickname is blocked by the Nintendo word filter");
        if (encGeneration() > 3 && nickname.size() > 4 && numberCount(nickname) > 4)
            invalid(CheckIdentifier::Trainer, "Nickname has more than 4 digits");
    }
}

// ----------------------------------------------------------------------- trainer
void Analyzer::verifyTrainerName() {
    if (!isPlayerOT()) return;
    const std::u16string ot = otText(pk_);
    if (ot.empty()) { invalid(CheckIdentifier::Trainer, "OT name is empty"); return; }
    if (ot.find(u'￿') != std::u16string::npos) {
        invalid(CheckIdentifier::Trainer, "OT name contains a character the Gen IV games cannot display");
        return;
    }
    auto fold = [](char16_t c) -> char16_t {
        if (c >= u'a' && c <= u'z') return static_cast<char16_t>(c - 32);
        if (c >= u'ａ' && c <= u'ｚ') return static_cast<char16_t>(c - 32);
        return c;
    };
    for (std::u16string_view s : {std::u16string_view(u"PKHEX"), std::u16string_view(u"ＰＫＨＥＸ")}) {
        if (ot.size() < s.size()) continue;
        bool same = true;
        for (std::size_t i = 0; i < s.size(); ++i) if (fold(ot[i]) != s[i]) { same = false; break; }
        if (same) { fishy(CheckIdentifier::Trainer, "OT name is suspicious"); break; }
    }
    const std::size_t max = (pk_.language == kLangJapanese || pk_.language == kLangKorean) ? 6 : 12;
    if (ot.size() > max) {
        // TrainerNameVerifier.IsEdgeCaseLength
        bool edge = false;
        if (enc_.isEgg) {
            if (enc_.kind == EncKind::Gift3 && pk_.isEgg && ot == std::u16string_view(D::kGift3Strings[D::kGifts3[enc_.index].otName]))
                edge = true; // fixed-OT Mystery Gift egg
            else
                edge = (pk_.isEgg ? pk_.metLocation() == tradedEggLocation() : wasTradedEgg()) && ot.size() <= 12;
        }
        if (!edge && !gtsSanitized_) invalid(CheckIdentifier::Trainer, "OT name is longer than its language allows");
    }
    if (isWordFiltered(ot, encContextGeneration())) invalid(CheckIdentifier::Trainer, "OT name is blocked by the Nintendo word filter");
    if (encGeneration() > 3 && ot.size() > 4 && numberCount(ot) > 4) invalid(CheckIdentifier::Trainer, "OT name has more than 4 digits");
}

void Analyzer::verifyTrainerId() {
    if (!isPlayerOT()) return;
    if (pk_.version == kVerR || pk_.version == kVerS) {
        // MethodH.TryGetSeedTrainerID: Ruby/Sapphire derive the TID/SID pair from one LCRNG seed. A hatched R/S
        // egg may carry an Emerald trainer's IDs without the version changing (Fishy).
        uint32_t seeds[kSeedBuffer];
        if (lcrng::getSeeds(seeds, static_cast<uint32_t>(pk_.sid) << 16, static_cast<uint32_t>(pk_.tid) << 16) == 0) {
            const char* text = "Trainer ID/Secret ID pair cannot be generated by Ruby/Sapphire";
            if (enc_.kind == EncKind::Egg3 && !pk_.isEgg) fishy(CheckIdentifier::Trainer, text);
            else invalid(CheckIdentifier::Trainer, text);
        }
    }
    if (format5()) return; // the trainer ID heuristics below are PKHeX Fishy results only
    if (pk_.tid == 0 && pk_.sid == 0) fishy(CheckIdentifier::Trainer, "Trainer ID and Secret ID are both zero");
    else if (pk_.tid == pk_.sid) fishy(CheckIdentifier::Trainer, "Trainer ID equals Secret ID");
    else if (pk_.tid == 0) fishy(CheckIdentifier::Trainer, "Trainer ID is zero");
    else if (pk_.sid == 0) fishy(CheckIdentifier::Trainer, "Secret ID is zero");
    else if ((pk_.tid == 12345 && pk_.sid == 54321) || (pk_.tid == 15040 && pk_.sid == 18831))
        fishy(CheckIdentifier::Trainer, "Trainer ID/Secret ID pair is suspicious");
}

// ----------------------------------------------------------------------- IVs / EVs / level
void Analyzer::verifyIvs() {
    if (enc_.kind == EncKind::Pcd && (enc_.gift.pk.iv32 & 0x3FFFFFFFu) != 0) {
        if (enc_.gift.pk.ivs != pk_.ivs) invalid(CheckIdentifier::EventGift, "IVs do not match the Mystery Gift's fixed IVs");
    }
    if (format5()) return; // equal IVs: a PKHeX Fishy heuristic
    const uint8_t hp = pk_.ivs[0];
    bool same = true;
    for (std::size_t i = 1; i < 6; ++i) if (pk_.ivs[i] != hp) same = false;
    if (hp < 30 && same) fishy(CheckIdentifier::Stats, "All IVs are equal");
}

void Analyzer::verifyEvs() {
    int sum = 0;
    for (uint8_t ev : pk_.evs) sum += ev;
    if (pk_.isEgg) {
        if (sum != 0) invalid(CheckIdentifier::Stats, "Eggs cannot have EVs");
        return;
    }
    if (sum > 510) { invalid(CheckIdentifier::Stats, "EV total is above 510"); return; }
    bool vitamins = true;
    for (uint8_t ev : pk_.evs) if (ev != 0 && (ev > 100 || ev % 10 != 0)) vitamins = false;
    if (!vitamins && !cxdUndecided_) { // the encounter level is unknown for an unverified Colosseum/XD origin
        if (enc_.levelMin == 100) {
            invalid(CheckIdentifier::Stats, "EVs above vitamin limits cannot be gained by a level 100 encounter");
        } else {
            uint8_t met = encGeneration() == 4 ? pk_.metLevel : enc_.levelMin;
            if (met < enc_.levelMin) met = enc_.levelMin;
            const uint8_t growth = personalAt(Table::HGSS, enc_.species <= kMaxSpecies4 ? enc_.species : 0).expGrowth();
            if (expForLevel(met, growth) == pk_.exp)
                invalid(CheckIdentifier::Stats, "EVs above vitamin limits need battles, but the Pokemon has gained no EXP");
        }
    }
    bool allSame = true;
    for (std::size_t i = 1; i < 6; ++i) if (pk_.evs[i] != pk_.evs[0]) allSame = false;
    // PKHeX grades the first two Fishy (EffortValueVerifier.cs:50-53); both come from normal play,
    // so they are Info here. EXP with no EVs: Rare Candy and the Day Care add EXP and no EVs
    // (pokeplatinum src/item_use_pokemon.c:291-293, src/overlay005/daycare.c:221-224;
    // pokeheartgold src/use_item_on_mon.c:294-296, src/get_egg.c:147-150). A 508 total: battle
    // EV gain stops only at 510 (pokeplatinum src/battle/battle_script.c:10237,
    // include/constants/pokemon.h:15-16; pokeheartgold include/constants/pokemon.h:9).
    if (sum == 0 && !isWithinEncounterRange()) note(CheckIdentifier::Stats, "Gained levels but has no EVs");
    else if (sum == 508) note(CheckIdentifier::Stats, "EV total is 508 (2 below the cap)");
    else if (pk_.evs[0] != 0 && allSame) fishy(CheckIdentifier::Stats, "All EVs are equal");
}

void Analyzer::verifyLevel() {
    if (format5()) {
        // LevelVerifier for a PK5: the met level is the transfer level (HasOriginalMetLocation is false);
        // the Gen V analyzer checks EXP, current and met level.
        if (isMysteryGift() && enc_.gift.level() > level_)
            invalid(CheckIdentifier::EventGift, "Level is below the Mystery Gift's level");
        return;
    }
    const uint8_t growth = pi_.expGrowth();
    if (isMysteryGift()) {
        const uint8_t giftLevel = enc_.gift.level();
        const bool pgtManaphy = enc_.kind == EncKind::RangerManaphy && pk_.metLevel == 0;
        if (giftLevel != pk_.metLevel && !pgtManaphy) {
            invalid(CheckIdentifier::EventGift, "Met level does not match the Mystery Gift");
            return;
        }
        if (giftLevel > level_) {
            invalid(CheckIdentifier::EventGift, "Level is below the Mystery Gift's level");
            return;
        }
    }
    if (pk_.isEgg) {
        if (level_ != enc_.levelMin) { invalid(CheckIdentifier::Egg, "Egg level does not match its encounter"); return; }
        if (pk_.exp != expForLevel(enc_.levelMin, growth)) invalid(CheckIdentifier::Egg, "Egg EXP must be the minimum for its level");
        return;
    }
    if (level_ >= 100 && pk_.exp != expForLevel(100, growth))
        invalid(CheckIdentifier::Stats, "EXP is above the level 100 maximum");
    if (level_ < pk_.metLevel) invalid(CheckIdentifier::Stats, "Current level is below the met level");
    else if (!isWithinEncounterRange() && level_ != 100 && pk_.exp == expForLevel(level_, growth))
        // PKHeX: Fishy (LevelVerifier.cs:56-57). A Rare Candy adds exactly the EXP to the next
        // level (pokeplatinum src/item_use_pokemon.c:291-293 with src/pokemon.c:2313-2319;
        // pokeheartgold src/use_item_on_mon.c:294-296), so the threshold is normal play: Info.
        note(CheckIdentifier::Stats, "EXP is exactly at a level threshold");
}

// ----------------------------------------------------------------------- ribbons
void Analyzer::verifyRibbons() {
    int bad = 0;
    auto flag = [&](Rib r) { if (rib(pk_, r)) ++bad; };
    const bool gift = isMysteryGift();
    const Pk4& g = enc_.gift.pk;
    auto compare = [&](Rib r) { if (rib(pk_, r) != rib(g, r)) ++bad; };
    if (pk_.isEgg) {
        flag(kEarth);
        for (Rib r : kEvent3Ribbons) flag(r);
        for (Rib r : kEvent4Ribbons) { if (gift) compare(r); else flag(r); }
        flag(kWinning); flag(kVictory);
        flag(kChampionG3); flag(kArtist); flag(kEffort);
        for (Rib r : kAbilityRibbons) flag(r);
        for (uint8_t b = 4; b <= 6; ++b) for (uint8_t bit = 0; bit < 8; ++bit) if (!(b == 6 && bit >= 4)) flag(Rib{b, bit});
        for (uint8_t b = 8; b <= 10; ++b) for (uint8_t bit = 0; bit < 8; ++bit) if (!(b == 10 && bit >= 4)) flag(Rib{b, bit});
        flag(kFootprint); flag(kRecord); flag(kLegend); flag(kChampionSinnoh);
        for (Rib r : kDailyRibbons) flag(r);
        flag(kGorgeous); flag(kRoyal); flag(kGorgeousRoyal);
    } else {
        const bool gen4 = hasVisitedGen4();
        const bool gen3 = hasVisitedGen3();
        // Event3. RibbonRules.IsEarthRibbonAllowed: Generation III encounters (Colosseum/XD Mt. Battle).
        const bool earthAllowed = encGeneration() == 3;
        if (enc_.kind == EncKind::Gift3) { // IRibbonSetEvent3: only the National Ribbon is set by the template
            if (rib(pk_, kNational) != (D::kGifts3[enc_.index].ribbonNational != 0)) ++bad;
            flag(kCountry); flag(kChampionBattle); flag(kChampionRegional); flag(kChampionNational);
        } else if (gift) {
            if (rib(g, kEarth)) { if (!rib(pk_, kEarth)) ++bad; }
            else flag(kEarth); // Earth needs a Gen III encounter
            for (Rib r : kEvent3Ribbons) compare(r);
        } else if (enc_.kind == EncKind::Cxd3) {
            // RibbonVerifierEvent3 for an Orre template: the Earth Ribbon (Mt. Battle) is allowed for a
            // Generation III encounter; RibbonRules.GetValidRibbonStateNational requires the National
            // Ribbon exactly for a (purified) Shadow Pokemon; the other event ribbons never.
            if (rib(pk_, kNational) != enc_.shadow) ++bad;
            flag(kCountry); flag(kChampionBattle); flag(kChampionRegional); flag(kChampionNational);
        } else {
            if (!earthAllowed) flag(kEarth);
            for (Rib r : kEvent3Ribbons) flag(r); // National: only Gen III Shadow Pokemon
        }
        // Event4
        for (Rib r : kEvent4Ribbons) { if (gift) compare(r); else flag(r); }
        // Unique3 (RibbonRules.IsRibbonValidWinning / IsRibbonValidVictory) and Common3
        if (gen3) {
            const bool frontier3 = isAllowedBattleFrontier(history3_.gen3.front().species);
            if (rib(pk_, kWinning) && !(frontier3 && enc_.levelMin <= 50)) ++bad;
            if (rib(pk_, kVictory) && !frontier3) ++bad;
        } else {
            flag(kWinning); flag(kVictory);
            flag(kChampionG3); flag(kArtist);
        }
        if (rib(pk_, kEffort) && !gen3 && !gen4) ++bad;
        // Unique4
        const Evo head = gen4 ? history_.front() : Evo{};
        const bool frontier = gen4 && isAllowedBattleFrontier(head.species) && !(head.species == kPichu && head.form == 1);
        if (!frontier) for (Rib r : kAbilityRibbons) flag(r);
        // Contest ribbon sets (initial, super/great, hyper/ultra, master): a higher rank needs every lower one.
        auto contestSet = [&](uint8_t b, uint8_t base, bool allowed) {
            const bool initial = rib(pk_, Rib{b, base}), great = rib(pk_, Rib{b, static_cast<uint8_t>(base + 1)});
            const bool ultra = rib(pk_, Rib{b, static_cast<uint8_t>(base + 2)}), master = rib(pk_, Rib{b, static_cast<uint8_t>(base + 3)});
            if (!allowed) {
                bad += initial + great + ultra + master;
                return;
            }
            bool top = master;
            if (ultra) top = true; else if (top) ++bad;
            if (great) top = true; else if (top) ++bad;
            if (top && !initial) ++bad;
        };
        // RibbonRules.IsAllowedContest3: any species with a Generation III history.
        for (uint8_t set = 0; set < 5; ++set)
            contestSet(static_cast<uint8_t>(4 + set / 2), static_cast<uint8_t>((set % 2) * 4), gen3);
        const bool contest4 = gen4 && head.species != 132 && head.species != kUnown && !(head.species == kPichu && head.form == 1);
        for (uint8_t set = 0; set < 5; ++set)
            contestSet(static_cast<uint8_t>(8 + set / 2), static_cast<uint8_t>((set % 2) * 4), contest4);
        // Common4
        if (rib(pk_, kFootprint) && !gen3 && !gen4) ++bad;
        flag(kRecord);
        const bool sinnoh4 = gen4 && !(enc_.kind == EncKind::Static && enc_.species == kPichu && enc_.form == 1);
        if (rib(pk_, kLegend) && !gen4) ++bad;
        if (rib(pk_, kChampionSinnoh) && !sinnoh4) ++bad;
        if (!gen4) for (Rib r : kDailyRibbons) flag(r);
        if (!sinnoh4) {
            flag(kGorgeous); flag(kRoyal); flag(kGorgeousRoyal);
        } else {
            bool pre = false;
            if (!rib(pk_, kRoyal)) pre = false;
            else if (!(pre = rib(pk_, kGorgeous))) ++bad;
            if (!pre && rib(pk_, kGorgeousRoyal)) ++bad;
        }
    }
    if (bad != 0) invalid(CheckIdentifier::Misc, std::to_string(bad) + " ribbon(s) are not obtainable or are missing a prerequisite");
}

// ----------------------------------------------------------------------- ability
void Analyzer::verifyAbility() {
    const uint8_t a1 = pi_.ability1(), a2 = pi_.ability2();
    const int abilIndex = pk_.ability == a1 ? 0 : (pk_.ability == a2 ? 1 : -1);
    if (abilIndex < 0) { invalid(CheckIdentifier::Ability, "Ability is not one of this species' abilities"); return; }
    const int pidIndex = static_cast<int>(pk_.pid & 1);
    auto pidMatch = [&]() {
        if ((pidIndex == 0 ? a1 : a2) != pk_.ability)
            invalid(CheckIdentifier::Ability, "Ability does not match the PID's ability bit");
    };
    if (enc_.kind == EncKind::Pcd) {
        if (enc_.species != pk_.species) pidMatch();
        else if (pk_.ability != enc_.gift.pk.ability) invalid(CheckIdentifier::Ability, "Ability does not match the Mystery Gift");
        return;
    }
    if (enc_.kind == EncKind::RangerManaphy) return;
    // VerifyAbility345: a Gen III origin may keep its Gen III ability without matching the PID (pre-capsule).
    const bool canMismatch = a1 != a2 && encGeneration() == 3 && abilityCanMismatch3();
    const int encAbility = enc_.ability;
    if (encAbility > 0) {
        const int single = encAbility == 1 ? 0 : (encAbility == 2 ? 1 : 2);
        if (enc_.species != pk_.species && !canMismatch) {
            // Evolving in Gen III keeps the ability index, so a Gen III fixed ability may still be held.
            if (encGeneration() == 3 && single == abilIndex) return;
            pidMatch();
            return;
        }
        if (single == abilIndex) return;
        if (pidIndex == single) return;
        if (canMismatch) { pidMatch(); return; }
        invalid(CheckIdentifier::Ability, "Ability does not match this encounter's fixed ability");
        return;
    }
    if (!canMismatch) pidMatch();
}

/// AbilityVerifier.VerifyAbilityPreCapsule / VerifyAbilityGen3Transfer for a Gen III origin in format 4.
bool Analyzer::abilityCanMismatch3() {
    if (pk_.species > 386) return false;              // evolved in Gen IV into a new species
    // EvoChainsAllGens.Gen3[0] is the record's own species whenever it is one of the 386 (every one
    // exists in Gen III), so the Colosseum/XD origin can be judged before its encounter is known.
    if (!cxdUndecided_ && history3_.gen3.empty()) return false; // could not exist in Gen III
    const uint16_t max3 = cxdUndecided_ ? pk_.species : history3_.gen3.front().species;
    const D::Personal3& pers = D::kPersonalE3[max3];
    // Two Gen III abilities: the PID decides, except for Colosseum/XD (AbilityVerifier.cs:273-274),
    // whose ability bit is stored apart from the PID and is what Pal Park reads
    // (pokeplatinum src/main_menu/gba_pokemon.c:759-783, GBA_MON_DATA_ABILITY_NUM).
    if (pers.ability1 != pers.ability2) return pk_.version == kVerCXD;
    if (pk_.species != max3) return false;            // evolved in Gen IV: must match the PID
    if (cxdUndecided_) {
        // Whether it evolved in Gen IV needs the encounter: the Gen III ability is open either way;
        // the Gen IV second ability must match the PID under every history (AbilityMismatch3 or MustMatch).
        return pk_.ability == pers.ability1;
    }
    if (history_.size() > 1) {
        if (pk_.ability == pers.ability1) return true;
        if (pk_.ability == pi_.ability2()) return false;
    }
    if (pk_.ability != pers.ability1)
        invalid(CheckIdentifier::Ability, "Ability must be the species' only Generation III ability");
    return true;
}

/// AbilityVerifier for a PK5 of Generation III/IV origin: PersonalTable.B2W2, AbilityNumber from the
/// hidden-ability flag and PID bit 0 (PKM.PIDAbility outside Generation V), VerifyAbilityMG for a PCD or
/// the Ranger Manaphy, otherwise VerifyAbility345 with VerifyAbilityPreCapsule / VerifyFixedAbility /
/// CheckMatch. An ability outside the species' three (AbilityUnexpected) is reported by the Gen V analyzer.
void Analyzer::verifyAbility5() {
    const Personal5 pi = f5_->personalB2W2(pk_.species, pk_.form);
    if (!pi.valid) return;
    const int abilIndex = pi.indexOfAbility(pk_.ability);
    if (abilIndex < 0) return;
    const int number = pk_.hiddenAbility5 ? 4 : (1 << (pk_.pid & 1));
    const uint8_t gen = encGeneration();
    // PKHeX keeps a species in the Generation III history even when the met level (the level at
    // transfer) is below its evolution level (EvolutionGroup3.Evolve leaves the unreached stage in
    // place), so for a Gen III Pokemon that evolved after the transfer it takes the evolved species'
    // Gen III abilities (VerifyAbilityGen3Transfer) and accepts its only Gen III ability, or any
    // Colosseum/XD ability, without a PID match. The games recalculate the ability from the PID on
    // evolution (pokeplatinum src/evolution.c:436, src/pokemon.c:4746-4764), so such a record is
    // suspicious, but it is reported as a Warning rather than ruled out.
    bool pkhexAccepts = false;
    if (gen3Origin() && pk_.species <= 386 && !h5_.gen3.empty() && h5_.gen3.front().species != pk_.species) {
        const D::Personal3& own = D::kPersonalE3[pk_.species];
        pkhexAccepts = own.ability1 != own.ability2 ? pk_.version == kVerCXD : pk_.ability == own.ability1;
    }
    auto pidMatch = [&]() { // GetPIDAbilityMatch
        if (pi.abilities[static_cast<std::size_t>(number >> 1)] == pk_.ability) return;
        if (pkhexAccepts)
            fishy(CheckIdentifier::Ability, "Ability does not match the PID, which sets the ability when a Pokemon evolves after "
                                            "its transfer; PKHeX accepts it");
        else
            invalid(CheckIdentifier::Ability, "Ability does not match the PID's ability bit");
    };
    auto checkMatch = [&](uint8_t generation, bool mustMatch) { // CheckMatch (format 5)
        if ((generation == 3 || generation == 4) && number == 4) {
            invalid(CheckIdentifier::Ability, "Hidden ability flag is set on a Pokemon from Generation III/IV, which had no hidden abilities");
            return;
        }
        if (mustMatch) pidMatch();
    };
    if (enc_.kind == EncKind::Pcd) { // VerifyAbilityPCD (format 5)
        if (enc_.species != pk_.species) checkMatch(4, true);
        else if (pk_.ability != enc_.gift.pk.ability) invalid(CheckIdentifier::Ability, "Ability does not match the Mystery Gift");
        return;
    }
    if (enc_.kind == EncKind::RangerManaphy) { // PGT: AbilityNumber < 4 below format 6
        if (number == 4) invalid(CheckIdentifier::Ability, "Ability does not match the Mystery Gift");
        return;
    }
    // VerifyAbility345: VerifyAbilityPreCapsule when the current species has two distinct abilities.
    bool canMismatch = false;
    if (pi.abilities[0] != pi.abilities[1] && gen3Origin() && pk_.species <= 386 && !h5_.gen3.empty()) {
        const uint16_t max3 = h5_.gen3.front().species;
        const D::Personal3& pers = D::kPersonalE3[max3];
        // Two Gen III abilities: Colosseum/XD stores the ability bit apart from the PID (AbilityVerifier.cs:273-274).
        if (pers.ability1 != pers.ability2) canMismatch = pk_.version == kVerCXD;
        else if (pk_.species == max3) { // VerifyAbilityGen3Transfer
            const bool evolved45 = h5_.gen4.size() > 1 || h5_.gen5.size() > 1;
            bool decided = false;
            if (evolved45) {
                if (pk_.ability == pers.ability1) { canMismatch = true; decided = true; }
                else if (pk_.ability == pi.abilities[1]) decided = true; // must match
            }
            if (!decided) {
                if (pk_.ability != pers.ability1)
                    invalid(CheckIdentifier::Ability, "Ability must be the species' only Generation III ability");
                canMismatch = true;
            }
        }
    }
    const int encAbility = enc_.ability;
    if (encAbility >= 0) {
        if ((number == 4) != (encAbility == 4)) {
            invalid(CheckIdentifier::Ability, number == 4 ? "Hidden ability flag is set, but this encounter cannot have its hidden ability"
                                                          : "This encounter always has its hidden ability");
            return;
        }
        if (encAbility > 0) { // VerifyFixedAbility (encounter generation 3 or 4)
            const int single = encAbility == 1 ? 0 : (encAbility == 2 ? 1 : 2);
            if (enc_.species != pk_.species && !canMismatch) {
                // Evolving in Gen III keeps the ability index; otherwise it must match the PID.
                if (gen == 3) {
                    if (single == abilIndex) return;
                    pidMatch();
                    return;
                }
                checkMatch(gen, true);
                return;
            }
            if (single == abilIndex) return;
            if (number == 1 << single) return;
            if (canMismatch) { checkMatch(gen, true); return; }
            invalid(CheckIdentifier::Ability, "Ability does not match this encounter's fixed ability");
            return;
        }
    }
    checkMatch(gen, !canMismatch);
}

/// GenderVerifier.IsValidFixedGenderFromBiGender for a PK5 (the species-ratio and PID checks are the
/// Gen V analyzer's): a fixed-gender evolution keeps the gender the PID gave the two-gender encounter species.
void Analyzer::verifyGender5() {
    const uint16_t s = pk_.species;
    if (!(s == kWormadam || s == kMothim || s == kVespiquen || s == kGallade || s == kFroslass)) return;
    if (pk_.gender == 2) return;
    const uint8_t ratio = personalAt(Table::HGSS, enc_.species <= kMaxSpecies4 ? enc_.species : 0).gender();
    if (genderFromPid(pk_.pid, ratio) != pk_.gender)
        invalid(CheckIdentifier::PidRng, "Gender does not match the PID of the two-gender species it was obtained as");
}

// ----------------------------------------------------------------------- ball
void Analyzer::verifyBall() {
    const uint8_t ball = pk_.ball();
    auto bad = [&]() { invalid(CheckIdentifier::Items, "Ball (#" + std::to_string(ball) + ") is not possible for this encounter"); };
    // Gen III evolution copies the ball; otherwise Shedinja gets a Poke Ball.
    const bool shedinja3 = enc_.kind == EncKind::Slot3 && history3_.gen3.size() == 2;
    if (enc_.species == kNincada && pk_.species == kShedinja && !shedinja3) {
        const bool bcc = enc_.kind == EncKind::Slot && enc_.area->type == SlotRef::kBugContest && history_.size() == 2;
        if (bcc ? !(ball == kBallSport || ball == kBallPoke) : ball != kBallPoke) bad();
        return;
    }
    if (enc_.kind == EncKind::Invalid) return;
    if (enc_.fixedBall != 0) { if (ball != enc_.fixedBall) bad(); return; }
    if (enc_.kind == EncKind::Egg || enc_.kind == EncKind::Egg3) { if (ball != kBallPoke) bad(); return; }
    // BallUseLegality.GetWildBalls(enc.Generation, enc.Version)
    const uint8_t v = enc_.version;
    const bool gen3 = encGeneration() == 3;
    const bool hgss = !gen3 && (v == D::kHG || v == D::kSS || v == D::kVerHGSS);
    uint64_t mask = 0;
    for (uint8_t b : {1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12}) mask |= uint64_t{1} << b;
    if (!gen3) for (uint8_t b : {13, 14, 15}) mask |= uint64_t{1} << b;
    if (hgss) for (uint8_t b = 17; b <= 23; ++b) mask |= uint64_t{1} << b;
    if (ball >= 64 || ((mask >> ball) & 1) == 0) bad();
}

// ----------------------------------------------------------------------- form
void Analyzer::verifyForm() {
    const uint8_t count = pi_.formCount();
    const uint8_t form = pk_.form;
    const uint16_t species = pk_.species;
    if (count <= 1 && form == 0) return;
    const uint8_t gen = encGeneration();
    const bool inRange = form == 0 || form < count;
    const bool oob = (species == kUnown && form < (gen == 2 ? 26 : 28)) || (species == kMothim && form < 3);
    if (!inRange && !oob) { invalid(CheckIdentifier::Species, "Form " + std::to_string(form) + " is out of range"); return; }
    if (species == kUnown && gen == 3) { // EntityPID.GetUnownForm3: Gen III Unown take their letter from the PID
        if (form != unownForm3(pk_.pid)) invalid(CheckIdentifier::Species, "Unown's form does not match its PID");
        return;
    }
    if (species == kGiratina && ((form == 1) != (pk_.heldItem == kItemGriseousOrb))) {
        invalid(CheckIdentifier::Species, "Giratina's form must match whether it holds the Griseous Orb");
        return;
    }
    if (species == kArceus) {
        if (arceusForm4(pk_.heldItem) != form) invalid(CheckIdentifier::Species, "Arceus' form must match its held plate");
        return;
    }
    if (species == kRotom && pk_.isEgg && form != 0) { invalid(CheckIdentifier::Egg, "Rotom eggs cannot have a form"); return; }
    // FormVerifier.cs:150-154 (Shaymin: FormParty unless the slot is a party slot). A 136-byte
    // record is a stored (PC) record. Platinum reverts Shaymin to Land Forme in every PC store
    // function (pokeplatinum src/pc_boxes.c:59-110; the box UI stores through them,
    // src/applications/pc_boxes/box_app_manager.c:3952-4017), in the Day Care (src/overlay005/daycare.c:112),
    // at the GTS deposit (src/gts_application/screens/deposit.c:994-998) and in the trade room
    // (src/applications/trade_room/application.c:623). HG/SS revert only party Shaymin (pokeheartgold
    // src/pokemon.c:3574-3610, src/get_egg.c:93); their PC store functions
    // (src/pokemon_storage_system.c:54-100) leave the form alone and their box UI is not in the
    // decomp. Every game runs the Wii link (Ranch, Battle Revolution) from data/eoo.dat
    // (pokeplatinum src/main_menu/main_menu.c:1317, pokeheartgold src/application/main_menu/
    // main_menu.c:1503), a module outside every decomp, so a stored Sky Forme is a Warning, not Invalid.
    if (species == kShaymin && form != 0 && !pk_.party && !pk_.isEgg) {
        fishy(CheckIdentifier::Species, cart_ == Table::Pt && cartKnown_
            ? "Shaymin's Sky Forme reverts to Land Forme when Platinum stores it in the PC"
            : "Shaymin's Sky Forme in a PC-stored record cannot be verified for this game");
    }
    const bool battle = ((species == kCastform || species == 382 || species == 383 || species == kCherrim) && form != 0) ||
                        (isMegaSpecies4(species) && form != 0);
    if (battle) invalid(CheckIdentifier::Species, "Battle-only form cannot exist outside battle");
}

// ----------------------------------------------------------------------- misc
void Analyzer::verifyMisc() {
    if (pk_.isEgg) {
        if (pk_.friendship > piEntity_.hatchCycles()) invalid(CheckIdentifier::Egg, "Egg hatch counter is above the species' maximum");
        bool contest = false;
        for (uint8_t c : pk_.contest) if (c != 0) contest = true;
        if (contest) invalid(CheckIdentifier::Egg, "Eggs cannot have contest stats");
        // EggVerifier: an egg cannot change context (a Gen III egg must hatch before Pal Park).
        if (isGen3Kind(enc_.kind)) invalid(CheckIdentifier::Egg, "Generation III eggs cannot be transferred to Generation IV");
    }
    // MiscVerifierG4
    if (pk_.walkingMood != 0 && pk_.isEgg) fishy(CheckIdentifier::Egg, "Eggs do not have a walking mood");
    // MiscVerifierG4.cs:115-134 (G4PartnerMoodZero, Fishy, only for a non-party slot of an HG/SS
    // save): HG/SS zero the mood when a Pokemon becomes the walking lead (pokeheartgold
    // src/follow_mon.c:1946-1992) and when it enters the Day Care (src/get_egg.c:91); D/P/Pt never
    // touch the byte and trades copy it, so only a stored record of a known HG/SS save is checked.
    if (pk_.walkingMood != 0 && !pk_.isEgg && !pk_.party && cartKnown_ && cart_ == Table::HGSS)
        fishy(CheckIdentifier::Misc, "Walking mood of a Pokemon stored in the PC of HeartGold/SoulSilver should be zero");
    const uint8_t leaf = pk_.shinyLeaf;
    if ((leaf > 0x1F && leaf != 0x3F) || (leaf != 0 && pk_.isEgg)) invalid(CheckIdentifier::Misc, "Shiny Leaf value is not obtainable");
    // Pal Park: D/P leave the Pt/HG/SS met location unset; Pt/HG/SS write it equal to the D/P value.
    const bool palPark = encGeneration() == 3;
    const bool palParkPtHGSS = palPark && pk_.metLocationExtended != 0;
    if (palParkPtHGSS && pk_.metLocationExtended != pk_.metLocationDP)
        invalid(CheckIdentifier::Transfer, "Pal Park met location differs between the D/P and Pt/HG/SS fields");
    // Pt/HG/SS write both met-location fields through one setter: a place D/P know goes into both,
    // any other place puts Faraway Place (3002) in the D/P field (pokeplatinum src/pokemon.c:1732-1740
    // with src/unk_02017038.c:38-45; pokeheartgold src/pokemon.c:1286-1292). D/P never write the
    // Pt/HG/SS field, and a non-egg Pokemon's met location is never rewritten; only hatching in D/P
    // rewrites the D/P field alone, so egg-origin records are left out. Every PKHeX Gen IV event
    // template leaves the Pt/HG/SS field at 0, so a gift received in D/P is skipped too. My Pokemon
    // Ranch records are written by the Wii software, which has no decomp, so they only get a Warning.
    // PKHeX does not compare the two fields (deliberate difference, PKHEX_DISAGREEMENT_REVIEW.md item 14).
    // A PK5 has one met-location field: the format-5 view never reaches this rule (run5 does not call
    // verifyMisc, and the view's Pt/HG/SS fields are 0), and the guard keeps it that way.
    if (!format5() && !palPark && pk_.metLocationExtended != 0 && pk_.eggLocationDP == 0 && pk_.eggLocationExtended == 0) {
        const uint16_t ext = pk_.metLocationExtended;
        const bool knownToDP = ext <= 111 || (ext >= 2000 && ext <= 2010) || (ext >= 3000 && ext <= 3076);
        const bool ptHGSSOnly = (ext >= 112 && ext <= 1999) || (ext >= 2011 && ext <= 2999);
        if ((knownToDP || ptHGSSOnly) && pk_.metLocationDP != (knownToDP ? ext : uint16_t{3002})) {
            std::string text = "D/P met location field holds " + std::to_string(pk_.metLocationDP) +
                               " where Pt/HG/SS write " + std::to_string(knownToDP ? ext : 3002) +
                               " for met location " + std::to_string(ext);
            if (enc_.kind == EncKind::Ranch) fishy(CheckIdentifier::Encounter, std::move(text));
            else invalid(CheckIdentifier::Encounter, std::move(text));
        }
    }
    if (pk_.ballDPPt > kBallCherish) invalid(CheckIdentifier::Items, "D/P/Pt ball value is above the Cherish Ball");
    {
        // MiscVerifierG4.IsValidBallHGSS: HG/SS-created records (including Pal Park into Pt/HG/SS) set this byte.
        const bool createdHGSS = isHGSS(pk_.version) ? enc_.kind != EncKind::Pcd : palParkPtHGSS;
        bool ok;
        if (!createdHGSS) ok = pk_.ballHGSS == 0;
        else if (palParkPtHGSS && pk_.ballHGSS == 0) ok = true; // could be a Platinum transfer
        else ok = pk_.ballHGSS == pk_.ballDPPt || (pk_.ballDPPt == kBallPoke && pk_.ballHGSS > kBallCherish);
        if (!ok) invalid(CheckIdentifier::Items, "HG/SS ball value does not match the D/P/Pt ball value");
    }
    verifyFateful();
    // Pokerus
    const int strain = pk_.pokerus >> 4, days = pk_.pokerus & 0xF;
    if (days > (strain & 3) + 1) invalid(CheckIdentifier::Misc, "Pokerus days remaining is above the strain's maximum");
    // Dates
    if (pk_.metLocation() != 0) {
        if (!isValidDate(pk_.metDate)) invalid(CheckIdentifier::Misc, "Met date is not a valid date");
    } else if (enc_.kind != EncKind::Invalid && !isZero(pk_.metDate)) {
        invalid(CheckIdentifier::Misc, "Met date is set without a met location");
    }
    if (pk_.eggLocation() != 0) {
        if (!isValidDate(pk_.eggDate)) invalid(CheckIdentifier::Misc, "Egg date is not a valid date");
    } else if (!isZero(pk_.eggDate)) {
        invalid(CheckIdentifier::Misc, "Egg date is set without an egg location");
    }
}

/// MiscFatefulEncounterVerifier (format independent for Generation III/IV encounters).
void Analyzer::verifyFateful() {
    if (cxdUndecided_) return; // XD templates set the flag, Colosseum ones do not: needs the encounter
    if (enc_.kind == EncKind::Gift3) {
        const auto& g = D::kGifts3[enc_.index];
        if (g.fateful != 0) {
            if (!g.isEgg && !pk_.fateful) invalid(CheckIdentifier::EventGift, "Fateful encounter flag is missing");
        } else if (pk_.fateful) {
            invalid(CheckIdentifier::EventGift, "Fateful encounter flag is set on a non-event encounter");
        }
    } else if (isMysteryGift()) {
        const bool giftFateful = enc_.kind == EncKind::Pcd ? enc_.gift.pk.fateful : enc_.gift.fateful();
        if (enc_.kind == EncKind::Pcd && enc_.gift.version() != pk_.version && enc_.gift.pk.version == 0)
            invalid(CheckIdentifier::EventGift, "This Mystery Gift was not distributed to the origin game");
        if (pk_.fateful != giftFateful)
            invalid(CheckIdentifier::EventGift, giftFateful ? "Fateful encounter flag is missing" : "Fateful encounter flag is set");
    } else if ((enc_.kind == EncKind::Static || enc_.kind == EncKind::Ranch || enc_.kind == EncKind::Static3 ||
                enc_.kind == EncKind::Cxd3) && enc_.fateful) {
        if (!pk_.fateful) invalid(CheckIdentifier::EventGift, "Fateful encounter flag is missing");
    } else if (pk_.fateful) {
        invalid(CheckIdentifier::EventGift, "Fateful encounter flag is set on a non-event encounter");
    }
}

// ----------------------------------------------------------------------- PP, gender, item, contest, markings, tile
void Analyzer::verifyMovePP() {
    auto basePP = [](uint16_t move) -> int { return move < D::kMoveCount ? D::kMovePP[move] : 0; };
    if (pk_.isEgg) {
        bool ups = false, pp = false;
        for (std::size_t i = 0; i < 4; ++i) {
            if (pk_.ppUps[i] != 0) ups = true;
            if (pk_.pp[i] != basePP(pk_.moves[i])) pp = true;
        }
        if (ups) invalid(CheckIdentifier::Egg, "Eggs cannot have PP Ups applied");
        if (pp) invalid(CheckIdentifier::Egg, "Egg move PP must be the base PP");
        return;
    }
    for (std::size_t i = 0; i < 4; ++i) {
        const uint16_t move = pk_.moves[i];
        if (pk_.ppUps[i] != 0 && (move == 0 || move == kMoveSketch))
            invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
        const int healed = basePP(move) * (5 + pk_.ppUps[i]) / 5;
        if (pk_.pp[i] > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
        // MovePPVerifier.cs:110-143 (GetPermittedStatePP/GetIsStoredHealed: a PK4 in a Box, GTS or
        // Battle Box slot is OnlyHealed). Every PC store function heals PP: pokeplatinum
        // src/pc_boxes.c:59-110 (callers: box UI src/applications/pc_boxes/box_app_manager.c:3952-4017,
        // GTS src/gts_application/screens/network_handler.c:1284-1328, catching show
        // src/scrcmd_catching_show.c:70, battle capture src/battle/battle_script.c:10748),
        // pokeheartgold src/pokemon_storage_system.c:54-100 (src/overlay_bug_contest.c:227,
        // src/scrcmd_12.c:68, src/battle/battle_command.c:7025), pokediamond
        // arm9/src/pokemon_storage_system.c:46-90 (arm9/src/scrcmd_13.c:73). The D/P and HG/SS box UIs
        // and the Wii-link module data/eoo.dat are outside the decomps, so a 136-byte (stored) record
        // below full PP is a Warning rather than PKHeX's Invalid.
        else if (!pk_.party && move != 0 && pk_.pp[i] < healed)
            fishy(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is below its maximum for a Pokemon stored in the PC");
    }
}

void Analyzer::verifyGender() {
    const uint8_t ratio = pi_.gender();
    const uint8_t gender = pk_.gender;
    if ((ratio == 255) != (gender == 2)) {
        const bool ignore = pk_.species == kShedinja && pk_.metLevel != level_;
        if (!ignore) invalid(CheckIdentifier::Stats, "Gender does not match the species' gender ratio");
        return;
    }
    const uint8_t gen = encGeneration();
    if (gen == 3 || gen == 4) {
        bool valid;
        if (ratio == 255) valid = gender == 2;
        else if (ratio == 254) valid = gender == 1;
        else if (ratio == 0) valid = gender == 0;
        else valid = gender < 2 && gender == genderFromPid(pk_.pid, ratio);
        if (!valid) {
            valid = pk_.species == kShedinja && gender == genderFromPid(pk_.pid, 0x7F);
        } else {
            const uint16_t s = pk_.species;
            if (s == kShedinja || s == kWormadam || s == kMothim || s == kVespiquen || s == kGallade || s == kFroslass) {
                if (gender != 2) {
                    const uint8_t r = personalAt(Table::HGSS, enc_.species <= kMaxSpecies4 ? enc_.species : 0).gender();
                    valid = genderFromPid(pk_.pid, r) == gender;
                }
            }
        }
        if (!valid) invalid(CheckIdentifier::PidRng, "Gender does not match the PID");
        return;
    }
    if ((ratio == 254 && gender != 1) || (ratio == 0 && gender != 0))
        invalid(CheckIdentifier::Stats, "Gender does not match the species' gender ratio");
}

void Analyzer::verifyItem() {
    const uint16_t item = pk_.heldItem;
    if (pk_.isEgg && item != 0) invalid(CheckIdentifier::Items, "Eggs cannot hold items");
    if (item != 0 && !std::binary_search(D::kHeldItems, D::kHeldItems + D::kHeldItemCount, item))
        invalid(CheckIdentifier::Items, "Held item #" + std::to_string(item) + " cannot be held in Generation IV");
}

void Analyzer::verifyContest() {
    std::array<uint8_t, 6> initial{};
    // ContestStatInfo.GetReferenceTemplate: in-game trades (IContestStatsReadOnly) carry contest stats.
    const bool trade = enc_.kind == EncKind::Trade || enc_.kind == EncKind::Trade3;
    if (enc_.kind == EncKind::Trade) for (std::size_t i = 0; i < 5; ++i) initial[i] = D::kTrades[enc_.index].contest;
    if (enc_.kind == EncKind::Trade3) for (std::size_t i = 0; i < 6; ++i) initial[i] = D::kTrades3[enc_.index].contest[i];
    bool any = false;
    for (uint8_t c : pk_.contest) if (c != 0) any = true;
    if (!any || (trade && pk_.contest == initial)) return;
    // Cool Beauty Cute Smart Tough -> amp indexes 0 (Atk) 2 (SpA) 4 (Spe) 3 (SpD) 1 (Def)
    static constexpr int kAmpIndex[5] = {0, 2, 4, 3, 1};
    auto gainedSum = [&](uint8_t nature) {
        int sum = 0;
        for (std::size_t i = 0; i < 5; ++i) {
            const int gain = pk_.contest[i] - initial[i];
            if (gain <= 0) continue;
            const int amp = nature < 25 ? kNatureAmp[nature][kAmpIndex[i]] : 0;
            if (amp == 0) { sum += gain; continue; }
            const int factor = amp == 1 ? 11 : 9;
            int boost = gain / factor;
            if (gain % factor >= 5) ++boost;
            sum += gain + (amp == -1 ? boost : -boost);
        }
        return sum;
    };
    auto averageFeel = [&](uint8_t nature) { const int s = gainedSum(nature); return (s + 4) / 5; };
    const uint8_t nature = pk_.nature();
    const uint8_t sheen = pk_.contest[5];
    const bool equal = pk_.contest == initial;
    if (encGeneration() == 3) { // Pokeblocks: ContestStatInfo.CalculateMinimumSheen3 / CalculateMaximumSheen3
        constexpr int kBestSheen3 = 214;
        int minSheen3;
        if (equal) minSheen3 = initial[5];
        else {
            const int raw = averageFeel(0);
            if (raw == 255) minSheen3 = kBestSheen3;
            else {
                int avg = std::max(1, nature % 6 == 0 ? raw : averageFeel(nature));
                avg = std::min(raw, avg);
                avg = kBestSheen3 * avg / 255;
                minSheen3 = std::clamp(avg, 1, kBestSheen3);
            }
        }
        if (sheen < minSheen3) invalid(CheckIdentifier::Stats, "Contest sheen is too low for the contest stats gained");
        int maxSheen3;
        bool anyMax3 = false;
        for (std::size_t i = 0; i < 5; ++i) if (pk_.contest[i] == 255) anyMax3 = true;
        if (anyMax3) maxSheen3 = 255;
        else if (equal) maxSheen3 = initial[5];
        else {
            const int sum = gainedSum(nature);
            if (sum == 0) maxSheen3 = 0;
            else {
                int gained = 0;
                for (std::size_t i = 0; i < 5; ++i) if (pk_.contest[i] - initial[i] >= 2) ++gained;
                const int permit = gained >= 3 ? sum * 83 / 6 : sum * 19 / 9;
                maxSheen3 = std::clamp(permit, 1, 255);
            }
        }
        if (sheen > maxSheen3) invalid(CheckIdentifier::Stats, "Contest sheen is too high for the contest stats gained");
        return;
    }
    // CalculateMinimumSheen4
    int minSheen;
    if (equal) minSheen = initial[5];
    else {
        const int raw = averageFeel(0);
        if (raw == 255) minSheen = 255;
        else {
            int avg = std::max(1, nature % 6 == 0 ? raw : averageFeel(nature));
            avg = std::min(raw, avg);
            minSheen = std::clamp(avg, 11, 255);
        }
    }
    if (sheen < minSheen) invalid(CheckIdentifier::Stats, "Contest sheen is too low for the contest stats gained");
    // CalculateMaximumSheen (Gen IV poffins)
    int maxSheen;
    bool anyMax = false;
    for (std::size_t i = 0; i < 5; ++i) if (pk_.contest[i] == 255) anyMax = true;
    if (anyMax) maxSheen = 255;
    else if (equal) maxSheen = initial[5];
    else {
        const int avg = averageFeel(nature);
        if (avg <= 0) maxSheen = initial[5];
        else if (avg <= 2) maxSheen = 59;
        else maxSheen = std::min(255, avg * 79);
    }
    if (sheen > maxSheen) invalid(CheckIdentifier::Stats, "Contest sheen is too high for the contest stats gained");
}

void Analyzer::verifyMarking() {
    if (pk_.markings > 0x3F) invalid(CheckIdentifier::Misc, "Markings use bits the Gen IV games do not have");
}

void Analyzer::verifyGroundTile() {
    bool hasTile = false;
    uint32_t allow = 0;
    if (enc_.kind == EncKind::Slot) { hasTile = true; allow = enc_.area->groundTile; }
    else if (enc_.kind == EncKind::Static) { hasTile = true; allow = D::kStatics[enc_.index].groundTile; }
    bool valid;
    if (!hasTile || allow == 1) valid = pk_.groundTile == 0;
    else valid = (allow & (uint32_t{1} << (pk_.groundTile & 31))) != 0;
    if (!valid) invalid(CheckIdentifier::Encounter, "Encounter ground tile type does not match the encounter");
}

void Analyzer::verifyMoveResults() {
    for (std::size_t i = 0; i < 4; ++i) {
        if (!moves_[i].valid()) invalid(CheckIdentifier::Moves, moveFailText(i, moves_[i], pk_.moves[i], format5()));
        // Lenient difference from PKHeX (see isTypeTutor): Arceus holding a Draco Plate is a
        // Dragon type for the Pt/HG/SS Draco Meteor tutor, which PKHeX does not accept.
        else if (pk_.species == kArceus && pk_.moves[i] == kMoveDracoMeteor && moves_[i].method == LearnMethod::Tutor)
            fishy(CheckIdentifier::Moves, "Draco Meteor: Arceus can learn it only from the Pt/HG/SS tutor while holding a Draco Plate; PKHeX rejects this move");
    }
}

// ----------------------------------------------------------------------- run
Report Analyzer::run() {
    Report r;
    r.coverage.structure = CoverageLevel::Complete;
    level_ = levelFromExp(pk_.exp, personal(Table::HGSS, pk_.species, pk_.form).expGrowth());
    // PKHeX analyzes a slot of a save with that game's personal table (form count,
    // abilities and gender ratio differ: D/P lack the Platinum forms).
    pi_ = personal(cart_, pk_.species, pk_.form);
    piEntity_ = personal(Table::HGSS, pk_.species, pk_.form);

    findVerifiedEncounter();

    verifyPid();
    verifyLanguage();
    verifyNickname();
    verifyTrainerName();
    verifyTrainerId();
    verifyIvs();
    verifyEvs();
    verifyLevel();
    if (!cxdUndecided_) verifyRibbons(); // the National/Earth ribbons need the Orre encounter
    verifyAbility();
    verifyBall();
    verifyForm();
    verifyMisc();
    verifyMovePP();
    verifyGender();
    verifyItem();
    verifyContest();
    verifyMarking();
    verifyGroundTile();
    if (!cxdUndecided_) verifyMoveResults(); // the moves need the Orre encounter's moveset
    // ParsePK3: TransferVerifier.VerifyTransferLegalityG3 (Pal Park writes met location 55).
    if (gen3Origin() && pk_.metLocation() != kLocTransfer3)
        invalid(CheckIdentifier::Transfer, "A Generation III Pokemon must have been met at Pal Park");

    bool anyInvalid = false;
    for (const Check& c : parse_) if (c.severity == kInvalid) anyInvalid = true;
    // LegalityAnalysis.IsPotentiallyMysteryGift: never for a Gen III origin outside format 3.
    if (anyInvalid && enc_.kind == EncKind::Invalid && pk_.fateful && !gen3Origin())
        invalid(CheckIdentifier::EventGift, "Fateful encounter without a matching Mystery Gift");

    for (const Check& c : parse_) {
        if (c.severity == kValid) continue;
        const Severity sev = c.severity == kInvalid ? Severity::Invalid
                             : c.severity == kNote  ? Severity::Info
                                                    : Severity::Warning;
        bool dup = false;
        for (const Issue& i : r.issues) if (i.severity == sev && i.text == c.message) dup = true;
        if (!dup) r.issues.push_back(Issue{sev, c.message, c.id});
    }
    if (r.problemCount() == 0) {
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Complete;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Complete;
    } else {
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Partial;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Partial;
    }
    return r;
}

Origin5 Analyzer::run5(uint8_t currentLevel) {
    Origin5 o;
    level_ = currentLevel;
    // Generation IV personal entries for the shared helpers; the format-5 ability rules read B2W2's.
    pi_ = personal(Table::HGSS, pk_.species, pk_.form);
    piEntity_ = pi_;

    findVerifiedEncounter();
    // LegalityAnalysis.UpdateChecks for a PK5, the parts that depend on the Generation III/IV
    // encounter and history. Names, item, form, PP, markings, EXP, dates, Pokerus, nature, the
    // species-ratio gender and the transfer location are the Gen V analyzer's.
    verifyPid();
    verifyLanguage();
    verifyNickname();
    verifyTrainerId();
    verifyIvs();
    verifyLevel();
    verifyRibbons();
    verifyAbility5();
    verifyBall();
    verifyFateful();
    verifyGender5();
    verifyContest();
    verifyGroundTile();
    verifyMoveResults();

    bool anyInvalid = false;
    for (const Check& c : parse_) if (c.severity == kInvalid) anyInvalid = true;
    // LegalityAnalysis.IsPotentiallyMysteryGift: a Generation IV EncounterInvalid with the fateful flag.
    if (anyInvalid && enc_.kind == EncKind::Invalid && pk_.fateful && !gen3Origin())
        invalid(CheckIdentifier::EventGift, "Fateful encounter without a matching Mystery Gift");

    o.analyzed = true;
    o.encounterMatched = !search_.list.empty() && enc_.kind != EncKind::Invalid;
    o.allMovesVerified = allValid(moves_);
    for (const Check& c : parse_) if (c.severity != kValid) o.checks.push_back(c);
    o.kind = enc_.kind;
    o.encSpecies = enc_.species;
    o.encGeneration = encGeneration();
    o.encContextGeneration = encContextGeneration();
    o.encIsEgg = enc_.isEgg;
    o.playerOT = isPlayerOT();
    o.fixedNickname = enc_.kind == EncKind::Trade || enc_.kind == EncKind::Trade3 ||
                      (enc_.kind == EncKind::Cxd3 && enc_.fixedNickname);
    o.gtsSanitized = gtsSanitized_;
    o.wasTradedEgg = wasTradedEgg();
    for (std::size_t i = 0; i < 4; ++i) o.moveSource[i] = moves_[i].valid() ? learnMethodText(moves_[i].method) : nullptr;
    return o;
}

MoveEvidence Analyzer::evidence() const noexcept {
    MoveEvidence e;
    e.encounterMatched = encounterSearched_ && !search_.list.empty() && enc_.kind != EncKind::Invalid;
    for (std::size_t i = 0; i < 4; ++i) {
        e.moves[i] = pk_.moves[i];
        e.source[i] = moves_[i].valid() ? learnMethodText(moves_[i].method) : nullptr;
    }
    return e;
}

} // namespace

namespace {
CxdOriginInput cxdInput(const Pk4& pk);
} // namespace

Origin5 analyzeOrigin5(const Pk4& pk, uint8_t currentLevel, const Format5Data& f5) {
    // Poke Transfer refuses eggs; species above 493 are left to the Gen V analyzer, as is any record
    // without a Generation III/IV or Colosseum/XD origin game.
    if (pk.format != 5 || !f5.ready() || pk.isEgg || pk.species == 0 || pk.species > kMaxSpecies4) return {};
    if (pk.version == kVerCXD) {
        // Colosseum/XD: the Gen III GameCube judge on the fields Poke Transfer keeps. Only a decided
        // answer (matched template, or no Orre encounter) is analyzed here; an undecided one stays
        // with the Gen V analyzer, which reports the origin as unverified.
        const CxdOriginJudgement j = judgeCxdOrigin(cxdInput(pk));
        if (j.status != CxdOriginJudgement::Status::Matched && j.status != CxdOriginJudgement::Status::NoMatch) return {};
        Analyzer a(pk, f5, &j);
        return a.run5(currentLevel);
    }
    const bool gen3 = pk.version >= 1 && pk.version <= 5;
    if (!gen3 && !isGen4Version(pk.version)) return {};
    Analyzer a(pk, f5);
    return a.run5(currentLevel);
}

} // namespace Gen4

Report analyzePk4(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId) {
    return analyzePk4(decrypted, exactSourceGameId, nullptr, &Gen4::judgeCxdOrigin);
}

Report analyzePk4(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId,
                  Gen4::MoveEvidence* evidence) {
    return analyzePk4(decrypted, exactSourceGameId, evidence, &Gen4::judgeCxdOrigin);
}

namespace Gen4 {
namespace {
/// The fields a Pal Park transfer keeps from its Colosseum/XD origin, for the GameCube judge.
CxdOriginInput cxdInput(const Pk4& pk) {
    CxdOriginInput in;
    in.species = pk.species; in.form = pk.form;
    in.level = levelFromExp(pk.exp, personal(Table::HGSS, pk.species, pk.form).expGrowth());
    in.metLevel = pk.metLevel; in.ball = pk.ball(); in.fateful = pk.fateful; in.isEgg = pk.isEgg;
    in.isShiny = pk.isShiny(); in.pid = pk.pid; in.ivs = pk.ivs; in.evs = pk.evs; in.exp = pk.exp;
    in.tid = pk.tid; in.sid = pk.sid; in.otGender = pk.otGender; in.language = pk.language;
    in.ability = pk.ability; in.heldItem = pk.heldItem;
    in.otName = otText(pk); in.nickname = nicknameText(pk); in.isNicknamed = pk.isNicknamed;
    in.moves = pk.moves; in.ribbons = pk.ribbons;
    in.ribbonNational = rib(pk, kNational); in.ribbonEarth = rib(pk, kEarth);
    return in;
}
} // namespace
} // namespace Gen4

Report analyzePk4(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId,
                  Gen4::MoveEvidence* evidence, Gen4::CxdOriginJudge judge) {
    if (evidence != nullptr) *evidence = Gen4::MoveEvidence{};
    Report r;
    r.coverage.structure = CoverageLevel::Complete;
    Gen4::Pk4 pk;
    if (!Gen4::read(decrypted, pk)) {
        r.coverage.structure = CoverageLevel::None;
        r.issues.push_back(Issue{Severity::Invalid, "PK4 record must be 136 or 236 bytes", CheckIdentifier::Structure});
        return r;
    }
    bool empty = true;
    for (uint8_t b : decrypted.first(136)) if (b != 0) { empty = false; break; }
    if (empty || pk.species == 0) return r;
    if (Gen4::checksum(decrypted) != pk.checksum || pk.sanity != 0) {
        // The games treat such a record as a Bad Egg, but PKHeX's legality analysis does not
        // check the checksum, so this stays a Warning (lenient) and the record is not certified.
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Partial;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Partial;
        r.issues.push_back(Issue{Severity::Warning,
                                 "PK4 checksum or sanity word does not match its data (the games show it as a Bad Egg)",
                                 CheckIdentifier::Checksum});
        return r;
    }
    if (pk.species > Gen4::kMaxSpecies4) {
        r.issues.push_back(Issue{Severity::Invalid, "Species #" + std::to_string(pk.species) + " does not exist in Generation IV",
                                 CheckIdentifier::Species});
        return r;
    }
    if (!Gen4::isGen4Version(pk.version) && !Gen4::isGen3Version(pk.version)) {
        r.issues.push_back(Issue{Severity::Invalid,
                                 "Origin game #" + std::to_string(pk.version) + " cannot produce a PK4 (not a Gen III or Gen IV game)",
                                 CheckIdentifier::SourceGame});
        return r;
    }
    Gen4::Table cart = Gen4::Table::HGSS;
    if (exactSourceGameId == "diamond_nds" || exactSourceGameId == "pearl_nds") cart = Gen4::Table::DP;
    else if (exactSourceGameId == "platinum_nds") cart = Gen4::Table::Pt;
    // Colosseum/XD origin (version 15): ask the Gen III GameCube judge first (Analyzer::findCxdEncounter).
    Gen4::CxdOriginJudgement cxd;
    const bool cxdOrigin = pk.version == 15;
    if (cxdOrigin && judge != nullptr) cxd = judge(Gen4::cxdInput(pk));
    const bool cartKnown = cart != Gen4::Table::HGSS || exactSourceGameId == "heartgold_nds" || exactSourceGameId == "soulsilver_nds";
    Gen4::Analyzer a(pk, cart, cxdOrigin ? &cxd : nullptr, cartKnown);
    Report result = a.run();
    if (evidence != nullptr) *evidence = a.evidence();
    return result;
}

} // namespace Legality
