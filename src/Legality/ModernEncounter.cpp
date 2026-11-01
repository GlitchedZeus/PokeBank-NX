// ModernEncounter.cpp - encounter facts as per-species unions: BallVerifier's ball sets and
// MiscFatefulEncounterVerifier's fateful flag, and the unhatched-egg facts of EncounterVerifier.
// The tables are include/Legality/ModernEncounterData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dumpencounters") and checked against the pinned PKHeX in CI.
#include "Legality/ModernEncounter.h"

#include <algorithm>
#include <array>
#include <span>

namespace Legality::ModernEncounter {
namespace {

// One raid's RaidRNG.Verify parameters (GenerateParam8); see the oracle's Raid().
struct RaidParam8 {
    uint8_t genderRatio, flawless, ability, toxtricity;
    std::array<int8_t, 6> ivs;  // HP, Atk, Def, SpA, SpD, Spe; -1 = random
};

#include "Legality/ModernEncounterData.inc"

template <typename T>
struct Tables {
    std::span<const T> pk6, pk7, pb7, pk8, pb8, pa8, pk9, pa9;
    std::span<const T> get(Modern::Format format) const noexcept {
        using F = Modern::Format;
        switch (format) {
        case F::PK6: return pk6;
        case F::PK7: return pk7;
        case F::PB7: return pb7;
        case F::PK8: return pk8;
        case F::PB8: return pb8;
        case F::PA8: return pa8;
        case F::PK9: return pk9;
        case F::PA9: return pa9;
        case F::Unknown: break;
        }
        return {};
    }
};
constexpr Tables<uint64_t> kBalls{kBalls6, kBalls7, kBalls7B, kBalls8, kBalls8B, kBalls8A, kBalls9, kBalls9A};
constexpr Tables<uint8_t> kFateful{kFateful6, kFateful7, kFateful7B, kFateful8, kFateful8B, kFateful8A, kFateful9, kFateful9A};
// Formats without eggs (Let's Go, Legends) have no egg table.
constexpr Tables<uint8_t> kEggs{kEgg6, kEgg7, {}, kEgg8, kEgg8B, {}, kEgg9, {}};

bool switchFormat(Modern::Format f) noexcept {
    using F = Modern::Format;
    return f == F::PK8 || f == F::PB8 || f == F::PA8 || f == F::PK9 || f == F::PA9;
}

// The species' entry in one format; 0 when it has no generated encounter there. HOME moves
// records between every Switch format, including back to an older game (a Scarlet/Violet gift
// in Sword/Shield), which the generator for the older format does not yield: a Switch format
// takes the union over all of them and Let's Go once the format itself has an entry.
template <typename T>
T entry(const Tables<T>& tables, Modern::Format format, uint16_t species) noexcept {
    using F = Modern::Format;
    auto one = [&](F f) -> T {
        const auto t = tables.get(f);
        return species < t.size() ? t[species] : T{0};
    };
    T value = one(format);
    if (value != 0 && switchFormat(format)) {
        for (F other : {F::PB7, F::PK8, F::PB8, F::PA8, F::PK9, F::PA9})
            value = static_cast<T>(value | one(other));
    }
    return value;
}

} // namespace

bool ballPossible(Modern::Format format, uint16_t species, uint8_t ball) noexcept {
    if (ball >= 64) return true;
    const uint64_t allowed = entry(kBalls, format, species);
    return allowed == 0 || ((allowed >> ball) & 1) != 0;
}

bool fatefulPossible(Modern::Format format, uint16_t species, bool fateful) noexcept {
    const uint8_t allowed = entry(kFateful, format, species);
    return allowed == 0 || (allowed & (fateful ? 1 : 2)) != 0;
}

bool gigantamaxPossible(uint16_t species, uint8_t form, bool flag, bool egg) noexcept {
    if (species + 1u >= kGmax8Index.size()) return true;
    const uint32_t at = kGmax8Index[species] + form;
    if (at >= kGmax8Index[species + 1u]) return true;
    const uint8_t v = kGmax8[at];
    if ((v & 0x80) == 0) return true;
    return (v & ((flag ? 1 : 2) << (egg ? 2 : 0))) != 0;
}

EggFacts eggFacts(Modern::Format format, uint16_t species) noexcept {
    // The format's own table only: no egg is ever carried into another format.
    const auto t = kEggs.get(format);
    const uint8_t v = species < t.size() ? t[species] : 0;
    EggFacts f;
    if ((v & 0x80) == 0) return f;
    f.decided = true;
    f.exists = (v & 1) != 0;
    f.gift = (v & 2) != 0;
    f.nicknameFlagSet = (v & 4) != 0;
    f.nicknameFlagClear = (v & 8) != 0;
    f.eggLocationZero = (v & 16) != 0;
    f.nonEggEggLocation = (v & 32) != 0;
    f.lineHasEgg = (v & 64) != 0;
    return f;
}

bool relearnPossible(Modern::Format origin, uint16_t species, uint16_t move, bool fromEgg) noexcept {
    std::span<const uint32_t> index;
    std::span<const uint16_t> moves;
    if (origin == Modern::Format::PK6) {
        index = fromEgg ? std::span<const uint32_t>(kRelearnEggIndex6) : kRelearnIndex6;
        moves = fromEgg ? std::span<const uint16_t>(kRelearnEgg6) : kRelearn6;
    } else if (origin == Modern::Format::PK7) {
        index = fromEgg ? std::span<const uint32_t>(kRelearnEggIndex7) : kRelearnIndex7;
        moves = fromEgg ? std::span<const uint16_t>(kRelearnEgg7) : kRelearn7;
    } else {
        return true;
    }
    if (static_cast<size_t>(species) + 1 >= index.size()) return true;
    const auto list = moves.subspan(index[species], index[species + 1] - index[species]);
    if (list.size() == 1 && list[0] == 0xFFFF) return true;  // undecided
    return std::binary_search(list.begin(), list.end(), move);
}

bool hatchLocation(uint8_t version, uint16_t location) noexcept {
    std::span<const uint16_t> list;
    switch (version) {
    case 24: case 25: list = kHatch6XY; break;
    case 26: case 27: list = kHatch6AO; break;
    case 30: case 31: list = kHatch7SM; break;
    case 32: case 33: list = kHatch7USUM; break;
    case 44: case 45: list = kHatch8SWSH; break;
    case 48: list = kHatch8BD; break;
    case 49: list = kHatch8SP; break;
    case 50: list = kHatch9SL; break;
    case 51: list = kHatch9VL; break;
    default: return true;
    }
    return std::binary_search(list.begin(), list.end(), location);
}

namespace {

// Xoroshiro128Plus as PKHeX.Core implements it.
struct Xoro {
    static constexpr uint64_t kConst = 0x82A2B175229D6A5Bull;
    uint64_t s0, s1;
    explicit Xoro(uint64_t seed) noexcept : s0(seed), s1(kConst) {}
    static uint64_t rotl(uint64_t x, int k) noexcept { return (x << k) | (x >> (64 - k)); }
    uint64_t next() noexcept {
        const uint64_t a = s0, result = s0 + s1;
        uint64_t b = s1 ^ a;
        s0 = rotl(a, 24) ^ b ^ (b << 16);
        s1 = rotl(b, 37);
        return result;
    }
    uint64_t nextInt(uint64_t max = 0xFFFFFFFFull) noexcept {
        // GetBitmask: the smallest all-ones mask covering max - 1.
        const uint64_t m = max - 1;
        uint64_t mask = 0;
        while (mask < m) mask = (mask << 1) | 1;
        for (;;) {
            const uint64_t r = next() & mask;
            if (r < max) return r;
        }
    }
};

// Xoroshiro128PlusReversal.ExploreDouble: seeds whose 1st and 3rd outputs (low 32 bits) are given.
bool exploreDouble(uint64_t& seed, uint32_t out1, uint32_t out2, uint8_t assume2, uint8_t carry1, uint8_t carry2) noexcept {
    constexpr uint64_t C = Xoro::kConst;
    const uint64_t baseSeed = static_cast<uint32_t>(out1 - static_cast<uint32_t>(C));
    const uint64_t bitsCheck = baseSeed & 7;
    uint64_t x0 = baseSeed ^ C;
    uint64_t x1_ = (baseSeed >> 19) ^ (x0 >> 6) ^ 0x56;
    uint64_t x2_ = (x0 >> 16) ^ 0x65;
    uint64_t x3_ = x1_ ^ (x0 >> 27);
    uint64_t x4_ = x2_ ^ (x0 >> 27);
    const uint64_t x5 = (baseSeed >> 16) ^ x0 ^ 0x2B1;
    const uint64_t x6 = (baseSeed >> 3) ^ (x0 >> 11) ^ 0xE0A;
    const uint64_t x7 = C ^ (x0 >> 24);
    const uint64_t t0 = out2 >> 16;
    const uint64_t t1 = out2 >> 27;
    const uint64_t assume = baseSeed | (uint64_t{assume2} << 43);
    x1_ ^= assume2;
    x2_ ^= assume2;
    x3_ ^= assume2;
    x4_ ^= assume2;
    const uint64_t sub0 = t0 - carry1;
    uint64_t tmp = ((((sub0 - x3_) ^ x4_) & 0x1F) << 32) | assume;
    x0 = tmp ^ C;
    tmp |= (((sub0 - (x1_ ^ (x0 >> 27))) ^ x2_ ^ (x0 >> 27)) & 0xFF) << 32;
    x0 = tmp ^ C;
    uint64_t r = (((uint64_t{out2} - (x5 ^ (x0 >> 27) ^ (x0 >> 24))) ^ x6 ^ (x0 >> 27)) & 0x1FFF);
    if ((r >> 10) != bitsCheck) { seed = 0; return false; }
    tmp |= (r & 0x3FF) << 54;
    x0 = tmp ^ C;
    const uint64_t x8 = (tmp >> 30) ^ (x0 >> 17) ^ (x0 >> 54);
    const uint64_t x9 = (tmp >> 43) ^ (tmp >> 51) ^ (x0 >> 27) ^ (x0 >> 54) ^ 3;
    const uint64_t x10 = x8 ^ (x0 >> 38);
    const uint64_t x11 = x9 ^ (x0 >> 38);
    const uint64_t x12 = x7 ^ (x0 >> 35) ^ (tmp >> 48);
    const uint64_t x13 = (tmp >> 19) ^ (x0 >> 6);
    const uint64_t x14 = x13 ^ (x0 >> 27);
    const uint64_t sub1 = t1 - carry2;
    seed = ((((sub1 - x10) ^ x11) & 3) << 51) | tmp;
    x0 = seed ^ C;
    seed |= ((((sub0 - (x14 ^ (x0 >> 43))) >> 8) ^ x12 ^ (x0 >> 51)) & 3) << 40;
    x0 = seed ^ C;
    seed |= (((sub1 - (x8 ^ (x0 >> 38))) ^ x9 ^ (x0 >> 38)) & 7) << 51;
    x0 = seed ^ C;
    r = ((((sub0 - (x13 ^ (x0 >> 27) ^ (x0 >> 43))) >> 8) ^ x7 ^ (x0 >> 35) ^ (seed >> 48) ^ (x0 >> 51)) & 0x7F);
    if ((assume2 & 0xFu) != (r >> 3)) return false;
    seed |= r << 40;
    Xoro check(seed);
    if (static_cast<uint32_t>(check.next()) != out1) return false;
    (void)check.next();
    return static_cast<uint32_t>(check.next()) == out2;
}

template <typename F>
bool forEachSeed(uint32_t first, uint32_t third, F&& f) {
    // XoroMachineSkip: every assumption of bits 43-50 and both carries.
    for (unsigned assume = 0; assume < 256; ++assume)
        for (uint8_t carry2 = 0; carry2 < 2; ++carry2)
            for (uint8_t carry1 = 0; carry1 < 2; ++carry1) {
                uint64_t seed = 0;
                if (exploreDouble(seed, first, third, static_cast<uint8_t>(assume), carry1, carry2) && f(seed)) return true;
            }
    return false;
}

uint32_t shinyXor(uint32_t v) noexcept { return (v ^ (v >> 16)) & 0xFFFF; }

constexpr std::array<uint8_t, 13> kToxNature0{3, 4, 2, 8, 9, 19, 22, 11, 13, 14, 0, 6, 24};
constexpr std::array<uint8_t, 12> kToxNature1{1, 5, 7, 10, 12, 15, 16, 17, 18, 20, 21, 23};

// RaidRNG.Verify, made a superset: either forceNoShiny state; Ability Patch, HOME size resets
// and the HOME re-roll of a 0/0 size are all allowed.
bool raidVerify(const RaidRecord& pk, uint64_t seed, const RaidParam8& p, bool forceNoShiny) noexcept {
    Xoro rng(seed);
    if (static_cast<uint32_t>(rng.nextInt()) != pk.ec) return false;
    const auto trId = static_cast<uint32_t>(rng.nextInt());
    auto pid = static_cast<uint32_t>(rng.nextInt());
    bool shiny = shinyXor(pid ^ trId) < 16;
    if (shiny && forceNoShiny) {
        pid ^= 0x10000000u;  // ForceShinyState(false) against the fake trainer
        shiny = false;
    }
    if (shiny) return false;  // the record is not shiny (checked by the caller)
    if (shinyXor(pid ^ pk.id32) < 16) pid ^= 0x10000000u;
    if (pid != pk.pid) return false;
    std::array<int, 6> ivs{};
    for (size_t i = 0; i < 6; ++i) ivs[i] = p.ivs[i];
    int perfect = 0;
    for (int v : ivs) perfect += v == 31;
    for (int i = perfect; i < p.flawless; ++i) {
        auto index = static_cast<size_t>(rng.nextInt(6));
        while (ivs[index] != -1) index = static_cast<size_t>(rng.nextInt(6));
        ivs[index] = 31;
    }
    for (int& v : ivs)
        if (v == -1) v = static_cast<int>(rng.nextInt(32));
    for (size_t i = 0; i < 6; ++i)
        if (ivs[i] != pk.ivs[i]) return false;
    int ability = p.ability == 0 ? static_cast<int>(rng.nextInt(3)) : p.ability == 1 ? static_cast<int>(rng.nextInt(2)) : p.ability == 3 ? 2 : 0;
    if (ability == 2 && pk.abilityNumber != 4 && !pk.homeTracker) return false;
    switch (p.genderRatio) {
    case 255: if (pk.gender != 2) return false; break;
    case 254: if (pk.gender != 1) return false; break;
    case 0: if (pk.gender != 0) return false; break;
    default: {
        const int gender = static_cast<int>(rng.nextInt(253)) + 1 < p.genderRatio ? 1 : 0;
        if (pk.gender != gender && pk.gender != 2) return false;
    }
    }
    int nature;
    if (p.toxtricity) {
        if (pk.form == 0) nature = kToxNature0[rng.nextInt(kToxNature0.size())];
        else nature = kToxNature1[rng.nextInt(kToxNature1.size())];
    } else {
        nature = static_cast<int>(rng.nextInt(25));
    }
    if (pk.nature != nature) return false;
    const auto h = static_cast<uint8_t>(rng.nextInt(0x81) + rng.nextInt(0x80));
    const auto w = static_cast<uint8_t>(rng.nextInt(0x81) + rng.nextInt(0x80));
    if (pk.homeTracker && ((h == 0 && w == 0) || (pk.height == 0 && pk.weight == 0))) return true;
    if (pk.height != h) return false;
    return pk.weight == w || (h == 0 && pk.weight == 0 && pk.homeTracker);
}

bool hasLocation(uint32_t set, uint16_t location) noexcept {
    return set < kRaidLocations8.size() && location < 256 && ((kRaidLocations8[set][location >> 6] >> (location & 63)) & 1) != 0;
}

}  // namespace

RaidResult raidSeed(const RaidRecord& pk) noexcept {
    // Pokemon Den (162) and Max Lair (244) are raid-only met locations in Sword/Shield's own text;
    // elsewhere a wandering symbol encounter may have crossed over from another area.
    constexpr uint16_t kDen = 162, kLair = 244;
    const uint16_t loc = pk.metLocation;
    if (loc >= 256 || pk.species + 1u >= kRaid8Index.size() || pk.species >= kRaidNonRaid8.size()) return RaidResult::Undecided;
    const uint16_t nonRaid = kRaidNonRaid8[pk.species];
    if (nonRaid == 0xFFFF) return RaidResult::Undecided;
    if ((nonRaid & 0x8000) != 0 && loc != kDen && loc != kLair) return RaidResult::Undecided;
    if (hasLocation(nonRaid & 0x7FFF, loc)) return RaidResult::Undecided;
    if (shinyXor(pk.pid ^ pk.id32) < 16) return RaidResult::Undecided;
    bool any = false;
    for (uint32_t i = kRaid8Index[pk.species]; i < kRaid8Index[pk.species + 1u]; ++i) {
        const uint32_t e = kRaid8[i];
        if (!hasLocation(e >> 16, loc)) continue;
        any = true;
        const RaidParam8& p = kRaidParams8[e & 0xFFFF];
        for (uint32_t third : {pk.pid, pk.pid ^ 0x10000000u})
            if (forEachSeed(pk.ec, third, [&](uint64_t seed) { return raidVerify(pk, seed, p, false) || raidVerify(pk, seed, p, true); }))
                return RaidResult::SeedFound;
    }
    return any ? RaidResult::NoSeed : RaidResult::Undecided;
}

} // namespace Legality::ModernEncounter
