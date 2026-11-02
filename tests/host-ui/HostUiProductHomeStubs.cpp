// HOST UI HARNESS ONLY (never shipped): Product Home (SaveSelectScreen) artwork on the host. The
// real UI/SystemIcons.cpp loads RomFS / account images (platform-only, excluded from the harness);
// here every lookup returns an empty image, which Product Home already draws as its missing-art
// fallback (SystemIcons.h: the generated fallback when an image is unavailable).
#include "Utils/NXTypes.h"
#include "UI/SystemIcons.h"

namespace UI::SystemIcons {
namespace {
const IconImage& empty() {
    static const IconImage image{};
    return image;
}
} // namespace
const IconImage& userIcon(AccountUid) { return empty(); }
const IconImage& gameCardIcon(std::string_view, u64) { return empty(); }
bool gameCardHasSpecificArtwork(std::string_view, u64) { return false; }
const IconImage& regionBackdrop(std::string_view) { return empty(); }
const IconImage& trainerPortrait(std::string_view) { return empty(); }
} // namespace UI::SystemIcons
