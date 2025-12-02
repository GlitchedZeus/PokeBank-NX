#include "UI/Dialogs/StatEditDialog.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"
#include "Pokemon/Pokemon.h"

using namespace Trainer;

namespace UI {
namespace Dialogs {
void drawStatEditDialog(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        // Draw dialog
        int dialogWidth = 600;
        int dialogHeight = 300;
        int dialogX = (fb.getWidth() - dialogWidth) / 2;
        int dialogY = (fb.getHeight() - dialogHeight) / 2;

        // Draw dialog background
        fb.drawFilledRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Panel);
        fb.drawRect(dialogX, dialogY, dialogWidth, dialogHeight, Colors::Border);

        // Get the Pokemon being edited based on whether it's party or box Pokemon
        const Pokemon::Pokemon* pokemon = nullptr;
        if (screen.pokemonDetailsIsParty) {
            // Get from party
            if (screen.pokemonDetailsPartyIndex >= 0 && screen.pokemonDetailsPartyIndex < static_cast<int>(screen.trainer.party.size())) {
                pokemon = screen.trainer.party[screen.pokemonDetailsPartyIndex].get();
            }
        } else {
            // Get from boxes
            if (screen.selectedBoxIndex < 0 || screen.selectedBoxIndex >= static_cast<int>(screen.trainer.boxes.size())) return;
            if (screen.selectedItemIndex < 0 || screen.selectedItemIndex >= static_cast<int>(BOX_SLOTS)) return;

            const auto& boxPokemon = screen.trainer.boxes[screen.selectedBoxIndex][screen.selectedItemIndex];
            if (boxPokemon) {
                pokemon = boxPokemon.get();
            }
        }

        if (!pokemon) return;

        // Stat names
        const char* statNames[] = {"HP", "ATK", "DEF", "SPE", "SPA", "SPD"};
        const char* currentStatName = statNames[screen.statEditSelectedStat];

        // Get current EV value
        uint8_t currentEV = 0;

        switch (screen.statEditSelectedStat) {
            case 0: currentEV = pokemon->evHP(); break;
            case 1: currentEV = pokemon->evATK(); break;
            case 2: currentEV = pokemon->evDEF(); break;
            case 3: currentEV = pokemon->evSPE(); break;
            case 4: currentEV = pokemon->evSPA(); break;
            case 5: currentEV = pokemon->evSPD(); break;
        }

        // Calculate total EVs
        int totalEVs = pokemon->evHP() + pokemon->evATK() + pokemon->evDEF() +
            pokemon->evSPE() + pokemon->evSPA() + pokemon->evSPD();

        // Draw title
        std::string titleText = std::string("Edit Stats - ") + currentStatName;
        fb.drawText(dialogX + 20, dialogY + 20, titleText, Colors::Text);
        fb.drawFilledRect(dialogX + 20, dialogY + 45, dialogWidth - 40, 2, Colors::Border);

        int lineY = dialogY + 70;
        int lineHeight = 30;

        // Show current stat name
        fb.drawText(dialogX + 20, lineY, std::string("Stat: ") + currentStatName, Colors::Text);
        lineY += lineHeight;

        // Show IV value (always show current edited value)
        char ivText[64];
        snprintf(ivText, sizeof(ivText), "IV:  %2d  (Min: 0, Max: 31)", screen.statEditCurrentIV);
        Color ivColor = screen.statEditMode == StatEditMode::IV ? Colors::Yellow : Colors::Text;
        if (screen.statEditMode == StatEditMode::IV) {
            fb.drawText(dialogX + 15, lineY, "", Colors::Yellow);
        }
        fb.drawText(dialogX + 40, lineY, ivText, ivColor);
        lineY += lineHeight;

        // Show EV value (always show current edited value)
        char evText[64];
        snprintf(evText, sizeof(evText), "EV:  %3d (Min: 0, Max: 252)", screen.statEditCurrentEV);
        Color evColor = screen.statEditMode == StatEditMode::EV ? Colors::Yellow : Colors::Text;
        if (screen.statEditMode == StatEditMode::EV) {
            fb.drawText(dialogX + 15, lineY, "", Colors::Yellow);
        }
        fb.drawText(dialogX + 40, lineY, evText, evColor);
        lineY += lineHeight;

        // Show total EVs
        lineY += 10;
        char totalText[64];

        // Calculate what the new total would be using current edited EV
        int projectedTotal = totalEVs - currentEV + screen.statEditCurrentEV;

        snprintf(totalText, sizeof(totalText), "Total EVs: %d/510", projectedTotal);
        Color totalColor = projectedTotal > 510 ? Colors::Red : Colors::TextDim;
        fb.drawText(dialogX + 20, lineY, totalText, totalColor);

        if (projectedTotal > 510) {
            fb.drawText(dialogX + 220, lineY, "(EXCEEDS LIMIT!)", Colors::Red);
        }
        lineY += lineHeight + 15;

        // Draw instructions
        fb.drawText(dialogX + 20, lineY, "Controls:", Colors::Text);
        lineY += lineHeight - 5;

        fb.drawText(dialogX + 20, lineY, "Up/Down: Switch IV/EV  |  Left/Right: +/-1  |  L/R: +/-10", Colors::TextDim);
        lineY += 20;

        if (screen.statEditMode == StatEditMode::EV) {
            fb.drawText(dialogX + 20, lineY, "ZL/ZR: +/-100 (EVs only)", Colors::TextDim);
        }
        lineY += 20;

        fb.drawText(dialogX + 20, lineY, "A: Confirm  |  B: Cancel", Colors::TextDim);
    }
}
}
