#ifndef POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

// Read-only Generation II Trainer bridge for the normal Trainer/Party/Boxes/Items screens.
// The source parser owns all save-format truth; this adapter only presents already-validated data
// and deliberately implements every write/update entry point as a no-op.
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
};

} // namespace PokeVault::Legacy

#endif
