#include "Pokemon/Pokemon4ReadOnly.h"

#include "Encryption/Encryption4.h"
#include "Pokemon/PersonalInfo4DP.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Utils/Gen4TextCodec.h"

namespace Pokemon {

Pokemon4ReadOnly::Pokemon4ReadOnly(std::span<const std::byte> encrypted,
                                   Enums::GameVersion sourceGroup)
    : encrypted_(encrypted.begin(), encrypted.end()), sourceGroup_(sourceGroup) {
    decrypted_ = Encryption::decryptArray4(encrypted);
}

bool Pokemon4ReadOnly::sizeValid() const noexcept {
    return Encryption::validRecordSize4(encrypted_.size()) && decrypted_.size() == encrypted_.size();
}
bool Pokemon4ReadOnly::checksumValid() const noexcept {
    return sizeValid() && checksum() == calculatedChecksum();
}
bool Pokemon4ReadOnly::sanityValid() const noexcept { return sizeValid() && sanity() == 0; }
bool Pokemon4ReadOnly::valid() const noexcept { return sizeValid() && sanityValid() && checksumValid(); }
bool Pokemon4ReadOnly::isParty() const noexcept { return encrypted_.size() == Encryption::SIZE_PARTY4; }
bool Pokemon4ReadOnly::empty() const noexcept { return sizeValid() && species() == 0; }

uint8_t Pokemon4ReadOnly::byteAt(size_t offset) const noexcept {
    return offset < decrypted_.size() ? static_cast<uint8_t>(decrypted_[offset]) : 0;
}
uint16_t Pokemon4ReadOnly::u16At(size_t offset) const noexcept {
    if (offset + 1 >= decrypted_.size()) return 0;
    return static_cast<uint16_t>(byteAt(offset) | (static_cast<uint16_t>(byteAt(offset + 1)) << 8));
}
uint32_t Pokemon4ReadOnly::u32At(size_t offset) const noexcept {
    if (offset + 3 >= decrypted_.size()) return 0;
    return static_cast<uint32_t>(byteAt(offset)) |
           (static_cast<uint32_t>(byteAt(offset + 1)) << 8) |
           (static_cast<uint32_t>(byteAt(offset + 2)) << 16) |
           (static_cast<uint32_t>(byteAt(offset + 3)) << 24);
}

uint32_t Pokemon4ReadOnly::pid() const noexcept { return u32At(0x00); }
uint16_t Pokemon4ReadOnly::sanity() const noexcept { return u16At(0x04); }
uint16_t Pokemon4ReadOnly::checksum() const noexcept { return u16At(0x06); }
uint16_t Pokemon4ReadOnly::calculatedChecksum() const noexcept {
    return Encryption::checksum4(decrypted_);
}
uint16_t Pokemon4ReadOnly::species() const noexcept { return u16At(0x08); }
uint16_t Pokemon4ReadOnly::heldItem() const noexcept { return u16At(0x0A); }
uint16_t Pokemon4ReadOnly::tid() const noexcept { return u16At(0x0C); }
uint16_t Pokemon4ReadOnly::sid() const noexcept { return u16At(0x0E); }
uint32_t Pokemon4ReadOnly::id32() const noexcept { return u32At(0x0C); }
uint32_t Pokemon4ReadOnly::experience() const noexcept { return u32At(0x10); }
uint8_t Pokemon4ReadOnly::friendship() const noexcept { return byteAt(0x14); }
uint8_t Pokemon4ReadOnly::ability() const noexcept { return byteAt(0x15); }
uint8_t Pokemon4ReadOnly::markings() const noexcept { return byteAt(0x16); }
uint8_t Pokemon4ReadOnly::language() const noexcept { return byteAt(0x17); }

std::array<uint8_t, 6> Pokemon4ReadOnly::evs() const noexcept {
    return {byteAt(0x18), byteAt(0x19), byteAt(0x1A), byteAt(0x1B), byteAt(0x1C), byteAt(0x1D)};
}
std::array<uint16_t, 4> Pokemon4ReadOnly::moves() const noexcept {
    return {u16At(0x28), u16At(0x2A), u16At(0x2C), u16At(0x2E)};
}
std::array<uint8_t, 4> Pokemon4ReadOnly::pp() const noexcept {
    return {byteAt(0x30), byteAt(0x31), byteAt(0x32), byteAt(0x33)};
}
std::array<uint8_t, 4> Pokemon4ReadOnly::ppUps() const noexcept {
    return {byteAt(0x34), byteAt(0x35), byteAt(0x36), byteAt(0x37)};
}
uint32_t Pokemon4ReadOnly::iv32() const noexcept { return u32At(0x38); }
std::array<uint8_t, 6> Pokemon4ReadOnly::ivs() const noexcept {
    const uint32_t v = iv32();
    return {static_cast<uint8_t>((v >> 0) & 31u), static_cast<uint8_t>((v >> 5) & 31u),
            static_cast<uint8_t>((v >> 10) & 31u), static_cast<uint8_t>((v >> 15) & 31u),
            static_cast<uint8_t>((v >> 20) & 31u), static_cast<uint8_t>((v >> 25) & 31u)};
}
bool Pokemon4ReadOnly::isEgg() const noexcept { return (iv32() & 0x40000000u) != 0; }
bool Pokemon4ReadOnly::isNicknamed() const noexcept { return (iv32() & 0x80000000u) != 0; }
bool Pokemon4ReadOnly::fatefulEncounter() const noexcept { return (byteAt(0x40) & 1u) != 0; }
uint8_t Pokemon4ReadOnly::gender() const noexcept { return static_cast<uint8_t>((byteAt(0x40) >> 1) & 3u); }
uint8_t Pokemon4ReadOnly::form() const noexcept { return static_cast<uint8_t>(byteAt(0x40) >> 3); }

std::u16string Pokemon4ReadOnly::nickname() const {
    if (decrypted_.size() < 0x5E) return {};
    return Utils::decodeGen4Field(std::span<const std::byte>(decrypted_.data() + 0x48, 22));
}
uint8_t Pokemon4ReadOnly::originVersion() const noexcept { return byteAt(0x5F); }
std::u16string Pokemon4ReadOnly::originalTrainerName() const {
    if (decrypted_.size() < 0x78) return {};
    return Utils::decodeGen4Field(std::span<const std::byte>(decrypted_.data() + 0x68, 16));
}
uint16_t Pokemon4ReadOnly::eggLocationExtended() const noexcept { return u16At(0x44); }
uint16_t Pokemon4ReadOnly::metLocationExtended() const noexcept { return u16At(0x46); }
uint16_t Pokemon4ReadOnly::eggLocationDP() const noexcept { return u16At(0x7E); }
uint16_t Pokemon4ReadOnly::metLocationDP() const noexcept { return u16At(0x80); }
uint8_t Pokemon4ReadOnly::pokerusState() const noexcept { return byteAt(0x82); }
uint8_t Pokemon4ReadOnly::ballDPPt() const noexcept { return byteAt(0x83); }
uint8_t Pokemon4ReadOnly::ballHGSS() const noexcept { return byteAt(0x86); }
uint8_t Pokemon4ReadOnly::metLevel() const noexcept { return static_cast<uint8_t>(byteAt(0x84) & 0x7Fu); }
uint8_t Pokemon4ReadOnly::originalTrainerGender() const noexcept { return static_cast<uint8_t>(byteAt(0x84) >> 7); }

uint32_t Pokemon4ReadOnly::partyStatus() const noexcept { return isParty() ? u32At(0x88) : 0; }
uint8_t Pokemon4ReadOnly::partyLevel() const noexcept { return isParty() ? byteAt(0x8C) : 0; }
uint8_t Pokemon4ReadOnly::ballCapsuleIndex() const noexcept { return isParty() ? byteAt(0x8D) : 0; }
uint16_t Pokemon4ReadOnly::currentHP() const noexcept { return isParty() ? u16At(0x8E) : 0; }
uint16_t Pokemon4ReadOnly::maxHP() const noexcept { return isParty() ? u16At(0x90) : 0; }
std::array<uint16_t, 5> Pokemon4ReadOnly::battleStats() const noexcept {
    if (!isParty()) return {};
    return {u16At(0x92), u16At(0x94), u16At(0x96), u16At(0x98), u16At(0x9A)};
}

const PersonalRecord& Pokemon4ReadOnly::personal() const noexcept {
    switch (sourceGroup_) {
        case Enums::GameVersion::DP: return getPersonalInfo4DP(species(), form());
        case Enums::GameVersion::PT: return getPersonalInfo4PT(species(), form());
        case Enums::GameVersion::HGSS: return getPersonalInfo4HGSS(species(), form());
        default: return PERSONAL_RECORD_EMPTY;
    }
}

}
