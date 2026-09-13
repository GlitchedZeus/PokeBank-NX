#ifndef POKEBANK_LEGACY_RBY_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_RBY_READ_ONLY_TRAINER_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

class RBYReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<RBYReadOnlyTrainer> create(
        const Integration::Gen1::ReadOnlySave& save, std::string& error);

    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return boxCount_; }
    size_t getSlotsPerBox() const noexcept override { return slotsPerBox_; }
    size_t getPartySize() const noexcept override { return party.size(); }
    Enums::GameVersion getGameGroup() const noexcept override { return Enums::GameVersion::RBY; }

    const std::string& sourceGameId() const noexcept { return sourceGameId_; }
    bool japaneseLayout() const noexcept { return japaneseLayout_; }
    bool stagedInventoryAvailable() const noexcept { return stagedInventory_ != nullptr; }
    Integration::Gen1::StagedInventoryEditor* stagedInventory() noexcept { return stagedInventory_.get(); }
    const Integration::Gen1::StagedInventoryEditor* stagedInventory() const noexcept { return stagedInventory_.get(); }
    const std::string& stagedInventoryUnavailableReason() const noexcept { return stagedInventoryUnavailableReason_; }

private:
    explicit RBYReadOnlyTrainer(const Integration::Gen1::Metadata& metadata);
    bool populate(const Integration::Gen1::ReadOnlySave& save, std::string& error);

    size_t boxCount_ = 0;
    size_t slotsPerBox_ = 0;
    std::string sourceGameId_;
    bool japaneseLayout_ = false;
    std::unique_ptr<Integration::Gen1::StagedInventoryEditor> stagedInventory_;
    std::string stagedInventoryUnavailableReason_;
};

} // namespace PokeVault::Legacy

#endif
