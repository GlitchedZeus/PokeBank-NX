#ifndef POKEBANK_GEN7B_NATIVE_SESSION_H
#define POKEBANK_GEN7B_NATIVE_SESSION_H

// Let's Go Pikachu / Eevee adapter for the shared Modern::NativeSession contract: the same Party,
// Boxes, View/Edit workspace, Actions sheet, Review, Discard and Export Copy as every other Gen VI+
// title, over Gen7bStagedSession. Party slots are pointers into the gapless storage list, so a party
// member is the same record in Party and in Boxes. Nothing here writes the original save.

#include "Integration/Gen7b/Gen7bStagedSession.h"
#include "Integration/Modern/NativeSession.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7b {

class Gen7bNativeSession final : public Modern::NativeSession {
public:
    [[nodiscard]] static Modern::OpenedSession open(std::span<const uint8_t> bytes, std::string_view assignedExactGame);
    explicit Gen7bNativeSession(Gen7bSave source);

    [[nodiscard]] Gen7bStagedSession& staged() noexcept { return session_; }
    [[nodiscard]] const Gen7bStagedSession& staged() const noexcept { return session_; }

    // Storage index for a shared slot (party slots resolve through the current pointers).
    [[nodiscard]] std::optional<size_t> indexOf(Modern::SlotRef slot) const noexcept;
    [[nodiscard]] static constexpr Modern::SlotRef boxSlotFor(size_t index) noexcept {
        return Modern::SlotRef::boxed(index / Layout::BoxSlots, index % Layout::BoxSlots);
    }

    std::string_view exactGameId() const noexcept override { return session_.source().exactGameId(); }
    const Capability::GameCapabilities& capabilities() const noexcept override { return *capabilities_; }
    std::string_view familyLabel() const noexcept override { return "Generation VII"; }
    size_t boxCount() const noexcept override { return Layout::BoxCount; }
    size_t slotsPerBox() const noexcept override { return Layout::BoxSlots; }
    size_t partyCount() const noexcept override { return session_.current().header.partyCount(); }
    std::string boxName(size_t box) const override { return "Box " + std::to_string(box + 1); }
    std::span<const uint8_t> sourceBytes() const noexcept override { return session_.source().bytes(); }

    std::optional<Modern::RecordView> view(Modern::SlotRef slot) const override;
    bool originalOccupied(Modern::SlotRef slot) const override;
    bool emptyDestination(Modern::SlotRef slot) const override;
    bool stagedAt(Modern::SlotRef slot) const override;
    std::optional<Modern::SlotRef> firstEmptyBoxSlot() const override;

    Modern::FieldState fieldState(Modern::Field field, Modern::Region) const noexcept override {
        return Gen7b::fieldState(field);
    }
    bool stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index, std::string& error) override;
    bool stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) override;
    uint32_t fieldMaximum(Modern::Field field) const noexcept override;
    std::vector<uint16_t> heldItemChoices() const override { return {}; }
    std::vector<uint16_t> ballChoices() const override { return {Balls.begin(), Balls.end()}; }
    int maxPP(uint16_t, uint8_t) const noexcept override { return 0; }

    bool beginDraft(Modern::SlotRef slot, std::string& error) override;
    bool draftActive() const noexcept override { return draft_.has_value(); }
    std::optional<Modern::SlotRef> draftSlot() const noexcept override { return draft_; }
    bool draftDirty() const noexcept override { return draft_ && session_.mark() > draftMark_; }
    void keepDraft() override { draft_.reset(); }
    void discardDraft() override;

    Modern::OperationSupport cloneSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport moveSupport(Modern::SlotRef) const override { return {false, kMoveReason}; }
    Modern::OperationSupport removeSupport(Modern::SlotRef at) const override;
    Modern::OperationSupport createSupport(Modern::SlotRef) const override { return {false, kCreateReason}; }
    bool clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool move(Modern::SlotRef, Modern::SlotRef, std::string& error) override {
        error = std::string(kMoveReason);
        return false;
    }
    bool remove(Modern::SlotRef at, std::string& error) override;
    bool undo(Modern::SlotRef at) override;

    bool bagAvailable() const override { return false; }
    std::vector<Modern::BagPocketView> bag() const override { return {}; }
    std::vector<uint16_t> bagAddChoices(size_t) const override { return {}; }
    bool bagSetQuantity(size_t, size_t, uint16_t, std::string& error) override { return bagRefused(error); }
    bool bagRemove(size_t, size_t, std::string& error) override { return bagRefused(error); }
    bool bagAdd(size_t, uint16_t, uint16_t, std::string& error) override { return bagRefused(error); }
    void discardBag() override {}

    std::vector<Modern::TrainerFact> trainerFacts() const override;
    std::string trainerName() const override;
    uint8_t trainerGender() const noexcept override;
    uint16_t trainerId() const noexcept override { return PK6::read16(session_.source().bytes(), Layout::Tid); }
    uint16_t secretId() const noexcept override { return PK6::read16(session_.source().bytes(), Layout::Sid); }
    Modern::DexProgress pokedex() const override { return {}; }

    bool hasChanges() const noexcept override { return session_.hasChanges(); }
    size_t changeCount() const noexcept override { return session_.ops().size(); }
    std::vector<Modern::ReviewRow> reviewRows() const override;
    void discardAll() override {
        draft_.reset();
        session_.clear();
    }

    Modern::OperationSupport exportSupport() const override;
    Common::VerifiedExportResult exportWorkingCopy(const Modern::ExportRequest& request) const override;

    static constexpr std::string_view kMoveReason =
        "Let's Go storage is a gapless list whose positions the game assigns; Move is not offered (Clone appends, Release compacts)";
    static constexpr std::string_view kCreateReason =
        "Let's Go Create is unsupported: EC/PID, personal data, met, GO origin, height/weight and cached CP cannot be derived verifiably";
    static constexpr std::string_view kBagReason =
        "Let's Go Bag needs a verified MyItem (block 0) pouch decoder; the installed-title reader shows pouches read-only";

private:
    static bool bagRefused(std::string& error) {
        error = std::string(kBagReason);
        return false;
    }

    Gen7bStagedSession session_;
    const Capability::GameCapabilities* capabilities_ = nullptr;
    std::optional<Modern::SlotRef> draft_;
    size_t draftMark_ = 0;
};

} // namespace PokeVault::Integration::Gen7b

#endif
