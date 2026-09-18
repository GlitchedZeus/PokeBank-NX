#include "Integration/Gen2/Gen2StagedEditor.h"

#include <algorithm>

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
    error.clear();
    if (packedMove_.active) {
        error = "A Generation II Pokemon is already being carried";
        return false;
    }

    const auto& layout = packedLayoutFor(metadata_.family);
    if (sourceBox >= layout.boxCount || sourceBox >= metadata_.boxCount ||
        sourceSlot >= layout.boxCapacity) {
        error = "Generation II move source is out of range";
        return false;
    }

    auto source = boxedPokemon(sourceBox, sourceSlot, error);
    if (!source) {
        if (error.empty()) error = "Generation II move source slot is empty";
        return false;
    }

    const auto list = boxStart(layout, sourceBox);
    const std::size_t count = staged_[list];
    if (count == 0 || sourceSlot >= count) {
        error = "Generation II move source slot is empty";
        return false;
    }

    PackedMoveState state;
    state.active = true;
    state.sourceBox = sourceBox;
    state.sourceSlot = sourceSlot;
    state.carried = *source;
    state.stagedBefore = staged_;
    state.changesBefore = changes_;
    state.expectationsBefore = pokemonExpectations_;
    packedMove_ = std::move(state);

    const auto beforeRemove = staged_;
    for (std::size_t slot = sourceSlot; slot + 1 < count; ++slot)
        copyStoredSlot(beforeRemove, layout, sourceBox, slot + 1, staged_, sourceBox, slot);

    const std::size_t last = count - 1;
    clearStoredSlot(staged_, layout, sourceBox, last);
    staged_[list] = static_cast<uint8_t>(last);
    staged_[list + 1 + last] = 0xFF;

    if (!syncCurrentBoxCopy(sourceBox, error)) {
        staged_ = packedMove_.stagedBefore;
        changes_ = packedMove_.changesBefore;
        pokemonExpectations_ = packedMove_.expectationsBefore;
        packedMove_ = {};
        return false;
    }

    // Expectations describe staged coordinates. Compaction shifts every record after the
    // picked slot one position left; the carried record has no staged coordinate until drop.
    pokemonExpectations_.erase(
        std::remove_if(pokemonExpectations_.begin(), pokemonExpectations_.end(),
            [&](PokemonExpectation& expectation) {
                if (expectation.box != sourceBox) return false;
                if (expectation.slot == sourceSlot) return true;
                if (expectation.slot > sourceSlot) --expectation.slot;
                return false;
            }),
        pokemonExpectations_.end());

    return true;
}

bool StagedEditor::placePackedMove(std::size_t destinationBox,
                                   std::size_t destinationSlot,
                                   std::size_t& placedSlot,
                                   std::string& error) {
    error.clear();
    placedSlot = 0;
    if (!packedMove_.active) {
        error = "No Generation II Pokemon is being carried";
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
    if (count >= layout.boxCapacity) {
        error = "This Generation II box is full";
        return false;
    }

    const std::size_t insert = std::min(destinationSlot, count);
    const auto stagedBeforePlace = staged_;
    const auto expectationsBeforePlace = pokemonExpectations_;

    for (std::size_t slot = count; slot > insert; --slot)
        copyStoredSlot(stagedBeforePlace, layout, destinationBox, slot - 1,
                       staged_, destinationBox, slot);

    copyStoredSlot(packedMove_.stagedBefore, layout,
                   packedMove_.sourceBox, packedMove_.sourceSlot,
                   staged_, destinationBox, insert);
    staged_[list] = static_cast<uint8_t>(count + 1);
    staged_[list + 1 + count + 1] = 0xFF;

    if (!syncCurrentBoxCopy(destinationBox, error)) {
        staged_ = stagedBeforePlace;
        pokemonExpectations_ = expectationsBeforePlace;
        return false;
    }

    auto verified = boxedPokemon(destinationBox, insert, error);
    if (!verified || !sameStoredPokemon(*verified, packedMove_.carried)) {
        if (error.empty()) error = "Moved Generation II Pokemon failed exact staged round-trip";
        staged_ = stagedBeforePlace;
        pokemonExpectations_ = expectationsBeforePlace;
        return false;
    }

    for (auto& expectation : pokemonExpectations_) {
        if (expectation.box == destinationBox && expectation.slot >= insert)
            ++expectation.slot;
    }
    rememberPokemonExpectation(destinationBox, insert, *verified);

    setChange(
        "packed_move:" + std::to_string(packedMove_.sourceBox) + ":" +
            std::to_string(packedMove_.sourceSlot) + "->" +
            std::to_string(destinationBox) + ":" + std::to_string(insert),
        "Packed Pokemon move",
        "Box " + std::to_string(packedMove_.sourceBox + 1) +
            " Slot " + std::to_string(packedMove_.sourceSlot + 1),
        "Box " + std::to_string(destinationBox + 1) +
            " Slot " + std::to_string(insert + 1));

    placedSlot = insert;
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

} // namespace PokeVault::Integration::Gen2
