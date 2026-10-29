#pragma once
// Audit-owned qualification of supplied receipts. This API checks consistency;
// it cannot authenticate a caller's claims or authorize data redistribution.
#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace PokeBank::L6::Qualification {
inline constexpr unsigned kGames = 16, kFamilies = 21, kCells = 336, kEvents = 247;
enum class Evidence : std::uint8_t { Missing, Synthetic, SourceModel, OriginalNative };
enum class State : std::uint8_t { Unknown, Malformed, ReceiptConsistent };
enum Witness : unsigned { Parent = 1, Encounter = 2, Event = 4, Transfer = 8 };
inline constexpr unsigned kWitnesses = Parent | Encounter | Event | Transfer;

constexpr bool digest(std::string_view value, unsigned width) noexcept {
    if (value.size() != width) return false;
    bool nonzero = false;
    for (char c : value) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        nonzero |= c != '0';
    }
    return nonzero;
}

struct Policy {
    std::string_view referenceRevision;
    std::string_view applicationTree;
    // Set by reviewed family policy, never copied from a submitted receipt.
    std::array<unsigned, kFamilies> requiredWitnesses{};
};
struct Cell {
    unsigned game = kGames, family = kFamilies;
    Evidence evidence = Evidence::Missing;
    std::string_view referenceRevision{}, applicationTree{}, specimenSha256{};
    std::string_view runtimeSpecimenSha256{}, runtimeLogSha256{}, historySha256{};
    unsigned originalGame = kGames, witnessed = 0;
    bool nativeDecoded = false, originalReceiptVerified = false;
    bool runtimeExecuted = false, runtimeCompleted = false, timedOut = false;
    bool comparisonAgrees = false, referenceUsePermitted = false;
    int exitCode = -1;
};
struct EventReceipt {
    unsigned index = kEvents;
    // Identity binds the index to fields, since 219 rows reuse card IDs.
    unsigned species = 0, tid = 0, sid = 0, card = 0;
    Evidence evidence = Evidence::Missing;
    std::string_view projectionRevision{}, archiveSha256{}, releaseReceiptSha256{};
    bool originalReceiptVerified = false, archiveDigestVerified = false;
    bool reviewPermitted = false, redistributionPermitted = false;
};
struct Summary {
    State state = State::Unknown;
    unsigned consistent = 0, unknown = 0, missing = 0, malformed = 0, duplicates = 0;
    bool authenticatedParity = false, acceptanceAuthorized = false;
};

constexpr bool valid(const Policy& p) noexcept {
    if (!digest(p.referenceRevision, 40) || !digest(p.applicationTree, 40)) return false;
    for (unsigned mask : p.requiredWitnesses) if (mask & ~kWitnesses) return false;
    return true;
}
constexpr State qualify(const Policy& p, const Cell& c) noexcept {
    if (!valid(p) || c.game >= kGames || c.family >= kFamilies ||
        static_cast<unsigned>(c.evidence) > static_cast<unsigned>(Evidence::OriginalNative) ||
        (c.witnessed & ~kWitnesses)) return State::Malformed;
    if (c.evidence != Evidence::OriginalNative) return State::Unknown;
    if (c.referenceRevision != p.referenceRevision || c.applicationTree != p.applicationTree ||
        !digest(c.specimenSha256, 64) || c.runtimeSpecimenSha256 != c.specimenSha256 ||
        !digest(c.runtimeLogSha256, 64) || c.originalGame != c.game ||
        !c.nativeDecoded || !c.originalReceiptVerified || !c.runtimeExecuted ||
        !c.runtimeCompleted || c.timedOut || c.exitCode != 0 ||
        !c.comparisonAgrees || !c.referenceUsePermitted) return State::Unknown;
    const auto required = p.requiredWitnesses[c.family];
    if ((c.witnessed & required) != required || (required && !digest(c.historySha256, 64)))
        return State::Unknown;
    return State::ReceiptConsistent;
}
constexpr State qualifyEvent(const EventReceipt& expected, const EventReceipt& actual) noexcept {
    if (expected.index >= kEvents || actual.index >= kEvents ||
        static_cast<unsigned>(actual.evidence) > static_cast<unsigned>(Evidence::OriginalNative))
        return State::Malformed;
    if (actual.index != expected.index || actual.species != expected.species ||
        actual.tid != expected.tid || actual.sid != expected.sid || actual.card != expected.card ||
        !digest(expected.projectionRevision, 40) ||
        actual.projectionRevision != expected.projectionRevision ||
        actual.evidence != Evidence::OriginalNative || !digest(actual.archiveSha256, 64) ||
        !digest(actual.releaseReceiptSha256, 64) || !actual.originalReceiptVerified ||
        !actual.archiveDigestVerified || !actual.reviewPermitted) return State::Unknown;
    // Redistribution remains an independent restriction even for a consistent
    // private-review receipt. The gate never copies or exports archive data.
    return State::ReceiptConsistent;
}
constexpr Summary evaluate(const Policy& p, std::span<const Cell> cells) noexcept {
    Summary result;
    if (!valid(p) || cells.size() > kCells) {
        result.state = State::Malformed; result.malformed = 1; return result;
    }
    std::array<bool, kCells> seen{};
    for (const auto& cell : cells) {
        const auto state = qualify(p, cell);
        if (state == State::Malformed) { ++result.malformed; continue; }
        const auto index = cell.game * kFamilies + cell.family;
        if (seen[index]) { ++result.duplicates; continue; }
        seen[index] = true;
        if (state == State::ReceiptConsistent) ++result.consistent;
        else ++result.unknown;
    }
    for (bool present : seen) if (!present) ++result.missing;
    if (result.malformed || result.duplicates) result.state = State::Malformed;
    else if (!result.missing && !result.unknown) result.state = State::ReceiptConsistent;
    return result;
}
} // namespace PokeBank::L6::Qualification
