#ifndef POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_GSC_READ_ONLY_TRAINER_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

// Strict read-only bridge from the validated Generation II model into the shared Trainer/Party/
// Boxes/Items screens. Every mutation hook is intentionally inert; RetroArch source writes remain
// hard disabled.
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
    Enums::GameVersion getGameGroup() const noexcept override { return Enums::GameVersion::GSC; }

    const std::string& sourceGameId() const noexcept { return sourceGameId_; }
    Integration::Gen2::RegionLayout regionLayout() const noexcept { return region_; }
    Integration::Gen2::VersionFamily versionFamily() const noexcept { return family_; }
    bool hasStoredTrainerGender() const noexcept { return hasStoredTrainerGender_; }

private:
    explicit GSCReadOnlyTrainer(const Integration::Gen2::Metadata& metadata);
    bool populate(const Integration::Gen2::ReadOnlySave& save, std::string& error);

    size_t boxCount_ = 0;
    size_t slotsPerBox_ = 0;
    std::string sourceGameId_;
    Integration::Gen2::RegionLayout region_ = Integration::Gen2::RegionLayout::International;
    Integration::Gen2::VersionFamily family_ = Integration::Gen2::VersionFamily::GoldSilver;
    bool hasStoredTrainerGender_ = false;
};

} // namespace PokeVault::Legacy

#endif
