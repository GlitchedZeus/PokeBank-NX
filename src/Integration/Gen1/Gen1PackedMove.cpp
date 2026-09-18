#include "Integration/Gen1/Gen1StagedPokemonEditor.h"

#include <algorithm>

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
    error.clear();
    if (packedMove_.active) {
        error = "A Generation I Pokemon is already being carried";
        return false;
    }
    if (sourceBox >= 12 || sourceSlot >= kCapacity) {
        error = "Generation I move source is out of range";
        return false;
    }

    const auto source = boxedPokemon(sourceBox, sourceSlot, error);
    if (!source) {
        if (error.empty()) error = "Generation I move source slot is empty";
        return false;
    }

    PackedMoveState state;
    state.active = true;
    state.sourceBox = sourceBox;
    state.sourceSlot = sourceSlot;
    state.beforeBytes.assign(stagedBytes().begin(), stagedBytes().end());
    packedMove_ = std::move(state);

    if (!stageRemove(sourceBox, sourceSlot, error)) {
        packedMove_ = {};
        return false;
    }
    return true;
}

bool StagedPokemonEditor::placePackedMove(std::size_t destinationBox,
                                          std::size_t destinationSlot,
                                          std::size_t& placedSlot,
                                          std::string& error) {
    error.clear();
    placedSlot = 0;
    if (!packedMove_.active) {
        error = "No Generation I Pokemon is being carried";
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
    if (count >= kCapacity) {
        error = "This Generation I box is full";
        return false;
    }

    const std::size_t insert = std::min(destinationSlot, count);
    const auto beforeInsert = bytes;
    for (std::size_t slot = count; slot > insert; --slot)
        copySlot(beforeInsert, destinationOffset, slot - 1, bytes, destinationOffset, slot);

    const auto sourceOffset = boxOffset(metadata(), packedMove_.sourceBox);
    copySlot(packedMove_.beforeBytes, sourceOffset, packedMove_.sourceSlot,
             bytes, destinationOffset, insert);

    bytes[destinationOffset] = static_cast<uint8_t>(count + 1);
    bytes[destinationOffset + 1 + count + 1] = 0xFF;

    if (destinationBox == metadata().currentBox && (bytes[0x284C] & 0x80))
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(kCurrentBox), kBoxSize,
                    bytes.begin() + static_cast<std::ptrdiff_t>(bankOffset(destinationBox)));

    if (!commit(std::move(bytes), error)) return false;

    placedSlot = insert;
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
