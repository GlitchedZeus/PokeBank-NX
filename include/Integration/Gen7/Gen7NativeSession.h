#ifndef POKEBANK_GEN7_NATIVE_SESSION_H
#define POKEBANK_GEN7_NATIVE_SESSION_H

// Gen VII (Sun / Moon / Ultra Sun / Ultra Moon) adapter for the shared Modern::NativeSession
// contract. It owns one Gen7StagedSession (PR #311 lane: strict signature-verified SAV7 parse,
// app-memory staged Edit/Clone/Remove, unsigned-candidate byte-whitelist verifier) and exposes it
// to the ONE shared surface exactly as Gen6NativeSession does. SAV7 export is Blocked by the
// capability registry (the MemeCrypto re-sign payload is unproven on a game-written save, see
// docs/gen7/README.md): exportSupport() repeats the registry reason and exportWorkingCopy() never
// writes a file. Nothing here writes the original save.

#include "Enums/GameVersion.h"
#include "Integration/Gen7/Gen7StagedSession.h"
#include "Integration/Modern/NativeSession.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7 {

class Gen7NativeSession final : public Modern::NativeSession {
public:
    // Opens a strictly validated, signature-verified save as a shared session. `assignedExactGame`
    // must match the MyStatus version byte; a mismatch is refused with a diagnostic (never
    // silently retargeted).
    [[nodiscard]] static Modern::OpenedSession open(std::span<const uint8_t> bytes,
                                                    std::string_view assignedExactGame);
    explicit Gen7NativeSession(Gen7Save source);

    [[nodiscard]] Gen7StagedSession& staged() noexcept { return session_; }
    [[nodiscard]] const Gen7StagedSession& staged() const noexcept { return session_; }
    [[nodiscard]] const Gen7Save& source() const noexcept { return session_.source(); }
    [[nodiscard]] Enums::GameVersion gameGroup() const noexcept;

    // ---- NativeSession ----
    std::string_view exactGameId() const noexcept override { return session_.source().exactGameId(); }
    const Capability::GameCapabilities& capabilities() const noexcept override { return *capabilities_; }
    std::string_view familyLabel() const noexcept override { return "Generation VII"; }
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
    // MyStatus7 TID at +0, SID at +2 (PKHeX 6501f0ab Saves/Substructures/Gen7/MyStatus7.cs; the
    // same offsets Gen7Save::trainer() reads).
    uint16_t trainerId() const noexcept override { return PK7::read16(session_.source().bytes(), myStatusOffset() + 0); }
    uint16_t secretId() const noexcept override { return PK7::read16(session_.source().bytes(), myStatusOffset() + 2); }
    Modern::DexProgress pokedex() const override;

    bool hasChanges() const noexcept override { return session_.hasChanges(); }
    size_t changeCount() const noexcept override;
    std::vector<Modern::ReviewRow> reviewRows() const override;
    void discardAll() override;

    // Always unavailable for Gen VII: the reason is the registry's Export status reason and the
    // export call writes nothing (no directory, no file) before failing with that same text.
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
    [[nodiscard]] size_t myStatusOffset() const noexcept {
        return Layout::blocks(session_.source().family())[Layout::MyStatusBlock].offset;
    }

    Gen7StagedSession session_;
    const Capability::GameCapabilities* capabilities_ = nullptr;
    std::optional<Modern::SlotRef> draft_;
    Gen7StagedSession::EntrySnapshot draftSnapshot_;
};

} // namespace PokeVault::Integration::Gen7

#endif
