#ifndef POKEBANK_INTEGRATION_GEN4_STAGED_TRAINER_EDITOR_H
#define POKEBANK_INTEGRATION_GEN4_STAGED_TRAINER_EDITOR_H

#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Utils/StringHelpers.h"

#include <cstdint>
#include <string>

namespace PokeVault::Integration::Gen4 {

// R2-BUG-002: the Gen IV Trainer page's Name / Money editor. It is a view onto the ONE staged
// Gen IV workspace (Gen4StagedPokemonEditor), so a trainer edit lives in the same app-owned image
// as Pokemon and bag edits: Review lists it, Discard restores it, the verified Gen IV export
// (with its stale-source check) writes it, and the source save is never touched. Field layout,
// bounds and checksum handling: Gen4StagedPokemonEditor::stageTrainerName / stageTrainerMoney.
class Gen4StagedTrainerEditor {
public:
    explicit Gen4StagedTrainerEditor(Gen4StagedPokemonEditor& workspace) noexcept : workspace_(workspace) {}

    static constexpr uint32_t kMaxMoney = Gen4StagedPokemonEditor::kMaxMoney;
    static constexpr int kNameMax = static_cast<int>(Gen4StagedPokemonEditor::kTrainerNameMax);

    // False when the staged image no longer strictly reparses (nothing can be staged then).
    [[nodiscard]] bool available() const { return workspace_.stagedTrainer().has_value(); }
    [[nodiscard]] std::string trainerName() const {
        const auto trainer = workspace_.stagedTrainer();
        return trainer ? Utils::utf16ToUtf8(trainer->name) : std::string();
    }
    [[nodiscard]] uint32_t money() const {
        const auto trainer = workspace_.stagedTrainer();
        return trainer ? trainer->money : 0;
    }
    [[nodiscard]] bool nameEditable() const { return workspace_.trainerNameEditable(); }

    // Stages only the fields that changed; returns false (nothing staged for that field) on refusal.
    bool stage(const std::string& name, uint32_t money, std::string& error) {
        error.clear();
        if (name != trainerName() && !workspace_.stageTrainerName(Utils::utf8ToUtf16(name), &error))
            return false;
        if (money != this->money() && !workspace_.stageTrainerMoney(money, &error)) return false;
        return true;
    }

private:
    Gen4StagedPokemonEditor& workspace_;
};

} // namespace PokeVault::Integration::Gen4

#endif
