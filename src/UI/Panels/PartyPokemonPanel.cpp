#include <cstdio>
#include <string>

#include "UI/Panels/PartyPokemonPanel.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include "UI/SpriteManager.h"
#include "Trainer/Trainer.h"

namespace Panels {

void drawPartyPokemon(PKSEFramebuffer& fb, const std::vector<std::unique_ptr<PKM>>& party, uint32_t trainerID32, int x, int y, int width, int height, int selectedIndex) {
    fb.drawFilledRect(x, y, width, height, Colors::Panel);
    fb.drawRect(x, y, width, height, Colors::Border);

    fb.drawText(x + 20, y + 20, "Party Pokemon", Colors::Text);
    fb.drawFilledRect(x + 20, y + 45, width - 40, 2, Colors::Border);

    // Draw in two columns: Slot 1-3 on left, Slot 4-6 on right
    int lineHeight = 20;
    int columnWidth = (width - 60) / 2;  // Dynamic column width based on panel width

    for (size_t i = 0; i < party.size() && i < 6; i++) {
        const PKM* pkm = party[i].get();
        if (!pkm) continue;

        // Determine column (0 = left, 1 = right)
        int column = (i >= 3) ? 1 : 0;
        int slotInColumn = (i >= 3) ? (i - 3) : i;

        // Calculate position
        int colX = x + 20 + (column * columnWidth);
        int colY = y + 60 + (slotInColumn * 170);  // Each slot takes ~170px vertically

        // Highlight selected Pokemon
        if (selectedIndex >= 0 && static_cast<int>(i) == selectedIndex) {
            fb.drawFilledRect(colX - 5, colY - 5, columnWidth - 10, 165, Colors::Selected);
        }

        if (pkm->speciesID() == 0) {
            std::string slotText = "Slot " + std::to_string(i + 1) + ": Empty";
            fb.drawText(colX, colY, slotText, Colors::TextDim);
            continue;
        }

        // Load and draw Pokemon sprite to the right of stats
        bool isShiny = pkm->isShiny(trainerID32, pkm->species());
        Sprite* sprite = SpriteManager::getSprite(pkm->speciesID(), isShiny);

        if (sprite && sprite->data) {
            // Draw sprite to the right of the stats section (around x=300)
            // This fills the empty space better while keeping stats visible
            int spriteX = colX + 280;
            int spriteY = colY + 10;
            fb.drawImage(spriteX, spriteY, sprite->width, sprite->height,
                        sprite->data, sprite->channels);
        }

        // Draw header: "Slot X: Species"
        std::string headerText = "Slot " + std::to_string(i + 1) + ": " + std::string(pkm->species());
        int textX = colX;
        fb.drawText(textX, colY, headerText, Colors::Text);
        textX += headerText.length() * 8;  // Approximate character width

        // Draw gender symbol next to species name
        std::string genderSymbol = pkm->genderSymbol();
        if (genderSymbol != "?" && genderSymbol != "Genderless" && std::string(genderSymbol) != "") {
            Color genderColor = (genderSymbol == "♂") ? Colors::Blue : Colors::Magenta;
            fb.drawText(textX, colY, std::string(" ") + genderSymbol, genderColor);
            textX += 16;  // Space + symbol width
        }

        // Draw shiny star in red after gender
        if (pkm->isShiny(trainerID32, pkm->species())) {
            fb.drawText(textX, colY, " ★", Colors::Red);
            textX += 16;
        }

        // Draw level
        std::string levelText = " - Lv." + std::to_string(pkm->level());
        fb.drawText(textX, colY, levelText, Colors::Text);

        colY += lineHeight;

        // Draw stats header
        fb.drawText(colX + 20, colY, "    Base | IV | EV  | Stat", Colors::TextDim);
        colY += lineHeight;

        // Draw each stat
        char statLine[100];
        snprintf(statLine, sizeof(statLine), "HP : %03d | %02d | %03d | %03d",
                pkm->baseHP(), pkm->ivHP(), pkm->evHP(), pkm->statHPMax());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
        colY += lineHeight;

        snprintf(statLine, sizeof(statLine), "ATK: %03d | %02d | %03d | %03d",
                pkm->baseATK(), pkm->ivATK(), pkm->evATK(), pkm->statATK());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
        colY += lineHeight;

        snprintf(statLine, sizeof(statLine), "DEF: %03d | %02d | %03d | %03d",
                pkm->baseDEF(), pkm->ivDEF(), pkm->evDEF(), pkm->statDEF());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
        colY += lineHeight;

        snprintf(statLine, sizeof(statLine), "SpA: %03d | %02d | %03d | %03d",
                pkm->baseSPA(), pkm->ivSPA(), pkm->evSPA(), pkm->statSPA());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
        colY += lineHeight;

        snprintf(statLine, sizeof(statLine), "SpD: %03d | %02d | %03d | %03d",
                pkm->baseSPD(), pkm->ivSPD(), pkm->evSPD(), pkm->statSPD());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
        colY += lineHeight;

        snprintf(statLine, sizeof(statLine), "Spe: %03d | %02d | %03d | %03d",
                pkm->baseSPE(), pkm->ivSPE(), pkm->evSPE(), pkm->statSPE());
        fb.drawText(colX + 20, colY, statLine, Colors::Text);
    }
}

} // namespace Panels
