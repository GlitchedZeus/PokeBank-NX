#include "Integration/Gen2/Gen2StagedEditor.h"

#include <algorithm>
#include <array>

namespace PokeVault::Integration::Gen2 {
namespace {

struct PackedLayout {
    std::size_t currentBoxCopy;
    uint8_t boxCapacity;
    uint8_t boxCount;
    uint8_t stringLength;
    uint8_t splitAtBox;
};

constexpr PackedLayout kGoldSilverLayout{0x2D6C, 20, 14, 11, 7};
constexpr PackedLayout kCrystalLayout{0x2D10, 20, 14, 11, 7};
constexpr std::size_t kStoredBodySize = 32;

const PackedLayout& packedLayoutFor(VersionFamily family) noexcept {
    return family == VersionFamily::Crystal ? kCrystalLayout : kGoldSilverLayout;
}

std::size_t boxListLength(const PackedLayout& layout) noexcept {
    return 1 + (static_cast<std::size_t>(layout.boxCapacity) + 1) +
           static_cast<std::size_t>(layout.boxCapacity) * kStoredBodySize +
           2 * static_cast<std::size_t>(layout.boxCapacity) * layout.stringLength;
}

std::size_t boxStride(const PackedLayout& layout) noexcept {
    return boxListLength(layout) + 2;
}

std::size_t boxStart(const PackedLayout& layout, std::size_t box) noexcept {
    if (box < layout.splitAtBox) return 0x4000 + box * boxStride(layout);
    return 0x6000 + (box - layout.splitAtBox) * boxStride(layout);
}

std::size_t boxBodyStart(const PackedLayout& layout, std::size_t box) noexcept {
    return boxStart(layout, box) + 1 + (static_cast<std::size_t>(layout.boxCapacity) + 1);
}

std::size_t boxOTStart(const PackedLayout& layout, std::size_t box) noexcept {
    return boxBodyStart(layout, box) +
           static_cast<std::size_t>(layout.boxCapacity) * kStoredBodySize;
}

std::size_t boxNicknameStart(const PackedLayout& layout, std::size_t box) noexcept {
    return boxOTStart(layout, box) +
           static_cast<std::size_t>(layout.boxCapacity) * layout.stringLength;
}

void copyStoredSlot(const std::vector<uint8_t>& from, const PackedLayout& layout,
                    std::size_t fromBox, std::size_t fromSlot,
                    std::vector<uint8_t>& to, std::size_t toBox, std::size_t toSlot) {
    const auto fromList = boxStart(layout, fromBox);
    const auto toList = boxStart(layout, toBox);
    to[toList + 1 + toSlot] = from[fromList + 1 + fromSlot];

    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(
                    boxBodyStart(layout, fromBox) + fromSlot * kStoredBodySize),
                kStoredBodySize,
                to.begin() + static_cast<std::ptrdiff_t>(
                    boxBodyStart(layout, toBox) + toSlot * kStoredBodySize));
    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(
                    boxOTStart(layout, fromBox) + fromSlot * layout.stringLength),
                layout.stringLength,
                to.begin() + static_cast<std::ptrdiff_t>(
                    boxOTStart(layout, toBox) + toSlot * layout.stringLength));
    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(
                    boxNicknameStart(layout, fromBox) + fromSlot * layout.stringLength),
                layout.stringLength,
                to.begin() + static_cast<std::ptrdiff_t>(
                    boxNicknameStart(layout, toBox) + toSlot * layout.stringLength));
}

void clearStoredSlot(std::vector<uint8_t>& bytes, const PackedLayout& layout,
                     std::size_t box, std::size_t slot) {
    const auto list = boxStart(layout, box);
    bytes[list + 1 + slot] = 0;
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(
                    boxBodyStart(layout, box) + slot * kStoredBodySize),
                kStoredBodySize, 0);
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(
                    boxOTStart(layout, box) + slot * layout.stringLength),
                layout.stringLength, 0x50);
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(
                    boxNicknameStart(layout, box) + slot * layout.stringLength),
                layout.stringLength, 0x50);
}

bool sameStoredPokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return a.species == b.species &&
           a.heldItem == b.heldItem &&
           a.moves == b.moves &&
           a.trainerId == b.trainerId &&
           a.experience == b.experience &&
           a.statExperience == b.statExperience &&
           a.dvs == b.dvs &&
           a.pp == b.pp &&
           a.ppUps == b.ppUps &&
           a.friendship == b.friendship &&
           a.pokerus == b.pokerus &&
           a.caughtData == b.caughtData &&
           a.level == b.level &&
           a.originalTrainer == b.originalTrainer &&
           a.nickname == b.nickname &&
           a.isEgg == b.isEgg &&
           a.rawBodySize == b.rawBodySize &&
           a.rawBody == b.rawBody;
}

} // namespace

bool StagedEditor::beginPackedMove(std::size_t sourceBox, std::size_t sourceSlot,
                                   std::string& error) {
    const std::array<std::size_t, 1> slots{sourceSlot};
    return beginPackedGroupMove(sourceBox, slots, error);
}

bool StagedEditor::beginPackedGroupMove(std::size_t sourceBox,
                                        std::span<const std::size_t> sourceSlots,
                                        std::string& error) {
    error.clear();
    if (packedMove_.active) {
        error = "A Generation II Pokemon group is already being carried";
        return false;
    }

    const auto& layout = packedLayoutFor(metadata_.family);
    if (sourceBox >= layout.boxCount || sourceBox >= metadata_.boxCount || sourceSlots.empty()) {
        error = "Generation II move selection is empty or out of range";
        return false;
    }

    std::vector<std::size_t> ordered(sourceSlots.begin(), sourceSlots.end());
    std::sort(ordered.begin(), ordered.end());
    if (ordered.back() >= layout.boxCapacity) {
        error = "Generation II move source is out of range";
        return false;
    }
    if (std::adjacent_find(ordered.begin(), ordered.end()) != ordered.end()) {
        error = "Generation II move selection contains duplicate slots";
        return false;
    }

    const auto list = boxStart(layout, sourceBox);
    const std::size_t count = staged_[list];
    if (count == 0 || ordered.back() >= count) {
        error = "Generation II move selection contains an empty slot";
        return false;
    }

    PackedMoveState state;
    state.active = true;
    state.sourceBox = sourceBox;
    state.sourceSlots = ordered;
    state.stagedBefore = staged_;
    state.changesBefore = changes_;
    state.expectationsBefore = pokemonExpectations_;
    state.carried.reserve(ordered.size());
    for (const auto slot : ordered) {
        auto pokemon = boxedPokemon(sourceBox, slot, error);
        if (!pokemon) return false;
        state.carried.push_back(*pokemon);
    }
    packedMove_ = std::move(state);

    // Compact the source in one transaction while preserving the exact stored bytes of every
    // survivor. Selection order is row/slot order, independent of the direction the rectangle grew.
    const auto beforeRemove = staged_;
    std::size_t write = 0;
    for (std::size_t read = 0; read < count; ++read) {
        if (std::binary_search(ordered.begin(), ordered.end(), read)) continue;
        if (write != read)
            copyStoredSlot(beforeRemove, layout, sourceBox, read, staged_, sourceBox, write);
        ++write;
    }
    for (std::size_t slot = write; slot < count; ++slot)
        clearStoredSlot(staged_, layout, sourceBox, slot);
    staged_[list] = static_cast<uint8_t>(write);
    staged_[list + 1 + write] = 0xFF;

    if (!syncCurrentBoxCopy(sourceBox, error)) {
        staged_ = packedMove_.stagedBefore;
        changes_ = packedMove_.changesBefore;
        pokemonExpectations_ = packedMove_.expectationsBefore;
        packedMove_ = {};
        return false;
    }

    pokemonExpectations_.erase(
        std::remove_if(pokemonExpectations_.begin(), pokemonExpectations_.end(),
            [&](PokemonExpectation& expectation) {
                if (expectation.box != sourceBox) return false;
                if (std::binary_search(ordered.begin(), ordered.end(), expectation.slot)) return true;
                const auto before = std::lower_bound(ordered.begin(), ordered.end(), expectation.slot);
                expectation.slot -= static_cast<std::size_t>(before - ordered.begin());
                return false;
            }),
        pokemonExpectations_.end());
    return true;
}

bool StagedEditor::placePackedMove(std::size_t destinationBox,
                                   std::size_t destinationSlot,
                                   std::size_t& placedSlot,
                                   std::string& error) {
    return placePackedGroupMove(destinationBox, destinationSlot, placedSlot, error);
}

bool StagedEditor::placePackedGroupMove(std::size_t destinationBox,
                                        std::size_t destinationSlot,
                                        std::size_t& firstPlacedSlot,
                                        std::string& error) {
    error.clear();
    firstPlacedSlot = 0;
    if (!packedMove_.active || packedMove_.sourceSlots.empty()) {
        error = "No Generation II Pokemon group is being carried";
        return false;
    }

    const auto& layout = packedLayoutFor(metadata_.family);
    if (destinationBox >= layout.boxCount || destinationBox >= metadata_.boxCount ||
        destinationSlot >= layout.boxCapacity) {
        error = "Generation II move destination is out of range";
        return false;
    }

    const auto list = boxStart(layout, destinationBox);
    const std::size_t count = staged_[list];
    const std::size_t groupSize = packedMove_.sourceSlots.size();
    if (count + groupSize > layout.boxCapacity) {
        error = "This Generation II box does not have enough room for the selected Pokemon";
        return false;
    }

    const std::size_t insert = std::min(destinationSlot, count);
    const auto stagedBeforePlace = staged_;
    const auto expectationsBeforePlace = pokemonExpectations_;

    for (std::size_t slot = count; slot > insert; --slot) {
        const std::size_t from = slot - 1;
        copyStoredSlot(stagedBeforePlace, layout, destinationBox, from,
                       staged_, destinationBox, from + groupSize);
    }
    for (std::size_t i = 0; i < groupSize; ++i) {
        copyStoredSlot(packedMove_.stagedBefore, layout,
                       packedMove_.sourceBox, packedMove_.sourceSlots[i],
                       staged_, destinationBox, insert + i);
    }

    const std::size_t newCount = count + groupSize;
    staged_[list] = static_cast<uint8_t>(newCount);
    staged_[list + 1 + newCount] = 0xFF;

    if (!syncCurrentBoxCopy(destinationBox, error)) {
        staged_ = stagedBeforePlace;
        pokemonExpectations_ = expectationsBeforePlace;
        return false;
    }

    for (std::size_t i = 0; i < groupSize; ++i) {
        auto verified = boxedPokemon(destinationBox, insert + i, error);
        if (!verified || !sameStoredPokemon(*verified, packedMove_.carried[i])) {
            if (error.empty()) error = "Moved Generation II Pokemon group failed exact staged round-trip";
            staged_ = stagedBeforePlace;
            pokemonExpectations_ = expectationsBeforePlace;
            return false;
        }
    }

    for (auto& expectation : pokemonExpectations_) {
        if (expectation.box == destinationBox && expectation.slot >= insert)
            expectation.slot += groupSize;
    }
    for (std::size_t i = 0; i < groupSize; ++i) {
        auto verified = boxedPokemon(destinationBox, insert + i, error);
        if (!verified) {
            staged_ = stagedBeforePlace;
            pokemonExpectations_ = expectationsBeforePlace;
            return false;
        }
        rememberPokemonExpectation(destinationBox, insert + i, *verified);
    }

    setChange(
        "packed_group_move:" + std::to_string(packedMove_.sourceBox) + ":" +
            std::to_string(packedMove_.sourceSlots.front()) + ":" + std::to_string(groupSize) + "->" +
            std::to_string(destinationBox) + ":" + std::to_string(insert),
        groupSize == 1 ? "Packed Pokemon move" : "Packed Pokemon group move",
        "Box " + std::to_string(packedMove_.sourceBox + 1) + " / " + std::to_string(groupSize) + " Pokemon",
        "Box " + std::to_string(destinationBox + 1) + " Slot " + std::to_string(insert + 1));

    firstPlacedSlot = insert;
    packedMove_ = {};
    return true;
}

bool StagedEditor::cancelPackedMove(std::string& error) {
    error.clear();
    if (!packedMove_.active) return true;

    staged_ = packedMove_.stagedBefore;
    changes_ = packedMove_.changesBefore;
    pokemonExpectations_ = packedMove_.expectationsBefore;
    packedMove_ = {};
    return true;
}

bool StagedEditor::stageReleaseBoxPokemon(std::size_t box, std::size_t slot,
                                          std::string& error) {
    error.clear();
    if (packedMove_.active) {
        error = "Finish or cancel the current Pokemon move before Release";
        return false;
    }
    auto pokemon = boxedPokemon(box, slot, error);
    if (!pokemon) return false;
    if (!beginPackedMove(box, slot, error)) return false;

    const std::string before = pokemon->nickname + " Lv. " + std::to_string(pokemon->level);
    packedMove_ = {}; // The packed removal is now the staged Release transaction.
    setChange("release:" + std::to_string(box) + ":" + std::to_string(slot),
              "Box " + std::to_string(box + 1) + " Slot " + std::to_string(slot + 1) + " Release",
              before, "Released (staged only)");
    return true;
}

} // namespace PokeVault::Integration::Gen2
