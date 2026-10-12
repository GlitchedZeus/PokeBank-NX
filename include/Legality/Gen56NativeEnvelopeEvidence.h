#pragma once
// Original PK5 / PK6 transport envelope (Gen V/VI) -- READ ONLY.
//
// Reference: Project Pokemon BW PK5 Structure (box 136 / party 220 bytes)
// and XY PKM Structure (box 232 / party 260 bytes).
// The first 8 bytes of each PKM contain an unencrypted header but the
// species, moves, origins and full checksum payload require format-specific
// decryption and shuffle. Never report a legal species or valid checksum
// from the envelope alone. Unknown physical source / acquisition stays Unknown.
//
// This is a bounded entry point for the planned real PK5/PK6 parsers and
// actual Gen V 3DS editor. No allocation, writes, static data or RTTI.
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen56NativeEnvelope {
enum class Format : uint8_t { Unknown, PK5, PK6 };
enum class Layout : uint8_t { Unknown, Stored, Party };
enum class Status : uint8_t { UnrecognizedLength, FormatMismatch, HeaderOnly };

constexpr std::size_t PK5StoredBytes=136u;
constexpr std::size_t PK5PartyBytes=220u;
constexpr std::size_t PK6StoredBytes=232u;
constexpr std::size_t PK6PartyBytes=260u;

struct Inspection {
    Format format=Format::Unknown;
    Layout layout=Layout::Unknown;
    Status status=Status::UnrecognizedLength;
    uint32_t pidOrEncryptionConstant=0; // PK5 PID; PK6 EC, not the PID
    uint16_t rawChecksum=0;             // NOT authenticated or verified
    bool decryptedFieldsAvailable=false;
    bool sourceHistoryAuthenticated=false;
};
constexpr Format formatForLength(std::size_t size) noexcept {
    if(size==PK5StoredBytes || size==PK5PartyBytes)return Format::PK5;
    if(size==PK6StoredBytes || size==PK6PartyBytes)return Format::PK6;
    return Format::Unknown;
}
constexpr Layout layoutForLength(std::size_t size) noexcept {
    if(size==PK5StoredBytes || size==PK6StoredBytes)return Layout::Stored;
    if(size==PK5PartyBytes || size==PK6PartyBytes)return Layout::Party;
    return Layout::Unknown;
}
constexpr uint16_t u16(std::span<const std::byte> raw,
                       std::size_t offset) noexcept {
    return static_cast<uint16_t>(
        std::to_integer<uint8_t>(raw[offset]) |
        (static_cast<uint16_t>(std::to_integer<uint8_t>(raw[offset+1]))<<8));
}
constexpr uint32_t u32(std::span<const std::byte> raw,
                       std::size_t offset) noexcept {
    return static_cast<uint32_t>(u16(raw,offset)) |
           (static_cast<uint32_t>(u16(raw,offset+2))<<16);
}
// Length is checked BEFORE any raw indexing. A length match is sufficient
// only to examine the opaque header, never to pronounce any PKM legitimate.
constexpr Inspection inspect(std::span<const std::byte> raw,
                             Format claimed=Format::Unknown) noexcept {
    Inspection result{};
    const auto found=formatForLength(raw.size());
    if(found==Format::Unknown)return result;
    if(claimed!=Format::Unknown && claimed!=found) {
        result.status=Status::FormatMismatch;
        return result;
    }
    result.format=found;
    result.layout=layoutForLength(raw.size());
    result.pidOrEncryptionConstant=u32(raw,0);
    result.rawChecksum=u16(raw,6);
    result.status=Status::HeaderOnly;
    return result;
}

static_assert(formatForLength(PK5StoredBytes)==Format::PK5);
static_assert(formatForLength(PK5PartyBytes)==Format::PK5);
static_assert(formatForLength(PK6StoredBytes)==Format::PK6);
static_assert(formatForLength(PK6PartyBytes)==Format::PK6);
static_assert(formatForLength(0)==Format::Unknown);
static_assert(layoutForLength(PK5PartyBytes)==Layout::Party);
static_assert(layoutForLength(PK6StoredBytes)==Layout::Stored);
} // namespace Legality::Gen56NativeEnvelope
