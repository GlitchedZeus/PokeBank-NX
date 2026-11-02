#pragma once

// Byte-surgical edits of Gen II item lists, following the game's own list routines (pret
// pokecrystal 5c877ef5, engine/items/items.asm):
//
//   pair list:  [count] [id qty] x count [0xFF] [stale bytes up to capacity]
//   key list:   [count] [id] x count [0xFF] [stale bytes up to capacity]
//
// - Changing a quantity rewrites that one quantity byte.
// - Adding writes the entry where the terminator was, then a new 0xFF after it, and count + 1
//   (PutItemInPocket).
// - Removing shifts the following entries and the terminator down by one entry and count - 1
//   (RemoveItemFromPocket / RemoveKeyItemFromPocket). The bytes after the new terminator keep
//   their stale values, exactly as the game leaves them.
// - Setting the quantity it already has changes nothing.
//
// Independent QA 2026-10-27 Q07: the previous writers zero-filled the whole list on every edit.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace PokeVault::Integration::Gen2::NativeItemList {

// Index of `id` in a well-formed list, or -1. Returns false if the list is malformed.
inline bool find(std::span<const uint8_t> bytes, std::size_t offset, std::size_t capacity,
                 std::size_t stride, uint8_t id, long& index) noexcept {
    index = -1;
    if (offset >= bytes.size() || 1 + stride * capacity + 1 > bytes.size() - offset) return false;
    const uint8_t count = bytes[offset];
    if (count > capacity || bytes[offset + 1 + stride * count] != 0xFF) return false;
    for (std::size_t i = 0; i < count; ++i)
        if (bytes[offset + 1 + stride * i] == id) { index = static_cast<long>(i); return true; }
    return true;
}

// Sets `id` to `quantity` (stride 2) or presence (stride 1, quantity 0/1). Returns false and
// leaves `bytes` untouched on a malformed list or a full list.
inline bool set(std::span<uint8_t> bytes, std::size_t offset, std::size_t capacity, std::size_t stride,
                uint8_t id, uint8_t quantity, std::string& error) noexcept {
    long index = -1;
    if (!find(bytes, offset, capacity, stride, id, index)) {
        error = "Generation II item list is malformed";
        return false;
    }
    const uint8_t count = bytes[offset];
    const std::size_t base = offset + 1;
    if (index >= 0) {
        const std::size_t entry = base + stride * static_cast<std::size_t>(index);
        if (quantity != 0) {
            if (stride == 2) bytes[entry + 1] = quantity;  // no-op when unchanged
            return true;
        }
        // Remove: shift the following entries and the terminator down by one entry.
        const std::size_t end = base + stride * count;  // terminator position
        for (std::size_t p = entry; p + stride <= end; ++p) bytes[p] = bytes[p + stride];
        bytes[offset] = static_cast<uint8_t>(count - 1);
        return true;
    }
    if (quantity == 0) return true;  // removing an absent item changes nothing
    if (count >= capacity) {
        error = "Generation II pocket is full; cannot add another item";
        return false;
    }
    const std::size_t entry = base + stride * count;
    bytes[entry] = id;
    if (stride == 2) bytes[entry + 1] = quantity;
    bytes[entry + stride] = 0xFF;
    bytes[offset] = static_cast<uint8_t>(count + 1);
    return true;
}

}  // namespace PokeVault::Integration::Gen2::NativeItemList
