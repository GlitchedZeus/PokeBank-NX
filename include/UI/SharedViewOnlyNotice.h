#ifndef POKEBANK_UI_SHARED_VIEW_ONLY_NOTICE_H
#define POKEBANK_UI_SHARED_VIEW_ONLY_NOTICE_H

// Round 6 L06 acceptance ("no dead taps or invisible restrictions"): in a Pokemon View, A or a tap
// on a field must say why nothing changes. One wording for Gen III, IV and V.
#include <string>
#include <string_view>

namespace PokeBank::UIModel::ViewOnlyNotice {

constexpr int kFrames = 200;

// A field Edit can change.
[[nodiscard]] inline std::string editHint(std::string_view field) {
    return "View only: choose Edit to change " + std::string(field);
}

// A field that stays read-only in every mode for this record (Origin, a transferred Pokemon's
// encounter data, a fixed form, ...): pointing at Edit would be a false promise.
[[nodiscard]] inline std::string lockedHint(std::string_view field) {
    return std::string(field) + " is read-only for this Pokemon";
}

} // namespace PokeBank::UIModel::ViewOnlyNotice

#endif
