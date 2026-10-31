#ifndef POKEBANK_GEN7_MINI_GMP_ALLOCATION_SCOPE_H
#define POKEBANK_GEN7_MINI_GMP_ALLOCATION_SCOPE_H

// Leak containment for the vendored FlagBrew MemeCrypto RSA code.
//
// The vendored FlagBrew rsa.c (vendor/PKSM-Core/memecrypto, pinned submodule) never mpz_clear()s
// its three temporaries, so every memecrypto_verify() leaks their mini-gmp limb buffers (~0.9 KiB
// per SAV7 open, on the console too; LeakSanitizer reports it in CI). The submodule stays
// untouched: for the duration of one verify this scope routes mini-gmp through tracking hooks and
// frees whatever is still live afterwards. Fixed-size bookkeeping (no allocation inside the hooks);
// a pathological overflow simply stops tracking, i.e. degrades to the upstream behaviour.
// memecrypto_sign() (rsa_decrypt, used only by host fixtures) has the same defect, so fixtures
// wrap it in the same scope.

extern "C" {
#include "mini-gmp.h"
}

#include <cstddef>
#include <cstdlib>

namespace PokeVault::Integration::Gen7 {

class MiniGmpAllocationScope {
public:
    MiniGmpAllocationScope() noexcept {
        mp_get_memory_functions(&previousAlloc_, &previousRealloc_, &previousFree_);
        count_ = 0;
        mp_set_memory_functions(&allocate, &reallocate, &release);
    }
    ~MiniGmpAllocationScope() {
        for (size_t i = 0; i < count_; ++i) std::free(live_[i]);
        count_ = 0;
        mp_set_memory_functions(previousAlloc_, previousRealloc_, previousFree_);
    }
    MiniGmpAllocationScope(const MiniGmpAllocationScope&) = delete;
    MiniGmpAllocationScope& operator=(const MiniGmpAllocationScope&) = delete;

private:
    static constexpr size_t Capacity = 64;
    static void track(void* p) noexcept {
        if (p && count_ < Capacity) live_[count_++] = p;
    }
    static void untrack(void* p) noexcept {
        for (size_t i = 0; i < count_; ++i)
            if (live_[i] == p) {
                live_[i] = live_[--count_];
                return;
            }
    }
    static void* allocate(size_t size) {
        void* p = std::malloc(size);
        if (!p) std::abort();  // mini-gmp's own default allocator aborts too
        track(p);
        return p;
    }
    static void* reallocate(void* old, size_t, size_t size) {
        untrack(old);  // before realloc: `old` must not be read once it may have been released
        void* p = std::realloc(old, size);
        if (!p) std::abort();
        track(p);
        return p;
    }
    static void release(void* p, size_t) {
        untrack(p);
        std::free(p);
    }

    static inline void* live_[Capacity]{};
    static inline size_t count_ = 0;
    void* (*previousAlloc_)(size_t) = nullptr;
    void* (*previousRealloc_)(void*, size_t, size_t) = nullptr;
    void (*previousFree_)(void*, size_t) = nullptr;
};

} // namespace PokeVault::Integration::Gen7

#endif
