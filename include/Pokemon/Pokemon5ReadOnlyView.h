#ifndef POKEBANK_POKEMON5_READ_ONLY_VIEW_H
#define POKEBANK_POKEMON5_READ_ONLY_VIEW_H

#include "Pokemon/Pokemon.h"
#include "Pokemon/Pokemon5ReadOnly.h"
#include "Names/SpeciesNames.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

namespace Pokemon {

// Existing shared Trainer/Party/Boxes UI consumes the same Pokemon interface
// for every generation. This Gen V bridge is PRESENTATION ONLY: no PK5 payload
// is exposed through mutable getData(), and every inherited mutator is inert.
class Pokemon5ReadOnlyView final : public Pokemon {
public:
    explicit Pokemon5ReadOnlyView(PokeVault::Integration::Gen5::Pokemon5ReadOnly source,
                                    Enums::GameVersion sourceGroup)
        : source_(std::move(source)), sourceGroup_(sourceGroup) {}

    [[nodiscard]] const auto& source() const noexcept { return source_; }

    uint16_t speciesID() const noexcept override { return source_.species(); }
    const char* species() const noexcept override { return Names::getSpeciesName(speciesID()); }
    std::u16string nickname() const override { return nativeString(0x48,11); }
    uint8_t formID() const noexcept override { return source_.form(); }
    uint8_t form() const noexcept override { return source_.form(); }
    uint16_t heldItem() const noexcept override { return source_.heldItem(); }
    uint32_t id32() const noexcept override {
        return uint32_t(source_.tid()) | (uint32_t(source_.sid())<<16);
    }
    uint32_t exp() const noexcept override { return source_.experience(); }
    uint16_t ability() const noexcept override { return source_.ability(); }
    uint8_t nature() const noexcept override { return source_.nature(); }
    uint8_t statNature() const noexcept override { return nature(); }
    uint8_t level() const noexcept override {
        // Native boxed PK5 has no current level. Do not invent a growth
        // curve or silently display level 1 for a boxed Pokemon.
        const uint8_t value=source_.partyLevel();
        return value>=1 && value<=100 ? value : 0;
    }
    uint8_t gender() const noexcept override { return source_.gender(); }
    const char* genderSymbol() const noexcept override {
        return gender()==0 ? "♂" : gender()==1 ? "♀" : "";
    }
    uint32_t pid() const noexcept override { return source_.pid(); }
    uint32_t encryptionConstant() const noexcept override { return source_.pid(); }

    uint8_t ivHP() const noexcept override { return source_.ivs()[0]; }
    uint8_t ivATK() const noexcept override { return source_.ivs()[1]; }
    uint8_t ivDEF() const noexcept override { return source_.ivs()[2]; }
    uint8_t ivSPE() const noexcept override { return source_.ivs()[3]; }
    uint8_t ivSPA() const noexcept override { return source_.ivs()[4]; }
    uint8_t ivSPD() const noexcept override { return source_.ivs()[5]; }
    void setIV(int,uint8_t) noexcept override {}

    uint8_t evHP() const noexcept override { return source_.evs()[0]; }
    uint8_t evATK() const noexcept override { return source_.evs()[1]; }
    uint8_t evDEF() const noexcept override { return source_.evs()[2]; }
    uint8_t evSPE() const noexcept override { return source_.evs()[3]; }
    uint8_t evSPA() const noexcept override { return source_.evs()[4]; }
    uint8_t evSPD() const noexcept override { return source_.evs()[5]; }
    void setEV(int,uint8_t) noexcept override {}

    uint16_t move(int slot) const noexcept override {
        return slot>=0 && slot<4 ? source_.moves()[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePP(int slot) const noexcept override {
        return slot>=0 && slot<4 ? source_.pp()[static_cast<size_t>(slot)] : 0;
    }
    uint8_t movePPUps(int slot) const noexcept override {
        return slot>=0 && slot<4 ? source_.ppUps()[static_cast<size_t>(slot)] : 0;
    }
    uint8_t originGame() const noexcept override { return source_.originVersion(); }
    Enums::GameVersion getGameGroup() const noexcept override { return sourceGroup_; }
    uint16_t tid16() const noexcept override { return source_.tid(); }
    uint16_t sid16() const noexcept override { return source_.sid(); }
    uint8_t otFriendship() const noexcept override { return source_.friendship(); }
    uint8_t language() const noexcept override { return source_.language(); }
    uint8_t ball() const noexcept override { return at(0x83); }
    uint16_t metLocation() const noexcept override { return u16(0x80); }
    uint8_t metLevel() const noexcept override { return at(0x84)&0x7F; }
    uint16_t eggLocation() const noexcept override { return u16(0x7E); }
    uint8_t metYear() const noexcept override { return at(0x7B); }
    uint8_t metMonth() const noexcept override { return at(0x7C); }
    uint8_t metDay() const noexcept override { return at(0x7D); }
    uint8_t eggYear() const noexcept override { return at(0x78); }
    uint8_t eggMonth() const noexcept override { return at(0x79); }
    uint8_t eggDay() const noexcept override { return at(0x7A); }
    int getMaxNicknameLength() const noexcept override { return 10; }
    bool isNicknamed() const noexcept override { return (u32(0x38)&0x80000000u)!=0; }
    bool isFatefulEncounter() const noexcept override { return (at(0x40)&1)!=0; }
    std::u16string otName() const override { return nativeString(0x68,8); }

    // A boxed Gen V record does not store current battle stats. These remain
    // unavailable until a format-correct Gen V personal-data/stat model ships.
    uint8_t baseHP() const noexcept override { return 0; }
    uint8_t baseATK() const noexcept override { return 0; }
    uint8_t baseDEF() const noexcept override { return 0; }
    uint8_t baseSPE() const noexcept override { return 0; }
    uint8_t baseSPA() const noexcept override { return 0; }
    uint8_t baseSPD() const noexcept override { return 0; }

    uint16_t statHPCurrent() const noexcept override { return partyStat(0x8E); }
    uint16_t statHPMax() const noexcept override { return partyStat(0x90); }
    uint16_t statATK() const noexcept override { return partyStat(0x92); }
    uint16_t statDEF() const noexcept override { return partyStat(0x94); }
    uint16_t statSPE() const noexcept override { return partyStat(0x96); }
    uint16_t statSPA() const noexcept override { return partyStat(0x98); }
    uint16_t statSPD() const noexcept override { return partyStat(0x9A); }

    uint8_t friendship() const noexcept override { return source_.friendship(); }
    bool isEgg() const noexcept override { return source_.egg(); }
    bool isShiny(uint32_t,std::string) const noexcept override { return source_.shiny(); }
    bool isPokerusInfected() const noexcept override { return (at(0x82)&0x0F)!=0; }
    bool isPokerusCured() const noexcept override {
        return (at(0x82)&0xF0)!=0 && (at(0x82)&0x0F)==0;
    }
    bool hasPokerus() const noexcept override { return true; }

    uint16_t checksum() const noexcept override { return u16(6); }
    uint16_t calculateChecksum() const noexcept override {
        return source_.valid() ? PokeVault::Integration::Gen5::Crypto::checksum(source_.decodedBytes()) : 0;
    }
    bool checksumValid() const noexcept override { return source_.checksumValid(); }
    void refreshChecksum() noexcept override {}
    void recalculateStats() noexcept override {}
    void regeneratePID(uint32_t) noexcept override {}
    void setShiny(bool,uint32_t) noexcept override {}
    std::unique_ptr<Pokemon> clone() const override { return nullptr; }

private:
    [[nodiscard]] uint8_t at(size_t n) const noexcept {
        const auto b=source_.decodedBytes();
        return source_.valid() && n<b.size() ? b[n] : 0;
    }
    [[nodiscard]] uint16_t u16(size_t n) const noexcept {
        return uint16_t(at(n)) | (uint16_t(at(n+1))<<8);
    }
    [[nodiscard]] uint32_t u32(size_t n) const noexcept {
        return uint32_t(u16(n)) | (uint32_t(u16(n+2))<<16);
    }
    [[nodiscard]] uint16_t partyStat(size_t n) const noexcept {
        return source_.partyRecord() ? u16(n) : 0;
    }
    [[nodiscard]] std::u16string nativeString(size_t offset,size_t count) const {
        std::u16string text;
        if(!source_.valid())return text;
        for(size_t i=0;i<count;++i) {
            const uint16_t c=u16(offset+i*2);
            if(c==0 || c==0xFFFF)break;
            // Lossless UTF-16 units are retained; native Gen V glyph
            // localization belongs to the separate string converter.
            text.push_back(static_cast<char16_t>(c));
        }
        return text;
    }

    PokeVault::Integration::Gen5::Pokemon5ReadOnly source_;
    Enums::GameVersion sourceGroup_;
};
} // namespace Pokemon
#endif
