/**
 * Gen4LegalityPalPark.cpp - Generation III encounter search for a PK4 that came through Pal Park.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): EncounterGenerator3 (deferrals, the wild-lead
 * queue, the last-resort yield and the PIDIV revisions of IRandomCorrelationEvent3), EncounterEnumerator3,
 * the IsMatchExact / GetMatchRating / IsCompatible members of EncounterSlot3(+Swarm), EncounterStatic3,
 * EncounterTrade3, EncounterGift3, EncounterGift3NY, EncounterGift3JPN, EncounterGift3Colo and
 * EncounterEgg3 (for a format-4 entity, whose met level and location were replaced on transfer),
 * MethodH (Gen III wild leads), SlotMethodH, CommonEvent3Checker, PCJPFifthAnniversary, WeightedTable3,
 * MystryMew and ChannelJirachi. As in Gen4LegalityEncounters.cpp the generator is evaluated eagerly;
 * PKHeX's finder only peeks one element ahead, so the eager list and its ManualFlag are equivalent.
 */
#include "Gen4LegalityInternal.h"
#include "Legality/Gen3Cxd.h"
#include "Legality/Gen3LegalityData.h"

#include <algorithm>

namespace Legality::Gen4 {

namespace {

enum Rating : int { kMatch = 0, kDeferred = 1, kDeferredErrors = 2, kPartialMatch = 3, kMaxNotMatch = 4 };
enum : uint8_t { kBallPoke = 4, kBallSafari = 5 };
enum : uint8_t { kVerS = 1, kVerR = 2, kVerE = 3, kVerFR = 4, kVerLG = 5 };
enum : uint8_t { kLangJapanese = 1, kLangEnglish = 2, kLangItalian = 4 };
enum : uint16_t { kJynx = 124, kTogepi = 175, kJirachi = 385, kMoveWish = 273 };
enum : uint16_t { kLocLinkTrade3NPC = 254, kLocEvent3 = 255, kLocHatchRSE = 32, kLocHatchFRLG = 146 };
// SlotType3
enum : uint8_t { kGrass = 0, kSurf = 1, kOldRod = 2, kGoodRod = 3, kSuperRod = 4, kRockSmash = 5, kSwarmGrass50 = 6, kSwarmFish50 = 7 };
// GiftGender3
enum : uint8_t { kGgRecipient = 0, kGgOnly0, kGgOnly1, kGgRandAlgo, kGgRandD3, kGgRandS3, kGgRandS7, kGgRandSG15, kGgRandD3_0, kGgRandD3_1 };
// EncounterGift3.Method as stored by the generator.
enum : uint8_t { kBACD_U = 1, kBACD_R = 2, kBACD_R_A = 3, kBACD_U_AX = 4, kBACD_TA = 5, kBACD_TS = 6, kBACD_RBCD = 7,
                 kBACD_M = 8, kChannel = 9, kMethod_2 = 10 };

uint8_t pkCurrentLevel(const Pk4& p) noexcept {
    return levelFromExp(p.exp, personal(Table::HGSS, p.species, p.form).expGrowth());
}
bool isContestBelow(const Pk4& pk, const uint8_t (&initial)[6]) noexcept {
    for (std::size_t i = 0; i < 6; ++i) if (pk.contest[i] < initial[i]) return true;
    return false;
}
bool isSafari3(uint8_t location) noexcept { return location == 57 || location == 136; } // Locations.IsSafariZoneLocation3
bool hasVersion(uint16_t mask, uint8_t version) noexcept { return version < 16 && ((mask >> version) & 1u) != 0; }

uint32_t lcNext(uint32_t s, int n) noexcept { return lcrng::advance(s, n); }
uint32_t lcPrev(uint32_t s, int n) noexcept { return lcrng::reverse(s, n); }

// ------------------------------------------------------------------ CommonEvent3 gender bits
uint32_t genderBit0(uint32_t r16) noexcept { return (r16 / 3) & 1u; }
uint32_t genderBit3(uint32_t r16) noexcept { return (r16 >> 3) & 1u; }
uint32_t genderBit7(uint32_t r16) noexcept { return ((r16 >> 7) & 1u) ^ 1u; }
uint32_t genderBit15(uint32_t r16) noexcept { return (r16 >> 15) & 1u; }

// ------------------------------------------------------------------ Method H (wild slots)
uint8_t slotMethodH(uint8_t type, uint32_t rand) noexcept {
    const uint32_t r = rand % 100;
    switch (type) {
    case kGrass:
        return r < 20 ? 0 : r < 40 ? 1 : r < 50 ? 2 : r < 60 ? 3 : r < 70 ? 4 : r < 80 ? 5 : r < 85 ? 6 : r < 90 ? 7
             : r < 94 ? 8 : r < 98 ? 9 : r < 99 ? 10 : 11;
    case kSurf: case kRockSmash: return r < 60 ? 0 : r < 90 ? 1 : r < 95 ? 2 : r < 99 ? 3 : 4;
    case kOldRod: return r < 70 ? 0 : 1;
    case kGoodRod: return r < 60 ? 0 : r < 80 ? 1 : 2;
    case kSuperRod: return r < 40 ? 0 : r < 80 ? 1 : r < 95 ? 2 : r < 99 ? 3 : 4;
    case kSwarmGrass50: case kSwarmFish50: return r < 50 ? 0 : 255;
    default: return 255;
    }
}

uint8_t pressureLevel3(const D::Area3& a, const D::Slot3& s) noexcept {
    if (a.type != kGrass) return s.levelMax;
    uint8_t max = s.levelMax;
    for (uint16_t k = 0; k < a.slotCount; ++k) {
        const auto& o = D::kSlots3[a.slotStart + k];
        if (o.species != s.species || o.levelMax < max) continue;
        max = o.levelMax;
    }
    return max;
}

struct Ctx3 {
    uint32_t seed1, seed2, seed3;
    explicit Ctx3(uint32_t seed) noexcept
        : seed1(lcrng::prev(seed)), seed2(lcrng::prev(seed1)), seed3(lcrng::prev(seed2)) {}
    uint32_t prev1() const noexcept { return seed1 >> 16; }
    uint32_t prev2() const noexcept { return seed2 >> 16; }
    uint32_t prev3() const noexcept { return seed3 >> 16; }
    uint32_t seed4() const noexcept { return lcrng::prev(seed3); }
};

class MethodH {
public:
    MethodH(const D::Area3& a, const D::Slot3& s, uint8_t levelMin, uint8_t levelMax) noexcept
        : a_(a), s_(s), min_(levelMin), max_(levelMax) {}

    LeadSeed getSeed(uint32_t seed, bool emerald, uint8_t gender) const noexcept {
        const bool unown = s_.species == kUnown;
        const uint32_t pid = unown ? reversePid(seed) : sequentialPid(seed);
        const auto nature = static_cast<uint8_t>(pid % 25);
        if (emerald) {
            const uint8_t ratio = s_.species < 387 ? D::kPersonalE3[s_.species].gender : 255;
            if (ratio == 255)
                return originEmerald(seed, nature, static_cast<uint16_t>(methodJReversalWindow(seed, nature)), 0);
            uint8_t gmin = 0, gmax = 255;
            if (ratio != 0 && ratio != 254) {
                if (gender == 0) { gmin = ratio; gmax = 255; }
                else if (gender == 1) { gmin = 0; gmax = static_cast<uint8_t>(ratio - 1); }
            }
            const auto [noLead, cute] = windowCute(seed, nature, gmin, gmax);
            return originEmerald(seed, nature, static_cast<uint16_t>(noLead), static_cast<uint16_t>(cute));
        }
        const int window = unown ? windowUnown(seed, s_.form) : methodJReversalWindow(seed, nature);
        return origin(seed, nature, static_cast<uint16_t>(window));
    }

private:
    const D::Area3& a_;
    const D::Slot3& s_;
    uint8_t min_, max_;

    static uint32_t reversePid(uint32_t seed) noexcept {
        const uint32_t r1 = lcrng::next16(seed);
        const uint32_t r2 = lcrng::next16(seed);
        return (r1 << 16) | r2;
    }
    static std::pair<int, int> windowCute(uint32_t seed, uint8_t nature, uint8_t gmin, uint8_t gmax) noexcept {
        int noLead = 0;
        int ctr = 0;
        uint32_t b = seed >> 16;
        while (true) {
            const uint32_t a = lcrng::prev16(seed);
            const uint32_t pid = (b << 16) | a;
            if (pid % 25 == nature) {
                if (noLead == 0) noLead = ctr;
                const uint32_t g = pid & 0xFF;
                if (gmin <= g && g <= gmax) return {noLead, ctr - noLead};
            }
            b = lcrng::prev16(seed);
            ++ctr;
        }
    }
    static int windowUnown(uint32_t seed, uint8_t form) noexcept {
        int ctr = 0;
        uint32_t b = seed >> 16;
        while (true) {
            const uint32_t a = lcrng::prev16(seed);
            const uint32_t pid = (a << 16) | b;
            if (unownForm3(pid) == form) break;
            b = lcrng::prev16(seed);
            ++ctr;
        }
        return ctr;
    }

    bool levelOk(uint32_t level) const noexcept { return min_ <= level && level <= max_; } // format 4: range only
    uint32_t randomLevel(uint32_t r) const noexcept {
        const uint32_t mod = 1u + s_.levelMax - s_.levelMin;
        return r % mod + s_.levelMin;
    }
    uint32_t randomLevelMinus1(uint32_t r) const noexcept {
        const uint32_t mod = 1u + s_.levelMax - s_.levelMin;
        uint32_t bias = r % mod;
        if (bias != 0) --bias;
        return s_.levelMin + bias;
    }
    bool slotOk(uint32_t r) const noexcept { return slotMethodH(a_.type, r) == s_.slotNumber; }
    bool staticMagnet(uint32_t r, Lead& lead) const noexcept {
        if (s_.staticCount != 0 && r % s_.staticCount == s_.staticIndex) { lead = Lead::Static; return true; }
        if (s_.magnetPullCount != 0 && r % s_.magnetPullCount == s_.magnetPullIndex) { lead = Lead::MagnetPull; return true; }
        lead = Lead::None;
        return false;
    }
    /// CheckEncounterActivation: only Rock Smash rolls its encounter rate (no lead influence).
    bool activation(uint32_t seed, Lead lead, LeadSeed& out) const noexcept {
        bool pass = true;
        if (a_.type == kRockSmash) {
            uint32_t rate = a_.rate * 16u;
            rate += rate >> 1;
            pass = (seed >> 16) % 2880u < rate;
            if (pass) seed = lcrng::prev(seed);
        }
        out = LeadSeed{seed, lead};
        return pass;
    }

    bool regular(const Ctx3& c, LeadSeed& out, Lead lead = Lead::None) const noexcept {
        if (levelOk(randomLevel(c.prev1())) && slotOk(c.prev2()) && activation(c.seed3, lead, out)) return true;
        out = LeadSeed{};
        return false;
    }
    bool from1Skip(const Ctx3& c, uint32_t& seed) const noexcept {
        if (levelOk(randomLevel(c.prev2())) && slotOk(c.prev3())) { seed = c.seed4(); return true; }
        return false;
    }
    bool from1SkipMinus1(const Ctx3& c, uint32_t& seed) const noexcept {
        if (levelOk(randomLevelMinus1(c.prev2())) && slotOk(c.prev3())) { seed = c.seed4(); return true; }
        return false;
    }

    bool noSync(const Ctx3& c, LeadSeed& out) const noexcept {
        if (regular(c, out)) return true;
        uint32_t seed = 0;
        // Synchronize failed to proc.
        if ((c.prev1() & 1) != 0 && from1Skip(c, seed) && activation(seed, Lead::SynchronizeFail, out)) return true;
        // Cute Charm failed to proc.
        if (c.prev1() % 3 == 0 && from1Skip(c, seed) && activation(seed, Lead::CuteCharmFail, out)) return true;
        // Pressure / Hustle / Vital Spirit failed: level range biased down by one.
        if ((c.prev1() & 1) != 1 && from1SkipMinus1(c, seed) && activation(seed, Lead::PressureHustleSpiritFail, out)) return true;
        // Static / Magnet Pull failed.
        if ((c.prev3() & 1) != 0 && levelOk(randomLevel(c.prev1())) && slotOk(c.prev2()) &&
            activation(c.seed4(), Lead::StaticMagnetFail, out)) return true;
        // Static / Magnet Pull.
        {
            Lead lead = Lead::None;
            if ((c.prev3() & 1) == 0 && levelOk(randomLevel(c.prev1())) && staticMagnet(c.prev2(), lead) &&
                activation(c.seed4(), lead, out)) return true;
        }
        // Pressure / Hustle / Vital Spirit: always the area's maximum level for the species.
        if ((c.prev1() & 1) == 1 && levelOk(pressureLevel3(a_, s_)) && slotOk(c.prev3()) &&
            activation(c.seed4(), Lead::PressureHustleSpirit, out)) return true;
        // Intimidate / Keen Eye did not reject.
        if ((c.prev1() & 1) != 1 && from1Skip(c, seed) && activation(seed, Lead::IntimidateKeenEyeFail, out)) return true;
        // Cute Charm.
        if (c.prev1() % 3 != 0 && from1Skip(c, seed) && activation(seed, Lead::CuteCharm, out)) return true;
        out = LeadSeed{};
        return false;
    }

    static uint32_t safariBlockSeed(uint32_t seed) noexcept { return 0xC048C851u * seed + 0x196302B4u; } // 300 prev calls
    static bool safariBlockProc(uint32_t seed) noexcept { return (seed >> 16) % 100 < 80; }

    bool tryMatchNoLead(uint32_t seed, uint8_t nature, LeadSeed& out) const noexcept {
        const uint32_t p0 = seed >> 16;
        const bool rseSafari = a_.location == 57;
        if (rseSafari) {
            const uint32_t sb = safariBlockSeed(seed);
            if (safariBlockProc(sb) && regular(Ctx3(sb), out)) return true;
        }
        if (s_.species == kUnown) {
            if (regular(Ctx3(lcrng::next(seed)), out)) return true;
        } else if (p0 % 25 == nature) {
            if (rseSafari) seed = lcrng::prev(seed);
            if (regular(Ctx3(seed), out)) return true;
        }
        out = LeadSeed{};
        return false;
    }
    bool tryMatch(uint32_t seed, uint8_t nature, LeadSeed& out) const noexcept {
        const uint32_t p0 = seed >> 16;
        if (a_.location == 57) {
            const uint32_t sb = safariBlockSeed(seed);
            if (safariBlockProc(sb) && noSync(Ctx3(sb), out)) return true;
            seed = lcrng::prev(seed);
        }
        if ((p0 & 1) == 0 && regular(Ctx3(seed), out, Lead::Synchronize)) return true;
        if (p0 % 25 == nature && noSync(Ctx3(seed), out)) return true;
        out = LeadSeed{};
        return false;
    }

    LeadSeed originEmerald(uint32_t seed, uint8_t nature, uint16_t reverseCount, uint16_t revCute) const noexcept {
        LeadSeed prefer{};
        int rc = reverseCount;
        int cute = revCute;
        while (true) {
            LeadSeed r{};
            if (tryMatch(seed, nature, r)) {
                if (r.lead == Lead::None) return r;
                if (r.lead > prefer.lead) prefer = r;
            }
            if (rc == 0) {
                if (cute == 0 || prefer.valid()) return prefer;
                break;
            }
            --rc;
            seed = lcPrev(seed, 2);
        }
        while (true) {
            LeadSeed r{};
            if (tryMatch(seed, nature, r) && r.lead == Lead::None) {
                r.lead = Lead::CuteCharm;
                if (r.lead > prefer.lead) prefer = r;
            }
            --cute;
            if (cute == 0) return prefer;
            seed = lcPrev(seed, 2);
        }
    }
    LeadSeed origin(uint32_t seed, uint8_t nature, uint16_t reverseCount) const noexcept {
        int rc = reverseCount;
        while (true) {
            LeadSeed r{};
            if (tryMatchNoLead(seed, nature, r)) return r;
            if (rc == 0) break;
            --rc;
            seed = lcPrev(seed, 2);
        }
        return LeadSeed{};
    }
};

bool isClassicMethod(PidType t) noexcept {
    switch (t) {
    case PidType::Method_1: case PidType::Method_2: case PidType::Method_3: case PidType::Method_4:
    case PidType::Method_1_Unown: case PidType::Method_2_Unown: case PidType::Method_3_Unown: case PidType::Method_4_Unown:
        return true;
    default:
        return false;
    }
}

// ------------------------------------------------------------------ event PID/IV revisions
/// PCJPFifthAnniversary / WeightedTable3 table roll of a restricted (16-bit) seed.
bool isMatchTableRand(uint16_t species, bool isWish, uint16_t seed16, bool isShiny) noexcept {
    const uint32_t index = ((static_cast<uint32_t>(species) >> 2) + 1) & 3u;
    uint32_t s = seed16;
    const uint32_t r1 = lcrng::next16(s);
    const uint32_t r2 = lcrng::next16(s);
    const uint32_t rand = (r1 << 16) | r2;
    const uint32_t high = rand >> 16;
    const uint32_t first = ((high << 2) & 0xFFFF) + high;
    uint32_t second = ((rand & 0xFFFF) << 1) + (first >> 16);
    second += high + (second >> 16);
    const uint32_t result = (1000u * (second & 0xFFFF)) >> 16;
    const uint32_t eighth = result / 125;
    const bool wish = (eighth & 1) == 1;
    const uint32_t idx = eighth >> 1;
    const bool shiny = idx == 0 && (result % 125) >= 100;
    return shiny == isShiny && idx == index && wish == isWish;
}
bool isRestrictedTable(uint32_t originSeed, uint16_t species, bool isWish, bool isShiny) noexcept {
    const uint32_t origin = lcPrev(originSeed, 2);
    if (origin > 0xFFFF) return false;
    if (species != kJirachi && !isMatchTableRand(species, isWish, static_cast<uint16_t>(origin), isShiny)) return false;
    return true;
}

constexpr uint16_t kMystryMewSeeds[] = {
    0x0652, 0x0932, 0x0C13, 0x0D43, 0x0EEE, 0x1263, 0x13C9, 0x1614, 0x1C09, 0x1EA5,
    0x20BF, 0x2389, 0x2939, 0x302D, 0x306E, 0x34F3, 0x45F3, 0x46CE, 0x4A0D, 0x4B63,
    0x4C79, 0x508E, 0x50AB, 0x5240, 0x5327, 0x56BA, 0x56CC, 0x5841, 0x5A60, 0x5BC1,
    0x5E2B, 0x5EF3, 0x6065, 0x643F, 0x6457, 0x67A3, 0x6944, 0x6E06, 0x6E62, 0x7667,
    0x77EF, 0x78D2, 0x8655, 0x8A92, 0x8B48, 0x93D0, 0x941D, 0x95A0, 0x967D, 0x9690,
    0x9C37, 0x9C40, 0x9D9C, 0x9DE4, 0x9E86, 0xA153, 0xA443, 0xA8AC, 0xAC08, 0xAFFB,
    0xB1F2, 0xB831, 0xBE96, 0xC2D4, 0xC385, 0xC6CE, 0xC92C, 0xC953, 0xC962, 0xCC43,
    0xCD47, 0xCD96, 0xD1E4, 0xDFED, 0xE62C, 0xE6CC, 0xE90A, 0xE95D, 0xE991, 0xEBB2,
    0xEE7F, 0xEE9F, 0xEFC8, 0xF0E4, 0xFE4E, 0xFE9D,
};
constexpr int kMystryMewCount = static_cast<int>(sizeof(kMystryMewSeeds) / sizeof(kMystryMewSeeds[0]));
/// Span.BinarySearch: the index when found, else the bitwise complement of the insertion point.
int mewBinarySearch(uint16_t value) noexcept {
    int lo = 0, hi = kMystryMewCount - 1;
    while (lo <= hi) {
        const int mid = lo + ((hi - lo) >> 1);
        const uint16_t v = kMystryMewSeeds[mid];
        if (v == value) return mid;
        if (v < value) lo = mid + 1;
        else hi = mid - 1;
    }
    return ~lo;
}
bool isMystryMew(uint32_t originSeed) noexcept {
    uint32_t seed = originSeed;
    int index = -1, sub = -1;
    if (seed <= 0xFFFF) {
        index = mewBinarySearch(static_cast<uint16_t>(seed));
        sub = 0;
    } else {
        for (int i = 1; i < 5; ++i) {
            seed = lcPrev(seed, 5);
            if (seed <= 0xFFFF) { index = mewBinarySearch(static_cast<uint16_t>(seed)); sub = i; break; }
        }
    }
    if (index == -1) return false;
    uint32_t check = originSeed;
    if (sub != 0) check = kMystryMewSeeds[static_cast<unsigned>(index) >= static_cast<unsigned>(kMystryMewCount) ? 0 : index];
    return !(check == 0x6065 && sub != 2);
}

bool channelAccept(uint32_t& seed, int pivot) noexcept {
    if (pivot == 0) {
        seed = xdrng::prev(seed);
        if (xdrng::prev16(seed) <= 0x547A) return false;
        if (xdrng::prev16(seed) <= 0x4000) return false;
    } else if (pivot == 1) {
        if (xdrng::prev16(seed) > 0x4000) return false;
    } else {
        if (xdrng::prev16(seed) > 0x547A) return false;
        if (xdrng::prev16(seed) <= 0x4000) return false;
    }
    for (int i = 0; i < 5; ++i) seed = xdrng::prev(seed);
    return true;
}
bool channelMenu(uint32_t& seed) noexcept {
    const uint32_t end = seed >> 30;
    if (end == 0) return false;
    uint32_t seen = 1u << end;
    while (true) {
        seed = xdrng::prev(seed);
        const uint32_t p = seed >> 30;
        if (p == end) return false;
        seen |= 1u << p;
        if (seen >= 0b1110u) return true;
    }
}
bool isChannelJirachi(uint32_t originSeed) noexcept {
    for (int pivot = 0; pivot < 3; ++pivot) {
        uint32_t seed = originSeed;
        if (channelAccept(seed, pivot) && channelMenu(seed)) return true;
    }
    return false;
}

uint32_t gift3Gender(uint8_t otGender, uint32_t seed) noexcept {
    switch (otGender) {
    case kGgOnly0: case kGgRandD3_0: return 0;
    case kGgOnly1: case kGgRandD3_1: return 1;
    case kGgRandD3: return genderBit0(lcrng::next(seed) >> 16);
    case kGgRandS3: return genderBit3(lcrng::next(seed) >> 16);
    case kGgRandS7: return genderBit7(lcrng::next(seed) >> 16);
    case kGgRandSG15: return genderBit15(lcNext(seed, 2) >> 16);
    default: return 0;
    }
}
bool gift3MatchGender(const D::Gift3& g, const Pk4& pk, uint32_t originSeed) noexcept {
    const uint32_t expect = (g.otGender == kGgRandD3_0 || g.otGender == kGgRandD3_1)
        ? genderBit0(lcNext(originSeed, 6) >> 16)
        : gift3Gender(g.otGender, lcNext(originSeed, 4));
    return pk.otGender == expect;
}

bool hasMove(const uint16_t (&moves)[4], uint16_t move) noexcept {
    return std::find(std::begin(moves), std::end(moves), move) != std::end(moves);
}

/// EncounterGift3.IsCompatibleReviseReset. mutated is PIDIV.Mutated (None until a revision succeeds).
bool gift3Revise(const D::Gift3& g, const Pk4& pk, const PidIv& pidiv, PidType& mutated) noexcept {
    const PidType type = mutated == PidType::None ? pidiv.type : mutated;
    if ((type == PidType::BACD_EA || type == PidType::BACD_ES) && !g.isEgg) return false;
    if (g.otGender != kGgRandAlgo && g.otGender != kGgRecipient && (!g.isEgg || pk.isEgg) &&
        !gift3MatchGender(g, pk, pidiv.seed))
        return false;
    const uint32_t seed = pidiv.seed;
    switch (g.method) {
    case kBACD_U: return type == PidType::BACD;
    case kBACD_R:
        if (type != PidType::BACD || seed > 0xFFFF) return false;
        mutated = PidType::BACD_R;
        return true;
    case kBACD_R_A:
        if (!(type == PidType::BACD || type == PidType::BACD_A || type == PidType::BACD_EA) || seed > 0xFFFF) return false;
        mutated = PidType::BACD_R;
        return true;
    case kBACD_U_AX:
        if (type != PidType::BACD_AX) return false;
        mutated = PidType::BACD_AX;
        return true;
    case kBACD_TA:
        if (!(type == PidType::BACD || type == PidType::BACD_A || type == PidType::BACD_EA)) return false;
        if (!isRestrictedTable(seed, g.species, hasMove(g.moves, kMoveWish), false)) return false;
        mutated = PidType::BACD_R;
        return true;
    case kBACD_TS:
        if (!(type == PidType::BACD_S || type == PidType::BACD_ES)) return false;
        if (!isRestrictedTable(seed, g.species, hasMove(g.moves, kMoveWish), true)) return false;
        mutated = PidType::BACD_R;
        return true;
    case kBACD_RBCD:
        if (type != PidType::BACD_S || seed < 3 || seed > 213) return false;
        mutated = PidType::BACD_R;
        return true;
    case kBACD_M:
        if (type != PidType::BACD || !isMystryMew(seed)) return false;
        mutated = PidType::BACD_R;
        return true;
    case kChannel:
        if (type != PidType::Channel || !isChannelJirachi(seed)) return false;
        mutated = PidType::Channel;
        return true;
    case kMethod_2:
        return type == PidType::Method_2 || type == PidType::Method_1 || type == PidType::Method_4;
    default:
        return false;
    }
}

// ------------------------------------------------------------------ PCNY / PCJP
bool nyNameValid(uint8_t dist, std::u16string_view name) noexcept {
    const bool b = name == u"PCNYb", c = name == u"PCNYc", d = name == u"PCNYd";
    switch (dist) {
    case 0: case 2: case 3: case 4: return b || c;   // Evolution, Monster, Halloween, EXDragon
    case 1: return b || c || d;                       // Dragon
    case 5: case 6: case 7: return c || d;            // UnknownSpring, Colosseum, Box
    default: return d;                                // BabyTrade, SlitherSwim, AncientAliens, Sixth
    }
}
constexpr uint16_t kJpnTid[6] = {51126, 51224, 60114, 60227, 60321, 60505};
bool jpnNameValid(uint8_t dist, std::u16string_view name) noexcept {
    if (name == u"トウキョー" || name == u"ヨコハマ" || name == u"ナゴヤ" || name == u"オーサカ" || name == u"フクオカ") return true;
    return dist != 5 && name == u"サッポロ";
}

// ------------------------------------------------------------------ per-kind matching (format 4)
bool slot3MatchExact(const D::Slot3& s, const Evo& evo) noexcept {
    if (s.form != evo.form && s.species != kBurmy) return false;
    return evo.levelMax >= s.levelMin;
}
int slot3Rating(const D::Area3& a, const D::Slot3& s, const Pk4& pk) noexcept {
    if ((pk.ball() == kBallSafari) != isSafari3(a.location)) return kPartialMatch;
    if (s.species == kWurmple && pk.species != kWurmple && !isWurmpleEvoValid(pk.pid, pk.species)) return kPartialMatch;
    return kMatch;
}

bool static3MatchExact(const D::Static3& t, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.eggLocation() != 0) return false;
    if (evo.levelMax < t.level) return false;
    if (t.form != evo.form && !isFormChangeable(t.species)) return false;
    return true;
}
int static3Rating(const D::Static3& t, const Pk4& pk) noexcept {
    if ((pk.ball() == kBallSafari) != isSafari3(t.location)) return kPartialMatch;
    if (t.ball == kBallPoke && pk.ball() != t.ball) return kPartialMatch;
    if ((t.fateful != 0) != pk.fateful) return kPartialMatch;
    return kMatch;
}

bool trade3MatchExact(const D::Trade3& t, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.pid != t.pid) return false;
    for (int i = 5; i >= 0; --i) {
        const auto k = static_cast<std::size_t>(i);
        const int iv = t.ivs[k];
        if (static_cast<unsigned>(iv) > 31u) continue;
        if (iv != pk.ivs[k]) return false;
    }
    if (pk.tid != t.tid || pk.sid != t.sid) return false;
    if (evo.levelMax < t.level) return false;
    if (evo.form != 0 && !isFormChangeable(t.species)) return false;
    if (pk.gender != t.gender) return false;
    if (pk.otGender != t.otGender) return false;
    if (pk.eggLocation() != 0) return false;
    if (isContestBelow(pk, t.contest)) return false;
    return true;
}

bool gift3MatchExact(const D::Gift3& g, const Pk4& pk, const Evo& evo) {
    if (!hasVersion(g.versionMask, pk.version)) return false;
    if (pk.isEgg && !g.isEgg) return false;
    const bool hatchedEgg = g.isEgg && !pk.isEgg;
    if (!hatchedEgg) {
        if (g.sid != 0xFFFF && g.sid != pk.sid) return false;
        if (g.tid != 0xFFFF && g.tid != pk.tid) return false;
        if ((g.otGender == kGgOnly0 || g.otGender == kGgRandD3_0) && pk.otGender != 0) return false;
        if ((g.otGender == kGgOnly1 || g.otGender == kGgRandD3_1) && pk.otGender != 1) return false;
        const std::u16string_view wc = D::kGift3Strings[g.otName];
        if (!wc.empty() && wc != otText(pk)) return false;
    }
    if (evo.form != 0 && !isFormChangeable(g.species)) return false;
    if (g.language != 0) {
        if (g.language != pk.language) return false;
    } else if (!g.isEgg && pk.language == kLangJapanese) {
        return false;
    }
    if (pk.ball() != kBallPoke) return false;
    if ((g.fateful != 0) != pk.fateful && !g.isEgg) return false;
    if (pk.isEgg) return false;
    if (g.level > pk.metLevel) return false;
    if (pk.eggLocation() != 0) return false;
    return true;
}
int gift3Rating(const D::Gift3& g, const Pk4& pk) noexcept {
    if (g.isEgg && pk.isEgg && (g.fateful != 0) != pk.fateful) return kDeferredErrors;
    return kMatch;
}

bool ny3MatchExact(const D::GiftNY3& g, const Pk4& pk, const Evo& evo) {
    if (pk.version != kVerR && pk.version != kVerS) return false;
    if (pk.isEgg) return false;
    if (pk.sid != 0) return false;
    if (pk.tid == 0 || pk.tid >= 3000) return false;
    const std::u16string ot = otText(pk);
    if (ot.size() <= 4) return false;
    if (!nyNameValid(g.distribution, ot)) return false;
    if (evo.form != 0 && !isFormChangeable(g.species)) return false;
    if (pk.language == kLangJapanese) return false;
    if (pk.ball() != kBallPoke) return false;
    if (g.level > pk.metLevel) return false;
    if (pk.eggLocation() != 0) return false;
    return true;
}

bool jp3MatchExact(const D::GiftJP3& g, const Pk4& pk, const Evo& evo) {
    if (pk.version != kVerR) return false;
    if (pk.isEgg) return false;
    if (pk.sid != 0) return false;
    if (g.distribution >= 6 || pk.tid != kJpnTid[g.distribution]) return false;
    const std::u16string ot = otText(pk);
    if (ot.size() <= 2) return false;
    if (!jpnNameValid(g.distribution, ot)) return false;
    if (evo.form != 0 && !isFormChangeable(g.species)) return false;
    if (pk.language != kLangJapanese) return false;
    if (pk.ball() != kBallPoke) return false;
    if (10 > pk.metLevel) return false;
    if (pk.eggLocation() != 0) return false;
    return true;
}

bool colo3MatchExact(const D::GiftColo3& g, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.version != g.version) return false;
    if (pk.eggLocation() != 0) return false;
    if (evo.levelMax < g.level) return false;
    if (evo.form != 0 && !isFormChangeable(g.species)) return false;
    return true;
}
int colo3Rating(const D::GiftColo3& g, const Pk4& pk) noexcept {
    if (pk.ball() != kBallPoke) return kPartialMatch;
    if (g.version == kVerR && pk.language != kLangJapanese) return kPartialMatch; // Japanese bonus disk
    return kMatch;
}

// ------------------------------------------------------------------ the enumerator
struct Enumerator3 {
    const Pk4& pk;
    const std::vector<Evo>& chain;
    std::vector<Candidate> out;
    Candidate deferred{};
    bool hasDeferred = false;
    int rating = kMaxNotMatch;

    template <class Exact, class Rate>
    void consider(uint16_t species, const Candidate& c, Exact&& exact, Rate&& rate) {
        for (const Evo& evo : chain) {
            if (evo.species != species) continue;
            if (!exact(evo)) break;
            const int r = rate();
            if (r == kMatch) out.push_back(c);
            else if (r < rating) { deferred = c; hasDeferred = true; rating = r; }
            break;
        }
    }

    void colo(std::size_t from, std::size_t to) {
        for (std::size_t i = from; i < to; ++i) {
            const auto& g = D::kGiftsColo3[i];
            consider(g.species, Candidate{EncKind::GiftColo3, static_cast<int>(i), -1, g.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return colo3MatchExact(g, pk, evo); }, [&] { return colo3Rating(g, pk); });
        }
    }
    void gifts() {
        for (std::size_t i = 0; i < D::kGift3Count; ++i) {
            const auto& g = D::kGifts3[i];
            consider(g.species, Candidate{EncKind::Gift3, static_cast<int>(i), -1, g.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return gift3MatchExact(g, pk, evo); }, [&] { return gift3Rating(g, pk); });
        }
    }
    void pcny() {
        for (std::size_t i = 0; i < D::kGiftNY3Count; ++i) {
            const auto& g = D::kGiftsNY3[i];
            consider(g.species, Candidate{EncKind::GiftNY3, static_cast<int>(i), -1, g.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return ny3MatchExact(g, pk, evo); }, [] { return int{kMatch}; });
        }
    }
    void pcjp() {
        for (std::size_t i = 0; i < D::kGiftJP3Count; ++i) {
            const auto& g = D::kGiftsJP3[i];
            consider(g.species, Candidate{EncKind::GiftJP3, static_cast<int>(i), -1, g.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return jp3MatchExact(g, pk, evo); }, [] { return int{kMatch}; });
        }
    }
    void trades(uint8_t list) {
        for (std::size_t i = 0; i < D::kTrade3Count; ++i) {
            const auto& t = D::kTrades3[i];
            if (t.list != list) continue;
            consider(t.species, Candidate{EncKind::Trade3, static_cast<int>(i), -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return trade3MatchExact(t, pk, evo); }, [] { return int{kMatch}; });
        }
    }
    void slots(uint8_t list) {
        for (int ai = D::kArea3ListStart[list]; ai < D::kArea3ListStart[list + 1]; ++ai) {
            const auto& a = D::kAreas3[ai];
            for (uint16_t k = 0; k < a.slotCount; ++k) {
                const int si = a.slotStart + k;
                const auto& s = D::kSlots3[si];
                consider(s.species, Candidate{EncKind::Slot3, si, ai, s.species, 0, Lead::Invalid},
                         [&](const Evo& evo) { return slot3MatchExact(s, evo); }, [&] { return slot3Rating(a, s, pk); });
            }
        }
    }
    void statics(uint8_t list) {
        for (int i = D::kStatic3ListStart[list]; i < D::kStatic3ListStart[list + 1]; ++i) {
            const auto& t = D::kStatics3[i];
            consider(t.species, Candidate{EncKind::Static3, i, -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return static3MatchExact(t, pk, evo); }, [&] { return static3Rating(t, pk); });
        }
    }
    void slotsForVersion() {
        if (pk.isEgg || pk.eggLocation() != 0) return; // CanBeWildEncounter
        switch (pk.version) {
        case kVerFR: slots(D::SlotsFR); break;
        case kVerLG: slots(D::SlotsLG); break;
        case kVerS: slots(D::SlotsSwarmRSE); slots(D::SlotsS); break;
        case kVerR: slots(D::SlotsSwarmRSE); slots(D::SlotsR); break;
        case kVerE: slots(D::SlotsSwarmRSE); slots(D::SlotsE); break;
        default: break;
        }
    }
    void staticsForVersion() {
        switch (pk.version) {
        case kVerR: statics(D::StaticR3); statics(D::StaticRSE3); break;
        case kVerS: statics(D::StaticS3); statics(D::StaticRSE3); break;
        case kVerE: statics(D::StaticE3); statics(D::StaticRSE3); break;
        case kVerFR: statics(D::StaticFR3); statics(D::StaticFRLG3); break;
        case kVerLG: statics(D::StaticLG3); statics(D::StaticFRLG3); break;
        default: break;
        }
    }

    static bool canHatchAsEgg(uint16_t species) noexcept {
        return species < 494 && (D::kBreed[species].flags & 1) != 0;
    }
    static bool isSplitBreedNotBaby3(uint16_t species) noexcept {
        return species < 494 && (D::kBreed[species].flags & 16) != 0;
    }
    bool tryGetEgg(Candidate& egg) const {
        const Evo& devolved = chain.back();
        if (!devolved.insideLevelRange(5)) return false;
        const auto [species, form] = baseSpeciesForm3(devolved.species, devolved.form);
        if (species != devolved.species && !isSplitBreedNotBaby3(devolved.species)) return false;
        if (!canHatchAsEgg(species)) return false;
        if (form != 0 && (species == kPikachu || species == kEevee || species == kPichu)) return false;
        if (!isPresentInGameE(species, form)) return false;
        egg = Candidate{EncKind::Egg3, -1, -1, species, pk.version, Lead::Invalid};
        return true;
    }
    bool tryGetSplit(const Candidate& other, Candidate& split) const {
        const Evo* devolved = &chain.back();
        if (other.species == devolved->species) {
            if (chain.size() < 2) return false;
            devolved = &chain[chain.size() - 2];
        }
        if (!isSplitBreedNotBaby3(devolved->species)) return false;
        split = other;
        split.species = devolved->species;
        return true;
    }

    void run() {
        if (chain.empty()) return;
        colo(0, D::kGiftColoR3Count);
        colo(D::kGiftColoR3Count, D::kGiftColo3Count);
        gifts();
        if (pk.language == kLangEnglish) pcny();
        else if (pk.language == kLangJapanese) pcjp();
        switch (pk.version) {
        case kVerE: trades(1); break;
        case kVerFR: trades(2); trades(4); break;
        case kVerLG: trades(3); trades(4); break;
        default: trades(0); trades(1); break;
        }
        const bool mustBeSlot = pk.ball() == kBallSafari;
        if (mustBeSlot) { slotsForVersion(); staticsForVersion(); }
        else { staticsForVersion(); slotsForVersion(); }
        Candidate egg{};
        if (tryGetEgg(egg)) {
            out.push_back(egg);
            Candidate split{};
            if (tryGetSplit(egg, split)) out.push_back(split);
        }
        if (hasDeferred) out.push_back(deferred);
    }
};

enum class Compat : uint8_t { Match, NotIdeal, Mismatch };

/// EncounterGenerator3.IsTypeCompatible
Compat typeCompatible3(const Candidate& c, const Pk4& pk, const PidIv& pidiv, PidType& mutated) noexcept {
    auto m = [](bool b) { return b ? Compat::Match : Compat::Mismatch; };
    const PidType type = pidiv.type;
    switch (c.kind) {
    case EncKind::Gift3: return m(gift3Revise(D::kGifts3[c.index], pk, pidiv, mutated));
    case EncKind::GiftNY3: {
        const PidType t = mutated == PidType::None ? type : mutated;
        return m(t == PidType::BACD_AX);
    }
    case EncKind::GiftJP3: {
        const PidType t = mutated == PidType::None ? type : mutated;
        if (t != PidType::BACD && t != PidType::BACD_R) return Compat::Mismatch;
        return m(pk.otGender == genderBit7(lcNext(pidiv.seed, 5) >> 16));
    }
    case EncKind::Slot3: {
        const bool unown = D::kSlots3[c.index].species == kUnown;
        const bool match = unown
            ? (type == PidType::Method_1_Unown || type == PidType::Method_2_Unown || type == PidType::Method_3_Unown || type == PidType::Method_4_Unown)
            : (type == PidType::Method_1 || type == PidType::Method_2 || type == PidType::Method_3 || type == PidType::Method_4);
        return m(match);
    }
    case EncKind::Static3: {
        const auto& t = D::kStatics3[c.index];
        if (pk.version == kVerE) return m(type == PidType::Method_1);
        if (t.roaming) {
            if (type == PidType::Method_1_Roamer) return Compat::Match;
            return m(type == PidType::Method_1 && (pk.iv32 & 0x3FFFFFFFu) <= 0xFF);
        }
        if (type == PidType::Method_1) return Compat::Match;
        if (pk.version == kVerR || pk.version == kVerS) return type == PidType::Method_4 ? Compat::NotIdeal : Compat::Mismatch;
        return type == PidType::Method_4 && t.species == kTogepi ? Compat::NotIdeal : Compat::Mismatch;
    }
    case EncKind::GiftColo3: return m(type == PidType::CXD || type == PidType::CXDAnti);
    case EncKind::Egg3: return m(type == PidType::None); // EggRandomAnyType3 is false by default
    default: return m(type == PidType::None);          // EncounterTrade3 is not IRandomCorrelation
    }
}

uint8_t fixedBall3(const Candidate& c) noexcept {
    switch (c.kind) {
    case EncKind::Slot3: return isSafari3(D::kAreas3[c.area].location) ? kBallSafari : 0;
    case EncKind::Static3: return D::kStatics3[c.index].ball;
    case EncKind::Trade3: case EncKind::Gift3: case EncKind::GiftNY3: case EncKind::GiftJP3:
    case EncKind::GiftColo3: case EncKind::Egg3:
        return kBallPoke;
    default: return 0;
    }
}

enum class Deferral : uint8_t { None, PIDIVDefer, SlotNumber, Ball, PIDIV };

} // namespace

// ======================================================================= public
Search searchEncounters3(const Pk4& pk, uint8_t currentLevel) {
    if (pk.version < 1 || pk.version > 5) {
        Search s;
        s.pidiv = analyzePidIv(pk);
        return s;
    }
    return searchEncounters3(pk, originChain3(pk, currentLevel));
}

Search searchEncounters3(const Pk4& pk, const std::vector<Evo>& chain) {
    // A format-5 entity runs the same Generation III search (every Gen III template already
    // treats the met level as the transfer level); only its origin chain differs.
    Search s;
    s.pidiv = analyzePidIv(pk);
    if (pk.version < 1 || pk.version > 5) return s;
    if (chain.empty()) return s;
    Enumerator3 e{pk, chain, {}, {}, false, kMaxNotMatch};
    e.run();

    Deferral deferType = Deferral::None;
    bool hasDefer = false;
    Candidate deferCand{};
    auto defer = [&](Deferral type, const Candidate& c) {
        if (type >= deferType && hasDefer) return;
        deferType = type;
        deferCand = c;
        hasDefer = true;
    };
    const bool emerald = pk.version == kVerE;
    uint8_t gender = pk.gender;
    if (pk.species == kMarill || pk.species == kAzumarill) gender = genderFromPid(pk.pid, 0x3F);
    PidType mutated = PidType::None;
    std::vector<std::pair<LeadSeed, Candidate>> queue;
    const uint8_t ball = pk.ball();
    for (const Candidate& c : e.out) {
        const Compat compat = typeCompatible3(c, pk, s.pidiv, mutated);
        if (compat != Compat::Match) {
            defer(compat == Compat::NotIdeal ? Deferral::PIDIVDefer : Deferral::PIDIV, c);
            continue;
        }
        const bool ballOk = fixedBall3(c) == kBallSafari ? ball == kBallSafari : ball != kBallSafari;
        if (!ballOk) { defer(Deferral::Ball, c); continue; }
        if (c.kind != EncKind::Slot3) { s.list.push_back(c); continue; }

        const auto& area = D::kAreas3[c.area];
        const auto& slot = D::kSlots3[c.index];
        // LeadFinder.GetLevelConstraint (format 4): the chain entry of the slot species, clamped to the slot.
        uint8_t levelMin = 0, levelMax = 0;
        for (const Evo& evo : chain) {
            if (evo.species != slot.species) continue;
            levelMin = std::max(evo.levelMin, slot.levelMin);
            levelMax = std::min(evo.levelMax, slot.levelMax);
            break;
        }
        LeadSeed lead{};
        if (isClassicMethod(s.pidiv.type)) lead = MethodH(area, slot, levelMin, levelMax).getSeed(s.pidiv.seed, emerald, gender);
        if (!lead.valid()) { defer(Deferral::SlotNumber, c); continue; }
        Candidate withLead = c;
        withLead.lead = lead.lead;
        auto it = std::find_if(queue.begin(), queue.end(), [&](const auto& q) { return lead.lead > q.first.lead; });
        queue.insert(it, {lead, withLead});
    }
    for (const auto& q : queue) s.list.push_back(q.second);
    // Gen III yields whatever else it has after the lead queue (no early return).
    if (!hasDefer) return s;
    if (deferType == Deferral::PIDIV) s.manualFlag = 1;
    else if (deferType == Deferral::SlotNumber) s.manualFlag = 2;
    s.list.push_back(deferCand);
    return s;
}

int matchRating3(const Template& t, const Pk4& pk) {
    switch (t.kind) {
    case EncKind::Slot3: return slot3Rating(*t.area3, *t.slot3, pk);
    case EncKind::Static3: return static3Rating(D::kStatics3[t.index], pk);
    case EncKind::Trade3: case EncKind::GiftNY3: case EncKind::GiftJP3: return kMatch;
    case EncKind::Gift3: return gift3Rating(D::kGifts3[t.index], pk);
    case EncKind::GiftColo3: return colo3Rating(D::kGiftsColo3[t.index], pk);
    case EncKind::Cxd3: return kMatch; // the GameCube judge already rated it (Gen4Legality.cpp findCxdEncounter)
    default: return -1; // EncounterEgg3 is not IEncounterMatch
    }
}

Template makeTemplate3(const Candidate& c, const Pk4& pk) {
    Template t;
    t.kind = c.kind;
    t.index = c.index;
    auto copyMoves = [&](const uint16_t (&m)[4]) { for (std::size_t i = 0; i < 4; ++i) t.moves[i] = m[i]; };
    switch (c.kind) {
    case EncKind::Slot3: {
        const auto& a = D::kAreas3[c.area];
        const auto& s = D::kSlots3[c.index];
        t.area3 = &a; t.slot3 = &s;
        t.species = s.species; t.form = s.form; t.levelMin = s.levelMin; t.levelMax = s.levelMax;
        t.version = a.version; t.location = a.location; t.fixedBall = isSafari3(a.location) ? kBallSafari : 0;
        t.ability = 0; t.shiny = kShinyRandom;
        copyMoves(s.moves); // EncounterSlot3Swarm moves; zero for regular slots
        break;
    }
    case EncKind::Static3: {
        const auto& s = D::kStatics3[c.index];
        t.species = s.species; t.form = s.form; t.levelMin = t.levelMax = s.level; t.version = s.version;
        t.location = s.location; t.isEgg = s.isEgg != 0; t.fixedBall = s.ball; t.fateful = s.fateful != 0;
        t.ability = 0; t.shiny = kShinyRandom;
        copyMoves(s.moves);
        break;
    }
    case EncKind::Trade3: {
        const auto& s = D::kTrades3[c.index];
        t.species = s.species; t.levelMin = s.level; t.levelMax = 100; t.version = s.version;
        t.location = kLocLinkTrade3NPC; t.fixedBall = kBallPoke; t.ability = s.ability; t.shiny = kShinyFixed;
        t.gender = s.gender; t.nature = static_cast<uint8_t>(s.pid % 25);
        break;
    }
    case EncKind::Gift3: {
        const auto& g = D::kGifts3[c.index];
        t.species = g.species; t.levelMin = t.levelMax = g.level; t.version = pk.version;
        t.location = kLocEvent3; t.isEgg = g.isEgg != 0; t.fixedBall = kBallPoke; t.ability = 0;
        t.shiny = g.shiny; t.fateful = g.fateful != 0;
        copyMoves(g.moves);
        break;
    }
    case EncKind::GiftNY3: {
        const auto& g = D::kGiftsNY3[c.index];
        t.species = g.species; t.levelMin = t.levelMax = g.level; t.version = pk.version;
        t.location = kLocEvent3; t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyNever;
        copyMoves(g.moves);
        break;
    }
    case EncKind::GiftJP3: {
        const auto& g = D::kGiftsJP3[c.index];
        t.species = g.species; t.levelMin = t.levelMax = 10; t.version = kVerR;
        t.location = kLocEvent3; t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyNever;
        break;
    }
    case EncKind::GiftColo3: {
        const auto& g = D::kGiftsColo3[c.index];
        t.species = g.species; t.levelMin = t.levelMax = g.level; t.version = g.version;
        t.location = g.location; t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyNever;
        copyMoves(g.moves);
        break;
    }
    case EncKind::Egg3: {
        t.species = c.species; t.levelMin = t.levelMax = 5; t.version = c.version;
        t.location = (c.version == kVerFR || c.version == kVerLG) ? kLocHatchFRLG : kLocHatchRSE;
        t.isEgg = true; t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyRandom;
        break;
    }
    case EncKind::Invalid: default: {
        t.kind = EncKind::Invalid;
        t.species = pk.species; t.form = pk.form; t.levelMin = pk.metLevel;
        t.levelMax = pkCurrentLevel(pk); t.isEgg = pk.isEgg || pk.eggDate[2] != 0; t.version = pk.version;
        t.ability = -1; t.shiny = kShinyRandom;
        break;
    }
    }
    return t;
}

bool isFixedTrainer3(const Template& t) noexcept {
    switch (t.kind) {
    case EncKind::Trade3: case EncKind::GiftNY3: case EncKind::GiftJP3: case EncKind::GiftColo3: return true;
    case EncKind::Gift3: return D::kGifts3[t.index].fixedTrainer != 0;
    case EncKind::Cxd3: return t.fixedTrainer; // Duking's trades, bonus-disc gifts: the GameCube judge checked the OT
    default: return false;
    }
}

bool isTrainerMatch3(const Template& t, const Pk4& pk, std::u16string_view trainer, int language) {
    if (t.kind == EncKind::Trade3) {
        const auto& s = D::kTrades3[t.index];
        if (s.species == kJynx && pk.version == kVerLG && language == kLangItalian) language = 2;
        if (language <= 0 || language >= 8) return false;
        return trainer == std::u16string_view(D::kTrade3Strings[s.otNames + language]);
    }
    if (t.kind == EncKind::GiftColo3) {
        const auto& g = D::kGiftsColo3[t.index];
        if (language < 0 || language >= 8) return false;
        std::u16string_view expect = D::kGiftColo3Strings[g.names + language];
        const std::size_t max = language == 1 ? 5 : 7;
        if (expect.size() > max) expect = expect.substr(0, max);
        return trainer == expect; // the Spanish Duking exception only applies to Plusle
    }
    return true; // Gift3 / PCNY / PCJP: checked in the explicit match
}

bool isNicknameMatch3(const Template& t, const Pk4& pk, std::u16string_view nickname, int language) {
    if (t.kind != EncKind::Trade3) return true;
    const auto& s = D::kTrades3[t.index];
    if (s.species == kJynx && pk.version == kVerLG && language == kLangItalian) language = 2;
    if (language <= 0 || language >= 8) return false;
    return nickname == std::u16string_view(D::kTrade3Strings[s.nicknames + language]);
}

// ======================================================================= Colosseum/XD judge adapter
// The production Colosseum/XD judge: the Gen III GameCube judge (include/Legality/Gen3Cxd.h)
// run on the fields a Pal Park transfer keeps. The met level is the transfer level and the met
// location is Pal Park's, so the judge runs with hasOriginalMet false (PKHeX Format != 3). The
// Gen IV moves are not the Gen III moves, so the judge does not see them; findCxdEncounter
// verifies them against the template moves it returns. The judge has already applied the
// GameCube ball, fateful, shiny, ribbon and ability rules to the known fields, so the matched
// template restates the record's own values for those (no second, divergent check).
CxdOriginJudgement judgeCxdOrigin(const CxdOriginInput& input) {
    using Status = CxdOriginJudgement::Status;
    namespace C = Legality::Gen3Cxd;
    C::Input in;
    in.species = input.species;
    // PKHeX bounds the Gen III part of a transferred record's evolution chain by its met level,
    // the level at transfer (EvolutionGroup3.Devolve for format 4, EvolutionGroup4 for format 5).
    in.currentLevel = input.metLevel != 0 && input.metLevel < input.level ? input.metLevel : input.level;
    in.hasOriginalMet = false;
    in.metLevel = input.metLevel;
    in.ball = input.ball;
    in.fateful = input.fateful;
    in.isEgg = input.isEgg;
    in.pid = input.pid;
    in.ivs = input.ivs;
    in.tid = input.tid;
    in.sid = input.sid;
    in.otGender = input.otGender;
    in.language = input.language;
    in.namesKnown = true;
    in.ot = input.otName;
    in.nickname = input.nickname;
    in.isNicknamed = input.isNicknamed;
    in.ribbonsKnown = true;
    in.ribbonNational = input.ribbonNational;
    in.ribbonEarth = input.ribbonEarth;
    // Orre stores the ability bit apart from the PID. Pal Park reads that bit only for a species
    // whose two Gen III abilities it keeps (pokeplatinum src/main_menu/gba_pokemon.c:759-783: a
    // species in sSpeciesWithNewAbilities, or with one ability, always gets ability 1), so the
    // stored Gen IV ability names the Orre bit only for a species with two Gen III abilities.
    if (input.species >= 1 && input.species <= Gen3Data::kMaxSpecies) {
        const Gen3Data::Personal& g3 = Gen3Data::kPersonal[input.species];
        if (g3.ability2 != 0 && g3.ability2 != g3.ability1 &&
            (input.ability == g3.ability1 || input.ability == g3.ability2)) {
            in.abilityKnown = true;
            in.abilityBit = input.ability == g3.ability2 ? 1 : 0;
        }
    }
    const C::Result r = C::judge(in);

    CxdOriginJudgement j;
    switch (r.verdict) {
    case C::Verdict::Invalid:
        j.status = Status::NoMatch;
        j.invalidReasons = r.invalid;
        j.warnings = r.warnings;
        return j;
    case C::Verdict::Undecided:
        j.status = Status::Undecided;
        // An undecided judge's findings are not proven: they stay Warnings.
        for (const std::string& t : r.invalid) j.warnings.push_back("Colosseum/XD origin (undecided): " + t);
        j.warnings.insert(j.warnings.end(), r.warnings.begin(), r.warnings.end());
        return j;
    case C::Verdict::Matched:
        break;
    }
    static constexpr const char* kNames[] = {
        "Colosseum/XD encounter", "XD in-game trade", "Colosseum shadow Pokemon",
        "Colosseum e-Reader shadow Pokemon", "Colosseum starter", "Colosseum gift",
        "XD shadow Pokemon", "XD gift", "XD Poke Spot",
    };
    const auto kind = static_cast<std::size_t>(r.matched);
    j.status = Status::Matched;
    j.templateName = kind < std::size(kNames) ? kNames[kind] : kNames[0];
    j.species = r.encounterSpecies;
    j.form = 0;
    j.levelMin = r.encounterLevel;
    j.levelMax = r.encounterLevelMax != 0 ? r.encounterLevelMax : r.encounterLevel;
    j.fixedBall = 0;
    j.fateful = input.fateful;
    j.shiny = 0;
    j.fixedTrainer = r.matched == C::Template::XdTrade || r.matched == C::Template::ColoGift;
    j.shadow = r.shadow;
    j.moves = r.encounterMoves;
    j.gender = 0xFF;
    j.ability = 0;
    j.invalidReasons = r.invalid;
    // The generic trainer-ID heuristics (zero, equal, PKHeX default pairs) are the Gen IV
    // analyzer's own (verifyTrainer); keep only the GameCube-specific findings here.
    for (const std::string& w : r.warnings) {
        const bool generic = w == "TID and SID are both zero" || w == "TID and SID are equal" || w == "TID is zero" ||
                             w == "SID is zero" || w == "TID/SID pair is suspicious";
        if (!generic) j.warnings.push_back(w);
    }
    if (!r.encounter.empty()) j.notes.push_back("Colosseum/XD origin matched " + r.encounter);
    return j;
}

} // namespace Legality::Gen4
