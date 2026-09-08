#ifndef POKEBANK_LEGACY_FRLG_SOURCE_BROWSER_H
#define POKEBANK_LEGACY_FRLG_SOURCE_BROWSER_H

#include "Legacy/RetroArchFRLGDiscovery.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace PokeVault::Legacy {
    enum class LegacySaveInstanceKind : unsigned char {
        BatterySave,
        SaveState,
        Backup,
        ManualImport,
    };

    struct FRLGSaveInstance {
        size_t sourceIndex = 0;
        LegacySaveInstanceKind kind = LegacySaveInstanceKind::BatterySave;
        std::string label;
        std::string sourceLabel;
        std::string location;
        std::string normalizedPath;
        std::string sourceIdentity;
        uint64_t fileSize = 0;
        int64_t modifiedTime = 0;
    };

    // One UI-neutral parent card per exact game identity/source family. Validated save files are
    // children, not duplicate top-level cards. The child retains the session-catalog index so
    // activation never copies or reparses the raw source file.
    struct FRLGSourceCard {
        std::string gameId;
        std::string title;
        std::string platformLabel;
        std::string sourceLabel;
        std::string artworkKey;
        std::vector<FRLGSaveInstance> instances;
    };

    // Only strict, ready sources become selectable cards. Invalid, ambiguous, or unreadable files
    // stay in the diagnostic catalog but can never enter the browsing route.
    [[nodiscard]] std::vector<FRLGSourceCard> buildFRLGSourceCards(
        const FRLGDiscoveryResult& discovery);

    // Revalidates parent identity and child catalog index at activation time. This prevents a stale
    // or malformed UI descriptor from routing to another release, platform, or source instance.
    [[nodiscard]] const FRLGSource* resolveFRLGSaveInstance(
        const FRLGDiscoveryResult& discovery, const FRLGSourceCard& card,
        size_t instanceIndex) noexcept;
}

#endif
