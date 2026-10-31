#ifndef POKEBANK_GEN6_NATIVE_SESSION_H
#define POKEBANK_GEN6_NATIVE_SESSION_H

// Gen VI (X / Y / Omega Ruby / Alpha Sapphire) adapter for the shared Modern::NativeSession
// contract. It owns one Gen6StagedSession (PR #309 lane: strict SAV6 parse, app-memory staged
// Edit/Clone/Remove, byte-whitelist verifier, VerifiedStagedExport) and exposes it to the ONE
// shared surface. Nothing here writes the original save.

#include "Enums/GameVersion.h"
#include "Integration/Gen6/Gen6StagedSession.h"
#include "Integration/Modern/NativeSession.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen6 {

class Gen6NativeSession final : public Modern::NativeSession {
public:
    // Opens a strictly validated save as a shared session. `assignedExactGame` must match the
    // MyStatus version byte; a mismatch is refused with a diagnostic (never silently retargeted).
    [[nodiscard]] static Modern::OpenedSession open(std::span<const uint8_t> bytes,
                                                    std::string_view assignedExactGame);
    explicit Gen6NativeSession(Gen6Save source);

    [[nodiscard]] Gen6StagedSession& staged() noexcept { return session_; }
    [[nodiscard]] const Gen6StagedSession& staged() const noexcept { return session_; }
    [[nodiscard]] const Gen6Save& source() const noexcept { return session_.source(); }
    [[nodiscard]] Enums::GameVersion gameGroup() const noexcept;

    // ---- NativeSession ----
    std::string_view exactGameId() const noexcept override { return session_.source().exactGameId(); }
    const Capability::GameCapabilities& capabilities() const noexcept override { return *capabilities_; }
    std::string_view familyLabel() const noexcept override { return "Generation VI"; }
    size_t boxCount() const noexcept override { return Layout::BoxCount; }
    size_t slotsPerBox() const noexcept override { return Layout::BoxSlots; }
    size_t partyCount() const noexcept override { return session_.source().partyCount(); }
    std::string boxName(size_t box) const override;
    std::span<const uint8_t> sourceBytes() const noexcept override { return session_.source().bytes(); }

    std::optional<Modern::RecordView> view(Modern::SlotRef slot) const override;
    bool originalOccupied(Modern::SlotRef slot) const override;
    bool emptyDestination(Modern::SlotRef slot) const override;
    bool stagedAt(Modern::SlotRef slot) const override;
    std::optional<Modern::SlotRef> firstEmptyBoxSlot() const override;

    Modern::FieldState fieldState(Modern::Field field, Modern::Region region) const noexcept override;
    bool stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                    std::string& error) override;
    bool stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) override;
    uint32_t fieldMaximum(Modern::Field field) const noexcept override;
    std::vector<uint16_t> heldItemChoices() const override;
    std::vector<uint16_t> ballChoices() const override;
    int maxPP(uint16_t move, uint8_t ppUps) const noexcept override;

    bool beginDraft(Modern::SlotRef slot, std::string& error) override;
    bool draftActive() const noexcept override { return draft_.has_value(); }
    std::optional<Modern::SlotRef> draftSlot() const noexcept override { return draft_; }
    bool draftDirty() const noexcept override;
    void keepDraft() override { draft_.reset(); }
    void discardDraft() override;

    Modern::OperationSupport cloneSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport moveSupport(Modern::SlotRef from) const override;
    Modern::OperationSupport removeSupport(Modern::SlotRef at) const override;
    Modern::OperationSupport createSupport(Modern::SlotRef at) const override;
    bool clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) override;
    bool remove(Modern::SlotRef at, std::string& error) override;
    bool undo(Modern::SlotRef at) override { return session_.undo(toNative(at)); }

    bool bagAvailable() const override;
    std::vector<Modern::BagPocketView> bag() const override;
    std::vector<uint16_t> bagAddChoices(size_t pocket) const override;
    bool bagSetQuantity(size_t pocket, size_t row, uint16_t quantity, std::string& error) override;
    bool bagRemove(size_t pocket, size_t row, std::string& error) override;
    bool bagAdd(size_t pocket, uint16_t itemId, uint16_t quantity, std::string& error) override;
    void discardBag() override;

    std::vector<Modern::TrainerFact> trainerFacts() const override;
    std::string trainerName() const override;
    uint8_t trainerGender() const noexcept override;
    uint16_t trainerId() const noexcept override {
        return PK6::read16(session_.source().bytes(), Layout::MyStatusOffset + 0);
    }
    uint16_t secretId() const noexcept override {
        return PK6::read16(session_.source().bytes(), Layout::MyStatusOffset + 2);
    }
    Modern::DexProgress pokedex() const override;

    bool hasChanges() const noexcept override { return session_.hasChanges(); }
    size_t changeCount() const noexcept override;
    std::vector<Modern::ReviewRow> reviewRows() const override;
    void discardAll() override { session_.clear(); }

    Modern::OperationSupport exportSupport() const override;
    Common::VerifiedExportResult exportWorkingCopy(const Modern::ExportRequest& request) const override;

    [[nodiscard]] static constexpr SlotRef toNative(Modern::SlotRef s) noexcept {
        return s.region == Modern::Region::Party ? SlotRef::party(s.slot) : SlotRef::boxed(s.box, s.slot);
    }
    [[nodiscard]] static constexpr Modern::SlotRef fromNative(SlotRef s) noexcept {
        return s.region == RecordRegion::Party ? Modern::SlotRef::party(s.slot) : Modern::SlotRef::boxed(s.box, s.slot);
    }

private:
    [[nodiscard]] SlotRecord original(Modern::SlotRef slot) const;

    Gen6StagedSession session_;
    const Capability::GameCapabilities* capabilities_ = nullptr;
    std::optional<Modern::SlotRef> draft_;
    Gen6StagedSession::EntrySnapshot draftSnapshot_;
};

} // namespace PokeVault::Integration::Gen6

#endif
