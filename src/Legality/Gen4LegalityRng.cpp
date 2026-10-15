/**
 * Gen4LegalityRng.cpp - PID/IV correlation and wild-encounter lead/frame checks for PK4.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): MethodFinder.Analyze, LCRNG/XDRNG
 * reversal, CommonEvent3 (BACD), CuteCharm4, PokewalkerRNG.GetPID, MethodJ (D/P/Pt),
 * MethodK (HG/SS), SlotMethodJ/K, LeadFinder.GetLeadInfo4, RuinsOfAlph4,
 * SolaceonRuins4, HoneyTreeUtil (Munchlax trees) and WurmpleUtil.
 */
#include "Gen4LegalityInternal.h"

namespace Legality::Gen4 {

// ======================================================================= personal tables
std::size_t tableCount(Table t) noexcept {
    switch (t) {
    case Table::DP: return D::kPersonalDPCount;
    case Table::Pt: return D::kPersonalPtCount;
    default: return D::kPersonalHGSSCount;
    }
}

Personal personalAt(Table t, std::size_t index) noexcept {
    if (index >= tableCount(t)) index = 0;
    const uint8_t* base = t == Table::DP ? D::kPersonalDP : (t == Table::Pt ? D::kPersonalPt : D::kPersonalHGSS);
    return Personal{base + index * D::kPersonalSize};
}

uint16_t formIndex(Table t, uint16_t species, uint8_t form) noexcept {
    if (species > kMaxSpecies4) return 0;
    return personalAt(t, species).formIndex(species, form);
}

Personal personal(Table t, uint16_t species, uint8_t form) noexcept {
    return personalAt(t, formIndex(t, species, form));
}

// PersonalTable4.IsPresentInGame
bool isPresentInGame(Table t, uint16_t species, uint8_t form) noexcept {
    if (species > kMaxSpecies4) return false;
    if (form == 0) return true;
    if (personalAt(t, species).hasForm(form)) return true;
    switch (species) {
    case kPichu: return form == 1;
    case kUnown: return form < 28;
    case kCastform: return form < 4;
    case kBurmy: case kMothim: return form < 3;
    case kCherrim: case kShellos: case kGastrodon: return form == 1;
    case kArceus: return form < 18 && form != 9;
    default: return false;
    }
}

uint32_t expForLevel(uint8_t level, uint8_t growth) noexcept {
    if (level <= 1 || growth > 5) return 0;
    if (level > 100) level = 100;
    return D::kExperience[growth * 100 + level - 1];
}

uint8_t levelFromExp(uint32_t exp, uint8_t growth) noexcept {
    if (growth > 5) return 1;
    if (exp >= D::kExperience[growth * 100 + 99]) return 100;
    uint8_t level = 1;
    while (level < 100 && D::kExperience[growth * 100 + level] <= exp) ++level;
    return level;
}

// ======================================================================= RNG reversal
namespace lcrng {
int getSeeds(uint32_t* out, uint32_t first, uint32_t second) noexcept {
    constexpr uint32_t kMod = 0x67D3, kPat = 0xD3E, kInc = 0x4034;
    const uint32_t diff = (second - (first * 0x41C64E6Du)) >> 16;
    uint32_t low = ((((diff * kMod) + kInc) >> 16) * kPat) % kMod;
    int ctr = 0;
    do {
        const uint32_t seed = first | low;
        if ((next(seed) & 0xFFFF0000u) == second && ctr < kSeedBuffer) out[ctr++] = prev(seed);
    } while ((low += kMod) < 0x10000);
    return ctr;
}

static void addSeedsIVs(uint32_t* out, uint32_t low, uint32_t first, uint32_t second, int& ctr) noexcept {
    constexpr uint32_t kMod = 0x67D3;
    do {
        const uint32_t test = first | low;
        if ((next(test) & 0x7FFF0000u) != second) continue;
        const uint32_t seed = prev(test);
        if (ctr + 2 <= kSeedBuffer) {
            out[ctr++] = seed;
            out[ctr++] = seed ^ 0x80000000u;
        }
    } while ((low += kMod) < 0x10000);
}

int getSeedsIVs(uint32_t* out, uint32_t first, uint32_t second) noexcept {
    constexpr uint32_t kMod = 0x67D3, kPat = 0xD3E, kInc = 0x4034;
    const uint32_t diff = (second - (first * 0x41C64E6Du)) >> 16;
    const uint32_t start1 = ((((diff * kMod) + kInc) >> 16) * kPat) % kMod;
    const uint32_t start2 = (((((diff ^ 0x8000u) * kMod) + kInc) >> 16) * kPat) % kMod;
    int ctr = 0;
    addSeedsIVs(out, start1, first, second, ctr);
    addSeedsIVs(out, start2, first, second, ctr);
    return ctr;
}

int getSeedsIVs(uint32_t* out, const std::array<uint8_t, 6>& iv) noexcept {
    // iv order: HP Atk Def Spe SpA SpD
    const uint32_t first = static_cast<uint32_t>(iv[0] | (iv[1] << 5) | (iv[2] << 10)) << 16;
    const uint32_t second = static_cast<uint32_t>(iv[3] | (iv[4] << 5) | (iv[5] << 10)) << 16;
    return getSeedsIVs(out, first, second);
}

int getSeedsSkip(uint32_t* out, uint32_t first, uint32_t third) noexcept {
    constexpr uint32_t kMod = 0x3A89, kPat = 0x2E4C, kInc = 0x5831;
    const uint32_t diff = (third - advance(first, 2)) >> 16;
    uint32_t low = ((((diff * kMod) + kInc) >> 16) * kPat) % kMod;
    int ctr = 0;
    do {
        const uint32_t seed = first | low;
        if ((advance(seed, 2) & 0xFFFF0000u) == third && ctr < kSeedBuffer) out[ctr++] = prev(seed);
    } while ((low += kMod) < 0x10000);
    return ctr;
}
} // namespace lcrng

namespace {
namespace xd {
constexpr uint32_t kMult = 0x343FD, kAdd = 0x269EC3;
constexpr uint32_t prev(uint32_t s) noexcept { return s * 0xB9B33155u + 0xA170F641u; }
int getSeeds(uint32_t* out, uint32_t first, uint32_t second) noexcept {
    constexpr uint32_t kSub = kAdd - 0xFFFFu;
    constexpr uint64_t kBase = (kMult + 1ull) * 0xFFFFull;
    uint64_t t = static_cast<uint32_t>(second - (first * kMult) - kSub);
    const uint64_t kmax = (kBase - t) >> 32;
    int ctr = 0;
    for (uint64_t k = 0; k <= kmax; k++, t += 0x100000000ull) {
        if (t % kMult < 0x10000 && ctr < kSeedBuffer)
            out[ctr++] = prev(first | static_cast<uint16_t>(t / kMult));
    }
    return ctr;
}
} // namespace xd

namespace arng {
constexpr uint32_t prev(uint32_t s) noexcept { return s * 0x9638806Du + 0x69C77F93u; }
}

uint32_t ivsRaw(const Pk4& pk) noexcept { return pk.iv32 & 0x3FFFFFFFu; }

bool lcrngMatch(uint32_t* seeds, uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    int count = lcrng::getSeeds(seeds, bot, top);
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        uint32_t s = lcrng::advance(seed, 2);
        if (iv1 == lcrng::next15(s)) {
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_1, seed}; return true; }
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_4, seed}; return true; }
        } else {
            if (iv1 != lcrng::next15(s)) continue;
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_2, seed}; return true; }
        }
    }
    count = lcrng::getSeedsSkip(seeds, bot, top);
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        uint32_t s = lcrng::advance(seed, 3);
        if (iv1 != lcrng::next15(s)) continue;
        if (iv2 != lcrng::next15(s)) continue;
        out = {PidType::Method_3, seed};
        return true;
    }
    return false;
}

bool lcrngUnownMatch(uint32_t* seeds, uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    int count = lcrng::getSeeds(seeds, top, bot);
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        uint32_t s = lcrng::advance(seed, 2);
        if (iv1 == lcrng::next15(s)) {
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_1_Unown, seed}; return true; }
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_4_Unown, seed}; return true; }
        } else {
            if (iv1 != lcrng::next15(s)) continue;
            if (iv2 == lcrng::next15(s)) { out = {PidType::Method_2_Unown, seed}; return true; }
        }
    }
    count = lcrng::getSeedsSkip(seeds, top, bot);
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        uint32_t s = lcrng::advance(seed, 3);
        if (iv1 != lcrng::next15(s)) continue;
        if (iv2 != lcrng::next15(s)) continue;
        out = {PidType::Method_3_Unown, seed};
        return true;
    }
    return false;
}

bool xdMatch(uint32_t* seeds, const Pk4& pk, uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    const int count = xd::getSeeds(seeds, top, bot);
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        uint32_t b = xd::prev(seed);
        uint32_t a = xd::prev(b);
        uint32_t hi = (a >> 16) & 0x7FFF;
        uint32_t lo = (b >> 16) & 0x7FFF;
        if (hi == iv1 && lo == iv2) { out = {PidType::CXD, xd::prev(a)}; return true; }
        const uint32_t tsv = static_cast<uint32_t>(pk.tid ^ pk.sid) >> 3;
        uint32_t psv = (top ^ bot) >> (16 + 3);
        if (psv == tsv) continue;
        uint32_t p2 = seed;
        uint32_t p1 = b;
        psv = (p2 ^ p1) >> 19;
        if (psv != tsv) continue;
        do {
            b = xd::prev(a);
            a = xd::prev(b);
            hi = (a >> 16) & 0x7FFF;
            lo = (b >> 16) & 0x7FFF;
            if (hi == iv1 && lo == iv2) { out = {PidType::CXDAnti, xd::prev(a)}; return true; }
            p2 = xd::prev(p1);
            p1 = xd::prev(p2);
            psv = (p2 ^ p1) >> 19;
        } while (psv == tsv);
    }
    return false;
}

bool roamerMatch(uint32_t* seeds, uint32_t top, uint32_t bot, uint32_t iv32, PidIv& out) noexcept {
    if (iv32 > 0xFF) return false;
    const int count = lcrng::getSeeds(seeds, bot, top);
    for (int i = 0; i < count; ++i) {
        const uint32_t ivc = (lcrng::advance(seeds[i], 3) >> 16) & 0xFF;
        if (iv32 != ivc) continue;
        out = {PidType::Method_1_Roamer, seeds[i]};
        return true;
    }
    return false;
}

uint32_t psvOf(uint32_t v) noexcept { return ((v >> 16) ^ (v & 0xFFFF)) >> 3; }

bool mg4Match(uint32_t* seeds, uint32_t pid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    const uint32_t currentPsv = psvOf(pid);
    pid = arng::prev(pid);
    const uint32_t originalPsv = psvOf(pid);
    if (originalPsv == currentPsv) return false;
    for (int r = 0; r < 3; ++r) {
        const int count = lcrng::getSeeds(seeds, pid << 16, pid & 0xFFFF0000u);
        for (int i = 0; i < count; ++i) {
            uint32_t s = lcrng::advance(seeds[i], 2);
            if (iv1 != lcrng::next15(s)) continue;
            if (iv2 != lcrng::next15(s)) continue;
            out = {PidType::G4MGAntiShiny, seeds[i]};
            return true;
        }
        pid = arng::prev(pid);
        if (psvOf(pid) != originalPsv) break;
    }
    return false;
}

// CommonEvent3
uint32_t ceRegular(uint32_t a16, uint32_t b16) noexcept { return (a16 << 16) | b16; }
uint32_t ceAntishiny(uint32_t a16, uint32_t b16, uint32_t idXor) noexcept { return ((a16 ^ (idXor ^ b16)) << 16) | b16; }
uint32_t ceForceShiny(uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    return (x16 << 16) | (((idXor ^ x16) & 0xFFF8u) | (b16 & 7u));
}
bool ceIsForceAntishiny(uint32_t pid, uint32_t a16, uint32_t b16, uint32_t idXor) noexcept {
    if ((a16 & ~7u) == 0) return false;
    return ceAntishiny(a16, b16, idXor) == pid;
}
bool ceIsForceShiny(uint32_t pid, uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    if (x16 != pid >> 16) return false;
    return ceForceShiny(x16, b16, idXor) == pid;
}
bool ceIsRegularAntishiny(uint32_t actual, uint32_t expect) noexcept { return ((expect + 8) & 0xFFFFFFF8u) == actual; }
bool ceIsRegularAntishinyDifferentOT(uint32_t actual, uint32_t expect) noexcept {
    if ((actual & 7u) != 0) return false;
    const uint32_t delta = actual - expect;
    return delta != 0 && delta < 8;
}
bool ceIsForceAntishinyDifferentOT(uint32_t pid, uint32_t a16, uint32_t b16, uint32_t idXor) noexcept {
    if ((a16 & ~7u) == 0) return false;
    if ((pid & 0xFFFF) != b16) return false;
    return (ceAntishiny(a16, b16, idXor) >> 19) != (pid >> 19);
}
bool ceIsForceShinyDifferentOT(uint32_t pid, uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    if (x16 != pid >> 16) return false;
    if ((pid & 7u) != (b16 & 7u)) return false;
    return ((idXor ^ x16) & 0xFFF8u) != (pid & 0xFFF8u);
}

bool bacdMatch(uint32_t* seeds, const Pk4& pk, uint32_t actualPid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    const int count = lcrng::getSeedsIVs(seeds, iv1 << 16, iv2 << 16);
    PidType type = PidType::BACD;
    const uint32_t idXor = static_cast<uint32_t>(pk.tid ^ pk.sid);
    for (int i = 0; i < count; ++i) {
        uint32_t seed = seeds[i];
        const uint32_t b16 = seed >> 16;
        const uint32_t a16 = lcrng::prev16(seed);
        const uint32_t expect = ceRegular(a16, b16);
        if (expect != actualPid) {
            const bool shiny = isShinyXor(idXor, actualPid);
            if (!shiny) {
                if (ceIsRegularAntishiny(actualPid, expect)) type = PidType::BACD_A;
                else if (ceIsForceAntishiny(actualPid, a16, b16, idXor)) type = PidType::BACD_AX;
                else if (ceIsForceShinyDifferentOT(actualPid, lcrng::prev16(seed), b16, idXor)) type = PidType::BACD_ES;
                else continue;
            } else {
                if (ceIsRegularAntishinyDifferentOT(actualPid, expect)) type = PidType::BACD_EA;
                else if (ceIsForceAntishinyDifferentOT(actualPid, a16, b16, idXor)) type = PidType::BACD_EAX;
                else if (ceIsForceShiny(actualPid, lcrng::prev16(seed), b16, idXor)) type = PidType::BACD_S;
                else continue;
            }
        }
        out = {type, lcrng::prev(seed)};
        return true;
    }
    return false;
}

bool chainShinyValid(const Pk4& pk, uint32_t pid, uint32_t seed, uint32_t& s) noexcept {
    s = seed;
    int i = 15;
    do {
        const uint32_t bit = (s >> 16) & 1;
        if (bit != ((pid >> i) & 1)) break;
        s = lcrng::prev(s);
    } while (--i != 2);
    if (i != 2) return false;
    const uint32_t upper = s;
    if (((upper >> 16) & 7) != ((pid >> 16) & 7)) return false;
    const uint32_t lower = lcrng::prev(upper);
    if (((lower >> 16) & 7) != (pid & 7)) return false;
    const uint32_t upid = (((pid & 0xFFFF) ^ pk.tid ^ pk.sid) & 0xFFF8u) | ((upper >> 16) & 7u);
    if (upid != pid >> 16) return false;
    s = lcrng::reverse(lower, 2);
    return true;
}

bool chainShinyMatch(uint32_t* seeds, const Pk4& pk, uint32_t pid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    const int count = lcrng::getSeedsIVs(seeds, iv1 << 16, iv2 << 16);
    for (int i = 0; i < count; ++i) {
        uint32_t s = 0;
        if (!chainShinyValid(pk, pid, seeds[i], s)) continue;
        out = {PidType::ChainShiny, s};
        return true;
    }
    return false;
}

bool g5mgShinyMatch(const Pk4& pk, uint32_t pid, PidIv& out) noexcept {
    const uint32_t low = pid & 0xFFFF;
    if (low > 0xFF) return false;
    const uint32_t abilBit = (pid >> 16) & 1;
    uint32_t expect = ((low ^ pk.tid ^ pk.sid) << 16) | low;
    if ((expect & 0x10000u) != ((abilBit & 1u) << 16)) expect ^= 0x10000u;
    if (pid != expect) return false;
    out = {PidType::G5MGShiny, 0};
    return true;
}

bool cuteCharmMatch(const Pk4& pk, uint32_t pid, PidIv& out) noexcept {
    if (!isCuteCharm(pk, pid)) return false;
    out = {PidType::CuteCharm, 0};
    return true;
}

bool pokewalkerMatch(const Pk4& pk, uint32_t actualPid, PidIv& out) noexcept {
    constexpr uint32_t kMid = 0x00FFFF00u;
    const uint32_t mid = actualPid & kMid;
    if (mid != 0 && mid != kMid) return false;
    const uint32_t nature = actualPid % 25;
    if (nature == 24) return false;
    const uint8_t gender = pk.gender;
    uint8_t gr = personal(Table::HGSS, pk.species, pk.form).gender();
    if (pk.species == kFroslass) gr = 0x7F;
    const uint32_t expect = pokewalkerPid(pk.tid, pk.sid, nature, gender, gr);
    if (expect != actualPid) {
        bool azurill = false;
        if (gender == 0 && (pk.species == kMarill || pk.species == kAzumarill)) {
            constexpr uint8_t kAzurillRatio = 0xBF; // EntityGender.MF (75% female)
            if (genderFromPid(actualPid, kAzurillRatio) == 1)
                azurill = pokewalkerPid(pk.tid, pk.sid, nature, 1, kAzurillRatio) == actualPid;
        }
        if (!azurill) return false;
    }
    out = {PidType::Pokewalker, 0};
    return true;
}

bool modifiedPidMatch(uint32_t* seeds, const Pk4& pk, uint32_t pid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    if (pk.isShiny()) {
        if (chainShinyMatch(seeds, pk, pid, iv1, iv2, out)) return true;
        // pk.Gen4: Cute Charm first, then the Gen V Mystery Gift shiny pattern.
        if (pid <= 0xFF && cuteCharmMatch(pk, pid, out)) return true;
        if (g5mgShinyMatch(pk, pid, out)) return true;
    } else {
        if (pid <= 0xFF && cuteCharmMatch(pk, pid, out)) return true;
    }
    return pokewalkerMatch(pk, pid, out);
}
} // namespace

PidIv analyzePidIv(const Pk4& pk) noexcept {
    uint32_t seeds[kSeedBuffer];
    const uint32_t pid = pk.pid;
    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bot = pid << 16;
    const uint32_t iv32 = ivsRaw(pk);
    const uint32_t iv1 = iv32 & 0x7FFF;
    const uint32_t iv2 = iv32 >> 15;
    PidIv r;
    if (lcrngMatch(seeds, top, bot, iv1, iv2, r)) return r;
    if (pk.species == kUnown && lcrngUnownMatch(seeds, top, bot, iv1, iv2, r)) return r;
    // GetColoStarterMatch requires Colosseum/XD origin: never for a Gen IV-origin PK4.
    if (xdMatch(seeds, pk, top, bot, iv1, iv2, r)) return r;
    if (roamerMatch(seeds, top, bot, iv32, r)) return r;
    // GetChannelMatch requires Ruby/Sapphire origin: never for a Gen IV-origin PK4.
    if (mg4Match(seeds, pid, iv1, iv2, r)) return r;
    if (bacdMatch(seeds, pk, pid, iv1, iv2, r)) return r;
    if (modifiedPidMatch(seeds, pk, pid, iv1, iv2, r)) return r;
    return PidIv{};
}

bool method1Seed(const Pk4& pk, uint32_t& result) noexcept {
    uint32_t seeds[kSeedBuffer];
    const uint32_t iv32 = ivsRaw(pk);
    const uint32_t iv1 = iv32 & 0x7FFF, iv2 = iv32 >> 15;
    const int count = lcrng::getSeeds(seeds, pk.pid << 16, pk.pid & 0xFFFF0000u);
    for (int i = 0; i < count; ++i) {
        uint32_t s = lcrng::advance(seeds[i], 2);
        if (iv1 != lcrng::next15(s)) continue;
        if (iv2 != lcrng::next15(s)) continue;
        result = seeds[i];
        return true;
    }
    result = 0;
    return false;
}

// ======================================================================= Cute Charm / Pokewalker
namespace {
uint8_t ccGenderRatio(uint16_t species) noexcept {
    return personal(Table::HGSS, species, 0).gender();
}
bool isAzurillMale(uint32_t pid) noexcept { return pid >= 0xC8 && pid <= 0xE0; }
} // namespace

bool isCuteCharm(const Pk4& pk, uint32_t pid) noexcept {
    if (pid > 0xFF) return false;
    uint16_t species = pk.species;
    uint8_t gender = pk.gender;
    switch (pk.species) {
    case kShedinja: species = kNincada; gender = genderFromPid(pid, ccGenderRatio(kNincada)); break;
    case kWormadam: species = kBurmy; gender = 1; break;
    case kMothim: species = kBurmy; gender = 0; break;
    case kVespiquen: species = kCombee; gender = 1; break;
    case kGallade: species = kKirlia; gender = 0; break;
    case kFroslass: species = kSnorunt; gender = 1; break;
    case kMarill: case kAzumarill:
        if (isAzurillMale(pid)) { species = kAzurill; gender = 0; }
        break;
    default: break;
    }
    if (gender == 0) {
        const uint8_t gr = ccGenderRatio(species);
        if (gr >= 254) return false;
        const uint32_t nature = pid % 25;
        const uint32_t expect = 25u * ((gr / 25u) + 1u) + nature;
        return pid == expect;
    }
    if (gender == 1) {
        if (pid >= 25) return false;
        if (ccGenderRatio(species) >= 254) return false;
        return true;
    }
    return false;
}

bool isCuteCharmValid(uint16_t encSpecies, const Pk4& pk) noexcept {
    if (pk.gender != 0 && pk.gender != 1) return pk.species == kShedinja;
    if (pk.species != kMarill && pk.species != kAzumarill) return true;
    if (!isAzurillMale(pk.pid)) return true;
    return encSpecies == kAzurill;
}

uint32_t pokewalkerPid(uint16_t tid, uint16_t sid, uint32_t nature, uint8_t gender, uint8_t ratio) noexcept {
    if (nature >= 24) nature = 0;
    uint32_t pid = (((static_cast<uint32_t>(tid) ^ sid) >> 8) ^ 0xFFu) << 24;
    pid += nature - (pid % 25);
    if (ratio == 0 || ratio >= 0xFE) return pid;
    const uint8_t pidGender = (pid & 0xFF) < ratio ? 1 : 0;
    if (gender == pidGender) return pid;
    if (gender == 0) {
        pid += (((ratio - (pid & 0xFF)) / 25) + 1) * 25;
        if ((nature & 1) != (pid & 1)) pid += 25;
    } else {
        pid -= ((((pid & 0xFF) - ratio) / 25) + 1) * 25;
        if ((nature & 1) != (pid & 1)) pid -= 25;
    }
    return pid;
}

// ======================================================================= Unown, honey trees, Wurmple
namespace {
bool unownRecurse(std::array<uint8_t, 26>& forms, uint8_t form, uint32_t rand, int count, int depth) noexcept {
    if (depth == 4) return false;
    static constexpr uint8_t kStart[4] = {0, 17, 10, 22};
    static constexpr uint8_t kEnd[4] = {9, 21, 16, 25};
    int newCount = count;
    for (int i = kStart[depth]; i <= kEnd[depth]; ++i) forms[static_cast<std::size_t>(newCount++)] = static_cast<uint8_t>(i);
    if (unownRecurse(forms, form, rand, newCount, depth + 1)) return true;
    const uint32_t roll = rand % static_cast<uint32_t>(newCount);
    if (forms[roll] == form) return true;
    if (unownRecurse(forms, form, rand, count, depth + 1)) return true;
    return false;
}
bool unownInterior(uint8_t form, uint32_t rand) noexcept {
    std::array<uint8_t, 26> forms{};
    return unownRecurse(forms, form, rand, 0, 0);
}
bool ruinsFrame(uint32_t seed, uint8_t form) noexcept {
    if (form >= 26) return form == static_cast<uint8_t>(26 + ((seed >> 16) & 1));
    const uint32_t u16 = seed >> 16;
    const bool radio = (u16 % 100) < 50;
    if (radio) return true; // IsFormValidRadio accepts every form
    if (unownInterior(form, u16)) return true;
    const uint32_t next = lcrng::next16(seed);
    return unownInterior(form, next);
}
bool solaceonSingleRoom(uint8_t form) noexcept {
    constexpr uint32_t kRandom = 0b1111111111011101111011000111u;
    return form >= 32 || (kRandom & (1u << form)) == 0;
}
} // namespace

bool isUnownFormValid(const Pk4& pk, uint8_t form, bool ruinsOfAlph) noexcept {
    if (ruinsOfAlph) {
        uint32_t seed = 0;
        if (!method1Seed(pk, seed)) return true;
        return ruinsFrame(lcrng::advance(seed, 6), form);
    }
    if (solaceonSingleRoom(form)) return true;
    uint32_t seed = 0;
    if (!method1Seed(pk, seed)) return true;
    const uint32_t f = lcrng::advance(seed, 6) >> 16;
    static constexpr uint8_t kUnown0[] = {0, 1, 2, 6, 7, 9, 10, 11, 12, 14, 15, 16, 18, 19, 20, 21, 22, 23, 24, 25};
    uint8_t expect;
    if (form >= 26) expect = static_cast<uint8_t>(26 + (f & 1));
    else expect = kUnown0[f % (sizeof(kUnown0))];
    return expect == form;
}

bool isMunchlaxTree(uint32_t id32, uint8_t location) noexcept {
    static constexpr uint8_t kTrees[21] = {20, 20, 21, 22, 23, 24, 25, 25, 26, 27, 27, 28, 29, 30, 33, 36, 37, 47, 48, 49, 58};
    std::array<uint8_t, 4> t{static_cast<uint8_t>(id32 >> 24), static_cast<uint8_t>(id32 >> 16),
                             static_cast<uint8_t>(id32 >> 8), static_cast<uint8_t>(id32)};
    for (auto& b : t) b = static_cast<uint8_t>(b % 21);
    for (std::size_t i = 1; i < t.size(); ++i) {
        for (std::size_t j = 0; j < i; ++j) {
            if (t[i] != t[j]) continue;
            t[i] = static_cast<uint8_t>(t[i] + 1);
            if (t[i] >= 21) t[i] = 0;
        }
    }
    for (auto b : t) if (kTrees[b] == location) return true;
    return false;
}

bool isWurmpleEvoValid(uint32_t pid, uint16_t species) noexcept {
    const int evoVal = static_cast<int>(((pid >> 16) % 10) / 5);
    const int w = species - kSilcoon;
    const int group = ((w & 3) != w) ? -1 : (w >> 1);
    return evoVal == group;
}

// ======================================================================= wild slot properties
uint8_t SlotRef::pressureLevel() const noexcept {
    if (area->type != kGrass) return slot->levelMax;
    uint8_t levelMax = slot->levelMax;
    for (uint16_t i = 0; i < area->slotCount; ++i) {
        const auto& s = D::kSlots[area->slotStart + i];
        if (s.species != slot->species) continue;
        if (s.levelMax <= levelMax) continue;
        levelMax = s.levelMax;
    }
    return levelMax;
}

bool SlotRef::canUseRadar() const noexcept {
    constexpr uint32_t kGrassTile = 1u << 2; // GroundTileAllowed.Grass (1 << GroundTileType.Grass)
    return area->version >= D::kD && (area->groundTile & kGrassTile) != 0 && area->location != 52;
}

// ======================================================================= Method J / Method K
namespace {
struct Frame {
    const SlotRef& enc;
    uint8_t levelMin, levelMax, format;
    uint32_t seed1, seed2, seed3;
    Frame(const SlotRef& e, uint32_t seed, uint8_t min, uint8_t max, uint8_t fmt) noexcept
        : enc(e), levelMin(min), levelMax(max), format(fmt) {
        seed1 = lcrng::prev(seed);
        seed2 = lcrng::prev(seed1);
        seed3 = lcrng::prev(seed2);
    }
    uint32_t seed4() const noexcept { return lcrng::prev(seed3); }
    uint32_t prev1() const noexcept { return seed1 >> 16; }
    uint32_t prev2() const noexcept { return seed2 >> 16; }
    uint32_t prev3() const noexcept { return seed3 >> 16; }
};

bool slotValidStaticMagnet(const SlotRef& e, uint32_t rand, Lead& lead) noexcept {
    const auto& s = *e.slot;
    if (s.staticCount != 0 && rand % s.staticCount == s.staticIndex) { lead = Lead::Static; return true; }
    if (s.magnetPullCount != 0 && rand % s.magnetPullCount == s.magnetPullIndex) { lead = Lead::MagnetPull; return true; }
    lead = Lead::None;
    return false;
}

bool levelWithin(uint32_t level, uint8_t min, uint8_t max) noexcept {
    const uint8_t l = static_cast<uint8_t>(level);
    return min <= l && l <= max;
}
bool originalLevelValid(uint8_t min, uint8_t max, uint8_t format, uint32_t level) noexcept {
    if (format == 4 && min > 1) return level == min;
    return levelWithin(level, min, max);
}

/// Index of the first threshold above roll (SlotMethodH-style tables); 255 when roll is past the last.
template <std::size_t N>
uint8_t bucket(uint32_t roll, const std::array<uint8_t, N>& upper, const std::array<uint8_t, N>& slot) noexcept {
    for (std::size_t i = 0; i < N; ++i)
        if (roll < upper[i]) return slot[i];
    return 255;
}
uint8_t slotRegularH(uint32_t roll) noexcept {
    static constexpr std::array<uint8_t, 12> kUpper{20, 40, 50, 60, 70, 80, 85, 90, 94, 98, 99, 100};
    static constexpr std::array<uint8_t, 12> kSlot{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    return bucket(roll, kUpper, kSlot);
}
uint8_t slotSurfH(uint32_t roll) noexcept {
    static constexpr std::array<uint8_t, 5> kUpper{60, 90, 95, 99, 100};
    static constexpr std::array<uint8_t, 5> kSlot{0, 1, 2, 3, 4};
    return bucket(roll, kUpper, kSlot);
}

// ------------------------------------------------------------------ Method J (D/P/Pt)
namespace J {
using S = SlotRef;
bool isFishing(uint8_t t) noexcept { return t == S::kOldRod || t == S::kGoodRod || t == S::kSuperRod; }
bool cuteCharmFail(uint32_t r) noexcept { return (r / 0x5556) == 0; }
bool cuteCharmPass(uint32_t r) noexcept { return (r / 0x5556) != 0; }
bool syncPass(uint32_t r) noexcept { return (r >> 15) == 0; }
bool staticMagnetFail(uint32_t r) noexcept { return (r >> 15) != 0; }
bool staticMagnetPass(uint32_t r) noexcept { return (r >> 15) == 0; }
bool hustleVitalFail(uint32_t r) noexcept { return (r >> 15) != 1; }
bool hustleVitalPass(uint32_t r) noexcept { return (r >> 15) == 1; }
bool intimidatePass(uint32_t r) noexcept { return (r >> 15) == 1; }
bool feebasChance(uint32_t r) noexcept { return (r >> 15) == 1; }
uint32_t nature(uint32_t r) noexcept { return r / 0xA3Eu; }
bool levelRand(const S& e) noexcept { return e.type() != S::kGrass; }

int getSlot(uint8_t type, uint32_t rand) noexcept {
    const uint32_t esv = rand / 656;
    switch (type) {
    case S::kOldRod: case S::kSurf: return slotSurfH(esv);
    case S::kGoodRod: case S::kSuperRod: {
        static constexpr std::array<uint8_t, 5> kUpper{40, 80, 95, 99, 100};
        static constexpr std::array<uint8_t, 5> kSlot{0, 1, 2, 3, 4};
        return bucket(esv, kUpper, kSlot);
    }
    case S::kHoneyTree: return 0;
    default: return slotRegularH(esv);
    }
}
bool slotValid(const S& e, uint32_t rand) noexcept {
    if (e.type() == S::kHoneyTree) return true;
    return getSlot(e.type(), rand) == e.slot->slotNumber;
}
uint32_t randomLevel(const S& e, uint32_t rand) noexcept {
    if (e.type() == S::kHoneyTree) return 5 + (rand / 0x1745);
    const uint32_t mod = 1u + e.levelMax() - e.levelMin();
    return (rand % mod) + e.levelMin();
}
bool levelValid(const S& e, uint8_t min, uint8_t max, uint8_t format, uint32_t rand) noexcept {
    return originalLevelValid(min, max, format, randomLevel(e, rand));
}
bool fishPossible(uint8_t type, uint32_t& seed) noexcept {
    const uint32_t rate = type == S::kOldRod ? 25 : type == S::kGoodRod ? 50 : type == S::kSuperRod ? 75 : 0;
    if ((seed >> 16) / 656 < rate) { seed = lcrng::prev(seed); return true; }
    return false;
}
bool activation(const S& e, uint32_t seed, Lead lead, LeadSeed& result) noexcept {
    bool pass = true;
    if (isFishing(e.type())) {
        if (e.isCoronetFeebasArea()) {
            if (e.slot->species == 349 /* Feebas */ && !feebasChance(seed >> 16)) pass = false;
            else seed = lcrng::prev(seed);
        }
        if (pass) pass = fishPossible(e.type(), seed);
    } else if (e.type() == S::kHoneyTree) {
        seed = lcrng::next(seed);
    }
    result = {seed, lead};
    return pass;
}
bool canRadar(const S& e) noexcept { return e.type() == S::kGrass && e.canUseRadar(); }

bool from1Skip(const Frame& c, uint32_t& result) noexcept {
    if (levelRand(c.enc)) {
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev2())) {
            if (slotValid(c.enc, c.prev3())) { result = c.seed4(); return true; }
        }
    } else {
        if (slotValid(c.enc, c.prev2())) { result = c.seed3; return true; }
    }
    result = 0;
    return false;
}
bool regular(const Frame& c, LeadSeed& result, Lead lead = Lead::None) noexcept {
    if (levelRand(c.enc)) {
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValid(c.enc, c.prev2()) && activation(c.enc, c.seed3, lead, result)) return true;
        }
    } else {
        if (slotValid(c.enc, c.prev1()) && activation(c.enc, c.seed2, lead, result)) return true;
    }
    result = {};
    return false;
}
bool hustleVital(const Frame& c, uint32_t& result) noexcept {
    result = 0;
    if (hustleVitalFail(c.prev1())) return false;
    if (!originalLevelValid(c.levelMin, c.levelMax, c.format, c.enc.pressureLevel())) return false;
    if (levelRand(c.enc)) {
        if (slotValid(c.enc, c.prev3())) { result = c.seed4(); return true; }
    } else {
        if (slotValid(c.enc, c.prev2())) { result = c.seed3; return true; }
    }
    return false;
}
bool staticMagnet(const Frame& c, uint32_t& result, Lead& lead) noexcept {
    lead = Lead::None;
    result = 0;
    if (levelRand(c.enc)) {
        if (staticMagnetFail(c.prev3())) return false;
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValidStaticMagnet(c.enc, c.prev2(), lead)) { result = c.seed4(); return true; }
        }
    } else {
        if (staticMagnetFail(c.prev3())) return false;
        if (slotValidStaticMagnet(c.enc, c.prev1(), lead)) { result = c.seed3; return true; }
    }
    return false;
}
bool staticMagnetFailed(const Frame& c, uint32_t& result) noexcept {
    result = 0;
    if (levelRand(c.enc)) {
        if (staticMagnetPass(c.prev3())) return false;
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValid(c.enc, c.prev2())) { result = c.seed4(); return true; }
        }
    } else {
        if (staticMagnetPass(c.prev2())) return false;
        if (slotValid(c.enc, c.prev1())) { result = c.seed3; return true; }
    }
    return false;
}
bool noSync(const Frame& c, LeadSeed& result) noexcept {
    uint32_t seed = 0;
    if (c.levelMin > c.enc.levelMax()) {
        if (hustleVital(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpirit, result)) return true;
        result = {};
        return false;
    }
    if (regular(c, result)) return true;
    if (!syncPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::SynchronizeFail, result)) return true;
    if (!cuteCharmPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::CuteCharmFail, result)) return true;
    if (!hustleVitalPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpiritFail, result)) return true;
    if (staticMagnetFailed(c, seed) && activation(c.enc, seed, Lead::StaticMagnetFail, result)) return true;
    Lead sm = Lead::None;
    if (staticMagnet(c, seed, sm) && activation(c.enc, seed, sm, result)) return true;
    if (!intimidatePass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::IntimidateKeenEyeFail, result)) return true;
    if (c.levelMax >= c.enc.pressureLevel()) {
        if (hustleVital(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpirit, result)) return true;
    }
    if (canRadar(c.enc)) { result = {c.seed1, Lead::Radar}; return true; }
    result = {};
    return false;
}
bool tryMatch(const S& e, uint8_t min, uint8_t max, uint32_t seed, uint8_t nat, uint8_t format, LeadSeed& result) noexcept {
    const uint32_t p0 = seed >> 16;
    if (nature(p0) == nat) {
        const Frame c(e, seed, min, max, format);
        return noSync(c, result);
    }
    if (syncPass(p0) && min <= e.levelMax()) {
        const Frame c(e, seed, min, max, format);
        if (regular(c, result, Lead::Synchronize)) return true;
        if (canRadar(e)) { result = {c.seed1, Lead::SynchronizeRadar}; return true; }
    }
    result = {};
    return false;
}
LeadSeed originSeed(const S& e, uint32_t seed, uint8_t nat, int reverseCount, uint8_t min, uint8_t max, uint8_t format) noexcept {
    LeadSeed prefer{};
    while (true) {
        LeadSeed r{};
        if (tryMatch(e, min, max, seed, nat, format, r)) {
            if (r.lead == Lead::None) return r;
            if (static_cast<uint8_t>(r.lead) > static_cast<uint8_t>(prefer.lead)) prefer = r;
        }
        if (reverseCount == 0) return prefer;
        reverseCount--;
        seed = lcrng::reverse(seed, 2);
    }
}
bool tryCuteCharm(const S& e, const uint32_t* seeds, int count, uint8_t nat, uint8_t min, uint8_t max, uint8_t format, LeadSeed& result) noexcept {
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        if (nature(seed >> 16) != nat) continue;
        const Frame c(e, seed, min, max, format);
        if (cuteCharmFail(c.prev1())) continue;
        uint32_t s = 0;
        if (!from1Skip(c, s)) continue;
        if (!activation(e, s, Lead::CuteCharm, result)) continue;
        return true;
    }
    if (canRadar(e)) {
        for (int i = 0; i < count; ++i) {
            const uint32_t seed = seeds[i];
            if (nature(seed >> 16) != nat) continue;
            const Frame c(e, seed, min, max, format);
            if (cuteCharmFail(c.prev1())) continue;
            result = {c.seed2, Lead::CuteCharmRadar};
            return true;
        }
    }
    result = {};
    return false;
}
} // namespace J

int reversalWindow(uint32_t seed, uint8_t nat) noexcept {
    int ctr = 0;
    uint32_t b = seed >> 16;
    while (true) {
        const uint32_t a = lcrng::prev16(seed);
        const uint32_t pid = (b << 16) | a;
        if (pid % 25 == nat) break;
        b = lcrng::prev16(seed);
        ctr++;
    }
    return ctr;
}

// ------------------------------------------------------------------ Method K (HG/SS)
namespace K {
using S = SlotRef;
constexpr uint8_t kSyncAllowed = 25, kSyncDisallowed = 26, kSyncFailUnspecific = 27;
bool isFishing(uint8_t t) noexcept {
    return t == S::kOldRod || t == S::kGoodRod || t == S::kSuperRod || t == S::kSafariOldRod ||
           t == S::kSafariGoodRod || t == S::kSafariSuperRod;
}
bool cuteCharmFail(uint32_t r) noexcept { return (r % 3) == 0; }
bool cuteCharmPass(uint32_t r) noexcept { return (r % 3) != 0; }
bool syncPass(uint32_t r) noexcept { return (r & 1) == 0; }
bool staticMagnetFail(uint32_t r) noexcept { return (r & 1) != 0; }
bool staticMagnetPass(uint32_t r) noexcept { return (r & 1) == 0; }
bool hustleVitalFail(uint32_t r) noexcept { return (r & 1) != 1; }
bool hustleVitalPass(uint32_t r) noexcept { return (r & 1) == 1; }
bool intimidatePass(uint32_t r) noexcept { return (r & 1) == 1; }
uint32_t nature(uint32_t r) noexcept { return r % 25; }
bool levelRand(const S& e) noexcept { return e.type() != S::kGrass && e.type() < S::kSafariGrass; }

uint8_t getSlot(uint8_t type, uint32_t rand) noexcept {
    const uint32_t r = rand % 100;
    switch (type) {
    case S::kGrass: return slotRegularH(r);
    case S::kSurf: return slotSurfH(r);
    case S::kOldRod: case S::kGoodRod: case S::kSuperRod: {
        static constexpr std::array<uint8_t, 5> kUpper{40, 70, 85, 95, 100};
        static constexpr std::array<uint8_t, 5> kSlot{0, 1, 2, 3, 4};
        return bucket(r, kUpper, kSlot);
    }
    case S::kRockSmash: return r >= 80 ? 1 : 0;
    case S::kHeadbutt: case S::kHeadbuttSpecial: {
        static constexpr std::array<uint8_t, 6> kUpper{50, 65, 80, 90, 95, 100};
        static constexpr std::array<uint8_t, 6> kSlot{0, 1, 2, 3, 4, 5};
        return bucket(r, kUpper, kSlot);
    }
    case S::kBugContest: {
        static constexpr std::array<uint8_t, 10> kUpper{5, 10, 15, 20, 30, 40, 50, 60, 80, 100};
        static constexpr std::array<uint8_t, 10> kSlot{9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        return bucket(r, kUpper, kSlot);
    }
    case S::kSafariGrass: case S::kSafariSurf: case S::kSafariOldRod: case S::kSafariGoodRod: case S::kSafariSuperRod:
        return static_cast<uint8_t>(rand % 10);
    default: return 255;
    }
}
bool slotValid(const S& e, uint32_t rand) noexcept { return getSlot(e.type(), rand) == e.slot->slotNumber; }
uint32_t randomLevel(const S& e, uint32_t rand) noexcept {
    const uint32_t mod = 1u + e.levelMax() - e.levelMin();
    return (rand % mod) + e.levelMin();
}
bool levelValid(const S& e, uint8_t min, uint8_t max, uint8_t format, uint32_t rand) noexcept {
    return originalLevelValid(min, max, format, randomLevel(e, rand));
}
bool ableToSweetScent(Lead l) noexcept {
    return l == Lead::None || l == Lead::IntimidateKeenEyeFail || l == Lead::PressureHustleSpirit ||
           l == Lead::PressureHustleSpiritFail;
}
bool activation(const S& e, uint32_t seed, Lead lead, LeadSeed& result) noexcept {
    bool pass = true;
    const uint8_t t = e.type();
    if (isFishing(t)) {
        uint32_t rate = (t == S::kOldRod || t == S::kSafariOldRod) ? 25 : (t == S::kGoodRod || t == S::kSafariGoodRod) ? 50 : 75;
        const uint32_t roll = (seed >> 16) % 100;
        rate += 50;
        if (roll < rate) seed = lcrng::prev(seed);
        else if (lead != Lead::None) pass = false;
        else if (roll < rate * 2) { seed = lcrng::prev(seed); lead = Lead::SuctionCups; }
        else pass = false;
    } else if (t == S::kRockSmash) {
        const uint32_t roll = (seed >> 16) % 100;
        const uint32_t rate = e.area->rate;
        if (roll < rate) seed = lcrng::prev(seed);
        else if (lead != Lead::None) pass = false;
        else if (roll < rate * 2u) { seed = lcrng::prev(seed); lead = Lead::Illuminate; }
        else pass = false;
    } else if (t == S::kBugContest && !ableToSweetScent(lead)) {
        const uint32_t roll = (seed >> 16) % 100;
        if (roll >= e.area->rate) pass = false;
        else seed = lcrng::reverse(seed, 2);
    }
    result = {seed, lead};
    return pass;
}

bool from1Skip(const Frame& c, uint32_t& result) noexcept {
    if (levelRand(c.enc)) {
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev2())) {
            if (slotValid(c.enc, c.prev3())) { result = c.seed4(); return true; }
        }
    } else {
        if (slotValid(c.enc, c.prev2())) { result = c.seed3; return true; }
    }
    result = 0;
    return false;
}
bool regular(const Frame& c, LeadSeed& result, Lead lead = Lead::None) noexcept {
    if (levelRand(c.enc)) {
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValid(c.enc, c.prev2()) && activation(c.enc, c.seed3, lead, result)) return true;
        }
    } else {
        if (slotValid(c.enc, c.prev1()) && activation(c.enc, c.seed2, lead, result)) return true;
    }
    result = {};
    return false;
}
bool hustleVital(const Frame& c, uint32_t& result) noexcept {
    result = 0;
    if (hustleVitalFail(c.prev1())) return false;
    if (!originalLevelValid(c.levelMin, c.levelMax, c.format, c.enc.pressureLevel())) return false;
    if (levelRand(c.enc)) {
        if (slotValid(c.enc, c.prev3())) { result = c.seed4(); return true; }
    } else {
        if (slotValid(c.enc, c.prev2())) { result = c.seed3; return true; }
    }
    return false;
}
bool staticMagnet(const Frame& c, uint32_t& result, Lead& lead) noexcept {
    lead = Lead::None;
    result = 0;
    if (levelRand(c.enc)) {
        if (staticMagnetFail(c.prev3())) return false;
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValidStaticMagnet(c.enc, c.prev2(), lead)) { result = c.seed4(); return true; }
        }
    } else {
        if (staticMagnetFail(c.prev2())) return false;
        if (slotValidStaticMagnet(c.enc, c.prev1(), lead)) { result = c.seed3; return true; }
    }
    return false;
}
bool staticMagnetFailed(const Frame& c, uint32_t& result) noexcept {
    result = 0;
    if (levelRand(c.enc)) {
        if (staticMagnetPass(c.prev3())) return false;
        if (c.enc.isFixedLevel() || levelValid(c.enc, c.levelMin, c.levelMax, c.format, c.prev1())) {
            if (slotValid(c.enc, c.prev2())) { result = c.seed4(); return true; }
        }
    } else {
        if (staticMagnetPass(c.prev2())) return false;
        if (slotValid(c.enc, c.prev1())) { result = c.seed2; return true; }
    }
    return false;
}
bool noSync(const Frame& c, LeadSeed& result) noexcept {
    uint32_t seed = 0;
    if (c.levelMin > c.enc.levelMax()) {
        if (hustleVital(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpirit, result)) return true;
        result = {};
        return false;
    }
    if (regular(c, result)) return true;
    if (!syncPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::SynchronizeFail, result)) return true;
    if (!cuteCharmPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::CuteCharmFail, result)) return true;
    if (!hustleVitalPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpiritFail, result)) return true;
    if (staticMagnetFailed(c, seed) && activation(c.enc, seed, Lead::StaticMagnetFail, result)) return true;
    Lead sm = Lead::None;
    if (staticMagnet(c, seed, sm) && activation(c.enc, seed, sm, result)) return true;
    if (!intimidatePass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::IntimidateKeenEyeFail, result)) return true;
    if (c.levelMax >= c.enc.pressureLevel()) {
        if (hustleVital(c, seed) && activation(c.enc, seed, Lead::PressureHustleSpirit, result)) return true;
    }
    result = {};
    return false;
}
bool onlyFailSync(const Frame& c, LeadSeed& result) noexcept {
    if (c.levelMin > c.enc.levelMax()) { result = {}; return false; }
    uint32_t seed = 0;
    if (!syncPass(c.prev1()) && from1Skip(c, seed) && activation(c.enc, seed, Lead::SynchronizeFail, result)) return true;
    result = {};
    return false;
}

bool isLow5Bits31(uint32_t v) noexcept { return (v & 0x1F) == 0x1F; }
bool isAny31(uint32_t iv16) noexcept { return isLow5Bits31(iv16) || isLow5Bits31(iv16 >> 5) || isLow5Bits31(iv16 >> 10); }
bool hasAny31IV(uint32_t origin) noexcept {
    uint32_t seed = lcrng::advance(origin, 3);
    if (isAny31(seed >> 16)) return true;
    return isAny31(lcrng::next16(seed));
}

struct Search {
    const S& enc;
    uint8_t levelMin, levelMax, format;
    bool reroll31, mustFailRerolls;
    Frame frame(uint32_t seed) const noexcept { return Frame(enc, seed, levelMin, levelMax, format); }
    bool canAcceptDirect(int depth) const noexcept { return !mustFailRerolls || depth == 3; }
    bool shouldRecurse(int depth) const noexcept { return reroll31 && depth < 4; }
};

LeadSeed seedCore(const Search& ctx, uint32_t seed, int depth, uint8_t forceSync) noexcept;

bool recurseReject(const Search& ctx, uint32_t seed, LeadSeed& result, int depth, uint8_t forceSync) noexcept {
    result = {};
    const uint32_t iv2 = lcrng::prev16(seed);
    if (isAny31(iv2)) return false;
    const uint32_t iv1 = lcrng::prev16(seed);
    if (isAny31(iv1)) return false;
    seed = lcrng::reverse(seed, 3);
    result = seedCore(ctx, seed, depth, forceSync);
    return result.valid();
}

bool tryMatch(const Search& ctx, uint32_t seed, uint8_t nat, LeadSeed& result, int depth, uint8_t forceSync) noexcept {
    const uint32_t p0 = seed >> 16;
    if (nature(p0) == nat) {
        const Frame frame = ctx.frame(seed);
        if (forceSync == kSyncAllowed || forceSync == kSyncDisallowed) {
            if (noSync(frame, result) && ctx.canAcceptDirect(depth)) return true;
        } else {
            if (onlyFailSync(frame, result) && ctx.canAcceptDirect(depth)) {
                if (forceSync != kSyncFailUnspecific) result.lead = Lead::Synchronize;
                return true;
            }
        }
        if (ctx.shouldRecurse(depth)) {
            if (forceSync == kSyncAllowed || forceSync == kSyncDisallowed) {
                if (recurseReject(ctx, seed, result, depth + 1, kSyncDisallowed)) return true;
            }
            const uint32_t prev = frame.seed1;
            if (forceSync != kSyncDisallowed && !syncPass(prev >> 16) && ctx.levelMin <= ctx.enc.levelMax()) {
                const uint8_t lead = forceSync == kSyncAllowed ? kSyncFailUnspecific : forceSync;
                if (recurseReject(ctx, prev, result, depth + 1, lead)) return true;
            }
        }
    }
    if (forceSync != kSyncAllowed && forceSync != kSyncFailUnspecific) {
        if (forceSync != nat) { result = {}; return false; }
    }
    if (syncPass(p0) && ctx.levelMin <= ctx.enc.levelMax()) {
        const Frame frame = ctx.frame(seed);
        if (regular(frame, result, Lead::Synchronize) && ctx.canAcceptDirect(depth)) return true;
        if (ctx.shouldRecurse(depth)) {
            if (recurseReject(ctx, seed, result, depth + 1, nat)) return true;
        }
    }
    result = {};
    return false;
}

LeadSeed originSeed(const Search& ctx, uint32_t seed, uint8_t nat, int reverseCount, int depth, uint8_t forceSync) noexcept {
    LeadSeed prefer{};
    while (true) {
        LeadSeed r{};
        if (tryMatch(ctx, seed, nat, r, depth, forceSync)) {
            if (r.lead == Lead::None) return r;
            if (static_cast<uint8_t>(r.lead) > static_cast<uint8_t>(prefer.lead)) prefer = r;
        }
        if (reverseCount == 0) return prefer;
        reverseCount--;
        seed = lcrng::reverse(seed, 2);
    }
}

LeadSeed seedCore(const Search& ctx, uint32_t seed, int depth, uint8_t forceSync) noexcept {
    const uint32_t pid = sequentialPid(seed);
    const uint8_t nat = static_cast<uint8_t>(pid % 25);
    const int frames = reversalWindow(seed, nat);
    return originSeed(ctx, seed, nat, frames, depth, forceSync);
}

LeadSeed getSeed(const S& e, uint32_t seed, uint8_t min, uint8_t max, uint8_t format) noexcept {
    const bool reroll = e.isRerollMinimum31();
    const Search ctx{e, min, max, format, reroll, reroll && !hasAny31IV(seed)};
    return seedCore(ctx, seed, 0, kSyncAllowed);
}

bool tryCuteCharm(const S& e, const uint32_t* seeds, int count, uint8_t nat, uint8_t min, uint8_t max, uint8_t format, LeadSeed& result) noexcept {
    for (int i = 0; i < count; ++i) {
        const uint32_t seed = seeds[i];
        if (nature(seed >> 16) != nat) continue;
        const Frame c(e, seed, min, max, format);
        if (cuteCharmFail(c.prev1())) continue;
        uint32_t s = 0;
        if (!from1Skip(c, s)) continue;
        if (!activation(e, s, Lead::CuteCharm, result)) continue;
        return true;
    }
    result = {};
    return false;
}
} // namespace K
} // namespace

LeadSeed leadInfo4(const Pk4& pk, const SlotRef& slot, const PidIv& pidiv, uint8_t levelMin, uint8_t levelMax) noexcept {
    const bool hgss = isHGSS(pk.version);
    constexpr uint8_t kFormat = 4;
    PidType type = pidiv.type;
    if (type == PidType::ChainShiny) {
        if (pk.isShiny() && !hgss && slot.canUseRadar()) return {pidiv.seed, Lead::None};
        if (isCuteCharm(pk, pk.pid)) type = PidType::CuteCharm;
    } else if (type == PidType::Method_1) {
        LeadSeed r{};
        if (hgss) {
            r = K::getSeed(slot, pidiv.seed, levelMin, levelMax, kFormat);
        } else {
            const uint32_t pid = sequentialPid(pidiv.seed);
            const uint8_t nat = static_cast<uint8_t>(pid % 25);
            r = J::originSeed(slot, pidiv.seed, nat, reversalWindow(pidiv.seed, nat), levelMin, levelMax, kFormat);
        }
        if (r.valid()) return r;
        // LeadFinder.TryGetMatchRadarShiny never reports a match (its loop result is discarded).
        if (isCuteCharm(pk, pk.pid)) type = PidType::CuteCharm;
    }
    if (type == PidType::CuteCharm) {
        uint32_t seeds[kSeedBuffer];
        const int count = lcrng::getSeedsIVs(seeds, pk.ivs);
        const uint8_t nat = static_cast<uint8_t>(pk.pid % 25);
        LeadSeed r{};
        const bool ok = hgss ? K::tryCuteCharm(slot, seeds, count, nat, levelMin, levelMax, kFormat, r)
                             : J::tryCuteCharm(slot, seeds, count, nat, levelMin, levelMax, kFormat, r);
        if (ok) return r;
    }
    return {};
}

} // namespace Legality::Gen4
