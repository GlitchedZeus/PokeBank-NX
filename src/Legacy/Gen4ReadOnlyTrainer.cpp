#include "Legacy/Gen4ReadOnlyTrainer.h"

#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Utils/StringHelpers.h"

namespace PokeVault::Legacy {

Gen4ReadOnlyTrainer::Gen4ReadOnlyTrainer(
    Integration::Gen4::Gen4ReadOnlySave save, std::string sourceGameId)
    : Trainer::Trainer(std::vector<Save::Block>{}), save_(std::move(save)), sourceGameId_(std::move(sourceGameId)) {}

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

    std::string stagedError;
    trainer->stagedPokemon_ = Integration::Gen4::Gen4StagedPokemonEditor::create(
        save.sourceBytes(), save.layout(), trainer->sourceGameId_, &stagedError);
    trainer->stagedPokemonUnavailableReason_ = std::move(stagedError);
    return trainer;
}


bool Gen4ReadOnlyTrainer::refreshStagedPokemonPresentation(std::string& error) {
    error.clear();
    if (!stagedPokemon_) {
        error = stagedPokemonUnavailableReason_.empty()
            ? "Generation IV staged Pokemon editing is unavailable"
            : stagedPokemonUnavailableReason_;
        return false;
    }

    auto bytes = stagedPokemon_->finalizedBytes(&error);
    if (bytes.empty()) return false;
    auto parsed = Integration::Gen4::Gen4ReadOnlySave::parse(
        bytes, save_.layout(), sourceGameId_, &error);
    if (!parsed || parsed->assignmentStatus() != Integration::Gen4::AssignmentStatus::Match) {
        if (error.empty()) error = "staged Generation IV presentation failed strict reparse";
        return false;
    }

    decltype(boxes) displayBoxes(18);
    for (size_t box = 0; box < 18; ++box) {
        for (size_t slot = 0; slot < 30; ++slot) {
            const auto& pokemon = parsed->box(box, slot);
            if (!pokemon.valid()) {
                error = "staged Generation IV presentation contains an invalid PK4";
                return false;
            }
            if (pokemon.empty()) continue;
            displayBoxes[box][slot] =
                std::make_unique<Pokemon::Pokemon4ReadOnlyView>(pokemon);
        }
    }
    boxes.swap(displayBoxes);
    return true;
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
    const auto& diagnostics = save_.diagnostics();
    const bool cartridgeHasNoPokemon =
        diagnostics.declaredPartyCount == 0 &&
        diagnostics.occupiedBoxRecords == 0 &&
        diagnostics.invalidPartyRecords == 0 &&
        diagnostics.invalidBoxRecords == 0;
    saveRevisionString = save_.recovered()
        ? "Recovered older copy"
        : cartridgeHasNoPokemon ? "0 Pokemon in cartridge save" : "Base";
    // Keep this compact enough for the title bar while making a real-hardware empty/quarantine
    // result self-diagnosing. "P" is declared party / invalid-in-party; "B" is occupied boxes /
    // invalid box records. A genuinely new save reads P0/0 B0/0; a crypto/layout problem exposes
    // rejected records instead of silently looking like an empty collection.
    saveRevisionString += " | G4 P" + std::to_string(diagnostics.declaredPartyCount) +
        "/" + std::to_string(diagnostics.invalidPartyRecords) +
        " B" + std::to_string(diagnostics.occupiedBoxRecords) +
        "/" + std::to_string(diagnostics.invalidBoxRecords) +
        " T" + std::to_string(tr.playedHours) + ":" +
        (tr.playedMinutes < 10 ? "0" : "") + std::to_string(tr.playedMinutes);

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
