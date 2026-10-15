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
 * A Gen III origin (Pal Park) record is checked only for what does not depend on the Gen III
 * encounter; the encounter itself is reported as a Warning (coverage stays partial).
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
    kLocDaycare4 = 2000, kLocLinkTrade4 = 2002, kLocTransfer3 = 55, kItemGriseousOrb = 112,
    kMoveSketch = 166,
};
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

std::string moveFailText(std::size_t slot, const MoveResult& m, uint16_t move) {
    std::string what;
    switch (m.method) {
    case LearnMethod::Duplicate: what = "is a duplicate of a later move"; break;
    case LearnMethod::EmptyInvalid: what = "is empty before a later move (moves must be packed)"; break;
    case LearnMethod::UnobtainableExpect: what = "is not the move this encounter must know"; break;
    default: what = "cannot be learned by this Pokemon in Generation IV"; break;
    }
    return "Move " + std::to_string(slot + 1) + " (#" + std::to_string(move) + ") " + what;
}

// ======================================================================= analyzer
class Analyzer {
public:
    explicit Analyzer(const Pk4& pk) : pk_(pk) {}
    Report run();

private:
    const Pk4& pk_;
    Template enc_{};
    EncInfo info_{};
    std::vector<Evo> history_;
    MoveResults moves_{};
    Search search_{};
    std::vector<Check> parse_;
    uint8_t level_ = 1;     // CurrentLevel (HGSS growth of the current species/form)
    Personal pi_{};         // PersonalTable.HGSS.GetFormEntry(species, form)
    bool gtsSanitized_ = false;
    bool encounterSearched_ = false;

    void add(uint8_t severity, CheckIdentifier id, std::string text) {
        parse_.push_back(Check{severity, id, std::move(text)});
    }
    void invalid(CheckIdentifier id, std::string text) { add(kInvalid, id, std::move(text)); }
    void fishy(CheckIdentifier id, std::string text) { add(kFishy, id, std::move(text)); }

    bool isMysteryGift() const noexcept { return enc_.kind == EncKind::Pcd || enc_.kind == EncKind::RangerManaphy; }
    bool isPlayerOT() const noexcept {
        if (enc_.kind == EncKind::Trade || enc_.kind == EncKind::Ranch) return false;
        if (isMysteryGift() && !enc_.isEgg) return false;
        return true;
    }
    bool hasVisitedGen4() const noexcept { return !history_.empty(); }
    bool wasTradedEgg() const noexcept { return pk_.eggLocation() == kLocLinkTrade4; }
    bool isWithinEncounterRange() const noexcept;
    uint8_t encGeneration() const noexcept { return enc_.kind == EncKind::Invalid ? (isGen4Version(pk_.version) ? 4 : 0) : 4; }

    // encounter search
    Check verifyEncounter(const Template& t) const;
    Check verifyEgg4Hatched() const;
    void findVerifiedEncounter();
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

Check Analyzer::verifyEvolution(const Template& t, const std::vector<Evo>& history, const MoveResults& moves) const {
    if (pk_.species != t.species) {
        if (!hasVisited(history, pk_.species) || !hasVisited(history, t.species))
            return {kInvalid, CheckIdentifier::Species, "Evolution from the encounter species is not possible"};
        if (!isValidEvolutionWithMove(pk_, encInfo(t), history, moves))
            return {kInvalid, CheckIdentifier::Species, "Evolution requires a move this Pokemon cannot have known"};
    }
    return {};
}

std::string encounterHint(const Pk4& pk) {
    if (!pk.fateful && pk.eggLocation() - 2009u <= 5u) return "No matching gift egg";
    if (pk.fateful && pk.eggDate[2] != 0) return "No matching event egg (the only Gen IV event egg is Manaphy)";
    const uint16_t met = pk.metLocation();
    if (pk.fateful || (met >= 3000 && met <= 3076)) return "No matching Mystery Gift (event) encounter";
    return "No matching Gen IV encounter for this species, level, location, ball and PID/IV";
}

void Analyzer::findVerifiedEncounter() {
    search_ = searchEncounters(pk_, level_);
    encounterSearched_ = true;
    if (search_.list.empty()) {
        enc_ = makeTemplate(Candidate{}, pk_);
        info_ = encInfo(enc_);
        history_ = evolutionHistory(pk_, info_, level_);
        parse_.clear();
        invalid(CheckIdentifier::Encounter, encounterHint(pk_));
        moves_ = verifyMoves(pk_, info_, history_);
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
        const Template t = makeTemplate(search_.list[i], pk_);
        Check e = verifyEncounter(t);
        if (e.severity != kValid && peek(i)) continue;
        enc_ = t;
        info_ = encInfo(t);
        parse_.clear();
        if (e.severity != kValid) {
            if (e.message.empty()) e.message = encounterHint(pk_);
            parse_.push_back(e);
        }
        history_ = evolutionHistory(pk_, info_, level_);
        moves_ = verifyMoves(pk_, info_, history_);
        if (!allValid(moves_) && peek(i)) continue;
        bool parseValid = true;
        for (const auto& c : parse_) if (c.severity == kInvalid) parseValid = false;
        if (!parseValid && peek(i)) continue;
        const Check evo = verifyEvolution(t, history_, moves_);
        if (evo.severity != kValid && peek(i)) continue;
        if (evo.severity != kValid) parse_.push_back(evo);
        const int rating = matchRating(t, pk_);
        if (rating < 0 || rating < 3) break; // not IEncounterMatch, or better than PartialMatch
        if (peek(i)) continue;
        invalid(CheckIdentifier::Encounter, "Encounter is only a partial match (wrong version, location or IVs)");
        break;
    }
    const bool lastResortReached = requested + 1 == n;
    if (lastResortReached && search_.manualFlag == 2)
        invalid(CheckIdentifier::PidRng, "The PID/IV does not match any reachable RNG frame for this wild encounter and lead");
    else if (lastResortReached && search_.manualFlag == 1)
        invalid(CheckIdentifier::PidRng, "The PID/IV correlation type does not match this encounter");
}

// ----------------------------------------------------------------------- PID / EC
void Analyzer::verifyPid() {
    if (enc_.species == kWurmple && pk_.species != kWurmple && !isWurmpleEvoValid(pk_.pid, pk_.species))
        invalid(CheckIdentifier::PidRng, "Wurmple's evolution does not match its PID");
    if (pk_.pid == 0) fishy(CheckIdentifier::PidRng, "PID is zero");
    if (enc_.kind == EncKind::Egg) {
        if (pk_.pid == 0) fishy(CheckIdentifier::PidRng, "Egg PID is zero");
        if (enc_.species <= kMaxSpecies4 && (D::kBreed[enc_.species].flags & 8) != 0) {
            const bool bit34 = pk_.gender != ((pk_.pid & 0x8000u) >> 15);
            if (!bit34) {
                bool inherited = false;
                for (const auto& m : moves_) if (m.method == LearnMethod::InheritLevelUp) inherited = true;
                if (pk_.gender == 1 || inherited)
                    invalid(CheckIdentifier::PidRng, "PID bit 15 does not match the gender for this Nidoran/Volbeat/Illumise egg");
            }
        }
    }
    const bool shiny = pk_.isShiny();
    if ((enc_.shiny == kShinyNever && shiny) || (enc_.shiny == kShinyAlways && !shiny))
        invalid(CheckIdentifier::PidRng, shiny ? "This encounter cannot be shiny" : "This encounter must be shiny");
    if (enc_.kind == EncKind::Pcd && enc_.gift.pk.pid != 1 && pk_.pid != enc_.gift.pk.pid)
        invalid(CheckIdentifier::EventGift, "PID does not match the Mystery Gift's fixed PID");
}

// ----------------------------------------------------------------------- language
void Analyzer::verifyLanguage() {
    const uint8_t lang = pk_.language;
    const int max = encGeneration() == 4 ? 8 : -1;
    if (lang == 6 || lang == 0 || static_cast<int>(lang) > max) {
        invalid(CheckIdentifier::Trainer, "Language ID " + std::to_string(lang) + " is not valid for a Gen IV Pokemon");
        return;
    }
    if (encGeneration() == 4 && lang == kLangKorean && !pk_.isEgg) {
        const std::u16string ot = otText(pk_);
        gtsSanitized_ = ot == u"Ahn" || ot == u"Han" || ot == u"Jeong" || ot == u"Hwang";
    }
}

// ----------------------------------------------------------------------- nickname
void Analyzer::verifyNicknameEgg(const std::u16string& nickname) {
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
            const std::size_t foreign = (enc_.isEgg && wasTradedEgg()) ? 10 : 0;
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
        if (isWordFiltered4(nickname)) invalid(CheckIdentifier::Trainer, "Nickname is blocked by the Nintendo word filter");
        if (nickname.size() > 4 && numberCount(nickname) > 4) invalid(CheckIdentifier::Trainer, "Nickname has more than 4 digits");
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
        bool edge = false;
        if (enc_.isEgg) edge = (pk_.isEgg ? pk_.metLocation() == kLocLinkTrade4 : wasTradedEgg()) && ot.size() <= 12;
        if (!edge && !gtsSanitized_) invalid(CheckIdentifier::Trainer, "OT name is longer than its language allows");
    }
    if (isWordFiltered4(ot)) invalid(CheckIdentifier::Trainer, "OT name is blocked by the Nintendo word filter");
    if (ot.size() > 4 && numberCount(ot) > 4) invalid(CheckIdentifier::Trainer, "OT name has more than 4 digits");
}

void Analyzer::verifyTrainerId() {
    if (!isPlayerOT()) return;
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
    if (!vitamins) {
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
    if (sum == 0 && !isWithinEncounterRange()) fishy(CheckIdentifier::Stats, "Gained levels but has no EVs");
    else if (sum == 508) fishy(CheckIdentifier::Stats, "EV total is 508 (2 below the cap)");
    else if (pk_.evs[0] != 0 && allSame) fishy(CheckIdentifier::Stats, "All EVs are equal");
}

void Analyzer::verifyLevel() {
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
        fishy(CheckIdentifier::Stats, "EXP is exactly at a level threshold");
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
        // Event3
        if (gift) {
            if (rib(g, kEarth)) { if (!rib(pk_, kEarth)) ++bad; }
            else flag(kEarth); // Earth needs a Gen III encounter
            for (Rib r : kEvent3Ribbons) compare(r);
        } else {
            flag(kEarth);
            for (Rib r : kEvent3Ribbons) flag(r); // National: only Gen III Shadow Pokemon
        }
        // Event4
        for (Rib r : kEvent4Ribbons) { if (gift) compare(r); else flag(r); }
        // Unique3 / Common3 (no Gen III history for a Gen IV origin)
        flag(kWinning); flag(kVictory);
        flag(kChampionG3); flag(kArtist);
        if (rib(pk_, kEffort) && !gen4) ++bad;
        // Unique4
        const Evo head = gen4 ? history_.front() : Evo{};
        const bool frontier = gen4 && isAllowedBattleFrontier(head.species) && !(head.species == kPichu && head.form == 1);
        if (!frontier) for (Rib r : kAbilityRibbons) flag(r);
        for (uint8_t b = 4; b <= 6; ++b) for (uint8_t bit = 0; bit < 8; ++bit) if (!(b == 6 && bit >= 4)) flag(Rib{b, bit});
        const bool contest4 = gen4 && head.species != 132 && head.species != kUnown && !(head.species == kPichu && head.form == 1);
        for (uint8_t set = 0; set < 5; ++set) {
            const uint8_t b = static_cast<uint8_t>(8 + set / 2);
            const uint8_t base = static_cast<uint8_t>((set % 2) * 4);
            const bool initial = rib(pk_, Rib{b, base}), great = rib(pk_, Rib{b, static_cast<uint8_t>(base + 1)});
            const bool ultra = rib(pk_, Rib{b, static_cast<uint8_t>(base + 2)}), master = rib(pk_, Rib{b, static_cast<uint8_t>(base + 3)});
            if (!contest4) {
                bad += initial + great + ultra + master;
                continue;
            }
            bool top = master;
            if (ultra) top = true; else if (top) ++bad;
            if (great) top = true; else if (top) ++bad;
            if (top && !initial) ++bad;
        }
        // Common4
        if (rib(pk_, kFootprint) && !gen4) ++bad;
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
    const int encAbility = enc_.ability;
    if (encAbility > 0) {
        const int single = encAbility == 1 ? 0 : (encAbility == 2 ? 1 : 2);
        if (enc_.species != pk_.species) { pidMatch(); return; }
        if (single == abilIndex) return;
        if ((1 << pidIndex) == (1 << single)) return;
        invalid(CheckIdentifier::Ability, "Ability does not match this encounter's fixed ability");
        return;
    }
    pidMatch();
}

// ----------------------------------------------------------------------- ball
void Analyzer::verifyBall() {
    const uint8_t ball = pk_.ball();
    auto bad = [&]() { invalid(CheckIdentifier::Items, "Ball (#" + std::to_string(ball) + ") is not possible for this encounter"); };
    if (enc_.species == kNincada && pk_.species == kShedinja) {
        const bool bcc = enc_.kind == EncKind::Slot && enc_.area->type == SlotRef::kBugContest && history_.size() == 2;
        if (bcc ? !(ball == kBallSport || ball == kBallPoke) : ball != kBallPoke) bad();
        return;
    }
    if (enc_.kind == EncKind::Invalid) return;
    if (enc_.fixedBall != 0) { if (ball != enc_.fixedBall) bad(); return; }
    if (enc_.kind == EncKind::Egg) { if (ball != kBallPoke) bad(); return; }
    const uint8_t v = enc_.version;
    const bool hgss = v == D::kHG || v == D::kSS || v == D::kVerHGSS;
    uint64_t mask = 0;
    for (uint8_t b : {1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}) mask |= uint64_t{1} << b;
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
    if (species == kGiratina && ((form == 1) != (pk_.heldItem == kItemGriseousOrb))) {
        invalid(CheckIdentifier::Species, "Giratina's form must match whether it holds the Griseous Orb");
        return;
    }
    if (species == kArceus) {
        if (arceusForm4(pk_.heldItem) != form) invalid(CheckIdentifier::Species, "Arceus' form must match its held plate");
        return;
    }
    if (species == kRotom && pk_.isEgg && form != 0) { invalid(CheckIdentifier::Egg, "Rotom eggs cannot have a form"); return; }
    const bool battle = ((species == kCastform || species == 382 || species == 383 || species == kCherrim) && form != 0) ||
                        (isMegaSpecies4(species) && form != 0);
    if (battle) invalid(CheckIdentifier::Species, "Battle-only form cannot exist outside battle");
}

// ----------------------------------------------------------------------- misc
void Analyzer::verifyMisc() {
    if (pk_.isEgg) {
        if (pk_.friendship > pi_.hatchCycles()) invalid(CheckIdentifier::Egg, "Egg hatch counter is above the species' maximum");
        bool contest = false;
        for (uint8_t c : pk_.contest) if (c != 0) contest = true;
        if (contest) invalid(CheckIdentifier::Egg, "Eggs cannot have contest stats");
    }
    // MiscVerifierG4
    if (pk_.walkingMood != 0 && pk_.isEgg) fishy(CheckIdentifier::Egg, "Eggs do not have a walking mood");
    const uint8_t leaf = pk_.shinyLeaf;
    if ((leaf > 0x1F && leaf != 0x3F) || (leaf != 0 && pk_.isEgg)) invalid(CheckIdentifier::Misc, "Shiny Leaf value is not obtainable");
    if (pk_.ballDPPt > kBallCherish) invalid(CheckIdentifier::Items, "D/P/Pt ball value is above the Cherish Ball");
    {
        const bool createdHGSS = isHGSS(pk_.version) && enc_.kind != EncKind::Pcd;
        bool ok;
        if (!createdHGSS) ok = pk_.ballHGSS == 0;
        else ok = pk_.ballHGSS == pk_.ballDPPt || (pk_.ballDPPt == kBallPoke && pk_.ballHGSS > kBallCherish);
        if (!ok) invalid(CheckIdentifier::Items, "HG/SS ball value does not match the D/P/Pt ball value");
    }
    // Fateful
    if (isMysteryGift()) {
        const bool giftFateful = enc_.kind == EncKind::Pcd ? enc_.gift.pk.fateful : enc_.gift.fateful();
        if (enc_.kind == EncKind::Pcd && enc_.gift.version() != pk_.version && enc_.gift.pk.version == 0)
            invalid(CheckIdentifier::EventGift, "This Mystery Gift was not distributed to the origin game");
        if (pk_.fateful != giftFateful)
            invalid(CheckIdentifier::EventGift, giftFateful ? "Fateful encounter flag is missing" : "Fateful encounter flag is set");
    } else if ((enc_.kind == EncKind::Static || enc_.kind == EncKind::Ranch) && enc_.fateful) {
        if (!pk_.fateful) invalid(CheckIdentifier::EventGift, "Fateful encounter flag is missing");
    } else if (pk_.fateful) {
        invalid(CheckIdentifier::EventGift, "Fateful encounter flag is set on a non-event encounter");
    }
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
    const bool trade = enc_.kind == EncKind::Trade;
    if (trade) for (std::size_t i = 0; i < 5; ++i) initial[i] = D::kTrades[enc_.index].contest;
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
    for (std::size_t i = 0; i < 4; ++i)
        if (!moves_[i].valid()) invalid(CheckIdentifier::Moves, moveFailText(i, moves_[i], pk_.moves[i]));
}

// ----------------------------------------------------------------------- run
Report Analyzer::run() {
    Report r;
    r.coverage.structure = CoverageLevel::Complete;
    level_ = levelFromExp(pk_.exp, personal(Table::HGSS, pk_.species, pk_.form).expGrowth());
    pi_ = personal(Table::HGSS, pk_.species, pk_.form);

    findVerifiedEncounter();

    verifyPid();
    verifyLanguage();
    verifyNickname();
    verifyTrainerName();
    verifyTrainerId();
    verifyIvs();
    verifyEvs();
    verifyLevel();
    verifyRibbons();
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
    verifyMoveResults();

    bool anyInvalid = false;
    for (const Check& c : parse_) if (c.severity == kInvalid) anyInvalid = true;
    if (anyInvalid && enc_.kind == EncKind::Invalid && pk_.fateful)
        invalid(CheckIdentifier::EventGift, "Fateful encounter without a matching Mystery Gift");

    for (const Check& c : parse_) {
        if (c.severity == kValid) continue;
        const Severity sev = c.severity == kInvalid ? Severity::Invalid : Severity::Warning;
        bool dup = false;
        for (const Issue& i : r.issues) if (i.severity == sev && i.text == c.message) dup = true;
        if (!dup) r.issues.push_back(Issue{sev, c.message, c.id});
    }
    if (r.issues.empty()) {
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Complete;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Complete;
    } else {
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Partial;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Partial;
    }
    return r;
}

} // namespace

} // namespace Gen4

Report analyzePk4(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId) {
    (void)exactSourceGameId;
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
    if (Gen4::isGen3Version(pk.version)) {
        const char* name = Gen4::versionName(pk.version);
        r.coverage.sourceGame = r.coverage.internal = r.coverage.moves = r.coverage.encounter = CoverageLevel::Partial;
        r.coverage.eventGift = r.coverage.pidRng = r.coverage.eggBreeding = r.coverage.transfer = CoverageLevel::Partial;
        r.issues.push_back(Issue{Severity::Warning,
                                 std::string("Pal Park transfer from ") + (name ? name : "a Gen III game") +
                                     ": the Generation III encounter is not verified",
                                 CheckIdentifier::Transfer});
        return r;
    }
    if (!Gen4::isGen4Version(pk.version)) {
        r.issues.push_back(Issue{Severity::Invalid,
                                 "Origin game #" + std::to_string(pk.version) + " cannot produce a PK4 (not a Gen III or Gen IV game)",
                                 CheckIdentifier::SourceGame});
        return r;
    }
    Gen4::Analyzer a(pk);
    return a.run();
}

} // namespace Legality
