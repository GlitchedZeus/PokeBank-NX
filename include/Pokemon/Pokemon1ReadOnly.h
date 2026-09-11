#ifndef POKEMON_POKEMON1_READ_ONLY_H
#define POKEMON_POKEMON1_READ_ONLY_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Pokemon/Pokemon.h"

#include <array>
#include <cstdint>
#include <memory>

namespace Pokemon {

// UI presentation wrapper around an already-validated strict Gen I record. It deliberately does
// not reinterpret DVs/stat experience as modern IVs/EVs and all mutation entry points remain inert.
class Pokemon1ReadOnly final : public Pokemon {
public:
    // Internal presentation-only tag. This is NOT a stored Pokemon game-version byte; it exists so
    // the no-RTTI Switch build can route validated PK1 wrappers to the Generation I summary UI.
    static constexpr Enums::GameVersion kReadOnlyGameGroup = static_cast<Enums::GameVersion>(71);

    explicit Pokemon1ReadOnly(const PokeVault::Integration::Gen1::PokemonRecord& record);

    uint16_t speciesID() const noexcept override { return record_.species; }
    const char* species() const noexcept override;
    std::u16string nickname() const override;
    uint8_t formID() const noexcept override { return 0; }
    uint8_t form() const noexcept override { return 0; }
    uint16_t heldItem() const noexcept override { return 0; }
    uint32_t id32() const noexcept override { return record_.trainerId; }
    uint32_t exp() const noexcept override { return record_.experience; }
    uint16_t ability() const noexcept override { return 0; }
    uint8_t nature() const noexcept override { return 0; }
    uint8_t statNature() const noexcept override { return 0; }
    uint8_t level() const noexcept override { return record_.level; }
    uint8_t gender() const noexcept override { return 2; }
    const char* genderSymbol() const noexcept override { return ""; }
    uint32_t pid() const noexcept override { return 0; }
    uint32_t encryptionConstant() const noexcept override { return 0; }

    // Gen I has 4-bit DVs and 16-bit stat experience, not modern IV/EV fields. These base-class
    // compatibility getters stay neutral; the Gen I details view uses the explicit accessors below.
    uint8_t ivHP() const noexcept override { return 0; }
    uint8_t ivATK() const noexcept override { return 0; }
    uint8_t ivDEF() const noexcept override { return 0; }
    uint8_t ivSPE() const noexcept override { return 0; }
    uint8_t ivSPA() const noexcept override { return 0; }
    uint8_t ivSPD() const noexcept override { return 0; }
    void setIV(int, uint8_t) noexcept override {}
    uint8_t evHP() const noexcept override { return 0; }
    uint8_t evATK() const noexcept override { return 0; }
    uint8_t evDEF() const noexcept override { return 0; }
    uint8_t evSPE() const noexcept override { return 0; }
    uint8_t evSPA() const noexcept override { return 0; }
    uint8_t evSPD() const noexcept override { return 0; }
    void setEV(int, uint8_t) noexcept override {}

    uint16_t move(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? record_.moves[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePP(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? record_.pp[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePPUps(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? record_.ppUps[static_cast<size_t>(slot)] : 0;
    }

    Enums::GameVersion getGameGroup() const noexcept override { return kReadOnlyGameGroup; }
    uint16_t tid16() const noexcept override { return record_.trainerId; }
    std::u16string otName() const override;

    uint8_t baseHP() const noexcept override { return 0; }
    uint8_t baseATK() const noexcept override { return 0; }
    uint8_t baseDEF() const noexcept override { return 0; }
    uint8_t baseSPE() const noexcept override { return 0; }
    uint8_t baseSPA() const noexcept override { return 0; }
    uint8_t baseSPD() const noexcept override { return 0; }

    uint16_t statHPMax() const noexcept override { return record_.partyRecord ? record_.maxHP : 0; }
    uint16_t statATK() const noexcept override { return record_.partyRecord ? record_.attack : 0; }
    uint16_t statDEF() const noexcept override { return record_.partyRecord ? record_.defense : 0; }
    uint16_t statSPE() const noexcept override { return record_.partyRecord ? record_.speed : 0; }
    uint16_t statSPA() const noexcept override { return record_.partyRecord ? record_.special : 0; }
    uint16_t statSPD() const noexcept override { return record_.partyRecord ? record_.special : 0; }
    uint16_t statHPCurrent() const noexcept override { return record_.currentHP; }

    uint8_t friendship() const noexcept override { return 0; }
    bool isEgg() const noexcept override { return false; }
    bool isShiny(uint32_t, std::string) const noexcept override { return false; }
    bool isPokerusInfected() const noexcept override { return false; }
    bool isPokerusCured() const noexcept override { return false; }
    uint16_t checksum() const noexcept override { return 0; }
    uint16_t calculateChecksum() const noexcept override { return 0; }
    void refreshChecksum() noexcept override {}
    bool checksumValid() const noexcept override { return true; }
    void recalculateStats() noexcept override {}
    void regeneratePID(uint32_t) noexcept override {}
    void setShiny(bool, uint32_t) noexcept override {}

    std::unique_ptr<Pokemon> clone() const override {
        return std::make_unique<Pokemon1ReadOnly>(record_);
    }

    uint8_t dvHP() const noexcept { return record_.dvs[0]; }
    uint8_t dvATK() const noexcept { return record_.dvs[1]; }
    uint8_t dvDEF() const noexcept { return record_.dvs[2]; }
    uint8_t dvSPE() const noexcept { return record_.dvs[3]; }
    uint8_t dvSpecial() const noexcept { return record_.dvs[4]; }
    uint16_t statExpHP() const noexcept { return record_.statExperience[0]; }
    uint16_t statExpATK() const noexcept { return record_.statExperience[1]; }
    uint16_t statExpDEF() const noexcept { return record_.statExperience[2]; }
    uint16_t statExpSPE() const noexcept { return record_.statExperience[3]; }
    uint16_t statExpSpecial() const noexcept { return record_.statExperience[4]; }
    uint8_t statusByte() const noexcept { return record_.status; }
    bool isPartyRecord() const noexcept { return record_.partyRecord; }
    uint16_t gen1Special() const noexcept { return record_.special; }
    const PokeVault::Integration::Gen1::PokemonRecord& strictRecord() const noexcept { return record_; }

private:
    PokeVault::Integration::Gen1::PokemonRecord record_;
};

} // namespace Pokemon

#endif
