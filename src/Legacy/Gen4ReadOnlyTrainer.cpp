#include "Legacy/Gen4ReadOnlyTrainer.h"

#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Utils/StringHelpers.h"

namespace PokeVault::Legacy {

Gen4ReadOnlyTrainer::Gen4ReadOnlyTrainer(
    Integration::Gen4::Gen4ReadOnlySave save, std::string sourceGameId)
    : Trainer::Trainer({}), save_(std::move(save)), sourceGameId_(std::move(sourceGameId)) {}

std::unique_ptr<Gen4ReadOnlyTrainer> Gen4ReadOnlyTrainer::create(
    const Integration::Gen4::Gen4ReadOnlySave& save,
    std::string sourceGameId,
    std::string& error) {
    error.clear();
    Enums::GameVersion assigned = Enums::GameVersion::Invalid;
    if (Integration::Gen4::Gen4ReadOnlySave::validateAssignment(
            save.layout(), save.exactGameFromSave(), sourceGameId, &assigned) ==
        Integration::Gen4::AssignmentStatus::Mismatch) {
        error = "Generation IV source no longer matches its assigned game card";
        return nullptr;
    }

    auto trainer = std::unique_ptr<Gen4ReadOnlyTrainer>(
        new Gen4ReadOnlyTrainer(save, std::move(sourceGameId)));
    trainer->buildPresentation(error);
    if (!error.empty()) return nullptr;
    return trainer;
}

void Gen4ReadOnlyTrainer::buildPresentation(std::string& error) {
    const auto& tr = save_.trainer();
    trainerName = Utils::utf16ToUtf8(tr.name);
    TID16 = tr.tid;
    SID16 = tr.sid;
    ID32 = static_cast<uint32_t>(TID16) | (static_cast<uint32_t>(SID16) << 16);
    TID = TID16;
    SID = SID16;
    money = tr.money;
    trainerGender = tr.gender;
    currentBox = save_.currentBox();
    saveRevisionString = save_.recovered() ? "Recovered older copy" : "Base";

    boxNames.clear();
    boxNames.reserve(18);
    const auto nativeNames = save_.boxNames();
    for (size_t box = 0; box < 18; ++box) {
        if (box < nativeNames.size() && !nativeNames[box].empty())
            boxNames.push_back(Utils::utf16ToUtf8(nativeNames[box]));
        else
            boxNames.push_back("Box " + std::to_string(box + 1));
    }

    party.clear();
    party.reserve(save_.partyCount());
    const auto nativeParty = save_.nativePartySlots();
    for (size_t slot = 0; slot < save_.partyCount(); ++slot) {
        if (slot >= nativeParty.size()) {
            error = "Generation IV party count exceeds native party records";
            return;
        }
        const auto& pokemon = nativeParty[slot];
        if (!pokemon.valid()) {
            // Keep malformed entities quarantined rather than converting their bytes into trusted
            // shared-model fields. The source save itself remains open/read-only and unchanged.
            party.push_back(nullptr);
            continue;
        }
        party.push_back(std::make_unique<Pokemon::Pokemon4ReadOnlyView>(pokemon));
    }

    boxes.clear();
    boxes.resize(18);
    for (size_t box = 0; box < 18; ++box) {
        for (size_t slot = 0; slot < 30; ++slot) {
            const auto& pokemon = save_.box(box, slot);
            if (!pokemon.valid() || pokemon.empty()) continue;
            boxes[box][slot] = std::make_unique<Pokemon::Pokemon4ReadOnlyView>(pokemon);
        }
    }

    // Inventory is intentionally not exposed in G4-02. Empty means unavailable, not empty bag.
    items.clear();
}

}
