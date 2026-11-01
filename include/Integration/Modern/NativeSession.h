#ifndef POKEBANK_MODERN_NATIVE_SESSION_H
#define POKEBANK_MODERN_NATIVE_SESSION_H

// The ONE backend contract behind the shared PokeBank NX surface for Generations VI-IX.
//
// The shared Trainer/Party/Boxes/Items/Trainer screen, the shared Pokemon editor workspace, Review,
// Discard and Export Copy are written once (src/UI/ModernSharedSurface.inc) against this interface.
// Each title family supplies an adapter (Gen6NativeSession over Gen6StagedSession, Gen7NativeSession
// over Gen7StagedSession, ...) and the capability registry says which workflows the adapter has
// verified. A later generation therefore inherits every shared workflow automatically; it only has
// to implement this contract and declare its own native mechanics.
//
// Nothing here can write an original save: adapters hold an immutable parsed copy plus staged
// app-memory changes, and Export produces a separate verified copy through VerifiedStagedExport.

#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "Trainer/Inventory.h"
#include "UI/ExactFormatEditorProvider.h"
#include "UI/SharedPokemonEditorContract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Modern {

using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using FieldState = PokeBank::UIModel::ExactFormatEditor::FieldState;

enum class Region : uint8_t { Party, Box };

struct SlotRef {
    Region region = Region::Box;
    size_t box = 0;   // zero for Party
    size_t slot = 0;

    [[nodiscard]] static constexpr SlotRef party(size_t s) noexcept { return {Region::Party, 0, s}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t b, size_t s) noexcept { return {Region::Box, b, s}; }
    constexpr bool operator==(const SlotRef& o) const noexcept {
        return region == o.region && box == o.box && slot == o.slot;
    }
    constexpr bool operator!=(const SlotRef& o) const noexcept { return !(*this == o); }
    constexpr bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
};

struct Date { uint16_t year = 0; uint8_t month = 0, day = 0; };

// A fully decoded, generation-neutral view of one record. Adapters fill what the native format
// stores; the shared surface shows only what the registry/provider marks visible. `nativeFacts`
// carries title-specific mechanics (e.g. "Dynamax Lv" or "Awakening HP") as read-only rows.
struct RecordView {
    bool valid = false;
    bool empty = true;
    bool partyRecord = false;
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t gender = 2;          // 0 male, 1 female, 2 genderless
    bool shiny = false;
    bool egg = false;
    bool nicknamed = false;
    bool fatefulEncounter = false;
    uint8_t level = 0;
    uint32_t experience = 0;
    std::string nickname;        // UTF-8
    std::string otName;          // UTF-8
    uint8_t otGender = 0;
    uint16_t tid = 0, sid = 0;
    uint32_t pid = 0, encryptionConstant = 0;
    uint16_t heldItem = 0;
    uint8_t nature = 0, statNature = 0;
    uint16_t ability = 0;
    uint8_t abilityNumber = 0;   // 1/2/4 bit flags as stored natively
    std::array<uint8_t, 6> ivs{}, evs{};      // HP Atk Def Spe SpA SpD
    std::array<uint8_t, 6> baseStats{};
    std::array<uint16_t, 6> stats{};          // computed/cached battle stats
    uint16_t currentHp = 0;
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{}, ppUps{};
    std::array<uint16_t, 4> relearnMoves{};
    uint8_t ball = 0, language = 0, friendship = 0, pokerus = 0, originGame = 0, metLevel = 0;
    uint16_t metLocation = 0, eggLocation = 0;
    Date metDate{}, eggDate{};
    std::array<uint8_t, 2> types{0xFF, 0xFF};   // canonical type ids, 0xFF unknown
    // Hyper Training flags as stored natively (PKHeX IHyperTrain bit order HP0 Atk1 Def2 SpA3 SpD4 Spe5);
    // 0 when none or when the format has no Hyper Training (Gen VI).
    uint8_t hyperTrainFlags = 0;
    std::vector<std::pair<std::string, std::string>> nativeFacts;
};

struct OperationSupport {
    bool available = false;
    std::string_view reason{};   // why not, when !available
};

struct ReviewRow {
    std::string label;
    std::string value;
};

struct BagPocketView {
    std::string name;
    std::vector<Trainer::InventoryItem> items;
    uint16_t maxQuantity = 0;
    bool editable = false;
    std::string reason;          // why not editable
};

struct TrainerFact {
    std::string label;
    std::string value;
};

struct DexProgress {
    uint16_t seen = 0, caught = 0, total = 0;
    [[nodiscard]] bool available() const noexcept { return total != 0; }
};

struct ExportRequest {
    std::string rootDirectory;     // app-owned export root (host fixture or exports/<gen>)
    std::string sourcePath;        // evidence only; never opened by the adapter
    std::string timestamp;         // YYYYMMDD_HHMMSS
    std::string applicationSha = "unknown";
    std::string profileIdentity;
};

class NativeSession {
public:
    virtual ~NativeSession() = default;

    // ---- identity ----
    [[nodiscard]] virtual std::string_view exactGameId() const noexcept = 0;
    [[nodiscard]] virtual const Capability::GameCapabilities& capabilities() const noexcept = 0;
    [[nodiscard]] virtual std::string_view familyLabel() const noexcept = 0;  // "Generation VI"
    [[nodiscard]] virtual size_t boxCount() const noexcept = 0;
    [[nodiscard]] virtual size_t slotsPerBox() const noexcept = 0;
    [[nodiscard]] virtual size_t partyCount() const noexcept = 0;
    [[nodiscard]] virtual std::string boxName(size_t box) const = 0;
    [[nodiscard]] virtual std::span<const uint8_t> sourceBytes() const noexcept = 0;

    // ---- records (staged view: source plus app-memory changes) ----
    [[nodiscard]] virtual std::optional<RecordView> view(SlotRef slot) const = 0;
    [[nodiscard]] virtual bool originalOccupied(SlotRef slot) const = 0;
    [[nodiscard]] virtual bool emptyDestination(SlotRef slot) const = 0;   // verified blank, unstaged
    [[nodiscard]] virtual bool stagedAt(SlotRef slot) const = 0;
    [[nodiscard]] virtual std::optional<SlotRef> firstEmptyBoxSlot() const = 0;

    // ---- shared editor field model ----
    [[nodiscard]] virtual FieldState fieldState(Field field, Region region) const noexcept = 0;
    [[nodiscard]] virtual bool stageValue(SlotRef slot, Field field, uint32_t value, size_t index,
                                          std::string& error) = 0;
    [[nodiscard]] virtual bool stageText(SlotRef slot, Field field, std::string_view utf8,
                                         std::string& error) = 0;
    [[nodiscard]] virtual uint32_t fieldMaximum(Field field) const noexcept = 0;   // e.g. max held item
    [[nodiscard]] virtual std::vector<uint16_t> heldItemChoices() const = 0;
    [[nodiscard]] virtual std::vector<uint16_t> ballChoices() const = 0;
    [[nodiscard]] virtual int maxPP(uint16_t move, uint8_t ppUps) const noexcept = 0;

    // ---- draft bracket for the shared View/Edit workspace ----
    // beginDraft snapshots one slot's staged state; edits inside the bracket go through
    // stageValue/stageText and show in view() at once; keepDraft ends the bracket keeping them;
    // discardDraft restores the snapshot (the Gen I-V "Discard draft" semantics).
    [[nodiscard]] virtual bool beginDraft(SlotRef slot, std::string& error) = 0;
    [[nodiscard]] virtual bool draftActive() const noexcept = 0;
    [[nodiscard]] virtual std::optional<SlotRef> draftSlot() const noexcept = 0;
    [[nodiscard]] virtual bool draftDirty() const noexcept = 0;
    virtual void keepDraft() = 0;
    virtual void discardDraft() = 0;

    // ---- slot operations ----
    [[nodiscard]] virtual OperationSupport cloneSupport(SlotRef from) const = 0;
    [[nodiscard]] virtual OperationSupport moveSupport(SlotRef from) const = 0;
    [[nodiscard]] virtual OperationSupport removeSupport(SlotRef at) const = 0;
    [[nodiscard]] virtual OperationSupport createSupport(SlotRef at) const = 0;
    [[nodiscard]] virtual bool clone(SlotRef from, SlotRef to, std::string& error) = 0;
    [[nodiscard]] virtual bool move(SlotRef from, SlotRef to, std::string& error) = 0;
    [[nodiscard]] virtual bool remove(SlotRef at, std::string& error) = 0;
    [[nodiscard]] virtual bool undo(SlotRef at) = 0;

    // ---- bag ----
    [[nodiscard]] virtual bool bagAvailable() const = 0;
    [[nodiscard]] virtual std::vector<BagPocketView> bag() const = 0;
    [[nodiscard]] virtual std::vector<uint16_t> bagAddChoices(size_t pocket) const = 0;
    [[nodiscard]] virtual bool bagSetQuantity(size_t pocket, size_t row, uint16_t quantity, std::string& error) = 0;
    [[nodiscard]] virtual bool bagRemove(size_t pocket, size_t row, std::string& error) = 0;
    [[nodiscard]] virtual bool bagAdd(size_t pocket, uint16_t itemId, uint16_t quantity, std::string& error) = 0;
    virtual void discardBag() = 0;

    // ---- trainer / pokedex (read-only facts; verified writers are adapter-specific) ----
    [[nodiscard]] virtual std::vector<TrainerFact> trainerFacts() const = 0;
    [[nodiscard]] virtual std::string trainerName() const = 0;
    [[nodiscard]] virtual uint8_t trainerGender() const noexcept = 0;
    [[nodiscard]] virtual uint16_t trainerId() const noexcept = 0;
    [[nodiscard]] virtual uint16_t secretId() const noexcept = 0;
    [[nodiscard]] virtual DexProgress pokedex() const = 0;

    // ---- staging lifecycle ----
    [[nodiscard]] virtual bool hasChanges() const noexcept = 0;
    [[nodiscard]] virtual size_t changeCount() const noexcept = 0;
    [[nodiscard]] virtual std::vector<ReviewRow> reviewRows() const = 0;
    virtual void discardAll() = 0;

    // ---- export (separate verified copy; never the source) ----
    [[nodiscard]] virtual OperationSupport exportSupport() const = 0;
    [[nodiscard]] virtual Common::VerifiedExportResult exportWorkingCopy(const ExportRequest& request) const = 0;
};

// A validated, immutable parse result wrapped as a session factory. Families register one of these
// through ModernFamilies (Integration/Modern/ModernFamilies.h).
struct OpenedSession {
    std::unique_ptr<NativeSession> session;
    std::string diagnostic;
    [[nodiscard]] bool ready() const noexcept { return session != nullptr; }
};

} // namespace PokeVault::Integration::Modern

#endif
