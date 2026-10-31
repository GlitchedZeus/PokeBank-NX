#ifndef POKEBANK_GEN5_TRAINER_H
#define POKEBANK_GEN5_TRAINER_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Native Black/White/Black 2/White 2 trainer data. Every constant below is
// cited from pinned PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722:
//   Saves/Access/SaveBlockAccessor5BW.cs / 5B2W2.cs:
//     PlayerData = Block(27) (BW 0x19400 len 0x68, B2W2 0x19400 len 0xB0)
//     Misc       = Block(52) (BW 0x21200 len 0xEC, B2W2 0x21100 len 0xF0)
//   Saves/Substructures/Gen5/PlayerData5.cs: OT trash 0x04 (0x10 bytes),
//     TID16 0x14, SID16 0x16, Language 0x1E, Game 0x1F, Gender 0x21,
//     PlayedHours u16 0x24, PlayedMinutes 0x26, PlayedSeconds 0x27.
//   Saves/Substructures/Gen5/Misc5.cs: Money u32 0x00, Badges byte 0x04.
//   Saves/SAV5.cs: MaxStringLengthTrainer 7; Saves/SaveFile.cs: MaxMoney 9999999.
//   PKM/Strings/StringConverter5.cs: UTF-16LE, 0xFFFF terminator, ClearZero.
// Name, Money, Play Time and (owner Oct 28) Gender are staged. Gender is the PlayerData byte 0x21
// (0 male / 1 female); it is offered only when the stored byte is one of those two, the same rule
// as Gen III / IV. The game's ownership test does not use the player's gender (pret pokeemerald
// src/pokemon.c IsOtherTrainer compares OT ID and name only; Gen V has no decompilation, so this is
// Gen III evidence, recorded as such). TID/SID, language, game and badges stay read-only (they gate
// ownership and story flags).
// Both blocks' CRCs are recomputed by the verified export candidate only.
namespace TrainerLayout5 {
inline constexpr size_t PlayerBlockIndex = 27;
inline constexpr size_t MiscBlockIndex = 52;
inline constexpr size_t NameOffset = 0x04;
inline constexpr size_t NameBytes = 0x10;
inline constexpr size_t MaxNameChars = 7;
inline constexpr size_t TidOffset = 0x14;
inline constexpr size_t SidOffset = 0x16;
inline constexpr size_t LanguageOffset = 0x1E;
inline constexpr size_t GameOffset = 0x1F;
inline constexpr size_t GenderOffset = 0x21;
inline constexpr size_t HoursOffset = 0x24;
inline constexpr size_t MinutesOffset = 0x26;
inline constexpr size_t SecondsOffset = 0x27;
inline constexpr size_t MoneyOffset = 0x00;
inline constexpr size_t BadgesOffset = 0x04;
inline constexpr uint32_t MaxMoney = 9999999;
// App policy, not a format rule: the u16 hour field is wider, but the
// editor never stages more than the 999:59:59 clock the games display.
inline constexpr uint16_t MaxHours = 999;

static_assert(LayoutInfo::BlackWhite[PlayerBlockIndex].offset == 0x19400 &&
              LayoutInfo::BlackWhite[PlayerBlockIndex].length == 0x68);
static_assert(LayoutInfo::Black2White2[PlayerBlockIndex].offset == 0x19400 &&
              LayoutInfo::Black2White2[PlayerBlockIndex].length == 0xB0);
static_assert(LayoutInfo::BlackWhite[MiscBlockIndex].offset == 0x21200 &&
              LayoutInfo::BlackWhite[MiscBlockIndex].length == 0xEC);
static_assert(LayoutInfo::Black2White2[MiscBlockIndex].offset == 0x21100 &&
              LayoutInfo::Black2White2[MiscBlockIndex].length == 0xF0);
} // namespace TrainerLayout5

[[nodiscard]] constexpr const LayoutInfo::SaveBlock& trainerBlock5(SaveFamily family,
                                                                   size_t index) noexcept {
    return family == SaveFamily::BlackWhite ? LayoutInfo::BlackWhite[index]
                                            : LayoutInfo::Black2White2[index];
}

struct Trainer5View {
    std::u16string rawName;
    uint16_t tid = 0, sid = 0;
    uint8_t gender = 0, language = 0, game = 0;
    uint32_t money = 0;
    uint8_t badges = 0;
    uint16_t hours = 0;
    uint8_t minutes = 0, seconds = 0;
    [[nodiscard]] size_t badgeCount() const noexcept {
        size_t n = 0;
        for (size_t i = 0; i < 8; ++i) n += (badges >> i) & 1u;
        return n;
    }
};

namespace detail5trainer {
[[nodiscard]] inline uint16_t rd16(std::span<const uint8_t> b, size_t at) noexcept {
    return static_cast<uint16_t>(b[at] | (b[at + 1] << 8));
}
[[nodiscard]] inline uint32_t rd32(std::span<const uint8_t> b, size_t at) noexcept {
    return static_cast<uint32_t>(rd16(b, at)) | (static_cast<uint32_t>(rd16(b, at + 2)) << 16);
}
} // namespace detail5trainer

// Decode the two native blocks. Fails closed on wrong geometry or a name
// without a terminator inside the 0x10-byte field.
[[nodiscard]] inline std::optional<Trainer5View> decodeTrainer5(SaveFamily family,
                                                                std::span<const uint8_t> player,
                                                                std::span<const uint8_t> misc) {
    using namespace TrainerLayout5;
    using detail5trainer::rd16;
    using detail5trainer::rd32;
    if (player.size() != trainerBlock5(family, PlayerBlockIndex).length ||
        misc.size() != trainerBlock5(family, MiscBlockIndex).length)
        return std::nullopt;
    Trainer5View v;
    bool terminated = false;
    for (size_t i = 0; i < NameBytes / 2; ++i) {
        const uint16_t ch = rd16(player, NameOffset + i * 2);
        if (ch == 0xFFFF || ch == 0) { terminated = true; break; }
        if (i >= MaxNameChars) break;
        v.rawName.push_back(static_cast<char16_t>(ch));
    }
    if (!terminated) return std::nullopt;
    v.tid = rd16(player, TidOffset);
    v.sid = rd16(player, SidOffset);
    v.language = player[LanguageOffset];
    v.game = player[GameOffset];
    v.gender = player[GenderOffset];
    v.hours = rd16(player, HoursOffset);
    v.minutes = player[MinutesOffset];
    v.seconds = player[SecondsOffset];
    v.money = rd32(misc, MoneyOffset);
    v.badges = misc[BadgesOffset];
    return v;
}

[[nodiscard]] inline std::span<const uint8_t> sourceTrainerBlock5(const Gen5ReadOnlySave& save,
                                                                  size_t index) noexcept {
    const auto bytes = save.sourceBytes();
    const auto& block = trainerBlock5(save.family(), index);
    const size_t at = save.selectedCopyOffset() + block.offset;
    if (at > bytes.size() || block.length > bytes.size() - at) return {};
    return bytes.subspan(at, block.length);
}

[[nodiscard]] inline std::optional<Trainer5View> decodeTrainer5(const Gen5ReadOnlySave& save) {
    return decodeTrainer5(save.family(),
                          sourceTrainerBlock5(save, TrainerLayout5::PlayerBlockIndex),
                          sourceTrainerBlock5(save, TrainerLayout5::MiscBlockIndex));
}

// Name input accepted for staging: 1..7 printable ASCII characters. ASCII is
// stored verbatim as UTF-16 by StringConverter5 (no glyph remapping).
[[nodiscard]] constexpr bool trainerNameAccepted5(std::string_view name) noexcept {
    if (name.empty() || name.size() > TrainerLayout5::MaxNameChars) return false;
    if (name.front() == ' ' || name.back() == ' ') return false;
    for (const char c : name)
        if (c < 0x20 || c > 0x7E) return false;
    return true;
}

struct TrainerChange5 {
    enum class Field : uint8_t { Name, Money, PlayTime, Gender };
    Field field = Field::Money;
    std::string before;
    std::string after;
};

[[nodiscard]] inline std::string asciiTrainerName5(const std::u16string& raw) {
    std::string out;
    for (const char16_t c : raw) out.push_back(c >= 0x20 && c <= 0x7E ? static_cast<char>(c) : '?');
    return out;
}
[[nodiscard]] inline std::string playTime5(uint16_t h, uint8_t m, uint8_t s) {
    const auto two = [](unsigned v) { return std::string(v < 10 ? "0" : "") + std::to_string(v); };
    return std::to_string(h) + ":" + two(m) + ":" + two(s);
}

// App-owned copies of PlayerData (27) and Misc (52). Each setter writes only
// its cited byte range, re-decodes, and audits that no other byte moved.
class Gen5StagedTrainer {
public:
    static std::optional<Gen5StagedTrainer> create(const Gen5ReadOnlySave& save,
                                                   std::string* error = nullptr) {
        const auto player = sourceTrainerBlock5(save, TrainerLayout5::PlayerBlockIndex);
        const auto misc = sourceTrainerBlock5(save, TrainerLayout5::MiscBlockIndex);
        if (player.empty() || misc.empty() || !decodeTrainer5(save.family(), player, misc)) {
            if (error) *error = "Gen V trainer blocks are not fully recognized";
            return std::nullopt;
        }
        return Gen5StagedTrainer(save.family(), {player.begin(), player.end()},
                                 {misc.begin(), misc.end()});
    }

    [[nodiscard]] SaveFamily family() const noexcept { return family_; }
    [[nodiscard]] std::span<const uint8_t> originalPlayer() const noexcept { return originalPlayer_; }
    [[nodiscard]] std::span<const uint8_t> stagedPlayer() const noexcept { return player_; }
    [[nodiscard]] std::span<const uint8_t> originalMisc() const noexcept { return originalMisc_; }
    [[nodiscard]] std::span<const uint8_t> stagedMisc() const noexcept { return misc_; }
    [[nodiscard]] bool playerDirty() const noexcept { return player_ != originalPlayer_; }
    [[nodiscard]] bool miscDirty() const noexcept { return misc_ != originalMisc_; }
    [[nodiscard]] bool dirty() const noexcept { return playerDirty() || miscDirty(); }
    [[nodiscard]] Trainer5View current() const { return *decodeTrainer5(family_, player_, misc_); }
    [[nodiscard]] Trainer5View baseline() const {
        return *decodeTrainer5(family_, originalPlayer_, originalMisc_);
    }

    [[nodiscard]] bool setName(std::string_view name, std::string* error = nullptr) {
        using namespace TrainerLayout5;
        if (!trainerNameAccepted5(name))
            return fail(error, "Gen V trainer name must be 1-7 printable ASCII characters");
        auto next = player_;
        std::fill_n(next.begin() + NameOffset, NameBytes, uint8_t{0}); // ClearZero
        for (size_t i = 0; i < name.size(); ++i) {
            next[NameOffset + i * 2] = static_cast<uint8_t>(name[i]);
            next[NameOffset + i * 2 + 1] = 0;
        }
        next[NameOffset + name.size() * 2] = 0xFF;
        next[NameOffset + name.size() * 2 + 1] = 0xFF;
        return commitPlayer(std::move(next), NameOffset, NameBytes, error);
    }
    [[nodiscard]] bool setMoney(uint32_t money, std::string* error = nullptr) {
        using namespace TrainerLayout5;
        if (money > MaxMoney) return fail(error, "Gen V money must be 0..9,999,999");
        auto next = misc_;
        for (size_t i = 0; i < 4; ++i) next[MoneyOffset + i] = static_cast<uint8_t>(money >> (8 * i));
        return commitMisc(std::move(next), MoneyOffset, 4, error);
    }
    [[nodiscard]] bool setPlayTime(uint32_t hours, uint32_t minutes, uint32_t seconds,
                                   std::string* error = nullptr) {
        using namespace TrainerLayout5;
        if (hours > MaxHours || minutes > 59 || seconds > 59)
            return fail(error, "Gen V play time must be 0:00:00..999:59:59");
        auto next = player_;
        next[HoursOffset] = static_cast<uint8_t>(hours);
        next[HoursOffset + 1] = static_cast<uint8_t>(hours >> 8);
        next[MinutesOffset] = static_cast<uint8_t>(minutes);
        next[SecondsOffset] = static_cast<uint8_t>(seconds);
        return commitPlayer(std::move(next), HoursOffset, 4, error);
    }

    // The stored byte decides whether Gender is offered: anything but 0 / 1 stays read-only.
    [[nodiscard]] bool genderEditable() const { return baseline().gender <= 1; }
    [[nodiscard]] bool setGender(uint8_t gender, std::string* error = nullptr) {
        using namespace TrainerLayout5;
        if (!genderEditable()) return fail(error, "Gen V trainer gender has an unrecognized stored value; read-only");
        if (gender > 1) return fail(error, "Gen V trainer gender must be Male or Female");
        auto next = player_;
        next[GenderOffset] = gender;
        return commitPlayer(std::move(next), GenderOffset, 1, error);
    }

    [[nodiscard]] std::vector<TrainerChange5> changes() const {
        std::vector<TrainerChange5> out;
        const auto a = baseline(), b = current();
        if (a.rawName != b.rawName)
            out.push_back({TrainerChange5::Field::Name, asciiTrainerName5(a.rawName),
                           asciiTrainerName5(b.rawName)});
        if (a.money != b.money)
            out.push_back({TrainerChange5::Field::Money, std::to_string(a.money), std::to_string(b.money)});
        if (a.hours != b.hours || a.minutes != b.minutes || a.seconds != b.seconds)
            out.push_back({TrainerChange5::Field::PlayTime, playTime5(a.hours, a.minutes, a.seconds),
                           playTime5(b.hours, b.minutes, b.seconds)});
        if (a.gender != b.gender)
            out.push_back({TrainerChange5::Field::Gender, a.gender == 0 ? "Male" : "Female",
                           b.gender == 0 ? "Male" : "Female"});
        return out;
    }

    // Whitelisted byte ranges within each block (for the export auditor).
    struct Range { size_t offset; size_t length; };
    static constexpr std::array<Range, 3> kPlayerRanges{{
        {TrainerLayout5::NameOffset, TrainerLayout5::NameBytes}, {TrainerLayout5::GenderOffset, 1},
        {TrainerLayout5::HoursOffset, 4}}};
    static constexpr std::array<Range, 1> kMiscRanges{{{TrainerLayout5::MoneyOffset, 4}}};

private:
    Gen5StagedTrainer(SaveFamily family, std::vector<uint8_t> player, std::vector<uint8_t> misc)
        : family_(family), originalPlayer_(player), player_(std::move(player)),
          originalMisc_(misc), misc_(std::move(misc)) {}
    static bool fail(std::string* error, const char* why) {
        if (error) *error = why;
        return false;
    }
    static bool onlyRange(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b,
                          size_t offset, size_t length) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if ((i < offset || i >= offset + length) && a[i] != b[i]) return false;
        return true;
    }
    bool commitPlayer(std::vector<uint8_t> next, size_t offset, size_t length, std::string* error) {
        if (!onlyRange(player_, next, offset, length) || !decodeTrainer5(family_, next, misc_))
            return fail(error, "Gen V trainer change failed exact native reparse");
        player_ = std::move(next);
        if (error) error->clear();
        return true;
    }
    bool commitMisc(std::vector<uint8_t> next, size_t offset, size_t length, std::string* error) {
        if (!onlyRange(misc_, next, offset, length) || !decodeTrainer5(family_, player_, next))
            return fail(error, "Gen V trainer change failed exact native reparse");
        misc_ = std::move(next);
        if (error) error->clear();
        return true;
    }

    SaveFamily family_;
    std::vector<uint8_t> originalPlayer_, player_;
    std::vector<uint8_t> originalMisc_, misc_;
};

} // namespace PokeVault::Integration::Gen5
#endif
