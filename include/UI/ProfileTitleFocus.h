#pragma once

// Product Home keeps the focused game per profile. Switching to another profile and back (Profile
// picker, Games-browser L/R) returns to the game that was focused there, for every card kind
// (Switch title, Gen I-III RetroArch / mGBA card, Gen IV / V assigned file), not only Gen V.
// Re-confirming the profile that is already active changes nothing (SaveSelectScreen::setUser).
//
// Keys are opaque per-card identities built by the caller (game id + title id + source kind);
// a key that is no longer on the profile's list (card removed, source unassigned) restores
// nothing, so the caller falls back to the first card. UI focus only: it never selects or
// authorizes a save. Pure and host-tested (tests/test_profile_title_focus.cpp).

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace PokeBank::UIModel {

class ProfileTitleFocus {
public:
    void remember(std::string_view profile, std::string_view titleKey) {
        if (profile.empty()) return;
        if (titleKey.empty()) focus_.erase(std::string(profile));
        else focus_[std::string(profile)] = std::string(titleKey);
    }

    // Index of the remembered card in `titleKeys` (the profile's current card order), if present.
    [[nodiscard]] std::optional<std::size_t> indexFor(std::string_view profile,
                                                      const std::vector<std::string>& titleKeys) const {
        const auto found = focus_.find(std::string(profile));
        if (found == focus_.end()) return std::nullopt;
        for (std::size_t i = 0; i < titleKeys.size(); ++i)
            if (titleKeys[i] == found->second) return i;
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, std::string> focus_;
};

} // namespace PokeBank::UIModel
