#ifndef POKEMON_POKEMON4_MUTABLE_H
#define POKEMON_POKEMON4_MUTABLE_H

#include "Enums/GameVersion.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Pokemon {

class Pokemon4Mutable {
public:
    static std::optional<Pokemon4Mutable> fromEncrypted(
        std::span<const std::byte> encrypted,
        Enums::GameVersion sourceGroup,
        std::string* error = nullptr);

    [[nodiscard]] bool valid() const noexcept { return valid_; }
    [[nodiscard]] bool isParty() const noexcept;
    [[nodiscard]] std::span<const std::byte> decryptedBytes() const noexcept { return decrypted_; }
    [[nodiscard]] std::vector<std::byte> encryptedBytes() const;

    [[nodiscard]] uint32_t pid() const noexcept;
    [[nodiscard]] uint16_t species() const noexcept;
    [[nodiscard]] uint16_t heldItem() const noexcept;
    [[nodiscard]] uint16_t tid() const noexcept;
    [[nodiscard]] uint16_t sid() const noexcept;
    [[nodiscard]] uint32_t experience() const noexcept;
    [[nodiscard]] uint8_t friendship() const noexcept;
    [[nodiscard]] uint8_t language() const noexcept;
    [[nodiscard]] uint8_t level() const noexcept;
    [[nodiscard]] std::u16string nickname() const;
    [[nodiscard]] std::array<uint8_t, 6> ivs() const noexcept;
    [[nodiscard]] std::array<uint8_t, 6> evs() const noexcept;
    [[nodiscard]] std::array<uint16_t, 4> moves() const noexcept;
    [[nodiscard]] std::array<uint8_t, 4> pp() const noexcept;
    [[nodiscard]] std::array<uint8_t, 4> ppUps() const noexcept;
    [[nodiscard]] uint8_t pokerus() const noexcept;
    [[nodiscard]] uint8_t ball() const noexcept;
    [[nodiscard]] uint8_t metLevel() const noexcept;
    [[nodiscard]] uint8_t form() const noexcept;
    [[nodiscard]] uint8_t gender() const noexcept;
    [[nodiscard]] uint8_t nature() const noexcept;
    [[nodiscard]] bool shiny() const noexcept;
    [[nodiscard]] uint8_t abilitySlot() const noexcept;
    [[nodiscard]] uint16_t ability() const noexcept;
    [[nodiscard]] uint16_t abilityForSlot(uint8_t slot) const noexcept;

    bool setNickname(const std::u16string& value) noexcept;
    bool setLevel(uint8_t level) noexcept;
    bool setExperience(uint32_t value) noexcept;
    bool setFriendship(uint8_t value) noexcept;
    bool setHeldItem(uint16_t value) noexcept;
    bool setLanguage(uint8_t value) noexcept;
    bool setIV(size_t stat, uint8_t value) noexcept;
    bool setEV(size_t stat, uint8_t value) noexcept;
    bool setMove(size_t slot, uint16_t move) noexcept;
    bool setPP(size_t slot, uint8_t pp) noexcept;
    bool setPPUps(size_t slot, uint8_t ppUps) noexcept;
    bool setPokerus(uint8_t value) noexcept;
    bool setBall(uint8_t value) noexcept;
    bool setMetLevel(uint8_t value) noexcept;

    // PID-linked edits are transactional. If no candidate satisfies every pinned
    // trait in the bounded search, the PK4 is left byte-identical and false is returned.
    bool setNature(uint8_t value) noexcept;
    bool setGender(uint8_t value) noexcept;
    bool setShiny(bool value) noexcept;
    bool setAbilitySlot(uint8_t slot) noexcept;
    bool setAbility(uint16_t abilityId) noexcept;

private:
    Pokemon4Mutable(std::vector<std::byte> decrypted,
                    Enums::GameVersion sourceGroup) noexcept;

    [[nodiscard]] uint8_t byteAt(size_t offset) const noexcept;
    [[nodiscard]] uint16_t u16At(size_t offset) const noexcept;
    [[nodiscard]] uint32_t u32At(size_t offset) const noexcept;
    void write8(size_t offset, uint8_t value) noexcept;
    void write16(size_t offset, uint16_t value) noexcept;
    void write32(size_t offset, uint32_t value) noexcept;

    [[nodiscard]] uint8_t genderForPid(uint32_t value) const noexcept;
    [[nodiscard]] int constrainedAbilityBit() const noexcept;
    [[nodiscard]] uint16_t calculatedStat(size_t stat) const noexcept;
    void refreshPartyDerivedData() noexcept;
    bool rerollPid(int wantShiny, int wantGender, int wantNature,
                   int wantAbilityBit) noexcept;
    bool writeTextPreservingTrash(size_t offset, size_t slotCount,
                                  size_t maxCharacters,
                                  const std::u16string& value) noexcept;

    std::vector<std::byte> decrypted_;
    Enums::GameVersion sourceGroup_ = Enums::GameVersion::Invalid;
    bool valid_ = false;
};

} // namespace Pokemon

#endif
