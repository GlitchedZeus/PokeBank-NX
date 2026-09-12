#ifndef POKEVAULT_GEN2_STAGED_EDITOR_H
#define POKEVAULT_GEN2_STAGED_EDITOR_H

#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Save/EditableSaveCapabilities.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen2 {

inline constexpr uint8_t kMasterBallItemId = 0x01;
inline constexpr uint8_t kPokeBallItemId = 0x05;
inline constexpr uint8_t kPotionItemId = 0x12;
inline constexpr uint32_t kMaxMoney = 999999;
inline constexpr uint8_t kMaxStackQuantity = 99;
inline constexpr std::size_t kInternationalTrainerNameMax = 7;

struct StagedChange {
    std::string key;
    std::string label;
    std::string beforeValue;
    std::string afterValue;
};

class StagedEditor final {
public:
    static std::unique_ptr<StagedEditor> create(const ReadOnlySave& source, std::string& error);

    const SaveEdit::Capabilities& capabilities() const noexcept { return capabilities_; }
    const Metadata& metadata() const noexcept { return metadata_; }
    std::span<const uint8_t> originalBytes() const noexcept { return original_; }
    std::span<const uint8_t> stagedBytes() const noexcept { return staged_; }

    const std::string& trainerName() const noexcept { return trainerName_; }
    uint32_t money() const noexcept { return money_; }
    uint8_t itemQuantity(InventoryPocket pocket, uint8_t itemId) const noexcept;

    bool stageTrainerName(std::string_view name, std::string& error);
    bool stageMoney(uint32_t money, std::string& error);
    bool stageItemQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                           std::string& error);

    const std::vector<StagedChange>& pendingChanges() const noexcept { return changes_; }
    bool hasPendingChanges() const noexcept { return !changes_.empty(); }
    void discard() noexcept;

    // Returns a fully serialized copy suitable for export. The original/source bytes are never
    // mutated. Retail mirror regions are synchronized and both overall checksums are repaired.
    std::vector<uint8_t> finalizedBytes(std::string& error) const;

private:
    explicit StagedEditor(const ReadOnlySave& source);

    bool encodeTrainerName(std::string_view name, std::vector<uint8_t>& encoded,
                           std::string& error) const;
    void setChange(std::string key, std::string label,
                   std::string beforeValue, std::string afterValue);
    void clearChange(std::string_view key);

    Metadata metadata_;
    SourceGame sourceGame_ = SourceGame::Gold;
    SaveEdit::Capabilities capabilities_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
    std::string originalTrainerName_;
    std::string trainerName_;
    uint32_t originalMoney_ = 0;
    uint32_t money_ = 0;
    std::vector<StagedChange> changes_;
};

} // namespace PokeVault::Integration::Gen2

#endif
