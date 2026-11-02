#pragma once

// Y in a Gen III-V Create/Edit draft: re-roll IVs and EVs immediately in the WORKING draft,
// with no confirmation, and every press gives a new result.
//
// Format facts this relies on (no new rules invented):
//   * Gen III PK3: IVs live in the IV32 word (5 bits each; bit 30 egg, bit 31 ability slot),
//     separate from the PID (pokeemerald MON_DATA_*_IV / MON_DATA_ABILITY_NUM).
//   * Gen IV PK4 / Gen V PK5: IVs live in IV32 at 0x38 (bits 30/31 egg / nicknamed), separate
//     from the PID at 0x00 (PKHeX PK4/PK5 IV32).
//   * EVs are six bytes, each 0..255, total <= 510 (PKHeX EffortValues.Max510).
// Shininess, gender, nature and ability are PID-derived (Gen V nature byte 0x41 and hidden
// ability bit 0x42 are separate fields too), so changing only IVs/EVs can never change them:
// the locked traits are preserved by construction and no PID is regenerated. This is
// format-valid generation only; it says nothing about legality.

#include <array>
#include <cstdint>

namespace PokeBank::UIModel::SharedStatReroll {

inline constexpr int kMaxIv = 31;
inline constexpr int kMaxEvEach = 255;
inline constexpr int kMaxEvTotal = 510;

// xorshift32: small, deterministic for tests; the live UI seeds it from Utils::rand32().
struct Rng {
    uint32_t state = 0x9E3779B9u;
    constexpr explicit Rng(uint32_t seed = 0x9E3779B9u) noexcept : state(seed ? seed : 0x9E3779B9u) {}
    constexpr uint32_t next() noexcept {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }
    constexpr int below(int bound) noexcept { return static_cast<int>(next() % static_cast<uint32_t>(bound)); }
};

struct Stats {
    std::array<uint8_t, 6> ivs{};
    std::array<uint8_t, 6> evs{};
    constexpr bool operator==(const Stats&) const = default;
};

constexpr std::array<uint8_t, 6> randomIvs(Rng& rng) noexcept {
    std::array<uint8_t, 6> ivs{};
    for (auto& iv : ivs) iv = static_cast<uint8_t>(rng.below(kMaxIv + 1));
    return ivs;
}

// Distribute a random total (0..510) across the six stats; each byte stays 0..255.
constexpr std::array<uint8_t, 6> randomEvs(Rng& rng) noexcept {
    std::array<int, 6> evs{};
    int remaining = rng.below(kMaxEvTotal + 1);
    for (int guard = 0; remaining > 0 && guard < 64; ++guard) {
        const int stat = rng.below(6);
        const int room = kMaxEvEach - evs[static_cast<std::size_t>(stat)];
        if (room <= 0) continue;
        int add = 1 + rng.below(remaining < 128 ? remaining : 128);
        if (add > room) add = room;
        evs[static_cast<std::size_t>(stat)] += add;
        remaining -= add;
    }
    std::array<uint8_t, 6> out{};
    for (std::size_t i = 0; i < 6; ++i) out[i] = static_cast<uint8_t>(evs[i]);
    return out;
}

constexpr bool valid(const Stats& s) noexcept {
    int total = 0;
    for (std::size_t i = 0; i < 6; ++i) {
        if (s.ivs[i] > kMaxIv) return false;
        total += s.evs[i];
    }
    return total <= kMaxEvTotal;
}

// A new IV/EV set that differs from `previous` (every Y press visibly changes something).
constexpr Stats reroll(Rng& rng, const Stats& previous) noexcept {
    Stats next{};
    for (int attempt = 0; attempt < 8; ++attempt) {
        next.ivs = randomIvs(rng);
        next.evs = randomEvs(rng);
        if (!(next == previous)) return next;
    }
    next.ivs[0] = static_cast<uint8_t>((previous.ivs[0] + 1) % (kMaxIv + 1));
    return next;
}

inline constexpr const char* kRerollHint = "Reroll IVs/EVs";
// No success status: a reroll is a routine edit and posts no toast (Round 11 H22, UI/EditFeedback.h).

} // namespace PokeBank::UIModel::SharedStatReroll
