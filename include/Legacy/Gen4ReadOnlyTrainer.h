#ifndef POKEBANK_LEGACY_GEN4_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_GEN4_READ_ONLY_TRAINER_H

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>

namespace PokeVault::Legacy {

// Read-only presentation bridge for a validated Gen IV source. It owns a copy of the immutable
// parsed save so all six native party records and the original source bytes remain available while
// the shared Trainer UI receives only presentation objects.
class Gen4ReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<Gen4ReadOnlyTrainer> create(
        const Integration::Gen4::Gen4ReadOnlySave& save,
        std::string sourceGameId,
        std::string& error);

    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    void updateBoxNameBlock() override {}
    void updateCurrentBoxBlock() override {}

    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return 18; }
    size_t getSlotsPerBox() const noexcept override { return 30; }
    size_t getPartySize() const noexcept override { return save_.partyCount(); }
    Enums::GameVersion getGameGroup() const noexcept override { return save_.rawFamily(); }
    bool supportsBoxNames() const noexcept override { return true; }
    size_t getMaxBoxNameLength() const noexcept override { return 8; }

    [[nodiscard]] const Integration::Gen4::Gen4ReadOnlySave& sourceSave() const noexcept {
        return save_;
    }
    [[nodiscard]] const std::string& sourceGameId() const noexcept { return sourceGameId_; }

    bool stagedPokemonAvailable() const noexcept { return stagedPokemon_ != nullptr; }
    Integration::Gen4::Gen4StagedPokemonEditor* stagedPokemon() noexcept {
        return stagedPokemon_.get();
    }
    const Integration::Gen4::Gen4StagedPokemonEditor* stagedPokemon() const noexcept {
        return stagedPokemon_.get();
    }
    const std::string& stagedPokemonUnavailableReason() const noexcept {
        return stagedPokemonUnavailableReason_;
    }
    bool refreshStagedPokemonPresentation(std::string& error);

private:
    Gen4ReadOnlyTrainer(Integration::Gen4::Gen4ReadOnlySave save, std::string sourceGameId);
    void buildPresentation(std::string& error);

    Integration::Gen4::Gen4ReadOnlySave save_;
    std::string sourceGameId_;
    std::unique_ptr<Integration::Gen4::Gen4StagedPokemonEditor> stagedPokemon_;
    std::string stagedPokemonUnavailableReason_;
};

}
#endif
