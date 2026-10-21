// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// Read-only evidence reader for the 0x2D0-byte Generation V distributor
// WC5FULL format. This is NOT an authentication or legality oracle.
// Original field map: https://projectpokemon.org/home/docs/gen-5/5th-generation-wonder-card-full-map-r148/
// Independent layout reference: PKHeX.Core/MysteryGifts/WC5Full.cs.
// A valid checksum does not establish an authentic historic distributor.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen5Gift::FullCard {
inline constexpr size_t kSize = 0x2D0;
inline constexpr size_t kPgfSize = 0xCC;
inline constexpr size_t kVersionOffset = 0xCE;
inline constexpr size_t kLanguageOffset = 0x2CB;
inline constexpr size_t kChecksumOffset = 0x2CE;

enum class ParseError : uint8_t { None, WrongSize, BadChecksum };
enum class Eligibility : uint8_t { Unverified, DeclaredAllowed, DeclaredDisallowed };

// CCITT-FALSE variant (poly 0x1021, init 0xFFFF, non-reflected); recorded
// beside each archive as an implementation assumption until verified against
// an independently sourced, unaltered WC5FULL binary.
constexpr uint16_t crc16(std::span<const uint8_t> bytes) noexcept {
    uint16_t crc = 0xFFFF;
    for (const auto byte : bytes) {
        crc ^= static_cast<uint16_t>(byte) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

struct View {
    std::span<const uint8_t> data; // borrowed from caller; never mutated
    ParseError error = ParseError::WrongSize;
    uint16_t versionFlags = 0;  // full little-endian u16; unknown bits retained
    uint8_t language = 0;
    uint16_t storedChecksum = 0;

    [[nodiscard]] bool checksumValid() const noexcept { return error == ParseError::None; }
    [[nodiscard]] std::span<const uint8_t> payload() const noexcept {
        return checksumValid() ? data.first(kPgfSize) : std::span<const uint8_t>{};
    }
    [[nodiscard]] bool matchesPayload(std::span<const uint8_t> pgf) const noexcept {
        return checksumValid() && pgf.size() == kPgfSize &&
               std::equal(pgf.begin(), pgf.end(), data.begin());
    }
    // Describes the card's encoded claim ONLY. The distributor's provenance,
    // language vs DS firmware interpretation, redemption history, and catalog
    // pairing must be verified separately before any Pokémon verdict changes.
    [[nodiscard]] Eligibility declaredEligibility(uint8_t gameVersion, uint8_t receivingLanguage) const noexcept {
        if (!checksumValid() || gameVersion < 20 || gameVersion > 23 ||
            (language != 1 && language != 2 && language != 3 && language != 4 &&
             language != 5 && language != 7 && language != 8) ||
            (receivingLanguage != 1 && receivingLanguage != 2 && receivingLanguage != 3 &&
             receivingLanguage != 4 && receivingLanguage != 5 && receivingLanguage != 7 &&
             receivingLanguage != 8))
            return Eligibility::Unverified;
        const uint16_t versionBit = static_cast<uint16_t>(1u << (gameVersion - 20));
        return ((versionFlags & versionBit) && language == receivingLanguage)
                   ? Eligibility::DeclaredAllowed : Eligibility::DeclaredDisallowed;
    }
};

[[nodiscard]] inline View parse(std::span<const uint8_t> raw) noexcept {
    View result{};
    if (raw.size() != kSize) return result;
    result.data = raw;
    result.versionFlags = static_cast<uint16_t>(raw[kVersionOffset] | (raw[kVersionOffset + 1] << 8));
    result.language = raw[kLanguageOffset];
    result.storedChecksum = static_cast<uint16_t>(raw[kChecksumOffset] | (raw[kChecksumOffset + 1] << 8));
    result.error = crc16(raw.first(kChecksumOffset)) == result.storedChecksum
                       ? ParseError::None : ParseError::BadChecksum;
    return result;
}
} // namespace Legality::Gen5Gift::FullCard
