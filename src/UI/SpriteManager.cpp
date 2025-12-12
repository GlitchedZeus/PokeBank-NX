#define STB_IMAGE_IMPLEMENTATION
#include <Libs/stb_image.h>

#include <cstdio>
#include <string>

#include "UI/SpriteManager.h"
#include "Pokemon/FormSpriteMapping.h"
#include "Utils/Logger.h"

using namespace Utils;

namespace UI {
    // Static member initialization
    std::map<uint32_t, Sprite*> SpriteManager::spriteCache;
    std::map<uint8_t, Sprite*> SpriteManager::typeSpriteCache;
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
        // Free all cached Pokemon sprites
        for (auto& pair : spriteCache) {
            delete pair.second;
        }
        spriteCache.clear();

        // Free all cached type sprites
        for (auto& pair : typeSpriteCache) {
            delete pair.second;
        }
        typeSpriteCache.clear();

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

        return sprite;
    }

    Sprite* SpriteManager::getSprite(uint16_t speciesId, bool isShiny) {
        // Delegate to form-aware version with base form (0)
        return getSprite(speciesId, 0, isShiny);
    }

    Sprite* SpriteManager::getSprite(uint16_t speciesId, uint8_t formId, bool isShiny) {
        if (!initialized) init();

        // Generate cache key
        uint32_t cacheKey = makeCacheKey(speciesId, formId, isShiny, false);

        // Check cache first
        auto it = spriteCache.find(cacheKey);
        if (it != spriteCache.end()) {
            return it->second;
        }

        // Get the sprite ID for this form
        uint32_t spriteId = Pokemon::getFormSpriteId(speciesId, formId);

        // Build sprite path
        std::string path = "sprites/pokemon/";
        path += std::to_string(spriteId);
        if (isShiny) {
            path += "s";  // Shiny variant
        }
        path += ".png";

        // Load sprite
        Sprite* sprite = loadSprite(path);

        // If form sprite not found and this is a form, try base form
        if (!sprite && formId > 0) {
            path = "sprites/pokemon/";
            path += std::to_string(speciesId);  // Fall back to species ID
            if (isShiny) {
                path += "s";
            }
            path += ".png";
            sprite = loadSprite(path);
        }

        // Cache it (even if nullptr, so we don't keep trying to load missing sprites)
        spriteCache[cacheKey] = sprite;

        return sprite;
    }

    Sprite* SpriteManager::getIconSprite(uint16_t speciesId, bool isShiny) {
        // Delegate to form-aware version with base form (0)
        return getIconSprite(speciesId, 0, isShiny);
    }

    Sprite* SpriteManager::getIconSprite(uint16_t speciesId, uint8_t formId, bool isShiny) {
        // We don't have separate icon sprites, so just use regular sprites
        // The sprite will be cached by getSprite() to avoid duplicate allocations
        return getSprite(speciesId, formId, isShiny);
    }

    bool SpriteManager::spriteExists(uint16_t speciesId, bool isShiny) {
        Sprite* sprite = getSprite(speciesId, isShiny);
        return sprite != nullptr;
    }

    Sprite* SpriteManager::getTypeSprite(uint8_t typeId) {
        if (!initialized) init();

        // Check if type ID is valid (0-17)
        if (typeId >= 18) {
            return nullptr;
        }

        // Check cache first
        auto it = typeSpriteCache.find(typeId);
        if (it != typeSpriteCache.end()) {
            return it->second;
        }

        // Build type sprite path
        // Type sprites are stored as 0.png, 1.png, etc. in sprites/types/
        std::string path = "sprites/types/";
        path += std::to_string(typeId);
        path += ".png";

        // Load sprite
        Sprite* sprite = loadSprite(path);

        // Cache it (even if nullptr)
        typeSpriteCache[typeId] = sprite;

        return sprite;
    }

    bool SpriteManager::typeSpriteExists(uint8_t typeId) {
        Sprite* sprite = getTypeSprite(typeId);
        return sprite != nullptr;
    }
}