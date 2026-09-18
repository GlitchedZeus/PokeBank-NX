#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <algorithm>
#include <array>

namespace PokeVault::Integration::Gen1 {
namespace {

constexpr std::size_t kCapacity = 20;
constexpr std::size_t kBoxSize = 0x462;
constexpr std::size_t kCurrentBox = 0x30C0;

std::size_t bankOffset(std::size_t box) {
    return (box < 6 ? 0x4000 : 0x6000) + (box % 6) * kBoxSize;
}

std::size_t boxOffset(const Metadata& metadata, std::size_t box) {
    return box == metadata.currentBox ? kCurrentBox : bankOffset(box);
}

std::size_t bodyOffset(std::size_t boxOffsetValue, std::size_t slot) {
    return boxOffsetValue + 22 + slot * 33;
}

std::size_t otOffset(std::size_t boxOffsetValue, std::size_t slot) {
    return boxOffsetValue + 22 + 20 * 33 + slot * 11;
}

std::size_t nicknameOffset(std::size_t boxOffsetValue, std::size_t slot) {
    return boxOffsetValue + 22 + 20 * 44 + slot * 11;
}

void copySlot(const std::vector<uint8_t>& from, std::size_t fromBox, std::size_t fromSlot,
              std::vector<uint8_t>& to, std::size_t toBox, std::size_t toSlot) {
    to[toBox + 1 + toSlot] = from[fromBox + 1 + fromSlot];
    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(bodyOffset(fromBox, fromSlot)),
                33, to.begin() + static_cast<std::ptrdiff_t>(bodyOffset(toBox, toSlot)));
    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(otOffset(fromBox, fromSlot)),
                11, to.begin() + static_cast<std::ptrdiff_t>(otOffset(toBox, toSlot)));
    std::copy_n(from.begin() + static_cast<std::ptrdiff_t>(nicknameOffset(fromBox, fromSlot)),
                11, to.begin() + static_cast<std::ptrdiff_t>(nicknameOffset(toBox, toSlot)));
}

void initializeBanks(std::vector<uint8_t>& bytes, const Metadata& metadata) {
    if (bytes[0x284C] & 0x80) return;
    for (std::size_t box = 0; box < 12; ++box) {
        const auto offset = bankOffset(box);
        std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), kBoxSize, 0);
        bytes[offset + 1] = 0xFF;
    }
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(kCurrentBox), kBoxSize,
                bytes.begin() + static_cast<std::ptrdiff_t>(bankOffset(metadata.currentBox)));
    bytes[0x284C] |= 0x80;
}

} // namespace

bool StagedPokemonEditor::beginPackedMove(std::size_t sourceBox, std::size_t sourceSlot,
                                          std::string& error) {
    const std::array<std::size_t, 1> slots{sourceSlot};
    return beginPackedGroupMove(sourceBox, slots, error);
}

bool StagedPokemonEditor::beginPackedGroupMove(std::size_t sourceBox,
                                               std::span<const std::size_t> sourceSlots,
                                               std::string& error) {
    error.clear();
    if (packedMove_.active) {
        error = "A Generation I Pokemon group is already being carried";
        return false;
    }
    if (sourceBox >= 12 || sourceSlots.empty()) {
        error = "Generation I move selection is empty or out of range";
        return false;
    }

    std::vector<std::size_t> ordered(sourceSlots.begin(), sourceSlots.end());
    std::sort(ordered.begin(), ordered.end());
    if (ordered.back() >= kCapacity) {
        error = "Generation I move source is out of range";
        return false;
    }
    if (std::adjacent_find(ordered.begin(), ordered.end()) != ordered.end()) {
        error = "Generation I move selection contains duplicate slots";
        return false;
    }

    for (const auto slot : ordered) {
        std::string slotError;
        if (!boxedPokemon(sourceBox, slot, slotError)) {
            error = slotError.empty() ? "Generation I move selection contains an empty slot" : slotError;
            return false;
        }
    }

    PackedMoveState state;
    state.active = true;
    state.sourceBox = sourceBox;
    state.sourceSlots = ordered;
    state.beforeBytes.assign(stagedBytes().begin(), stagedBytes().end());
    packedMove_ = std::move(state);

    // Remove from the highest source index downward. Each proven Part 1 remove compacts the
    // staged box, so lower selected coordinates stay stable until their turn.
    for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
        if (!stageRemove(sourceBox, *it, error)) {
            auto restore = packedMove_.beforeBytes;
            std::string restoreError;
            commit(std::move(restore), restoreError);
            packedMove_ = {};
            if (error.empty()) error = restoreError.empty() ? "Generation I group pickup failed" : restoreError;
            return false;
        }
    }
    return true;
}

bool StagedPokemonEditor::placePackedMove(std::size_t destinationBox,
                                          std::size_t destinationSlot,
                                          std::size_t& placedSlot,
                                          std::string& error) {
    return placePackedGroupMove(destinationBox, destinationSlot, placedSlot, error);
}

bool StagedPokemonEditor::placePackedGroupMove(std::size_t destinationBox,
                                               std::size_t destinationSlot,
                                               std::size_t& firstPlacedSlot,
                                               std::string& error) {
    error.clear();
    firstPlacedSlot = 0;
    if (!packedMove_.active || packedMove_.sourceSlots.empty()) {
        error = "No Generation I Pokemon group is being carried";
        return false;
    }
    if (destinationBox >= 12 || destinationSlot >= kCapacity) {
        error = "Generation I move destination is out of range";
        return false;
    }

    std::vector<uint8_t> bytes(stagedBytes().begin(), stagedBytes().end());
    if (!(bytes[0x284C] & 0x80) && destinationBox != metadata().currentBox)
        initializeBanks(bytes, metadata());

    const auto destinationOffset = boxOffset(metadata(), destinationBox);
    const std::size_t count = bytes[destinationOffset];
    const std::size_t groupSize = packedMove_.sourceSlots.size();
    if (count + groupSize > kCapacity) {
        error = "This Generation I box does not have enough room for the selected Pokemon";
        return false;
    }

    const std::size_t insert = std::min(destinationSlot, count);
    const auto beforeInsert = bytes;
    for (std::size_t slot = count; slot > insert; --slot) {
        const std::size_t from = slot - 1;
        copySlot(beforeInsert, destinationOffset, from,
                 bytes, destinationOffset, from + groupSize);
    }

    const auto sourceOffset = boxOffset(metadata(), packedMove_.sourceBox);
    for (std::size_t i = 0; i < groupSize; ++i) {
        copySlot(packedMove_.beforeBytes, sourceOffset, packedMove_.sourceSlots[i],
                 bytes, destinationOffset, insert + i);
    }

    const std::size_t newCount = count + groupSize;
    bytes[destinationOffset] = static_cast<uint8_t>(newCount);
    bytes[destinationOffset + 1 + newCount] = 0xFF;

    if (destinationBox == metadata().currentBox && (bytes[0x284C] & 0x80))
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(kCurrentBox), kBoxSize,
                    bytes.begin() + static_cast<std::ptrdiff_t>(bankOffset(destinationBox)));

    if (!commit(std::move(bytes), error)) return false;

    firstPlacedSlot = insert;
    packedMove_ = {};
    return true;
}

bool StagedPokemonEditor::cancelPackedMove(std::string& error) {
    error.clear();
    if (!packedMove_.active) return true;

    auto restore = packedMove_.beforeBytes;
    if (!commit(std::move(restore), error)) return false;
    packedMove_ = {};
    return true;
}

} // namespace PokeVault::Integration::Gen1
