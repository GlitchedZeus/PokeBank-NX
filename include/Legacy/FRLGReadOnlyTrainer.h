#ifndef POKEBANK_LEGACY_FRLG_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_FRLG_READ_ONLY_TRAINER_H

#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {
class FRLGReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<FRLGReadOnlyTrainer> create(
        const Integration::Gen3::ReadOnlySave& save, std::string& error);
    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return boxCount_; }
    size_t getSlotsPerBox() const noexcept override { return slotsPerBox_; }
    size_t getPartySize() const noexcept override { return party.size(); }
    Enums::GameVersion getGameGroup() const noexcept override { return Enums::GameVersion::FRLG; }
    const std::string& sourceGameId() const noexcept { return sourceGameId_; }
    bool stagedInventoryAvailable() const noexcept { return stagedInventory_ != nullptr; }
    Integration::Gen3::StagedInventoryEditor* stagedInventory() noexcept { return stagedInventory_.get(); }
    const Integration::Gen3::StagedInventoryEditor* stagedInventory() const noexcept { return stagedInventory_.get(); }
    const std::string& stagedInventoryUnavailableReason() const noexcept { return stagedInventoryUnavailableReason_; }
private:
    explicit FRLGReadOnlyTrainer(const Integration::Gen3::SaveMetadata& metadata);
    bool populate(const Integration::Gen3::ReadOnlySave& save, std::string& error);
    size_t boxCount_ = 14;
    size_t slotsPerBox_ = 30;
    std::string sourceGameId_;
    std::unique_ptr<Integration::Gen3::StagedInventoryEditor> stagedInventory_;
    std::string stagedInventoryUnavailableReason_;
};
}

#endif
