// Audit-owned property contract for the actual native PK4 intake helper.
// All records are synthetic; no source save or receipt is used.
#include "Pokemon/Gen4NativeRecordBoundary.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace B = Pokemon::Gen4NativeRecordBoundary;
namespace {
uint32_t next(uint32_t& state) {
    return state = state * 1664525u + 1013904223u;
}
}
int main() {
    unsigned checks = 0;
    for (size_t size = 0; size <= 512; ++size) {
        std::vector<std::byte> original(size, std::byte{0xA5});
        const auto preserved = original;
        auto copy = B::copyExact(original);
        const bool exact = size == Encryption::SIZE_STORED4 ||
                           size == Encryption::SIZE_PARTY4;
        assert(copy.size() == (exact ? size : 0));
        assert(!exact || copy == original);
        if (exact) {
            copy.front() ^= std::byte{1};
            assert(original == preserved); // never aliases caller memory
        }
        ++checks;
    }
    uint32_t seed = 0x68ad9d2bu;
    for (const auto size : {Encryption::SIZE_STORED4, Encryption::SIZE_PARTY4}) {
        for (unsigned variant = 0; variant < 256; ++variant) {
            std::vector<std::byte> source(size);
            for (auto& value : source) value = static_cast<std::byte>(next(seed) >> 24);
            const auto preserved = source;
            auto duplicate = B::copyExact(source);
            assert(duplicate == source);
            std::fill(duplicate.begin(), duplicate.end(), std::byte{0});
            assert(source == preserved);
            ++checks;
        }
    }
    // Previously an invalid PK4 wrapper copied every byte of an oversized
    // input even though its semantic accessor later rejected that size.
    for (const auto size : {size_t{1} << 20, size_t{4} << 20, size_t{16} << 20}) {
        std::vector<std::byte> large(size, std::byte{0x5C});
        const auto copy = B::copyExact(large);
        assert(copy.empty());
        assert(large.front() == std::byte{0x5C} && large.back() == std::byte{0x5C});
        ++checks;
    }
    assert(checks == 1028);
    std::cout << "L6 Stage 2 bounded PK4 intake: " << checks
              << " source-preserving cases PASS\n";
}
