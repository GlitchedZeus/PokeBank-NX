#pragma once
// L6 audit-only exact-identity native gate receipts. These do NOT authenticate
// an encounter, original retail history, or authorize an integration merge.
// No application or production Legality.cpp reference is introduced.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace PokeBank::L6::GateLedger {
enum class Lane : std::uint8_t { L1, L2, L3, L4, L5, L6, Count };
enum class Profile : std::uint8_t { Normal, ASanUBSan, Count };
enum class State : std::uint8_t {
    Blocked, MalformedReceipt, NativeOnlyVerified
};
inline constexpr std::size_t kLaneCount = static_cast<std::size_t>(Lane::Count);
inline constexpr std::size_t kProfileCount = static_cast<std::size_t>(Profile::Count);
inline constexpr std::size_t kExpectedReceipts = kLaneCount * kProfileCount;
inline constexpr std::size_t kMaxReceipts = 64;

struct Receipt {
    Lane lane = Lane::Count;
    Profile profile = Profile::Count;
    std::string_view applicationHead{};
    std::string_view applicationTree{};
    bool started = false;
    bool completed = false;
    bool timedOut = false;
    int exitCode = -1;
    std::uint16_t requiredPrograms = 0;
    std::uint16_t executedPrograms = 0;
};
struct Summary {
    State state = State::Blocked;
    std::uint16_t verified = 0;
    std::uint16_t missing = 0;
    std::uint16_t failed = 0;
    std::uint16_t wrongIdentity = 0;
    std::uint16_t duplicate = 0;
    std::uint16_t malformed = 0;
    // These remain false even if the entire native test matrix succeeds:
    // passing synthetic tests does not verify archived retail provenance.
    bool originalRetailAttested = false;
    bool globallyLegalSupported = false;
    bool globallyInvalidSupported = false;
    bool integrationAcceptanceAuthorized = false;
};
[[nodiscard]] constexpr bool validHexSha(std::string_view sha) noexcept {
    if (sha.size() != 40) return false;
    for (char digit : sha)
        if (!((digit >= '0' && digit <= '9') ||
              (digit >= 'a' && digit <= 'f'))) return false;
    return true;
}
[[nodiscard]] constexpr Summary evaluate(
    std::string_view expectedHead, std::string_view expectedTree,
    std::span<const Receipt> receipts) noexcept {
    Summary result{};
    if (!validHexSha(expectedHead) || !validHexSha(expectedTree) ||
        receipts.size() > kMaxReceipts) {
        result.state = State::MalformedReceipt;
        result.malformed = 1;
        return result;
    }
    std::array<bool, kExpectedReceipts> seen{};
    for (const Receipt& receipt : receipts) {
        const auto lane = static_cast<unsigned>(receipt.lane);
        const auto profile = static_cast<unsigned>(receipt.profile);
        if (lane >= kLaneCount || profile >= kProfileCount ||
            !validHexSha(receipt.applicationHead) ||
            !validHexSha(receipt.applicationTree) ||
            receipt.requiredPrograms == 0 ||
            receipt.executedPrograms > receipt.requiredPrograms) {
            ++result.malformed;
            continue;
        }
        const auto index = lane * kProfileCount + profile;
        if (seen[index]) {
            ++result.duplicate;
            continue;
        }
        seen[index] = true;
        if (receipt.applicationHead != expectedHead ||
            receipt.applicationTree != expectedTree) {
            ++result.wrongIdentity;
            continue;
        }
        if (!receipt.started || !receipt.completed || receipt.timedOut ||
            receipt.exitCode != 0 ||
            receipt.executedPrograms != receipt.requiredPrograms) {
            ++result.failed;
            continue;
        }
        ++result.verified;
    }
    for (bool present : seen)
        if (!present) ++result.missing;
    if (result.malformed || result.duplicate)
        result.state = State::MalformedReceipt;
    else if (result.missing || result.failed || result.wrongIdentity)
        result.state = State::Blocked;
    else
        result.state = State::NativeOnlyVerified;
    return result;
}
} // namespace PokeBank::L6::GateLedger
