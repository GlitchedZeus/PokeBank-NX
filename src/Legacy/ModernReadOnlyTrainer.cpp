#include "Legacy/ModernReadOnlyTrainer.h"

#include "Pokemon/ModernRecordView.h"

#include <utility>
#include <vector>

namespace PokeVault::Legacy {
namespace Modern = PokeVault::Integration::Modern;

ModernReadOnlyTrainer::ModernReadOnlyTrainer(std::unique_ptr<Modern::NativeSession> session, std::string gameId,
                                             Enums::GameVersion group)
    : Trainer::Trainer(std::vector<Save::Block>{}), session_(std::move(session)), gameId_(std::move(gameId)),
      group_(group) {}

std::unique_ptr<ModernReadOnlyTrainer> ModernReadOnlyTrainer::create(
    std::unique_ptr<Modern::NativeSession> session, std::string exactGameId, Enums::GameVersion group,
    std::string& error) {
    error.clear();
    if (!session) {
        error = "Shared Modern bridge requires an opened native session";
        return nullptr;
    }
    if (session->exactGameId() != exactGameId) {
        error = "Shared Modern bridge requires a matching exact title";
        return nullptr;
    }
    auto trainer = std::unique_ptr<ModernReadOnlyTrainer>(
        new ModernReadOnlyTrainer(std::move(session), std::move(exactGameId), group));
    auto& s = *trainer->session_;
    trainer->trainerName = s.trainerName();
    trainer->TID16 = s.trainerId();
    trainer->SID16 = s.secretId();
    trainer->TID = trainer->TID16;
    trainer->SID = trainer->SID16;
    trainer->ID32 = uint32_t(trainer->TID16) | (uint32_t(trainer->SID16) << 16);
    trainer->trainerGender = s.trainerGender();
    trainer->money = 0;
    trainer->currentBox = 0;
    trainer->saveRevisionString = std::string(s.familyLabel()) + " / READ ONLY / P" + std::to_string(s.partyCount());
    trainer->boxNames.reserve(s.boxCount());
    for (size_t box = 0; box < s.boxCount(); ++box) trainer->boxNames.push_back(s.boxName(box));
    const auto dex = s.pokedex();
    trainer->nativeDexProgress = {dex.seen, dex.caught, dex.total};
    if (!trainer->refreshPresentation(error)) return nullptr;
    return trainer;
}

bool ModernReadOnlyTrainer::refreshPresentation(std::string& error) {
    error.clear();
    auto& s = *session_;
    std::vector<std::unique_ptr<Pokemon::Pokemon>> displayParty;
    displayParty.reserve(s.partyCount());
    for (size_t slot = 0; slot < s.partyCount(); ++slot) {
        auto view = s.view(Modern::SlotRef::party(slot));
        if (!view || !view->valid || view->empty) {
            error = std::string(s.familyLabel()) + " declared party slot cannot be shown as a verified record";
            return false;
        }
        displayParty.push_back(std::make_unique<Pokemon::ModernRecordView>(std::move(*view), group_));
    }
    // The shared panels address boxes as fixed BOX_SLOTS-wide rows; every Modern family fits
    // (31x30, 32x30, LGPE 40x25), so a wider native box is refused rather than truncated.
    decltype(boxes) displayBoxes(s.boxCount());
    if (s.slotsPerBox() > displayBoxes.front().size()) {
        error = std::string(s.familyLabel()) + " box geometry exceeds the shared presentation";
        return false;
    }
    for (size_t box = 0; box < s.boxCount(); ++box) {
        for (size_t slot = 0; slot < s.slotsPerBox(); ++slot) {
            auto view = s.view(Modern::SlotRef::boxed(box, slot));
            if (!view || !view->valid || view->empty) continue;  // blank or quarantined: shown empty
            displayBoxes[box][slot] = std::make_unique<Pokemon::ModernRecordView>(std::move(*view), group_);
        }
    }
    party.swap(displayParty);
    boxes.swap(displayBoxes);
    decltype(items) displayItems;
    for (const auto& pocket : s.bag()) displayItems.push_back(pocket.items);
    items.swap(displayItems);
    trainerName = s.trainerName();
    return true;
}

} // namespace PokeVault::Legacy
