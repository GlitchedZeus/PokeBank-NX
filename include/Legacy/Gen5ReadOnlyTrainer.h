#ifndef POKEBANK_LEGACY_GEN5_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_GEN5_READ_ONLY_TRAINER_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

// Same Trainer contract and UI as Gen I-IV. Source SAV5 never receives
// mutable serialization authority; pending PK5 fields are app-owned only.
class Gen5ReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<Gen5ReadOnlyTrainer> create(
        const Integration::Gen5::Gen5ReadOnlySave& validated,
        std::string exactGameId,std::string& error);

    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    void updateBoxNameBlock() override {}
    void updateCurrentBoxBlock() override {}

    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return 24; }
    size_t getSlotsPerBox() const noexcept override { return 30; }
    size_t getPartySize() const noexcept override { return save_.partyCount(); }
    bool hasStagedChanges() const noexcept override { return staged_.hasChanges(); }
    Enums::GameVersion getGameGroup() const noexcept override;
    bool supportsBoxNames() const noexcept override { return false; }
    size_t getMaxBoxNameLength() const noexcept override { return 8; }

    [[nodiscard]] const Integration::Gen5::Gen5ReadOnlySave& sourceSave() const noexcept {
        return save_;
    }
    [[nodiscard]] const std::string& sourceGameId() const noexcept {
        return gameId_;
    }
    [[nodiscard]] Integration::Gen5::Gen5StagedPokemonWorkspace& stagedPokemon() noexcept {
        return staged_;
    }
    [[nodiscard]] const Integration::Gen5::Gen5StagedPokemonWorkspace& stagedPokemon() const noexcept {
        return staged_;
    }
    // Present only already validated staged records. This never assembles,
    // writes, injects or exposes a modified SAV5.
    [[nodiscard]] bool refreshStagedPokemonPresentation(std::string& error);

private:
    Gen5ReadOnlyTrainer(Integration::Gen5::Gen5ReadOnlySave save,std::string gameId);
    [[nodiscard]] bool rebuildPresentation(std::string& error);

    Integration::Gen5::Gen5ReadOnlySave save_;
    std::string gameId_;
    Integration::Gen5::Gen5StagedPokemonWorkspace staged_;
};
} // namespace PokeVault::Legacy
#endif
