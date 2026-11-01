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

} // namespace Legality::ModernEncounter
