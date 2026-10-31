#ifndef POKEBANK_POKEMON_MODERN_RECORD_VIEW_H
#define POKEBANK_POKEMON_MODERN_RECORD_VIEW_H

// Read-only presentation adapter from a generation-neutral Modern::RecordView to the Pokemon
// interface the shared Party/Box panels and sprite layer consume. One class serves every Gen VI-IX
// adapter: no per-generation presentation Pokemon is needed. Every mutator is a no-op; staging
// happens through Modern::NativeSession only.

#include "Integration/Modern/NativeSession.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon.h"
#include "Utils/StringHelpers.h"

#include <memory>
#include <string>
#include <utility>

namespace Pokemon {

class ModernRecordView final : public Pokemon {
public:
    ModernRecordView(PokeVault::Integration::Modern::RecordView view, Enums::GameVersion group)
        : view_(std::move(view)), group_(group) {}

    [[nodiscard]] const PokeVault::Integration::Modern::RecordView& record() const noexcept { return view_; }

    uint16_t speciesID() const noexcept override { return view_.species; }
    const char* species() const noexcept override { return Names::getSpeciesName(view_.species); }
    std::u16string nickname() const override { return Utils::utf8ToUtf16(view_.nickname); }
    uint8_t formID() const noexcept override { return view_.form; }
    uint8_t form() const noexcept override { return view_.form; }
    uint16_t heldItem() const noexcept override { return view_.heldItem; }
    uint32_t id32() const noexcept override { return uint32_t(view_.tid) | (uint32_t(view_.sid) << 16); }
    uint32_t exp() const noexcept override { return view_.experience; }
    uint16_t ability() const noexcept override { return view_.ability; }
    uint8_t abilityNumber() const noexcept override { return view_.abilityNumber; }
    uint8_t nature() const noexcept override { return view_.nature; }
    uint8_t statNature() const noexcept override { return view_.statNature; }
    uint8_t level() const noexcept override { return view_.level; }
    uint8_t gender() const noexcept override { return view_.gender; }
    const char* genderSymbol() const noexcept override {
        return view_.gender == 0 ? "♂" : view_.gender == 1 ? "♀" : "";
    }
    uint32_t pid() const noexcept override { return view_.pid; }
    uint32_t encryptionConstant() const noexcept override { return view_.encryptionConstant; }

    uint8_t ivHP() const noexcept override { return view_.ivs[0]; }
    uint8_t ivATK() const noexcept override { return view_.ivs[1]; }
    uint8_t ivDEF() const noexcept override { return view_.ivs[2]; }
    uint8_t ivSPE() const noexcept override { return view_.ivs[3]; }
    uint8_t ivSPA() const noexcept override { return view_.ivs[4]; }
    uint8_t ivSPD() const noexcept override { return view_.ivs[5]; }
    void setIV(int, uint8_t) noexcept override {}
    uint8_t evHP() const noexcept override { return view_.evs[0]; }
    uint8_t evATK() const noexcept override { return view_.evs[1]; }
    uint8_t evDEF() const noexcept override { return view_.evs[2]; }
    uint8_t evSPE() const noexcept override { return view_.evs[3]; }
    uint8_t evSPA() const noexcept override { return view_.evs[4]; }
    uint8_t evSPD() const noexcept override { return view_.evs[5]; }
    void setEV(int, uint8_t) noexcept override {}

    uint16_t move(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? view_.moves[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePP(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? view_.pp[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePPUps(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? view_.ppUps[static_cast<size_t>(slot)] : 0;
    }
    uint16_t relearnMove(int slot) const noexcept override {
        return slot >= 0 && slot < 4 ? view_.relearnMoves[static_cast<size_t>(slot)] : 0;
    }
    uint8_t originGame() const noexcept override { return view_.originGame; }
    Enums::GameVersion getGameGroup() const noexcept override { return group_; }
    uint16_t tid16() const noexcept override { return view_.tid; }
    uint16_t sid16() const noexcept override { return view_.sid; }
    uint8_t otGender() const noexcept override { return view_.otGender; }
    uint8_t otFriendship() const noexcept override { return view_.friendship; }
    uint8_t language() const noexcept override { return view_.language; }
    uint8_t ball() const noexcept override { return view_.ball; }
    uint16_t metLocation() const noexcept override { return view_.metLocation; }
    uint8_t metLevel() const noexcept override { return view_.metLevel; }
    uint16_t eggLocation() const noexcept override { return view_.eggLocation; }
    uint8_t metYear() const noexcept override { return static_cast<uint8_t>(view_.metDate.year % 100); }
    uint8_t metMonth() const noexcept override { return view_.metDate.month; }
    uint8_t metDay() const noexcept override { return view_.metDate.day; }
    uint8_t eggYear() const noexcept override { return static_cast<uint8_t>(view_.eggDate.year % 100); }
    uint8_t eggMonth() const noexcept override { return view_.eggDate.month; }
    uint8_t eggDay() const noexcept override { return view_.eggDate.day; }
    int getMaxNicknameLength() const noexcept override { return 12; }
    bool isNicknamed() const noexcept override { return view_.nicknamed; }
    bool isFatefulEncounter() const noexcept override { return view_.fatefulEncounter; }
    std::u16string otName() const override { return Utils::utf8ToUtf16(view_.otName); }

    uint8_t baseHP() const noexcept override { return view_.baseStats[0]; }
    uint8_t baseATK() const noexcept override { return view_.baseStats[1]; }
    uint8_t baseDEF() const noexcept override { return view_.baseStats[2]; }
    uint8_t baseSPE() const noexcept override { return view_.baseStats[3]; }
    uint8_t baseSPA() const noexcept override { return view_.baseStats[4]; }
    uint8_t baseSPD() const noexcept override { return view_.baseStats[5]; }

    uint16_t statHPCurrent() const noexcept override { return view_.partyRecord ? view_.currentHp : view_.stats[0]; }
    uint16_t statHPMax() const noexcept override { return view_.stats[0]; }
    uint16_t statATK() const noexcept override { return view_.stats[1]; }
    uint16_t statDEF() const noexcept override { return view_.stats[2]; }
    uint16_t statSPE() const noexcept override { return view_.stats[3]; }
    uint16_t statSPA() const noexcept override { return view_.stats[4]; }
    uint16_t statSPD() const noexcept override { return view_.stats[5]; }
    uint8_t friendship() const noexcept override { return view_.friendship; }
    bool isEgg() const noexcept override { return view_.egg; }
    bool isShiny(uint32_t, std::string) const noexcept override { return view_.shiny; }
    bool isPokerusInfected() const noexcept override { return (view_.pokerus & 0x0F) != 0; }
    bool isPokerusCured() const noexcept override { return (view_.pokerus & 0x0F) == 0 && (view_.pokerus >> 4) != 0; }
    bool hasPokerus() const noexcept override { return true; }
    uint16_t checksum() const noexcept override { return 0; }
    uint16_t calculateChecksum() const noexcept override { return 0; }
    bool checksumValid() const noexcept override { return view_.valid; }
    void refreshChecksum() noexcept override {}
    void recalculateStats() noexcept override {}
    void regeneratePID(uint32_t) noexcept override {}
    void setShiny(bool, uint32_t) noexcept override {}
    std::unique_ptr<Pokemon> clone() const override { return nullptr; }

private:
    PokeVault::Integration::Modern::RecordView view_;
    Enums::GameVersion group_;
};

} // namespace Pokemon

#endif
