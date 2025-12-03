#include <algorithm>

#include "UI/Panels/BoxPokemonPanel.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "Trainer/Trainer.h"

using namespace Trainer;

namespace UI {
namespace Panels {
    void drawBoxPokemon(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height) {
        fb.drawFilledRect(x, y, width, height, Colors::Panel);
        fb.drawRect(x, y, width, height, Colors::Border);

        // Check if we have valid box data
        if (screen.selectedBoxIndex < 0 || screen.selectedBoxIndex >= static_cast<int>(screen.trainer.boxes.size())) {
            fb.drawText(x + 20, y + 80, "No box data available", Colors::TextDim);
            return;
        }

        // Get current box name
        std::string boxName = screen.selectedBoxIndex < static_cast<int>(screen.trainer.boxNames.size())
            ? screen.trainer.boxNames[screen.selectedBoxIndex]
            : ("Box " + std::to_string(screen.selectedBoxIndex + 1));

        // Header with box name
        std::string headerText = boxName;
        // if (screen.detailViewActive) {
        //     headerText += " [DETAIL VIEW]";
        // }
        headerText += " (" + std::to_string(screen.selectedBoxIndex + 1) + "/" + std::to_string(screen.trainer.boxes.size()) + ")";
        fb.drawText(x + 20, y + 20, headerText, Colors::Text);
        fb.drawFilledRect(x + 20, y + 45, width - 40, 2, Colors::Border);

        // Get current box
        const auto& currentBox = screen.trainer.boxes[screen.selectedBoxIndex];

        // Calculate grid layout - 6 columns x 5 rows = 30 slots
        constexpr int GRID_COLS = 6;
        constexpr int GRID_ROWS = 5;
        constexpr int SLOT_WIDTH = 164;
        constexpr int SLOT_HEIGHT = 90;
        constexpr int GRID_PADDING_X = 10;
        constexpr int GRID_PADDING_Y = 5;
        const int startX = x + 20;
        const int startY = y + 70;

        // Draw all 30 slots in a 6x5 grid
        for (int row = 0; row < GRID_ROWS; ++row) {
            for (int col = 0; col < GRID_COLS; ++col) {
                int slotIndex = row * GRID_COLS + col;
                int slotX = startX + col * (SLOT_WIDTH + GRID_PADDING_X);
                int slotY = startY + row * (SLOT_HEIGHT + GRID_PADDING_Y);

                // Highlight selected slot in detail view
                if (screen.detailViewActive && slotIndex == screen.selectedItemIndex) {
                    fb.drawFilledRect(slotX - 2, slotY - 2, SLOT_WIDTH + 4, SLOT_HEIGHT + 4, Colors::Selected);
                }

                // Draw slot border
                fb.drawRect(slotX, slotY, SLOT_WIDTH, SLOT_HEIGHT, Colors::Border);

                // Check if this slot has a pokemon
                const auto& pokemon = currentBox[slotIndex];
                if (pokemon) {
                    // Draw species name
                    std::string speciesName = pokemon->species();
                    if (speciesName == "None") {
                        speciesName = "Empty";
                        fb.drawText(slotX + 5, slotY + 35, speciesName.c_str(), Colors::TextDim);
                        continue;
                    }

                    // Load and draw Pokemon sprite icon (left side of cell, smaller)
                    bool isShiny = pokemon->isShiny(screen.trainer.ID32, speciesName);
                    Sprite* sprite = SpriteManager::getIconSprite(pokemon->speciesID(), isShiny);

                    const int SPRITE_SIZE = 50;  // Scaled down sprite size (was 96x96, now 50x50)
                    int spriteWidth = 0;
                    if (sprite && sprite->data) {
                        // Position sprite on left side of cell, centered vertically
                        int spriteX = slotX + 5;
                        int spriteY = slotY + (SLOT_HEIGHT - SPRITE_SIZE) / 2;

                        // Draw sprite scaled down to 50x50
                        fb.drawImageScaled(spriteX, spriteY, sprite->width, sprite->height,
                            SPRITE_SIZE, SPRITE_SIZE,
                            sprite->data, sprite->channels);

                        spriteWidth = SPRITE_SIZE + 8;  // Add padding
                    }

                    // Draw species name to the right of sprite
                    int textX = slotX + 5 + spriteWidth;
                    fb.drawText(textX, slotY + 10, speciesName.c_str(), Colors::Text);
                    textX += speciesName.length() * 8;

                    // Draw gender symbol right after species name
                    const char* genderSymbol = pokemon->genderSymbol();
                    if (genderSymbol[0] != '\0' && std::string(genderSymbol) != "") {
                        Color genderColor = (std::string(genderSymbol) == "♂") ? Colors::Blue : Colors::Magenta;
                        fb.drawText(textX, slotY + 10, std::string(" ") + genderSymbol, genderColor);
                        textX += 16;  // Space + symbol width
                    }

                    // Add shiny star in red after gender
                    if (pokemon->isShiny(screen.trainer.ID32, speciesName)) {
                        fb.drawText(textX, slotY + 10, " ★", Colors::Red);
                    }

                    if (speciesName != "Empty") {
                        // Draw level below species name, to the right of sprite
                        char levelText[16];
                        snprintf(levelText, sizeof(levelText), "Lv.%d", pokemon->level());
                        fb.drawText(slotX + 5 + spriteWidth, slotY + 30, levelText, Colors::TextDim);
                    }

                    // // Show cursor if this slot is selected in detail view
                    // if (screen.detailViewActive && slotIndex == screen.selectedItemIndex) {
                    //     fb.drawText(slotX + 5, slotY + 60, "> Selected", Colors::Yellow);
                    // }
                } else {
                    // Empty slot
                    fb.drawText(slotX + 5, slotY + 35, "Empty", Colors::TextDim);
                }
            }
        }
    }
}
}
