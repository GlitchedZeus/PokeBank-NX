#include "Pokemon/Pokemon4Mutable.h"

#include "Encryption/Encryption4.h"
#include "Pokemon/Experience.h"
#include "Pokemon/PersonalInfo4DP.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Names/NameLanguage.h"
#include "Names/SpeciesNames.h"
#include "Utils/StringHelpers.h"
#include "Enums/LanguageID.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <utility>

namespace Pokemon {
namespace {

const PersonalRecord& personalFor(Enums::GameVersion group,
                                  uint16_t species,
                                  uint8_t form) noexcept {
    switch (group) {
        case Enums::GameVersion::DP: return getPersonalInfo4DP(species, form);
        case Enums::GameVersion::PT: return getPersonalInfo4PT(species, form);
        case Enums::GameVersion::HGSS: return getPersonalInfo4HGSS(species, form);
        default: return PERSONAL_RECORD_EMPTY;
    }
}

uint8_t fixedGender(uint8_t ratio) noexcept {
    if (ratio == 255) return 2;
    if (ratio == 254) return 1;
    if (ratio == 0) return 0;
    return 3;
}

} // namespace

Pokemon4Mutable::Pokemon4Mutable(std::vector<std::byte> decrypted,
                                 Enums::GameVersion sourceGroup) noexcept
    : decrypted_(std::move(decrypted)), sourceGroup_(sourceGroup), valid_(true) {}

std::optional<Pokemon4Mutable> Pokemon4Mutable::fromEncrypted(
    std::span<const std::byte> encrypted,
    Enums::GameVersion sourceGroup,
    std::string* error) {
    Pokemon4ReadOnly parsed(encrypted, sourceGroup);
    if (!parsed.valid() || parsed.empty()) {
        if (error) *error = parsed.empty()
            ? "cannot edit an empty PK4 record"
            : "PK4 failed strict checksum/sanity validation";
        return std::nullopt;
    }
    if (error) error->clear();
    return Pokemon4Mutable(
        std::vector<std::byte>(parsed.decryptedBytes().begin(), parsed.decryptedBytes().end()),
        sourceGroup);
}

std::optional<Pokemon4Mutable> Pokemon4Mutable::createStored(
    uint16_t species,
    Enums::GameVersion sourceGroup,
    uint8_t originVersion,
    const std::u16string& originalTrainerName,
    uint16_t tid,
    uint16_t sid,
    uint8_t originalTrainerGender,
    uint8_t language,
    uint8_t level,
    uint32_t pidSeed,
    std::string* error) {
    const auto fail = [&](const char* message) -> std::optional<Pokemon4Mutable> {
        if (error) *error = message;
        return std::nullopt;
    };
    if (species == 0 || species > 493) return fail("Generation IV Create species must be 1-493");
    if (sourceGroup != Enums::GameVersion::DP && sourceGroup != Enums::GameVersion::PT &&
        sourceGroup != Enums::GameVersion::HGSS)
        return fail("Generation IV Create requires a DP/Pt/HGSS source group");
    if (level < 1 || level > 100) return fail("Generation IV Create level must be 1-100");
    if (originVersion != static_cast<uint8_t>(Enums::GameVersion::D) &&
        originVersion != static_cast<uint8_t>(Enums::GameVersion::P) &&
        originVersion != static_cast<uint8_t>(Enums::GameVersion::Pt) &&
        originVersion != static_cast<uint8_t>(Enums::GameVersion::HG) &&
        originVersion != static_cast<uint8_t>(Enums::GameVersion::SS))
        return fail("Generation IV Create requires an exact native origin game");
    if (originalTrainerGender > 1) return fail("Generation IV trainer gender is outside the native range");
    if (!Enums::groupHasLanguage(sourceGroup, language))
        return fail("Generation IV Create language is unavailable in this source group");

    const auto& personal = personalFor(sourceGroup, species, 0);
    if (personal.hp == 0) return fail("Generation IV species is not representable in this exact game");

    std::vector<std::byte> decrypted(Encryption::SIZE_STORED4, std::byte{0});
    Pokemon4Mutable result(std::move(decrypted), sourceGroup);
    result.write32(0x00, pidSeed == 0 ? 1u : pidSeed);
    result.write16(0x04, 0); // sanity
    result.write16(0x08, species);
    result.write16(0x0A, 0); // held item
    result.write16(0x0C, tid);
    result.write16(0x0E, sid);
    result.write32(0x10, getExpForLevel(level, personal.growthRate));
    result.write8(0x14, personal.baseFriendship);
    const uint8_t abilitySlot =
        (personal.ability2 != 0 && personal.ability2 != personal.ability1)
            ? static_cast<uint8_t>(result.pid() & 1u) : 0;
    result.write8(0x15, static_cast<uint8_t>(abilitySlot ? personal.ability2 : personal.ability1));
    result.write8(0x17, language);

    const uint8_t nativeGender = result.genderForPid(result.pid());
    result.write8(0x40, static_cast<uint8_t>((nativeGender & 3u) << 1));

    const std::size_t nameLanguage = Names::languageIndexFor(
        static_cast<Enums::LanguageID>(language));
    const auto speciesName = Utils::utf8ToUtf16(
        Names::getSpeciesNameLocalized(species, nameLanguage));
    const auto nick = Utils::encodeGen4Field(speciesName, 11, 10, language);
    if (Utils::decodeGen4Field(nick) != speciesName.substr(0, std::min<size_t>(10, speciesName.size())))
        return fail("Generation IV species name is not representable in the selected save language");
    std::copy(nick.begin(), nick.end(), result.decrypted_.begin() + 0x48);

    result.write8(0x5F, originVersion);
    const auto ot = Utils::encodeGen4Field(originalTrainerName, 8, 7, language);
    const auto boundedOt = originalTrainerName.substr(0, std::min<size_t>(7, originalTrainerName.size()));
    if (Utils::decodeGen4Field(ot) != boundedOt)
        return fail("Generation IV trainer name is not representable in the selected save language");
    std::copy(ot.begin(), ot.end(), result.decrypted_.begin() + 0x68);

    result.write16(0x44, 0); // extended egg location
    result.write16(0x46, 0); // extended met location
    result.write16(0x7E, 0); // DP egg location
    result.write16(0x80, 0); // DP met location
    result.write8(0x82, 0);  // Pokerus none
    if (!result.setBall(4)) return fail("Generation IV default Poke Ball could not be encoded");
    result.write8(0x84, static_cast<uint8_t>((originalTrainerGender << 7) | (level & 0x7Fu)));

    const auto encrypted = result.encryptedBytes();
    Pokemon4ReadOnly reparsed(encrypted, sourceGroup);
    if (!reparsed.valid() || reparsed.empty() || reparsed.isParty() ||
        reparsed.species() != species || reparsed.tid() != tid || reparsed.sid() != sid ||
        reparsed.originVersion() != originVersion || reparsed.language() != language ||
        reparsed.metLevel() != level || reparsed.originalTrainerGender() != originalTrainerGender)
        return fail("Generation IV created PK4 failed strict round-trip validation");

    if (error) error->clear();
    return result;
}

bool Pokemon4Mutable::isParty() const noexcept {
    return decrypted_.size() == Encryption::SIZE_PARTY4;
}

uint8_t Pokemon4Mutable::byteAt(size_t offset) const noexcept {
    return offset < decrypted_.size() ? static_cast<uint8_t>(decrypted_[offset]) : 0;
}

uint16_t Pokemon4Mutable::u16At(size_t offset) const noexcept {
    if (offset + 1 >= decrypted_.size()) return 0;
    return static_cast<uint16_t>(
        byteAt(offset) | (static_cast<uint16_t>(byteAt(offset + 1)) << 8));
}

uint32_t Pokemon4Mutable::u32At(size_t offset) const noexcept {
    if (offset + 3 >= decrypted_.size()) return 0;
    return static_cast<uint32_t>(byteAt(offset)) |
           (static_cast<uint32_t>(byteAt(offset + 1)) << 8) |
           (static_cast<uint32_t>(byteAt(offset + 2)) << 16) |
           (static_cast<uint32_t>(byteAt(offset + 3)) << 24);
}

void Pokemon4Mutable::write8(size_t offset, uint8_t value) noexcept {
    if (offset < decrypted_.size()) decrypted_[offset] = static_cast<std::byte>(value);
}

void Pokemon4Mutable::write16(size_t offset, uint16_t value) noexcept {
    if (offset + 1 >= decrypted_.size()) return;
    write8(offset, static_cast<uint8_t>(value));
    write8(offset + 1, static_cast<uint8_t>(value >> 8));
}

void Pokemon4Mutable::write32(size_t offset, uint32_t value) noexcept {
    if (offset + 3 >= decrypted_.size()) return;
    write8(offset, static_cast<uint8_t>(value));
    write8(offset + 1, static_cast<uint8_t>(value >> 8));
    write8(offset + 2, static_cast<uint8_t>(value >> 16));
    write8(offset + 3, static_cast<uint8_t>(value >> 24));
}

std::vector<std::byte> Pokemon4Mutable::encryptedBytes() const {
    if (!valid_) return {};
    return Encryption::encryptArray4(decrypted_);
}

uint32_t Pokemon4Mutable::pid() const noexcept { return u32At(0x00); }
uint16_t Pokemon4Mutable::species() const noexcept { return u16At(0x08); }
uint16_t Pokemon4Mutable::heldItem() const noexcept { return u16At(0x0A); }
uint16_t Pokemon4Mutable::tid() const noexcept { return u16At(0x0C); }
uint16_t Pokemon4Mutable::sid() const noexcept { return u16At(0x0E); }
uint32_t Pokemon4Mutable::experience() const noexcept { return u32At(0x10); }
uint8_t Pokemon4Mutable::friendship() const noexcept { return byteAt(0x14); }
uint16_t Pokemon4Mutable::ability() const noexcept { return byteAt(0x15); }
uint8_t Pokemon4Mutable::language() const noexcept { return byteAt(0x17); }
uint8_t Pokemon4Mutable::level() const noexcept {
    const auto& personal = personalFor(sourceGroup_, species(), form());
    return personal.hp == 0 ? 1 : getLevelFromExp(experience(), personal.growthRate);
}
std::u16string Pokemon4Mutable::nickname() const {
    if (!valid_) return {};
    return Utils::decodeGen4Field(std::span<const std::byte>(decrypted_.data() + 0x48, 22));
}
std::array<uint8_t, 6> Pokemon4Mutable::ivs() const noexcept {
    const uint32_t v = u32At(0x38);
    return {static_cast<uint8_t>((v >> 0) & 31u), static_cast<uint8_t>((v >> 5) & 31u),
            static_cast<uint8_t>((v >> 10) & 31u), static_cast<uint8_t>((v >> 15) & 31u),
            static_cast<uint8_t>((v >> 20) & 31u), static_cast<uint8_t>((v >> 25) & 31u)};
}
std::array<uint8_t, 6> Pokemon4Mutable::evs() const noexcept {
    return {byteAt(0x18), byteAt(0x19), byteAt(0x1A), byteAt(0x1B), byteAt(0x1C), byteAt(0x1D)};
}
std::array<uint16_t, 4> Pokemon4Mutable::moves() const noexcept {
    return {u16At(0x28), u16At(0x2A), u16At(0x2C), u16At(0x2E)};
}
std::array<uint8_t, 4> Pokemon4Mutable::pp() const noexcept {
    return {byteAt(0x30), byteAt(0x31), byteAt(0x32), byteAt(0x33)};
}
std::array<uint8_t, 4> Pokemon4Mutable::ppUps() const noexcept {
    return {byteAt(0x34), byteAt(0x35), byteAt(0x36), byteAt(0x37)};
}
uint8_t Pokemon4Mutable::pokerus() const noexcept { return byteAt(0x82); }
uint8_t Pokemon4Mutable::ball() const noexcept {
    return sourceGroup_ == Enums::GameVersion::HGSS ? byteAt(0x86) : byteAt(0x83);
}
uint8_t Pokemon4Mutable::metLevel() const noexcept {
    return static_cast<uint8_t>(byteAt(0x84) & 0x7Fu);
}
uint8_t Pokemon4Mutable::form() const noexcept {
    return static_cast<uint8_t>(byteAt(0x40) >> 3);
}
uint8_t Pokemon4Mutable::gender() const noexcept {
    return static_cast<uint8_t>((byteAt(0x40) >> 1) & 3u);
}
uint8_t Pokemon4Mutable::nature() const noexcept {
    return static_cast<uint8_t>(pid() % 25u);
}
bool Pokemon4Mutable::shiny() const noexcept {
    const uint16_t psv = static_cast<uint16_t>((pid() & 0xFFFFu) ^ (pid() >> 16));
    return static_cast<uint16_t>(tid() ^ sid() ^ psv) < 8u;
}

uint8_t Pokemon4Mutable::abilitySlot() const noexcept {
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (personal.ability2 == 0 || personal.ability2 == personal.ability1) return 0;
    return static_cast<uint8_t>(pid() & 1u);
}

uint16_t Pokemon4Mutable::abilityForSlot(uint8_t slot) const noexcept {
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (slot == 0) return personal.ability1;
    if (slot == 1 && personal.ability2 != 0) return personal.ability2;
    return 0;
}

uint8_t Pokemon4Mutable::genderForPid(uint32_t value) const noexcept {
    const uint8_t ratio = personalFor(sourceGroup_, species(), form()).genderRatio;
    const uint8_t fixed = fixedGender(ratio);
    if (fixed != 3) return fixed;
    return (value & 0xFFu) < ratio ? 1 : 0;
}

int Pokemon4Mutable::constrainedAbilityBit() const noexcept {
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (personal.ability2 == 0 || personal.ability2 == personal.ability1) return -1;
    return static_cast<int>(pid() & 1u);
}

uint16_t Pokemon4Mutable::calculatedStat(size_t stat) const noexcept {
    if (!valid_ || stat >= 6) return 0;
    // Shedinja is the Gen III+ hard exception to the normal HP formula: its maximum HP is
    // always exactly 1 regardless of level, IV or EV. Party edits must preserve that native rule.
    if (stat == 0 && species() == 292) return 1;
    const auto& personal = personalFor(sourceGroup_, species(), form());
    const std::array<uint8_t,6> base{{
        personal.hp, personal.atk, personal.def,
        personal.spe, personal.spa, personal.spd
    }};
    if (base[stat] == 0) return 0;
    const auto iv = ivs();
    const auto ev = evs();
    const uint32_t lv = level();
    const uint32_t common =
        ((2u * base[stat] + iv[stat] + ev[stat] / 4u) * lv) / 100u;
    if (stat == 0) return static_cast<uint16_t>(common + lv + 10u);

    uint32_t value = common + 5u;
    const uint8_t n = nature();
    const int up = n / 5;
    const int down = n % 5;
    const int natureIndex = static_cast<int>(stat) - 1;
    if (up != down) {
        if (natureIndex == up) value = value * 110u / 100u;
        else if (natureIndex == down) value = value * 90u / 100u;
    }
    return static_cast<uint16_t>(value);
}

void Pokemon4Mutable::refreshPartyDerivedData() noexcept {
    if (!isParty() || !valid_) return;
    const uint16_t oldCurrentHp = u16At(0x8E);
    write8(0x8C, level());
    const uint16_t newMaxHp = calculatedStat(0);
    write16(0x90, newMaxHp);
    for (size_t stat = 1; stat < 6; ++stat)
        write16(0x90 + stat * 2, calculatedStat(stat));
    // Preserve fainted state and otherwise keep the current HP value, clamped to
    // the recalculated maximum. Ordinary edits must not unexpectedly heal a party member.
    write16(0x8E, oldCurrentHp == 0 ? 0 : std::min(oldCurrentHp, newMaxHp));
}

bool Pokemon4Mutable::writeTextPreservingTrash(
    size_t offset, size_t slotCount, size_t maxCharacters,
    const std::u16string& value) noexcept {
    if (!valid_ || offset + slotCount * 2 > decrypted_.size() || slotCount == 0)
        return false;
    const std::u16string bounded =
        value.substr(0, std::min(value.size(), maxCharacters));
    const auto encoded =
        Utils::encodeGen4Field(bounded, slotCount, maxCharacters, language());
    if (encoded.size() != slotCount * 2) return false;
    // The codec deliberately substitutes '?' for unsupported glyphs. Editing should
    // refuse an unrepresentable keyboard result rather than silently mangling it.
    if (Utils::decodeGen4Field(encoded) != bounded) return false;

    size_t copyBytes = encoded.size();
    for (size_t i = 0; i + 1 < encoded.size(); i += 2) {
        const uint16_t code = static_cast<uint16_t>(
            static_cast<uint8_t>(encoded[i]) |
            (static_cast<uint16_t>(static_cast<uint8_t>(encoded[i + 1])) << 8));
        if (code == Utils::GEN4_TERMINATOR) {
            copyBytes = i + 2;
            break;
        }
    }
    std::copy_n(encoded.begin(), static_cast<std::ptrdiff_t>(copyBytes),
                decrypted_.begin() + static_cast<std::ptrdiff_t>(offset));
    return true;
}

bool Pokemon4Mutable::setSpecies(uint16_t value) noexcept {
    if (!valid_ || value == 0 || value > 493) return false;
    if (value == species()) return true;
    const auto nextPersonal = personalFor(sourceGroup_, value, 0);
    if (nextPersonal.hp == 0) return false;

    const auto backup = decrypted_;
    const uint8_t oldLevel = level();
    const uint8_t oldNature = nature();
    const uint8_t oldGender = gender();
    const bool oldShiny = shiny();
    const bool oldWasNicknamed = (u32At(0x38) & 0x80000000u) != 0;
    const int oldAbilityBit = constrainedAbilityBit();

    write16(0x08, value);
    // Changing species resets to the base form; explicit form editing is a separate exact field.
    write8(0x40, static_cast<uint8_t>(byteAt(0x40) & 0x07u));
    write32(0x10, getExpForLevel(oldLevel, nextPersonal.growthRate));

    uint8_t desiredGender = oldGender;
    if (nextPersonal.genderRatio == 255) desiredGender = 2;
    else if (nextPersonal.genderRatio == 254) desiredGender = 1;
    else if (nextPersonal.genderRatio == 0) desiredGender = 0;
    else if (desiredGender > 1) desiredGender = genderForPid(pid());

    const bool dual = nextPersonal.ability2 != 0 && nextPersonal.ability2 != nextPersonal.ability1;
    const int desiredAbilityBit = dual ? (oldAbilityBit >= 0 ? oldAbilityBit : 0) : -1;
    if (!rerollPid(oldShiny ? 1 : 0, desiredGender, oldNature, desiredAbilityBit)) {
        decrypted_ = backup;
        return false;
    }
    // Gen IV stores the presentation gender alongside the PID-linked value. Keep both coherent,
    // especially when changing into a fixed-gender or genderless species.
    write8(0x40, static_cast<uint8_t>((byteAt(0x40) & ~0x06u) | ((desiredGender & 3u) << 1)));
    const uint8_t slot = dual ? static_cast<uint8_t>(pid() & 1u) : 0;
    write8(0x15, static_cast<uint8_t>(slot ? nextPersonal.ability2 : nextPersonal.ability1));

    if (!oldWasNicknamed) {
        const auto nameIndex = Names::languageIndexFor(
            static_cast<Enums::LanguageID>(language()));
        const auto speciesName = Utils::utf8ToUtf16(
            Names::getSpeciesNameLocalized(value, nameIndex));
        const auto encoded = Utils::encodeGen4Field(speciesName, 11, 10, language());
        const auto bounded = speciesName.substr(0, std::min<size_t>(10, speciesName.size()));
        if (Utils::decodeGen4Field(encoded) != bounded) {
            decrypted_ = backup;
            return false;
        }
        std::copy(encoded.begin(), encoded.end(), decrypted_.begin() + 0x48);
        write32(0x38, u32At(0x38) & ~0x80000000u);
    }
    refreshPartyDerivedData();
    return true;
}

bool Pokemon4Mutable::setOriginalTrainerName(const std::u16string& value) noexcept {
    if (!valid_ || value.size() > 7) return false;
    return writeTextPreservingTrash(0x68, 8, 7, value);
}

bool Pokemon4Mutable::setTID(uint16_t value) noexcept {
    if (!valid_) return false;
    const auto backup = decrypted_;
    const bool wasShiny = shiny();
    const uint8_t keepGender = gender();
    const uint8_t keepNature = nature();
    const int keepAbility = constrainedAbilityBit();
    write16(0x0C, value);
    if (!rerollPid(wasShiny ? 1 : 0, keepGender, keepNature, keepAbility)) {
        decrypted_ = backup;
        return false;
    }
    return true;
}

bool Pokemon4Mutable::setSID(uint16_t value) noexcept {
    if (!valid_) return false;
    const auto backup = decrypted_;
    const bool wasShiny = shiny();
    const uint8_t keepGender = gender();
    const uint8_t keepNature = nature();
    const int keepAbility = constrainedAbilityBit();
    write16(0x0E, value);
    if (!rerollPid(wasShiny ? 1 : 0, keepGender, keepNature, keepAbility)) {
        decrypted_ = backup;
        return false;
    }
    return true;
}

bool Pokemon4Mutable::setOriginalTrainerGender(uint8_t value) noexcept {
    if (!valid_ || value > 1) return false;
    write8(0x84, static_cast<uint8_t>((byteAt(0x84) & 0x7Fu) | (value << 7)));
    return true;
}

bool Pokemon4Mutable::setNickname(const std::u16string& value) noexcept {
    if (!writeTextPreservingTrash(0x48, 11, 10, value)) return false;
    // A user-initiated nickname edit is a nickname even when it happens to spell
    // the species name. Preserve the rest of IV32 and set only the native flag.
    write32(0x38, u32At(0x38) | 0x80000000u);
    return true;
}

bool Pokemon4Mutable::setLevel(uint8_t level) noexcept {
    if (!valid_ || level < 1 || level > 100) return false;
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (personal.hp == 0) return false;
    return setExperience(getExpForLevel(level, personal.growthRate));
}

bool Pokemon4Mutable::setExperience(uint32_t value) noexcept {
    if (!valid_) return false;
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (personal.hp == 0) return false;
    const uint32_t maximum = getExpForLevel(100, personal.growthRate);
    if (value > maximum) return false;
    write32(0x10, value);
    refreshPartyDerivedData();
    return true;
}

bool Pokemon4Mutable::setFriendship(uint8_t value) noexcept {
    if (!valid_) return false;
    write8(0x14, value);
    return true;
}

bool Pokemon4Mutable::setHeldItem(uint16_t value) noexcept {
    if (!valid_) return false;
    write16(0x0A, value);
    return true;
}

bool Pokemon4Mutable::setLanguage(uint8_t value) noexcept {
    if (!valid_ || !Enums::groupHasLanguage(sourceGroup_, value)) return false;
    if (value == language()) return true;
    const auto backup = decrypted_;
    const auto keepNickname = nickname();
    Pokemon4ReadOnly current(encryptedBytes(), sourceGroup_);
    if (!current.valid()) return false;
    const auto keepOt = current.originalTrainerName();
    write8(0x17, value);
    if (!writeTextPreservingTrash(0x48, 11, 10, keepNickname) ||
        !writeTextPreservingTrash(0x68, 8, 7, keepOt)) {
        decrypted_ = backup;
        return false;
    }
    return true;
}

bool Pokemon4Mutable::setIV(size_t stat, uint8_t value) noexcept {
    if (!valid_ || stat >= 6 || value > 31) return false;
    uint32_t packed = u32At(0x38);
    const uint32_t shift = static_cast<uint32_t>(stat * 5);
    packed &= ~(31u << shift);
    packed |= static_cast<uint32_t>(value) << shift;
    write32(0x38, packed);
    refreshPartyDerivedData();
    return true;
}

bool Pokemon4Mutable::setEV(size_t stat, uint8_t value) noexcept {
    if (!valid_ || stat >= 6) return false;
    write8(0x18 + stat, value);
    refreshPartyDerivedData();
    return true;
}

bool Pokemon4Mutable::setMove(size_t slot, uint16_t move) noexcept {
    if (!valid_ || slot >= 4) return false;
    write16(0x28 + slot * 2, move);
    return true;
}

bool Pokemon4Mutable::setPP(size_t slot, uint8_t pp) noexcept {
    if (!valid_ || slot >= 4) return false;
    write8(0x30 + slot, pp);
    return true;
}

bool Pokemon4Mutable::setPPUps(size_t slot, uint8_t ppUps) noexcept {
    if (!valid_ || slot >= 4 || ppUps > 3) return false;
    write8(0x34 + slot, ppUps);
    return true;
}

bool Pokemon4Mutable::setPokerus(uint8_t value) noexcept {
    if (!valid_) return false;
    write8(0x82, value);
    return true;
}

bool Pokemon4Mutable::setBall(uint8_t value) noexcept {
    if (!valid_) return false;
    if (sourceGroup_ == Enums::GameVersion::HGSS) write8(0x86, value);
    else if (sourceGroup_ == Enums::GameVersion::DP || sourceGroup_ == Enums::GameVersion::PT)
        write8(0x83, value);
    else return false;
    return true;
}

bool Pokemon4Mutable::setMetLevel(uint8_t value) noexcept {
    if (!valid_ || value > 100) return false;
    write8(0x84, static_cast<uint8_t>((byteAt(0x84) & 0x80u) | value));
    return true;
}

bool Pokemon4Mutable::rerollPid(
    int wantShiny, int wantGender, int wantNature, int wantAbilityBit) noexcept {
    if (!valid_) return false;

    const uint32_t original = pid();
    const uint8_t ratio = personalFor(sourceGroup_, species(), form()).genderRatio;
    if (wantGender >= 0) {
        const uint8_t fixed = fixedGender(ratio);
        if (fixed != 3 && wantGender != fixed) return false;
        if (fixed == 3 && wantGender == 2) return false;
    }

    uint32_t walk = original;
    for (int i = 0; i < 8000000; ++i) {
        walk = walk * 0x41C64E6Du + 0x00006073u;
        const uint32_t candidate = walk;
        if (wantAbilityBit >= 0 &&
            static_cast<int>(candidate & 1u) != wantAbilityBit)
            continue;
        if (wantNature >= 0 &&
            static_cast<int>(candidate % 25u) != wantNature)
            continue;
        if (wantGender >= 0 &&
            static_cast<int>(genderForPid(candidate)) != wantGender)
            continue;
        if (wantShiny >= 0) {
            const uint16_t psv = static_cast<uint16_t>(
                (candidate & 0xFFFFu) ^ (candidate >> 16));
            const bool isShiny = static_cast<uint16_t>(tid() ^ sid() ^ psv) < 8u;
            if (isShiny != (wantShiny != 0)) continue;
        }
        write32(0x00, candidate);
        return true;
    }
    return false;
}

bool Pokemon4Mutable::setNature(uint8_t value) noexcept {
    if (value >= 25) return false;
    if (value == nature()) return true;
    if (!rerollPid(shiny() ? 1 : 0, gender(), value, constrainedAbilityBit()))
        return false;
    refreshPartyDerivedData();
    return true;
}

bool Pokemon4Mutable::setGender(uint8_t value) noexcept {
    if (value > 2) return false;
    if (value == gender()) return true;
    const uint32_t oldPid = pid();
    const uint8_t oldPacked = byteAt(0x40);
    if (!rerollPid(shiny() ? 1 : 0, value, nature(), constrainedAbilityBit()))
        return false;
    write8(0x40, static_cast<uint8_t>((oldPacked & ~0x06u) | ((value & 3u) << 1)));
    if (genderForPid(pid()) != value) {
        write32(0x00, oldPid);
        write8(0x40, oldPacked);
        return false;
    }
    return true;
}

bool Pokemon4Mutable::setShiny(bool value) noexcept {
    if (value == shiny()) return true;
    return rerollPid(value ? 1 : 0, gender(), nature(), constrainedAbilityBit());
}

bool Pokemon4Mutable::setAbilitySlot(uint8_t slot) noexcept {
    if (slot > 1) return false;
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (personal.hp == 0) return false;

    const bool single = personal.ability2 == 0 || personal.ability2 == personal.ability1;
    if (single) {
        if (slot != 0) return false;
        if (ability() == personal.ability1) return true;
        write8(0x15, static_cast<uint8_t>(personal.ability1));
        return true;
    }

    const uint16_t requestedAbility = slot == 0 ? personal.ability1 : personal.ability2;
    if (slot == abilitySlot() && ability() == requestedAbility) return true;

    const uint32_t oldPid = pid();
    const uint8_t oldAbility = byteAt(0x15);
    if (!rerollPid(shiny() ? 1 : 0, gender(), nature(), slot))
        return false;
    write8(0x15, static_cast<uint8_t>(slot == 0 ? personal.ability1 : personal.ability2));
    if (abilitySlot() != slot) {
        write32(0x00, oldPid);
        write8(0x15, oldAbility);
        return false;
    }
    return true;
}

bool Pokemon4Mutable::setAbility(uint16_t abilityId) noexcept {
    const auto& personal = personalFor(sourceGroup_, species(), form());
    if (abilityId == personal.ability1) return setAbilitySlot(0);
    if (personal.ability2 != 0 && abilityId == personal.ability2)
        return setAbilitySlot(1);
    return false;
}

} // namespace Pokemon
