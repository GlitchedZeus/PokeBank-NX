#include "Legality/Gen4LeadEffectEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

int main() {
    using namespace Legality::Gen4LeadEffect;

    // Method J uses the high bit for Sync / Static-Magnet / Pressure / Intimidate.
    static_assert(synchronizePass(Method::J, 0x0000));
    static_assert(synchronizePass(Method::J, 0x7FFF));
    static_assert(!synchronizePass(Method::J, 0x8000));
    static_assert(staticMagnetPass(Method::J, 0x7FFF));
    static_assert(!staticMagnetPass(Method::J, 0x8000));
    static_assert(!pressureHustleVitalSpiritPass(Method::J, 0x7FFF));
    static_assert(pressureHustleVitalSpiritPass(Method::J, 0x8000));
    static_assert(!intimidateKeenEyeAbort(Method::J, 0x7FFF));
    static_assert(intimidateKeenEyeAbort(Method::J, 0x8000));
    static_assert(intimidateKeenEyeEncounterContinues(Method::J, 0x7FFF));
    static_assert(!intimidateKeenEyeEncounterContinues(Method::J, 0x8000));

    // Method J Cute Charm divides the 16-bit roll into three buckets; bucket 0
    // is the 1/3 fail branch, while buckets 1 and 2 are the 2/3 activation branch.
    static_assert(cuteCharmFail(Method::J, 0x0000));
    static_assert(cuteCharmFail(Method::J, 0x5555));
    static_assert(cuteCharmPass(Method::J, 0x5556));
    static_assert(cuteCharmPass(Method::J, 0xFFFF));

    // Method K uses low-bit parity for the same binary lead effects.
    static_assert(synchronizePass(Method::K, 0));
    static_assert(!synchronizePass(Method::K, 1));
    static_assert(staticMagnetPass(Method::K, 2));
    static_assert(!staticMagnetPass(Method::K, 3));
    static_assert(!pressureHustleVitalSpiritPass(Method::K, 2));
    static_assert(pressureHustleVitalSpiritPass(Method::K, 3));
    static_assert(!intimidateKeenEyeAbort(Method::K, 2));
    static_assert(intimidateKeenEyeAbort(Method::K, 3));

    // Method K Cute Charm is rand % 3: remainder zero fails, 1/2 pass.
    static_assert(cuteCharmFail(Method::K, 0));
    static_assert(cuteCharmPass(Method::K, 1));
    static_assert(cuteCharmPass(Method::K, 2));
    static_assert(cuteCharmFail(Method::K, 3));

    // Exhaust every possible rand16 value against the pinned formulas. This also
    // protects the complementary fail branches from accidentally drifting apart.
    for (uint32_t value = 0; value <= 0xFFFFu; ++value) {
        const auto r = static_cast<uint16_t>(value);

        const bool jHighClear = (value >> 15) == 0u;
        const bool jHighSet = !jHighClear;
        assert(synchronizePass(Method::J, r) == jHighClear);
        assert(staticMagnetPass(Method::J, r) == jHighClear);
        assert(pressureHustleVitalSpiritPass(Method::J, r) == jHighSet);
        assert(intimidateKeenEyeAbort(Method::J, r) == jHighSet);
        assert(cuteCharmPass(Method::J, r) == ((value / 0x5556u) != 0u));

        const bool kEven = (value & 1u) == 0u;
        const bool kOdd = !kEven;
        assert(synchronizePass(Method::K, r) == kEven);
        assert(staticMagnetPass(Method::K, r) == kEven);
        assert(pressureHustleVitalSpiritPass(Method::K, r) == kOdd);
        assert(intimidateKeenEyeAbort(Method::K, r) == kOdd);
        assert(cuteCharmPass(Method::K, r) == ((value % 3u) != 0u));

        assert(synchronizeFail(Method::J, r) == !synchronizePass(Method::J, r));
        assert(cuteCharmFail(Method::J, r) == !cuteCharmPass(Method::J, r));
        assert(staticMagnetFail(Method::J, r) == !staticMagnetPass(Method::J, r));
        assert(pressureHustleVitalSpiritFail(Method::J, r) ==
               !pressureHustleVitalSpiritPass(Method::J, r));
        assert(intimidateKeenEyeEncounterContinues(Method::J, r) ==
               !intimidateKeenEyeAbort(Method::J, r));

        assert(synchronizeFail(Method::K, r) == !synchronizePass(Method::K, r));
        assert(cuteCharmFail(Method::K, r) == !cuteCharmPass(Method::K, r));
        assert(staticMagnetFail(Method::K, r) == !staticMagnetPass(Method::K, r));
        assert(pressureHustleVitalSpiritFail(Method::K, r) ==
               !pressureHustleVitalSpiritPass(Method::K, r));
        assert(intimidateKeenEyeEncounterContinues(Method::K, r) ==
               !intimidateKeenEyeAbort(Method::K, r));
    }

    std::cout << "Gen IV Method J/K lead-effect predicates: PASS\n";
}
