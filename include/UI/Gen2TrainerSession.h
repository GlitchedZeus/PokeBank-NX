#pragma once
#include "Integration/Gen2/Gen2StagedEditor.h"

namespace PokeBank::UIModel::Gen2Trainer {
struct Session {
    bool active = false;
    bool confirmExit = false;
    std::string name, baselineName;
    uint32_t money = 0, baselineMoney = 0;
    void begin(const PokeVault::Integration::Gen2::StagedEditor& editor) {
        name = baselineName = editor.trainerName();
        money = baselineMoney = editor.money();
        active = true;
        confirmExit = false;
    }
    bool dirty() const noexcept { return name != baselineName || money != baselineMoney; }
    bool back() noexcept {
        if (dirty()) { confirmExit = true; return false; }
        active = false; return true;
    }
    void continueEditing() noexcept { confirmExit = false; }
    void discard() {
        name = baselineName; money = baselineMoney;
        active = false; confirmExit = false;
    }
    bool keep(PokeVault::Integration::Gen2::StagedEditor& editor, std::string& error) {
        if (!active) { error = "No Trainer edit session"; return false; }
        if (!editor.stageTrainerEdit(name, money, error)) return false;
        active = false; confirmExit = false; return true;
    }
};
} // namespace PokeBank::UIModel::Gen2Trainer
