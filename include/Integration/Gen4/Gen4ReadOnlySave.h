#ifndef POKEBANK_INTEGRATION_GEN4_READ_ONLY_SAVE_H
#define POKEBANK_INTEGRATION_GEN4_READ_ONLY_SAVE_H

#include "Enums/GameVersion.h"
#include "Pokemon/Pokemon4ReadOnly.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen4 {

enum class Layout : uint8_t { DiamondPearl, Platinum, HeartGoldSoulSilver };
enum class AssignmentStatus : uint8_t { Unspecified, Match, Mismatch };

struct BlockSelection {
    size_t offset = 0;
    uint8_t partition = 0;
    bool recoveredOlderCopy = false;
    uint32_t majorCounter = 0;
    uint32_t minorCounter = 0;
};

struct TrainerReadOnly {
    std::u16string name;
    uint16_t tid = 0;
    uint16_t sid = 0;
    uint32_t money = 0;
    uint8_t gender = 0;
    uint8_t language = 0;
    uint8_t badges = 0;
    uint8_t romCode = 0;
    uint16_t playedHours = 0;
    uint8_t playedMinutes = 0;
    uint8_t playedSeconds = 0;
};

class Gen4ReadOnlySave {
public:
    static std::optional<Gen4ReadOnlySave> parse(std::span<const uint8_t> source,
                                                 Layout layout,
                                                 std::string_view assignedGameId = {},
                                                 std::string* error = nullptr);

    [[nodiscard]] Layout layout() const noexcept { return layout_; }
    [[nodiscard]] Enums::GameVersion rawFamily() const noexcept { return rawFamily_; }
    [[nodiscard]] Enums::GameVersion exactGameFromSave() const noexcept { return exactGameFromSave_; }
    [[nodiscard]] Enums::GameVersion assignedExactGame() const noexcept { return assignedExactGame_; }
    [[nodiscard]] AssignmentStatus assignmentStatus() const noexcept { return assignmentStatus_; }

    [[nodiscard]] const BlockSelection& generalSelection() const noexcept { return general_; }
    [[nodiscard]] const BlockSelection& storageSelection() const noexcept { return storage_; }
    [[nodiscard]] bool recovered() const noexcept {
        return general_.recoveredOlderCopy || storage_.recoveredOlderCopy;
    }

    [[nodiscard]] const TrainerReadOnly& trainer() const noexcept { return trainer_; }
    [[nodiscard]] std::span<const Pokemon::Pokemon4ReadOnly> party() const noexcept { return std::span<const Pokemon::Pokemon4ReadOnly>(party_).first(partyCount_); }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] std::span<const Pokemon::Pokemon4ReadOnly> nativePartySlots() const noexcept { return party_; }
    [[nodiscard]] const Pokemon::Pokemon4ReadOnly& box(size_t boxIndex, size_t slotIndex) const;
    [[nodiscard]] uint8_t currentBox() const noexcept { return currentBox_; }
    [[nodiscard]] std::span<const std::u16string> boxNames() const noexcept { return boxNames_; }
    [[nodiscard]] std::span<const uint8_t> sourceBytes() const noexcept { return source_; }
    [[nodiscard]] std::span<const uint8_t> hgssBoxPadding(size_t boxIndex) const noexcept;

    [[nodiscard]] static AssignmentStatus validateAssignment(Layout layout,
                                                             Enums::GameVersion exactGameFromSave,
                                                             std::string_view assignedGameId,
                                                             Enums::GameVersion* assignedExact = nullptr) noexcept;

private:
    Gen4ReadOnlySave() = default;

    Layout layout_ = Layout::DiamondPearl;
    Enums::GameVersion rawFamily_ = Enums::GameVersion::Invalid;
    Enums::GameVersion exactGameFromSave_ = Enums::GameVersion::Invalid;
    Enums::GameVersion assignedExactGame_ = Enums::GameVersion::Invalid;
    AssignmentStatus assignmentStatus_ = AssignmentStatus::Unspecified;
    BlockSelection general_{};
    BlockSelection storage_{};
    TrainerReadOnly trainer_{};
    std::vector<Pokemon::Pokemon4ReadOnly> party_;
    std::vector<Pokemon::Pokemon4ReadOnly> boxes_;
    std::vector<std::u16string> boxNames_;
    uint8_t partyCount_ = 0;
    uint8_t currentBox_ = 0;
    size_t storageOffset_ = 0;
    std::vector<uint8_t> source_;
};

}

#endif
