#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Legality::Gen3ColoEReaderShadowEvidence {

inline constexpr uint32_t kDefaultIterationLimit = 100000;
inline constexpr uint32_t kNoShinyValue = UINT32_MAX;

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t otGender = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    bool isEgg = false;
    bool fateful = false;
    uint32_t pid = 0;
    std::array<uint8_t, 6> ivs{};
};

struct Result {
    bool identityMatched = false;
    bool matched = false;
    bool searchLimited = false;
    uint8_t encounter = 0xFF;
    uint32_t originSeed = 0;
};

namespace Detail {

struct LockEntry {
    uint8_t nature;
    uint8_t gender;
    uint8_t ratio;
};

inline constexpr std::array<LockEntry, 4> kTogepiLocks{{
    {23, 0, 127},
    {8, 0, 127},
    {24, 0, 127},
    {22, 1, 31},
}};

inline constexpr std::array<LockEntry, 4> kMareepLocks{{
    {4, 1, 191},
    {10, 1, 127},
    {12, 1, 127},
    {16, 1, 127},
}};

inline constexpr std::array<LockEntry, 4> kScizorLocks{{
    {13, 1, 191},
    {2, 2, 255},
    {3, 0, 127},
    {11, 0, 127},
}};

constexpr bool zeroIvs(const std::array<uint8_t, 6>& ivs) noexcept {
    for (const uint8_t iv : ivs) {
        if (iv != 0)
            return false;
    }
    return true;
}

constexpr int encounterIndex(const Candidate& c) noexcept {
    // Pinned Encounters3Colo.EReader rows. These are Japanese-only, always
    // stored with GameCube origin 15, all-zero IVs, male OT gender, no egg,
    // no fateful flag and exact level/location in native PK3.
    if (c.originGame != 15 || c.language != 1 || c.otGender != 0 ||
        c.metLocation != 128 || c.isEgg || c.fateful || !zeroIvs(c.ivs))
        return -1;
    if (c.species == 175 && c.metLevel == 20)
        return 0;
    if (c.species == 179 && c.metLevel == 37)
        return 1;
    if (c.species == 212 && c.metLevel == 50)
        return 2;
    return -1;
}

constexpr bool matchesLock(const LockEntry& lock, uint32_t pid) noexcept {
    const uint8_t gender = lock.ratio == 255
        ? 2
        : static_cast<uint8_t>((pid & 0xFFu) < lock.ratio ? 1 : 0);
    if (lock.gender != 2 && lock.gender != gender)
        return false;
    return lock.nature == (pid % 25u);
}

struct SeedFrame {
    uint32_t pid = 0;
    int frame = 0;
};

class FrameCache {
public:
    explicit FrameCache(uint32_t origin) {
        seeds_.push_back(Gen3CxdPidIv::Detail::prev(
            Gen3CxdPidIv::Detail::prev(origin)));
        values_.push_back(seeds_.back() >> 16);
    }

    uint32_t value(std::size_t index) {
        while (index >= seeds_.size()) {
            const uint32_t seed = Gen3CxdPidIv::Detail::prev(seeds_.back());
            seeds_.push_back(seed);
            values_.push_back(seed >> 16);
        }
        return values_[index];
    }

private:
    std::vector<uint32_t> seeds_;
    std::vector<uint32_t> values_;
};

enum class SearchResult : uint8_t {
    Matched,
    NotMatched,
    SearchLimit,
};

class Search {
public:
    Search(const std::array<LockEntry, 4>& locks, uint32_t originSeed,
           uint32_t iterationLimit)
        : locks_(locks), cache_(originSeed), iterationLimit_(iterationLimit) {}

    SearchResult run() {
        return find(3, 0, nullptr);
    }

private:
    bool consumeIteration() noexcept {
        if (iterations_ >= iterationLimit_)
            return false;
        ++iterations_;
        return true;
    }

    SearchResult recurseWith(const SeedFrame& node, int nextLock,
                             const LockEntry& current) {
        team_[teamCount_++] = node;
        const SearchResult result = find(nextLock, node.frame, &current);
        --teamCount_;
        return result;
    }

    SearchResult find(int lockIndex, int frame, const LockEntry* prior) {
        if (lockIndex < 0)
            return verifyNpc(frame) ? SearchResult::Matched
                                    : SearchResult::NotMatched;
        const LockEntry& current = locks_[static_cast<std::size_t>(lockIndex)];
        if (prior == nullptr)
            return single(lockIndex, frame, current);
        return all(lockIndex, frame, current, *prior);
    }

    SearchResult single(int lockIndex, int ctr, const LockEntry& current) {
        const uint32_t pid =
            (cache_.value(static_cast<std::size_t>(ctr + 1)) << 16) |
             cache_.value(static_cast<std::size_t>(ctr));
        if (!matchesLock(current, pid))
            return SearchResult::NotMatched;

        {
            const SearchResult result = recurseWith(
                {pid, ctr + 7}, lockIndex - 1, current);
            if (result != SearchResult::NotMatched)
                return result;
        }

        bool forcedCpuTrainer = false;
        int start = 2;
        while (true) {
            if (!consumeIteration())
                return SearchResult::SearchLimit;

            const uint32_t upper =
                cache_.value(static_cast<std::size_t>(start + 1));
            const uint32_t lower =
                cache_.value(static_cast<std::size_t>(start));
            const uint32_t shinyValue = (upper ^ lower) >> 3;

            if (requiredCpuShinyValue_ != kNoShinyValue) {
                // Mirror the pinned TeamLockResult repeat branch exactly; the
                // dedicated bound makes the non-advancing path fail safely.
                if (shinyValue == requiredCpuShinyValue_) {
                    forcedCpuTrainer = true;
                    continue;
                }
                if (forcedCpuTrainer)
                    requiredCpuShinyValue_ = kNoShinyValue;
                return SearchResult::NotMatched;
            }

            const SearchResult result = recurseWith(
                {pid, start + 7}, lockIndex - 1, current);
            if (result != SearchResult::NotMatched)
                return result;
            start += 2;
        }
    }

    SearchResult all(int lockIndex, int ctr, const LockEntry& current,
                     const LockEntry& prior) {
        const int start = ctr;
        bool forcedCpuTrainer = false;

        while (true) {
            if (!consumeIteration())
                return SearchResult::SearchLimit;

            const int p7 = ctr - 7;
            if (p7 > start) {
                const uint32_t upper =
                    cache_.value(static_cast<std::size_t>(p7 + 1));
                const uint32_t lower =
                    cache_.value(static_cast<std::size_t>(p7));
                const uint32_t pid = (upper << 16) | lower;
                const uint32_t shinyValue = (upper ^ lower) >> 3;

                if (matchesLock(prior, pid)) {
                    if (requiredCpuShinyValue_ != kNoShinyValue) {
                        if (shinyValue != requiredCpuShinyValue_) {
                            if (forcedCpuTrainer)
                                requiredCpuShinyValue_ = kNoShinyValue;
                            return SearchResult::NotMatched;
                        }
                    } else {
                        requiredCpuShinyValue_ = shinyValue;
                        forcedCpuTrainer = true;
                    }
                }
            }

            const uint32_t pid =
                (cache_.value(static_cast<std::size_t>(ctr + 1)) << 16) |
                 cache_.value(static_cast<std::size_t>(ctr));
            if (matchesLock(current, pid)) {
                const SearchResult result = recurseWith(
                    {pid, ctr + 7}, lockIndex - 1, current);
                if (result != SearchResult::NotMatched)
                    return result;
            }
            ctr += 2;
        }
    }

    bool verifyNpc(int ctr) {
        const uint32_t tid = cache_.value(static_cast<std::size_t>(ctr + 1));
        const uint32_t sid = cache_.value(static_cast<std::size_t>(ctr));
        const uint32_t cpuShinyValue = (tid ^ sid) >> 3;
        if (requiredCpuShinyValue_ != kNoShinyValue &&
            requiredCpuShinyValue_ != cpuShinyValue)
            return false;

        for (std::size_t rev = teamCount_; rev > 0; --rev) {
            const uint32_t pid = team_[rev - 1].pid;
            const uint32_t psv = ((pid & 0xFFFFu) ^ (pid >> 16)) >> 3;
            if (psv == cpuShinyValue)
                return false;
        }
        return true;
    }

    const std::array<LockEntry, 4>& locks_;
    FrameCache cache_;
    uint32_t requiredCpuShinyValue_ = kNoShinyValue;
    uint32_t iterationLimit_ = kDefaultIterationLimit;
    uint32_t iterations_ = 0;
    std::array<SeedFrame, 4> team_{};
    std::size_t teamCount_ = 0;
};

constexpr uint32_t next4(uint32_t seed) noexcept {
    for (int i = 0; i < 4; ++i)
        seed = Gen3CxdPidIv::Detail::next(seed);
    return seed;
}

inline const std::array<LockEntry, 4>& locksFor(int index) noexcept {
    if (index == 0)
        return kTogepiLocks;
    if (index == 1)
        return kMareepLocks;
    return kScizorLocks;
}

} // namespace Detail

inline Result analyze(const Candidate& candidate,
                      uint32_t iterationLimit = kDefaultIterationLimit) {
    Result out{};
    const int index = Detail::encounterIndex(candidate);
    if (index < 0)
        return out;

    out.identityMatched = true;
    out.encounter = static_cast<uint8_t>(index);

    const uint32_t top = candidate.pid & 0xFFFF0000u;
    const uint32_t bottom = candidate.pid << 16;
    const auto seeds = Gen3CxdPidIv::Detail::reversePid(top, bottom);
    bool limited = false;
    for (std::size_t i = 0; i < seeds.count; ++i) {
        const uint32_t origin = Detail::next4(seeds.values[i]);
        Detail::Search search(Detail::locksFor(index), origin, iterationLimit);
        const auto result = search.run();
        if (result == Detail::SearchResult::Matched) {
            out.matched = true;
            out.originSeed = origin;
            return out;
        }
        if (result == Detail::SearchResult::SearchLimit)
            limited = true;
    }
    out.searchLimited = limited;
    return out;
}

} // namespace Legality::Gen3ColoEReaderShadowEvidence
