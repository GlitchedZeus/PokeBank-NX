#ifndef POKEBANK_GEN5_STAGED_RECORD_H
#define POKEBANK_GEN5_STAGED_RECORD_H

#include "Pokemon/Pokemon5ReadOnly.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5Progression.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Entity-only Gen V staging. Not a SAV writer, source provider, legality
// auto-fixer, or shared-editor UI. Every transaction operates on app-owned
// buffers and must pass a decrypt/encrypt/reparse postcondition.
class StagedPokemon5Record {
public:
    enum class Field : uint8_t { Nature, Friendship, IV, EV, HeldItem, Ball, MetLevel,
                                Move, MovePP, MovePPUps, TrainerId, SecretId, OriginalTrainerGender,
                                Pokerus, MetLocation, FatefulEncounter, Form, AbilitySlot, Gender, Level, Experience };

    static std::optional<StagedPokemon5Record> create(
        std::span<const uint8_t> source, std::string* error = nullptr,
        bool black2White2 = false) {
        const Pokemon5ReadOnly parsed(source);
        if (!parsed.valid() || parsed.empty()) {
            if (error) *error = "Gen V edit requires a valid occupied PK5 record";
            return std::nullopt;
        }
        if (error) error->clear();
        return StagedPokemon5Record(std::vector<uint8_t>(source.begin(), source.end()),
                                    black2White2);
    }

    [[nodiscard]] std::span<const uint8_t> originalBytes() const noexcept { return original_; }
    [[nodiscard]] std::span<const uint8_t> stagedBytes() const noexcept { return staged_; }
    [[nodiscard]] bool dirty() const noexcept { return staged_ != original_; }
    [[nodiscard]] Pokemon5ReadOnly current() const { return Pokemon5ReadOnly(staged_); }
    void rollback() noexcept { staged_ = original_; }

    // Field-specific bounded mutation. Item and ball IDs here are native
    // ranges, NOT a legality promise for an encounter/title. Move legality,
    // species replacement, party-stat regeneration and native SAV block
    // writes remain outside this record-only transaction.
    [[nodiscard]] bool stage(Field field, size_t stat, uint32_t value,
                             std::string* error = nullptr) {
        auto fail = [&](const char* msg) {
            if (error) *error = msg;
            return false;
        };
        const std::vector<uint8_t> previous=Crypto::decrypt(staged_);
        std::vector<uint8_t> candidate=previous;
        if (!Crypto::recordSizeValid(candidate.size()))
            return fail("Gen V staged source has invalid PK5 size");

        // Identical encrypted PK5 layout in BW and B2W2, but native
        // Personal tables (abilities/forms/base stats) differ. Never use a
        // modern Gen IX personal table for Gen V identity mutations.
        const uint16_t species=Crypto::read16(candidate,0x08);
        const uint8_t sourceForm=static_cast<uint8_t>(candidate[0x40]>>3);
        const auto personal=personal5(species,sourceForm,black2White2_);
        switch (field) {
            case Field::Form: {
                if(stat!=0 || value>31)
                    return fail("Gen V form is outside the native 5-bit range");
                const auto next=personal5(species,value,black2White2_);
                if(!personal || !next)
                    return fail("Gen V form does not exist in this exact BW/B2W2 title");
                if(value==sourceForm)break;
                // Party battle stats are regenerated after the native identity
                // transaction, using the target form's exact personal record.
                const uint8_t currentSlot=(candidate[0x42]&1)?2:
                    static_cast<uint8_t>((Crypto::read32(candidate,0)>>16)&1);
                const uint8_t chosen=abilityForSlot5(*next.info,currentSlot);
                // Keep the same PID and selected ability slot where possible.
                // An unusual existing ability is reconciled to the target
                // native form without silently rerolling PID/shininess.
                const uint8_t targetSlot=(chosen==0)
                    ? static_cast<uint8_t>((Crypto::read32(candidate,0)>>16)&1)
                    : currentSlot;
                const uint8_t targetAbility=abilityForSlot5(*next.info,targetSlot);
                if(targetAbility==0)
                    return fail("Gen V target form has no supported native ability");
                candidate[0x40]=static_cast<uint8_t>(
                    (candidate[0x40]&0x07) | (value<<3));
                candidate[0x15]=targetAbility;
                candidate[0x42]=static_cast<uint8_t>(
                    (candidate[0x42]&~uint8_t{1}) | (targetSlot==2?1:0));
                const uint8_t gender=static_cast<uint8_t>((candidate[0x40]>>1)&3);
                const auto nextGender=normalizedGender5(next.info->genderRatio,gender);
                candidate[0x40]=static_cast<uint8_t>(
                    (candidate[0x40]&~uint8_t{6}) | (nextGender<<1));
                break;
            }
            case Field::AbilitySlot: {
                if(stat!=0 || value>2 || !personal)
                    return fail("Gen V ability requires an exact native form and slot 0..2");
                const uint8_t ability=abilityForSlot5(*personal.info,value);
                if(ability==0)
                    return fail("Gen V selected ability slot is not present for this form");
                const uint32_t beforePID=Crypto::read32(candidate,0);
                const uint32_t newPID=value==2?beforePID:
                    ((beforePID&~(1u<<16))|(value<<16));
                // Changing the PID ability bit must not silently change shiny
                // status or hidden/coupled game metadata.
                const uint16_t tid=Crypto::read16(candidate,0x0C);
                const uint16_t sid=Crypto::read16(candidate,0x0E);
                auto shiny=[&](uint32_t pid) {
                    return (tid^sid^static_cast<uint16_t>(pid)^
                            static_cast<uint16_t>(pid>>16))<8;
                };
                if(shiny(beforePID)!=shiny(newPID))
                    return fail("Gen V ability would change shininess; keep existing PID");
                Crypto::write16(candidate,0,static_cast<uint16_t>(newPID));
                Crypto::write16(candidate,2,static_cast<uint16_t>(newPID>>16));
                candidate[0x15]=ability;
                candidate[0x42]=static_cast<uint8_t>(
                    (candidate[0x42]&~uint8_t{1})|(value==2?1:0));
                break;
            }
            case Field::Gender: {
                if(stat!=0 || value>2 || !personal ||
                   !genderAllowed5(personal.info->genderRatio,value))
                    return fail("Gen V gender is incompatible with this form's native ratio");
                candidate[0x40]=static_cast<uint8_t>(
                    (candidate[0x40]&~uint8_t{6}) | (value<<1));
                break;
            }
            case Field::Level:
                if(stat!=0 || value<1 || value>100 || !personal ||
                   personal.info->growthRate>5)
                    return fail("Gen V level requires exact personal growth and 1..100");
                if(Crypto::read32(candidate,0x10)==
                   experienceForLevel5(personal.info->growthRate,value))
                    break;
                Crypto::write16(candidate,0x10,static_cast<uint16_t>(
                    experienceForLevel5(personal.info->growthRate,value)));
                Crypto::write16(candidate,0x12,static_cast<uint16_t>(
                    experienceForLevel5(personal.info->growthRate,value)>>16));
                if(candidate.size()==Crypto::PartySize)
                    candidate[0x8C]=static_cast<uint8_t>(value);
                break;
            case Field::Experience:
                if(stat!=0 || !personal || personal.info->growthRate>5 ||
                   value>experienceForLevel5(personal.info->growthRate,100))
                    return fail("Gen V EXP exceeds this species' native level-100 limit");
                if(Crypto::read32(candidate,0x10)==value)break;
                Crypto::write16(candidate,0x10,static_cast<uint16_t>(value));
                Crypto::write16(candidate,0x12,static_cast<uint16_t>(value>>16));
                if(candidate.size()==Crypto::PartySize)
                    candidate[0x8C]=levelForExperience5(personal.info->growthRate,value);
                break;
            case Field::Nature:
                if (stat != 0 || value > 24)
                    return fail("Gen V nature index must be in [0,24]");
                candidate[0x41] = static_cast<uint8_t>(value);
                break;
            case Field::Friendship:
                if (stat != 0 || value > 255)
                    return fail("Gen V friendship must be in [0,255]");
                candidate[0x14] = static_cast<uint8_t>(value);
                break;
            case Field::IV: {
                if (stat >= 6 || value > 31)
                    return fail("Gen V IV stat/value is out of native range");
                constexpr size_t offset = 0x38;
                const uint32_t shift = static_cast<uint32_t>(stat * 5);
                uint32_t iv32 = Crypto::read32(candidate, offset);
                iv32 = (iv32 & ~(0x1Fu << shift)) | (value << shift);
                Crypto::write16(candidate, offset, static_cast<uint16_t>(iv32));
                Crypto::write16(candidate, offset+2, static_cast<uint16_t>(iv32 >> 16));
                break;
            }
            case Field::EV: {
                if (stat >= 6 || value > 255)
                    return fail("Gen V EV stat/value is out of native range");
                uint32_t total = value;
                for (size_t i=0; i<6; ++i)
                    if (i != stat) total += candidate[0x18+i];
                if (total > 510)
                    return fail("Gen V EV total cannot exceed 510");
                candidate[0x18+stat] = static_cast<uint8_t>(value);
                break;
            }
            case Field::HeldItem:
                if (stat != 0 || value > 638)
                    return fail("Gen V held-item ID exceeds supported Generation V range");
                Crypto::write16(candidate,0x0A,static_cast<uint16_t>(value));
                break;
            case Field::Ball:
                if (stat != 0 || value > 25)
                    return fail("Gen V ball ID exceeds supported Generation V range");
                candidate[0x83]=static_cast<uint8_t>(value);
                break;
            case Field::MetLevel:
                if (stat != 0 || value > 100)
                    return fail("Gen V met level must be in [0,100]");
                // Preserve the high bit: original trainer gender.
                candidate[0x84]=static_cast<uint8_t>(
                    (candidate[0x84] & 0x80) | value);
                break;
            case Field::TrainerId:
                if(stat!=0 || value>65535)
                    return fail("Gen V trainer ID exceeds native 16-bit range");
                Crypto::write16(candidate,0x0C,static_cast<uint16_t>(value));
                break;
            case Field::SecretId:
                if(stat!=0 || value>65535)
                    return fail("Gen V secret ID exceeds native 16-bit range");
                Crypto::write16(candidate,0x0E,static_cast<uint16_t>(value));
                break;
            case Field::OriginalTrainerGender:
                if(stat!=0 || value>1)
                    return fail("Gen V original trainer gender must be male/female");
                // Met level and OT gender share one byte; never lose level.
                candidate[0x84]=static_cast<uint8_t>(
                    (candidate[0x84]&0x7F) | (value<<7));
                break;
            case Field::FatefulEncounter:
                if(stat!=0 || value>1)
                    return fail("Gen V fateful encounter must be off/on");
                // Preserve native form and individual gender flags.
                candidate[0x40]=static_cast<uint8_t>(
                    (candidate[0x40]&~uint8_t{1}) | value);
                break;
            case Field::MetLocation:
                if(stat!=0 || !isNativeMetLocation5(value))
                    return fail("Gen V met location not in pinned BW/B2W2 location set");
                Crypto::write16(candidate,0x80,static_cast<uint16_t>(value));
                break;
            case Field::Pokerus: {
                if(stat!=0 || value>2)
                    return fail("Gen V Pokerus mode must be None/Cured/Infected");
                const uint8_t old=candidate[0x82];
                const auto originalDecoded=Crypto::decrypt(original_);
                const uint8_t baseline=originalDecoded[0x82];
                const uint8_t baselineMode=baseline==0?0:((baseline&0x0F)?2:1);
                const uint8_t mode=old==0?0:((old&0x0F)?2:1);
                // If a user toggles back to the original mode, restore the
                // exact original strain and duration. Otherwise a transient
                // toggle could silently rewrite an unrelated duration even
                // though the visible value returns to its original state.
                if(value==baselineMode)candidate[0x82]=baseline;
                else if(mode!=value) {
                    if(value==0)candidate[0x82]=0;
                    else {
                        // Preserve existing strain; brand-new infections
                        // receive strain 1, days=1 (within native maximum).
                        const uint8_t strain=(old&0xF0)?(old&0xF0):0x10;
                        candidate[0x82]=static_cast<uint8_t>(
                            strain | (value==2?1:0));
                    }
                }
                break;
            }
            case Field::Move: {
                if(stat>=4 || !validNativeMove5(value))
                    return fail("Gen V move ID/slot outside the native BW/B2W2 range");
                const size_t offset=0x28+stat*2;
                const uint16_t prior=Crypto::read16(candidate,offset);
                if(prior!=value) {
                    Crypto::write16(candidate,offset,static_cast<uint16_t>(value));
                    // Selecting a different move resets only its own PP/PP Ups,
                    // using the pinned Gen V PP table, never modern fallback.
                    candidate[0x30+stat]=basePP5(value);
                    candidate[0x34+stat]=0;
                }
                break;
            }
            case Field::MovePPUps: {
                if(stat>=4 || value>3)
                    return fail("Gen V PP Ups require a native slot and 0..3 uses");
                const uint16_t move=Crypto::read16(candidate,0x28+stat*2);
                if(!validNativeMove5(move) || (move==0 && value!=0))
                    return fail("Gen V PP Ups cannot be applied to an absent/unknown move");
                if(candidate[0x34+stat]!=value) {
                    candidate[0x34+stat]=static_cast<uint8_t>(value);
                    candidate[0x30+stat]=maxPP5(move,value);
                }
                break;
            }
            case Field::MovePP: {
                if(stat>=4 || value>255)
                    return fail("Gen V move PP requires a native slot and byte range");
                const uint16_t move=Crypto::read16(candidate,0x28+stat*2);
                const uint8_t ups=candidate[0x34+stat];
                if(!validNativeMove5(move) || ups>3 ||
                   value>maxPP5(move,ups))
                    return fail("Gen V current PP exceeds the move's native PP maximum");
                candidate[0x30+stat]=static_cast<uint8_t>(value);
                break;
            }
            default:
                // A corrupted/forged enum is not a noop mutation. It must
                // never be accepted as a valid native PK5 field transaction.
                return fail("Gen V staged editor field is unsupported");
        }

        // Stat-dependent native edits must never leave stale party battle
        // caches: HP/current HP, stats and level are part of the PK5 extension.
        // Recalculate only when a meaningful stat-related byte changed.
        const bool statEdit=field==Field::Nature || field==Field::IV ||
            field==Field::EV || field==Field::Form ||
            field==Field::Level || field==Field::Experience;
        if(statEdit && candidate.size()==Crypto::PartySize &&
           candidate!=previous && !regeneratePartyStats5(candidate,Crypto::decrypt(original_),black2White2_))
            return fail("Gen V party battle statistics cannot be safely regenerated");

        const auto encrypted = Crypto::encryptCandidate(candidate);
        const Pokemon5ReadOnly strict(encrypted);
        if (!strict.valid() || strict.empty())
            return fail("Gen V staged PK5 failed strict round-trip validation");
        const auto decoded = Crypto::decrypt(encrypted);
        // PK5 encryption is deterministic. Header checksum bytes 6-7 are the
        // *only* expected serialization change beyond the intended edit.
        Crypto::write16(candidate, 6, Crypto::checksum(candidate));
        if (decoded != candidate)
            return fail("Gen V staged PK5 candidate altered unrelated fields");

        // Commit atomically only after all checks have succeeded.
        staged_ = encrypted;
        if (error) error->clear();
        return true;
    }

private:
    explicit StagedPokemon5Record(std::vector<uint8_t> source,bool black2White2)
        : original_(source), staged_(std::move(source)),
          black2White2_(black2White2) {}

    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    bool black2White2_=false;
};

} // namespace PokeVault::Integration::Gen5
#endif
