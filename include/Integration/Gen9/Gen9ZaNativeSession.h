#ifndef POKEBANK_GEN9_ZA_NATIVE_SESSION_H
#define POKEBANK_GEN9_ZA_NATIVE_SESSION_H

// Legends: Z-A adapter for the shared Modern::NativeSession contract. It owns one Gen9ZaStagedSession
// (strict SCBlock parse, app-memory staged Edit/Clone/Remove with battle-team protection, canonical
// re-encryption verifier, VerifiedStagedExport) and exposes it to the ONE shared surface: the same
// Party/Boxes, View/Edit workspace, Actions sheet, Review, Discard and Export Copy that Gen VI and
// Gen VII use. Bag and Pokedex are read-unavailable with reasons until verified decoders exist.
// Nothing here writes the original save or an installed save.

#include "Integration/Gen9/Gen9ZaSession.h"
#include "Integration/Modern/NativeSession.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen9Za {

class Gen9ZaNativeSession final : public Modern::NativeSession {
public:
    // Opens a strictly validated `main` as a shared session. `assignedExactGame` must match the
    // MyStatus version byte; a mismatch is refused with a diagnostic (never silently retargeted).
    [[nodiscard]] static Modern::OpenedSession open(std::span<const uint8_t> bytes,
                                                    std::string_view assignedExactGame);
    explicit Gen9ZaNativeSession(Gen9ZaSave source);

    [[nodiscard]] Gen9ZaStagedSession& staged() noexcept { return session_; }
    [[nodiscard]] const Gen9ZaStagedSession& staged() const noexcept { return session_; }
    [[nodiscard]] const Gen9ZaSave& source() const noexcept { return session_.source(); }

    std::string_view exactGameId() const noexcept override { return session_.source().exactGameId(); }
    const Capability::GameCapabilities& capabilities() const noexcept override { return *capabilities_; }
    std::string_view familyLabel() const noexcept override { return "Generation IX"; }
    size_t boxCount() const noexcept override { return Layout::BoxCount; }
    size_t slotsPerBox() const noexcept override { return Layout::BoxSlots; }
    size_t partyCount() const noexcept override { return session_.source().partyCount(); }
    std::string boxName(size_t box) const override { return "Box " + std::to_string(box + 1); }
    std::span<const uint8_t> sourceBytes() const noexcept override { return session_.source().bytes(); }

    std::optional<Modern::RecordView> view(Modern::SlotRef slot) const override;
    bool originalOccupied(Modern::SlotRef slot) const override;
    bool emptyDestination(Modern::SlotRef slot) const override;
    bool stagedAt(Modern::SlotRef slot) const override { return session_.isStaged(toNative(slot)); }
    std::optional<Modern::SlotRef> firstEmptyBoxSlot() const override;

    Modern::FieldState fieldState(Modern::Field field, Modern::Region region) const noexcept override;
    bool stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                    std::string& error) override;
    bool stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) override;
    uint32_t fieldMaximum(Modern::Field field) const noexcept override;
    std::vector<uint16_t> heldItemChoices() const override;
    std::vector<uint16_t> ballChoices() const override;
    int maxPP(uint16_t, uint8_t) const noexcept override { return 0; }  // moves are read-only

    bool beginDraft(Modern::SlotRef slot, std::string& error) override;
    bool draftActive() const noexcept override { return draft_.has_value(); }
    std::optional<Modern::SlotRef> draftSlot() const noexcept override { return draft_; }
    bool draftDirty() const noexcept override;
    void keepDraft() override { draft_.reset(); }
    void discardDraft() override;

    Modern::OperationSupport cloneSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport moveSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport removeSupport(Modern::SlotRef at) const override;
    Modern::OperationSupport createSupport(Modern::SlotRef) const override {
        return {false, Gen9ZaStagedSession::kCreateReason};
    }
    bool clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool remove(Modern::SlotRef at, std::string& error) override;
    bool undo(Modern::SlotRef at) override {
        if (draft_ && *draft_ == at) return false;  // the open draft owns this slot until keep/discard
        return session_.undo(toNative(at));
    }

    // Bag: the MyItem8b staged editor is not verified in this build.
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
    uint16_t trainerId() const noexcept override;
    uint16_t secretId() const noexcept override;
    Modern::DexProgress pokedex() const override { return {}; }  // Zukan9 staged decoder not verified here

    bool hasChanges() const noexcept override { return session_.hasChanges(); }
    size_t changeCount() const noexcept override { return session_.changeCount(); }
    std::vector<Modern::ReviewRow> reviewRows() const override;
    void discardAll() override {
        draft_.reset();
        session_.clear();
    }

    Modern::OperationSupport exportSupport() const override;
    Common::VerifiedExportResult exportWorkingCopy(const Modern::ExportRequest& request) const override;

    [[nodiscard]] static constexpr SlotRef toNative(Modern::SlotRef s) noexcept {
        return s.region == Modern::Region::Party ? SlotRef::party(s.slot) : SlotRef::boxed(s.box, s.slot);
    }
    [[nodiscard]] static constexpr Modern::SlotRef fromNative(SlotRef s) noexcept {
        return s.region == Region::Party ? Modern::SlotRef::party(s.slot) : Modern::SlotRef::boxed(s.box, s.slot);
    }

    static constexpr std::string_view kBagReason =
        "Legends: Z-A Bag needs a verified MyItem9a staged editor; the installed-title reader shows pockets read-only";

private:
    static bool bagRefused(std::string& error) {
        error = std::string(kBagReason);
        return false;
    }

    Gen9ZaStagedSession session_;
    const Capability::GameCapabilities* capabilities_ = nullptr;
    std::optional<Modern::SlotRef> draft_;
    Gen9ZaStagedSession::EntrySnapshot draftSnapshot_;
};

} // namespace PokeVault::Integration::Gen9Za

#endif
