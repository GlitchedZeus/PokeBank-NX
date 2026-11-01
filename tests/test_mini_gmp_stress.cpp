// QA D2 stress regression for MiniGmpAllocationScope (1509a314), independent of
// tests/test_mini_gmp_scope.cpp: 8 threads x 40 iterations of outer scope -> leaked temporaries ->
// nested scope -> real memecrypto sign + verify, with an observer asserting mutual exclusion (inside a
// scope this thread is the only holder: depth is exactly its own nesting) and that mini-gmp's hooks are
// the scope's while held and the defaults after. LeakSanitizer (CI: detect_leaks=1) proves reclamation.
#include "Integration/Gen7/MiniGmpAllocationScope.h"

extern "C" {
#include "memecrypto.h"
}

#include <array>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

using PokeVault::Integration::Gen7::MiniGmpAllocationScope;

namespace {
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
std::atomic<int> violations{0}, roundTrips{0};
void leak(int n) {
    for (int i = 0; i < n; ++i) {
        mpz_t x;
        mpz_init_set_ui(x, 0x9E3779B9u + static_cast<unsigned>(i));
        mpz_mul(x, x, x);
    }
}
void worker(int id, const Hooks defaults) {
    for (int it = 0; it < 40; ++it) {
        std::array<unsigned char, 0x80> payload{}, signedBytes{}, out{};
        for (size_t i = 0; i < 0x20; ++i) payload[i] = static_cast<unsigned char>(id * 31 + it * 7 + i);
        const MiniGmpAllocationScope outer;
        if (MiniGmpAllocationScope::depth() != 1 || current() == defaults) ++violations;  // exclusive + hooked
        leak(1 + (it % 3));
        {
            const MiniGmpAllocationScope inner;
            if (MiniGmpAllocationScope::depth() != 2) ++violations;
            memecrypto_sign(payload.data(), signedBytes.data(), static_cast<int>(payload.size()));
            if (memecrypto_verify(signedBytes.data(), out.data(), static_cast<int>(signedBytes.size())) &&
                std::equal(payload.begin(), payload.begin() + 0x20, out.begin()))
                ++roundTrips;
        }
        if (MiniGmpAllocationScope::depth() != 1 || current() == defaults) ++violations;  // inner end is a no-op
    }
}
} // namespace

int main() {
    const Hooks defaults = current();
    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) threads.emplace_back(worker, t, defaults);
    for (auto& th : threads) th.join();
    const bool ok = violations == 0 && roundTrips == 8 * 40 && current() == defaults &&
                    MiniGmpAllocationScope::depth() == 0 && MiniGmpAllocationScope::liveTracked() == 0 &&
                    !MiniGmpAllocationScope::overflowed();
    std::cout << "test_mini_gmp_stress: " << (ok ? "PASS" : "FAILED") << " (" << roundTrips << "/320 sign+verify round trips, "
              << violations << " exclusion/hook violations)\n";
    return ok ? 0 : 1;
}
