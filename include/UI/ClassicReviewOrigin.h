#pragma once

#include <cstdint>
#include <string>

// Round 5: where a Gen I-IV Review was opened from decides where its B (and a Discard All that
// closes it) returns. Box Actions -> Review Changes returns to that slot's Actions sheet, as
// accepted on hardware. A Review opened anywhere else (the staged-exit menu's Review Changes or
// Export Copy, the Bag's Review, a Trainer card) is "standalone": it closes back to the screen
// underneath, never to an Actions sheet for a stale, unrelated slot.
// TrainerViewScreen::openStagedReview marks a Review standalone; each generation's Review exit
// takes the mark once.
namespace PokeBank::UIModel::ClassicReviewOrigin {

struct State {
    const void* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    std::string sourceGameId;
    bool standalone = false;
};

inline State& stateFor(const void* owner, uint64_t ownerSerial, const std::string& sourceGameId) {
    static State state;
    if (state.owner != owner || state.ownerSerial != ownerSerial || state.sourceGameId != sourceGameId) {
        state = {};
        state.owner = owner;
        state.ownerSerial = ownerSerial;
        state.sourceGameId = sourceGameId;
    }
    return state;
}

// The screen's per-construction serial when it has one (TrainerViewScreen), so a later screen
// built at the same address never inherits the mark.
template <class Screen>
uint64_t serialOf(const Screen& screen) noexcept {
    if constexpr (requires { screen.instanceSerial; }) return screen.instanceSerial;
    else return 0;
}

template <class Screen>
void markStandalone(const Screen& screen) {
    stateFor(static_cast<const void*>(&screen), serialOf(screen), screen.sourceGameId).standalone = true;
}

// True once for a standalone Review; the caller then closes Review to the screen underneath.
template <class Screen>
bool takeStandalone(const Screen& screen) {
    auto& state = stateFor(static_cast<const void*>(&screen), serialOf(screen), screen.sourceGameId);
    const bool standalone = state.standalone;
    state.standalone = false;
    return standalone;
}

} // namespace PokeBank::UIModel::ClassicReviewOrigin
