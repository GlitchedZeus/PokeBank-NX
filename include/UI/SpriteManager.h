#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include <cstdint>
#include <map>
#include <string>

// Sprite data structure
struct Sprite {
    unsigned char* data;  // Image pixel data (RGBA)
    int width;
    int height;
    int channels;         // Number of color channels (1-4)

    Sprite() : data(nullptr), width(0), height(0), channels(0) {}
    ~Sprite();
};

// Sprite manager for loading and caching Pokemon sprites
class SpriteManager {
public:
    // Initialize the sprite manager
    static void init();

    // Cleanup and free all cached sprites
    static void cleanup();

    // Get sprite for a Pokemon (normal size)
    // Returns nullptr if sprite not found
    static Sprite* getSprite(uint16_t speciesId, bool isShiny = false);

    // Get small icon sprite for box display (40x40)
    // Returns nullptr if sprite not found
    static Sprite* getIconSprite(uint16_t speciesId, bool isShiny = false);

    // Check if a sprite exists
    static bool spriteExists(uint16_t speciesId, bool isShiny = false);

private:
    // Load a sprite from ROMFS
    static Sprite* loadSprite(const std::string& path);

    // Sprite cache: key = species ID | (isShiny << 16) | (isIcon << 17)
    static std::map<uint32_t, Sprite*> spriteCache;
    static bool initialized;

    // Generate cache key
    static uint32_t makeCacheKey(uint16_t speciesId, bool isShiny, bool isIcon) {
        return speciesId | (isShiny ? (1 << 16) : 0) | (isIcon ? (1 << 17) : 0);
    }
};

#endif // SPRITE_MANAGER_H
