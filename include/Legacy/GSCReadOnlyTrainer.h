#ifndef POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

// Read-only Generation II Trainer bridge for the normal Trainer/Party/Boxes/Items screens.
// The source parser owns all save-format truth. The bridge's inherited write/update entry points stay
// no-op so a RetroArch source can never become mutable by accident. The optional StagedEditor below
// owns a separate byte clone used only by the explicit export-copy workflow.
class GSCReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<GSCReadOnlyTrainer> create(
        const Integration::Gen2::ReadOnlySave& save, std::string& error);

    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return boxCount_; }
    size_t getSlotsPerBox() const noexcept override { return slotsPerBox_; }
    size_t getPartySize() const noexcept override { return party.size(); }
    Enums::GameVersion getGameGroup() const noexcept override {
        return Enums::GameVersion::GSC;
    }

    const std::string& sourceGameId() const noexcept { return sourceGameId_; }
    bool japaneseLayout() const noexcept { return japaneseLayout_; }
    bool crystalFamily() const noexcept { return crystalFamily_; }
    bool hasTrainerGender() const noexcept { return trainerGenderAvailable_; }
    bool inventoryAvailable() const noexcept { return inventoryAvailable_; }

    bool stagedEditingAvailable() const noexcept { return stagedEditor_ != nullptr; }
    Integration::Gen2::StagedEditor* stagedEditor() noexcept { return stagedEditor_.get(); }
    const Integration::Gen2::StagedEditor* stagedEditor() const noexcept { return stagedEditor_.get(); }
    const std::string& stagedEditingUnavailableReason() const noexcept {
        return stagedEditingUnavailableReason_;
    }

private:
    explicit GSCReadOnlyTrainer(const Integration::Gen2::Metadata& metadata);
    bool populate(const Integration::Gen2::ReadOnlySave& save, std::string& error);

    size_t boxCount_ = 0;
    size_t slotsPerBox_ = 0;
    std::string sourceGameId_;
    bool japaneseLayout_ = false;
    bool crystalFamily_ = false;
    bool trainerGenderAvailable_ = false;
    bool inventoryAvailable_ = false;
    std::unique_ptr<Integration::Gen2::StagedEditor> stagedEditor_;
    std::string stagedEditingUnavailableReason_;
};

} // namespace PokeVault::Legacy

#endif
