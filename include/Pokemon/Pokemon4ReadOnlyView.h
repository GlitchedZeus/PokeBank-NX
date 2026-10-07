#ifndef POKEBANK_POKEMON4_READ_ONLY_VIEW_H
#define POKEBANK_POKEMON4_READ_ONLY_VIEW_H

#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon4ReadOnly.h"

namespace Pokemon {

// Presentation-only bridge from the strict immutable PK4 model into the existing shared
// Trainer/Boxes UI. Every mutator intentionally does nothing; source mutation remains impossible
// even if an inherited UI path were accidentally reached.
class Pokemon4ReadOnlyView final : public Pokemon {
public:
    explicit Pokemon4ReadOnlyView(const Pokemon4ReadOnly& source);

    [[nodiscard]] const Pokemon4ReadOnly& source() const noexcept { return source_; }

    uint16_t speciesID() const noexcept override;
    const char* species() const noexcept override;
    std::u16string nickname() const override;
    uint8_t formID() const noexcept override;
    uint8_t form() const noexcept override;
    uint16_t heldItem() const noexcept override;
    uint32_t id32() const noexcept override;
    uint32_t exp() const noexcept override;
    uint16_t ability() const noexcept override;
    uint8_t nature() const noexcept override;
    uint8_t statNature() const noexcept override;
    uint8_t level() const noexcept override;
    uint8_t gender() const noexcept override;
    const char* genderSymbol() const noexcept override;
    uint32_t pid() const noexcept override;
    uint32_t encryptionConstant() const noexcept override;

    uint8_t ivHP() const noexcept override;
    uint8_t ivATK() const noexcept override;
    uint8_t ivDEF() const noexcept override;
    uint8_t ivSPE() const noexcept override;
    uint8_t ivSPA() const noexcept override;
    uint8_t ivSPD() const noexcept override;
    void setIV(int, uint8_t) noexcept override {}

    uint8_t evHP() const noexcept override;
    uint8_t evATK() const noexcept override;
    uint8_t evDEF() const noexcept override;
    uint8_t evSPE() const noexcept override;
    uint8_t evSPA() const noexcept override;
    uint8_t evSPD() const noexcept override;
    void setEV(int, uint8_t) noexcept override {}

    uint16_t move(int slot) const noexcept override;
    uint8_t movePP(int slot) const noexcept override;
    uint8_t movePPUps(int slot) const noexcept override;
    uint8_t originGame() const noexcept override;
    Enums::GameVersion getGameGroup() const noexcept override;
    uint16_t tid16() const noexcept override;
    uint16_t sid16() const noexcept override;
    uint8_t otGender() const noexcept override;
    uint8_t otFriendship() const noexcept override;
    uint8_t language() const noexcept override;
    uint8_t ball() const noexcept override;
    uint16_t metLocation() const noexcept override;
    uint8_t metLevel() const noexcept override;
    uint16_t eggLocation() const noexcept override;
    int getMaxNicknameLength() const noexcept override { return 10; }
    bool isNicknamed() const noexcept override;
    bool isFatefulEncounter() const noexcept override;
    std::u16string otName() const override;

    uint8_t baseHP() const noexcept override;
    uint8_t baseATK() const noexcept override;
    uint8_t baseDEF() const noexcept override;
    uint8_t baseSPE() const noexcept override;
    uint8_t baseSPA() const noexcept override;
    uint8_t baseSPD() const noexcept override;

    uint16_t statHPMax() const noexcept override;
    uint16_t statATK() const noexcept override;
    uint16_t statDEF() const noexcept override;
    uint16_t statSPE() const noexcept override;
    uint16_t statSPA() const noexcept override;
    uint16_t statSPD() const noexcept override;
    uint16_t statHPCurrent() const noexcept override;

    uint8_t friendship() const noexcept override;
    bool isEgg() const noexcept override;
    bool isShiny(uint32_t, std::string) const noexcept override;
    bool isPokerusInfected() const noexcept override;
    bool isPokerusCured() const noexcept override;
    bool hasPokerus() const noexcept override { return true; }

    uint16_t checksum() const noexcept override;
    uint16_t calculateChecksum() const noexcept override;
    void refreshChecksum() noexcept override {}
    bool checksumValid() const noexcept override;
    void recalculateStats() noexcept override {}
    void regeneratePID(uint32_t) noexcept override {}
    void setShiny(bool, uint32_t) noexcept override {}

    // Defense in depth: no source-side clone/edit object is ever handed to generic mutators.
    std::unique_ptr<Pokemon> clone() const override { return nullptr; }

private:
    uint16_t calculatedStat(int index) const noexcept;
    Pokemon4ReadOnly source_;
};

}
#endif
