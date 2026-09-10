#include "Integration/Gen1/Gen1ReadOnlySave.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <sstream>

namespace PokeVault::Integration::Gen1 {
namespace {

struct Layout {
    RegionLayout region;
    std::size_t money;
    std::size_t tid;
    std::size_t currentBoxIndex;
    std::size_t party;
    std::size_t currentBox;
    std::size_t checksum;
    std::size_t mainLength;
    std::size_t starter;
    std::size_t pikaFriendship;
    uint8_t boxCapacity;
    uint8_t boxCount;
    uint8_t stringLength;
    std::size_t boxSize;
    std::size_t bankBoxesSize;
};

// PKHeX e15d246... SAV1Offsets.cs and PKSM-Core aa22d7... Sav1.cpp.
constexpr Layout kInternational{
    RegionLayout::International,
    0x25F3, 0x2605, 0x284C, 0x2F2C, 0x30C0, 0x3523, 0x0F8B,
    0x29C3, 0x271C, 20, 12, 11, 0x0462, 0x1A4C,
};
constexpr Layout kJapanese{
    RegionLayout::Japanese,
    0x25EE, 0x25FB, 0x2842, 0x2ED5, 0x302D, 0x3594, 0x0FFC,
    0x29B9, 0x2712, 30, 8, 6, 0x0566, 0x1598,
};
constexpr std::size_t kMainStart = 0x2598;
constexpr std::size_t kPartyCapacity = 6;
constexpr std::size_t kPartyBody = 44;
constexpr std::size_t kStoredBody = 33;

constexpr std::array<uint8_t, 256> kInternalToNational = {
    0x00,0x70,0x73,0x20,0x23,0x15,0x64,0x22,0x50,0x02,0x67,0x6C,0x66,0x58,0x5E,0x1D,
    0x1F,0x68,0x6F,0x83,0x3B,0x97,0x82,0x5A,0x48,0x5C,0x7B,0x78,0x09,0x7F,0x72,0x00,
    0x00,0x3A,0x5F,0x16,0x10,0x4F,0x40,0x4B,0x71,0x43,0x7A,0x6A,0x6B,0x18,0x2F,0x36,
    0x60,0x4C,0x00,0x7E,0x00,0x7D,0x52,0x6D,0x00,0x38,0x56,0x32,0x80,0x00,0x00,0x00,
    0x53,0x30,0x95,0x00,0x00,0x00,0x54,0x3C,0x7C,0x92,0x90,0x91,0x84,0x34,0x62,0x00,
    0x00,0x00,0x25,0x26,0x19,0x1A,0x00,0x00,0x93,0x94,0x8C,0x8D,0x74,0x75,0x00,0x00,
    0x1B,0x1C,0x8A,0x8B,0x27,0x28,0x85,0x88,0x87,0x86,0x42,0x29,0x17,0x2E,0x3D,0x3E,
    0x0D,0x0E,0x0F,0x00,0x55,0x39,0x33,0x31,0x57,0x00,0x00,0x0A,0x0B,0x0C,0x44,0x00,
    0x37,0x61,0x2A,0x96,0x8F,0x81,0x00,0x00,0x59,0x00,0x63,0x5B,0x00,0x65,0x24,0x6E,
    0x35,0x69,0x00,0x5D,0x3F,0x41,0x11,0x12,0x79,0x01,0x03,0x49,0x00,0x76,0x77,0x00,
    0x00,0x00,0x00,0x4D,0x4E,0x13,0x14,0x21,0x1E,0x4A,0x89,0x8E,0x00,0x51,0x00,0x00,
    0x04,0x07,0x05,0x08,0x06,0x00,0x00,0x00,0x00,0x2B,0x2C,0x2D,0x45,0x46,0x47,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

uint16_t readBE16(std::span<const uint8_t> bytes, std::size_t o) {
    return static_cast<uint16_t>((static_cast<uint16_t>(bytes[o]) << 8) | bytes[o + 1]);
}

uint32_t readBE24(std::span<const uint8_t> bytes, std::size_t o) {
    return (static_cast<uint32_t>(bytes[o]) << 16) |
           (static_cast<uint32_t>(bytes[o + 1]) << 8) | bytes[o + 2];
}

bool decodeBCD3(std::span<const uint8_t> raw, std::size_t o, uint32_t& out) {
    out = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        const uint8_t hi = raw[o + i] >> 4;
        const uint8_t lo = raw[o + i] & 0x0F;
        if (hi > 9 || lo > 9) return false;
        out = out * 100 + hi * 10 + lo;
    }
    return true;
}

const char* enSpecial(uint8_t b) {
    switch (b) {
        case 0x70: return "@"; case 0x71: return "#"; case 0x72: return "“";
        case 0x73: return "”"; case 0x75: return "…"; case 0x7F: return " ";
        case 0x9A: return "("; case 0x9B: return ")"; case 0x9C: return ":";
        case 0x9D: return ";"; case 0x9E: return "["; case 0x9F: return "]";
        case 0xBA: return "à"; case 0xBB: return "è"; case 0xBC: return "é";
        case 0xBD: return "ù"; case 0xBE: return "À"; case 0xBF: return "Á";
        case 0xC0: return "Ä"; case 0xC1: return "Ö"; case 0xC2: return "Ü";
        case 0xC3: return "ä"; case 0xC4: return "ö"; case 0xC5: return "ü";
        case 0xC6: return "È"; case 0xC7: return "É"; case 0xC8: return "Ì";
        case 0xC9: return "Í"; case 0xCA: return "Ñ"; case 0xCB: return "Ò";
        case 0xCC: return "Ó"; case 0xCD: return "Ù"; case 0xCE: return "Ú";
        case 0xCF: return "á"; case 0xD0: return "ì"; case 0xD1: return "í";
        case 0xD2: return "ñ"; case 0xD3: return "ò"; case 0xD4: return "ó";
        case 0xD5: return "ú"; case 0xD6: return "º"; case 0xDE: return "←";
        case 0xDF: return "'"; case 0xE0: return "’"; case 0xE1: return "{";
        case 0xE2: return "}"; case 0xE3: return "-"; case 0xE6: return "?";
        case 0xE7: return "!"; case 0xE8: return "․"; case 0xE9: return "&";
        case 0xEA: return "%"; case 0xEB: return "→"; case 0xEC: return "▷";
        case 0xED: return "▶"; case 0xEE: return "▼"; case 0xEF: return "♂";
        case 0xF0: return "¥"; case 0xF1: return "×"; case 0xF2: return ".";
        case 0xF3: return "/"; case 0xF4: return ","; case 0xF5: return "♀";
        default: return nullptr;
    }
}

const char* jpGlyph(uint8_t b) {
    // Exact populated entries from PKHeX StringConverter1.TableJP at e15d246... .
    switch (b) {
        case 0x05:return "ガ"; case 0x06:return "ギ"; case 0x07:return "グ"; case 0x08:return "ゲ"; case 0x09:return "ゴ";
        case 0x0A:return "ザ"; case 0x0B:return "ジ"; case 0x0C:return "ズ"; case 0x0D:return "ゼ"; case 0x0E:return "ゾ";
        case 0x0F:return "ダ"; case 0x10:return "ヂ"; case 0x11:return "ヅ"; case 0x12:return "デ"; case 0x13:return "ド";
        case 0x19:return "バ"; case 0x1A:return "ビ"; case 0x1B:return "ブ"; case 0x1C:return "ボ";
        case 0x26:return "が"; case 0x27:return "ぎ"; case 0x28:return "ぐ"; case 0x29:return "げ"; case 0x2A:return "ご";
        case 0x2B:return "ざ"; case 0x2C:return "じ"; case 0x2D:return "ず"; case 0x2E:return "ぜ"; case 0x2F:return "ぞ";
        case 0x30:return "だ"; case 0x31:return "ぢ"; case 0x32:return "づ"; case 0x33:return "で"; case 0x34:return "ど";
        case 0x3A:return "ば"; case 0x3B:return "び"; case 0x3C:return "ぶ"; case 0x3D:return "ベ"; case 0x3E:return "ぼ";
        case 0x40:return "パ"; case 0x41:return "ピ"; case 0x42:return "プ"; case 0x43:return "ポ";
        case 0x44:return "ぱ"; case 0x45:return "ぴ"; case 0x46:return "ぷ"; case 0x47:return "ペ"; case 0x48:return "ぽ";
        case 0x5D:return "*"; case 0x6E:return "ぃ"; case 0x6F:return "ぅ";
        case 0x70:return "「"; case 0x71:return "」"; case 0x72:return "『"; case 0x73:return "』"; case 0x74:return "・";
        case 0x75:return "⋯"; case 0x76:return "ぁ"; case 0x77:return "ぇ"; case 0x78:return "ぉ"; case 0x7F:return "　";
        case 0x80:return "ア"; case 0x81:return "イ"; case 0x82:return "ウ"; case 0x83:return "エ"; case 0x84:return "オ";
        case 0x85:return "カ"; case 0x86:return "キ"; case 0x87:return "ク"; case 0x88:return "ケ"; case 0x89:return "コ";
        case 0x8A:return "サ"; case 0x8B:return "シ"; case 0x8C:return "ス"; case 0x8D:return "セ"; case 0x8E:return "ソ";
        case 0x8F:return "タ"; case 0x90:return "チ"; case 0x91:return "ツ"; case 0x92:return "テ"; case 0x93:return "ト";
        case 0x94:return "ナ"; case 0x95:return "ニ"; case 0x96:return "ヌ"; case 0x97:return "ネ"; case 0x98:return "ノ";
        case 0x99:return "ハ"; case 0x9A:return "ヒ"; case 0x9B:return "フ"; case 0x9C:return "ホ"; case 0x9D:return "マ";
        case 0x9E:return "ミ"; case 0x9F:return "ム"; case 0xA0:return "メ"; case 0xA1:return "モ"; case 0xA2:return "ヤ";
        case 0xA3:return "ユ"; case 0xA4:return "ヨ"; case 0xA5:return "ラ"; case 0xA6:return "ル"; case 0xA7:return "レ";
        case 0xA8:return "ロ"; case 0xA9:return "ワ"; case 0xAA:return "ヲ"; case 0xAB:return "ン"; case 0xAC:return "ッ";
        case 0xAD:return "ャ"; case 0xAE:return "ュ"; case 0xAF:return "ョ"; case 0xB0:return "ィ";
        case 0xB1:return "あ"; case 0xB2:return "い"; case 0xB3:return "う"; case 0xB4:return "え"; case 0xB5:return "お";
        case 0xB6:return "か"; case 0xB7:return "き"; case 0xB8:return "く"; case 0xB9:return "け"; case 0xBA:return "こ";
        case 0xBB:return "さ"; case 0xBC:return "し"; case 0xBD:return "す"; case 0xBE:return "せ"; case 0xBF:return "そ";
        case 0xC0:return "た"; case 0xC1:return "ち"; case 0xC2:return "つ"; case 0xC3:return "て"; case 0xC4:return "と";
        case 0xC5:return "な"; case 0xC6:return "に"; case 0xC7:return "ぬ"; case 0xC8:return "ね"; case 0xC9:return "の";
        case 0xCA:return "は"; case 0xCB:return "ひ"; case 0xCC:return "ふ"; case 0xCD:return "ヘ"; case 0xCE:return "ほ";
        case 0xCF:return "ま"; case 0xD0:return "み"; case 0xD1:return "む"; case 0xD2:return "め"; case 0xD3:return "も";
        case 0xD4:return "や"; case 0xD5:return "ゆ"; case 0xD6:return "よ"; case 0xD7:return "ら"; case 0xD8:return "リ";
        case 0xD9:return "る"; case 0xDA:return "れ"; case 0xDB:return "ろ"; case 0xDC:return "わ"; case 0xDD:return "を";
        case 0xDE:return "ん"; case 0xDF:return "っ"; case 0xE0:return "ゃ"; case 0xE1:return "ゅ"; case 0xE2:return "ょ";
        case 0xE3:return "ー"; case 0xE4:return "ﾟ"; case 0xE5:return "ﾞ"; case 0xE6:return "？"; case 0xE7:return "！";
        case 0xE8:return "。"; case 0xE9:return "ァ"; case 0xEA:return "ゥ"; case 0xEB:return "ェ"; case 0xEF:return "♂";
        case 0xF0:return "¥"; case 0xF1:return "×"; case 0xF2:return "．"; case 0xF3:return "／"; case 0xF4:return "ォ";
        case 0xF5:return "♀"; case 0xF6:return "０"; case 0xF7:return "１"; case 0xF8:return "２"; case 0xF9:return "３";
        case 0xFA:return "４"; case 0xFB:return "５"; case 0xFC:return "６"; case 0xFD:return "７"; case 0xFE:return "８"; case 0xFF:return "９";
        default:return nullptr;
    }
}

bool isListHeaderValid(std::span<const uint8_t> raw, std::size_t offset, std::size_t capacity) {
    if (offset + capacity + 2 > raw.size()) return false;
    const uint8_t count = raw[offset];
    if (count > capacity) return false;
    if (raw[offset + 1 + count] != 0xFF) return false;
    for (std::size_t i = 0; i < capacity; ++i) {
        const uint8_t mark = raw[offset + 1 + i];
        const bool present = mark != 0 && mark != 0xFF;
        if (present != (i < count)) return false;
        if (present && gen1InternalToNational(mark) == 0) return false;
    }
    return true;
}

bool candidateLooksValid(std::span<const uint8_t> raw, const Layout& l) {
    if (l.checksum >= raw.size()) return false;
    if (calculateDiff8(raw.subspan(kMainStart, l.mainLength)) != raw[l.checksum]) return false;
    if (!isListHeaderValid(raw, l.party, kPartyCapacity)) return false;
    if (!isListHeaderValid(raw, l.currentBox, l.boxCapacity)) return false;
    const uint8_t current = raw[l.currentBoxIndex] & 0x7F;
    return current < l.boxCount;
}

VersionEvidence versionEvidence(std::span<const uint8_t> raw, const Layout& l) {
    const uint8_t starter = raw[l.starter];
    if (starter != 0) return starter == 0x54 ? VersionEvidence::Yellow : VersionEvidence::RedBlueFamily;
    return raw[l.pikaFriendship] != 0 ? VersionEvidence::Yellow : VersionEvidence::Unknown;
}

bool gameMatchesEvidence(SourceGame game, VersionEvidence evidence) {
    if (evidence == VersionEvidence::Unknown) return true;
    if (game == SourceGame::Yellow) return evidence == VersionEvidence::Yellow;
    return evidence != VersionEvidence::Yellow;
}

bool checkBankChecksums(std::span<const uint8_t> raw, const Layout& l, std::string& detail) {
    const std::size_t half = l.boxCount / 2;
    for (int bank = 0; bank < 2; ++bank) {
        const std::size_t start = bank == 0 ? 0x4000 : 0x6000;
        const std::size_t sumOfs = start + l.bankBoxesSize;
        if (sumOfs + 1 + half > raw.size()) {
            detail = "Gen I box checksum table exceeds raw save";
            return false;
        }
        if (calculateDiff8(raw.subspan(start, l.bankBoxesSize)) != raw[sumOfs]) {
            detail = bank == 0 ? "Gen I first box-bank checksum failed" : "Gen I second box-bank checksum failed";
            return false;
        }
        for (std::size_t i = 0; i < half; ++i) {
            const std::size_t boxStart = start + i * l.boxSize;
            if (calculateDiff8(raw.subspan(boxStart, l.boxSize)) != raw[sumOfs + 1 + i]) {
                std::ostringstream ss;
                ss << "Gen I stored box checksum failed at box " << (bank * half + i + 1);
                detail = ss.str();
                return false;
            }
        }
    }
    return true;
}

bool parsePokemon(std::span<const uint8_t> body, std::span<const uint8_t> ot,
                  std::span<const uint8_t> nick, RegionLayout region, bool party,
                  uint8_t expectedMarker, PokemonRecord& out, std::string& detail) {
    if (body.size() < (party ? kPartyBody : kStoredBody)) return false;
    if (body[0] != expectedMarker) {
        detail = "Gen I list marker does not match Pokemon body species";
        return false;
    }
    const uint16_t species = gen1InternalToNational(body[0]);
    if (species == 0 || species > 151) {
        detail = "Gen I Pokemon uses an invalid internal species id";
        return false;
    }

    out.rawSpecies = body[0];
    out.species = species;
    out.currentHP = readBE16(body, 0x01);
    out.status = body[0x04];
    out.type1 = body[0x05];
    out.type2 = body[0x06];
    out.catchRate = body[0x07];
    for (int i = 0; i < 4; ++i) out.moves[i] = body[0x08 + i];
    out.trainerId = readBE16(body, 0x0C);
    out.experience = readBE24(body, 0x0E);
    for (int i = 0; i < 5; ++i) out.statExperience[i] = readBE16(body, 0x11 + i * 2);
    const uint16_t packed = readBE16(body, 0x1B);
    const uint8_t atk = static_cast<uint8_t>((packed >> 12) & 0x0F);
    const uint8_t def = static_cast<uint8_t>((packed >> 8) & 0x0F);
    const uint8_t spe = static_cast<uint8_t>((packed >> 4) & 0x0F);
    const uint8_t spc = static_cast<uint8_t>(packed & 0x0F);
    const uint8_t hp = static_cast<uint8_t>(((atk & 1) << 3) | ((def & 1) << 2) |
                                            ((spe & 1) << 1) | (spc & 1));
    out.dvs = {hp, atk, def, spe, spc};
    for (int i = 0; i < 4; ++i) {
        out.pp[i] = body[0x1D + i] & 0x3F;
        out.ppUps[i] = static_cast<uint8_t>(body[0x1D + i] >> 6);
    }
    out.level = party ? body[0x21] : body[0x03];
    if (out.level > 100) {
        detail = "Gen I Pokemon level exceeds 100";
        return false;
    }
    if (party) {
        out.maxHP = readBE16(body, 0x22);
        out.attack = readBE16(body, 0x24);
        out.defense = readBE16(body, 0x26);
        out.speed = readBE16(body, 0x28);
        out.special = readBE16(body, 0x2A);
    }
    out.originalTrainer = decodeGen1String(ot, region);
    out.nickname = decodeGen1String(nick, region);
    out.partyRecord = party;
    out.rawBodySize = body.size();
    std::copy(body.begin(), body.end(), out.rawBody.begin());
    return true;
}

bool parseList(std::span<const uint8_t> raw, std::size_t offset, std::size_t capacity,
               std::size_t bodySize, std::size_t stringLength, RegionLayout region, bool party,
               std::vector<std::optional<PokemonRecord>>& slots, std::string& detail) {
    if (!isListHeaderValid(raw, offset, capacity)) {
        detail = "Gen I Pokemon list header is malformed";
        return false;
    }
    const std::size_t bodyStart = offset + capacity + 2;
    const std::size_t otStart = bodyStart + capacity * bodySize;
    const std::size_t nickStart = otStart + capacity * stringLength;
    const std::size_t end = nickStart + capacity * stringLength;
    if (end > raw.size()) {
        detail = "Gen I Pokemon list exceeds raw save";
        return false;
    }

    const uint8_t count = raw[offset];
    slots.assign(capacity, std::nullopt);
    for (std::size_t i = 0; i < count; ++i) {
        PokemonRecord record;
        const uint8_t marker = raw[offset + 1 + i];
        const auto body = raw.subspan(bodyStart + i * bodySize, bodySize);
        const auto ot = raw.subspan(otStart + i * stringLength, stringLength);
        const auto nick = raw.subspan(nickStart + i * stringLength, stringLength);
        if (!parsePokemon(body, ot, nick, region, party, marker, record, detail)) return false;
        slots[i] = std::move(record);
    }
    return true;
}

std::size_t storedBoxStart(const Layout& l, std::size_t box) {
    const std::size_t half = l.boxCount / 2;
    return box < half ? 0x4000 + box * l.boxSize
                      : 0x6000 + (box - half) * l.boxSize;
}

} // namespace

uint8_t calculateDiff8(std::span<const uint8_t> bytes) noexcept {
    uint8_t value = 0xFF;
    for (uint8_t b : bytes) value = static_cast<uint8_t>(value - b);
    return value;
}

uint16_t gen1InternalToNational(uint8_t rawSpecies) noexcept {
    return kInternalToNational[rawSpecies];
}

std::string decodeGen1String(std::span<const uint8_t> bytes, RegionLayout region) {
    std::string out;
    for (uint8_t b : bytes) {
        if (b == 0x00 || b == 0x50) break;
        if (b == 0x5D) { out += '*'; continue; }
        if (region == RegionLayout::Japanese) {
            if (const char* g = jpGlyph(b)) out += g;
            else out += "�";
            continue;
        }
        if (b >= 0x80 && b <= 0x99) { out += static_cast<char>('A' + (b - 0x80)); continue; }
        if (b >= 0xA0 && b <= 0xB9) { out += static_cast<char>('a' + (b - 0xA0)); continue; }
        if (b >= 0xF6) { out += static_cast<char>('0' + (b - 0xF6)); continue; }
        if (const char* g = enSpecial(b)) out += g;
        else out += "�";
    }
    return out;
}

const char* sourceGameId(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Red: return "red_gb";
        case SourceGame::Blue: return "blue_gb";
        case SourceGame::Yellow: return "yellow_gb";
    }
    return "unknown_gb";
}

const char* regionName(RegionLayout region) noexcept {
    return region == RegionLayout::Japanese ? "Japanese" : "International";
}

const char* errorName(SaveError error) noexcept {
    switch (error) {
        case SaveError::None: return "none";
        case SaveError::WrongSize: return "wrong_size";
        case SaveError::ChecksumMismatch: return "checksum_mismatch";
        case SaveError::AmbiguousRegion: return "ambiguous_region";
        case SaveError::InvalidStructure: return "invalid_structure";
        case SaveError::InvalidTrainerData: return "invalid_trainer_data";
        case SaveError::InvalidParty: return "invalid_party";
        case SaveError::InvalidBox: return "invalid_box";
        case SaveError::GameHintMismatch: return "game_hint_mismatch";
    }
    return "unknown";
}

ParseResult parse(std::span<const uint8_t> raw, SourceGame game) {
    ParseResult result;
    if (raw.size() != kRawSaveSize) {
        result.error = SaveError::WrongSize;
        result.detail = "Generation I raw battery save must be exactly 0x8000 bytes";
        return result;
    }

    const bool intl = candidateLooksValid(raw, kInternational);
    const bool jpn = candidateLooksValid(raw, kJapanese);
    if (intl == jpn) {
        result.error = intl ? SaveError::AmbiguousRegion : SaveError::ChecksumMismatch;
        result.detail = intl ? "Both Gen I regional layouts validated; refusing to guess"
                             : "No supported Gen I regional layout passed checksum + list validation";
        return result;
    }
    const Layout& l = intl ? kInternational : kJapanese;
    const VersionEvidence evidence = versionEvidence(raw, l);
    if (!gameMatchesEvidence(game, evidence)) {
        result.error = SaveError::GameHintMismatch;
        result.detail = "Validated Gen I save conflicts with Red/Blue vs Yellow evidence";
        return result;
    }

    auto save = std::make_shared<ReadOnlySave>();
    save->sourceBytes_.assign(raw.begin(), raw.end());
    save->metadata_.sourceGame = game;
    save->metadata_.sourceGameId = sourceGameId(game);
    save->metadata_.region = l.region;
    save->metadata_.versionEvidence = evidence;
    save->metadata_.currentBox = raw[l.currentBoxIndex] & 0x7F;
    save->metadata_.boxesInitialized = (raw[l.currentBoxIndex] & 0x80) != 0;

    save->trainer_.name = decodeGen1String(raw.subspan(kMainStart, l.stringLength), l.region);
    save->trainer_.trainerId = readBE16(raw, l.tid);
    if (!decodeBCD3(raw, l.money, save->trainer_.money)) {
        result.error = SaveError::InvalidTrainerData;
        result.detail = "Generation I money field is not valid packed BCD";
        return result;
    }

    std::vector<std::optional<PokemonRecord>> partySlots;
    if (!parseList(raw, l.party, kPartyCapacity, kPartyBody, l.stringLength, l.region, true,
                   partySlots, result.detail)) {
        result.error = SaveError::InvalidParty;
        return result;
    }
    for (auto& slot : partySlots) {
        if (slot) save->party_.push_back(std::move(*slot));
    }

    if (save->metadata_.boxesInitialized && !checkBankChecksums(raw, l, result.detail)) {
        result.error = SaveError::ChecksumMismatch;
        return result;
    }

    save->boxes_.resize(l.boxCount);
    for (std::size_t box = 0; box < l.boxCount; ++box) {
        auto& out = save->boxes_[box].slots;
        out.assign(l.boxCapacity, std::nullopt);
        if (!save->metadata_.boxesInitialized && box != save->metadata_.currentBox)
            continue; // PKSM-Core: these banks may still be uninitialized garbage.

        const std::size_t boxOfs = box == save->metadata_.currentBox
            ? l.currentBox : storedBoxStart(l, box);
        std::vector<std::optional<PokemonRecord>> parsed;
        if (!parseList(raw, boxOfs, l.boxCapacity, kStoredBody, l.stringLength, l.region, false,
                       parsed, result.detail)) {
            result.error = SaveError::InvalidBox;
            return result;
        }
        out = std::move(parsed);
    }

    result.save = std::move(save);
    result.error = SaveError::None;
    return result;
}

} // namespace PokeVault::Integration::Gen1
