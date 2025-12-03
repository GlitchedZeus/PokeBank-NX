#define STB_IMAGE_IMPLEMENTATION
#include <Libs/stb_image.h>

#include <cstdio>
#include <string>

#include "UI/SpriteManager.h"
#include "Utils/Logger.h"

using namespace Utils;

namespace UI {
    // Static member initialization
    std::map<uint32_t, Sprite*> SpriteManager::spriteCache;
    bool SpriteManager::initialized = false;

    Sprite::~Sprite() {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
    }

    void SpriteManager::init() {
        if (initialized) return;

        logInfoToFile("SpriteManager initialized");
        initialized = true;
    }

    void SpriteManager::cleanup() {
        // Free all cached sprites
        for (auto& pair : spriteCache) {
            delete pair.second;
        }
        spriteCache.clear();
        initialized = false;
        logInfoToFile("SpriteManager cleanup complete");
    }

    Sprite* SpriteManager::loadSprite(const std::string& path) {
        // Try to load from ROMFS
        std::string fullPath = "romfs:/" + path;

        int width, height, channels;
        unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);

        if (!data) {
            logInfoToFile("Failed to load sprite", fullPath.c_str());
            return nullptr;
        }

        // Create sprite object
        Sprite* sprite = new Sprite();
        sprite->data = data;
        sprite->width = width;
        sprite->height = height;
        sprite->channels = channels;

        // logInfoToFile("Loaded sprite: " + fullPath + " (" +
        //               std::to_string(width) + "x" + std::to_string(height) +
        //               ", " + std::to_string(channels) + " channels)");

        return sprite;
    }

    Sprite* SpriteManager::getSprite(uint16_t speciesId, bool isShiny) {
        if (!initialized) init();

        // Generate cache key
        uint32_t cacheKey = makeCacheKey(speciesId, isShiny, false);

        // Check cache first
        auto it = spriteCache.find(cacheKey);
        if (it != spriteCache.end()) {
            return it->second;
        }

        // Build sprite path
        std::string path = "sprites/pokemon/";
        path += std::to_string(speciesId);
        if (isShiny) {
            path += "s";  // Shiny variant
        }
        path += ".png";

        // Load sprite
        Sprite* sprite = loadSprite(path);

        // Cache it (even if nullptr, so we don't keep trying to load missing sprites)
        spriteCache[cacheKey] = sprite;

        return sprite;
    }

    Sprite* SpriteManager::getIconSprite(uint16_t speciesId, bool isShiny) {
        // We don't have separate icon sprites, so just use regular sprites
        // The sprite will be cached by getSprite() to avoid duplicate allocations
        return getSprite(speciesId, isShiny);
    }

    bool SpriteManager::spriteExists(uint16_t speciesId, bool isShiny) {
        Sprite* sprite = getSprite(speciesId, isShiny);
        return sprite != nullptr;
    }
}