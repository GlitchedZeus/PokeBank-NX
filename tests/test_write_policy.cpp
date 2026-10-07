#include "Safety/WritePolicy.h"

#include <cassert>

using namespace PokeVault::Safety;

static_assert(!LIVE_SAVE_WRITES_ENABLED);
static_assert(VISIBLE_SAVE_DESTINATIONS.size() == 2);
static_assert(VISIBLE_SAVE_DESTINATIONS[0] == SaveDestination::WorkingBackup);
static_assert(VISIBLE_SAVE_DESTINATIONS[1] == SaveDestination::NewBackup);
static_assert(canWriteTo(SaveDestination::WorkingBackup));
static_assert(canWriteTo(SaveDestination::NewBackup));
static_assert(!canWriteTo(SaveDestination::LiveGame));
static_assert(!allowLiveInjectionRequest(false));
static_assert(!allowLiveInjectionRequest(true));

int main() {
    for (const auto destination : VISIBLE_SAVE_DESTINATIONS) {
        assert(destination != SaveDestination::LiveGame);
        assert(canWriteTo(destination));
    }

    // A stale settings file or accidental caller cannot turn a request into permission.
    const bool staleSettingRequestedInjection = true;
    assert(!allowLiveInjectionRequest(staleSettingRequestedInjection));
}
