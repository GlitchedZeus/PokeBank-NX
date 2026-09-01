#ifndef POKEVAULT_SAFETY_WRITE_POLICY_H
#define POKEVAULT_SAFETY_WRITE_POLICY_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeVault::Safety {
    enum class SaveDestination : uint8_t {
        WorkingBackup = 0,
        NewBackup = 1,
        LiveGame = 2,
    };

    // Current alpha invariant: installed game saves are read-only sources. Enabling live writes is
    // intentionally a code change, not a setting or config-file value, so an old PKSE settings.cfg
    // cannot silently weaken the policy.
    inline constexpr bool LIVE_SAVE_WRITES_ENABLED = false;

    inline constexpr std::array<SaveDestination, 2> VISIBLE_SAVE_DESTINATIONS{
        SaveDestination::WorkingBackup,
        SaveDestination::NewBackup,
    };

    constexpr bool canWriteTo(SaveDestination destination) noexcept {
        return destination != SaveDestination::LiveGame || LIVE_SAVE_WRITES_ENABLED;
    }

    constexpr bool allowLiveInjectionRequest(bool requested) noexcept {
        return requested && LIVE_SAVE_WRITES_ENABLED;
    }
}

#endif
