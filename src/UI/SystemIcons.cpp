#include "UI/SystemIcons.h"

#include <Libs/stb_image.h>   // implementation lives in SpriteManager.cpp; we only need the decls

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "Games/GameIdentity.h"
#include "Utils/Logger.h"

using namespace Utils;

namespace UI {
    namespace {
        // AccountUid is a 128-bit value (u64[2]); make it usable as a map key.
        struct UidKey {
            u64 a, b;
            bool operator<(const UidKey& o) const { return a != o.a ? a < o.a : b < o.b; }
        };

        std::map<UidKey, IconImage> s_userCache;
        std::map<u64, IconImage>    s_titleCache;
        std::map<std::string, IconImage> s_gameCardCache;
        std::map<std::string, IconImage> s_trainerPortraitCache;
        IconImage s_trainerPortraitAtlas;

        constexpr int TRAINER_ATLAS_COLS = 3;
        constexpr int TRAINER_ATLAS_ROWS = 3;
        constexpr int TRAINER_ATLAS_CELL_W = 96;
        constexpr int TRAINER_ATLAS_CELL_H = 160;
        constexpr std::array<std::string_view, 9> TRAINER_ATLAS_KEYS{{
            "red", "gold", "kris",
            "brendan", "may", "lucas",
            "dawn", "ethan", "lyra",
        }};

        // Decode a JPEG blob to a session-owned RGBA IconImage (invalid on failure).
        IconImage decodeToRGBA(const unsigned char* jpg, int len) {
            IconImage img;
            int w = 0, h = 0, comp = 0;
            unsigned char* rgba = stbi_load_from_memory(jpg, len, &w, &h, &comp, 4);
            if (!rgba) return img;
            img.data = rgba;
            img.width = w;
            img.height = h;
            return img;
        }

        IconImage decodeFileToRGBA(std::string_view path) {
            IconImage img;
            int w = 0, h = 0, comp = 0;
            unsigned char* rgba = stbi_load(std::string(path).c_str(), &w, &h, &comp, 4);
            if (!rgba) return img;
            img.data = rgba;
            img.width = w;
            img.height = h;
            return img;
        }

        IconImage trainerPortraitFromAtlas(std::string_view key) {
            IconImage out;
            int index = -1;
            for (int i = 0; i < static_cast<int>(TRAINER_ATLAS_KEYS.size()); ++i) {
                if (TRAINER_ATLAS_KEYS[static_cast<size_t>(i)] == key) {
                    index = i;
                    break;
                }
            }
            if (index < 0) return out;

            if (!s_trainerPortraitAtlas.valid()) {
                s_trainerPortraitAtlas =
                    decodeFileToRGBA("romfs:/trainer_portraits/atlas.png");
                if (!s_trainerPortraitAtlas.valid()) {
                    logInfoToFile("SystemIcons: trainer portrait atlas not packaged");
                    return out;
                }
            }

            const int expectedW = TRAINER_ATLAS_COLS * TRAINER_ATLAS_CELL_W;
            const int expectedH = TRAINER_ATLAS_ROWS * TRAINER_ATLAS_CELL_H;
            if (s_trainerPortraitAtlas.width != expectedW ||
                s_trainerPortraitAtlas.height != expectedH) {
                logErrorToFile("SystemIcons: trainer portrait atlas dimensions are invalid");
                return out;
            }

            const int cellX = (index % TRAINER_ATLAS_COLS) * TRAINER_ATLAS_CELL_W;
            const int cellY = (index / TRAINER_ATLAS_COLS) * TRAINER_ATLAS_CELL_H;

            // Tight-crop each transparent atlas cell at runtime so the portrait fills the
            // Product Home card instead of inheriting unused cell padding.
            int minX = TRAINER_ATLAS_CELL_W, minY = TRAINER_ATLAS_CELL_H;
            int maxX = -1, maxY = -1;
            for (int y = 0; y < TRAINER_ATLAS_CELL_H; ++y) {
                for (int x = 0; x < TRAINER_ATLAS_CELL_W; ++x) {
                    const size_t p = static_cast<size_t>(
                        ((cellY + y) * s_trainerPortraitAtlas.width + cellX + x) * 4);
                    if (s_trainerPortraitAtlas.data[p + 3] == 0) continue;
                    minX = std::min(minX, x);
                    minY = std::min(minY, y);
                    maxX = std::max(maxX, x);
                    maxY = std::max(maxY, y);
                }
            }
            if (maxX < minX || maxY < minY) return out;

            minX = std::max(0, minX - 2);
            minY = std::max(0, minY - 2);
            maxX = std::min(TRAINER_ATLAS_CELL_W - 1, maxX + 2);
            maxY = std::min(TRAINER_ATLAS_CELL_H - 1, maxY + 2);

            const int w = maxX - minX + 1;
            const int h = maxY - minY + 1;
            auto* rgba = static_cast<unsigned char*>(
                std::malloc(static_cast<size_t>(w * h * 4)));
            if (!rgba) return out;

            for (int y = 0; y < h; ++y) {
                const size_t src = static_cast<size_t>(
                    ((cellY + minY + y) * s_trainerPortraitAtlas.width +
                     cellX + minX) * 4);
                const size_t dst = static_cast<size_t>(y * w * 4);
                std::memcpy(rgba + dst, s_trainerPortraitAtlas.data + src,
                            static_cast<size_t>(w * 4));
            }

            out.data = rgba;
            out.width = w;
            out.height = h;
            return out;
        }

        // Deliberate last-resort card art for optional/missing resources. The surrounding game card
        // still prints the exact release/platform label, so this only needs to prevent a blank hole.
        // Allocate with malloc because stb_image_free uses the matching free() path by default.
        IconImage makeGameCardFallback() {
            IconImage img;
            constexpr int w = 96, h = 96;
            constexpr int cx = w / 2, cy = h / 2, r = 32;
            constexpr int borderR = r - 2, buttonR = 10, buttonInnerR = 6;

            auto* rgba = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(w * h * 4)));
            if (!rgba) return img;

            for (int i = 0; i < w * h; ++i) {
                rgba[i * 4 + 0] = 0;
                rgba[i * 4 + 1] = 0;
                rgba[i * 4 + 2] = 0;
                rgba[i * 4 + 3] = 0;
            }

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const int dx = x - cx, dy = y - cy;
                    const int d2 = dx * dx + dy * dy;
                    if (d2 > r * r) continue;

                    unsigned char red = y < cy ? 214 : 246;
                    unsigned char green = y < cy ? 70 : 246;
                    unsigned char blue = y < cy ? 78 : 248;

                    const bool outerBand = d2 >= borderR * borderR;
                    const bool centerBand = std::abs(y - cy) <= 2;
                    const bool button = d2 <= buttonR * buttonR;
                    const bool buttonInner = d2 <= buttonInnerR * buttonInnerR;
                    if (outerBand || centerBand || button) {
                        red = 28; green = 29; blue = 34;
                    }
                    if (buttonInner) {
                        red = 246; green = 246; blue = 248;
                    }

                    const size_t p = static_cast<size_t>((y * w + x) * 4);
                    rgba[p + 0] = red;
                    rgba[p + 1] = green;
                    rgba[p + 2] = blue;
                    rgba[p + 3] = 255;
                }
            }

            img.data = rgba;
            img.width = w;
            img.height = h;
            return img;
        }
    }

    const IconImage& SystemIcons::userIcon(AccountUid uid) {
        UidKey key{uid.uid[0], uid.uid[1]};
        auto it = s_userCache.find(key);
        if (it != s_userCache.end()) return it->second;

        IconImage img;
        AccountProfile profile;
        if (R_SUCCEEDED(accountGetProfile(&profile, uid))) {
            u32 imgSize = 0;
            if (R_SUCCEEDED(accountProfileGetImageSize(&profile, &imgSize)) && imgSize > 0) {
                std::vector<unsigned char> jpg(imgSize);
                u32 realSize = 0;
                if (R_SUCCEEDED(accountProfileLoadImage(&profile, jpg.data(), imgSize, &realSize)) && realSize > 0) {
                    img = decodeToRGBA(jpg.data(), static_cast<int>(realSize));
                }
            }
            accountProfileClose(&profile);
        }
        if (!img.valid()) logErrorToFile("SystemIcons: failed to load user avatar");
        return s_userCache.emplace(key, img).first->second;
    }

    const IconImage& SystemIcons::titleIcon(u64 titleId) {
        auto it = s_titleCache.find(titleId);
        if (it != s_titleCache.end()) return it->second;

        IconImage img;
        // NsApplicationControlData is large (~0x24000); heap-allocate it.
        NsApplicationControlData* ctl = static_cast<NsApplicationControlData*>(malloc(sizeof(NsApplicationControlData)));
        if (ctl) {
            u64 outSize = 0;
            Result rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, titleId,
                                                    ctl, sizeof(NsApplicationControlData), &outSize);
            if (R_SUCCEEDED(rc) && outSize > sizeof(ctl->nacp)) {
                int iconLen = static_cast<int>(outSize - sizeof(ctl->nacp));
                img = decodeToRGBA(ctl->icon, iconLen);
            }
            free(ctl);
        }
        if (!img.valid()) logErrorToFile("SystemIcons: failed to load title icon");
        return s_titleCache.emplace(titleId, img).first->second;
    }

    const IconImage& SystemIcons::gameCardIcon(std::string_view gameId, u64 titleId) {
        // Installed titles prefer Nintendo's control-data icon. If that optional image cannot be
        // decoded, keep going: exact game identity can still resolve the packaged release artwork.
        if (titleId != 0) {
            const IconImage& systemIcon = titleIcon(titleId);
            if (systemIcon.valid()) return systemIcon;
        }

        const std::string key = gameId.empty() ? std::string("__missing_game_card__")
                                               : std::string(gameId);
        auto it = s_gameCardCache.find(key);
        if (it != s_gameCardCache.end()) return it->second;

        IconImage img;
        const std::string_view path = PokeVault::Games::gameCardArtworkPath(gameId);
        if (!path.empty()) img = decodeFileToRGBA(path);
        if (!img.valid()) {
            logErrorToFile("SystemIcons: failed to load packaged game-card artwork; using generated fallback");
            img = makeGameCardFallback();
            if (!img.valid())
                logErrorToFile("SystemIcons: failed to allocate generated game-card fallback");
        }
        return s_gameCardCache.emplace(key, img).first->second;
    }

    const IconImage& SystemIcons::trainerPortrait(std::string_view assetKey) {
        const std::string key(assetKey);
        auto it = s_trainerPortraitCache.find(key);
        if (it != s_trainerPortraitCache.end()) return it->second;

        IconImage img;
        if (!key.empty()) {
            const std::string path = "romfs:/trainer_portraits/" + key + ".png";
            img = decodeFileToRGBA(path);
            if (!img.valid()) img = trainerPortraitFromAtlas(key);
            if (!img.valid())
                logInfoToFile("SystemIcons: optional trainer portrait not packaged", key.c_str());
        }
        return s_trainerPortraitCache.emplace(key, img).first->second;
    }

    void SystemIcons::cleanup() {
        for (auto& kv : s_userCache)  if (kv.second.data) stbi_image_free(kv.second.data);
        for (auto& kv : s_titleCache) if (kv.second.data) stbi_image_free(kv.second.data);
        for (auto& kv : s_gameCardCache) if (kv.second.data) stbi_image_free(kv.second.data);
        for (auto& kv : s_trainerPortraitCache) if (kv.second.data) stbi_image_free(kv.second.data);
        if (s_trainerPortraitAtlas.data) stbi_image_free(s_trainerPortraitAtlas.data);
        s_trainerPortraitAtlas = {};
        s_userCache.clear();
        s_titleCache.clear();
        s_gameCardCache.clear();
        s_trainerPortraitCache.clear();
    }
}
