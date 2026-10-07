#ifndef UI_STORAGE_PLACEMENT_COMMIT_H
#define UI_STORAGE_PLACEMENT_COMMIT_H

#include <memory>
#include <utility>

namespace PokeBank::UIModel {

// Native-format placement (Bank destination): move the exact authoritative object
// into the destination and bring any displaced object back into custody.
template <typename T>
inline void commitNativeSwap(std::unique_ptr<T>& custody,
                             std::unique_ptr<T>& destination) noexcept {
    std::swap(destination, custody);
}

// Prepared save placement: destination ownership changes first. Only after the
// destination owns the candidate is the authoritative original retired/replaced
// by the displaced destination object.
template <typename T>
inline void commitPreparedCandidate(std::unique_ptr<T>& custody,
                                    std::unique_ptr<T>& destination,
                                    std::unique_ptr<T>& candidate) noexcept {
    auto displaced = std::move(destination);
    destination = std::move(candidate);
    custody = std::move(displaced);
}

} // namespace PokeBank::UIModel

#endif
