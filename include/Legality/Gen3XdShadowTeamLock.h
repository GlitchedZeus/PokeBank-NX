#pragma once

#include "Legality/Gen3CxdPidIvCorrelation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Legality::Gen3XdShadowTeamLock {

enum class LockState : uint8_t {
    Normal = 0,
    Shadow = 1,
    ShadowSeen = 2,
};

struct LockEntry {
    uint8_t nature;
    uint8_t gender;
    uint8_t ratio;
    uint8_t state;
};

struct VariantEntry {
    uint16_t lockOffset;
    uint8_t lockCount;
};

struct TeamSetEntry {
    uint16_t variantOffset;
    uint8_t variantCount;
};

#include "Legality/Gen3XdShadowTeamLockData.inc"

inline constexpr uint32_t kNoTrainerShinyValue = UINT32_MAX;
inline constexpr uint32_t kDefaultIterationLimit = 16384;

enum class Result : uint8_t {
    Matched,
    NotMatched,
    SearchLimit,
};

namespace Detail {

constexpr bool isShadow(const LockEntry& lock) noexcept {
    return lock.state != static_cast<uint8_t>(LockState::Normal);
}

constexpr bool isSeen(const LockEntry& lock) noexcept {
    return lock.state == static_cast<uint8_t>(LockState::ShadowSeen);
}

constexpr uint8_t framesConsumed(const LockEntry& lock) noexcept {
    return isSeen(lock) ? 5 : 7;
}

constexpr bool matchesLock(const LockEntry& lock, uint32_t pid) noexcept {
    if (isShadow(lock) && lock.nature == 0)
        return true;

    const uint8_t gender =
        static_cast<uint8_t>((pid & 0xFFu) < lock.ratio ? 1 : 0);
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
    Search(const VariantEntry& variant, uint32_t originSeed,
           uint32_t trainerShinyValue, uint32_t iterationLimit)
        : variant_(variant),
          cache_(Gen3CxdPidIv::Detail::prev(
              Gen3CxdPidIv::Detail::prev(originSeed))),
          trainerShinyValue_(trainerShinyValue),
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
        if (iterations_ >= iterationLimit_) {
            exhausted_ = true;
            return false;
        }
        ++iterations_;
        return true;
    }

    Result recurseWith(const SeedFrame& node, int nextLock,
                       const LockEntry& current) {
        team_[teamCount_++] = node;
        const Result result = find(nextLock, node.frame, &current);
        --teamCount_;
        return result;
    }

    Result find(int lockIndex, int frame, const LockEntry* prior) {
        if (lockIndex < 0)
            return verifyNpc(frame) ? Result::Matched : Result::NotMatched;

        const LockEntry& current = lockAt(static_cast<std::size_t>(lockIndex));
        if (prior == nullptr || isShadow(*prior))
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
            const SeedFrame node{pid, ctr + framesConsumed(current)};
            const Result result = recurseWith(node, lockIndex - 1, current);
            if (result != Result::NotMatched)
                return result;
        }

        // Mirrors PKHeX TeamLockResult.GetSingleLock. The upstream logic contains
        // a non-advancing 'continue' path when a forced CPU shiny value repeats;
        // our global iteration bound turns that pathological history into
        // SearchLimit instead of allowing legality analysis to hang.
        bool forcedCpuTrainer = false;
        int start = 2;
        while (true) {
            if (!consumeIteration())
                return Result::SearchLimit;

            const uint32_t upper =
                cache_.value(static_cast<std::size_t>(start + 1));
            const uint32_t lower =
                cache_.value(static_cast<std::size_t>(start));
            const uint32_t shinyValue = (upper ^ lower) >> 3;

            if (shinyValue == trainerShinyValue_) {
                // XD anti-shiny reroll: this skipped frame can precede the lock.
            } else if (requiredCpuShinyValue_ != kNoTrainerShinyValue) {
                if (shinyValue == requiredCpuShinyValue_) {
                    requiredCpuShinyValue_ = shinyValue;
                    forcedCpuTrainer = true;
                    continue;
                }

                if (forcedCpuTrainer)
                    requiredCpuShinyValue_ = kNoTrainerShinyValue;
                return Result::NotMatched;
            }

            const SeedFrame node{pid, start + framesConsumed(current)};
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
                const uint32_t upper =
                    cache_.value(static_cast<std::size_t>(p7 + 1));
                const uint32_t lower =
                    cache_.value(static_cast<std::size_t>(p7));
                const uint32_t pid = (upper << 16) | lower;
                const uint32_t shinyValue = (upper ^ lower) >> 3;

                if (shinyValue == trainerShinyValue_) {
                    // Player TSV makes this an ignored XD anti-shiny interrupt.
                } else if (matchesLock(prior, pid)) {
                    if (requiredCpuShinyValue_ != kNoTrainerShinyValue) {
                        if (shinyValue != requiredCpuShinyValue_) {
                            if (forcedCpuTrainer)
                                requiredCpuShinyValue_ = kNoTrainerShinyValue;
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
                const SeedFrame node{pid, ctr + framesConsumed(current)};
                const Result result = recurseWith(node, lockIndex - 1, current);
                if (result != Result::NotMatched)
                    return result;
            }
            ctr += 2;
        }
    }

    bool verifyNpc(int ctr) {
        const uint32_t tid =
            cache_.value(static_cast<std::size_t>(ctr + 1));
        const uint32_t sid =
            cache_.value(static_cast<std::size_t>(ctr));
        const uint32_t cpuShinyValue = (tid ^ sid) >> 3;
        if (requiredCpuShinyValue_ != kNoTrainerShinyValue &&
            requiredCpuShinyValue_ != cpuShinyValue)
            return false;

        int pos = static_cast<int>(teamCount_) - 1;
        for (std::size_t rev = teamCount_; rev > 0; --rev, --pos) {
            const uint32_t pid = team_[rev - 1].pid;
            const uint32_t psv = ((pid & 0xFFFFu) ^ (pid >> 16)) >> 3;
            if (psv != cpuShinyValue)
                continue;

            if (!isShadow(lockAt(static_cast<std::size_t>(pos))))
                return false;
            if (trainerShinyValue_ != kNoTrainerShinyValue)
                return false;
        }
        return true;
    }

    const VariantEntry& variant_;
    FrameCache cache_;
    uint32_t trainerShinyValue_ = kNoTrainerShinyValue;
    uint32_t requiredCpuShinyValue_ = kNoTrainerShinyValue;
    uint32_t iterationLimit_ = kDefaultIterationLimit;
    uint32_t iterations_ = 0;
    bool exhausted_ = false;
    std::array<SeedFrame, 5> team_{};
    std::size_t teamCount_ = 0;
};

} // namespace Detail

inline Result validate(uint8_t shadowIndex, uint32_t originSeed,
                       uint32_t trainerShinyValue = kNoTrainerShinyValue,
                       uint32_t iterationLimit = kDefaultIterationLimit) {
    if (shadowIndex == 0 || shadowIndex >= kShadowIndexToTeamSet.size())
        return Result::NotMatched;

    const uint8_t setIndex = kShadowIndexToTeamSet[shadowIndex];
    if (setIndex == 0xFF || setIndex >= kTeamSets.size())
        return Result::NotMatched;

    const TeamSetEntry& set = kTeamSets[setIndex];
    if (set.variantCount == 0)
        return Result::Matched;

    bool limited = false;
    for (uint8_t i = 0; i < set.variantCount; ++i) {
        const VariantEntry& variant = kVariants[set.variantOffset + i];
        Detail::Search search(
            variant, originSeed, trainerShinyValue, iterationLimit);
        const Result result = search.run();
        if (result == Result::Matched)
            return Result::Matched;
        if (result == Result::SearchLimit)
            limited = true;
    }
    return limited ? Result::SearchLimit : Result::NotMatched;
}

inline Result validateXd(uint8_t shadowIndex, uint32_t originSeed,
                         uint16_t tid, uint16_t sid,
                         uint32_t iterationLimit = kDefaultIterationLimit) {
    const uint32_t tsv = static_cast<uint32_t>(tid ^ sid) >> 3;
    return validate(shadowIndex, originSeed, tsv, iterationLimit);
}

inline constexpr std::size_t kTeamSetCount = kTeamSets.size();
inline constexpr std::size_t kTeamVariantCount = kVariants.size();
inline constexpr std::size_t kLockCount = kLocks.size();

} // namespace Legality::Gen3XdShadowTeamLock
