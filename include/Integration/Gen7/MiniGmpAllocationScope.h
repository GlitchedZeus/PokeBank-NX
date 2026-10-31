#ifndef POKEBANK_GEN7_MINI_GMP_ALLOCATION_SCOPE_H
#define POKEBANK_GEN7_MINI_GMP_ALLOCATION_SCOPE_H

// Leak containment for the vendored FlagBrew MemeCrypto RSA code.
//
// The vendored FlagBrew rsa.c (vendor/PKSM-Core/memecrypto, pinned submodule) never mpz_clear()s
// its temporaries, so every memecrypto_verify() / memecrypto_sign() leaks their mini-gmp limb
// buffers (~0.9 KiB per SAV7 open, on the console too). The submodule stays untouched: while a
// scope is alive, mini-gmp allocates through tracking hooks, and the OUTERMOST scope frees whatever
// is still live when it ends and restores the previous hooks.
//
// Invariants (audited 2026-10-31; tests/test_mini_gmp_scope.cpp pins them):
//   * Only wrap calls whose mpz values are all local to the call. memecrypto.c holds no static
//     mpz and rsa.c's are stack locals; nothing in src/ uses mini-gmp otherwise. An mpz that
//     outlived the scope would be freed under its owner, so never wrap code that keeps one.
//   * Nesting: inner scopes are no-ops (depth counter); only the outermost installs the hooks,
//     owns the table and reclaims. An inner scope therefore can never drop the outer's records.
//   * Concurrency: mini-gmp's hooks are process-global, so the whole scope holds one recursive
//     mutex. Concurrent verifies serialize instead of sharing a table; a nested scope on the
//     same thread re-enters. (The application has no worker threads today.)
//   * Hooks: memory that existed before the scope is freed by our release() with std::free,
//     matching mini-gmp's malloc-based default allocator; memory allocated inside the scope is
//     tracked. Tracking is fixed-size (no allocation inside a hook); overflow stops tracking,
//     which only degrades to the upstream leak and is reported by overflowed().

extern "C" {
#include "mini-gmp.h"
}

#include <cstddef>
#include <cstdlib>
#include <mutex>

namespace PokeVault::Integration::Gen7 {

class MiniGmpAllocationScope {
public:
    MiniGmpAllocationScope() : lock_(mutex()) {
        if (depth_++ != 0) return;  // nested on this thread: the outer scope owns everything
        mp_get_memory_functions(&previousAlloc_, &previousRealloc_, &previousFree_);
        count_ = 0;
        overflowed_ = false;
        mp_set_memory_functions(&allocate, &reallocate, &release);
    }
    ~MiniGmpAllocationScope() {
        if (--depth_ != 0) return;
        for (size_t i = 0; i < count_; ++i) std::free(live_[i]);
        count_ = 0;
        mp_set_memory_functions(previousAlloc_, previousRealloc_, previousFree_);
    }
    MiniGmpAllocationScope(const MiniGmpAllocationScope&) = delete;
    MiniGmpAllocationScope& operator=(const MiniGmpAllocationScope&) = delete;

    // Introspection for tests (read while holding a scope, or after all scopes ended).
    [[nodiscard]] static size_t liveTracked() noexcept { return count_; }
    [[nodiscard]] static size_t depth() noexcept { return depth_; }
    [[nodiscard]] static bool overflowed() noexcept { return overflowed_; }
    static constexpr size_t Capacity = 64;

private:
    static std::recursive_mutex& mutex() {
        static std::recursive_mutex m;
        return m;
    }
    static void track(void* p) noexcept {
        if (!p) return;
        if (count_ < Capacity) live_[count_++] = p;
        else overflowed_ = true;
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

    std::lock_guard<std::recursive_mutex> lock_;
    static inline void* live_[Capacity]{};
    static inline size_t count_ = 0;
    static inline size_t depth_ = 0;
    static inline bool overflowed_ = false;
    static inline void* (*previousAlloc_)(size_t) = nullptr;
    static inline void* (*previousRealloc_)(void*, size_t, size_t) = nullptr;
    static inline void (*previousFree_)(void*, size_t) = nullptr;
};

} // namespace PokeVault::Integration::Gen7

#endif
