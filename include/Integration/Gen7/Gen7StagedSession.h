#ifndef POKEBANK_GEN7_STAGED_SESSION_H
#define POKEBANK_GEN7_STAGED_SESSION_H

#include "Integration/Gen7/Gen7Bag.h"
#include "Integration/Gen7/Gen7ExactFormatEditorProvider.h"
#include "Integration/Gen7/Gen7Save.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7 {

using RecordRegion = Gen7EditorProvider::RecordRegion;
using Field = Gen7EditorProvider::Field;

struct SlotRef {
    RecordRegion region = RecordRegion::Box;
    size_t box = 0;
    size_t slot = 0;
    [[nodiscard]] static constexpr SlotRef party(size_t s) noexcept { return {RecordRegion::Party, 0, s}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t b, size_t s) noexcept { return {RecordRegion::Box, b, s}; }
    [[nodiscard]] constexpr bool canonical() const noexcept {
        return region == RecordRegion::Party ? (box == 0 && slot < Layout::PartySlots)
                                             : (box < Layout::BoxCount && slot < Layout::BoxSlots);
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept = default;
};

enum class ChangeKind : uint8_t { Edit, Clone, Remove };

struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain;
    std::vector<uint8_t> stagedPlain;  // checksum refreshed
    std::vector<std::string> notes;
};

// Everything up to (but excluding) the MemeCrypto re-sign: staged records written,
// owning block CRCs refreshed, the old (now stale) signature left in place.
struct UnsignedCandidate {
    std::vector<uint8_t> bytes;
    bool signaturePending = true;
};

// Staged PK7 Edit / Clone / Remove on an app-owned copy of a signature-verified
// SAV7. The source Gen7Save is never mutated. There is NO export: SAV7 output is
// explicitly unsupported until the re-signing construction is confirmed.
class Gen7StagedSession {
public:
    explicit Gen7StagedSession(Gen7Save source) : source_(std::move(source)) {}

    [[nodiscard]] const Gen7Save& source() const noexcept { return source_; }
    [[nodiscard]] Gen7EditorProvider::ValueLimits limits() const noexcept {
        return Gen7EditorProvider::limitsFor(source_.family());
    }

    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, size_t index = 0,
                                  std::string* error = nullptr);
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text, std::string* error = nullptr);
    [[nodiscard]] bool cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error = nullptr);
    [[nodiscard]] bool removeFromBox(size_t box, size_t slot, std::string* error = nullptr);

    bool undo(SlotRef slot) noexcept { return entries_.erase(slot) != 0; }
    [[nodiscard]] bool hasChanges() const noexcept { return !entries_.empty() || bagDirty(); }

    // Staged native Bag (MyItem block 0); nullptr (with error) when unrecognized.
    [[nodiscard]] Gen7StagedBag* bag(std::string* error = nullptr) {
        if (!bag_) bag_ = Gen7StagedBag::create(source_, error);
        return bag_ ? &*bag_ : nullptr;
    }
    [[nodiscard]] bool bagDirty() const noexcept { return bag_ && bag_->dirty(); }
    [[nodiscard]] std::span<const uint8_t> stagedBagBlock() const noexcept {
        return bagDirty() ? bag_->staged() : std::span<const uint8_t>{};
    }
    [[nodiscard]] std::vector<StagedChange> changes() const;
    [[nodiscard]] std::vector<std::string> changeLines() const;
    [[nodiscard]] std::optional<PK7::Pokemon6> current(SlotRef slot) const;

    [[nodiscard]] std::optional<UnsignedCandidate> buildUnsignedCandidate(std::string* error = nullptr) const;

    // Always refuses: no SAV7 working copy may be published without a confirmed signature.
    [[nodiscard]] static std::string_view exportUnsupportedReason() noexcept {
        return "Gen VII export is unsupported: the MemeCrypto re-sign payload (PKHeX zero tail vs "
               "PKSM-Core reverseCrypt tail) is not yet confirmed on a game-written save";
    }

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK7::Pokemon6 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    [[nodiscard]] SlotRecord original(SlotRef slot) const;
    void commitEdit(SlotRef slot, PK7::Pokemon6 next, std::string note);

    Gen7Save source_;
    std::map<SlotRef, Entry> entries_;
    std::optional<Gen7StagedBag> bag_;
};

// Independent check of an unsigned candidate: same size, reparses (signature
// ignored, block CRCs strict) as the same exact game with the same party count,
// and differs from the source only inside staged records (decrypted diff within
// the editable-field mask, or exact clone/blank) and the CRCs of owning blocks.
// The signature region must be untouched.
// With a non-empty `stagedBagBlock`, MyItem must equal it, decode strictly and differ
// only inside pouch slot arrays (plus block 0's CRC); otherwise the bag must be identical.
[[nodiscard]] bool verifyUnsignedCandidate(const Gen7Save& source, std::span<const uint8_t> edited,
                                           const std::vector<StagedChange>& changes, std::string* error = nullptr,
                                           std::span<const uint8_t> stagedBagBlock = {});

} // namespace PokeVault::Integration::Gen7

#endif
