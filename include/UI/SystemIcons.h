#ifndef UI_SYSTEM_ICONS_H
#define UI_SYSTEM_ICONS_H

#include <switch.h>
#include <string_view>

namespace UI {
    // A decoded RGBA image (Switch user avatar or game title icon). The pixel buffer is owned by
    // the session-long cache below and is NEVER freed until cleanup(), so its pointer stays stable
    // for the whole app run — PKSEFramebuffer caches GL images keyed by buffer pointer, so a stable
    // pointer is required to avoid stale-image collisions (same as SpriteManager's sprites).
    struct IconImage {
        unsigned char* data = nullptr;   // RGBA8, cache-owned
        int width  = 0;
        int height = 0;
        bool valid() const { return data != nullptr && width > 0 && height > 0; }
    };

    // Lazily decode + cache system icons for the app session.
    namespace SystemIcons {
        // The account's profile picture (JPEG decoded to RGBA). Cached by user id.
        const IconImage& userIcon(AccountUid uid);
        // A game's icon from its control data (JPEG decoded to RGBA). Cached by title id.
        const IconImage& titleIcon(u64 titleId);
        // Shared release-aware card-art resolver. Installed titles use control data; file-based
        // games use a packaged RomFS artwork path keyed by exact game identity.
        const IconImage& gameCardIcon(std::string_view gameId, u64 titleId = 0);
        // True only when the resolver can identify actual title/packaged artwork rather than the
        // generated missing-art fallback. Quick Games uses this to decide whether the title text
        // is redundant or required for identity.
        bool gameCardHasSpecificArtwork(std::string_view gameId, u64 titleId = 0);
        // Region-scene image for the Product Home hero card. Only real packaged artwork
        // is accepted. Missing art leaves the normal card surface visible; never synthesize scenery.
        const IconImage& regionBackdrop(std::string_view regionKey);
        // Real packaged trainer portrait from romfs:/trainer_portraits/<assetKey>.png.
        // Missing/invalid art is reported and the caller may show a neutral identity placeholder;
        // the loader never substitutes another trainer or a generated human portrait.
        const IconImage& trainerPortrait(std::string_view assetKey);
        // Free every cached buffer. Call once at shutdown.
        void cleanup();
    }
}

#endif
