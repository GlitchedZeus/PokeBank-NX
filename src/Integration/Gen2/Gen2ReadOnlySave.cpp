#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Save/RtcFooter.h"
#include "Utils/Gen2Text.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <sstream>

namespace PokeVault::Integration::Gen2 {
namespace {

struct Layout {
    RegionLayout region;
    VersionFamily family;
    std::size_t money;
    std::size_t tid;
    std::size_t currentBoxIndex;
    std::size_t boxNames;
    std::size_t party;
    std::size_t currentBoxCopy;
    std::size_t checksumEnd;
    std::size_t checksumPrimary;
    std::size_t gender;
    uint8_t boxCapacity;
    uint8_t boxCount;
    uint8_t stringLength;
    uint8_t boxNameLength;
    uint8_t splitAtBox;
};

constexpr std::size_t kNoOffset = static_cast<std::size_t>(-1);
constexpr std::size_t kChecksumStart = 0x2009;
constexpr std::size_t kStoredBody = 32;
constexpr std::size_t kPartyBody = 48;
constexpr std::size_t kPartyCapacity = 6;

constexpr Layout kIntlGS{
    RegionLayout::International, VersionFamily::GoldSilver,
    0x23DB, 0x2009, 0x2724, 0x2727, 0x288A, 0x2D6C, 0x2D68, 0x2D69, kNoOffset,
    20, 14, 11, 9, 7,
};
constexpr Layout kIntlC{
    RegionLayout::International, VersionFamily::Crystal,
    0x23DC, 0x2009, 0x2700, 0x2703, 0x2865, 0x2D10, 0x2B82, 0x2D0D, 0x3E3D,
    20, 14, 11, 9, 7,
};
constexpr Layout kJpnGS{
    RegionLayout::Japanese, VersionFamily::GoldSilver,
    0x23BC, 0x2009, 0x2705, 0x2708, 0x283E, 0x2D10, 0x2C8B, 0x2D0D, kNoOffset,
    30, 9, 6, 9, 6,
};
constexpr Layout kJpnC{
    RegionLayout::Japanese, VersionFamily::Crystal,
    0x23BE, 0x2009, 0x26E2, 0x26E5, 0x281A, 0x2D10, 0x2AE2, 0x2D0D, 0x8000,
    30, 9, 6, 9, 6,
};
constexpr std::array<Layout, 4> kLayouts{kIntlGS, kIntlC, kJpnGS, kJpnC};

uint16_t readBE16(std::span<const uint8_t> bytes, std::size_t o) noexcept {
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[o]) << 8) | bytes[o + 1]);
}
uint16_t readLE16(std::span<const uint8_t> bytes, std::size_t o) noexcept {
    return static_cast<uint16_t>(bytes[o] | (static_cast<uint16_t>(bytes[o + 1]) << 8));
}
uint32_t readBE24(std::span<const uint8_t> bytes, std::size_t o) noexcept {
    return (static_cast<uint32_t>(bytes[o]) << 16) |
           (static_cast<uint32_t>(bytes[o + 1]) << 8) | bytes[o + 2];
}

bool presentMarker(uint8_t marker) noexcept {
    return marker != 0 && marker != 0xFF;
}

bool validListHeader(std::span<const uint8_t> bytes, std::size_t ofs, std::size_t capacity) noexcept {
    if (ofs >= bytes.size() || capacity > bytes.size() - ofs - 2) return false;
    const uint8_t count = bytes[ofs];
    if (count > capacity) return false;
    // Count plus the immediate 0xFF cap define the logical list. Marker bytes after the cap can
    // legitimately retain stale species values and must not reject an otherwise-valid cartridge.
    if (bytes[ofs + 1 + count] != 0xFF) return false;
    for (std::size_t i = 0; i < count; ++i)
        if (!presentMarker(bytes[ofs + 1 + i])) return false;
    return true;
}

std::size_t listLength(std::size_t capacity, std::size_t bodySize, std::size_t stringLength) noexcept {
    return 1 + (capacity + 1) + bodySize * capacity + 2 * stringLength * capacity;
}

std::size_t storedBoxListLength(const Layout& l) noexcept {
    return listLength(l.boxCapacity, kStoredBody, l.stringLength);
}

std::size_t storedBoxStride(const Layout& l) noexcept {
    return storedBoxListLength(l) + 2; // Retail bank layout has two padding bytes after each list.
}

std::size_t storedBoxStart(const Layout& l, std::size_t box) noexcept {
    const std::size_t stride = storedBoxStride(l);
    if (box < l.splitAtBox) return 0x4000 + box * stride;
    return 0x6000 + (box - l.splitAtBox) * stride;
}

bool familyMatches(SourceGame hint, VersionFamily family) noexcept {
    return (hint == SourceGame::Crystal) == (family == VersionFamily::Crystal);
}

std::optional<std::pair<std::size_t, std::size_t>> splitPayload(std::span<const uint8_t> raw) noexcept {
    if (raw.size() == kRawSaveSize32K || raw.size() == kRawSaveSize64K)
        return std::pair{raw.size(), std::size_t{0}};
    for (const std::size_t base : {kRawSaveSize32K, kRawSaveSize64K}) {
        if (raw.size() <= base) continue;
        const std::size_t footer = raw.size() - base;
        if (isKnownRTCFooterSize(footer)) return std::pair{base, footer};
    }
    return std::nullopt;
}

bool fitsLayout(std::span<const uint8_t> payload, const Layout& l) noexcept {
    if (l.gender != kNoOffset && l.gender >= payload.size()) return false;
    if (l.checksumPrimary + 1 >= payload.size()) return false;
    if (l.currentBoxCopy + storedBoxListLength(l) > payload.size()) return false;
    if (l.party + listLength(kPartyCapacity, kPartyBody, l.stringLength) > payload.size()) return false;
    if (l.boxNames + static_cast<std::size_t>(l.boxNameLength) * l.boxCount > payload.size()) return false;
    for (std::size_t box = 0; box < l.boxCount; ++box) {
        if (storedBoxStart(l, box) + storedBoxListLength(l) > payload.size()) return false;
    }
    return true;
}

bool checksumValid(std::span<const uint8_t> payload, const Layout& l) noexcept {
    if (!fitsLayout(payload, l)) return false;
    const auto expected = calculateChecksum(payload, l.region, l.family);
    return expected == readLE16(payload, l.checksumPrimary);
}

bool structuralProbe(std::span<const uint8_t> payload, const Layout& l) noexcept {
    if (!fitsLayout(payload, l)) return false;
    if (!validListHeader(payload, l.party, kPartyCapacity)) return false;
    if (!validListHeader(payload, l.currentBoxCopy, l.boxCapacity)) return false;
    const uint8_t current = payload[l.currentBoxIndex] & 0x7F;
    return current < l.boxCount;
}

std::string decodeGen2Bytes(std::span<const uint8_t> bytes, bool japanese) {
    std::u16string decoded;
    bool first = true;
    for (const uint8_t value : bytes) {
        // 0x5D at byte zero is an in-game-trade OT marker, not a printable glyph.
        // The marker represents the whole OT name.
        if (first && value == Utils::GEN2_TRADE_OT)
            return "*";
        const char16_t glyph = Utils::gen2ToChar(value, japanese);
        if (glyph == 0)
            break;
        decoded.push_back(glyph);
        first = false;
    }
    return Utils::utf16ToUtf8(decoded);
}

bool parsePokemon(
    std::span<const uint8_t> body,
    std::span<const uint8_t> ot,
    std::span<const uint8_t> nick,
    uint8_t marker,
    RegionLayout region,
    bool party,
    PokemonRecord& out,
    std::string& detail) {
    const std::size_t expected = party ? kPartyBody : kStoredBody;
    if (body.size() != expected) { detail = "PK2 body has unexpected size"; return false; }
    const uint8_t species = body[0];
    if (species == 0 || species > 251) { detail = "PK2 body species is outside 1..251"; return false; }
    const bool egg = marker == 0xFD;
    if (!egg && marker != species) { detail = "PK2 list marker/body species mismatch"; return false; }
    if (egg == false && (marker == 0 || marker == 0xFF)) { detail = "PK2 list marker is empty"; return false; }

    out.species = species;
    out.heldItem = body[1];
    std::copy_n(body.begin() + 2, 4, out.moves.begin());
    out.trainerId = readBE16(body, 6);
    out.experience = readBE24(body, 8);
    for (std::size_t i = 0; i < 5; ++i) out.statExperience[i] = readBE16(body, 11 + i * 2);
    const uint8_t ad = body[21], ss = body[22];
    out.dvs[1] = ad >> 4;
    out.dvs[2] = ad & 0x0F;
    out.dvs[3] = ss >> 4;
    out.dvs[4] = ss & 0x0F;
    out.dvs[0] = static_cast<uint8_t>(((out.dvs[1] & 1) << 3) | ((out.dvs[2] & 1) << 2) |
                                      ((out.dvs[3] & 1) << 1) | (out.dvs[4] & 1));
    for (std::size_t i = 0; i < 4; ++i) {
        out.pp[i] = body[23 + i] & 0x3F;
        out.ppUps[i] = body[23 + i] >> 6;
    }
    out.friendship = body[27];
    out.pokerus = body[28];
    out.caughtData = readBE16(body, 29);
    out.level = body[31];
    if (out.level > 100) { detail = "PK2 cached level exceeds 100"; return false; }
    out.isEgg = egg;
    out.shiny = out.dvs[2] == 10 && out.dvs[3] == 10 && out.dvs[4] == 10 && (out.dvs[1] & 0x02) != 0;
    if (species == 201) {
        const unsigned value = (((out.dvs[1] >> 1) & 3u) << 6) |
                               (((out.dvs[2] >> 1) & 3u) << 4) |
                               (((out.dvs[3] >> 1) & 3u) << 2) |
                               ((out.dvs[4] >> 1) & 3u);
        out.form = static_cast<uint8_t>(value / 10u);
        if (out.form > 25) out.form = 25;
    }
    if (party) {
        out.status = body[32];
        out.currentHP = readBE16(body, 34);
        out.maxHP = readBE16(body, 36);
        out.attack = readBE16(body, 38);
        out.defense = readBE16(body, 40);
        out.speed = readBE16(body, 42);
        out.specialAttack = readBE16(body, 44);
        out.specialDefense = readBE16(body, 46);
    }
    out.originalTrainer = decodeGen2String(ot, region);
    out.nickname = decodeGen2String(nick, region);
    out.partyRecord = party;
    out.rawBodySize = body.size();
    std::copy(body.begin(), body.end(), out.rawBody.begin());
    return true;
}

bool parseList(
    std::span<const uint8_t> bytes,
    std::size_t ofs,
    std::size_t capacity,
    std::size_t bodySize,
    std::size_t stringLength,
    RegionLayout region,
    bool party,
    std::vector<std::optional<PokemonRecord>>& out,
    std::string& detail) {
    const std::size_t len = listLength(capacity, bodySize, stringLength);
    if (ofs > bytes.size() || len > bytes.size() - ofs) { detail = "PK2 list extends beyond payload"; return false; }
    if (!validListHeader(bytes, ofs, capacity)) { detail = "PK2 list header is malformed"; return false; }
    const uint8_t count = bytes[ofs];
    out.assign(capacity, std::nullopt);
    const std::size_t bodyStart = ofs + 1 + (capacity + 1);
    const std::size_t otStart = bodyStart + capacity * bodySize;
    const std::size_t nickStart = otStart + capacity * stringLength;
    for (std::size_t i = 0; i < count; ++i) {
        PokemonRecord p;
        if (!parsePokemon(
                bytes.subspan(bodyStart + i * bodySize, bodySize),
                bytes.subspan(otStart + i * stringLength, stringLength),
                bytes.subspan(nickStart + i * stringLength, stringLength),
                bytes[ofs + 1 + i], region, party, p, detail))
            return false;
        out[i] = std::move(p);
    }
    return true;
}

} // namespace

bool isKnownRTCFooterSize(std::size_t size) noexcept {
    return PokeVault::Save::isKnownRtcFooterSize(size);
}

uint16_t calculateChecksum(
    std::span<const uint8_t> payload, RegionLayout region, VersionFamily family) noexcept {
    const Layout* chosen = nullptr;
    for (const auto& l : kLayouts)
        if (l.region == region && l.family == family) { chosen = &l; break; }
    if (!chosen || chosen->checksumEnd >= payload.size() || kChecksumStart > chosen->checksumEnd) return 0;
    uint16_t sum = 0;
    for (std::size_t i = kChecksumStart; i <= chosen->checksumEnd; ++i)
        sum = static_cast<uint16_t>(sum + payload[i]);
    return sum;
}

std::string decodeGen2String(std::span<const uint8_t> bytes, RegionLayout region) {
    return decodeGen2Bytes(bytes, region == RegionLayout::Japanese);
}

const char* sourceGameId(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Gold: return "gold_gbc";
        case SourceGame::Silver: return "silver_gbc";
        case SourceGame::Crystal: return "crystal_gbc";
    }
    return "unknown_gbc";
}
const char* regionName(RegionLayout region) noexcept {
    return region == RegionLayout::Japanese ? "Japanese" : "International";
}
const char* familyName(VersionFamily family) noexcept {
    return family == VersionFamily::Crystal ? "Crystal" : "Gold/Silver";
}
const char* errorName(SaveError e) noexcept {
    switch (e) {
        case SaveError::None:return "none";
        case SaveError::WrongSize:return "wrong_size";
        case SaveError::UnsupportedRegion:return "unsupported_region";
        case SaveError::ChecksumMismatch:return "checksum_mismatch";
        case SaveError::AmbiguousLayout:return "ambiguous_layout";
        case SaveError::InvalidStructure:return "invalid_structure";
        case SaveError::InvalidTrainerData:return "invalid_trainer_data";
        case SaveError::InvalidParty:return "invalid_party";
        case SaveError::InvalidBox:return "invalid_box";
        case SaveError::GameHintMismatch:return "game_hint_mismatch";
    }
    return "unknown";
}

ParseResult parse(std::span<const uint8_t> raw, SourceGame hint) {
    ParseResult result;
    const auto split = splitPayload(raw);
    if (!split) {
        result.error = SaveError::WrongSize;
        result.detail = "Gen II battery save must be 0x8000/0x10000 bytes, optionally plus a known RTC footer";
        return result;
    }
    const auto [payloadSize, footerSize] = *split;
    const auto payload = raw.first(payloadSize);

    std::vector<const Layout*> candidates;
    bool anyChecksum = false;
    for (const auto& l : kLayouts) {
        if (!fitsLayout(payload, l)) continue;
        const bool checksum = checksumValid(payload, l);
        const bool structure = structuralProbe(payload, l);
        anyChecksum |= checksum;
        if (checksum && structure) candidates.push_back(&l);
    }
    if (candidates.empty()) {
        result.error = anyChecksum ? SaveError::InvalidStructure : SaveError::ChecksumMismatch;
        result.detail = anyChecksum
            ? "A supported Gen II checksum matched but strict party/current-box structure did not"
            : "No supported International/Japanese Gen II layout passed its primary checksum";
        return result;
    }
    if (candidates.size() != 1) {
        result.error = SaveError::AmbiguousLayout;
        result.detail = "Multiple Gen II layouts validated; refusing to guess";
        return result;
    }
    const Layout& l = *candidates.front();
    if (!familyMatches(hint, l.family)) {
        result.error = SaveError::GameHintMismatch;
        result.detail = "Validated Gen II save family conflicts with Gold/Silver vs Crystal source hint";
        return result;
    }

    auto save = std::make_shared<ReadOnlySave>();
    save->sourceBytes_.assign(raw.begin(), raw.end());
    save->metadata_.sourceGame = hint;
    save->metadata_.family = l.family;
    save->metadata_.region = l.region;
    save->metadata_.sourceGameId = sourceGameId(hint);
    save->metadata_.currentBox = payload[l.currentBoxIndex] & 0x7F;
    save->metadata_.boxCount = l.boxCount;
    save->metadata_.boxCapacity = l.boxCapacity;
    save->metadata_.payloadSize = payloadSize;
    save->metadata_.rtcFooterSize = footerSize;

    save->trainer_.trainerId = readBE16(payload, l.tid);
    save->trainer_.name = decodeGen2String(payload.subspan(l.tid + 2, l.stringLength), l.region);
    save->trainer_.money = readBE24(payload, l.money);
    if (save->trainer_.money > 999999) {
        result.error = SaveError::InvalidTrainerData;
        result.detail = "Gen II money exceeds retail maximum 999999";
        return result;
    }
    if (l.family == VersionFamily::Crystal) {
        const uint8_t g = payload[l.gender];
        if (g > 1) {
            result.error = SaveError::InvalidTrainerData;
            result.detail = "Crystal trainer gender byte is outside 0..1";
            return result;
        }
        save->trainer_.gender = g;
    }

    std::vector<std::optional<PokemonRecord>> partySlots;
    if (!parseList(payload, l.party, kPartyCapacity, kPartyBody, l.stringLength, l.region, true,
                   partySlots, result.detail)) {
        result.error = SaveError::InvalidParty;
        return result;
    }
    for (auto& slot : partySlots) if (slot) save->party_.push_back(std::move(*slot));

    save->boxes_.resize(l.boxCount);
    for (std::size_t box = 0; box < l.boxCount; ++box) {
        auto& dst = save->boxes_[box];
        dst.name = decodeGen2String(
            payload.subspan(l.boxNames + box * l.boxNameLength, l.boxNameLength), l.region);
        std::vector<std::optional<PokemonRecord>> slots;
        if (!parseList(payload, storedBoxStart(l, box), l.boxCapacity, kStoredBody, l.stringLength,
                       l.region, false, slots, result.detail)) {
            result.error = SaveError::InvalidBox;
            std::ostringstream os;
            os << "Box " << (box + 1) << ": " << result.detail;
            result.detail = os.str();
            return result;
        }
        dst.slots = std::move(slots);
    }

    result.save = std::move(save);
    return result;
}

} // namespace PokeVault::Integration::Gen2
