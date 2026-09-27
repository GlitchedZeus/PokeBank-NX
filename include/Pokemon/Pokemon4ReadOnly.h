#ifndef POKEMON_POKEMON4_READ_ONLY_H
#define POKEMON_POKEMON4_READ_ONLY_H

#include "Enums/GameVersion.h"
#include "Pokemon/PersonalRecord.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Pokemon {
class Pokemon4ReadOnly {
public:
    Pokemon4ReadOnly() = default;
    explicit Pokemon4ReadOnly(std::span<const std::byte> encrypted,
                              Enums::GameVersion sourceGroup = Enums::GameVersion::Invalid);

    [[nodiscard]] bool sizeValid() const noexcept;
    [[nodiscard]] bool checksumValid() const noexcept;
    [[nodiscard]] bool sanityValid() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool isParty() const noexcept;
    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] std::span<const std::byte> originalEncryptedBytes() const noexcept { return encrypted_; }
    [[nodiscard]] std::span<const std::byte> decryptedBytes() const noexcept { return decrypted_; }

    [[nodiscard]] uint32_t pid() const noexcept;
    [[nodiscard]] uint16_t sanity() const noexcept;
    [[nodiscard]] uint16_t checksum() const noexcept;
    [[nodiscard]] uint16_t calculatedChecksum() const noexcept;
    [[nodiscard]] uint16_t species() const noexcept;
    [[nodiscard]] uint16_t heldItem() const noexcept;
    [[nodiscard]] uint16_t tid() const noexcept;
    [[nodiscard]] uint16_t sid() const noexcept;
    [[nodiscard]] uint32_t id32() const noexcept;
    [[nodiscard]] uint32_t experience() const noexcept;
    [[nodiscard]] uint8_t friendship() const noexcept;
    [[nodiscard]] uint8_t ability() const noexcept;
    [[nodiscard]] uint8_t markings() const noexcept;
    [[nodiscard]] uint8_t language() const noexcept;
    [[nodiscard]] std::array<uint8_t, 6> evs() const noexcept;
    [[nodiscard]] std::array<uint16_t, 4> moves() const noexcept;
    [[nodiscard]] std::array<uint8_t, 4> pp() const noexcept;
    [[nodiscard]] std::array<uint8_t, 4> ppUps() const noexcept;
    [[nodiscard]] uint32_t iv32() const noexcept;
    [[nodiscard]] std::array<uint8_t, 6> ivs() const noexcept;
    [[nodiscard]] bool isEgg() const noexcept;
    [[nodiscard]] bool isNicknamed() const noexcept;
    [[nodiscard]] bool fatefulEncounter() const noexcept;
    [[nodiscard]] uint8_t gender() const noexcept;
    [[nodiscard]] uint8_t form() const noexcept;
    [[nodiscard]] std::u16string nickname() const;
    [[nodiscard]] uint8_t originVersion() const noexcept;
    [[nodiscard]] std::u16string originalTrainerName() const;
    [[nodiscard]] uint16_t eggLocationExtended() const noexcept;
    [[nodiscard]] uint16_t metLocationExtended() const noexcept;
    [[nodiscard]] uint16_t eggLocationDP() const noexcept;
    [[nodiscard]] uint16_t metLocationDP() const noexcept;
    [[nodiscard]] uint8_t pokerusState() const noexcept;
    [[nodiscard]] uint8_t ballDPPt() const noexcept;
    [[nodiscard]] uint8_t ballHGSS() const noexcept;
    [[nodiscard]] uint8_t metLevel() const noexcept;
    [[nodiscard]] uint8_t originalTrainerGender() const noexcept;

    [[nodiscard]] uint32_t partyStatus() const noexcept;
    [[nodiscard]] uint8_t partyLevel() const noexcept;
    [[nodiscard]] uint8_t ballCapsuleIndex() const noexcept;
    [[nodiscard]] uint16_t currentHP() const noexcept;
    [[nodiscard]] uint16_t maxHP() const noexcept;
    [[nodiscard]] std::array<uint16_t, 5> battleStats() const noexcept;

    [[nodiscard]] const PersonalRecord& personal() const noexcept;
    [[nodiscard]] Enums::GameVersion sourceGroup() const noexcept { return sourceGroup_; }

private:
    uint8_t byteAt(size_t offset) const noexcept;
    uint16_t u16At(size_t offset) const noexcept;
    uint32_t u32At(size_t offset) const noexcept;

    std::vector<std::byte> encrypted_;
    std::vector<std::byte> decrypted_;
    Enums::GameVersion sourceGroup_ = Enums::GameVersion::Invalid;
};
}

#endif
