#include "Pokemon/Pokemon4ReadOnlyView.h"

#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"

#include <algorithm>

namespace Pokemon {

Pokemon4ReadOnlyView::Pokemon4ReadOnlyView(const Pokemon4ReadOnly& source) : source_(source) {
    buffer = nullptr;
    data = {};
    dataSize = 0;
}

uint16_t Pokemon4ReadOnlyView::speciesID() const noexcept { return source_.species(); }
const char* Pokemon4ReadOnlyView::species() const noexcept { return Names::getSpeciesName(speciesID()); }
std::u16string Pokemon4ReadOnlyView::nickname() const { return source_.nickname(); }
uint8_t Pokemon4ReadOnlyView::formID() const noexcept { return source_.form(); }
uint8_t Pokemon4ReadOnlyView::form() const noexcept { return source_.form(); }
uint16_t Pokemon4ReadOnlyView::heldItem() const noexcept { return source_.heldItem(); }
uint32_t Pokemon4ReadOnlyView::id32() const noexcept { return source_.id32(); }
uint32_t Pokemon4ReadOnlyView::exp() const noexcept { return source_.experience(); }
uint16_t Pokemon4ReadOnlyView::ability() const noexcept { return source_.ability(); }
uint8_t Pokemon4ReadOnlyView::nature() const noexcept { return static_cast<uint8_t>(source_.pid() % 25u); }
uint8_t Pokemon4ReadOnlyView::statNature() const noexcept { return nature(); }
uint8_t Pokemon4ReadOnlyView::level() const noexcept {
    if (source_.isParty() && source_.partyLevel() >= 1 && source_.partyLevel() <= 100)
        return source_.partyLevel();
    return getLevelFromExp(source_.experience(), source_.personal().growthRate);
}
uint8_t Pokemon4ReadOnlyView::gender() const noexcept { return source_.gender(); }
const char* Pokemon4ReadOnlyView::genderSymbol() const noexcept {
    switch (gender()) {
        case 0: return "♂";
        case 1: return "♀";
        default: return "";
    }
}
uint32_t Pokemon4ReadOnlyView::pid() const noexcept { return source_.pid(); }
uint32_t Pokemon4ReadOnlyView::encryptionConstant() const noexcept { return source_.pid(); }

uint8_t Pokemon4ReadOnlyView::ivHP() const noexcept { return source_.ivs()[0]; }
uint8_t Pokemon4ReadOnlyView::ivATK() const noexcept { return source_.ivs()[1]; }
uint8_t Pokemon4ReadOnlyView::ivDEF() const noexcept { return source_.ivs()[2]; }
uint8_t Pokemon4ReadOnlyView::ivSPE() const noexcept { return source_.ivs()[3]; }
uint8_t Pokemon4ReadOnlyView::ivSPA() const noexcept { return source_.ivs()[4]; }
uint8_t Pokemon4ReadOnlyView::ivSPD() const noexcept { return source_.ivs()[5]; }

uint8_t Pokemon4ReadOnlyView::evHP() const noexcept { return source_.evs()[0]; }
uint8_t Pokemon4ReadOnlyView::evATK() const noexcept { return source_.evs()[1]; }
uint8_t Pokemon4ReadOnlyView::evDEF() const noexcept { return source_.evs()[2]; }
uint8_t Pokemon4ReadOnlyView::evSPE() const noexcept { return source_.evs()[3]; }
uint8_t Pokemon4ReadOnlyView::evSPA() const noexcept { return source_.evs()[4]; }
uint8_t Pokemon4ReadOnlyView::evSPD() const noexcept { return source_.evs()[5]; }

uint16_t Pokemon4ReadOnlyView::move(int slot) const noexcept {
    if (slot < 0 || slot >= 4) return 0;
    return source_.moves()[static_cast<size_t>(slot)];
}
uint8_t Pokemon4ReadOnlyView::movePP(int slot) const noexcept {
    if (slot < 0 || slot >= 4) return 0;
    return source_.pp()[static_cast<size_t>(slot)];
}
uint8_t Pokemon4ReadOnlyView::movePPUps(int slot) const noexcept {
    if (slot < 0 || slot >= 4) return 0;
    return source_.ppUps()[static_cast<size_t>(slot)];
}
uint8_t Pokemon4ReadOnlyView::originGame() const noexcept { return source_.originVersion(); }
Enums::GameVersion Pokemon4ReadOnlyView::getGameGroup() const noexcept { return source_.sourceGroup(); }
uint16_t Pokemon4ReadOnlyView::tid16() const noexcept { return source_.tid(); }
uint16_t Pokemon4ReadOnlyView::sid16() const noexcept { return source_.sid(); }
uint8_t Pokemon4ReadOnlyView::otGender() const noexcept { return source_.originalTrainerGender(); }
uint8_t Pokemon4ReadOnlyView::otFriendship() const noexcept { return source_.friendship(); }
uint8_t Pokemon4ReadOnlyView::language() const noexcept { return source_.language(); }
uint8_t Pokemon4ReadOnlyView::ball() const noexcept {
    return std::max(source_.ballDPPt(), source_.ballHGSS());
}
uint16_t Pokemon4ReadOnlyView::metLocation() const noexcept {
    // PK4's canonical display value always prefers the Pt/HGSS extended field when
    // populated, even when the Pokémon is currently stored in a D/P save.
    if (source_.metLocationExtended() != 0)
        return source_.metLocationExtended();
    return source_.metLocationDP();
}
uint8_t Pokemon4ReadOnlyView::metLevel() const noexcept { return source_.metLevel(); }
uint16_t Pokemon4ReadOnlyView::eggLocation() const noexcept {
    if (source_.eggLocationExtended() != 0)
        return source_.eggLocationExtended();
    return source_.eggLocationDP();
}
bool Pokemon4ReadOnlyView::isNicknamed() const noexcept { return source_.isNicknamed(); }
bool Pokemon4ReadOnlyView::isFatefulEncounter() const noexcept { return source_.fatefulEncounter(); }
std::u16string Pokemon4ReadOnlyView::otName() const { return source_.originalTrainerName(); }

uint8_t Pokemon4ReadOnlyView::baseHP() const noexcept { return source_.personal().hp; }
uint8_t Pokemon4ReadOnlyView::baseATK() const noexcept { return source_.personal().atk; }
uint8_t Pokemon4ReadOnlyView::baseDEF() const noexcept { return source_.personal().def; }
uint8_t Pokemon4ReadOnlyView::baseSPE() const noexcept { return source_.personal().spe; }
uint8_t Pokemon4ReadOnlyView::baseSPA() const noexcept { return source_.personal().spa; }
uint8_t Pokemon4ReadOnlyView::baseSPD() const noexcept { return source_.personal().spd; }

uint16_t Pokemon4ReadOnlyView::calculatedStat(int index) const noexcept {
    if (!source_.valid() || source_.empty()) return 0;
    const auto& p = source_.personal();
    const std::array<uint8_t,6> base{{p.hp,p.atk,p.def,p.spe,p.spa,p.spd}};
    const auto iv = source_.ivs();
    const auto ev = source_.evs();
    const uint32_t lv = level();
    if (index < 0 || index >= 6 || base[static_cast<size_t>(index)] == 0) return 0;
    const uint32_t common =
        ((2u * base[static_cast<size_t>(index)] + iv[static_cast<size_t>(index)] +
          ev[static_cast<size_t>(index)] / 4u) * lv) / 100u;
    if (index == 0) return static_cast<uint16_t>(common + lv + 10u);

    uint32_t value = common + 5u;
    const uint8_t n = nature();
    const int up = n / 5;
    const int down = n % 5;
    const int natureIndex = index - 1; // ATK, DEF, SPE, SPA, SPD
    if (up != down) {
        if (natureIndex == up) value = value * 110u / 100u;
        else if (natureIndex == down) value = value * 90u / 100u;
    }
    return static_cast<uint16_t>(value);
}

uint16_t Pokemon4ReadOnlyView::statHPMax() const noexcept {
    return source_.isParty() ? source_.maxHP() : calculatedStat(0);
}
uint16_t Pokemon4ReadOnlyView::statATK() const noexcept {
    return source_.isParty() ? source_.battleStats()[0] : calculatedStat(1);
}
uint16_t Pokemon4ReadOnlyView::statDEF() const noexcept {
    return source_.isParty() ? source_.battleStats()[1] : calculatedStat(2);
}
uint16_t Pokemon4ReadOnlyView::statSPE() const noexcept {
    return source_.isParty() ? source_.battleStats()[2] : calculatedStat(3);
}
uint16_t Pokemon4ReadOnlyView::statSPA() const noexcept {
    return source_.isParty() ? source_.battleStats()[3] : calculatedStat(4);
}
uint16_t Pokemon4ReadOnlyView::statSPD() const noexcept {
    return source_.isParty() ? source_.battleStats()[4] : calculatedStat(5);
}
uint16_t Pokemon4ReadOnlyView::statHPCurrent() const noexcept {
    return source_.isParty() ? source_.currentHP() : statHPMax();
}

uint8_t Pokemon4ReadOnlyView::friendship() const noexcept { return source_.friendship(); }
bool Pokemon4ReadOnlyView::isEgg() const noexcept { return source_.isEgg(); }
bool Pokemon4ReadOnlyView::isShiny(uint32_t, std::string) const noexcept {
    const uint32_t value = source_.tid() ^ source_.sid() ^ (source_.pid() & 0xFFFFu) ^ (source_.pid() >> 16);
    return value < 8u;
}
bool Pokemon4ReadOnlyView::isPokerusInfected() const noexcept {
    return (source_.pokerusState() & 0x0Fu) != 0;
}
bool Pokemon4ReadOnlyView::isPokerusCured() const noexcept {
    return (source_.pokerusState() & 0xF0u) != 0 && (source_.pokerusState() & 0x0Fu) == 0;
}

uint16_t Pokemon4ReadOnlyView::checksum() const noexcept { return source_.checksum(); }
uint16_t Pokemon4ReadOnlyView::calculateChecksum() const noexcept { return source_.calculatedChecksum(); }
bool Pokemon4ReadOnlyView::checksumValid() const noexcept { return source_.checksumValid(); }

}
