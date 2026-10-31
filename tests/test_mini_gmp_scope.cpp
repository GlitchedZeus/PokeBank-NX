// MiniGmpAllocationScope (include/Integration/Gen7/MiniGmpAllocationScope.h): pins the audited
// invariants of the leak-containment wrapper around the vendored FlagBrew MemeCrypto RSA code.
//   * leaked mpz temporaries inside a scope are reclaimed (LeakSanitizer proves it in CI);
//   * the previous mini-gmp hooks are restored exactly;
//   * a nested scope is a no-op that never drops the outer scope's records;
//   * memory allocated BEFORE a scope can be freed inside it, and survives past it when not freed;
//   * overflow beyond the fixed table only degrades to the upstream leak and is reported;
//   * concurrent scopes on several threads serialize (ASan would flag a shared-table corruption);
//   * real memecrypto sign + verify still round-trip inside the scope (SAV7 verification preserved).
#include "Integration/Gen7/MiniGmpAllocationScope.h"

extern "C" {
#include "memecrypto.h"
}

#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using PokeVault::Integration::Gen7::MiniGmpAllocationScope;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
struct Hooks {
    void* (*a)(size_t) = nullptr;
    void* (*r)(void*, size_t, size_t) = nullptr;
    void (*f)(void*, size_t) = nullptr;
    bool operator==(const Hooks& o) const { return a == o.a && r == o.r && f == o.f; }
};
Hooks current() {
    Hooks h;
    mp_get_memory_functions(&h.a, &h.r, &h.f);
    return h;
}
// The rsa.c pattern: temporaries initialised and never cleared.
void leakLikeRsa(int n) {
    for (int i = 0; i < n; ++i) {
        mpz_t x;
        mpz_init_set_ui(x, 0xFFFFFFFFu);
        mpz_mul(x, x, x);
        mpz_mul(x, x, x);  // forces a realloc of the limb buffer
    }
}
bool roundTrip() {
    std::array<unsigned char, 0x80> payload{}, signedBytes{}, recovered{};
    for (size_t i = 0; i < 0x20; ++i) payload[i] = static_cast<unsigned char>(i * 7 + 1);
    {
        const MiniGmpAllocationScope scope;
        memecrypto_sign(payload.data(), signedBytes.data(), static_cast<int>(payload.size()));
    }
    bool ok = false;
    {
        const MiniGmpAllocationScope scope;
        ok = memecrypto_verify(signedBytes.data(), recovered.data(), static_cast<int>(signedBytes.size())) != 0;
    }
    return ok && std::equal(payload.begin(), payload.begin() + 0x20, recovered.begin());
}
} // namespace

int main() {
    const Hooks defaults = current();

    // Reclaim + restore.
    {
        const MiniGmpAllocationScope scope;
        check(!(current() == defaults) && MiniGmpAllocationScope::depth() == 1, "scope installs its hooks");
        leakLikeRsa(3);
        check(MiniGmpAllocationScope::liveTracked() == 3, "three leaked temporaries are tracked");
    }
    check(current() == defaults && MiniGmpAllocationScope::liveTracked() == 0 && MiniGmpAllocationScope::depth() == 0,
          "scope end reclaims and restores the default hooks");

    // Nesting never drops the outer scope's records.
    {
        const MiniGmpAllocationScope outer;
        leakLikeRsa(2);
        {
            const MiniGmpAllocationScope inner;
            check(MiniGmpAllocationScope::depth() == 2 && MiniGmpAllocationScope::liveTracked() == 2,
                  "an inner scope keeps the outer's table");
            leakLikeRsa(1);
        }
        check(MiniGmpAllocationScope::depth() == 1 && MiniGmpAllocationScope::liveTracked() == 3 && !(current() == defaults),
              "inner end is a no-op: nothing freed early, hooks stay installed");
    }
    check(current() == defaults && MiniGmpAllocationScope::liveTracked() == 0, "outer end reclaims both levels");

    // Memory from before the scope: freeing it inside is safe; keeping it past the scope is safe.
    {
        mpz_t before, kept;
        mpz_init_set_ui(before, 12345);
        mpz_init_set_ui(kept, 777);
        {
            const MiniGmpAllocationScope scope;
            mpz_clear(before);  // pre-scope malloc memory released through our std::free hook
            check(MiniGmpAllocationScope::liveTracked() == 0, "pre-scope memory is never tracked");
        }
        check(mpz_cmp_ui(kept, 777) == 0, "pre-scope mpz survives the scope untouched");
        mpz_clear(kept);
    }

    // Overflow degrades to the upstream behaviour, flagged; untracked memory is never freed by us.
    {
        mpz_t extra[4];
        {
            const MiniGmpAllocationScope scope;
            leakLikeRsa(static_cast<int>(MiniGmpAllocationScope::Capacity));
            for (auto& e : extra) {
                mpz_init_set_ui(e, 0xFFFFFFFFu);
                mpz_mul(e, e, e);
            }
            check(MiniGmpAllocationScope::overflowed() && MiniGmpAllocationScope::liveTracked() == MiniGmpAllocationScope::Capacity,
                  "overflow is reported and tracking stops at capacity");
        }
        check(current() == defaults, "hooks restored after overflow");
        bool intact = true;
        for (auto& e : extra) {
            intact = intact && mpz_cmp_ui(e, 0) > 0;  // still owned by us: the scope did not free them
            mpz_clear(e);
        }
        check(intact, "untracked (overflow) allocations are left to their owner, never double-freed");
    }

    // Real MemeCrypto still round-trips (SAV7 signature verification preserved).
    check(roundTrip(), "memecrypto sign/verify round-trip inside scopes");

    // Concurrency: scopes on several threads serialize.
    {
        std::atomic<int> good{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t)
            threads.emplace_back([&good] {
                for (int i = 0; i < 6; ++i)
                    if (roundTrip()) ++good;
            });
        for (auto& th : threads) th.join();
        check(good == 24 && current() == defaults && MiniGmpAllocationScope::depth() == 0,
              "24 concurrent sign/verify round-trips succeed and leave the defaults installed");
    }

    std::cout << "test_mini_gmp_scope: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? 1 : 0;
}
