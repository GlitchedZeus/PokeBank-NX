#ifndef POKEBANK_LEGACY_MODERN_READ_ONLY_TRAINER_H
#define POKEBANK_LEGACY_MODERN_READ_ONLY_TRAINER_H

// The ONE Trainer bridge for every Gen VI-IX title served by a Modern::NativeSession adapter. It
// feeds the existing shared Trainer/Party/Boxes/Items renderer from the adapter's staged view and
// never calls an inherited source writer: every update*Block() is a no-op and the original save is
// immutable. Gen VI, VII, VIII and IX all share this class; only the adapter differs.

#include "Integration/Modern/NativeSession.h"
#include "Trainer/Trainer.h"

#include <memory>
#include <string>
#include <string_view>

namespace PokeVault::Legacy {

class ModernReadOnlyTrainer final : public Trainer::Trainer {
public:
    static std::unique_ptr<ModernReadOnlyTrainer> create(
        std::unique_ptr<Integration::Modern::NativeSession> session, std::string exactGameId,
        Enums::GameVersion group, std::string& error);

    void updatePartyBlock() override {}
    void updateBoxBlock() override {}
    void updateItemBlock() override {}
    void updateTrainerInfoBlock() override {}
    void updateBoxNameBlock() override {}
    void updateCurrentBoxBlock() override {}

    std::unique_ptr<Pokemon::Pokemon> createBlankPokemon() const override { return nullptr; }
    size_t getBoxCount() const noexcept override { return session_->boxCount(); }
    size_t getSlotsPerBox() const noexcept override { return session_->slotsPerBox(); }
    size_t getPartySize() const noexcept override { return session_->partyCount(); }
    bool hasStagedChanges() const noexcept override { return session_->hasChanges(); }
    Enums::GameVersion getGameGroup() const noexcept override { return group_; }
    bool supportsBoxNames() const noexcept override { return false; }
    size_t getMaxBoxNameLength() const noexcept override { return 16; }
    bool itemsAreIdIndexed() const override { return true; }

    [[nodiscard]] Integration::Modern::NativeSession& session() noexcept { return *session_; }
    [[nodiscard]] const Integration::Modern::NativeSession& session() const noexcept { return *session_; }
    [[nodiscard]] const std::string& sourceGameId() const noexcept { return gameId_; }

    // Rebuilds the presentation (party, boxes, items, trainer facts) from the adapter's current
    // staged view. Returns false and leaves the previous presentation intact when any declared
    // party slot cannot be shown as a verified record.
    [[nodiscard]] bool refreshPresentation(std::string& error);

private:
    ModernReadOnlyTrainer(std::unique_ptr<Integration::Modern::NativeSession> session, std::string gameId,
                          Enums::GameVersion group);

    std::unique_ptr<Integration::Modern::NativeSession> session_;
    std::string gameId_;
    Enums::GameVersion group_;
};

} // namespace PokeVault::Legacy

#endif
