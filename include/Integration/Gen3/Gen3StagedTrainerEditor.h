#pragma once

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen3 {

// R2-BUG-002: the Gen III Trainer page's Name / Money editor for Ruby, Sapphire, Emerald, FireRed
// and LeafGreen. Header-only so every target that links the Gen III export picks it up.
//
// It records the two target values and applies them ON TOP of the finalized export image
// (applyTo), after the Pokemon and Bag workspaces are merged. Money shares logical section 1 with
// the Bag, so a separate staged copy could not be merged sector-disjointly; applying the fields
// last keeps every workspace independent. The source save is never touched.
//
// Layout (pret pokeruby / pokeemerald / pokefirered SaveBlock2 / SaveBlock1, as already read by
// FRLGReadModel and Gen3StagedInventoryEditor):
//   - name: section 0 +0x00, 8 bytes (PLAYER_NAME_LENGTH 7 + EOS 0xFF), Gen III charset.
//     Written like the naming screen: the characters, then EOS 0xFF to the end of the 8 bytes.
//   - money: section 1 +0x490 (Ruby / Sapphire / Emerald) or +0x290 (FireRed / LeafGreen), u32.
//     Emerald and FireRed / LeafGreen store it XOR the save's u32 encryption key
//     (section 0 +0xAC Emerald, +0xF20 FireRed / LeafGreen); Ruby / Sapphire have no key.
//     Cap: MAX_MONEY 999999.
//   - the checksum of every changed section is recomputed (sectorChecksum at +0xFF6).
//   - P3 (Round 5) gender: section 0 +0x08 (u8, 0 male / 1 female) and play time: section 0
//     +0x0E u16 hours, +0x10 minutes, +0x11 seconds (pinned PKSM-Core aa22d7a Sav3::gender /
//     playedHours / playedMinutes / playedSeconds; pret SaveBlock2 playerGender / playTime*).
//     The frame counter (+0x12) is never touched. Gender is offered only when the stored byte is a
//     native 0 / 1; hours are capped at 999 (pret PlayTimeCounter maximum 999:59:59).
// Language: a Gen III save has no language field. The repo's (PKHeX / PKSM) rule is the name
// shape: bytes 6-7 both zero is a Japanese 5-character save. Name editing is offered only for an
// unambiguous international save (bytes 6-7 not both zero); a Japanese save keeps the name
// read-only (money still stages).
class Gen3StagedTrainerEditor {
public:
    static constexpr uint32_t kMaxMoney = 999999;
    static constexpr int kNameMax = 7;
    static constexpr std::size_t kNameBytes = 8;
    static constexpr uint16_t kMaxPlayedHours = 999;
    static constexpr std::size_t kGenderOffset = 0x08;
    static constexpr std::size_t kPlayTimeOffset = 0x0E;

    struct PlayTime {
        uint16_t hours = 0;
        uint8_t minutes = 0;
        uint8_t seconds = 0;
        friend bool operator==(const PlayTime&, const PlayTime&) = default;
    };

    struct Change {
        std::string label;
        std::string before;
        std::string after;
    };

    static std::unique_ptr<Gen3StagedTrainerEditor> create(std::span<const uint8_t> source, SourceGame game,
                                                           std::string& error) {
        error.clear();
        if (source.size() != Detail::kSaveSize || !Detail::sourceGameSupported(game)) {
            error = "Generation III Trainer editing needs an exact 128 KiB Gen III save";
            return nullptr;
        }
        const Detail::SlotValidation slots[2] = {Detail::validateSlot(source, 0), Detail::validateSlot(source, 1)};
        if (!slots[0].valid && !slots[1].valid) {
            error = "No checksum-valid Generation III save slot is available for Trainer editing";
            return nullptr;
        }
        const uint8_t active = Detail::selectActiveSlot(slots);
        if (!Detail::familyMatches(game, Detail::detectFamily(source, slots[active]))) {
            error = "Generation III save family does not match the selected exact game";
            return nullptr;
        }
        auto editor = std::unique_ptr<Gen3StagedTrainerEditor>(new Gen3StagedTrainerEditor());
        editor->game_ = game;
        editor->active_ = active;
        editor->sectors_ = slots[active].logicalSectorOffsets;
        editor->original_.assign(source.begin(), source.end());
        if (!editor->read(editor->original_, editor->originalName_, editor->originalMoney_) ||
            editor->originalMoney_ > kMaxMoney) {
            error = "Generation III trainer Name / Money could not be read";
            return nullptr;
        }
        editor->name_ = editor->originalName_;
        editor->money_ = editor->originalMoney_;
        editor->originalGender_ = editor->gender_ = editor->readGender(editor->original_);
        editor->originalPlayTime_ = editor->playTime_ = editor->readPlayTime(editor->original_);
        return editor;
    }

    [[nodiscard]] SourceGame sourceGame() const noexcept { return game_; }
    [[nodiscard]] const std::vector<uint8_t>& originalBytes() const noexcept { return original_; }
    [[nodiscard]] std::string trainerName() const { return Utils::utf16ToUtf8(name_); }
    [[nodiscard]] uint32_t money() const noexcept { return money_; }
    [[nodiscard]] bool hasPendingChanges() const noexcept {
        return name_ != originalName_ || money_ != originalMoney_ || gender_ != originalGender_ ||
               playTime_ != originalPlayTime_;
    }
    [[nodiscard]] uint8_t gender() const noexcept { return gender_; }
    [[nodiscard]] bool genderEditable() const noexcept { return originalGender_ <= 1; }
    [[nodiscard]] PlayTime playTime() const noexcept { return playTime_; }
    // A stored clock with minutes / seconds above 59 is not a native value; it stays read-only.
    [[nodiscard]] bool playTimeEditable() const noexcept {
        return originalPlayTime_.minutes <= 59 && originalPlayTime_.seconds <= 59;
    }
    [[nodiscard]] bool nameEditable() const noexcept {
        const std::size_t name = sectors_[0];
        return original_[name + 6] != 0 || original_[name + 7] != 0;
    }

    [[nodiscard]] std::vector<Change> pendingChanges() const {
        std::vector<Change> out;
        if (name_ != originalName_)
            out.push_back({"Trainer name", Utils::utf16ToUtf8(originalName_), Utils::utf16ToUtf8(name_)});
        if (money_ != originalMoney_)
            out.push_back({"Money", std::to_string(originalMoney_), std::to_string(money_)});
        if (gender_ != originalGender_)
            out.push_back({"Trainer gender", genderText(originalGender_), genderText(gender_)});
        if (playTime_ != originalPlayTime_)
            out.push_back({"Play time", playTimeText(originalPlayTime_), playTimeText(playTime_)});
        return out;
    }

    bool stageTrainerName(const std::string& utf8, std::string& error) {
        error.clear();
        if (!nameEditable()) {
            error = "Japanese Gen III trainer names are read-only here";
            return false;
        }
        const std::u16string name = Utils::utf8ToUtf16(utf8);
        if (name.empty() || name.size() > static_cast<std::size_t>(kNameMax)) {
            error = "Gen III trainer name must be 1-7 characters";
            return false;
        }
        for (char16_t character : name) {
            if (Utils::charToGen3(character, kInternational) == Utils::GEN3_TERMINATOR) {
                error = "Gen III trainer name has a character the game cannot store";
                return false;
            }
        }
        // Prove the value round-trips through the real encoder / decoder before accepting it.
        auto probe = original_;
        std::string ignored;
        if (!write(probe, &name, nullptr, ignored)) {
            error = ignored;
            return false;
        }
        std::u16string decoded;
        uint32_t money = 0;
        if (!read(probe, decoded, money) || decoded != name) {
            error = "Gen III trainer name does not round-trip in this game's charset";
            return false;
        }
        name_ = name;
        return true;
    }

    bool stageMoney(uint32_t money, std::string& error) {
        error.clear();
        if (money > kMaxMoney) {
            error = "Gen III money must be 0-999999";
            return false;
        }
        money_ = money;
        return true;
    }

    bool stageGender(uint8_t gender, std::string& error) {
        error.clear();
        if (!genderEditable() || gender > 1) {
            error = "This save's trainer gender byte is not a native Male / Female value";
            return false;
        }
        gender_ = gender;
        return true;
    }

    bool stagePlayTime(PlayTime time, std::string& error) {
        error.clear();
        if (!playTimeEditable()) {
            error = "This save's play time clock is not a native value; it stays read-only";
            return false;
        }
        if (time.hours > kMaxPlayedHours || time.minutes > 59 || time.seconds > 59) {
            error = "Gen III play time must be 0:00:00-999:59:59";
            return false;
        }
        playTime_ = time;
        return true;
    }

    // Stages only the fields that differ from what is shown.
    bool stage(const std::string& name, uint32_t money, std::string& error) {
        if (name != trainerName() && !stageTrainerName(name, error)) return false;
        if (money != money_ && !stageMoney(money, error)) return false;
        return true;
    }

    void discard() noexcept {
        name_ = originalName_;
        money_ = originalMoney_;
        gender_ = originalGender_;
        playTime_ = originalPlayTime_;
    }

    // Writes the pending fields into `bytes` (a finalized image of the SAME source: same active slot
    // and sector rotation), repairs the touched sections' checksums and checks the values read back.
    // `bytes` is left unchanged on failure.
    bool applyTo(std::vector<uint8_t>& bytes, std::string& error) const {
        error.clear();
        if (!hasPendingChanges()) return true;
        if (bytes.size() != original_.size()) {
            error = "Gen III Trainer edit cannot be applied to a different-size image";
            return false;
        }
        const Detail::SlotValidation slots[2] = {Detail::validateSlot(bytes, 0), Detail::validateSlot(bytes, 1)};
        if ((!slots[0].valid && !slots[1].valid) || Detail::selectActiveSlot(slots) != active_ ||
            slots[active_].logicalSectorOffsets != sectors_) {
            error = "Gen III Trainer edit: the staged image no longer has the source's active save slot";
            return false;
        }
        auto candidate = bytes;
        const std::u16string* name = name_ != originalName_ ? &name_ : nullptr;
        const uint32_t* money = money_ != originalMoney_ ? &money_ : nullptr;
        if (!write(candidate, name, money, error)) return false;
        if (gender_ != originalGender_ || playTime_ != originalPlayTime_) {
            const std::size_t block = sectors_[0];
            candidate[block + kGenderOffset] = gender_;
            candidate[block + kPlayTimeOffset] = static_cast<uint8_t>(playTime_.hours);
            candidate[block + kPlayTimeOffset + 1] = static_cast<uint8_t>(playTime_.hours >> 8);
            candidate[block + kPlayTimeOffset + 2] = playTime_.minutes;
            candidate[block + kPlayTimeOffset + 3] = playTime_.seconds;
            repair(candidate, 0);
        }
        std::u16string readName;
        uint32_t readMoney = 0;
        if (!read(candidate, readName, readMoney) || readName != name_ || readMoney != money_ ||
            readGender(candidate) != gender_ || readPlayTime(candidate) != playTime_) {
            error = "Gen III Trainer edit did not read back as entered";
            return false;
        }
        const Detail::SlotValidation after = Detail::validateSlot(candidate, active_);
        if (!after.valid) {
            error = "Gen III Trainer edit broke the active save slot's checksums";
            return false;
        }
        bytes = std::move(candidate);
        return true;
    }

private:
    // Trainer names on an international save are encoded with the shared international table
    // (the repo's English language id), exactly as FRLGReadModel decodes them.
    static constexpr uint8_t kInternational = 2;

    Gen3StagedTrainerEditor() = default;

    [[nodiscard]] std::size_t moneyOffset() const noexcept { return Detail::isFRLG(game_) ? 0x290 : 0x490; }
    [[nodiscard]] uint32_t key(std::span<const uint8_t> bytes) const noexcept {
        if (game_ == SourceGame::EmeraldGBA) return Detail::read32(bytes, sectors_[0] + 0xAC);
        if (Detail::isFRLG(game_)) return Detail::read32(bytes, sectors_[0] + 0xF20);
        return 0;
    }
    [[nodiscard]] uint8_t language() const noexcept { return nameEditable() ? kInternational : 1; }
    [[nodiscard]] uint8_t readGender(std::span<const uint8_t> bytes) const noexcept {
        return bytes[sectors_[0] + kGenderOffset];
    }
    [[nodiscard]] PlayTime readPlayTime(std::span<const uint8_t> bytes) const noexcept {
        const std::size_t at = sectors_[0] + kPlayTimeOffset;
        return {static_cast<uint16_t>(bytes[at] | (bytes[at + 1] << 8)), bytes[at + 2], bytes[at + 3]};
    }
    static std::string genderText(uint8_t gender) { return gender == 0 ? "Male" : gender == 1 ? "Female" : "Unknown"; }
    static std::string playTimeText(PlayTime time) {
        const auto two = [](unsigned v) { return (v < 10 ? "0" : "") + std::to_string(v); };
        return std::to_string(time.hours) + ":" + two(time.minutes) + ":" + two(time.seconds);
    }

    bool read(std::span<const uint8_t> bytes, std::u16string& name, uint32_t& money) const {
        name.clear();
        const std::size_t base = sectors_[0];
        const std::size_t limit = nameEditable() ? kNameBytes - 1 : 5;
        for (std::size_t i = 0; i < limit; ++i) {
            const uint8_t value = bytes[base + i];
            if (value == Utils::GEN3_TERMINATOR) break;
            const char16_t character = Utils::gen3ToChar(value, language());
            if (!character) return false;
            name.push_back(character);
        }
        money = Detail::read32(bytes, sectors_[1] + moneyOffset()) ^ key(bytes);
        return true;
    }

    bool write(std::vector<uint8_t>& bytes, const std::u16string* name, const uint32_t* money,
               std::string& error) const {
        if (name) {
            std::array<uint8_t, kNameBytes> field{};
            field.fill(Utils::GEN3_TERMINATOR);
            for (std::size_t i = 0; i < name->size(); ++i) {
                const uint8_t encoded = Utils::charToGen3((*name)[i], kInternational);
                if (encoded == Utils::GEN3_TERMINATOR) {
                    error = "Gen III trainer name has a character the game cannot store";
                    return false;
                }
                field[i] = encoded;
            }
            std::copy(field.begin(), field.end(), bytes.begin() + static_cast<std::ptrdiff_t>(sectors_[0]));
            repair(bytes, 0);
        }
        if (money) {
            const uint32_t stored = *money ^ key(bytes);
            const std::size_t at = sectors_[1] + moneyOffset();
            for (int i = 0; i < 4; ++i) bytes[at + static_cast<std::size_t>(i)] = static_cast<uint8_t>(stored >> (8 * i));
            repair(bytes, 1);
        }
        return true;
    }

    void repair(std::vector<uint8_t>& bytes, std::size_t id) const {
        const std::size_t sector = sectors_[id];
        const uint16_t checksum = Detail::sectorChecksum(
            std::span<const uint8_t>(bytes).subspan(sector, Detail::kChunkLengths[id]));
        bytes[sector + 0xFF6] = static_cast<uint8_t>(checksum);
        bytes[sector + 0xFF7] = static_cast<uint8_t>(checksum >> 8);
    }

    SourceGame game_ = SourceGame::RubyGBA;
    uint8_t active_ = 0;
    std::array<std::size_t, 14> sectors_{};
    std::vector<uint8_t> original_;
    std::u16string originalName_;
    std::u16string name_;
    uint32_t originalMoney_ = 0;
    uint32_t money_ = 0;
    uint8_t originalGender_ = 0;
    uint8_t gender_ = 0;
    PlayTime originalPlayTime_{};
    PlayTime playTime_{};
};

} // namespace PokeVault::Integration::Gen3
