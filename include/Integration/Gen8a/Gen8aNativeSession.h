#ifndef POKEBANK_GEN8A_NATIVE_SESSION_H
#define POKEBANK_GEN8A_NATIVE_SESSION_H

// Legends: Arceus adapter for the shared Modern::NativeSession contract over Gen8aStagedSession:
// the same Party, Boxes, View/Edit workspace, Actions sheet, Review, Discard and verified Export
// Copy as every other Gen VI+ title. Alpha / Noble, Effort Levels, Alpha move and size scalars are
// shown as native read-only facts. Bag and Pokedex are unavailable in the adapter with reasons.

#include "Integration/Gen8a/Gen8aSession.h"
#include "Integration/Modern/NativeSession.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8a {

class Gen8aNativeSession final : public Modern::NativeSession {
public:
    [[nodiscard]] static Modern::OpenedSession open(std::span<const uint8_t> bytes, std::string_view assignedExactGame);
    explicit Gen8aNativeSession(Gen8aSave source);
    [[nodiscard]] Gen8aStagedSession& staged() noexcept { return session_; }

    std::string_view exactGameId() const noexcept override { return "legends_arceus_switch"; }
    const Capability::GameCapabilities& capabilities() const noexcept override { return *capabilities_; }
    std::string_view familyLabel() const noexcept override { return "Generation VIII"; }
    size_t boxCount() const noexcept override { return Layout::BoxCount; }
    size_t slotsPerBox() const noexcept override { return Layout::BoxSlots; }
    size_t partyCount() const noexcept override { return session_.source().partyCount(); }
    std::string boxName(size_t box) const override { return "Box " + std::to_string(box + 1); }
    std::span<const uint8_t> sourceBytes() const noexcept override { return session_.source().bytes(); }

    std::optional<Modern::RecordView> view(Modern::SlotRef slot) const override;
    bool originalOccupied(Modern::SlotRef slot) const override { return session_.source().at(toNative(slot)).occupied(); }
    bool emptyDestination(Modern::SlotRef slot) const override {
        return slot.region == Modern::Region::Box && session_.source().at(toNative(slot)).blank() && !session_.isStaged(toNative(slot));
    }
    bool stagedAt(Modern::SlotRef slot) const override { return session_.isStaged(toNative(slot)); }
    std::optional<Modern::SlotRef> firstEmptyBoxSlot() const override;

    Modern::FieldState fieldState(Modern::Field field, Modern::Region region) const noexcept override {
        return Gen8a::fieldState(field, region == Modern::Region::Party ? Region::Party : Region::Box);
    }
    bool stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t, std::string& error) override {
        return session_.stageValue(toNative(slot), field, value, &error);
    }
    bool stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) override {
        return session_.stageText(toNative(slot), field, utf8, &error);
    }
    uint32_t fieldMaximum(Modern::Field field) const noexcept override;
    std::vector<uint16_t> heldItemChoices() const override;
    std::vector<uint16_t> ballChoices() const override;
    int maxPP(uint16_t, uint8_t) const noexcept override { return 0; }

    bool beginDraft(Modern::SlotRef slot, std::string& error) override;
    bool draftActive() const noexcept override { return draft_.has_value(); }
    std::optional<Modern::SlotRef> draftSlot() const noexcept override { return draft_; }
    bool draftDirty() const noexcept override { return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_); }
    void keepDraft() override { draft_.reset(); }
    void discardDraft() override;

    Modern::OperationSupport cloneSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport moveSupport(Modern::SlotRef from) const override { return cloneSupport(from); }
    Modern::OperationSupport removeSupport(Modern::SlotRef at) const override;
    Modern::OperationSupport createSupport(Modern::SlotRef) const override { return {false, Gen8aStagedSession::kCreateReason}; }
    bool clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool remove(Modern::SlotRef at, std::string& error) override;
    bool undo(Modern::SlotRef at) override {
        if (draft_ && *draft_ == at) return false;
        return session_.undo(toNative(at));
    }

    bool bagAvailable() const override { return false; }
    std::vector<Modern::BagPocketView> bag() const override { return {}; }
    std::vector<uint16_t> bagAddChoices(size_t) const override { return {}; }
    bool bagSetQuantity(size_t, size_t, uint16_t, std::string& e) override { return refused(e); }
    bool bagRemove(size_t, size_t, std::string& e) override { return refused(e); }
    bool bagAdd(size_t, uint16_t, uint16_t, std::string& e) override { return refused(e); }
    void discardBag() override {}

    std::vector<Modern::TrainerFact> trainerFacts() const override;
    std::string trainerName() const override;
    uint8_t trainerGender() const noexcept override;
    uint16_t trainerId() const noexcept override { return PA8::read16(session_.source().myStatus().data, Layout::MyTid); }
    uint16_t secretId() const noexcept override { return PA8::read16(session_.source().myStatus().data, Layout::MySid); }
    Modern::DexProgress pokedex() const override { return {}; }

    bool hasChanges() const noexcept override { return session_.hasChanges(); }
    size_t changeCount() const noexcept override { return session_.changeCount(); }
    std::vector<Modern::ReviewRow> reviewRows() const override;
    void discardAll() override {
        draft_.reset();
        session_.clear();
    }
    Modern::OperationSupport exportSupport() const override {
        return session_.hasChanges() ? Modern::OperationSupport{true, {}}
                                     : Modern::OperationSupport{false, "Export Copy needs at least one staged change"};
    }
    Common::VerifiedExportResult exportWorkingCopy(const Modern::ExportRequest& request) const override;

    [[nodiscard]] static constexpr SlotRef toNative(Modern::SlotRef s) noexcept {
        return s.region == Modern::Region::Party ? SlotRef::party(s.slot) : SlotRef::boxed(s.box, s.slot);
    }
    static constexpr std::string_view kBagReason =
        "Legends: Arceus Bag needs a verified PlayerBag8a staged editor; the installed-title reader shows pouches read-only";

private:
    static bool refused(std::string& e) {
        e = std::string(kBagReason);
        return false;
    }
    Gen8aStagedSession session_;
    const Capability::GameCapabilities* capabilities_ = nullptr;
    std::optional<Modern::SlotRef> draft_;
    Gen8aStagedSession::EntrySnapshot draftSnapshot_;
};

} // namespace PokeVault::Integration::Gen8a

#endif
