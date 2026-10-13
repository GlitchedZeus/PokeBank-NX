#ifndef POKEBANK_GEN5_CREATE_VALIDATION_H
#define POKEBANK_GEN5_CREATE_VALIDATION_H

#include "Integration/Gen5/Gen5Progression.h"
#include "Integration/Gen5/Gen5MovePP.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

enum class CreateGame5 : uint32_t { Unspecified, White = 20, Black = 21, White2 = 22, Black2 = 23 };
enum class CreateFormat5 { Boxed, Party };
enum class CreateHistory5 { Unspecified, CallerSuppliedNative, Egg, Event, Transfer };
enum class CreateError5 {
    None, Format, Game, SpeciesForm, Level, Nature, IV, EV, Friendship,
    Trainer, Language, Text, PID, Gender, Ability, Moves, History, Verification
};
struct CreateTrainer5 {
    // Invalid defaults force callers to supply identity, including legitimate zero IDs.
    uint32_t tid = 65536, sid = 65536, gender = 2, language = 0;
    std::string name;
    bool operator==(const CreateTrainer5&) const = default;
};
struct CreateEncounter5 {
    CreateHistory5 kind = CreateHistory5::Unspecified;
    uint32_t location = 0, level = 0, ball = 0;
    uint32_t year = 0, month = 0, day = 0; // full calendar year, 2000..2099
    bool operator==(const CreateEncounter5&) const = default;
};
struct CreateMove5 {
    uint32_t id = 0, ppUps = 0;
    // Absent means initialize full native PP; explicit zero means depleted.
    std::optional<uint32_t> pp;
    bool operator==(const CreateMove5&) const = default;
};
struct CreateRequest5 {
    CreateGame5 game = CreateGame5::Unspecified;
    CreateFormat5 format = CreateFormat5::Boxed;
    uint32_t species = 0, form = 0, level = 0, nature = 25;
    uint32_t gender = 3, abilitySlot = 3, friendship = 256;
    std::optional<uint32_t> pid;
    std::optional<CreateTrainer5> trainer;
    CreateEncounter5 encounter;
    // Explicit custom nickname only: no guessed/localized species name.
    std::string nickname;
    // Native order: HP, Attack, Defense, Speed, Sp. Attack, Sp. Defense.
    std::array<uint32_t, 6> ivs{}, evs{};
    std::array<CreateMove5, 4> moves{};
    bool operator==(const CreateRequest5&) const = default;
};
struct CreateValidation5 {
    CreateError5 error = CreateError5::None;
    std::string_view message;
    explicit operator bool() const noexcept { return error == CreateError5::None; }
};

// Isolated correction for PR #300's Erratic rounding defect. All other curves
// reuse native progression. Reference: pinned PKHeX Experience.Growth1 table.
// Coordinator patch is supplied separately; do not change the shared editor here.
constexpr uint32_t createExperience5(uint8_t growth, uint32_t level) noexcept {
    if (level < 1 || level > 100 || growth > 5) return 0;
    if (growth == 1 && level >= 69 && level <= 98)
        return level * level * level * ((1911 - 10 * level) / 3) / 500;
    return experienceForLevel5(growth, level);
}
constexpr uint8_t createGenderForPID5(uint8_t ratio, uint32_t pid) noexcept {
    if (ratio == 255) return 2;
    if (ratio == 254) return 1;
    if (ratio == 0) return 0;
    return (pid & 255) < ratio ? 1 : 0;
}
inline bool createAscii5(std::string_view text, size_t max) noexcept {
    if (text.empty() || text.size() > max) return false;
    for (const unsigned char c : text) if (c < 0x20 || c > 0x7e) return false;
    return true;
}
inline bool createDate5(const CreateEncounter5& e) noexcept {
    if (e.year < 2000 || e.year > 2099 || e.month < 1 || e.month > 12) return false;
    constexpr std::array<uint32_t, 12> days{31,28,31,30,31,30,31,31,30,31,30,31};
    const uint32_t limit = days[e.month - 1] + (e.month == 2 && e.year % 4 == 0 ? 1 : 0);
    return e.day >= 1 && e.day <= limit;
}

[[nodiscard]] inline CreateValidation5 validateCreate5(const CreateRequest5& r) {
    using E = CreateError5;
    if (r.format != CreateFormat5::Boxed)
        return {E::Format, "Party creation is gated: native extension initialization is not certified"};
    const auto version = static_cast<uint32_t>(r.game);
    if (version < 20 || version > 23) return {E::Game, "Select exact Black/White/Black 2/White 2 title"};
    const auto p = personal5(r.species, r.form, version >= 22);
    if (!p || !p.info->hp || p.info->growthRate > 5)
        return {E::SpeciesForm, "Species/form has no exact native personal data in this title"};
    // This foundation has no held-item or battle-state initialization. Keep
    // dependent forms gated rather than constructing a misleading combination.
    if (r.form && (r.species == 351 || r.species == 421 || r.species == 487 ||
                   r.species == 492 || r.species == 493 || r.species == 555 ||
                   r.species == 648 || r.species == 649))
        return {E::SpeciesForm, "This form needs unsupported item, battle or reversion handling"};
    if (r.level < 1 || r.level > 100) return {E::Level, "Level must be 1..100"};
    if (r.nature > 24) return {E::Nature, "Nature must be 0..24"};
    uint32_t totalEV = 0;
    for (size_t i = 0; i < 6; ++i) {
        if (r.ivs[i] > 31) return {E::IV, "Native IV must be 0..31"};
        if (r.evs[i] > 255) return {E::EV, "Native Gen V EV must be 0..255"};
        totalEV += r.evs[i];
    }
    if (totalEV > 510) return {E::EV, "Total EVs must not exceed 510"};
    if (r.friendship > 255) return {E::Friendship, "Supply friendship in 0..255"};
    if (!r.trainer || r.trainer->tid > 65535 || r.trainer->sid > 65535 || r.trainer->gender > 1)
        return {E::Trainer, "Explicit native TID, SID and OT gender are required"};
    const auto language = r.trainer->language;
    if (language != 2 && language != 3 && language != 4 && language != 5 && language != 7)
        return {E::Language, "Create currently supports ASCII text in Western-language records only"};
    if (!createAscii5(r.nickname, 10) || !createAscii5(r.trainer->name, 7))
        return {E::Text, "Supply ASCII nickname (1..10) and OT name (1..7); no truncation or substitution"};
    if (!r.pid) return {E::PID, "Explicit PID required; Create never rerolls identity or shininess"};
    if (r.gender > 2 || r.gender != createGenderForPID5(p.info->genderRatio, *r.pid))
        return {E::Gender, "Gender must agree with the native ratio and supplied PID"};
    if (r.abilitySlot > 2 || abilityForSlot5(*p.info, r.abilitySlot) == 0 ||
        (r.abilitySlot < 2 && ((*r.pid >> 16) & 1) != r.abilitySlot))
        return {E::Ability, "Ability slot must exist and normal slot must agree with PID bit 16"};
    bool empty = false;
    for (size_t i = 0; i < r.moves.size(); ++i) {
        const auto& m = r.moves[i];
        if (!m.id) {
            if (i == 0 || m.ppUps || m.pp.value_or(0))
                return {E::Moves, "First move required; empty slots must have zero PP and PP Ups"};
            empty = true;
        } else {
            if (empty || !validNativeMove5(m.id) || m.ppUps > 3 ||
                (basePP5(m.id) == 1 && m.ppUps != 0) ||
                m.pp.value_or(maxPP5(m.id, m.ppUps)) > maxPP5(m.id, m.ppUps))
                return {E::Moves, "Invalid native move, PP, PP Ups or noncontiguous move slots"};
            for (size_t j = 0; j < i; ++j) if (m.id == r.moves[j].id)
                return {E::Moves, "Duplicate move slots are unsupported"};
        }
    }
    const auto& e = r.encounter;
    if (e.kind != CreateHistory5::CallerSuppliedNative)
        return {E::History, "Explicit native non-egg history required; event, egg and transfer creation unsupported"};
    if (!createDate5(e) || !e.location || e.location > 153 || !isNativeMetLocation5(e.location) ||
        e.level < 1 || e.level > r.level || e.ball < 1 || e.ball > 25)
        return {E::History, "Supply a valid date, native ordinary location, met level and ball; none are inferred"};
    return {};
}
static_assert(createExperience5(1,70) == 276458);
static_assert(createExperience5(1,95) == 548720);
static_assert(createExperience5(1,98) == 583539);
} // namespace PokeVault::Integration::Gen5
#endif
