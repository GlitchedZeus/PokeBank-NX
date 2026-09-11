#include "Integration/Gen2/Gen2ReadOnlySave.h"

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
    for (std::size_t i = 0; i < capacity; ++i) {
        const bool present = presentMarker(bytes[ofs + 1 + i]);
        if (present != (i < count)) return false;
    }
    // Both PKSM-Core's probe and the retail list representation require the first empty marker
    // immediately after the logical list to be 0xFF.
    if (bytes[ofs + 1 + count] != 0xFF) return false;
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

std::string decodeInternational(std::span<const uint8_t> bytes) {
    std::string out;
    for (const uint8_t b : bytes) {
        if (b == 0x00 || b == 0x50) break;
        if (b == 0x5D) { out += "*"; continue; }
        if (b >= 0x80 && b <= 0x99) { out += static_cast<char>('A' + b - 0x80); continue; }
        if (b >= 0xA0 && b <= 0xB9) { out += static_cast<char>('a' + b - 0xA0); continue; }
        if (b >= 0xF6) { out += static_cast<char>('0' + b - 0xF6); continue; }
        switch (b) {
            case 0x70: out += "Po"; break; case 0x71: out += "Ke"; break;
            case 0x72: out += "“"; break; case 0x73: out += "”"; break;
            case 0x75: out += "…"; break; case 0x7F: out += ' '; break;
            case 0x9A: out += '('; break; case 0x9B: out += ')'; break;
            case 0x9C: out += ':'; break; case 0x9D: out += ';'; break;
            case 0x9E: out += '['; break; case 0x9F: out += ']'; break;
            case 0xBA: out += "à"; break; case 0xBB: out += "è"; break;
            case 0xBC: out += "é"; break; case 0xBD: out += "ù"; break;
            case 0xBE: out += "À"; break; case 0xBF: out += "Á"; break;
            case 0xC0: out += "Ä"; break; case 0xC1: out += "Ö"; break;
            case 0xC2: out += "Ü"; break; case 0xC3: out += "ä"; break;
            case 0xC4: out += "ö"; break; case 0xC5: out += "ü"; break;
            case 0xC6: out += "È"; break; case 0xC7: out += "É"; break;
            case 0xC8: out += "Ì"; break; case 0xC9: out += "Í"; break;
            case 0xCA: out += "Ñ"; break; case 0xCB: out += "Ò"; break;
            case 0xCC: out += "Ó"; break; case 0xCD: out += "Ù"; break;
            case 0xCE: out += "Ú"; break; case 0xCF: out += "á"; break;
            case 0xDF: out += "←"; break; case 0xE0: out += "’"; break;
            case 0xE1: out += "Pk"; break; case 0xE2: out += "Mn"; break;
            case 0xE3: out += '-'; break; case 0xE4: out += '+'; break;
            case 0xE6: out += '?'; break; case 0xE7: out += '!'; break;
            case 0xE8: out += "․"; break; case 0xE9: out += '&'; break;
            case 0xEA: out += "é"; break; case 0xEB: out += "→"; break;
            case 0xEC: out += "▷"; break; case 0xED: out += "▶"; break;
            case 0xEE: out += "▼"; break; case 0xEF: out += "♂"; break;
            case 0xF0: out += "¥"; break; case 0xF1: out += "×"; break;
            case 0xF2: out += '.'; break; case 0xF3: out += '/'; break;
            case 0xF4: out += ','; break; case 0xF5: out += "♀"; break;
            default: out += "�"; break;
        }
    }
    return out;
}

// Gen I/II Japanese name glyph positions are compatible for the normal OT/nickname subset.
// The table below deliberately includes the characters used by retail trainer/Pokémon names and
// punctuation. Unknown control/mail-only values become U+FFFD rather than being silently guessed.
const char* jpGlyph(uint8_t b) noexcept {
    static const char* const katakana[] = {
        "ア","イ","ウ","エ","オ","カ","キ","ク","ケ","コ","サ","シ","ス","セ","ソ","タ",
        "チ","ツ","テ","ト","ナ","ニ","ヌ","ネ","ノ","ハ","ヒ","フ","ホ","マ","ミ","ム",
        "メ","モ","ヤ","ユ","ヨ","ラ","ル","レ","ロ","ワ","ヲ","ン","ッ","ャ","ュ","ョ","ィ"
    };
    if (b >= 0x80 && b <= 0xB0) return katakana[b - 0x80];
    static const char* const hiragana[] = {
        "あ","い","う","え","お","か","き","く","け","こ","さ","し","す","せ","そ","た",
        "ち","つ","て","と","な","に","ぬ","ね","の","は","ひ","ふ","へ","ほ","ま","み",
        "む","め","も","や","ゆ","よ","ら","り","る","れ","ろ","わ","を","ん","っ","ゃ","ゅ","ょ"
    };
    if (b >= 0xB1 && b <= 0xE4) return hiragana[b - 0xB1];
    switch (b) {
        case 0x05:return "ガ"; case 0x06:return "ギ"; case 0x07:return "グ"; case 0x08:return "ゲ"; case 0x09:return "ゴ";
        case 0x0A:return "ザ"; case 0x0B:return "ジ"; case 0x0C:return "ズ"; case 0x0D:return "ゼ"; case 0x0E:return "ゾ";
        case 0x0F:return "ダ"; case 0x10:return "ヂ"; case 0x11:return "ヅ"; case 0x12:return "デ"; case 0x13:return "ド";
        case 0x19:return "バ"; case 0x1A:return "ビ"; case 0x1B:return "ブ"; case 0x1C:return "ボ";
        case 0x26:return "が"; case 0x27:return "ぎ"; case 0x28:return "ぐ"; case 0x29:return "げ"; case 0x2A:return "ご";
        case 0x2B:return "ざ"; case 0x2C:return "じ"; case 0x2D:return "ず"; case 0x2E:return "ぜ"; case 0x2F:return "ぞ";
        case 0x30:return "だ"; case 0x31:return "ぢ"; case 0x32:return "づ"; case 0x33:return "で"; case 0x34:return "ど";
        case 0x3A:return "ば"; case 0x3B:return "び"; case 0x3C:return "ぶ"; case 0x3D:return "べ"; case 0x3E:return "ぼ";
        case 0x40:return "パ"; case 0x41:return "ピ"; case 0x42:return "プ"; case 0x43:return "ポ";
        case 0x44:return "ぱ"; case 0x45:return "ぴ"; case 0x46:return "ぷ"; case 0x47:return "ぺ"; case 0x48:return "ぽ";
        case 0x70:return "「"; case 0x71:return "」"; case 0x72:return "『"; case 0x73:return "』"; case 0x74:return "・";
        case 0x75:return "…"; case 0x76:return "ぁ"; case 0x77:return "ぇ"; case 0x78:return "ぉ"; case 0x7F:return "　";
        case 0xE5:return "ー"; case 0xE6:return "?"; case 0xE7:return "!"; case 0xE8:return "。";
        case 0xE9:return "ァ"; case 0xEA:return "ゥ"; case 0xEB:return "ェ"; case 0xEC:return "▷";
        case 0xED:return "▶"; case 0xEE:return "▼"; case 0xEF:return "♂"; case 0xF0:return "¥";
        case 0xF1:return "×"; case 0xF2:return "."; case 0xF3:return "/"; case 0xF4:return "ォ";
        case 0xF5:return "♀";
        default:return nullptr;
    }
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
    if (size == 7) return true; // FlashGBX >2.0 exception mirrored by PKHeX.
    return size >= 0x0C && size <= 0x30 && (size & 1u) == 0;
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
    if (region == RegionLayout::International) return decodeInternational(bytes);
    std::string out;
    for (const uint8_t b : bytes) {
        if (b == 0 || b == 0x50) break;
        if (b == 0x5D) { out += "*"; continue; }
        if (const char* glyph = jpGlyph(b)) out += glyph;
        else out += "�";
    }
    return out;
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
