#include <cstring>

#include "UI/Modals/PokemonDetailsModal.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"
#include "Utils/HelperUtilities.h"
#include "Pokemon/Pokemon.h"
#include "Names/FormNames.h"

using namespace Trainer;
using namespace Utils;

namespace UI {
namespace Modals {
    void drawPokemonDetailsModal(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
        // Get the selected pokemon based on whether it's party or box Pokemon
        const Pokemon::Pokemon* pokemon = nullptr;
        if (screen.pokemonDetailsIsParty) {
            // Get from party
            if (screen.pokemonDetailsPartyIndex >= 0 && screen.pokemonDetailsPartyIndex < static_cast<int>(screen.trainer.party.size())) {
                pokemon = screen.trainer.party[screen.pokemonDetailsPartyIndex].get();
                if (pokemon->speciesID() == 0) return; // Empty slot
            } else {
                return;
            }
        } else {
            // Get from boxes
            if (screen.selectedBoxIndex < 0 || screen.selectedBoxIndex >= static_cast<int>(screen.trainer.boxes.size())) return;
            if (screen.selectedItemIndex < 0 || screen.selectedItemIndex >= static_cast<int>(BOX_SLOTS)) return;

            const auto& boxPokemon = screen.trainer.boxes[screen.selectedBoxIndex][screen.selectedItemIndex];
            if (!boxPokemon) return; // Empty slot
            pokemon = boxPokemon.get();
        }

        if (!pokemon) return;

        // Modal dimensions - take up most of the screen
        constexpr int MODAL_WIDTH = 1100;
        constexpr int MODAL_HEIGHT = 600;
        const int modalX = (fb.getWidth() - MODAL_WIDTH) / 2;
        const int modalY = (fb.getHeight() - MODAL_HEIGHT) / 2;

        // Draw modal background
        fb.drawFilledRect(modalX, modalY, MODAL_WIDTH, MODAL_HEIGHT, Colors::Panel);
        fb.drawRect(modalX, modalY, MODAL_WIDTH, MODAL_HEIGHT, Colors::Border);

        // Left panel - Categories
        constexpr int CATEGORY_PANEL_WIDTH = 200;
        fb.drawRect(modalX + 10, modalY + 10, CATEGORY_PANEL_WIDTH, MODAL_HEIGHT - 20, Colors::Border);

        const char* categories[] = {"Main", "Met", "Stats", "Moves", "Cosmetic", "OT/Misc"};
        int catY = modalY + 30;
        for (int i = 0; i < 6; i++) {
            if (i == screen.pokemonDetailsCategory) {
                fb.drawFilledRect(modalX + 15, catY - 2, CATEGORY_PANEL_WIDTH - 10, 22, Colors::Selected);
            }
            std::string catText = std::string("> ") + categories[i];
            fb.drawText(modalX + 25, catY, catText, Colors::Text);
            catY += 30;
        }

        // Right panel - Content area
        const int contentX = modalX + CATEGORY_PANEL_WIDTH + 30;
        const int contentY = modalY + 20;
        const int contentWidth = MODAL_WIDTH - CATEGORY_PANEL_WIDTH - 50;

        // TODO: We need to calculate the amount of fields and then pass it to the TrainerViewScreen for iteration when active
        // Draw category content
        if (screen.pokemonDetailsCategory == 0) { // Main
            int lineY = contentY;
            int lineHeight = 25;

            fb.drawText(contentX, lineY, "=== Main ===", Colors::Text);
            if (screen.pokemonDetailsEditing) {
                fb.drawText(contentX + 150, lineY, "(Use Up/Down to select, A to edit)", Colors::TextDim);
            } else {
                fb.drawText(contentX + 150, lineY, "(Press A to edit)", Colors::TextDim);
            }
            lineY += lineHeight + 5;

            char buffer[128];
            // Field 0: PID
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 0) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "PID: %08X", pokemon->pid());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 1: Species
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 1) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            // Display form name
            const char* formName = Names::getFormName(pokemon->speciesID(), pokemon->form());
            if (formName && strlen(formName) > 0) {
                snprintf(buffer, sizeof(buffer), "Species: %s (%s) (#%d)",
                    pokemon->species(), formName, pokemon->speciesID());
            } else {
                snprintf(buffer, sizeof(buffer), "Species: %s (#%d)",
                    pokemon->species(), pokemon->speciesID());
            }
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 2: Form ID (debugging)
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 1) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Form: %d (formID: %d)", pokemon->form(), pokemon->form());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 3: Gender
            std::string genderSymbol = pokemon->genderSymbol();
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 2) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            if (genderSymbol == "") {
                std::string genderText = "Gender: Genderless";
                fb.drawText(contentX, lineY, genderText, Colors::Text);
            }
            else {
                std::string genderText = "Gender: ";
                Color genderColor = (genderSymbol == "♂") ? Colors::Blue : Colors::Magenta;
                fb.drawText(contentX, lineY, genderText, Colors::Text);
                fb.drawText(contentX + 55, lineY, genderSymbol, genderColor);
            }
            lineY += lineHeight;

            // Field 4: Shiny
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 3) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Shiny: %s", pokemon->isShiny(screen.trainer.ID32, pokemon->species()) ? "Yes" : "No");
            Color shinyColor = screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 3 ? Colors::Yellow : Colors::Text;
            fb.drawText(contentX, lineY, buffer, shinyColor);
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 3) {
                fb.drawText(contentX + 150, lineY, "(Press A to toggle)", Colors::TextDim);
            }
            lineY += lineHeight;

            // Field 5: Nickname
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 4) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            std::string nickname = utf16ToUtf8(pokemon->nickname());
            snprintf(buffer, sizeof(buffer), "Nickname: %s", nickname.c_str());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 6: EXP
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 5) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "EXP: %u", pokemon->exp());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 7: Level
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 6) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Level: %d", pokemon->level());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 8: Nature
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 7) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Nature: %s (%d)", getNatureName(pokemon->nature()), pokemon->nature());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 9: Stat Nature
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 8) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Stat Nature: %s (%d)", getNatureName(pokemon->statNature()), pokemon->statNature());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 10: Held Item
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 9) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Held Item: %s (%d)", getItemName(pokemon->heldItem()), pokemon->heldItem());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 11: Ability
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 10) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Ability: %s (%d)", getAbilityName(pokemon->ability()), pokemon->ability());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 12: Friendship Value (0-255)(Needs confirmation, maybe some titles have different values?)
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 11) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Friendship: %d", pokemon->friendship());
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 13: Whether this Pokemon is this an egg
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 12) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            snprintf(buffer, sizeof(buffer), "Is Egg: %s", pokemon->isEgg() ? "Yes" : "No");
            fb.drawText(contentX, lineY, buffer, Colors::Text);
            lineY += lineHeight;

            // Field 14: Whether this Pokemon is infected, cured or has not been/is not infected with Pokerus
            if (screen.pokemonDetailsEditing && screen.pokemonDetailsSelectedField == 13) {
                fb.drawText(contentX - 15, lineY, ">", Colors::Yellow);
            }
            const char* pkrsStatus = pokemon->isPokerusInfected() ? "Infected" :
                pokemon->isPokerusCured() ? "Cured" : "None";
            snprintf(buffer, sizeof(buffer), "Pokerus: %s", pkrsStatus);
            fb.drawText(contentX, lineY, buffer, Colors::Text);

        } else if (screen.pokemonDetailsCategory == 2) { // Stats
            int lineY = contentY;
            int lineHeight = 25;

            fb.drawText(contentX, lineY, "=== Stats ===", Colors::Text);
            lineY += lineHeight + 5;

            // Header
            fb.drawText(contentX + 31, lineY, "Base | IV  | EV  | Stat", Colors::TextDim);
            lineY += lineHeight;

            // Calculate totals
            int ivTotal = pokemon->ivHP() + pokemon->ivATK() + pokemon->ivDEF() +
                pokemon->ivSPE() + pokemon->ivSPA() + pokemon->ivSPD();
            int evTotal = pokemon->evHP() + pokemon->evATK() + pokemon->evDEF() +
                pokemon->evSPE() + pokemon->evSPA() + pokemon->evSPD();
            int baseTotal = pokemon->baseHP() + pokemon->baseATK() + pokemon->baseDEF() +
                pokemon->baseSPE() + pokemon->baseSPA() + pokemon->baseSPD();
            int statTotal = pokemon->statHPMax() + pokemon->statATK() + pokemon->statDEF() +
                pokemon->statSPE() + pokemon->statSPA() + pokemon->statSPD();

            // Draw each stat
            const char* statNames[] = {"HP ", "ATK", "DEF", "SPA", "SPD", "SPE"};
            uint8_t baseStats[] = {pokemon->baseHP(), pokemon->baseATK(), pokemon->baseDEF(),
                pokemon->baseSPA(), pokemon->baseSPD(), pokemon->baseSPE()};
            uint8_t ivs[] = {pokemon->ivHP(), pokemon->ivATK(), pokemon->ivDEF(),
                pokemon->ivSPA(), pokemon->ivSPD(), pokemon->ivSPE()};
            uint8_t evs[] = {pokemon->evHP(), pokemon->evATK(), pokemon->evDEF(),
                pokemon->evSPA(), pokemon->evSPD(), pokemon->evSPE()};
            uint16_t stats[] = {pokemon->statHPMax(), pokemon->statATK(), pokemon->statDEF(),
                pokemon->statSPA(), pokemon->statSPD(), pokemon->statSPE()};

            for (int i = 0; i < 6; i++) {
                // Highlight if editing this stat
                if (screen.pokemonDetailsEditing && i == screen.pokemonDetailsSelectedStat) {
                    fb.drawFilledRect(contentX - 5, lineY - 2, contentWidth - 10, lineHeight - 2, Colors::Selected);
                }

                char statLine[128];
                snprintf(statLine, sizeof(statLine), "%s: %03d | %02d  | %03d | %03d",
                        statNames[i], baseStats[i], ivs[i], evs[i], stats[i]);

                Color textColor = (screen.pokemonDetailsEditing && i == screen.pokemonDetailsSelectedStat) ? Colors::Yellow : Colors::Text;
                fb.drawText(contentX, lineY, statLine, textColor);
                lineY += lineHeight;
            }

            // Show totals
            lineY += 5;
            char totalLine[128];
            snprintf(totalLine, sizeof(totalLine), "Tot: %03d | %03d | %03d | %03d", baseTotal, ivTotal, evTotal, statTotal);
            fb.drawText(contentX, lineY, totalLine, Colors::TextDim);

            lineY += lineHeight + 10;
            if (!screen.pokemonDetailsEditing) {
                fb.drawText(contentX, lineY, "Press A to edit EVs/IVs", Colors::TextDim);
            } else {
                fb.drawText(contentX, lineY, "Left/Right: +/-1  |  Up/Down: +/-10  |  A: Confirm  |  B: Cancel", Colors::TextDim);
            }

        } else { // Other categories
            int lineY = contentY;
            fb.drawText(contentX, lineY, categories[screen.pokemonDetailsCategory], Colors::Text);
            lineY += 40;
            fb.drawText(contentX, lineY, "Coming soon...", Colors::TextDim);
            lineY += 30;
            fb.drawText(contentX, lineY, "This category will be implemented in a future update.", Colors::TextDim);
        }

        // Draw footer with controls
        const char* controlText = screen.pokemonDetailsEditing
            ? "Up/Down: Select Field  |  B: Back"
            : "Up/Down: Select Category  |  B: Close";
        fb.drawText(modalX + 20, modalY + MODAL_HEIGHT - 30, controlText, Colors::TextDim);
    }
}
}