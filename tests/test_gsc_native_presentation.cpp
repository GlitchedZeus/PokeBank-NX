#include "UI/Gen2NativePresentation.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    namespace Native = PokeBank::UIModel::Gen2Native;
    using PokeVault::Integration::Gen2::PokemonRecord;

    // Crystal byte layout: time[15:14], level[13:8], OT gender[7], location[6:0].
    constexpr uint16_t morningGoldenrodFemale =
        static_cast<uint16_t>((1u << 14) | (20u << 8) | (1u << 7) | 0x10u);
    constexpr auto caught = Native::decodeCrystalCaughtData(morningGoldenrodFemale);
    static_assert(caught.present);
    static_assert(caught.timeOfDay == 1);
    static_assert(caught.levelCode == 20);
    static_assert(caught.originalTrainerFemale);
    static_assert(caught.location == 0x10);
    assert(std::string(Native::crystalMetTimeName(caught.timeOfDay)) == "Morning");
    assert(Native::crystalCaughtLevelText(caught) == "Lv 20");
    assert(std::string(Native::crystalOriginalTrainerGenderText(caught)) == "Female");
    assert(std::string(Native::crystalCaughtLocationName(caught.location)) == "Goldenrod City");

    constexpr uint16_t nightSilverCaveMale =
        static_cast<uint16_t>((3u << 14) | (55u << 8) | 0x2Eu);
    constexpr auto night = Native::decodeCrystalCaughtData(nightSilverCaveMale);
    static_assert(night.timeOfDay == 3 && night.levelCode == 55 && !night.originalTrainerFemale);
    assert(std::string(Native::crystalMetTimeName(night.timeOfDay)) == "Night");
    assert(std::string(Native::crystalCaughtLocationName(night.location)) == "Silver Cave");

    constexpr auto absent = Native::decodeCrystalCaughtData(0);
    static_assert(!absent.present);
    assert(Native::crystalCaughtLevelText(absent) == "Unknown");
    assert(std::string(Native::crystalOriginalTrainerGenderText(absent)) == "Unknown");
    assert(std::string(Native::crystalCaughtLocationName(0)) == "Unknown");
    assert(std::string(Native::crystalCaughtLocationName(0x7E)) == "Gift");
    assert(std::string(Native::crystalCaughtLocationName(0x7F)) == "Event");
    assert(std::string(Native::crystalCaughtLocationName(0x60)) == "Unused / invalid");

    constexpr auto egg = Native::decodeCrystalCaughtData(
        static_cast<uint16_t>((2u << 14) | (1u << 8) | 0x7Eu));
    assert(Native::crystalCaughtLevelText(egg) == "Egg marker");
    assert(std::string(Native::crystalMetTimeName(egg.timeOfDay)) == "Day");

    // Party-only fields are exposed only when the record actually came from a party structure.
    PokemonRecord party;
    party.partyRecord = true;
    party.currentHP = 35;
    party.maxHP = 50;
    party.status = 0x40;
    auto partyView = Native::partyViewData(party);
    assert(partyView);
    assert(partyView->currentHP == 35 && partyView->maxHP == 50);
    assert(partyView->statusText == "Paralyzed");
    assert(Native::partyStatusText(0x08, 20) == "Poisoned");
    assert(Native::partyStatusText(0x10, 20) == "Burned");
    assert(Native::partyStatusText(0x20, 20) == "Frozen");
    assert(Native::partyStatusText(0x03, 20) == "Asleep (3)");
    assert(Native::partyStatusText(0, 0) == "Fainted");

    PokemonRecord box = party;
    box.partyRecord = false;
    box.currentHP = 999; // Deliberately nonsensical: a box presentation must ignore these bytes.
    box.status = 0x08;
    assert(!Native::partyViewData(box));

    // Pokérus helper is exact-byte round-trip infrastructure for both passive display and editor UX.
    for (uint8_t strain = 0; strain < 16; ++strain) {
        for (uint8_t days = 0; days < 16; ++days) {
            const uint8_t raw = Native::encodePokerus(strain, days);
            const auto decoded = Native::decodePokerus(raw);
            assert(decoded.strain == strain);
            assert(decoded.days == days);
        }
    }
    assert(Native::pokerusText(0) == "None");
    assert(Native::pokerusText(Native::encodePokerus(3, 4)) == "S3 / 4d");
    assert(Native::pokerusText(Native::encodePokerus(3, 0)) == "S3 / cured");

    std::cout << "GSC Crystal/party native presentation: PASS\n";
    return 0;
}
