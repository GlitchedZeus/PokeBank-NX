#include "Legacy/Gen5ReadOnlyTrainer.h"

#include "Integration/Gen5/Gen5TrainerName.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Legacy {
namespace Gen5 = PokeVault::Integration::Gen5;

Gen5ReadOnlyTrainer::Gen5ReadOnlyTrainer(Gen5::Gen5ReadOnlySave save,std::string gameId)
    : Trainer::Trainer(std::vector<Save::Block>{}),
      save_(std::move(save)),gameId_(std::move(gameId)),staged_(save_) {}

std::unique_ptr<Gen5ReadOnlyTrainer> Gen5ReadOnlyTrainer::create(
    const Gen5::Gen5ReadOnlySave& validated,
    std::string exactGameId,std::string& error) {
    error.clear();
    if(!Gen5::isExactGen5Id(exactGameId) ||
       validated.exactGameId()!=exactGameId) {
        error="Gen V shared Trainer bridge requires a matching exact title";
        return nullptr;
    }
    auto trainer=std::unique_ptr<Gen5ReadOnlyTrainer>(
        new Gen5ReadOnlyTrainer(validated,std::move(exactGameId)));
    const auto native=trainer->save_.trainer();
    trainer->trainerName=Gen5::displayTrainerName(native.rawName).value_or("Unknown");
    trainer->TID16=native.tid;
    trainer->SID16=native.sid;
    trainer->ID32=uint32_t(native.tid) | (uint32_t(native.sid)<<16);
    trainer->TID=native.tid;
    trainer->SID=native.sid;
    trainer->trainerGender=native.gender<=1?native.gender:0;
    trainer->money=0; // Not decoded; no write or title-money editing.
    trainer->currentBox=0;
    trainer->saveRevisionString="Gen V / READ ONLY";
    trainer->saveRevisionString+=validated.selectedBackupPartition() ?
        " / Selected backup copy" : " / Selected primary copy";
    trainer->saveRevisionString+=" / P"+std::to_string(validated.partyCount())+
        " B"+std::to_string(validated.diagnostics().occupiedBoxRecords)+
        " Q"+std::to_string(validated.diagnostics().invalidBoxRecords);
    trainer->boxNames.reserve(24);
    for(size_t box=0;box<24;++box)
        trainer->boxNames.push_back("Box "+std::to_string(box+1));
    if(!trainer->rebuildPresentation(error))return nullptr;
    return trainer;
}

Enums::GameVersion Gen5ReadOnlyTrainer::getGameGroup() const noexcept {
    return save_.family()==Gen5::SaveFamily::Black2White2
        ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;
}

bool Gen5ReadOnlyTrainer::refreshStagedPokemonPresentation(std::string& error) {
    return rebuildPresentation(error);
}

bool Gen5ReadOnlyTrainer::rebuildPresentation(std::string& error) {
    error.clear();
    std::vector<std::unique_ptr<Pokemon::Pokemon>> displayParty;
    displayParty.reserve(save_.partyCount());
    decltype(boxes) displayBoxes(24);
    for(size_t slot=0;slot<save_.partyCount();++slot) {
        auto pk=staged_.viewParty(slot);
        if(!pk || !pk->valid() || pk->empty() || !pk->partyRecord()) {
            error="Gen V declared party slot cannot be shown as verified PK5";
            return false;
        }
        displayParty.push_back(
            std::make_unique<Pokemon::Pokemon5ReadOnlyView>(std::move(*pk),getGameGroup()));
    }
    for(size_t box=0;box<24;++box) {
        for(size_t slot=0;slot<30;++slot) {
            auto pk=staged_.viewBox(box,slot);
            if(!pk || !pk->valid() || pk->empty())continue;
            if(pk->partyRecord()) {
                error="Gen V boxed record unexpectedly contains a party PK5 payload";
                return false;
            }
            displayBoxes[box][slot]=std::make_unique<Pokemon::Pokemon5ReadOnlyView>(
                std::move(*pk),getGameGroup());
        }
    }
    party.swap(displayParty);
    boxes.swap(displayBoxes);
    items.clear(); // Unsupported pouch data is not represented as a known inventory.
    return true;
}
} // namespace PokeVault::Legacy
