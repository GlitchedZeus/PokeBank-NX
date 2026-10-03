#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Legality::Gen3ColoShadowTeamLock {

enum class TeamSet : uint8_t {
    First = 0,
    ColoMakuhita = 1,
    Gligar = 2,
    Murkrow = 3,
    Heracross = 4,
    Ursaring = 5,
};

enum class Result : uint8_t {
    Matched,
    NotMatched,
    SearchLimit,
};

struct LockEntry {
    uint8_t nature;
    uint8_t gender;
    uint8_t ratio;
};

struct VariantEntry {
    uint8_t lockOffset;
    uint8_t lockCount;
};

#include "Legality/Gen3ColoShadowTeamLockData.inc"

inline constexpr uint32_t kDefaultIterationLimit = 16384;
inline constexpr uint32_t kNoShinyValue = UINT32_MAX;

namespace Detail {

constexpr bool matchesLock(const LockEntry& lock, uint32_t pid) noexcept {
    const uint8_t gender = static_cast<uint8_t>((pid & 0xFFu) < lock.ratio ? 1 : 0);
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
        seeds_.push_back(origin);
        values_.push_back(origin >> 16);
    }

    uint32_t value(std::size_t index) {
        ensure(index);
        return values_[index];
    }

private:
    void ensure(std::size_t index) {
        while (index >= seeds_.size()) {
            const uint32_t seed = Gen3CxdPidIv::Detail::prev(seeds_.back());
            seeds_.push_back(seed);
            values_.push_back(seed >> 16);
        }
    }

    std::vector<uint32_t> seeds_;
    std::vector<uint32_t> values_;
};

class Search {
public:
    Search(const VariantEntry& variant, uint32_t originSeed, uint32_t iterationLimit)
        : variant_(variant),
          cache_(Gen3CxdPidIv::Detail::prev(
              Gen3CxdPidIv::Detail::prev(originSeed))),
          iterationLimit_(iterationLimit) {}

    Result run() {
        if (variant_.lockCount == 0)
            return Result::Matched;
        return find(static_cast<int>(variant_.lockCount) - 1, 0, nullptr);
    }

private:
    const LockEntry& lockAt(std::size_t index) const {
        return kLocks[variant_.lockOffset + index];
    }

    bool consumeIteration() {
        if (iterations_ >= iterationLimit_)
            return false;
        ++iterations_;
        return true;
    }

    Result recurseWith(const SeedFrame& node, int nextLock, const LockEntry& current) {
        team_[teamCount_++] = node;
        const Result result = find(nextLock, node.frame, &current);
        --teamCount_;
        return result;
    }

    Result find(int lockIndex, int frame, const LockEntry* prior) {
        if (lockIndex < 0)
            return verifyNpc(frame) ? Result::Matched : Result::NotMatched;

        const LockEntry& current = lockAt(static_cast<std::size_t>(lockIndex));
        if (prior == nullptr)
            return single(lockIndex, frame, current);
        return all(lockIndex, frame, current, *prior);
    }

    Result single(int lockIndex, int ctr, const LockEntry& current) {
        const uint32_t pid =
            (cache_.value(static_cast<std::size_t>(ctr + 1)) << 16) |
             cache_.value(static_cast<std::size_t>(ctr));
        if (!matchesLock(current, pid))
            return Result::NotMatched;

        {
            const SeedFrame node{pid, ctr + 7};
            const Result result = recurseWith(node, lockIndex - 1, current);
            if (result != Result::NotMatched)
                return result;
        }

        bool forcedCpuTrainer = false;
        int start = 2;
        while (true) {
            if (!consumeIteration())
                return Result::SearchLimit;

            const uint32_t upper = cache_.value(static_cast<std::size_t>(start + 1));
            const uint32_t lower = cache_.value(static_cast<std::size_t>(start));
            const uint32_t shinyValue = (upper ^ lower) >> 3;

            if (requiredCpuShinyValue_ != kNoShinyValue) {
                // Mirrors PKHeX TeamLockResult.GetSingleLock. The source contains
                // a non-advancing repeat branch; the global bound converts that
                // pathological history into SearchLimit instead of a hang.
                if (shinyValue == requiredCpuShinyValue_) {
                    forcedCpuTrainer = true;
                    continue;
                }
                if (forcedCpuTrainer)
                    requiredCpuShinyValue_ = kNoShinyValue;
                return Result::NotMatched;
            }

            const SeedFrame node{pid, start + 7};
            const Result result = recurseWith(node, lockIndex - 1, current);
            if (result != Result::NotMatched)
                return result;
            start += 2;
        }
    }

    Result all(int lockIndex, int ctr, const LockEntry& current,
               const LockEntry& prior) {
        const int start = ctr;
        bool forcedCpuTrainer = false;

        while (true) {
            if (!consumeIteration())
                return Result::SearchLimit;

            const int p7 = ctr - 7;
            if (p7 > start) {
                const uint32_t upper = cache_.value(static_cast<std::size_t>(p7 + 1));
                const uint32_t lower = cache_.value(static_cast<std::size_t>(p7));
                const uint32_t pid = (upper << 16) | lower;
                const uint32_t shinyValue = (upper ^ lower) >> 3;

                if (matchesLock(prior, pid)) {
                    if (requiredCpuShinyValue_ != kNoShinyValue) {
                        if (shinyValue != requiredCpuShinyValue_) {
                            if (forcedCpuTrainer)
                                requiredCpuShinyValue_ = kNoShinyValue;
                            return Result::NotMatched;
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
                const SeedFrame node{pid, ctr + 7};
                const Result result = recurseWith(node, lockIndex - 1, current);
                if (result != Result::NotMatched)
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

        // Normal Colosseum prior-party locks are all ordinary NPC Pokémon.
        // Any one of them being shiny to the generated CPU trainer invalidates
        // that reverse history; unlike XD, there is no player-TSV anti-shiny rule.
        for (std::size_t rev = teamCount_; rev > 0; --rev) {
            const uint32_t pid = team_[rev - 1].pid;
            const uint32_t psv = ((pid & 0xFFFFu) ^ (pid >> 16)) >> 3;
            if (psv == cpuShinyValue)
                return false;
        }
        return true;
    }

    const VariantEntry& variant_;
    FrameCache cache_;
    uint32_t requiredCpuShinyValue_ = kNoShinyValue;
    uint32_t iterationLimit_ = kDefaultIterationLimit;
    uint32_t iterations_ = 0;
    std::array<SeedFrame, 3> team_{};
    std::size_t teamCount_ = 0;
};

} // namespace Detail

inline Result validate(TeamSet set, uint32_t originSeed,
                       uint32_t iterationLimit = kDefaultIterationLimit) {
    const std::size_t index = static_cast<std::size_t>(set);
    if (index >= kSetToVariant.size())
        return Result::NotMatched;

    const int8_t variantIndex = kSetToVariant[index];
    if (variantIndex < 0)
        return Result::Matched; // PKHeX Encounters3ColoTeams.First = []

    Detail::Search search(kVariants[static_cast<std::size_t>(variantIndex)],
                          originSeed, iterationLimit);
    return search.run();
}

inline constexpr std::size_t kTeamSetCount = kSetToVariant.size();
inline constexpr std::size_t kTeamVariantCount = kVariants.size();
inline constexpr std::size_t kLockCount = kLocks.size();

} // namespace Legality::Gen3ColoShadowTeamLock
