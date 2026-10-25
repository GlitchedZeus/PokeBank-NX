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
inline constexpr size_t kDateOffset = 0xAC;
inline constexpr size_t kDateEnd = 0xB0;

enum class ParseError : uint8_t { None, WrongSize, BadChecksum };
enum class Eligibility : uint8_t { Unverified, DeclaredAllowed, DeclaredDisallowed };
enum class PayloadRelation : uint8_t { Unverified, Different, Exact, ReceiptDateOnly };

// CCITT-FALSE variant (poly 0x1021, init 0xFFFF, non-reflected).
// Verified against all 81 pinned EventsGallery full cards (63 released, 18
// unreleased). This establishes the checksum dialect, not their authenticity.
constexpr uint16_t crc16(std::span<const uint8_t> bytes) noexcept {
    uint16_t crc = 0xFFFF;
    for (const auto byte : bytes) {
        crc ^= static_cast<uint16_t>(byte) << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = static_cast<uint16_t>((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

// Exact full-PGF identity, with a separately labelled receipt-date-only relation.
// This never ignores a non-date byte or treats a malformed date as a wildcard.
[[nodiscard]] inline PayloadRelation comparePayloads(std::span<const uint8_t> distributor,
                                                    std::span<const uint8_t> pgf) noexcept {
    if (distributor.size()!=kPgfSize || pgf.size()!=kPgfSize) return PayloadRelation::Unverified;
    if (std::equal(pgf.begin(),pgf.end(),distributor.begin())) return PayloadRelation::Exact;
    if (!std::all_of(distributor.begin() + kDateOffset, distributor.begin() + kDateEnd,
                     [](uint8_t b) { return b == 0; }) ||
        !std::equal(pgf.begin(), pgf.begin() + kDateOffset, distributor.begin()) ||
        !std::equal(pgf.begin() + kDateEnd, pgf.end(), distributor.begin() + kDateEnd))
        return PayloadRelation::Different;
    const unsigned year = pgf[kDateOffset + 2] | (pgf[kDateOffset + 3] << 8);
    const unsigned month = pgf[kDateOffset + 1], day = pgf[kDateOffset];
    constexpr unsigned days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (year == 0 || year > 9999 || month == 0 || month > 12 || day == 0)
        return PayloadRelation::Different;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return day <= days[month - 1] + (month == 2 && leap)
               ? PayloadRelation::ReceiptDateOnly : PayloadRelation::Different;
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
    // A distributor card has no receipt date. Catalogued PGFs commonly have
    // one filled in (PGF.cs: day AC, month AD, little-endian year AE/AF).
    // Keep exact comparison exact; expose this narrower relation separately.
    [[nodiscard]] PayloadRelation payloadRelation(std::span<const uint8_t> pgf) const noexcept {
        if (!checksumValid() || pgf.size() != kPgfSize) return PayloadRelation::Unverified;
        return comparePayloads(data.first(kPgfSize),pgf);
    }
    // Raw full-card flags are NOT the low-nibble PGF catalog mask. These four
    // observed masks have pinned archive witnesses: B=10, W=20, BW=30,
    // B/W/B2/W2=F0. Do not invent the ordering of isolated sequel bits or
    // silently discard unknown bits; other combinations remain unverified.
    [[nodiscard]] uint8_t catalogVersionClaims() const noexcept {
        if (!checksumValid()) return 0;
        switch (versionFlags) {
        case 0x10: return 0x02; // PKHeX game21 Black
        case 0x20: return 0x01; // game20 White
        case 0x30: return 0x03;
        case 0xF0: return 0x0F;
        default: return 0;
        }
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
        const auto claims = catalogVersionClaims();
        if (claims == 0) return Eligibility::Unverified;
        const uint8_t versionBit = static_cast<uint8_t>(1u << (gameVersion - 20));
        return ((claims & versionBit) && language == receivingLanguage)
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
